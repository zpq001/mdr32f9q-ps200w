/*******************************************************************
	Module encoder.c
	
		Low-level functions for incremental encoder.

********************************************************************/

#include "MDR32Fx.h"



#include "defines.h"
#include "encoder.h"
#include "systick.h"

int16_t encoder_delta;
static volatile int16_t encoder_counter = 0;

void ProcessEncoder(void)
{
	static uint8_t enc_state = 0;
	enc_state = enc_state << 2;
	
	// Get new encoder state
	if (MDR_PORTB->RXTX & (1<<ENCA)) 
		enc_state |= 0x01;
	if (MDR_PORTB->RXTX & (1<<ENCB)) 
		enc_state |= 0x02;
	
	// Detect direction
	switch(enc_state & 0x0F)
	{
		case 0x2: case 0x4: case 0xB: case 0xD:
			encoder_counter --;
		break;
		case 0x1: case 0x7: case 0x8: case 0xE:
			encoder_counter ++;
		break;
	}
}


void UpdateEncoderDelta(void)
{
	static int16_t old_counter = 0;
	int16_t delta = old_counter;
	old_counter = encoder_counter&0xFFFC;
	delta = old_counter - delta;
	encoder_delta = delta>>2;
}


