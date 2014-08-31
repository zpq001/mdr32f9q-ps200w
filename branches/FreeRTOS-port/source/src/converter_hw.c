
#include "MDR32Fx.h" 

#include "FreeRTOS.h"

#include "systemfunc.h"
#include "control.h"
#include "led.h"
#include "sound_driver.h"
#include "converter.h"
#include "converter_hw.h"
#include "eeprom.h"
#include "dispatcher.h"


//-------------------------------------------------------//
// Special time parameters
#define OVERLOAD_IGNORE_TIMEOUT		(5*100)		// 100 ms	- delay after converter turn ON and first overload check
//#define OVERLOAD_TIMEOUT			5			// 1 ms		- moved to UI

#define HW_OFF_TIMEOUT				(5*15)		// 15 ms	- delay after switching converter OFF and other actions (channel switch, ON)
												//			  voltage and current DACs can be modified at any moment
#define HW_SETUP_TIMEOUT			(5*10)		// 10 ms	- delay after switching channel or current range and other actions
												
#define LED_BLINK_TIMEOUT			(5*100)		// 100ms	- used for charging indication
#define OVERLOAD_WARNING_TIMEOUT	(5*100)		// 100ms	- used for sound overload warning


//---------------------------------------------//
// Converter_HWProcess

// state_HWProcess bits 
#define STATE_HW_ON						0x01
#define STATE_HW_OFF					0x02
#define STATE_HW_OVERLOADED				0x04
#define STATE_HW_OFF_BY_ADC				0x08
#define STATE_HW_OFF_BY_SHUTDOWN		0x20
#define STATE_HW_SAFE_TIMER_EXPIRED		0x10

// ctrl_HWProcess bits
#define CMD_HW_ON						0x01
#define CMD_HW_OFF						0x02
#define CMD_HW_RESET_OVERLOAD			0x04
#define CMD_HW_RESTART_SETUP_TIMEOUT	0x08
#define CMD_HW_RESTART_LED_BLINK_TIMER 	0x10


volatile static uint8_t state_HWProcess = STATE_HW_OFF;
volatile static uint8_t ctrl_HWProcess = 0;
// Global used for communicating with converter control task called from ISR
volatile uint8_t cmd_ADC_to_HWProcess = 0;
volatile uint8_t cmd_shutdown_to_HWProcess = 0;

// Externs
extern converter_state_t converter_state;
extern const converter_message_t converter_overload_msg;
extern xQueueHandle xQueueConverter;


/*	
	DAC uses some values from global settings structure.
	global_settings->dac_voltage_offset			- offset for voltage DAC
	global_settings->dac_current_low_offset		- offset for current DAC, low current range
	global_settings->dac_current_high_offset	- offset for current DAC, high current range

	Using 12-bit PWM DAC.
*/


//---------------------------------------------//
// Input is [mV]
// DAC offset is [mV]
// DAC resolution is 0.005V
// Maximum voltage setting is 0.005 * 4095 = 20.475V
// Offset in units of 0.005V
//---------------------------------------------//
void SetVoltageDAC(uint16_t val)
{
	int32_t res_val;
	val /= 5;
	// Offset voltage DAC
	//res_val = (int32_t)val + dac_settings.voltage_offset;		
	res_val = (int32_t)val + (int32_t)global_settings->dac_voltage_offset / 5;		
	if (res_val < 0) res_val = 0;				// cannot offset
	else if (res_val > 4095) res_val = 4095;	// max limit
	val = (uint16_t)res_val;
	SetVoltagePWMPeriod(val);		// FIXME - we are setting not period but duty	
}

