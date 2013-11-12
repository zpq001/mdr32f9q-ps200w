
#ifndef __sound_defs_h
#define __sound_defs_h

typedef struct {
	uint8_t tone_period;		// in 16us gradation	
	uint8_t duration;			// in 10ms gradation
} tone_t;

typedef struct {
	uint8_t num_repeats;
	const tone_t *tones;
} sample_t;


#endif
