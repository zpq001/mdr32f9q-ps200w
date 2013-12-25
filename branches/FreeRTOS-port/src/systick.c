

#include "systick.h"
#include "MDR32Fx.h"                      /* MDR32F9x definitions            */
#include "MDR32F9Qx_timer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

//#include "gui_top.h"
#include "guiTop.h"

#include "encoder.h"
#include "systemfunc.h"
#include "control.h"
#include "converter.h"
#include "adc.h"
#include "dispatcher.h"
#include "uart.h"
#include "dwt_delay.h"
#include "sound_driver.h"

/* —четчик 
volatile uint32_t sysTicks = 0;
uint32_t ticks_per_us;
uint16_t beep_cnt;
*/

// Profiling
time_profile_t time_profile;


//-----------------------------------------------------------------//
// FreeRTOS tick hook - called from ISR
// T = 2ms
//-----------------------------------------------------------------//
void vApplicationTickHook( void )
{
	static uint32_t tmr_gui_update = GUI_UPDATE_INTERVAL;
	static uint32_t tmr_converter_tick = CONVERTER_TICK_INTERVAL;
	static uint32_t tmr_dispatcher_tick = DISPATCHER_TICK_INTERVAL;
	static uint32_t tmr_sound_driver_tick = SOUND_DRIVER_TICK_INTERVAL;
	portBASE_TYPE xHigherPriorityTaskWokenByPost;
	uint32_t msg;
	// Time profiling
	uint32_t time_mark = DWT_Get();
	uint32_t ticks_count;
		
	// We have not woken a task at the start of the ISR.
	xHigherPriorityTaskWokenByPost = pdFALSE;
	
	if (--tmr_gui_update == 0)
	{
		msg = GUI_TASK_REDRAW;
		xQueueSendToBackFromISR(xQueueGUI, &msg, &xHigherPriorityTaskWokenByPost);
		tmr_gui_update = GUI_UPDATE_INTERVAL;
	}
	
	if (--tmr_converter_tick == 0)
	{
		//msg = CONVERTER_TICK;
		xQueueSendToBackFromISR(xQueueConverter, &converter_tick_message, &xHigherPriorityTaskWokenByPost);
		tmr_converter_tick = CONVERTER_TICK_INTERVAL;
	}
	
	if (--tmr_dispatcher_tick == 0)
	{
		xQueueSendToBackFromISR(xQueueDispatcher, &dispatcher_tick_msg, &xHigherPriorityTaskWokenByPost);
		tmr_dispatcher_tick = DISPATCHER_TICK_INTERVAL;
	}
	
	if (--tmr_sound_driver_tick == 0)
	{
		xQueueSendToBackFromISR(xQueueSound, &sound_driver_sync_msg, &xHigherPriorityTaskWokenByPost);
		tmr_sound_driver_tick = SOUND_DRIVER_TICK_INTERVAL;
	}
	
	// Force context switching if required
	// CHECKME
	portEND_SWITCHING_ISR(xHigherPriorityTaskWokenByPost);
	
	// Update time
	ticks_count = DWT_GetDeltaForNow(time_mark);
	if (ticks_count > time_profile.max_ticks_in_Systick_hook)
		time_profile.max_ticks_in_Systick_hook = ticks_count;
}




/*----------------------------------------------------------------------------
  delays number of tick Systicks
 *----------------------------------------------------------------------------
void SysTickDelay (uint32_t dlyTicks) {                                              
  uint32_t nextTicks = sysTicks + dlyTicks;

	while(sysTicks != nextTicks);
}
*/
/*
void StartBeep(uint16_t time)
{
	// Enable channel outputs
	uint32_t temp = MDR_TIMER1->CH2_CNTRL1;
	temp &= ~( (3 << TIMER_CH_CNTRL1_SELO_Pos) | (3 << TIMER_CH_CNTRL1_NSELO_Pos));
	temp |= (TIMER_CH_OutSrc_REF << TIMER_CH_CNTRL1_SELO_Pos) | (TIMER_CH_OutSrc_REF << TIMER_CH_CNTRL1_NSELO_Pos);
	MDR_TIMER1->CH2_CNTRL1 = temp;
	// copy counter
	beep_cnt = time;
}


void StopBeep(void)
{
	uint32_t temp = MDR_TIMER1->CH2_CNTRL1;
	temp &= ~( (3 << TIMER_CH_CNTRL1_SELO_Pos) | (3 << TIMER_CH_CNTRL1_NSELO_Pos));
	temp |= (TIMER_CH_OutSrc_Only_1 << TIMER_CH_CNTRL1_SELO_Pos) | (TIMER_CH_OutSrc_Only_1 << TIMER_CH_CNTRL1_NSELO_Pos);
	MDR_TIMER1->CH2_CNTRL1 = temp;
}

void Beep(uint16_t time)
{
	StartBeep(time);
	while(beep_cnt);
}

void WaitBeep(void)
{
	while(beep_cnt);
}
*/



//---------------------------------------------//
//	Fast low-level control dispatcher
//
//	Timer2 is also used to generate PWM for voltage 
//  and current setting
//---------------------------------------------//
void Timer2_IRQHandler(void) 
{
	static uint16_t hw_adc_counter = HW_ADC_CALL_PERIOD;
	static uint16_t hw_uart2_rx_counter = HW_UART2_RX_CALL_PERIOD - 1;
	static uint16_t hw_uart2_tx_counter = HW_UART2_TX_CALL_PERIOD - 2;
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
	ProcessPowerOff();				// Check AC line disconnection
	if (--hw_adc_counter == 0)
	{
		hw_adc_counter = HW_ADC_CALL_PERIOD;
		//-------------------------------//
		// 286
		Converter_HW_ADCProcess();	// Converter low-level ADC control
		//-------------------------------//
	}
/*	
	//-------------------------------//
	// 2369
	if (--hw_uart2_rx_counter == 0)
	{
		hw_uart2_rx_counter = HW_UART2_RX_CALL_PERIOD;
		//-------------------------------//
		// 373
		processUartRX();			// UART2 receiver service
		//-------------------------------//
	}
	if (--hw_uart2_tx_counter == 0)
	{
		hw_uart2_tx_counter = HW_UART2_TX_CALL_PERIOD;
		time_mark1 = DWT_Get();
		//-------------------------------//
		// 2320
		processUartTX();			// UART2 transmitter service
		//-------------------------------//
		time_mark2 = DWT_Get();
	}
	//-------------------------------//
*/	
	
	//-------------------------------//
	// 608
	Converter_HWProcess();			// Converter low-level ON/OFF control and overload handling
	//-------------------------------//
	
	
	
	//-------------------------------//
	// 198
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




