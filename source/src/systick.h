#ifndef __SYSTICK_H
#define __SYSTICK_H


#include "MDR32Fx.h" 


// Low level interrupt-driven routines timing
#define HW_IRQ_PERIOD				(16*200)		// in units of 62.5ns, must be <= timer period
#define HW_ADC_CALL_PERIOD			5				// in units of HW_IRQ period


// Task synch timing
#define NUM_TIMERS			5
#define TIMER_CONVERTER		0
#define TIMER_SOUND			1
#define TIMER_GUI			2
#define TIMER_BUTTONS		3
#define TIMER_UART_RX		4



typedef struct {
	uint32_t max_ticks_in_Systick_hook;
	uint32_t max_ticks_in_Timer2_ISR;
} time_profile_t;


extern time_profile_t time_profile;



void Synchronizer_Initialize(void);
void Synchronizer_Start(void);


#endif 


