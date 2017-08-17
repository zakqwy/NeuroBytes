#include <avr/io.h>
#include "neuron.h"
#include "HAL.h"
#include "RGB_PWM.h"


void neuronInit(neuron_t *n)
{
	uint8_t i;

	n->potential = 0;
	n->mode = IAF;
	n->state = INTEGRATE;

	n->fire_time = 0;
	n->fire_potential = 0;

	n->hebb_time = 0;
	n->learning_state = NONE;

	for (i=0;i<DENDRITE_COUNT;i++){
		n->dendrites[i].state = OFF;
		n->dendrites[i].history = 0b00000000;
		n->dendrites[i].current_value = 0;
		n->dendrites[i].mode = DIGITAL;
		n->dendrites[i].type = EXCITATORY;
		n->dendrites[i].timestamp = 0;
	}

	n->dendrites[0].magnitude = dendrite_magnitude_lookup[0];
	n->dendrites[1].magnitude = dendrite_magnitude_lookup[1];
	n->dendrites[2].magnitude = dendrite_magnitude_lookup[2];
	n->dendrites[3].magnitude = dendrite_magnitude_lookup[3];
	n->dendrites[4].magnitude = dendrite_magnitude_lookup[4];

	led_resolution = 156;
}

void checkDendrites(neuron_t * n)
{
	uint8_t i;
	dendrite_states current_state = OFF;

	for (i=0; i<DENDRITE_COUNT; i++){
		switch(n->dendrites[i].mode) {
			case DIGITAL:
			// read the inhibitory and excitatory pins for each dendrite
				if (readDendritePin(i, INHIBITORY) == 1){
					n->dendrites[i].type = INHIBITORY;
					current_state = ON;
				} else if (readDendritePin(i, EXCITATORY) == 1){
					n->dendrites[i].type = EXCITATORY;
					current_state = ON;
				} else {
					current_state = OFF;
				}

				// keep last 8 state measurements and debounce them to determine dendrite's state
				n->dendrites[i].history <<= 1;
				if (current_state == ON){
					n->dendrites[i].history |= 0x01;
				}
				
				n->dendrites[i].timestamp++;

				if (n->dendrites[i].history & 0b00000111){
					n->dendrites[i].state = ON;
					n->dendrites[i].timestamp = 0;
				} else if ((n->dendrites[i].history & 0b11000111) == 0b11000000 && n->dendrites[i].state == ON){
					n->dendrites[i].state = OFF;
					dendriteSwitchOff(&n->dendrites[i]); // this is the end of pulse so restart history and start decaying *.current_value
				} 
				break;
			case ANALOG:
				break;
		}
	}
}

void incrementHebbTime(neuron_t * n)
{
	uint8_t i;

	n->ms_count++;
	if (n->ms_count == n->time_multiple){
		n->hebb_time++;
		n->ms_count = 0;
	}

	if (n->hebb_time == UINT16_MAX - 1){
		n->hebb_time /= 2;
		for (i=0; i<DENDRITE_COUNT; i++){
			n->dendrites[i].timestamp /= 2;
		}
		n->time_multiple *= 2;
	}
}

void dendriteSwitchOff(dendrite_t *dendrite)
{
	dendrite->history = 0b00000000;
	switch (dendrite->mode){
		case DIGITAL:
			switch(dendrite->type){
				case EXCITATORY:
					dendrite->current_value += dendrite->magnitude;
					break;
				case INHIBITORY:
					dendrite->current_value -= dendrite->magnitude;
					break;
			}
			break;
	}
}

void dendriteDecayStep(dendrite_t * dendrites)
{
	uint8_t i;

	for(i=0; i<DENDRITE_COUNT; i++){
		dendrites[i].current_value = (dendrites[i].current_value * 63 ) / 64;
	}
}

void membraneDecayStep(neuron_t * n)
{
	n->fire_potential = (n->fire_potential * 63) / 64;
}

int16_t calcNeuronPotential(neuron_t *n)
{
	uint8_t i;
	int16_t new_v = 0;
	for (i=0; i<DENDRITE_COUNT; i++){
		if (n->dendrites[i].state == ON){
			switch(n->dendrites[i].mode){
				case DIGITAL:
					switch(n->dendrites[i].type){
						case EXCITATORY:
							new_v += n->dendrites[i].magnitude;
							break;
						case INHIBITORY:
							new_v -= n->dendrites[i].magnitude;
							break;
					}
					break;
			}
		}
		new_v += n->dendrites[i].current_value; // each dendrite contributes its decay (*.current_value) and magnitude
	}
	return new_v;
}
