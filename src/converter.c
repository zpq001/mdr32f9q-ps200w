/********************************************************************
	Module		Converter
	
	Purpose:
		
					
	Globals used:

		
	Operation: FIXME

********************************************************************/

#include "MDR32Fx.h" 
#include "MDR32F9Qx_port.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "systemfunc.h"
#include "led.h"
#include "adc.h"
#include "defines.h"
#include "control.h"
#include "converter.h"
#include "sound_driver.h"

#include "guiTop.h"


// Globals used for communicating with converter control task called from ISR
volatile uint8_t state_HWProcess = STATE_HW_OFF;	
volatile uint8_t ctrl_HWProcess = 0;

volatile uint8_t cmd_ADC_to_HWProcess = 0;


static uint32_t gui_msg;


converter_regulation_t channel_5v_setting;
converter_regulation_t channel_12v_setting;
converter_regulation_t *regulation_setting_p;







const conveter_message_t converter_tick_message = 	{	CONVERTER_TICK, 0, 0 };
const conveter_message_t converter_update_message = {	CONVERTER_UPDATE, 0, 0 };
//const conveter_message_t converter_init_message = {	CONVERTER_INITIALIZE, 0, 0 };


xQueueHandle xQueueConverter;


static void apply_regulation(void);
static uint16_t CheckSetVoltageRange(int32_t new_set_voltage, uint8_t *err_code);
static uint16_t CheckSetCurrentRange(int32_t new_set_current, uint8_t *err_code);






uint32_t conv_state = CONV_OFF;




static uint16_t CheckSetVoltageRange(int32_t new_set_voltage, uint8_t *err_code)
{
	uint8_t error = VCHECK_OK;

	// First check soft limits
	if (regulation_setting_p->soft_voltage_range_enable)
	{
		if (new_set_voltage <= (int32_t)regulation_setting_p->soft_min_voltage)
		{
			new_set_voltage = (int32_t)regulation_setting_p->soft_min_voltage;
			error = VCHECK_SOFT_MIN;
		}
		else if (new_set_voltage >= (int32_t)regulation_setting_p->soft_max_voltage)
		{
			new_set_voltage = (int32_t)regulation_setting_p->soft_max_voltage;
			error = VCHECK_SOFT_MAX;
		}
	}
	
	// Check absolute limits
	if (new_set_voltage <= (int32_t)regulation_setting_p->MIN_VOLTAGE)
	{
		new_set_voltage = (int32_t)regulation_setting_p->MIN_VOLTAGE;
		error = VCHECK_ABS_MIN;
	}
	else if (new_set_voltage >= (int32_t)regulation_setting_p->MAX_VOLTAGE)
	{
		new_set_voltage = (int32_t)regulation_setting_p->MAX_VOLTAGE;
		error = VCHECK_ABS_MAX;
	}
	
	*err_code = error;
	return (uint16_t)new_set_voltage;
}


static uint16_t CheckSetCurrentRange(int32_t new_set_current, uint8_t *err_code)
{
	int32_t max_current;
	int32_t min_current;	
	uint8_t error = CCHECK_OK;
	
	if (regulation_setting_p->current_limit == CURRENT_LIM_HIGH)
	{
		max_current = regulation_setting_p->HIGH_LIM_MAX_CURRENT;
		min_current = regulation_setting_p->HIGH_LIM_MIN_CURRENT;
	}
	else
	{
		max_current = regulation_setting_p->LOW_LIM_MAX_CURRENT;
		min_current = regulation_setting_p->LOW_LIM_MIN_CURRENT;
	}
	
	// First check soft limits
	if (regulation_setting_p->soft_current_range_enable)
	{
		if (new_set_current <= (int32_t)regulation_setting_p->soft_min_current)
		{
			new_set_current = (int32_t)regulation_setting_p->soft_min_current;
			error = CCHECK_SOFT_MIN;
		}
		else if (new_set_current >= (int32_t)regulation_setting_p->soft_max_current)
		{
			new_set_current = (int32_t)regulation_setting_p->soft_max_current;
			error = CCHECK_SOFT_MAX;
		}	
	}
	
	// Check absolute limits - this will overwrite possibly incorrect soft limits
	// This can happen for exapmle, if soft_min = 35A and current limit is switched to 20A
	if (new_set_current <= min_current)
	{
		new_set_current = min_current;
		error = CCHECK_ABS_MIN;
	}
	else if (new_set_current >= max_current)
	{
		new_set_current = max_current;
		error = CCHECK_ABS_MAX;
	}
	
	*err_code = error;
	return (uint16_t)new_set_current;
}


