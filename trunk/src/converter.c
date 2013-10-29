/********************************************************************
	Module		Converter
	
	Purpose:
		
					
	Globals used:

		
	Operation: FIXME

********************************************************************/

#include "MDR32Fx.h" 
#include "MDR32F9Qx_port.h"
#include "MDR32F9Qx_adc.h"

#include "systemfunc.h"
#include "led.h"

#include "defines.h"
#include "control.h"
#include "converter.h"

// Globals used for communicating with converter control task called from ISR
uint8_t state_HWProcess = STATE_HW_OFF;	
uint8_t ctrl_HWProcess = 0;
uint8_t ctrl_ADCProcess = 0;
uint8_t cmd_ADC_to_HWProcess = 0;



converter_regulation_t channel_5v_setting;
converter_regulation_t channel_12v_setting;
converter_regulation_t *regulation_setting_p;

uint8_t HW_request = 0;


uint16_t voltage_adc;	// [mV]
uint16_t current_adc;	// [mA]
uint32_t power_adc;		// [mW]

uint16_t adc_voltage_counts;	// [ADC counts]
uint16_t adc_current_counts;	// [ADC counts]


static uint16_t CheckSetVoltageRange(int32_t new_set_voltage, uint8_t *err_code)
{
	uint8_t error = VCHECK_OK;

	// First check soft limits
	if (regulation_setting_p->soft_voltage_range_enable)
	{
		if (new_set_voltage < (int32_t)regulation_setting_p->soft_min_voltage)
		{
			new_set_voltage = (int32_t)regulation_setting_p->soft_min_voltage;
			error = VCHECK_SOFT_MIN;
		}
		else if (new_set_voltage > (int32_t)regulation_setting_p->soft_max_voltage)
		{
			new_set_voltage = (int32_t)regulation_setting_p->soft_max_voltage;
			error = VCHECK_SOFT_MAX;
		}
	}
	
	// Check absolute limits
	if (new_set_voltage < (int32_t)regulation_setting_p->MIN_VOLTAGE)
	{
		new_set_voltage = (int32_t)regulation_setting_p->MIN_VOLTAGE;
		error = VCHECK_ABS_MIN;
	}
	else if (new_set_voltage > (int32_t)regulation_setting_p->MAX_VOLTAGE)
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
		if (new_set_current < (int32_t)regulation_setting_p->soft_min_current)
		{
			new_set_current = (int32_t)regulation_setting_p->soft_min_current;
			error = CCHECK_SOFT_MIN;
		}
		else if (new_set_current > (int32_t)regulation_setting_p->soft_max_current)
		{
			new_set_current = (int32_t)regulation_setting_p->soft_max_current;
			error = CCHECK_SOFT_MAX;
		}	
	}
	
	// Check absolute limits - this will overwrite possibly incorrect soft limits
	// This can happen for exapmle, if soft_min = 35A and current limit is switched to 20A
	if (new_set_current < min_current)
	{
		new_set_current = min_current;
		error = CCHECK_ABS_MIN;
	}
	else if (new_set_current > max_current)
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
		case SET_LOW_VOLTAGE_SOFT_LIMIT:
			lim_p = &reg_p->soft_min_voltage;
			minimum = reg_p->SOFT_MIN_VOLTAGE_LIMIT;
			maximum = reg_p->soft_max_voltage;
			break;
		case SET_HIGH_VOLTAGE_SOFT_LIMIT:
			lim_p = &reg_p->soft_max_voltage;
			minimum = reg_p->soft_min_voltage;
			maximum = reg_p->SOFT_MAX_VOLTAGE_LIMIT;
			break;
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



void Converter_ProcessADC(void)
{
	voltage_adc = adc_voltage_counts>>2;
	voltage_adc *= 5;
	
	current_adc = adc_current_counts>>2;
	if (regulation_setting_p -> current_limit == CURRENT_LIM_HIGH)
		current_adc *= 10;
	else
		current_adc *= 5;
	
	power_adc = voltage_adc * current_adc / 1000;
}

