
#include "sound_defs.h"

//---------------------------------------------//
// Tone arrays
//  frequency    |  duration

const tone_t _beep_1000Hz_10ms[] = 
{
	{ FREQ(1000),	LAST(10) },
	{0,	0}
};

const tone_t _beep_800Hz_10ms[] = 
{
	{ FREQ(800),	LAST(10) },
	{0,	0}
};

const tone_t _beep_600Hz_10ms[] = 
{
	{ FREQ(600),	LAST(10) },
	{0,	0}
};

const tone_t _silence_10ms[] = 
{
	{ 0,			LAST(50) },
	{0,	0}
};



//---------------------------------------------//
// Samples
//  num_repeats    |    tone

const sample_t sample_beep_1000Hz_50ms[] = 		// OK
{
	{5, 			_beep_1000Hz_10ms},
	{0,0}
};

const sample_t sample_beep_800Hz_50ms[] = 		// Illegal regulation parameter
{
	{5, 			_beep_800Hz_10ms},
	{0,0}
};

const sample_t sample_beep_800Hz_x3[] = 		// Overload
{
	{5, 			_beep_800Hz_10ms},
	{5, 			_silence_10ms	},
	{5, 			_beep_800Hz_10ms},
	{5, 			_silence_10ms	},
	{5, 			_beep_800Hz_10ms},
	{0,0}
};

const sample_t sample_beep_800Hz_20ms[] = 		// Overload (instant)
{
	{2, 			_beep_800Hz_10ms},
	{0,0}
};

const sample_t sample_beep_600Hz_x2[] = 		// All other event codes
{
	{10, 			_beep_600Hz_10ms},
	{5, 			_silence_10ms	},
	{10, 			_beep_600Hz_10ms},
	{0,0}
};




