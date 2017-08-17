#ifndef NEURON_H_
#define NEURON_H_

#define MEMBRANE_THRESHOLD      120
#define HYPERPOLARIZATION		-200
#define DENDRITE_COUNT          5

typedef enum{
DIGITAL =   0,
ANALOG =    1
} dendrite_modes;

typedef enum{
OFF =   0,
ON =    1
} dendrite_states;

typedef enum{
EXCITATORY = 0,
INHIBITORY = 1
} dendrite_types;

typedef struct{
dendrite_states    state;
dendrite_modes      mode;
dendrite_types      type;
int16_t             magnitude; // weighting
int16_t             current_value;
uint8_t             history;
uint16_t			timestamp;
} dendrite_t;

typedef enum{
IAF =   0,
MOTOR = 1,
IZHIK = 2 // Izhikevich
} neuron_modes;

typedef enum{
INTEGRATE =       0,
FIRE =          1,
REST =			2 // izhik
} neuron_states;

typedef enum{
NONE =	0,
HEBB =	1,
SLEEP =	2
} learning_states;

typedef struct{
// mode independent vars
int16_t     potential;
dendrite_t  dendrites[DENDRITE_COUNT];
neuron_modes     mode;
neuron_states   state;
learning_states learning_state; // Hebb learning mode

uint16_t    fire_time;
int16_t		fire_potential;

// Izikevich vars
int16_t    fire_period;
uint16_t	scaling;
uint16_t	next_fire_time;
uint8_t		analog_in_flag;

// Hebb vars
uint16_t	hebb_time;
uint8_t		time_multiple;
uint16_t	ms_count;
} neuron_t;

void neuronInit(neuron_t *n);
void checkDendrites(neuron_t * n);
int16_t calcNeuronPotential(neuron_t *n);
void dendriteDecayStep(dendrite_t * dendrites);
void membraneDecayStep(neuron_t * n);
void dendriteSwitchOff(dendrite_t * dendrite);
void incrementHebbTime(neuron_t *n);

int16_t dendrite_magnitude_lookup[5];

#endif