/*
	NeuroBytes v09 test program
	Lights up RGB LED
	Flashes at rate controlled by pot 
	Zach Fredin
	5/31/2016
*/

#include <avr/io.h>
#include <avr/interrupt.h>

/*
	FRONT VIEW (connectors on BACK):
	   _
 	_-- \
	\ D4 \_----______________________
	 \      D5     MODE     POT      \
	 _\                            A2 \
	|                NEURO            |
	| D3     LED    TINKER            |
	|                              A1 /
	 \ D2  _ D1 _____________________/
	  \  _-	----   
	   \-

	DEND1-SIG	PC0		DEND1-TYPE	PC1
	DEND2-SIG	PC3		DEND2-TYPE	PC2
	DEND3-SIG	PC5		DEND3-TYPE	PC4
	DEND4-SIG	PD0		DEND4-TYPE	PD1
	DEND5-SIG	PD5		DEND5-TYPE	PB7

	AXON1-SIG	PB2/OC1B	AXON1-TYPE	PB3
	AXON2-SIG	PB1/OC1A	AXON2-TYPE	PB0

	LED-R		PD2
	LED-G		PD3
	LED-B		PD4

	MODE		PD7
	POT		PD6/AIN0
*/

void systemInit(void) {
/* set up LED */
	DDRD |= ((1<<PD2) | (1<<PD3) | (1<<PD4));
	PORTD &= ~((1<<PD2) | (1<<PD3) | (1<<PD4));

/* set up ADC */
	DDRD &= ~(1<<PD6);
	PRR &= ~(1<<PRADC); //ensures power reduction bit is cleared
	ADCSRA |= (1<<ADEN); //enables the ADC
	ADCSRA |= ((1<<ADPS2) | (1<<ADPS1)); //prescales ADC clock to clk/64 (125 kHz)
	ADCSRA |= (1<<ADSC); //start running ADC conversion (in free-running mode by default)
	ADMUX |= (1<<ADLAR); //left adjusts ADC result
	ADMUX |= (1<<REFS0); //ensures ADC references AVcc
	//note: MUX[3:0] set to 0000 by default which selects AIN0

}

int main(void) {
	systemInit();
	for(;;) {
		if (ADCH > 127) {
			PORTD |= ((1<<PD2) | (1<<PD3) | (1<<PD4));
		}
		else {
			PORTD &= ~((1<<PD2) | (1<<PD3) | (1<<PD4));
		}
	}
}
