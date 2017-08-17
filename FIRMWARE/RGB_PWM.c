#include <avr/io.h>
#include <RGB_PWM.h>
#include <HAL.h>
#include <neuron.h>

//uint8_t led_val[3] = {0,0,0};


uint16_t led_tick;

void LEDfullWhite(void)
{
	led_red_val = 156;
	led_green_val = 156;
	led_blue_val = 156;
}



void translateColor(int16_t v, neuron_modes mode)
{
	uint8_t temp = 0;
	uint8_t temp_max = 24;

	switch (mode){
		case IAF:
			if((v < -99)) {
				led_red_val     = 24;
				led_green_val   = 0;
				led_blue_val    = 0;
			}
			else if ((v >= -99) && (v < 0)) {
				temp = (-v / 10); //debug
				led_red_val     = 14 + temp;
				led_green_val   = 10 - temp;
				led_blue_val    = 0;
			}
			else if (v == 0) {
				led_red_val     = 14;
				led_green_val   = 8;
				led_blue_val    = 0;
			}
			else if ((v > 0) && (v <= 99)) {
				temp = v / 7; //debug
				led_red_val     = 14 - temp;
				temp = v / 10;
				led_green_val   = 8 + temp; //debug
				led_blue_val   = 0;
			}
			else if (v > 99) {
				led_red_val     = 0;
				led_green_val   = 18;
				led_blue_val    = 0;
			}
			break;
		case MOTOR:
			if((v < -100)) {
				led_red_val     = 0;
				led_green_val   = 0;
				led_blue_val    = 24;
			}
			else if ((v >= -100) && (v < 0)) {
				temp = (-v / 4) - 1;
				led_red_val     = 24 - temp;
				led_green_val   = 0;
				led_blue_val    = 24;
			}
			else if (v == 0) {
				led_red_val     = 24;
				led_green_val   = 0;
				led_blue_val    = 24;
			}
			else if ((v > 0) && (v <= 99)) {
				temp = v / 4;
				led_red_val     = 24;
				led_green_val   = 0;
				led_blue_val   = 24 - temp;
			}
			else if (v > 99) {
				led_red_val     = 24;
				led_green_val   = 0;
				led_blue_val    = 0;
			}
			break;
		case IZHIK:
			if((v < -100)) {
				led_red_val     = 0;
				led_green_val   = 0;
				led_blue_val    = 24;
			}
			else if ((v >= -100) && (v < 0)) {
				temp = -v / 4;
				led_red_val     = 0;
				led_green_val   = 24 - temp;
				led_blue_val    = temp;
			}
			else if (v == 0) {
				led_red_val     = 24;
				led_green_val   = 0;
				led_blue_val    = 0;
			}
			else if ((v > 0) && (v <= 600)) {
				temp = v / 24;
				led_red_val     = 24 - temp;
				led_green_val   = temp;
				led_blue_val   = 0;
			}
			else if (v > 600) {
				led_red_val     = 0;
				led_green_val   = 24;
				led_blue_val    = 0;
			}
			break;
	}
}

void updateLED(void)
{
	if (led_red_val > led_tick) {
		clearOutputPin(PIN_R_LED);
		} else {
		setOutputPin(PIN_R_LED);
	}

	if (led_green_val > led_tick) {
		clearOutputPin(PIN_G_LED);
		} else {
		setOutputPin(PIN_G_LED);
	}

	if (led_blue_val > led_tick) {
		clearOutputPin(PIN_B_LED);
		} else {
		setOutputPin(PIN_B_LED);
	}
	if (led_tick++ > led_resolution) {
		led_tick = 0;
	}
}