//---------------------------------------------//
// Input is [mA]
// DAC offset is [mA]
// DAC resolution is 0.005A for low range, 0.01A for high range
// Maximum current setting is 0.005 * 4095 = 20,475A or 0.01 * 4095 = 40.95A
// Offset in units of 0.005A for low range, 0.01A for high range
//---------------------------------------------//
void SetCurrentDAC(uint16_t val, uint8_t current_range)
{
	int32_t res_val;
	if (current_range == CURRENT_RANGE_HIGH)
	{
		val /= 10;
		// Offset current DAC
		res_val = (int32_t)val + (int32_t)global_settings->dac_current_low_offset / 10;		
		if (res_val < 0) res_val = 0;				// cannot offset
		else if (res_val > 4095) res_val = 4095;	// max limit
	}
	else
	{
		val /= 5;
		// Offset current DAC
		res_val = (int32_t)val + (int32_t)global_settings->dac_current_high_offset / 5;		
		if (res_val < 0) res_val = 0;				// cannot offset
		else if (res_val > 4095) res_val = 4095;	// max limit
	}
	val = (uint16_t)res_val;
	SetCurrentPWMPeriod(val);		// FIXME - we are setting not period but duty
}


uint8_t Converter_IsReady(void)
{
	return (state_HWProcess & STATE_HW_SAFE_TIMER_EXPIRED);
}






//---------------------------------------------------------------------------//
//	Additional checks are a kind of extra protection
//	
//---------------------------------------------------------------------------//
uint8_t Converter_SetFeedBackChannel(uint8_t channel)
{
	// Check converter state
	if (state_HWProcess & STATE_HW_ON)
	{
		// Converter is ON
		return CONVERTER_ILLEGAL_ON_STATE;
	}
	
	// Check safe timers
	if (!(state_HWProcess & STATE_HW_SAFE_TIMER_EXPIRED))
	{
		// Minimum timeout is not reached yet.
		return CONVERTER_IS_NOT_READY;
	}
	
	__disable_irq();
	SetFeedbackChannel(channel);		// PORTF can be accessed from ISR
	__enable_irq();
	
	// Start safe timer
	ctrl_HWProcess = CMD_HW_RESTART_SETUP_TIMEOUT;
	while(ctrl_HWProcess);
	
	return CONVERTER_CMD_OK;
}


//---------------------------------------------------------------------------//
//	Additional checks are a kind of extra protection
//
//---------------------------------------------------------------------------//
uint8_t Converter_SetCurrentRange(uint8_t range)
{
	// Check converter state
	if (state_HWProcess & STATE_HW_ON)
	{
		// Converter is ON
		return CONVERTER_ILLEGAL_ON_STATE;
	}
	// Check safe timers
	if (!(state_HWProcess & STATE_HW_SAFE_TIMER_EXPIRED))
	{
		// Minimum timeout is not reached yet.
		return CONVERTER_IS_NOT_READY;
	}
	
	SetCurrentRange(range);
	
	// Start safe timer
	ctrl_HWProcess = CMD_HW_RESTART_SETUP_TIMEOUT;
	while(ctrl_HWProcess);
	
	return CONVERTER_CMD_OK;
}





//---------------------------------------------------------------------------//
//	Additional checks are a kind of extra protection
//
//---------------------------------------------------------------------------//
uint8_t Converter_TurnOn(void)
{
	if (state_HWProcess & STATE_HW_ON)
	{
		// Converter is already ON
		return CONVERTER_CMD_OK;
	}
	// Converter is OFF. It is either OFF state by command or OFF state by overload.
	// Check if converter was not overloaded
	if (state_HWProcess & STATE_HW_OVERLOADED)
	{
		// Converter is overloaded - top level should call ClearOverloadFlag() function
		return CONVERTER_IS_OVERLOADED;
	}
	// Check safe timers
	if (!(state_HWProcess & STATE_HW_SAFE_TIMER_EXPIRED))
	{
		// Minimum timeout is not reached yet.
		return CONVERTER_IS_NOT_READY;
	}
	// Converter is OFF and can be safely turned ON.
	
	ctrl_HWProcess = CMD_HW_ON;
	while(ctrl_HWProcess);
	return CONVERTER_CMD_OK;
}


//---------------------------------------------------------------------------//
//
//---------------------------------------------------------------------------//
uint8_t Converter_TurnOff(void)
{
	if (state_HWProcess & STATE_HW_OFF)
	{
		// Converter is already OFF
		return CONVERTER_CMD_OK;
	}
	// Converter is ON. Send command to turn OFF. 
	// This command can be processed just after overload detection in a normal way - it does not clear overload flag.
	ctrl_HWProcess = CMD_HW_OFF;
	while(ctrl_HWProcess);
	return CONVERTER_CMD_OK;
}


