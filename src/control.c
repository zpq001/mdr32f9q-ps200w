/*******************************************************************
	Module control.c
	
		Functions for main board control.
		

********************************************************************/


#include "MDR32Fx.h" 
#include "MDR32F9Qx_port.h"

#include "defines.h"
#include "control.h"


SYSTEM_CONTROL_Typedef system_control = {CONVERTER_OFF,CHANNEL_12V,CURRENT_LIM_MIN,LOAD_ENABLE};
SYSTEM_STATUS_Typedef system_status;


 
//==============================================================//
// Updates the global system_status variable
// global system state
//==============================================================//

void UpdateSystemStatus(SYSTEM_STATUS_Typedef* system_status)
{
	uint16_t temp = MDR_PORTA->RXTX;
	
	// Overload of converter
	if (! (temp & (1<<OVERLD)) )
		system_status -> ConverterOverload = OVERLOAD;
	else
		system_status -> ConverterOverload = NORMAL;  
	
	// External switch state
	if (temp & (1<<EEN))
		system_status -> ExternalSwitchState = SWITCH_OFF;
	else
		system_status -> ExternalSwitchState = SWITCH_ON;
	
	// 220V AC input line state
	temp = MDR_PORTB->RXTX;
	if (! (temp & (1<<PG)) )
		system_status -> LineInStatus = ONLINE;
	else
		system_status -> LineInStatus = OFFLINE;
}



//==============================================================//
// Applies system control settings
// global system control
//==============================================================//
void ApplySystemControl(SYSTEM_CONTROL_Typedef* system_control)
{
	// Power converter enable/disable
	if (system_control->ConverterState == CONVERTER_ON)
		PORT_SetBits(MDR_PORTF, 1<<EN);
	else
		PORT_ResetBits(MDR_PORTF, 1<<EN);
	
	// Feedback channel select
	if (system_control->SelectedChannel == CHANNEL_5V)
		PORT_SetBits(MDR_PORTF, 1<<STAB_SEL);
	else
		PORT_ResetBits(MDR_PORTF, 1<<STAB_SEL);
	
	// Current amplifier gain select
	if (system_control->CurrentLimit == CURRENT_LIM_MAX)
		PORT_SetBits(MDR_PORTA, 1<<CLIM_SEL);
	else
		PORT_ResetBits(MDR_PORTA, 1<<CLIM_SEL);
	
	// 12V channel load enable/disable
	if (system_control->LoadDisable == LOAD_DISABLE)
		PORT_SetBits(MDR_PORTE, 1<<LDIS);
	else
		PORT_ResetBits(MDR_PORTE, 1<<LDIS);
}