uint8_t Converter_SetSoftLimit(int32_t new_limit, converter_regulation_t *reg_p, uint8_t mode)
{
	uint8_t err_code = SLIM_OK;
	uint16_t *lim_p;
	int32_t minimum;
	int32_t maximum;
	
	switch (mode)
	{
		case SET_LOW_CURRENT_SOFT_LIMIT:
			lim_p = &reg_p->soft_min_current;
			minimum = reg_p->SOFT_MIN_CURRENT_LIMIT;
			maximum = reg_p->soft_max_current;
			break;
		case SET_HIGH_CURRENT_SOFT_LIMIT:
			lim_p = &reg_p->soft_max_current;
			minimum = reg_p->soft_min_current;
			maximum = reg_p->SOFT_MAX_CURRENT_LIMIT;
			break;
		case SET_LOW_VOLTAGE_SOFT_LIMIT:
			lim_p = &reg_p->soft_min_voltage;
			minimum = reg_p->SOFT_MIN_VOLTAGE_LIMIT;
			maximum = reg_p->soft_max_voltage;
			break;
		default: // SET_HIGH_VOLTAGE_SOFT_LIMIT:
			lim_p = &reg_p->soft_max_voltage;
			minimum = reg_p->soft_min_voltage;
			maximum = reg_p->SOFT_MAX_VOLTAGE_LIMIT;
			break;
	}
	
	if (new_limit < minimum)
	{
		new_limit = minimum;
		err_code = SLIM_MIN;
	}
	else if (new_limit > maximum)
	{
		new_limit = maximum;
		err_code = SLIM_MAX;
	}
	*lim_p = (uint16_t)new_limit;
	
	return err_code;
}


static void apply_regulation(void)
{
	uint16_t temp;
	
	// Apply voltage - same for both 5V and 12V channels
	temp = regulation_setting_p -> set_voltage;
	temp /= 5;
	SetVoltagePWMPeriod(temp);		// FIXME - we are setting not period but duty
	
	// Apply current different for 20A and 40A limits
	temp = regulation_setting_p -> set_current;
	temp = (regulation_setting_p -> current_limit == CURRENT_LIM_HIGH) ? temp / 2 : temp;
	temp /= 5;
	SetCurrentPWMPeriod(temp);		// FIXME - we are setting not period but duty
}





// TODO: add regulation of overload parameters - different for each channel or common for both ?


