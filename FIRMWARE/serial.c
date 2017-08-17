/*
 * serial.c
 *
 * Created: 1/18/2017 7:13:25 AM
 *  Author: jrwhi_000
 */ 

 /*

 SPI Command List:

 Membrane Potential:
 0x00 -- get membrane potential bits 0-7
 0x01 -- get membrane potential bits 8-15

 Dendrites:
 0x02 -- get dendrite 0 0b00000TS ;; T = TYPE ;; S = STATUS
 0x03 -- get dendrite 1
 0x04 -- get dendrite 2
 0x05 -- get dendrite 3
 0x06 -- get dendrite 4

 Operating Modes:
 0x07 -- set IAF
 0x08 -- set Motor
 0x09 -- set Izhikevich
 0x0A -- set Pacemaker
 0x0D -- get current mode
 
 Lead Bytes:
 0x0B -- get parameter value -- followed by parameter code
 0x0C -- set parameter value -- followed by parameter code -- followed by new value

 Axons:
 0x0E -- get axon 1
 0x0F -- get axon 2

 LEDs:
 0x10 -- get LED values

 Parameter Codes:
 0x00 -- dendrite 0 weighting
 0x01 -- dendrite 1 weighting
 0x02 -- dendrite 2 weighting
 0x03 -- dendrite 3 weighting
 0x04 -- dendrite 4 weighting
 0x05 -- Hyperpolarization
 
 */
 /*
 char processCommand(char command_byte)
 {
	uint16_t staging;
	char temp;
	switch (command_byte) {
		case 0x00: // get membrane potential, bits 0-7
			staging = neuron.potential
			temp << staging;
			SPDR = temp;
		case 0x01: // get membrane potential, bits 8-15
			
	}
 }

 char shiftSPI(char transmit_byte)
 {
	
 }
 */