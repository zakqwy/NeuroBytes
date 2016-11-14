/*
	NeuroBytes v091 run program
	v091_run-iaf/main.c
	Integrate-and-fire runtime program similar to that implemented in v04.	
	Zach Fredin
	6/11/2016
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

/*
	FRONT VIEW (connectors on BACK):
	   _
 	_-- \
	\ D4 \_----______________________
	 \      D5     MODE    ADJUST    \
	 _\                            A2 \
	|                NEURO            |
	| D3     LED    TINKER            |
	|                              A1 /
	 \ D2  _ D1 _____________________/
	  \  _-	----   
	   \-

	DEND1-SIG	PC0/ADC0	DEND1-TYPE	PC1
	DEND2-SIG	PC2/ADC2	DEND2-TYPE	PD0
	DEND3-SIG	PC3/ADC3	DEND3-TYPE	PD1
	DEND4-SIG	PC4/ADC4	DEND4-TYPE	PB7
	DEND5-SIG	PC5/ADC5	DEND5-TYPE	PD5

	AXON1-SIG	PB2/OC1B	AXON1-TYPE	PB3
	AXON2-SIG	PB1/OC1A	AXON2-TYPE	PB0

	LED-R		PD2
	LED-G		PD3
	LED-B		PD4

	MODE		PD6
	ADJUST		PD7
*/

/* neuron variables */
volatile uint16_t n_var_v; //current membrane potential value
volatile uint16_t n_var_v_temp; //temp used to stage calculation
const uint16_t n_con_v_rest = 32767; //membrane potential rest value, usually 32767
const uint16_t n_con_threshold = 32867; //action potential threshold value, usually 32867
volatile uint8_t n_var_mode = 0; //0 = IAF, 1 = motor
volatile uint16_t n_var_mode_count = 0;
const uint16_t n_con_mode_count_reset = 3000;
volatile uint8_t n_var_mode_armed = 1; 	
volatile uint8_t n_var_v_delay_newest;
volatile uint8_t n_var_v_delay_oldest;
volatile uint16_t n_var_v_delay[60]; //delay buffer
volatile uint8_t n_var_v_delay_mult;
volatile uint8_t n_var_v_delay_mult_reset = 8;

/* dendrite variables */
volatile uint8_t d_var_status[5]; //current and previous dendrite status, LSB = most recent value
volatile uint8_t d_var_type; //current type xxxTTTTT, where TTTTT represents the TYPE pins of the dendrites, LSB = DEND1, high = exc
volatile uint8_t d_var_selected; //selected dendrite (used for staging algorithm)
volatile uint16_t d_var_val[5]; //sensed membrane potential contribution +/- 32767
volatile uint16_t d_var_decay[5] = {32767,32767,32767,32767,32767}; //stored membrane potential decay contribution +/- 32767
const uint16_t d_con_mag_lookup[5] = {130,60,70,80,110}; //input magnitudes

/* LED variables */
volatile uint8_t l_var_val[3]; //current LED value, R/G/B, 0 to [LED_con_resolution]
volatile uint8_t l_var_tick = 0; //current LED tick, resets when equal to LED_con_resolution
const uint8_t l_con_resolution = 255; //LED fading resolution, flicker rate = 1 / (L_con_resolution * 0.00004) Hz

/* servo variables */
volatile uint16_t s_var_val = 0;
volatile uint16_t s_var_val_count = 48;
const uint16_t s_con_val_reset = 1000;

/* timing variables */
volatile uint8_t t_var_tick = 0; //ISR flips this to non-zero to update main loop every 40 us
volatile uint8_t t_var_timer = 0; //slow loop counter
volatile uint8_t t_var_timer_reset = 20; //slow counter reset in ticks
volatile uint8_t t_var_decay = 0;
volatile uint8_t t_var_decay_reset = 40;
volatile uint8_t t_var_fire = 50;
volatile uint8_t t_var_fire_reset = 50;
volatile uint16_t t_var_axon_delay = 0;
const uint16_t t_con_axon_delay_reset = 500;
volatile uint8_t t_var_axon_pulse = 0;
const uint8_t t_con_axon_pulse_reset = 50;