void Converter_Init(uint8_t default_channel)
{
	// Converter is powered off.
	// TODO: add restore from EEPROM
	
	// Common
	channel_5v_setting.CHANNEL = CHANNEL_5V;
	channel_5v_setting.load_state = LOAD_ENABLE;							// dummy - load at 5V channel can not be disabled
	// Voltage
	channel_5v_setting.set_voltage = 5000;
	channel_5v_setting.MAX_VOLTAGE = CONV_MAX_VOLTAGE_5V_CHANNEL;			// Maximum voltage setting for channel
	channel_5v_setting.MIN_VOLTAGE = CONV_MIN_VOLTAGE_5V_CHANNEL;			// Minimum voltage setting for channel
	channel_5v_setting.soft_max_voltage = 8000;
	channel_5v_setting.soft_min_voltage = 3000;
	channel_5v_setting.SOFT_MAX_VOLTAGE_LIMIT = 10000;						// Maximum soft voltage limit
	channel_5v_setting.SOFT_MIN_VOLTAGE_LIMIT = 0;							// Minimum soft voltage limit
	channel_5v_setting.soft_voltage_range_enable = 0;
	// Current
	channel_5v_setting.current_limit = CURRENT_LIM_LOW;
	channel_5v_setting.set_current = 4000;
	channel_5v_setting.LOW_LIM_MAX_CURRENT = CONV_LOW_LIM_MAX_CURRENT;		// Low limit (20A) maximum current setting
	channel_5v_setting.LOW_LIM_MIN_CURRENT = CONV_MIN_CURRENT;				// Low limit (20A) min current setting
	channel_5v_setting.HIGH_LIM_MAX_CURRENT = CONV_HIGH_LIM_MAX_CURRENT;	// High limit (40A) maximum current setting
	channel_5v_setting.HIGH_LIM_MIN_CURRENT = CONV_MIN_CURRENT;				// High limit (40A) min current setting
	channel_5v_setting.soft_max_current = 37000;
	channel_5v_setting.soft_min_current = 30000;
	channel_5v_setting.SOFT_MAX_CURRENT_LIMIT = 40000;						// Maximum soft current limit
	channel_5v_setting.SOFT_MIN_CURRENT_LIMIT = 0;							// Minimum soft current limit
	channel_5v_setting.soft_current_range_enable = 0;
	
	
	
	// Common
	channel_12v_setting.CHANNEL = CHANNEL_12V;
	channel_12v_setting.load_state = LOAD_ENABLE;
	// Voltage
	channel_12v_setting.set_voltage = 12000;
	channel_12v_setting.MAX_VOLTAGE = CONV_MAX_VOLTAGE_12V_CHANNEL;			// Maximum voltage setting for channel
	channel_12v_setting.MIN_VOLTAGE = CONV_MIN_VOLTAGE_12V_CHANNEL;			// Minimum voltage setting for channel
	channel_12v_setting.soft_max_voltage = 16000;
	channel_12v_setting.soft_min_voltage = 1500;
	channel_12v_setting.SOFT_MAX_VOLTAGE_LIMIT = 20000;						// Maximum soft voltage limit
	channel_12v_setting.SOFT_MIN_VOLTAGE_LIMIT = 0;							// Minimum soft voltage limit
	channel_12v_setting.soft_voltage_range_enable = 0;
	// Current
	channel_12v_setting.current_limit = CURRENT_LIM_LOW;
	channel_12v_setting.set_current = 2000;
	channel_12v_setting.LOW_LIM_MAX_CURRENT = CONV_LOW_LIM_MAX_CURRENT;		// Low limit (20A) maximum current setting
	channel_12v_setting.LOW_LIM_MIN_CURRENT = CONV_MIN_CURRENT;				// Low limit (20A) min current setting
	channel_12v_setting.HIGH_LIM_MAX_CURRENT = CONV_LOW_LIM_MAX_CURRENT;	// High limit (40A) maximum current setting
	channel_12v_setting.HIGH_LIM_MIN_CURRENT = CONV_MIN_CURRENT;			// High limit (40A) min current setting
	channel_12v_setting.soft_max_current = 18000;
	channel_12v_setting.soft_min_current = 6000;
	channel_12v_setting.SOFT_MAX_CURRENT_LIMIT = 20000;						// Maximum soft current limit
	channel_12v_setting.SOFT_MIN_CURRENT_LIMIT = 0;							// Minimum soft current limit
	channel_12v_setting.soft_current_range_enable = 0;
	
	// Select default channel
	if (default_channel == CHANNEL_12V)
		regulation_setting_p = &channel_12v_setting;
	else
		regulation_setting_p = &channel_5v_setting;
	
	
	// Apply controls
	__disable_irq();
	SetFeedbackChannel(regulation_setting_p->CHANNEL);		// PORTF can be accessed from ISR
	__enable_irq();
	SetCurrentLimit(regulation_setting_p->current_limit);
	SetOutputLoad(regulation_setting_p->load_state);
	apply_regulation();										// Apply voltage and current settings
}





static uint32_t analyzeAndResetHWErrorState(void)
{
	uint32_t state_flags;
	if (state_HWProcess & STATE_HW_OVERLOADED)		
	{
		ctrl_HWProcess = CMD_HW_RESET_OVERLOAD;		
		while(ctrl_HWProcess);
		state_flags = CONV_OVERLOAD;	
	}
	// Add more if necessary
	else
	{
		state_flags = 0;
	}
	return state_flags;
}

static uint32_t disableConverterAndCheckHWState(void)
{
	uint32_t new_state;
	
#if CMD_HAS_PRIORITY == 1
	// If error status is generated simultaneously with OFF command,
	// converter will be turned off and no error status will be shown
	ctrl_HWProcess = CMD_HW_OFF | CMD_HW_RESET_OVERLOAD;	// Turn off converter and suppress error status (if any)
	while(ctrl_HWProcess);
	new_state = CONV_OFF;		
	
#elif ERROR_HAS_PRIORITY == 1
	// If error status is generated simultaneously with OFF command,
	// converter will be turned off, but error status will be indicated
	ctrl_HWProcess = CMD_HW_OFF;					// Turn converter off
	while(ctrl_HWProcess);
	new_state = CONV_OFF;	
	new_state |= analyzeAndResetHWErrorState();	
						
#endif
	return new_state;
}



