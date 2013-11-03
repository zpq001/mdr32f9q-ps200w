/*******************************************************************
	Module dwt_delay.c
	
		Functions for implementing delays.
		
		Uses counter DWT_CYCCN standard for all cortex M3 devices
		When enabled, this counter increments every core cycle

	TODO: check out sharing between different OS tasks - possibly use defines instead of functions
	
********************************************************************/

#include "MDR32Fx.h"
#include "dwt_delay.h"


//---------------------------------------------//
//	Enabled DWT counter
//---------------------------------------------//
void DWTCounterInit(void)
{
	if (!(DWT_CONTROL & 1))
    {
        SCB_DEMCR   |= 0x01000000;
        DWT_CYCCNT   = 0; 
        DWT_CONTROL |= 1; // enable the counter
    }
}


//---------------------------------------------//
//	Returns future time mark for specified
//		delay in us
//---------------------------------------------//
uint32_t DWTStartDelayUs(uint32_t us)
{
	uint32_t time_mark = DWT_CYCCNT + us * (SystemCoreClock/1000000);
	return time_mark;
}


//---------------------------------------------//
//	Checks if DWT counter has not reached 
//		specified time mark
//---------------------------------------------//
uint8_t DWTDelayInProgress(uint32_t time_mark)
{
	return (((int32_t)DWT_CYCCNT - (int32_t)time_mark) < 0);
}







