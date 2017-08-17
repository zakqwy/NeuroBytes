/*

State machine for switching through operating modes and controlling parameters.

TODO: add changing pin modes to HAL and change pin modes during inits.

*/
#include <avr/io.h>
#include <HAL.h>
//#include <RGB_PWM.h>

 void changeMode(neuron_t *n)
 {
	switch(n->mode){
		case IAF:
			IZHIK_init(n);
			n->mode = IZHIK;
			break;
		case IZHIK:
			MOTOR_init(n);
			n->mode = MOTOR;
			break;
		case MOTOR:
			n->mode = IAF;
			IAF_init(n);
			break;
	}
 }

 void IAF_init(neuron_t *n)
 {
	uint8_t i;

	disableServo();

	n->potential = 0;
	n->mode = IAF;
	n->state = INTEGRATE;

	n->fire_time = 0;

	for (i=0;i<DENDRITE_COUNT;i++){
		n->dendrites[i].state = OFF;
		n->dendrites[i].history = 0b00000000;
		n->dendrites[i].current_value = 0;
		n->dendrites[i].mode = DIGITAL;
		n->dendrites[i].type = EXCITATORY;
	}
 }

 void MOTOR_init(neuron_t *n)
 {
	enableServo();
	TCCR1A |= ((1<<COM1A1) | (1<<COM1B1)); //Fast PWM mode: clear OC1A/OC1B on Compare Match, set at TOP
	TCCR1A |= ((1<<WGM10) | (1<<WGM11)); //Fast PWM, 10-bit
	TCCR1B |= (1<<WGM12);
	TCCR1B |= (1<<CS12); //clk/256

	n->potential = 0;
	n->mode = MOTOR;
 }

 void IZHIK_init(neuron_t *n)
 {
	uint8_t i;
	disableServo();
	 /* set up ADC */
	 
	 PRR &= ~(1<<PRADC); //ensures power reduction bit is cleared
	 ADCSRA |= (1<<ADEN); //enables the ADC
	 ADCSRA |= (1<<ADSC); //start running ADC conversion (in free-running mode by default)
	 ADCSRA |= ((1<<ADPS2) | (1<<ADPS1)); //prescales ADC clock to clk/64 (125 kHz)
	ADCSRA |= (1<<ADATE); // set conversion auto-triggering
	 ADMUX |= (1<<ADLAR); //left adjusts ADC result
	 ADMUX |= (1<<REFS0); //ensures ADC references AVcc
	 //note: MUX[3:0] set to 0000 by default which selects AIN0
	 ADMUX = 0b01100000; // select ADC 0 => dendrite 0 excitatory

	 n->analog_in_flag = 0;

	ADCSRB &= 11111000; // set free-running mode
	n->fire_period	= 11;
	n->mode = IZHIK;
	n->scaling = 1;
	n->state = REST;
 }

 void changeHebb(neuron_t *n)
 {
	if (n->learning_state == NONE){
		translateColor(0, MOTOR);
		n->learning_state = HEBB;
		HEBB_init(n);
	} else{
		n->learning_state = NONE;
		neuronInit(n);
		//led_resolution = 156;
	}
 }

 void changeADC(neuron_t *n)
 {
		 //ADMUX = 0b01100000; // select ADC 0 => dendrite 0 excitatory
		 //ADMUX = 0b01100010; // select ADC 2 => dendrite 1 excitatory
		 //ADMUX = 0b01100011; // select ADC 3 => dendrite 2 excitatory
		 //ADMUX = 0b01100100; // select ADC 4 => dendrite 3 excitatory
		 //ADMUX = 0b01100101; // select ADC 5 => dendrite 4 excitatory
	switch(ADMUX){

		case 0b01100000:
			ADMUX = 0b1100010;
			break;

		case 0b01100010:
			ADMUX = 0b01100011;
			break;

		case 0b01100011:
			ADMUX = 0b01100100;
			break;

		case 0b01100100:
			ADMUX = 0b01100101;
			break;

		case 0b01100101:
			ADMUX = 0b01100000;
			break;
	}
 }

 void HEBB_init(neuron_t *n)
 {
	n->hebb_time = 0;
 }