void getDendriteStatus(void) {
/*	implementation of Elliot Williams' debounce strategy. shifts previous dendrite status history and stores latest value
	in d_var_status[] LSB. */
	uint8_t dend;
	for (dend=0;dend<5;dend++) {
		d_var_status[dend] <<= 1;
	}
	d_var_status[0] |= (PINC & (1<<PC0));
	d_var_status[1] |= ((PINC & (1<<PC2)) >> 2);
	d_var_status[2] |= ((PINC & (1<<PC3)) >> 3);
	d_var_status[3] |= ((PINC & (1<<PC4)) >> 4);
	d_var_status[4] |= ((PINC & (1<<PC5)) >> 5);
}

void getDendriteType(void) {
// updates d_var_type to xxxTTTTT, where TTTTT represents the input _type_ of the five dendrites (LSB = DEND, exc = 0, inh = 1)
	uint8_t temp = d_var_type;
	if (n_var_v >= n_con_v_rest) {
		temp = 0;
	}
	temp |= ((PINC & (1<<PC1)) >> 1);
	temp |= ((PIND & (1<<PD0)) << 1);
	temp |= ((PIND & (1<<PD1)) << 1);
	temp |= ((PINB & (1<<PB7)) >> 4);
	temp |= ((PIND & (1<<PD5)) >> 1);
	d_var_type = temp;
}

void calcDend(uint8_t dend) {
/*	updates the dendrite value (d_var_val) and dendrite decay (d_var_dec) arrays based on input and history. 
	staged using [dend] argument. uses 16-bit integers so interrupts should be disabled prior to calling. */
	d_var_val[dend] = 32767;
	if (d_var_status[dend] & 0b00000111) {
		if (d_var_type & (1<<dend)) {
			d_var_val[dend] -= d_con_mag_lookup[dend];
		}
		else {
			d_var_val[dend] += d_con_mag_lookup[dend];
		}
	}
	if ((d_var_status[dend] & 0b11000111) == 0b11000000) {
		d_var_status[dend] = 0;
		if (d_var_type & (1<<dend)) {
			d_var_decay[dend] -= d_con_mag_lookup[dend];
		}
		else {
			d_var_decay[dend] += d_con_mag_lookup[dend];
		}
	}
}

void decayDend(uint8_t dend) {
/* decays d_var_dec by d_con_dec when called. staged with [dend]. checks input values to avoid overflows. 16 bit integers so 
disable interrupts prior to calling. */
	if (d_var_decay[dend] > 32767) {
		if (t_var_decay > t_var_decay_reset) {
			t_var_decay = 0;
			d_var_decay[dend] = 32767 + (((d_var_decay[dend] - 32767) * 63) >> 6);
			if (d_var_decay[dend] < 32767) {
				d_var_decay[dend] = 32767;
			}
		}
	}
	else if (d_var_decay[dend] < 32767) {
		if (t_var_decay > t_var_decay_reset) {
			t_var_decay = 0;
			d_var_decay[dend] = 32767 - (((32767 - d_var_decay[dend]) * 63) >> 6); 
			if (d_var_decay[dend] > 32767) {
				d_var_decay[dend] = 32767;
			}
		}
	}	

	t_var_decay++;
}

void calcV(uint8_t dend) {
/* 	updates v with fresh d_var_val and d_var_dec values. 16 bit integers so disable interrupts prior to calling.
	staged to control execution time. */
	if (dend == 0) {
		n_var_v_temp = n_con_v_rest;
	}

	if (d_var_val[dend] >= 32767) {
		n_var_v_temp += (d_var_val[dend] - 32767);
	}
	else {
		n_var_v_temp -= (32767 - d_var_val[dend]);
	}

	if (d_var_decay[dend] >= 32767) {
		n_var_v_temp += (d_var_decay[dend] - 32767);
	}
	else {
		n_var_v_temp -= (32767 - d_var_decay[dend]);
	}

	if (dend == 4) {
		n_var_v = n_var_v_temp;
	}

}

void bufferV(void) {
/*	used to introduce delays into v calculation, specifically for excitatory motor application (so inhibitory
	and excitatory pulses arrive simultaneously to opposing servos). delay time is based on buffer size, which
	is _severely_ limited by on-board SRAM! should only be called if no inputs are inhibitory. */
	if (n_var_v_delay_mult == n_var_v_delay_mult_reset) {
		n_var_v_delay_newest++;
		n_var_v_delay_mult = 0;
	}
	if (n_var_v_delay_newest > 59) {
		n_var_v_delay_newest = 0;
	}
	n_var_v_delay[n_var_v_delay_newest] = n_var_v;

	n_var_v_delay_oldest = n_var_v_delay_newest + 1;
	if (n_var_v_delay_oldest > 59) {
		n_var_v_delay_oldest = 0;
	}
	n_var_v = n_var_v_delay[n_var_v_delay_oldest];
	n_var_v_delay_mult++;
}