//---------------------------------------------//
//	Set the converter output voltage
//	
//---------------------------------------------//
uint8_t Converter_SetVoltage(int32_t new_voltage)
{
	uint8_t err_code;
	regulation_setting_p->set_voltage = CheckSetVoltageRange(new_voltage, &err_code);
	return err_code;
}

//---------------------------------------------//
//	Set the converter output current
//	
//---------------------------------------------//
uint8_t Converter_SetCurrent(int32_t new_current)
{
	uint8_t err_code;
	regulation_setting_p->set_current = CheckSetCurrentRange(new_current, &err_code);
	return err_code;
}


void Converter_SetFeedbackChannel(uint8_t new_channel)
{
	if (new_channel != regulation_setting_p->CHANNEL)
	{
		if (new_channel == CHANNEL_5V)
			HW_request |= CMD_FB_5V;
		else
			HW_request |= CMD_FB_12V;
	}
}


void Converter_SetCurrentLimit(uint8_t new_limit) 	
{
	if (new_limit != regulation_setting_p->current_limit)
	{
		if (new_limit == CURRENT_LIM_HIGH)
			HW_request |= CMD_CLIM_40A;
		else
			HW_request |= CMD_CLIM_20A;
	}
}


void Converter_Enable(void)
{
	HW_request |= CMD_ON;
}

void Converter_Disable(void)
{
	HW_request |= CMD_OFF;
}

void Converter_StartCharge(void)
{

}


void Converter_Init(void)
{
	// Converter is powered off.

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
	
	// 
	regulation_setting_p = &channel_12v_setting;

}



void Converter_Process(void)
{
	static uint8_t conv_state = CONV_OFF;
	uint8_t next_conv_state = conv_state;
	uint8_t HW_cmd = 0;
	uint8_t dummy_err_code;
	
	
	switch(conv_state)
	{
		case CONV_OFF:
			if (HW_request & CMD_FB_5V)
			{
				HW_request &= ~(CMD_FB_5V | CMD_FB_12V | CMD_CLIM_20A | CMD_CLIM_40A);
				regulation_setting_p = &channel_5v_setting;
				break;
			}
			if (HW_request & CMD_FB_12V)
			{
				HW_request &= ~(CMD_FB_12V | CMD_CLIM_20A | CMD_CLIM_40A);
				regulation_setting_p = &channel_12v_setting;
				break;
			}
			if (HW_request & CMD_CLIM_20A)
			{
				HW_request &= ~CMD_CLIM_20A;
				regulation_setting_p -> current_limit = CURRENT_LIM_LOW;				
				break;
			}
			if (HW_request & CMD_CLIM_40A)
			{
				HW_request &= ~CMD_CLIM_40A;
				regulation_setting_p -> current_limit = CURRENT_LIM_HIGH;
				break;
			}
			
			if (HW_request & CMD_OFF)
			{
				HW_request &= ~CMD_OFF;
				HW_cmd |= CMD_HW_RESET_OVERLOAD;
				break;
			}
			
			if (HW_request & CMD_ON)
			{
				HW_request &= ~CMD_ON;
				next_conv_state = CONV_ON;
				HW_cmd |= (CMD_HW_ON | CMD_HW_RESET_OVERLOAD);
				break;
			}
			
			HW_request &= ~CMD_OFF;
			break;
		case CONV_ON:
			HW_request &= ~(CMD_ON | CMD_CLIM_20A | CMD_CLIM_40A);
			
			if ( (state_HWProcess & STATE_HW_OFF) || (HW_request & CMD_OFF) )
			{
				HW_request &= ~CMD_OFF;
				HW_cmd |= CMD_HW_OFF;
				next_conv_state = CONV_OFF;
				break;
			}
			if (HW_request & (CMD_FB_5V | CMD_FB_12V))
			{
				HW_cmd |= CMD_HW_OFF;
				next_conv_state = CONV_OFF;
				break;
			}
			
			break;
	}
	
	conv_state = next_conv_state;
	
	
	// Apply controls
	__disable_irq();
	SetFeedbackChannel(regulation_setting_p->CHANNEL);		// PORTF can be accessed from ISR
	__enable_irq();
	SetCurrentLimit(regulation_setting_p->current_limit);
	//SetOutputLoad(regulation_setting_p->load_state);
	SetOutputLoad(channel_12v_setting.load_state);
	
	// Always make sure settings are within allowed range
	regulation_setting_p->set_current = CheckSetCurrentRange((int32_t)regulation_setting_p->set_current, &dummy_err_code);
	regulation_setting_p->set_voltage = CheckSetVoltageRange((int32_t)regulation_setting_p->set_voltage, &dummy_err_code);
	
	// Apply voltage and current settings
	apply_regulation();		
	
	// Send command to low-level task - must be atomic operations
	ctrl_ADCProcess = CMD_ADC_START_VOLTAGE | CMD_ADC_START_CURRENT;
	ctrl_HWProcess = HW_cmd;		
	
	
	// LED indication
	led_state = 0;
	if (state_HWProcess & STATE_HW_ON)
		led_state = LED_GREEN;
	else if (state_HWProcess & STATE_HW_OVERLOADED)
		led_state = LED_RED;
	UpdateLEDs();
}		




