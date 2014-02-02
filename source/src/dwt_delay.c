/*******************************************************************
	Module dwt_delay.c
	
		Functions for implementing delays.
		
		Uses 32-bit counter DWT_CYCCN standard for all cortex M3 devices.
		When enabled, this counter increments every core cycle
	
	http://electronix.ru/forum/index.php?showtopic=99506
		
	TODO: check out sharing between different OS tasks - possibly use defines instead of functions
	
********************************************************************/

#include "MDR32Fx.h"
#include "dwt_delay.h"


//---------------------------------------------//
//	Enables DWT counter
//---------------------------------------------//
void DWT_Init(void)
{
	if (!(SCB_DEMCR & 1))
    {
        SCB_DEMCR   |= 0x01000000;
        DWT_CYCCNT   = 0; 
        DWT_CONTROL |= 1; // enable the counter
    }
}

//---------------------------------------------//
//	Returns current DWT counter value
//---------------------------------------------//
#pragma inline
uint32_t DWT_Get(void)
{
	 return DWT_CYCCNT;
}

//---------------------------------------------//
//	Returns number of ticks between two time marks
//	time2 >= time1
//---------------------------------------------//
uint32_t DWT_GetDelta(uint32_t time1, uint32_t time2)
{
	return time2 - time1;
}

//---------------------------------------------//
//	Returns number of ticks between a time mark
//	and current DWT counter
//---------------------------------------------//
uint32_t DWT_GetDeltaForNow(uint32_t time_mark)
{
	return DWT_CYCCNT - time_mark;
}

//---------------------------------------------//
//	Returns future time mark for specified
//		delay in us
//---------------------------------------------//
uint32_t DWT_StartDelayUs(uint32_t us)
{
	uint32_t time_mark = DWT_CYCCNT + us * (F_CPU/1000000UL);
	return time_mark;
}

//---------------------------------------------//
//	Checks if DWT counter has not reached 
//		specified time mark
//---------------------------------------------//
uint8_t DWT_DelayInProgress(uint32_t time_mark)
{
	return (((int32_t)DWT_CYCCNT - (int32_t)time_mark) < 0);
}




