#ifndef PWM_H_
#define PWM_H_

#include <neuron.h>

#define led_red_val led_val[0]
#define led_green_val led_val[1]
#define led_blue_val led_val[2]

void translateColor(int16_t v, neuron_modes modes);
void updateLED(void);
void LEDfullWhite(void);

uint8_t led_val[3];
uint16_t led_resolution;



#endif