//---------------------------------------------//
//	Converter hardware Enable/Disable control task
//	
//	Low-level task for accessing Enbale/Disable functions.
//	There may be an output overload which has to be correctly handled - converter should be disabled
//	This task takes care for overload and top-level enable/disable control
//	
// TODO: ensure that ISR is non-interruplable
//---------------------------------------------//
void Converter_HWProcess(void) 
{
	static uint16_t overload_ignore_counter;
	static uint16_t overload_counter;
	

	// Due to hardware specialty overload input is active when converter is powered off
	if ( (state_HWProcess & (STATE_HW_OFF | STATE_HW_OFF_BY_ADC)) || (0/*_overload_functions_disabled_*/) )
		overload_ignore_counter = OVERLOAD_IGNORE_TIMEOUT;
	else if (overload_ignore_counter != 0)
		overload_ignore_counter--;
	
	// Overload timeout counter reaches 0 when converter has been enabled for OVERLOAD_IGNORE_TIMEOUT ticks
	if ( (overload_ignore_counter != 0) || (GetOverloadStatus() == OVERLOAD) )
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
	}
	
	// Process ON/OFF commands
	if (ctrl_HWProcess & CMD_HW_RESET_OVERLOAD)
	{
		state_HWProcess &= ~STATE_HW_OVERLOADED;
	}
	if ( (ctrl_HWProcess & CMD_HW_OFF) && (!(state_HWProcess & STATE_HW_OVERLOADED)) )
	{
		state_HWProcess &= ~STATE_HW_ON;
		state_HWProcess |= STATE_HW_OFF;				
	}
	else if ( (ctrl_HWProcess & CMD_HW_ON) && (!(state_HWProcess & STATE_HW_OVERLOADED)) )
	{
		state_HWProcess &= ~STATE_HW_OFF;
		state_HWProcess |= STATE_HW_ON;								
	}
	
	// Process ON/OFF commands from ADC controller
	if (cmd_ADC_to_HWProcess & CMD_HW_OFF_BY_ADC)
	{
		state_HWProcess |= STATE_HW_OFF_BY_ADC;
	}
	else if (cmd_ADC_to_HWProcess & CMD_HW_ON_BY_ADC)
	{
		state_HWProcess &= ~STATE_HW_OFF_BY_ADC;
	}
	
	
	
	// Reset command
	ctrl_HWProcess = 0;
	cmd_ADC_to_HWProcess = 0;
	
	
	// Apply converter state
	// FIXME: care must be taken when using portF - this code is called from ISR
	// ATOMIC access should be used
	if (state_HWProcess & (STATE_HW_OFF | STATE_HW_OFF_BY_ADC))
		SetConverterState(CONVERTER_OFF);
	else if (state_HWProcess & STATE_HW_ON)
		SetConverterState(CONVERTER_ON);
	
}