//---------------------------------------------------------------------------//
//	Additional checks are a kind of extra protection
//
//---------------------------------------------------------------------------//
uint8_t Converter_ClearOverloadFlag(void)
{
	if (state_HWProcess & STATE_HW_ON)
	{
		// Converter is already ON and this function should not have been called.
		return CONVERTER_IS_NOT_READY;
	}
	// Converter is OFF. Clear overload flag in case it is set.
	ctrl_HWProcess = CMD_HW_RESET_OVERLOAD;		
	while(ctrl_HWProcess);
	
	return CONVERTER_CMD_OK;
}







//=================================================================//
//=================================================================//
//          L O W  -  L E V E L   P R O C E S S I N G              //
//=================================================================//
//=================================================================//




//---------------------------------------------//
//	Converter hardware Enable/Disable control task
//	
//	Low-level task for accessing Enbale/Disable functions.
//	There may be an output overload which has to be correctly handled - converter should be disabled
//	The routine takes care for overload and enable/disable control coming from top-level controllers
//	
// TODO: ensure that ISR is non-interruptable
//---------------------------------------------//
void Converter_HWProcess(void) 
{
	static uint16_t overload_ignore_counter;
	static uint16_t overload_counter;
	static uint16_t safe_counter = 0;
	static uint16_t led_blink_counter = 0;
	static uint16_t overload_warning_counter = 0;
	uint8_t raw_overload_flag;
	uint8_t led_state;
	portBASE_TYPE xHigherPriorityTaskWokenByPost = pdFALSE;
	
	// First check shutdown
	if ((GetACLineStatus() == OFFLINE) && ((state_HWProcess & STATE_HW_OFF_BY_SHUTDOWN) == 0))
	{	
		if (xQueueDispatcher != 0)
		{
			// Notify top-level controller
			if (xQueueSendToFrontFromISR(xQueueDispatcher, &dispatcher_shutdown_msg, &xHigherPriorityTaskWokenByPost) == pdPASS)
			{
				// Send message just once
				state_HWProcess |= STATE_HW_OFF_BY_SHUTDOWN;
			}
		}
	}
	
		
	//-------------------------------//
	// Get converter status and process overload timers
	
	// Due to hardware specialty overload input is active when converter is powered off
	// Overload timeout counter reaches 0 when converter has been enabled for OVERLOAD_IGNORE_TIMEOUT ticks
	if ((state_HWProcess & (STATE_HW_OFF | STATE_HW_OFF_BY_ADC | STATE_HW_OFF_BY_SHUTDOWN)) || 0/*(converter_state.overload_protection_enable == 0)*/)
		overload_ignore_counter = OVERLOAD_IGNORE_TIMEOUT;
	else if (overload_ignore_counter != 0)
		overload_ignore_counter--;
		
	if (overload_ignore_counter == 0)
		raw_overload_flag = GetOverloadStatus();
	else
		raw_overload_flag = NORMAL;
	
	if (raw_overload_flag == NORMAL)
		overload_counter = converter_state.overload_threshold / 2;
	else if (overload_counter != 0)
		overload_counter--;
	

	//-------------------------------//
	// Overload sound warning
	// overload_warning_counter is used to provide some minimal time between succesive warnings
	if ((raw_overload_flag == OVERLOAD) && (overload_warning_counter == 0) && (converter_state.overload_warning_enable))
	{
		xQueueSendToFrontFromISR(xQueueSound, &sound_instant_overload_msg, 0);	// No need for exact timing
		overload_warning_counter = OVERLOAD_WARNING_TIMEOUT;
	}
	
	//-------------------------------//
	// Check if converter should be disabled
	// overload_counter is used for specified time threshold
	if ((raw_overload_flag == OVERLOAD) && (overload_counter == 0) && (converter_state.overload_protection_enable))
	{
		// Converter is overloaded
		state_HWProcess &= ~STATE_HW_ON;
		state_HWProcess |= STATE_HW_OFF | STATE_HW_OVERLOADED;	// Set status for itself and top-level software
		safe_counter = HW_OFF_TIMEOUT;							// Start timer to provide minimal OFF timeout
		xQueueSendToFrontFromISR(xQueueConverter, &converter_overload_msg, 0);	// No need for exact timing
	}
	
	//-------------------------------//
	// Process commands from top-level converter controller
	if (ctrl_HWProcess & CMD_HW_RESET_OVERLOAD)
	{
		state_HWProcess &= ~STATE_HW_OVERLOADED;
	}
	if ( (ctrl_HWProcess & CMD_HW_OFF) && (!(state_HWProcess & STATE_HW_OFF)) )
	{
		state_HWProcess &= ~STATE_HW_ON;
		state_HWProcess |= STATE_HW_OFF;		
		safe_counter = HW_OFF_TIMEOUT;							// Start timer to provide minimal OFF timeout
	}
	else if ( (ctrl_HWProcess & CMD_HW_ON) && (!(state_HWProcess & STATE_HW_ON)) && (!(state_HWProcess & STATE_HW_OVERLOADED)) )
	{
		state_HWProcess &= ~STATE_HW_OFF;
		state_HWProcess |= STATE_HW_ON;								
	}
	//-----------------------//
	if (ctrl_HWProcess & CMD_HW_RESTART_SETUP_TIMEOUT)
	{
		safe_counter = HW_SETUP_TIMEOUT;
	}
	
	//-----------------------//
	if (ctrl_HWProcess & CMD_HW_RESTART_LED_BLINK_TIMER)
	{
		led_blink_counter = LED_BLINK_TIMEOUT;
	} 
	
	//-------------------------------//
	// Process commands from top-level ADC controller
	if (cmd_ADC_to_HWProcess & CMD_HW_OFF_BY_ADC)
	{
		state_HWProcess |= STATE_HW_OFF_BY_ADC;
	}
	else if (cmd_ADC_to_HWProcess & CMD_HW_ON_BY_ADC)
	{
		state_HWProcess &= ~STATE_HW_OFF_BY_ADC;
	}
	
	// Reset commands
	ctrl_HWProcess = 0;
	cmd_ADC_to_HWProcess = 0;
	cmd_shutdown_to_HWProcess = 0;

	//-------------------------------//
	// Apply converter state
	// TODO - check IRQ disable while accessing converter control port (MDR_PORTF) for write
	if (state_HWProcess & (STATE_HW_OFF | STATE_HW_OFF_BY_ADC | STATE_HW_OFF_BY_SHUTDOWN))
		SetConverterState(CONVERTER_OFF);
	else if (state_HWProcess & STATE_HW_ON)
		SetConverterState(CONVERTER_ON);
		
	//-------------------------------//
	// LED indication
	// Uses raw converter state and top-level conveter status
	// TODO - check IRQ disable while accessing LED port (MDR_PORTB) for write
	led_state = 0;
	if (((state_HWProcess & (STATE_HW_ON | STATE_HW_OFF_BY_SHUTDOWN)) == STATE_HW_ON) && (led_blink_counter == 0))
		led_state |= LED_GREEN;
	if ((raw_overload_flag == OVERLOAD) || (converter_state.state == CONVERTER_STATE_OVERLOADED))
		led_state |= LED_RED;
	UpdateLEDs(led_state);
	
	
	
	//-------------------------------//
	// Process timers
	
	// Process safe timer - used by top-level controller to provide a minimal safe OFF timeout
	if (safe_counter != 0)
	{
		safe_counter--;
		state_HWProcess &= ~STATE_HW_SAFE_TIMER_EXPIRED;
	}
	else
	{
		state_HWProcess |= STATE_HW_SAFE_TIMER_EXPIRED;
	}
	
	
	// Process LED blink timer
	if (led_blink_counter != 0)
	{
		led_blink_counter--;
	}
	
	// Process overload sound warning timer
	if (overload_warning_counter != 0)
	{
		overload_warning_counter--;
	}
	
	// Force context switching if required
	portEND_SWITCHING_ISR(xHigherPriorityTaskWokenByPost);
}










