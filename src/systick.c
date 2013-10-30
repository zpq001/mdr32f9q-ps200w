

#include "systick.h"
#include "MDR32Fx.h"                      /* MDR32F9x definitions            */
#include "MDR32F9Qx_timer.h"

#include "encoder.h"
#include "systemfunc.h"
#include "control.h"
#include "converter.h"

/* —четчик */
volatile uint32_t sysTicks = 0;
uint32_t ticks_per_us;
uint16_t beep_cnt;




/*----------------------------------------------------------------------------
  SysTick_Handler
 *----------------------------------------------------------------------------*/
 /*
void SysTick_Handler(void) {
  sysTicks++;                               // inc. counter necessary in Delay
	ProcessEncoder();
	if ( beep_cnt == 1 )
		StopBeep();
	if(	beep_cnt )
		beep_cnt--;
	
	ProcessPowerOff();				// Serves AC line disconnect
	Converter_HW_ADCProcess();		// ADC functions
	Converter_HWProcess();			// Converter ON/OFF control and overload handling
}
*/

/* ”правление системным таймером */
void SysTickStart(uint32_t ticks) {
	if (ticks > 0x00FFFFFF) ticks = 0x00FFFFFF;
	// call function from core_cm3.c
	SysTick_Config(ticks);
}

void SysTickStop() {
   SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}


/*----------------------------------------------------------------------------
  delays number of tick Systicks
 *----------------------------------------------------------------------------*/
void SysTickDelay (uint32_t dlyTicks) {                                              
  uint32_t nextTicks = sysTicks + dlyTicks;

	while(sysTicks != nextTicks);
}


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