void translateColor() {
/* 	translates membrane potential (v) values into the RGB array. 16 bit so disable interrupts prior to calling.
	v < 32667		pure blue
	32667 <= v < 32767	fade blue to green
	v = 32767		pure green
	32767 < v <= 32867	fade green to red
	v > 32867		pure red and firing (bright white) */
	uint8_t temp;
	if((n_var_v < 32667)) {
		l_var_val[0] = 0;
		l_var_val[1] = 0;
		l_var_val[2] = 24;
	}
	else if ((n_var_v >= 32667) && (n_var_v < 32767)) {
		temp = n_var_v - 32667;
		l_var_val[0] = 0;
		l_var_val[1] = temp >> 2;
		l_var_val[2] = 24 - (temp >> 2);
	}
	else if (n_var_v == 32767) {
		l_var_val[0] = 0;
		l_var_val[1] = 24;
		l_var_val[2] = 0;
	}
	else if ((n_var_v > 32767) && (n_var_v <= 32866)) {
		temp = n_var_v - 32767;
		l_var_val[0] = temp >> 2;
		l_var_val[1] = 24 - (temp >> 2);
		l_var_val[2] = 0;
	}
	else if (n_var_v > 32866) {
		l_var_val[0] = 24;
		l_var_val[1] = 0;
		l_var_val[2] = 0;
	}
}

void updateLED(void) { 
/* 	Updates the RGB LED based on the current membrane potential value. Uses PWM fading and global variables to
	figure out when the various elements should be on. Resolution controlled by LEDres. 
	Execution time: 5.6 us */
	if(l_var_val[0] > l_var_tick) { //Red
		PORTD &= ~(1<<PD2);
	}
	else {
		PORTD |= (1<<PD2);
	}
	if(l_var_val[1] > l_var_tick) { //Green
		PORTD &= ~(1<<PD3);
	}
	else {
		PORTD |= (1<<PD3);
	}
	if(l_var_val[2] > l_var_tick) { //Blue
		PORTD &= ~(1<<PD4);
	}
	else {
		PORTD |= (1<<PD4);
	}
	l_var_tick++;	 
	if (l_var_tick > l_con_resolution) {
		l_var_tick = 0;
	}	
}

void systemInit(void) {
/* set up LED */
	DDRD |= ((1<<PD2) | (1<<PD3) | (1<<PD4));
	PORTD |= ((1<<PD2) | (1<<PD3) | (1<<PD4)); //set pins = LED off 

/* set up dendrites */
	DDRC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5));
	DDRD &= ~((1<<PD0) | (1<<PD1) | (1<<PD5));
	DDRB &= ~(1<<PB7);

/* set up axons */
	DDRB |= ((1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3));
	PORTB |= ((1<<PB0) | (1<<PB3));

/* set up mode switch */
	DDRD &= ~(1<<PD7);

/* set up adjust switch */
	DDRD &= ~(1<<PD6);

/* set up Timer/Counter1: OC1A is PB1 (A2), OC1B is PB2 (A1) */
	TCCR1A |= ((1<<COM1A1) | (1<<COM1B1)); //Fast PWM mode: clear OC1A/OC1B on Compare Match, set at TOP
	TCCR1A |= ((1<<WGM10) | (1<<WGM11)); //Fast PWM, 10-bit
	TCCR1B |= (1<<WGM12);
	TCCR1B |= (1<<CS12); //clk/256
	OCR1BL = 48; //servo range: 32 (1ms) - 63 (2ms)	
}

