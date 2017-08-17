#ifndef HAL_H_
#define HAL_H_

#define disableServo() do{TCCR1A=0; TCCR1B=0;}while(0)
#define enableServo() do{}while(0)
#define mode_button_pressed	(PIND & 1<<PD6)
#define adjust_button_pressed (PIND & 1<<PD7)
#define PIN_R_LED   0
#define PIN_G_LED   1
#define PIN_B_LED   2
#define PIN_AXON_1  4
#define PIN_AXON_2  6

#include "neuron.h"

void systemInit(void);
unsigned char readDendritePin(unsigned char dendrite_number, dendrite_types dendrite_type);
void setOutputPin(unsigned char pin_number);
void clearOutputPin(unsigned char pin_number);

void setupTimer(void);

extern volatile unsigned char ms_tick;

#endif
