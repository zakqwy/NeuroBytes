/*
 * states.h
 *
 * Created: 1/18/2017 7:12:21 AM
 *  Author: jrwhi_000
 */ 


#ifndef STATES_H_
#define STATES_H_

#include "neuron.h"

void changeMode(neuron_t *n);
void IAF_init(neuron_t *n);
void MOTOR_init(neuron_t *n);
void IZHIK_init(neuron_t *n);
void changeHebb(neuron_t *n);
void HEBB_init(neuron_t *n);


#endif /* STATES_H_ */
