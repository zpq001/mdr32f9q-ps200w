/*******************************************************************
	Module control.c
	
		Functions for main board control.
		

********************************************************************/


#include "MDR32Fx.h" 
#include "MDR32F9Qx_port.h"

#include "defines.h"
#include "control.h"



//-------------------------------------------------------//
// Overload of converter
//-------------------------------------------------------//
uint8_t GetOverloadStatus(void)
{
	return (MDR_PORTA->RXTX & (1<<OVERLD)) ? NORMAL : OVERLOAD;
}

// 220V AC input line state
uint8_t GetACLineStatus(void)
{
	return (MDR_PORTB->RXTX & (1<<PG)) ? OFFLINE : ONLINE; 
}


// TODO: make atomic access to PORTF

//-------------------------------------------------------//
// Power converter enable/disable
//-------------------------------------------------------//
void SetConverterState(uint8_t newState)
{
	if (newState == CONVERTER_ON)
		PORT_SetBits(MDR_PORTF, 1<<EN);
	else
		PORT_ResetBits(MDR_PORTF, 1<<EN);
}

uint8_t GetConverterState(void)
{
	//return PORT_CheckBits(MDR_PORTF, 1<<EN);
	return (MDR_PORTF->RXTX & (1<<EN)) ? CONVERTER_ON : CONVERTER_OFF;
}

//-------------------------------------------------------//
// Feedback channel select (5V or 12V)
//-------------------------------------------------------//
void SetFeedbackChannel(uint8_t newChannel)
{
	if (newChannel == CHANNEL_5V)
		PORT_SetBits(MDR_PORTF, 1<<STAB_SEL);
	else
		PORT_ResetBits(MDR_PORTF, 1<<STAB_SEL);
}

//-------------------------------------------------------//
// Current amplifier gain select (20A or 40A)
//-------------------------------------------------------//
void SetCurrentRange(uint8_t newRange)
{
	if (newRange == CURRENT_RANGE_HIGH)
		PORT_SetBits(MDR_PORTA, 1<<CLIM_SEL);
	else
		PORT_ResetBits(MDR_PORTA, 1<<CLIM_SEL);
}

//-------------------------------------------------------//
// 12V channel load enable/disable
//-------------------------------------------------------//
void SetOutputLoad(uint8_t newLoad)
{
	if (newLoad == LOAD_DISABLE)
		PORT_SetBits(MDR_PORTE, 1<<LDIS);
	else
		PORT_ResetBits(MDR_PORTE, 1<<LDIS);	
}




