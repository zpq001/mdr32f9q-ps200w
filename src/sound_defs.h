
#ifndef __sound_defs_h
#define __sound_defs_h

#include "stdint.h"	// std typedefs


#define FREQ(x)	(uint16_t)(1000000/x)
#define LAST(x)	(uint16_t)(x/10)

typedef struct {
	uint16_t tone_period;		// in 20us gradation	
	uint16_t duration;			// in 10ms gradation
} tone_t;

typedef struct {
	uint8_t num_repeats;
	const tone_t *tones;
} sample_t;


#endif
