/*******************************************************************
	Module buttons.c
	
		Low-level functions for buttons.

********************************************************************/

#include "MDR32Fx.h"

#include "defines.h"
#include "buttons.h"
#include "dwt_delay.h"


uint8_t	button_state = 0;				// global buttons state
uint8_t raw_button_state = 0;		// global raw buttons state

//==============================================================//
// Updates the global button_state variable
//==============================================================//
void UpdateButtons(void)
{
	uint8_t raw_buttons = 0;
	static uint8_t button_mask = 0xFF;
	static uint8_t interval;
  static uint8_t old_buttons_state = 0;
	uint32_t temp;
	// SB_ON and SB_OFF are combined with the green and red leds
	// disable output for leds
	MDR_PORTB->OE &= ~(1<<LGREEN | 1<<LRED);
	// Start delay
	DWTStartDelayUs(10);
	// get other buttons by delay time
	temp = MDR_PORTA->RXTX;
	if (! (temp & (1<<ENC_BTN)) )
		raw_buttons |= BUTTON_ENCODER;
	temp = MDR_PORTB->RXTX;
	if (! (temp & (1<<SB_ESC)) )
		raw_buttons |= BUTTON_ESC;
	if (! (temp & (1<<SB_LEFT)) )
		raw_buttons |= BUTTON_LEFT;
	if (! (temp & (1<<SB_RIGHT)) )
		raw_buttons |= BUTTON_RIGHT;
	if (! (temp & (1<<SB_OK)) )
		raw_buttons |= BUTTON_OK;
	if (! (temp & (1<<SB_MODE)) )
		raw_buttons |= MODE_SWITCH;
	// wait until delay is done
	while(DWTDelayInProgress()) {};
	// read once more
	temp = MDR_PORTB->RXTX;
	// enable outputs for leds again
	MDR_PORTB->OE |= (1<<LGREEN | 1<<LRED);
	// add remaining buttons	
	if (! (temp & (1<<LGREEN)) )
		raw_buttons |= BUTTON_ON;
	if (! (temp & (1<<LRED)) )
		raw_buttons |= BUTTON_OFF;
	
	// Update global variable
	raw_button_state = raw_buttons;
	
	// All buttons except BUTTON_LEFT, BUTTON_RIGHT and MODE_SWITCH
	// are set on the first function call and cleared on next,
	// regardless of real button state. 
	// BUTTON_LEFT and BUTTON_RIGHT are repeated over a small pause
	// MODE_SWITCH is always real switch state
	
	button_state = raw_buttons & (button_mask | MODE_SWITCH);
	button_mask = ~raw_buttons;
	
	if (old_buttons_state != (raw_buttons & (BUTTON_LEFT | BUTTON_RIGHT)) )
	{
		interval = 0;
		old_buttons_state = raw_buttons & (BUTTON_LEFT | BUTTON_RIGHT) ;
	}		
	else
		if (interval<10)
      interval++;
    else
			button_mask |= (BUTTON_LEFT | BUTTON_RIGHT);

}







