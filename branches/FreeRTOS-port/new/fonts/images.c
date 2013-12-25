#include "image_defs.h"

const uint8_t image_Select0_data[] =
{
	0x60,0xF0,0xF8,0xFC,0xFE,0x00,0x00,0x00,
	0x01,0x03,0x07,0x00
};

const tImage imgSelect0 =
{
	6,                        // Width
  12,                       // Height
  image_Select0_data   
};




const uint8_t image_Underline0_data[] =
{
	0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
  0x03,0x03
};

const tImage imgUnderline0 =
{
	10,                      // Width
  2,                       // Height
  image_Underline0_data   
};
