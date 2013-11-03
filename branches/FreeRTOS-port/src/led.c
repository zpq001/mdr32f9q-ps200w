/*******************************************************************
	Module led.c
	
		Low-level functions for leds

********************************************************************/

#include "MDR32Fx.h"
#include "MDR32F9Qx_port.h"

#include "led.h"
#include "systemfunc.h"
#include "defines.h"


//==============================================================//
// Update LEDs
// Standard library functions are used in order to save 
// JTAG functionality
//==============================================================//
void UpdateLEDs(uint8_t led_state)
{
	if (led_state & LED_GREEN)
		PORT_ResetBits(MDR_PORTB, 1<<LGREEN);
		//MDR_PORTB->RXTX &= ~(1<<LGREEN);
	else
		PORT_SetBits(MDR_PORTB, 1<<LGREEN);
		//MDR_PORTB->RXTX |= (1<<LGREEN);
	
	if (led_state & LED_RED)
		PORT_ResetBits(MDR_PORTB, 1<<LRED);
		//MDR_PORTB->RXTX &= ~(1<<LRED);
	else
		PORT_SetBits(MDR_PORTB, 1<<LRED);
	  //MDR_PORTB->RXTX |= (1<<LRED);
}


