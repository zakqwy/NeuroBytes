/*
    NeuroBytes v1.0 firmware

    Contributors:
    Zach Fredin
    Patrick Sullivan
    Jarod White

    1/7/2017
    
    
                    `.                                                                               
                   +                                                                                
                  .o                                                                                
               `-.+y               `                                                                
                `/yh.        .-  -:`                                                                
                   dm:        `sds         :++.  `++.  :+oo+-  -++` .++. -++-/+: -/+o+:            
               `/hmddd/        `d.`:.      /sso. .ss- +so::ss/ :ss` .ss- :sso/:.:ss/:oso            
              :s-`  `:yh.       dyo`       /ssss-.ss- ss/  +so :ss` .ss- :ss.   +so  :ss`           
             -:        /do`    -ys//-      /so:ss+ss- ssssssso :ss` .ss- :ss`   +so  :ss`           
                        dmdo--/do          /so :ssss- ss/  ``` :ss` .ss- :ss`   +so  :ss`           
         .             `hmNmhhmNh:`        /so  -sss- +so::ss/ :ss+:+ss- :ss`   :ss/:oso           
          -/o-.`     `:shhy..:`:hmds/:-`   :+/   -++.  :+++/-   :++/:++. -++`    -/+++:`         
         `-+hddhyhhhmmdmd/`+dmh-.smdso+++//:::::::::::::::::::::::::::////////////////////////:::   
       `-.` ys.   `/dmNdy: sNNh:`hNo       .%%%%%%..%%%%%%..%%..%%..%%..%%..%%%%%%..%%%%%..   
           .o      /hdddyso`-/`:syNm-      ...%%......%%....%%%.%%..%%.%%...%%......%%..%%.         
                 :hs/.``-hdhmNyhNmhdm+     ...%%......%%....%%.%%%..%%%%....%%%%....%%%%%..         
               -+:`      +shmy:` `+mhho`   ...%%......%%....%%..%%..%%.%%...%%......%%..%%.         
        .-..-oh/`       /dh/`      dm--//   ..%%....%%%%%%..%%..%%..%%..%%..%%%%%%..%%..%%.         
         -:/oho        od:        oNN:        -:.   .:- -:-  -:. -:.  .:- `-:::-`  ::`              
             `s`     `hss`      .:. ++                                                             
              `-     h: `/`         -+                                                              
                    :/              .:                                                              
                    +               .                                                               
                    -               
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <neuron.h>
#include <HAL.h>
#include <RGB_PWM.h>
#include <bit_ops.h>
#include <states.h>

//#include <serial.h>

// Timing Macros
#define FIRE_RESET_TIME         200
#define AXON_PULSE_DELAY_TIME   90
#define AXON_PULSE_LENGTH       100
#define FIRE_LED_TIME           50
#define DECAY_TIME_RESET        8    //8

#define MODE_BUTTON_HOLD_TIME   500

int main(void)
{
	dendrite_magnitude_lookup[0] = 130;
	dendrite_magnitude_lookup[1] = 30;
	dendrite_magnitude_lookup[2] = 60;
	dendrite_magnitude_lookup[3] = 80;
	dendrite_magnitude_lookup[4] = 120;

    neuron_t neuron;

    uint16_t mode_button_count = 0;
	uint16_t adjust_button_count = 0;
	uint16_t decay_time = 0;
	uint16_t adbg = 0; // debug
	uint8_t	 led_breathing = 0;
	uint8_t	 breathing_count = 0;
	uint16_t	 flash_count = 0;
	uint16_t inhib_flag = 0;
	uint16_t last_fire_period = 0;
	volatile uint8_t n_var_v_delay_newest = 0;
	volatile uint8_t n_var_v_delay_oldest;
	volatile int16_t n_var_v_delay[10] = {0,0,0,0,0,0,0,0,0,0}; //delay buffer
	volatile uint8_t n_var_v_delay_mult;
	volatile uint8_t n_var_v_delay_mult_reset = 8;


    systemInit();
    setupTimer();

    neuronInit(&neuron);

    for(;;){
		if (ms_tick != 0){
			ms_tick = 0;
			// check SPI for updated values
			/*
				SPI can:
				-change dendrite weightings and modes
				-change operating mode
				-change neuron parameters
			*/
			// send data to SPI
			// check mode button
			if (mode_button_pressed){
				mode_button_count++;
			} else{
				mode_button_count = 0;
			}

			if (adjust_button_pressed){
				adjust_button_count++;
			} else{
				adjust_button_count = 0;
			}
		
			if (mode_button_count == MODE_BUTTON_HOLD_TIME){
				changeMode(&neuron);
			}

			if (adjust_button_count == MODE_BUTTON_HOLD_TIME){
				changeHebb(&neuron);
			}
		
			//disableServo();
			switch(neuron.mode){
            
			case IAF:
				disableServo(); //debug
				// check dendrites
				checkDendrites(&neuron);
				// decay dendrites
				if (++decay_time > DECAY_TIME_RESET){
					decay_time = 0;
					dendriteDecayStep(neuron.dendrites);
					membraneDecayStep(&neuron);
				}
				switch(neuron.state){ // is the neuron integrating or firing?
            
					case INTEGRATE:
						
						// calculate new membrane potential and see if the neuron fires
						neuron.potential = neuron.fire_potential + calcNeuronPotential(&neuron);
						neuron.hebb_time++;
						if (neuron.potential >= MEMBRANE_THRESHOLD){
							neuron.state = FIRE;
							neuron.fire_time = 0;
							neuron.fire_potential = HYPERPOLARIZATION;
							for (uint8_t i=0;i<5;i++){ // TODO: clear dendrites function
								neuron.dendrites[i].current_value = 0;
								neuron.dendrites[i].state = OFF;
								if (neuron.learning_state == HEBB){
									if (neuron.dendrites[i].timestamp < (neuron.hebb_time / 10)){
										neuron.dendrites[i].magnitude += (neuron.dendrites[i].magnitude * ((neuron.hebb_time / 10) - neuron.dendrites[i].timestamp) / (neuron.hebb_time / 10)) / 2;
										dendrite_magnitude_lookup[i] = neuron.dendrites[i].magnitude; // preserve dendritic weightings after switching hebb mode
									}
									if (neuron.dendrites[i].magnitude > 130){
										neuron.dendrites[i].magnitude = 130;
										dendrite_magnitude_lookup[i] = neuron.dendrites[i].magnitude;
									}
									neuron.dendrites[i].timestamp = 0;
								}
							}
							neuron.hebb_time = 0;

						}
						//translateColor(neuron.potential, IAF); // translate membrane potential to LED RGB values 
						if (neuron.potential == 0){
							if (neuron.learning_state == HEBB){
								if (++flash_count > 1000 && flash_count < 1100){
									translateColor(0, MOTOR);
								} else if (flash_count >= 1300 && flash_count < 1400){
									translateColor(0, MOTOR);
								} else if(flash_count >= 1400){
									flash_count = 0;
								}else{
									translateColor(neuron.potential, IAF);
								}
							} else{
							translateColor(neuron.potential, IAF);
							}
						}else{
						translateColor(neuron.potential, IAF);
						}
						
							/*
							if (++breathing_count > 4){
								//led_breathing = 0;
								if (led_breathing == 0){
									led_resolution++;
									if (led_resolution > 400){
										led_resolution = 400;
										led_breathing = 1;
									}
								} else if (led_breathing == 1){
									led_resolution--;
									if (led_resolution < 24){
										led_resolution = 24;
										led_breathing = 0;
									}
								}
								breathing_count = 0;
							}
							*/
						break;
                    
					case FIRE:
						neuron.fire_time++;
						neuron.potential = neuron.fire_potential + calcNeuronPotential(&neuron);
						if (neuron.fire_time > FIRE_RESET_TIME){ // neuron done firing
							neuron.state = INTEGRATE;
						} 
						else if (neuron.fire_time > AXON_PULSE_DELAY_TIME && neuron.fire_time - AXON_PULSE_DELAY_TIME < AXON_PULSE_LENGTH){
							// output pulse through axon after a delay
							setOutputPin(PIN_AXON_1);
							setOutputPin(PIN_AXON_2);
							PORTB |= ((1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3));
						} 
						else{
							clearOutputPin(PIN_AXON_1);
							clearOutputPin(PIN_AXON_2);
							PORTB &= ~((1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3));
						}

						if (neuron.fire_time < FIRE_LED_TIME){
							LEDfullWhite(); // make LED full white during fire time
						} else {
							translateColor(neuron.potential, IAF);
						}
						break;
				}
				break;

			case MOTOR:
				TCCR1A |= ((1<<COM1A1) | (1<<COM1B1)); //Fast PWM mode: clear OC1A/OC1B on Compare Match, set at TOP
				TCCR1A |= ((1<<WGM10) | (1<<WGM11)); //Fast PWM, 10-bit
				TCCR1B |= (1<<WGM12);
				TCCR1B |= (1<<CS12); //clk/256
				// calculate potential, buffer
				checkDendrites(&neuron);
				if (++decay_time > DECAY_TIME_RESET){
					decay_time = 0;
					dendriteDecayStep(neuron.dendrites);
					membraneDecayStep(&neuron);
				}

				neuron.potential = calcNeuronPotential(&neuron);
				inhib_flag = 0;
				for (uint8_t i=0; i<5; i++){
					if (neuron.dendrites[i].type == INHIBITORY){
						inhib_flag = 1;
					}
				}
				if (inhib_flag == 0){
					if (n_var_v_delay_mult == n_var_v_delay_mult_reset) {
						n_var_v_delay_newest++;
						n_var_v_delay_mult = 0;
					}
					if (n_var_v_delay_newest > 9) {
						n_var_v_delay_newest = 0;
					}
					n_var_v_delay[n_var_v_delay_newest] = neuron.potential;

					n_var_v_delay_oldest = n_var_v_delay_newest + 1;
					if (n_var_v_delay_oldest > 9) {
						n_var_v_delay_oldest = 0;
					}
					neuron.potential = n_var_v_delay[n_var_v_delay_oldest];
					n_var_v_delay_mult++;
				}
            
				// axon PWM
				if (neuron.potential >= 120) {
					OCR1BL = 80;
					OCR1AL = 80;
				} else if (neuron.potential <= -120) {
					OCR1BL = 20;
					OCR1AL = 20;
				} else {
					OCR1BL =(neuron.potential / 4) + 50;
					OCR1AL =(neuron.potential / 4 ) + 50;
				}
				translateColor(neuron.potential, MOTOR);
				break;

			case IZHIK:
				disableServo(); //debug
				if (++decay_time > DECAY_TIME_RESET){
					decay_time = 0;
					membraneDecayStep(&neuron);
				}
				//translateColor(ADCH * neuron.scaling, IZHIK);
				translateColor(adbg, IZHIK);
				switch(neuron.state){
					case REST:
					
						if (--neuron.fire_period == 10){
							//neuron.fire_period = ADCH * neuron.scaling;
							adbg = ADCH;
							if (adbg == 0){
								changeADC(&neuron);
								if (neuron.analog_in_flag == 0){
									neuron.fire_period = 11;
								} else{
									neuron.fire_period = last_fire_period;
									neuron.state = FIRE;
									neuron.fire_time = 0;
									neuron.fire_potential = -50;
								}
							}else{
								neuron.analog_in_flag = 1;
								adbg = adbg * 6 + 30;
								neuron.fire_period = adbg;
								last_fire_period = neuron.fire_period;
								neuron.state = FIRE;
								neuron.fire_time = 0;
								neuron.fire_potential = -50;
							}
							//neuron.fire_period = 1000; //debug
							//neuron.potential = MEMBRANE_THRESHOLD;
						} else if (neuron.fire_period < 0){
							neuron.fire_period = ADCH * neuron.scaling;
						}
						neuron.potential = 0;
						//translateColor(neuron.potential, IZHIK); // translate membrane potential to LED RGB values
						break;
					
					case FIRE:
						neuron.fire_time++;
						neuron.potential = neuron.fire_potential;
						if (neuron.fire_time > FIRE_RESET_TIME){ // neuron done firing
							neuron.state = REST;
						}
						else if (neuron.fire_time > AXON_PULSE_DELAY_TIME && neuron.fire_time - AXON_PULSE_DELAY_TIME < AXON_PULSE_LENGTH){
							// output pulse through axon after a delay
							setOutputPin(PIN_AXON_1);
							setOutputPin(PIN_AXON_2);
						}
						else{
							clearOutputPin(PIN_AXON_1);
							clearOutputPin(PIN_AXON_2);
						}

						if (neuron.fire_time < FIRE_LED_TIME){
							LEDfullWhite(); // make LED full white during fire time
							} else {
							//translateColor(neuron.potential, IZHIK);
						}
						break;
				}
				break;
			}
			
		}
	}
}
