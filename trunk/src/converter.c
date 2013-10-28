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
uint8_t state_HWProcess = HW_OFF;	
uint8_t ctrl_HWProcess = 0;

converter_state_t converter_state;

converter_regulation_t channel_5v_setting;
converter_regulation_t channel_12v_setting;
converter_regulation_t *regulation_setting_p;

uint8_t HW_request = 0;


uint16_t voltage_adc;	// [mV]
uint16_t current_adc;	// [mA]
uint32_t power_adc;		// [mW]

uint16_t adc_voltage_counts;	// [ADC counts]
uint16_t adc_current_counts;	// [ADC counts]





static void apply_regulation(void)
{
	uint16_t temp;
	
	// Apply voltage - same for both 5V and 12V channels
	temp = regulation_setting_p -> set_voltage;
	temp /= 5;
	SetVoltagePWMPeriod(temp);		// FIXME - we are setting not period but duty
	
	// Apply current different for 20A and 40A limits
	temp = regulation_setting_p -> set_current;
	temp = (regulation_setting_p -> current_limit == CURRENT_LIM_MAX) ? temp / 2 : temp;
	temp /= 5;
	SetCurrentPWMPeriod(temp);		// FIXME - we are setting not period but duty
}



void Converter_ProcessADC(void)
{
	voltage_adc = adc_voltage_counts>>2;
	voltage_adc *= 5;
	
	current_adc = adc_current_counts>>2;
	if (regulation_setting_p -> current_limit == CURRENT_LIM_MAX)
		current_adc *= 10;
	else
		current_adc *= 5;
	
	power_adc = voltage_adc * current_adc / 1000;
}

//---------------------------------------------//
//	Set the converter output voltage
//	
//---------------------------------------------//
void Converter_SetVoltage(int32_t new_voltage)
{
	int32_t max_voltage = (converter_state.feedback_channel == CHANNEL_5V) ? 10000 : 20000;	// mV
	int32_t min_voltage = 0;
	
	if (new_voltage < min_voltage)
		regulation_setting_p -> set_voltage = (uint16_t)min_voltage;
	else if (new_voltage > max_voltage)
		regulation_setting_p -> set_voltage = (uint16_t)max_voltage;
	else
		regulation_setting_p -> set_voltage = (uint16_t)new_voltage;
}

//---------------------------------------------//
//	Set the converter output current
//	
//---------------------------------------------//
void Converter_SetCurrent(int32_t new_current)
{
	int32_t max_current = (regulation_setting_p -> current_limit == CURRENT_LIM_MAX) ? 40000 : 20000;	// mA
	int32_t min_current = 0;
	
	if (new_current < min_current)
		regulation_setting_p -> set_current = (uint16_t)min_current;
	else if (new_current > max_current)
		regulation_setting_p -> set_current = (uint16_t)max_current;
	else
		regulation_setting_p -> set_current = (uint16_t)new_current;
}


void Converter_SetFeedbackChannel(uint8_t new_channel)
{
	if (new_channel != converter_state.feedback_channel)
	{
		if (new_channel == CHANNEL_5V)
			HW_request |= CMD_FB_5V;
		else
			HW_request |= CMD_FB_12V;
	}
}


