/*******************************************************************
	Module encoder.c
	
		Functions for implementing delays.
		
		Uses counter DWT_CYCCN standard for all cortex M3 devices
		When enabled, this counter increments every core cycle

********************************************************************/

#include "MDR32Fx.h"

#include "dwt_delay.h"

// time stamp
volatile int32_t time_mark;

//==============================================================//
// Enables DWT counter
//==============================================================//
void DWTCounterInit(void)
{
	if (!(DWT_CONTROL & 1))
    {
        SCB_DEMCR   |= 0x01000000;
        DWT_CYCCNT   = 0; 
        DWT_CONTROL |= 1; // enable the counter
    }
}


//==============================================================//
// Non - blocking us delay.
// Function starts new delay but does not wait for it
//==============================================================//
void DWTStartDelayUs(uint32_t us)
{
	time_mark = DWT_CYCCNT + us * (SystemCoreClock/1000000);
}


//==============================================================//
// Non - blocking delay status.
// Function returns 0 if delay is done
//==============================================================//
uint8_t DWTDelayInProgress(void)
{
	return (((int32_t)DWT_CYCCNT - time_mark) < 0);
}


//==============================================================//
// Blocking us delay.
// Function executes for specified period of time
// There is a small overhead of 2~4 us @16 MHz
//==============================================================//
void DWTDelayUs(uint32_t us)
{
	int32_t time_mark = DWT_CYCCNT + us * (SystemCoreClock/1000000);
	while ((((int32_t)DWT_CYCCNT - time_mark) < 0)) {};
}