//---------------------------------------------//
//	Converter hardware ADC control task
//	
//	Low-level task for ADC start, normal or charging mode
//	
// TODO: ensure that ISR is non-interruptable
//---------------------------------------------//
void Converter_HW_ADCProcess(void)
{
	static uint8_t state_ADCProcess = STATE_ADC_IDLE;
	static uint8_t adc_cmd;
	static uint16_t voltage_samples;
	static uint16_t current_samples;
	static uint8_t adc_counter;
	
	
	// Process ADC FSM-based controller
	switch (state_ADCProcess)
	{
		case STATE_ADC_IDLE:
			adc_cmd = ctrl_ADCProcess;
			if (adc_cmd)
				state_ADCProcess = STATE_ADC_DISPATCH;
			break;
		case STATE_ADC_DISPATCH:
			if ((adc_cmd & (CMD_ADC_START_VOLTAGE | CMD_ADC_START_DISCON)) == CMD_ADC_START_VOLTAGE)
			{
				// Normal voltage measure 
				adc_cmd &= ~(CMD_ADC_START_VOLTAGE | CMD_ADC_START_DISCON);
				state_ADCProcess = STATE_ADC_NORMAL_START_U;
				voltage_samples = 0;
			}
			else if (adc_cmd & CMD_ADC_START_CURRENT)
			{
				// Current measure
				adc_cmd &= ~CMD_ADC_START_CURRENT;
				state_ADCProcess = STATE_ADC_START_I;
				current_samples = 0;
			}
			else
			{
				// Update global Voltage and Current
				adc_voltage_counts = voltage_samples;
				adc_current_counts = current_samples;
				state_ADCProcess = STATE_ADC_IDLE;
			}
			break;
		case STATE_ADC_NORMAL_START_U:
			// TODO: use DMA for this purpose
			ADC1_SetChannel(ADC_CHANNEL_VOLTAGE);
			adc_counter = 4;
			state_ADCProcess = STATE_ADC_NORMAL_REPEAT_U;
			break;
		case STATE_ADC_NORMAL_REPEAT_U:
			if (adc_counter < 4)
				voltage_samples += ADC1_GetResult();
			if (adc_counter != 0)
			{
				ADC1_Start();
				adc_counter--;
			}
			else
			{
				state_ADCProcess = STATE_ADC_DISPATCH;
			}
			break;
		case STATE_ADC_START_I:
			// TODO: use DMA for this purpose
			ADC1_SetChannel(ADC_CHANNEL_CURRENT);
			adc_counter = 4;
			state_ADCProcess = STATE_ADC_NORMAL_REPEAT_I;
			break;
		case STATE_ADC_NORMAL_REPEAT_I:
			if (adc_counter < 4)
				current_samples += ADC1_GetResult();
			ADC1_Start();
			if (adc_counter != 0)
			{
				ADC1_Start();
				adc_counter--;
			}
			else
			{
				state_ADCProcess = STATE_ADC_DISPATCH;
			}
			break;
		default:
			state_ADCProcess = STATE_ADC_IDLE;
			break;
	}
	
	
	ctrl_ADCProcess = 0;
	/*
		//...
		
		// Disable converter for ADC
		cmd_ADC_to_HWProcess = STATE_HW_OFF_BY_ADC;			// Will disallow converter operation
		
		//...
		
		// Enable converter for ADC
		cmd_ADC_to_HWProcess = CMD_HW_ON_BY_ADC;			// Will allow converter operation
	*/
	

}