// FIXME: should be internal function for this module
void Converter_SetCurrentLimit(uint8_t new_limit) 	
{
	if ( (new_limit != regulation_setting_p->current_limit) && (converter_state.feedback_channel != CHANNEL_12V) )
	{
		if (new_limit == CURRENT_LIM_MAX)
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

	converter_state.feedback_channel = CHANNEL_12V;
	converter_state.load_state = LOAD_ENABLE;
	
	channel_5v_setting.current_limit = CURRENT_LIM_MIN;
	channel_5v_setting.set_voltage = 5000;
	channel_5v_setting.set_current = 4000;
	
	channel_12v_setting.current_limit = CURRENT_LIM_MIN;
	channel_12v_setting.set_voltage = 12000;
	channel_12v_setting.set_current = 2000;	

	regulation_setting_p = &channel_12v_setting;

}


// TODO: put feedback_channel into converter_regulation_t structure as const

void Converter_Process(void)
{
	static uint8_t conv_state = CONV_OFF;
	uint8_t next_conv_state = conv_state;
	uint8_t HW_cmd = 0;
	//uint8_t led_state;
	
	
	switch(conv_state)
	{
		case CONV_OFF:
			if (HW_request & CMD_FB_5V)
			{
				converter_state.feedback_channel = CHANNEL_5V;
				converter_state.load_state = LOAD_ENABLE;
				HW_request &= ~(CMD_FB_5V | CMD_FB_12V | CMD_CLIM_20A | CMD_CLIM_40A);
				regulation_setting_p = &channel_5v_setting;
				break;
			}
			if (HW_request & CMD_FB_12V)
			{
				converter_state.feedback_channel = CHANNEL_12V;
				converter_state.load_state = LOAD_ENABLE;
				HW_request &= ~(CMD_FB_12V | CMD_CLIM_20A | CMD_CLIM_40A);
				regulation_setting_p = &channel_12v_setting;
				break;
			}
			if (HW_request & CMD_CLIM_20A)
			{
				HW_request &= ~CMD_CLIM_20A;
				regulation_setting_p -> current_limit = CURRENT_LIM_MIN;
				break;
			}
			if (HW_request & CMD_CLIM_40A)
			{
				HW_request &= ~CMD_CLIM_40A;
				regulation_setting_p -> current_limit = CURRENT_LIM_MAX;
				break;
			}
			
			if (HW_request & CMD_OFF)
			{
				HW_request &= ~CMD_OFF;
				HW_cmd |= HW_RESET_OVERLOAD;
				break;
			}
			
			if (HW_request & CMD_ON)
			{
				HW_request &= ~CMD_ON;
				next_conv_state = CONV_ON;
				HW_cmd |= (HW_ON | HW_RESET_OVERLOAD);
				break;
			}
			
			HW_request &= ~CMD_OFF;
			break;
		case CONV_ON:
			HW_request &= ~(CMD_ON | CMD_CLIM_20A | CMD_CLIM_40A);
			
			if ( (state_HWProcess & HW_OFF) || (HW_request & CMD_OFF) )
			{
				HW_request &= ~CMD_OFF;
				HW_cmd |= HW_OFF;
				next_conv_state = CONV_OFF;
				break;
			}
			if (HW_request & (CMD_FB_5V | CMD_FB_12V))
			{
				HW_cmd |= HW_OFF;
				next_conv_state = CONV_OFF;
				break;
			}
			
			break;
	}
	
	conv_state = next_conv_state;
	
	
	// Apply controls
	__disable_irq();
	SetFeedbackChannel(converter_state.feedback_channel);	// PORTF can be accessed from ISR
	__enable_irq();
	SetCurrentLimit(regulation_setting_p->current_limit);
	SetOutputLoad(converter_state.load_state);
	
	// Apply voltage and current settings
	apply_regulation();		
	
	HW_cmd |= HW_START_ADC_VOLTAGE | HW_START_ADC_CURRENT;
	ctrl_HWProcess = HW_cmd;		// Atomic

	// LED indication
	led_state = 0;
	if (state_HWProcess & HW_ON)
		led_state = LED_GREEN;
	else if (state_HWProcess & HW_OVERLOADED)
		led_state = LED_RED;
	UpdateLEDs();
}		



/*
	// Measuring voltage while charging
	Converter_HWProcess(HW_OFF);
	wait();
	select_ADC_channel(Voltage);
	wait();
	ADC_start();
	wait();
	Voltage = ADC_read();
	Converter_HWProcess(HW_ON);
	 
	// Measuring current or voltage normally
	select_ADC_channel(Current);
	wait();
	ADC_start();
	wait();
	Current = ADC_read();
*/


//---------------------------------------------//
//	Converter hardware Enable/Disable control task
//	
//	Low-level task for accessing Enbale/Disable functions.
//	There may be an output overload which has to be correctly handled - converter should be disabled
//	This task takes care for overload and top-level enable/disable control
//	This task also starts Voltage/Current ADC operation
//---------------------------------------------//
void Converter_HWProcess(void) 
{
	static uint16_t overload_ignore_counter;
	static uint16_t overload_counter;
	
	static uint8_t adc_state = ADC_IDLE;
	uint8_t next_adc_state;
	static uint8_t adc_cmd;
	static uint16_t voltage_samples;
	static uint16_t current_samples;
	static uint8_t adc_counter;
	
	// Due to hardware specialty overload input is active when converter is powered off
	if ( (state_HWProcess & (HW_OFF | HW_OFF_BY_ADC)) || (0/*_overload_functions_disabled_*/) )
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
		state_HWProcess &= ~HW_ON;
		state_HWProcess |= HW_OFF | HW_OVERLOADED;	// Set status for itself and top-level software
	}
	
	// Process ON/OFF commands
	if (ctrl_HWProcess & HW_RESET_OVERLOAD)
	{
		state_HWProcess &= ~HW_OVERLOADED;
	}
	if ( (ctrl_HWProcess & HW_OFF) && (!(state_HWProcess & HW_OVERLOADED)) )
	{
		state_HWProcess &= ~(HW_ON);
		state_HWProcess |= HW_OFF;				
	}
	else if ( (ctrl_HWProcess & HW_ON) && (!(state_HWProcess & HW_OVERLOADED)) )
	{
		state_HWProcess &= ~(HW_OFF);
		state_HWProcess |= HW_ON;								
	}
	
	
	

	
	// Process ADC FSM-based controller
	next_adc_state = adc_state;
	switch (adc_state)
	{
		case ADC_IDLE:
			adc_cmd = ctrl_HWProcess & (HW_START_ADC_VOLTAGE | HW_START_ADC_CURRENT | HW_START_ADC_DISCON);
			if (adc_cmd)
				next_adc_state = ADC_DISPATCH;
			break;
		case ADC_DISPATCH:
			if ((adc_cmd & (HW_START_ADC_VOLTAGE | HW_START_ADC_DISCON)) == HW_START_ADC_VOLTAGE)
			{
				// Normal voltage measure 
				adc_cmd &= ~(HW_START_ADC_VOLTAGE | HW_START_ADC_DISCON);
				next_adc_state = ADC_NORMAL_START_U;
				voltage_samples = 0;
			}
			else if (adc_cmd & HW_START_ADC_CURRENT)
			{
				// Current measure
				adc_cmd &= ~HW_START_ADC_CURRENT;
				next_adc_state = ADC_START_I;
				current_samples = 0;
			}
			else
			{
				// Update global Voltage and Current
				adc_voltage_counts = voltage_samples;
				adc_current_counts = current_samples;
				next_adc_state = ADC_IDLE;
			}
			break;
			
		case ADC_NORMAL_START_U:
			// TODO: use DMA for this purpose
			ADC1_SetChannel(ADC_CHANNEL_VOLTAGE);
			adc_counter = 4;
			next_adc_state = ADC_NORMAL_REPEAT_U;
			break;
		case ADC_NORMAL_REPEAT_U:
			if (adc_counter < 4)
				voltage_samples += ADC1_GetResult();
			if (adc_counter != 0)
			{
				ADC1_Start();
				adc_counter--;
			}
			else
			{
				next_adc_state = ADC_DISPATCH;
			}
			break;
			
			
		case ADC_START_I:
			// TODO: use DMA for this purpose
			ADC1_SetChannel(ADC_CHANNEL_CURRENT);
			adc_counter = 4;
			next_adc_state = ADC_NORMAL_REPEAT_I;
			break;
		case ADC_NORMAL_REPEAT_I:
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
				next_adc_state = ADC_DISPATCH;
			}
			break;
		
		default:
			next_adc_state = ADC_IDLE;
			break;
	}
	
	adc_state = next_adc_state;
	
	/*
	if (ctrl_HWProcess & HW_START_ADC)
	{
		//...
		
		// Disable converter for ADC
		state_HWProcess |= HW_OFF_BY_ADC;
		
		//...
		
		// Enable converter for ADC
		state_HWProcess &= ~HW_OFF_BY_ADC;
		
	}
	*/
	
	
	// Reset command
	ctrl_HWProcess = 0;
	
	
	
	// Apply converter state
	// FIXME: care must be taken when using portF - this code is called from ISR
	// ATOMIC access should be used
	if (state_HWProcess & (HW_OFF | HW_OFF_BY_ADC))
		SetConverterState(CONVERTER_OFF);
	else if (state_HWProcess & HW_ON)
		SetConverterState(CONVERTER_ON);
	
}






