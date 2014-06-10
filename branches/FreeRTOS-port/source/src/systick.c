

#include "systick.h"
#include "MDR32Fx.h"                    
#include "MDR32F9Qx_timer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "guiTop.h"

#include "encoder.h"
#include "systemfunc.h"
#include "control.h"
#include "converter.h"
#include "converter_hw.h"
#include "adc.h"
#include "dispatcher.h"
#include "dwt_delay.h"
#include "sound_driver.h"
#include "buttons_top.h"
#include "uart_rx.h"

// Profiling
time_profile_t time_profile;

static xTimerHandle xTimers[NUM_TIMERS];


//-------------------------------------------------------//
// Task synchronnizer - 
// callback function for FreeRTOS software timers task
//-------------------------------------------------------//
static void vTimerCallback(xTimerHandle pxTimer)
{
	uint8_t timer_id = (uint8_t)pvTimerGetTimerID(pxTimer);
	switch (timer_id)
	{
		case TIMER_CONVERTER:
			xQueueSendToBack(xQueueConverter, &converter_tick_message, 0);	// do not wait
			break;
		case TIMER_SOUND:
			xQueueSendToBack(xQueueSound, &sound_driver_sync_msg, 0);	// do not wait
			break;
		case TIMER_GUI:
			xQueueSendToBack(xQueueGUI, &gui_msg_redraw, 0);	
			break;
		case TIMER_BUTTONS:
			xQueueSendToBack(xQueueButtons, &buttons_tick_msg, 0);	
			break;
		case TIMER_UART_RX:
			xQueueSendToBack(xQueueUART1RX, &uart_rx_tick_msg, 0);	
			xQueueSendToBack(xQueueUART2RX, &uart_rx_tick_msg, 0);	
			break;
	}
}


void Synchronizer_Initialize(void)
{
	uint8_t i;
	// OS tick = 2ms
    xTimers[TIMER_CONVERTER] = xTimerCreate((signed char *)"Converter timer", 		50,	pdTRUE, (void *)TIMER_CONVERTER, 	vTimerCallback);
	xTimers[TIMER_SOUND] = xTimerCreate(	(signed char *)"Sound driver timer", 	5, 	pdTRUE, (void *)TIMER_SOUND, 		vTimerCallback);
	xTimers[TIMER_GUI] = xTimerCreate(		(signed char *)"GUI timer", 			25,	pdTRUE, (void *)TIMER_GUI, 			vTimerCallback);
	xTimers[TIMER_BUTTONS] = xTimerCreate(	(signed char *)"Buttons driver timer", 	10,	pdTRUE, (void *)TIMER_BUTTONS, 		vTimerCallback);
	xTimers[TIMER_UART_RX] = xTimerCreate(	(signed char *)"UART RX timer", 		5,	pdTRUE, (void *)TIMER_UART_RX, 		vTimerCallback);
	// Check
	for (i=0; i<NUM_TIMERS; i++)
	{
		if( xTimers[i] == NULL )
		{
			// The timer was not created
			while(1);	// Stop
		}
	}
}


void Synchronizer_Start(void)
{
	uint8_t i;
	for (i=0; i<NUM_TIMERS; i++)
	{
		xTimerStart( xTimers[i], portMAX_DELAY );
	}
}







//---------------------------------------------//
//	Fast low-level control dispatcher
//
//	Timer2 is also used to generate PWM for voltage 
//  and current setting
//---------------------------------------------//
void Timer2_IRQHandler(void) 
{
	static uint16_t hw_adc_counter = HW_ADC_CALL_PERIOD;
	uint16_t temp;
	// Time profiling
	uint32_t time_mark1 = 0;
	uint32_t time_mark2 = 0;
	uint32_t ticks_count;
	
	time_mark1 = DWT_Get();
/*	
	// Debug
	if (MDR_PORTA->RXTX & (1<<TXD1))
		PORT_ResetBits(MDR_PORTA, 1<<TXD1);
	else
		PORT_SetBits(MDR_PORTA, 1<<TXD1);
*/	
	
	
	if (--hw_adc_counter == 0)
	{
		hw_adc_counter = HW_ADC_CALL_PERIOD;
		Converter_HW_ADCProcess();	// Converter low-level ADC control
	}
	Converter_HWProcess();			// Converter low-level ON/OFF control and overload handling
	ProcessEncoder();				// Poll encoder				

	// Reinit timer2 CCR
	temp = MDR_TIMER2->CCR2 + HW_IRQ_PERIOD;	
	MDR_TIMER2->CCR2 = (temp > MDR_TIMER2->ARR) ? temp - MDR_TIMER2->ARR : temp;
	TIMER_ClearFlag(MDR_TIMER2, TIMER_STATUS_CCR_REF_CH2);
	//-------------------------------//
	
	time_mark2 = DWT_Get();
	
	// Update time
	ticks_count = DWT_GetDelta(time_mark1, time_mark2);
	if (ticks_count > time_profile.max_ticks_in_Timer2_ISR)
		time_profile.max_ticks_in_Timer2_ISR = ticks_count;
}