//---------------------------------------------//
//	Main converter task
//	
//---------------------------------------------//
void vTaskConverter(void *pvParameters) 
{
	conveter_message_t msg;
	
	uint8_t err_code;
	uint32_t adc_msg;
	uint32_t sound_msg;
	
	// Initialize
	xQueueConverter = xQueueCreate( 5, sizeof( conveter_message_t ) );		// Queue can contain 5 elements of type conveter_message_t
	if( xQueueConverter == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	// Wait until task is started by dispatcher
	vTaskSuspend(NULL);			
	xQueueReset(xQueueConverter);
	
	
	while(1)
	{
		xQueueReceive(xQueueConverter, &msg, portMAX_DELAY);
	
		// CHECKME - Possibly need to add a mechanism to protect against illegal messages ?
		// For example, GUI sends message to set new voltage, but converter has switched channel by comand from UART.
		// In this case, command from GUI is old and bad and should be ignored somehow.

		switch (msg.type)
		{
			case CONVERTER_SET_VOLTAGE:
				regulation_setting_p->set_voltage = CheckSetVoltageRange(msg.data_a, &err_code);
				//----- Send notification to GUI -----//
				gui_msg = GUI_TASK_UPDATE_VOLTAGE_SETTING;
				xQueueSendToFront(xQueueGUI, &gui_msg, 0);
				//------------------------------------//
				if ((err_code == VCHECK_ABS_MAX) || (err_code == VCHECK_ABS_MIN))
					sound_msg = SND_CONV_SETTING_ILLEGAL;
				else if ((err_code == VCHECK_SOFT_MAX) || (err_code == VCHECK_SOFT_MIN))
					sound_msg = SND_CONV_SETTING_ILLEGAL;
				else
					sound_msg = SND_CONV_SETTING_OK;
				sound_msg |= SND_CONVERTER_PRIORITY_NORMAL;
				xQueueSendToBack(xQueueSound, &sound_msg, 0);
				break;
			case CONVERTER_SET_CURRENT:
				regulation_setting_p->set_current = CheckSetCurrentRange(msg.data_a, &err_code);
				//----- Send notification to GUI -----//
				gui_msg = GUI_TASK_UPDATE_CURRENT_SETTING;
				xQueueSendToFront(xQueueGUI, &gui_msg, 0);
				//------------------------------------//
				if ((err_code == CCHECK_ABS_MAX) || (err_code == CCHECK_ABS_MIN))
					sound_msg = SND_CONV_SETTING_ILLEGAL;
				else if ((err_code == CCHECK_SOFT_MAX) || (err_code == CCHECK_SOFT_MIN))
					sound_msg = SND_CONV_SETTING_ILLEGAL;
				else
					sound_msg = SND_CONV_SETTING_OK;
				sound_msg |= SND_CONVERTER_PRIORITY_NORMAL;
				xQueueSendToBack(xQueueSound, &sound_msg, 0);
				break;
		}
		

		
		switch(conv_state & CONV_STATE_MASK)
		{
			case CONV_OFF:
				if (msg.type == CONVERTER_SWITCH_TO_5VCH)
				{
					regulation_setting_p = &channel_5v_setting;
					ctrl_HWProcess = CMD_HW_RESTART_USER_TIMER;
					//----- Send notification to GUI -----//
					gui_msg = GUI_TASK_UPDATE_FEEDBACK_CHANNEL;
					xQueueSendToFront(xQueueGUI, &gui_msg, 0);
					//------------------------------------//
					while(ctrl_HWProcess);
					break;
				}
				if (msg.type == CONVERTER_SWITCH_TO_12VCH)
				{
					regulation_setting_p = &channel_12v_setting;
					ctrl_HWProcess = CMD_HW_RESTART_USER_TIMER;
					//----- Send notification to GUI -----//
					gui_msg = GUI_TASK_UPDATE_FEEDBACK_CHANNEL;
					xQueueSendToFront(xQueueGUI, &gui_msg, 0);
					//------------------------------------//
					while(ctrl_HWProcess);
					break;
				}
				if (msg.type == SET_CURRENT_LIMIT_20A)
				{
					regulation_setting_p -> current_limit = CURRENT_LIM_LOW;
					ctrl_HWProcess = CMD_HW_RESTART_USER_TIMER;
					//----- Send notification to GUI -----//
					gui_msg = GUI_TASK_UPDATE_CURRENT_LIMIT;
					xQueueSendToFront(xQueueGUI, &gui_msg, 0);
					//------------------------------------//
					while(ctrl_HWProcess);
					break;
				}
				if (msg.type == SET_CURRENT_LIMIT_40A)
				{
					regulation_setting_p -> current_limit = CURRENT_LIM_HIGH;
					ctrl_HWProcess = CMD_HW_RESTART_USER_TIMER;
					//----- Send notification to GUI -----//
					gui_msg = GUI_TASK_UPDATE_CURRENT_LIMIT;
					xQueueSendToFront(xQueueGUI, &gui_msg, 0);
					//------------------------------------//
					while(ctrl_HWProcess);
					break;
				}
				if (msg.type == CONVERTER_TURN_OFF)
				{
					conv_state = CONV_OFF;			// Reset overload flag
					break;
				}
				if (msg.type == CONVERTER_TURN_ON)
				{
					// Message to turn on converter is received
					if ((state_HWProcess & STATE_HW_TIMER_NOT_EXPIRED) || (!(state_HWProcess & STATE_HW_USER_TIMER_EXPIRED)))
					{
						// Safe timeout is not expired
						break;
					}
					ctrl_HWProcess = CMD_HW_ON;
					while(ctrl_HWProcess);
					conv_state = CONV_ON;			// Switch to new state and reset overload flag
					break;
				}
				break;
			case CONV_ON:
				if (msg.type == CONVERTER_TURN_OFF)
				{
					conv_state = disableConverterAndCheckHWState();
					break;
				}
				if ( (msg.type == CONVERTER_SWITCH_TO_5VCH) && (regulation_setting_p != &channel_5v_setting) )
				{
					conv_state = disableConverterAndCheckHWState();
					vTaskDelay(4);
					regulation_setting_p = &channel_5v_setting;
					ctrl_HWProcess = CMD_HW_RESTART_USER_TIMER;
					//----- Send notification to GUI -----//
					gui_msg = GUI_TASK_UPDATE_FEEDBACK_CHANNEL;
					xQueueSendToFront(xQueueGUI, &gui_msg, 0);
					//------------------------------------//
					while(ctrl_HWProcess);
					break;
				}
				if ( (msg.type == CONVERTER_SWITCH_TO_12VCH) && (regulation_setting_p != &channel_12v_setting) )
				{
					conv_state = disableConverterAndCheckHWState();
					vTaskDelay(4);
					regulation_setting_p = &channel_5v_setting;
					ctrl_HWProcess = CMD_HW_RESTART_USER_TIMER;
					//----- Send notification to GUI -----//
					gui_msg = GUI_TASK_UPDATE_FEEDBACK_CHANNEL;
					xQueueSendToFront(xQueueGUI, &gui_msg, 0);
					//------------------------------------//
					while(ctrl_HWProcess);
					break;
				}
				if (state_HWProcess & STATE_HW_OFF)
				{
					// Some hardware error has happened and converter had been switched off
					conv_state = CONV_OFF;
					conv_state |= analyzeAndResetHWErrorState();
					if (conv_state & CONV_OVERLOAD)
					{
						// Send message to sound driver
						// TODO: send this message in other cases (when overload is detected simultaneously with button)
						sound_msg = SND_CONV_OVERLOADED | SND_CONVERTER_PRIORITY_HIGHEST;
						xQueueSendToBack(xQueueSound, &sound_msg, 0);
					}
					break;
				}
				break;
		}
		
		
		// Will be special for charging mode - TODO
		if (msg.type == CONVERTER_TICK)
		{
			// ADC task is responsible for sampling and filtering voltage and current
			adc_msg = ADC_GET_ALL_NORMAL;
			xQueueSendToBack(xQueueADC, &adc_msg, 0);
		}			
		
		// Apply controls
		__disable_irq();
		SetFeedbackChannel(regulation_setting_p->CHANNEL);		// PORTF can be accessed from ISR
		__enable_irq();
		SetCurrentLimit(regulation_setting_p->current_limit);
		SetOutputLoad(channel_12v_setting.load_state);
	
		// Always make sure settings are within allowed range
		regulation_setting_p->set_current = CheckSetCurrentRange((int32_t)regulation_setting_p->set_current, &err_code);
		regulation_setting_p->set_voltage = CheckSetVoltageRange((int32_t)regulation_setting_p->set_voltage, &err_code);

		// Apply voltage and current settings
		apply_regulation();		
		
	
		// LED indication

	}
	
}




//=================================================================//
//=================================================================//
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
	static uint16_t user_counter = 0;
	static uint16_t led_blink_counter = 0;
	static uint16_t overload_warning_counter = 0;
	uint8_t overload_check_enable;
	uint8_t raw_overload_flag;
	uint8_t led_state;

	//-------------------------------//
	// Get converter status and process overload timers
	
	// Due to hardware specialty overload input is active when converter is powered off
	// Overload timeout counter reaches 0 when converter has been enabled for OVERLOAD_IGNORE_TIMEOUT ticks
	overload_check_enable = 0;
	if (state_HWProcess & (STATE_HW_OFF | STATE_HW_OFF_BY_ADC))
		overload_ignore_counter = OVERLOAD_IGNORE_TIMEOUT;
	else if (overload_ignore_counter != 0)
		overload_ignore_counter--;
	else
		overload_check_enable = 1;
	
	// Apply top-level overload check control
	//if (__overload_functions_disabled__)
	//	overload_check_enable = 0;
	
	if (overload_check_enable)
		raw_overload_flag = GetOverloadStatus();
	else
		raw_overload_flag = NORMAL;
	
	if (raw_overload_flag == NORMAL)
		overload_counter = OVERLOAD_TIMEOUT;
	else if (overload_counter != 0)
		overload_counter--;
	

	//-------------------------------//
	// Check overload 	
	if (overload_counter == 0)
	{
		// Converter is overloaded
		state_HWProcess &= ~STATE_HW_ON;
		state_HWProcess |= STATE_HW_OFF | STATE_HW_OVERLOADED;	// Set status for itself and top-level software
		safe_counter = MINIMAL_OFF_TIME;						// Start timer to provide minimal OFF timeout
		xQueueSendToFrontFromISR(xQueueConverter, &converter_update_message, 0);	// No need for exact timing
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
		safe_counter = MINIMAL_OFF_TIME;						// Start timer to provide minimal OFF timeout
	}
	else if ( (ctrl_HWProcess & CMD_HW_ON) && (!(state_HWProcess & STATE_HW_ON)) )
	{
		state_HWProcess &= ~STATE_HW_OFF;
		state_HWProcess |= STATE_HW_ON;								
	}
	if (ctrl_HWProcess & CMD_HW_RESTART_USER_TIMER)
	{
		user_counter = USER_TIMEOUT;
	}
	if (ctrl_HWProcess & CMD_HW_RESTART_LED_BLINK_TIMER)
	{
		user_counter = LED_BLINK_TIMEOUT;
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

	//-------------------------------//
	// Apply converter state
	// TODO - check IRQ disable while accessing converter control port (MDR_PORTF) for write
	if (state_HWProcess & (STATE_HW_OFF | STATE_HW_OFF_BY_ADC))
		SetConverterState(CONVERTER_OFF);
	else if (state_HWProcess & STATE_HW_ON)
		SetConverterState(CONVERTER_ON);
		
	//-------------------------------//
	// LED indication
	// Uses raw converter state and top-level conveter status
	// TODO - check IRQ disable while accessing LED port (MDR_PORTB) for write
	led_state = 0;
	if ((state_HWProcess & STATE_HW_ON) && (led_blink_counter == 0))
		led_state |= LED_GREEN;
	if ((raw_overload_flag == OVERLOAD) || (conv_state & CONV_OVERLOAD))
		led_state |= LED_RED;
	UpdateLEDs(led_state);
	
	//-------------------------------//
	// Overload sound warning
	if ((raw_overload_flag == OVERLOAD) && (overload_warning_counter == 0))
	{
		xQueueSendToFrontFromISR(xQueueSound, &sound_instant_overload_msg, 0);	// No need for exact timing
		overload_warning_counter = OVERLOAD_WARNING_TIMEOUT;
	}
	
	//-------------------------------//
	// Process timers
	
	// Process safe timer - used by top-level controller to provide a safe minimal OFF timeout
	if (safe_counter != 0)
	{
		safe_counter--;
		state_HWProcess |= STATE_HW_TIMER_NOT_EXPIRED;
	}
	else
	{
		state_HWProcess &= ~STATE_HW_TIMER_NOT_EXPIRED;
	}
	
	// Process user timer - used by top-level controller to provide a safe interval after switching channels
	if (user_counter != 0)
	{
		user_counter--;
		state_HWProcess &= ~STATE_HW_USER_TIMER_EXPIRED;
	}
	else
	{
		state_HWProcess |= STATE_HW_USER_TIMER_EXPIRED;
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
}















