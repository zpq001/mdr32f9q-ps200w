
#include "stdint.h"	// std typedefs

#ifndef __font_defs_h
#define __font_defs_h




//----- Special fonts typedefs -----//
typedef struct 
{
	uint8_t code;
	uint8_t width;
	const uint8_t *data;
} tSpecialFontItem;


typedef struct 
{
	uint8_t height;
	uint8_t count;
	const tSpecialFontItem *Chars;
} tSpecialFont;

//----------------------------------//


//----- Regular fonts typedefs -----//

typedef struct 
{
	uint8_t width;
	uint8_t height;
	uint8_t count;
	uint8_t bytesPerChar;
	const uint8_t *data;
} tNormalFont;


//----------------------------------//






#endif	// __font_defs_h


