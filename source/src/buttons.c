/**********************************************************
	Module for processing buttons and switches
	using bit FIFOs
	
	Type: Source file
	Written by: AvegaWanderer 10.2013
**********************************************************/
#include "MDR32Fx.h"
#include "FreeRTOS.h"
#include "task.h"

#include "defines.h"
#include "dwt_delay.h"
#include "buttons.h"

// Processed output
buttons_t buttons;
static uint16_t raw_button_state = 0;
static bfifo_type_t bit_fifo[TOTAL_BUTTONS];


//---------------------------------------------//
//	Gets raw button state
//	
//---------------------------------------------//
static void UpdateRawButtonState(void)
{
	uint16_t raw_buttons = 0;
	uint32_t temp;
	uint32_t time_delay;
	// SB_ON and SB_OFF are combined with the green and red leds
	// disable output for leds
	MDR_PORTB->OE &= ~(1<<LGREEN | 1<<LRED);
	// Start delay
	time_delay = DWT_StartDelayUs(15);
	// get other buttons by delay time
	temp = MDR_PORTA->RXTX;
	if (! (temp & (1<<ENC_BTN)) )
		raw_buttons |= BTN_ENCODER;
	if (! (temp & (1<<EEN)) )
		raw_buttons |= SW_EXTERNAL;
	temp = MDR_PORTB->RXTX;
	if (! (temp & (1<<SB_ESC)) )
		raw_buttons |= BTN_ESC;
	if (! (temp & (1<<SB_LEFT)) )
		raw_buttons |= BTN_LEFT;
	if (! (temp & (1<<SB_RIGHT)) )
		raw_buttons |= BTN_RIGHT;
	if (! (temp & (1<<SB_OK)) )
		raw_buttons |= BTN_OK;
	if (! (temp & (1<<SB_MODE)) )
		raw_buttons |= SW_CHANNEL;
	// wait until delay is done
	while(DWT_DelayInProgress(time_delay));
	// read once more
	temp = MDR_PORTB->RXTX;
	// enable outputs for leds again
	MDR_PORTB->OE |= (1<<LGREEN | 1<<LRED);
	// add remaining buttons	
	if (! (temp & (1<<LGREEN)) )
		raw_buttons |= BTN_ON;
	if (! (temp & (1<<LRED)) )
		raw_buttons |= BTN_OFF;

	
	// Update global variable
	raw_button_state = raw_buttons;
}

//---------------------------------------------//
//	Clear all events
//	
//---------------------------------------------//
static void ResetButtonEvents(void)
{
	buttons.action_down = 0;
	#ifdef USE_ACTION_REP
	buttons.action_rep = 0;
	#endif
	#ifdef USE_ACTION_UP
	buttons.action_up = 0;
	#endif
	#ifdef USE_ACTION_UP_SHORT
	buttons.action_up_short = 0;
	#endif
	#ifdef USE_ACTION_UP_LONG
	buttons.action_up_long = 0;
	#endif
	#ifdef USE_ACTION_HOLD
	buttons.action_hold = 0;
	#endif
}

//---------------------------------------------//
//	Initial buttons setting
//	
//---------------------------------------------//
void InitButtons(void)
{
	uint8_t i;
	for (i=0; i<TOTAL_BUTTONS; i++)
	{
		bit_fifo[i] = 0;
	}
	ResetButtonEvents();
	#ifdef USE_ACTION_TOGGLE
	buttons.action_toggle = 0;
	#endif
}

//---------------------------------------------//
//	Buttons processor
//	
//---------------------------------------------//
void ProcessButtons(void)
{
	bfifo_type_t temp;	
	btn_type_t raw_current;
	btn_type_t current_bit;
	uint8_t i;
	
	taskENTER_CRITICAL();
	UpdateRawButtonState();
	taskEXIT_CRITICAL();
	
	raw_current = raw_button_state ^ RAW_BUTTON_INVERSE_MASK;
	#ifdef NOT_USE_JTAG_BUTTONS
	raw_current &= ~BTN_JTAG_MASK;
	#endif
	ResetButtonEvents();
	
	// Loop through all buttons
	for (i=0; i<TOTAL_BUTTONS; i++)
	{
		current_bit = (1<<i);
		temp = bit_fifo[i];
		// Shift in current button state bit
		temp <<= 1;
		if (raw_current & current_bit)
			temp |= 0x01;
		// Analyse FIFO and produce events
		switch(temp & BFIFO_BUTTON_MASK)
		{
			case BFIFO_BUTTON_DOWN:
				buttons.action_down |= current_bit;
				#ifdef USE_ACTION_REP
				buttons.action_rep |= current_bit;
				#endif
				#ifdef USE_ACTION_TOGGLE
				buttons.action_toggle ^= current_bit;
				#endif
				break;
			#ifdef ANALYSE_ACTION_UP
			case BFIFO_BUTTON_UP:
				#ifdef USE_ACTION_UP
				buttons.action_up |= current_bit;
				#endif
				#ifdef ANALYZE_TIMED_ACTION_UP
				if ((temp & BFIFO_LONG_MASK) == BFIFO_LONG_MASK)
				{
					#ifdef USE_ACTION_UP_LONG
					buttons.action_up_long |= current_bit;
					#endif
				}
				else
				{
					#ifdef USE_ACTION_UP_SHORT
					buttons.action_up_short |= current_bit;
					#endif
				}
				break;
				#endif
			#endif
			#ifdef ANALYSE_HOLD
			case BFIFO_BUTTON_HOLD:
				#ifdef USE_ACTION_REP
				if ((temp & BFIFO_REPEAT_MASK) == BFIFO_REPEAT_MASK)
				{
					buttons.action_rep |= current_bit;
				}
				#endif
				#ifdef USE_ACTION_HOLD
				if ((temp & BFIFO_LONG_MASK) == BFIFO_HOLD)
				{
					buttons.action_hold |= current_bit;
				}
				break;
				#endif
			#endif
		}
		bit_fifo[i] = temp;
	}
	buttons.raw_state = raw_current;
}


