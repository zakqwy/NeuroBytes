#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <HAL.h>
#include <bit_ops.h>
#include <neuron.h>

volatile unsigned char ms_tick;
static unsigned char timer_tick = 0;


void systemInit(void)
{
	// TODO: use bit ops here
	/* set up LED */
	DDRD |= ((1<<PD2) | (1<<PD3) | (1<<PD4));
	PORTD |= ((1<<PD2) | (1<<PD3) | (1<<PD4)); //set pins = LED off

	/* set up dendrites */
	DDRC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5));
	DDRD &= ~((1<<PD0) | (1<<PD1) | (1<<PD5));
	DDRB &= ~(1<<PB7);

	/* set up axons */
	DDRB |= ((1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3));
	//PORTB &= ~((1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3));

	/* set up mode switch */
	DDRD &= ~(1<<PD6);

	/* set up adjust switch */
	DDRD &= ~(1<<PD7);


	// set up timer/counter1: OC1B is PB2
	TCCR1A |= ((1<<COM1A1) | (1<<COM1B1)); //Fast PWM mode: clear OC1B on Compare Match, set at TOP
	TCCR1A |= ((1<<WGM10) | (1<<WGM11)); //Fast PWM, 10-bit

	OCR1BL = 48; //servo range: 32 (1ms) - 63 (2ms)

	// set up SPI in slave mode
	/*
	DDRB = (1<<PB5);
	SPCR = (1<<SPE);
	
	bit_clear(PRR, BIT(2)); // tell the power reduction register not to turn off SPI
	*/
}

void setupTimer(void)
{
	// write timer registers
	// set timer to interrupt at 20 kHz
	
	TCCR0A = 0x0A;	//  Set CTC0, and CS01 for clear timer on compare mode
	//	and divide by 8 prescaler
	TIMSK0 = 0x02;	//	Set OCIE0A to enable Compare A interrupt
	TIMSK1 = 0;
	OCR0A  = 49;	//	Set Compare A register for 49 so interrupt triggers
	//  every 50 ticks of prescaled clock
	
	//test these:
	sei(); // enable interrupts
}

unsigned char readDendritePin(unsigned char pin_number, dendrite_types dendrite_type)
{
	switch(pin_number) {
		case 0:
		switch(dendrite_type) {
			case EXCITATORY:
				return bit_get(PINC,BIT0_MASK) ? 1 : 0;
			case INHIBITORY:
				return bit_get(PINC,BIT1_MASK) ? 1 : 0;
		}
		case 1:
		switch(dendrite_type) {
			case EXCITATORY:
				return bit_get(PINC,BIT2_MASK) ? 1 : 0;
			case INHIBITORY:
				return bit_get(PIND,BIT0_MASK) ? 1 : 0;
		}
		case 2:
		switch(dendrite_type) {
			case EXCITATORY:
				return bit_get(PINC,BIT3_MASK) ? 1 : 0;
			case INHIBITORY:
				return bit_get(PIND,BIT1_MASK) ? 1 : 0;
		}
		case 3:
		switch(dendrite_type) {
			case EXCITATORY:
				return bit_get(PINC,BIT4_MASK) ? 1 : 0;
			case INHIBITORY:
				return bit_get(PINB,BIT7_MASK) ? 1 : 0;
		}
		case 4:
		switch(dendrite_type) {
			case EXCITATORY:
				return bit_get(PINC,BIT5_MASK) ? 1 : 0;
			case INHIBITORY:
				return bit_get(PIND,BIT5_MASK) ? 1 : 0;
		}
	}
	return 0;
}

static inline void timeInterruptRoutine(void)
{	
	if(++timer_tick > 20) { 
		timer_tick = 0;
		ms_tick = 1;
	}
	updateLED();
}

inline void setOutputPin(unsigned char pin_number)
{
	/*
		TODO: separate this into setAxonPin(axon_num) and setLED(led_color)

		this is a super dumb abstraction right now with a pin_number lookup table
	*/
	switch(pin_number) {
		case 0:
		bit_set(PORTD,BIT2_MASK);
		break;
		case 1:
		bit_set(PORTD,BIT3_MASK);
		break;
		case 2:
		bit_set(PORTD,BIT4_MASK);
		break;
		case 3:
		bit_set(PORTB,BIT3_MASK);
		break;
		case 4:
		bit_set(PORTB,BIT2_MASK);
		bit_set(PORTB,BIT3_MASK);
		break;
		case 5:
		bit_set(PORTB,BIT1_MASK);
		break;
		case 6:
		bit_set(PORTB,BIT1_MASK);
		bit_set(PORTB,BIT0_MASK);
		break;
		default:
		break;
	}
}

inline void clearOutputPin(unsigned char pin_number)
{
	/*
		TODO: separate this into clearAxonPin(axon_num) and clearLED(led_color)

		this is a super dumb abstraction right now with a pin_number lookup table
	*/
	switch(pin_number) {
		case 0:
		bit_clear(PORTD,BIT2_MASK);
		break;
		case 1:
		bit_clear(PORTD,BIT3_MASK);
		break;
		case 2:
		bit_clear(PORTD,BIT4_MASK);
		break;
		case 4:
		bit_clear(PORTB,BIT2_MASK);
		bit_clear(PORTB,BIT3_MASK);
		break;
		case 6:
		bit_clear(PORTB,BIT1_MASK);
		bit_clear(PORTB,BIT0_MASK);
		break;
		default:
		break;
	}
}

char SPI_Receive(void)
{
/*
	if (SPSR & (1<<SPIF)) {
		return SPDR;
	}
	*/
	return 0;
}

ISR(TIMER0_COMPA_vect)
{
	timeInterruptRoutine();
}