int main(void) {
	systemInit();
	uint8_t i;
	
	for (i=0;i<60;i++) {
		n_var_v_delay[i] = 32767;
	}

	for(;;) {
		if (n_var_mode == 0) { //IAF mode
			TCCR1A = 0;
			TCCR1B = 0; //stop Timer/Counter1 (used for servo stuff only)
			t_var_timer_reset = 20;
			if (t_var_timer == 0) { // 16.0 us
				getDendriteStatus();
			}
			else if (t_var_timer == 1) { // 9.8 us
				getDendriteType();
			}
			else if ((t_var_timer >= 2) && (t_var_timer < 7)) { // 11.7 us
				calcDend(t_var_timer - 2);
			}
			else if ((t_var_timer >= 7) && (t_var_timer < 12)) { // 7.7 us
				decayDend(t_var_timer - 7);
			}
			else if ((t_var_timer >= 12) && (t_var_timer < 17)) { // 14.0 us
				calcV(t_var_timer - 12);
			}
			else if (t_var_timer == 17) {
				if (n_var_v > 32867) {
					t_var_fire = 0;
					t_var_decay_reset = 60;
					t_var_axon_delay = t_con_axon_delay_reset;
					for (i=0;i<5;i++) {
						d_var_decay[i] = 32724;
					}
				}
				else if (t_var_fire < t_var_fire_reset) {	
					t_var_fire++;
				}
			}
			else if (t_var_timer == 18) {
				if (t_var_axon_delay > 1) {
					t_var_axon_delay--;
				}
				else if (t_var_axon_delay == 1) {
					t_var_axon_delay = 0;
					t_var_axon_pulse = t_con_axon_pulse_reset;
				}

				if (t_var_axon_pulse > 0) {
					t_var_axon_pulse--;
					PORTB |= ((1<<PB1) | (1<<PB2));
				}
				else {
					PORTB &= ~(1<<PB1);
					PORTB &= ~(1<<PB2);
				}
			}		
			else if (t_var_timer == 19) { // 9.1 us
				translateColor();
			}
			else if (t_var_timer == 20) {
				if ((PIND & (1<<PD6)) && (n_var_mode_armed == 1)) {
					n_var_mode_count++;
				}
				else {
					n_var_mode_count = 0;
				}
				
				if (n_var_mode_count == n_con_mode_count_reset) {
					n_var_mode = 1;
					n_var_mode_armed = 0;
				}
	
				if ((PIND & (1<<PD6)) == 0) {
					n_var_mode_armed = 1;
				}
			}
		}
		else if (n_var_mode == 1) { //motor mode
			TCCR1A |= ((1<<COM1A1) | (1<<COM1B1) | (1<<WGM11) | (1<<WGM10));
			TCCR1B |= ((1<<CS12) | (1<<WGM12));	
	
			if (t_var_timer == 0) {
				getDendriteStatus();
			}
			else if (t_var_timer == 1) {
				getDendriteType();
			}
			else if ((t_var_timer >= 2) && (t_var_timer < 7)) {
				calcDend(t_var_timer - 2);		
			}
			else if ((t_var_timer >= 7) && (t_var_timer < 12)) {
				decayDend(t_var_timer - 7);	
			}	
			else if ((t_var_timer >= 12) && (t_var_timer < 17)) {
				calcV(t_var_timer - 12);
			}
			else if ((t_var_timer == 17) && (d_var_type == 0)) {
				bufferV();
			}
			else if (t_var_timer == 18) {
				if (n_var_v > 32895) {
					OCR1AL = 80;
					OCR1BL = 80;
				}
				else if (n_var_v < 32648) {
					OCR1AL = 15;
					OCR1BL = 15;
				}
				else {
					OCR1AL = ((n_var_v - 32640) >> 2) + 15;
					OCR1BL = ((n_var_v - 32640) >> 2) + 15;
				}
			}
			else if (t_var_timer == 19) {
				if (n_var_v > 32767) {
					l_var_val[0] = 24;
					l_var_val[1] = 0;
					l_var_val[2] = 0;
				}
				else if (n_var_v < 32767) {
					l_var_val[0] = 0;
					l_var_val[1] = 0;
					l_var_val[2] = 24;
				}
				else {
					l_var_val[0] = 24;
					l_var_val[1] = 0;
					l_var_val[2] = 24;
				}
			}
			else if (t_var_timer == 20) {
				if ((PIND & (1<<PD6)) && (n_var_mode_armed == 1)) {
					if (n_var_mode_armed == 1) {
						n_var_mode_count++;
					}
				}
				else {
					n_var_mode_count = 0;
				}
			
				if (n_var_mode_count == n_con_mode_count_reset) {
					n_var_mode = 0;
					n_var_mode_armed = 0;
				}

				if ((PIND & (1<<PD6)) == 0) {
					n_var_mode_armed = 1;
				}
			}	
		
		}
		t_var_timer++;
		if (t_var_timer > t_var_timer_reset) {
			t_var_timer = 0;
		}
	
		if (t_var_fire < t_var_fire_reset) {
			PORTD &= ~((1<<PD2) | (1<<PD3) | (1<<PD4));
		}
		else {
			updateLED(); // 5.3 us
		
		}
	}
}
