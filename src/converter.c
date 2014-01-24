/********************************************************************
	Module		Converter
	
	Purpose:
		
					
	Globals used:

		
	Operation: 
	
 ___________________________________
|          CONVERTER TASK           |       <- top layer
|___________________________________|          provides interface to other system components
                  |
                  v
 ___________________________________
|   Converter HW control funtions   |       <- middle layer
|___________________________________|		   stores settings of voltage, current, overload
                  |
                  v
 ___________________________________
|   ISR - based low level control   |       <- low layer
|___________________________________|	




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


static gui_msg_t gui_msg;


converter_regulation_t channel_5v;
converter_regulation_t channel_12v;
converter_regulation_t *regulation_setting_p;







const conveter_message_t converter_tick_message = 	{	CONVERTER_TICK, 0	};
const conveter_message_t converter_update_message = {	CONVERTER_UPDATE, 0 };
//const conveter_message_t converter_init_message = {	CONVERTER_INITIALIZE, 0, 0 };


xQueueHandle xQueueConverter;


//static void apply_regulation(void);
//static uint16_t CheckSetVoltageRange(int32_t new_set_voltage, uint8_t *err_code);
//static uint16_t CheckSetCurrentRange(int32_t new_set_current, uint8_t *err_code);

static void ApplyAnalogRegulation(uint8_t code);
static uint8_t SetRegulationValue(reg_setting_t *s, int32_t new_value);
static uint8_t SetRegulationLimit(reg_setting_t *s, uint8_t limit_type, int32_t new_value, uint8_t enable);

static uint8_t Converter_SetVoltage(uint8_t channel, int32_t new_value, uint8_t *err_code);
static uint8_t Converter_SetCurrent(uint8_t channel, uint8_t current_range, int32_t new_value, uint8_t *err_code);
static uint8_t Converter_SetVoltageLimit(uint8_t channel, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code);
static uint8_t Converter_SetCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code);
static uint8_t Converter_SetProtectionControl(uint8_t channel, uint8_t protection_enable, int16_t overload_timeout, uint8_t *err_code = 0);
static uint8_t Converter_SetChannel(uint8_t new_value);
static uint8_t Converter_SetCurrentRange(uint8_t channel, uint8_t new_value);


uint32_t conv_state = CONV_OFF;



// Analog regulation apply function arguments
#define APPLY_VOLTAGE	0x01
#define APPLY_CURRENT	0x02


// Voltage and current setting return codes
#define VALUE_OK					0x00
#define VALUE_BOUND_BY_SOFT_MAX		0x01
#define VALUE_BOUND_BY_SOFT_MIN		0x02
#define VALUE_BOUND_BY_ABS_MAX		0x04
#define VALUE_BOUND_BY_ABS_MIN		0x08




//---------------------------------------------------------------------------//
//	Applies operating channel voltage and current settings
//---------------------------------------------------------------------------//
static void ApplyAnalogRegulation(uint8_t code)
{
	uint16_t temp;
	
	if (code & APPLY_VOLTAGE)
	{
		temp = regulation_setting_p->voltage.setting;
		temp /= 5;
		SetVoltagePWMPeriod(temp);		// FIXME - we are setting not period but duty
	}
	if (code & APPLY_CURRENT)
	{
		temp = regulation_setting_p->current->setting;
		temp = (regulation_setting_p->current->RANGE == CURRENT_RANGE_HIGH) ? temp / 2 : temp;
		temp /= 5;
		SetCurrentPWMPeriod(temp);		// FIXME - we are setting not period but duty
	}
	
	/*
static void apply_regulation(void)
{
	uint16_t temp;
	
	// Apply voltage - same for both 5V and 12V channels
	temp = regulation_setting_p->voltage.setting;
	temp /= 5;
	SetVoltagePWMPeriod(temp);		// FIXME - we are setting not period but duty
	
	// Apply current different for 20A and 40A limits
	temp = regulation_setting_p->current->setting;
	temp = (regulation_setting_p->current->RANGE == CURRENT_RANGE_HIGH) ? temp / 2 : temp;
	temp /= 5;
	SetCurrentPWMPeriod(temp);		// FIXME - we are setting not period but duty
}
*/
}




//---------------------------------------------------------------------------//
//	Checks and sets new value in regultaion structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationValue(reg_setting_t *s, int32_t new_value)
{
	uint8_t result = VALUE_OK;;
	// Check software limits
	if (new_value <= (int32_t)s->limit_low)
	{	
		new_value = (int32_t)s->limit_low;
		result = VALUE_BOUND_BY_SOFT_MIN;
	}
	if (new_value >= (int32_t)s->limit_high) 
	{
		new_value = (int32_t)s->limit_high;
		result = VALUE_BOUND_BY_SOFT_MAX;
	} 
	// Check absolute limits
	if (new_value <= (int32_t)s->MINIMUM)
	{	
		new_value = (int32_t)s->MINIMUM;
		result = VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->MAXIMUM) 
	{
		new_value = (int32_t)s->MAXIMUM;
		result = VALUE_BOUND_BY_ABS_MAX;
	} 
	s->setting = new_value;
	return result;
}

//---------------------------------------------------------------------------//
//	Checks and sets new value limit in regultaion structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationLimit(reg_setting_t *s, uint8_t limit_type, int32_t new_value, uint8_t enable)
{
	uint8_t result = VALUE_OK;;
	if (new_value <= (int32_t)s->LIMIT_MIN)
	{	
		new_value = (int32_t)s->LIMIT_MIN;
		result = VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->LIMIT_MAX) 
	{
		new_value = (int32_t)s->LIMIT_MAX;
		result = VALUE_BOUND_BY_ABS_MAX;
	} 
	if (limit_type == LIMIT_TYPE_LOW)
	{
		s->limit_low = new_value;
		s->enable_low_limit = enable;
	}
	else if (limit_type == LIMIT_TYPE_HIGH)
	{
		s->limit_high = new_value;
		s->enable_high_limit = enable;
	}
	return result;
}





//---------------------------------------------------------------------------//
// 	Set converter output voltage
//	input:
//		channel - converter channel to affect
//		new_value - new value to check and set
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		non-zero if operating channel has been affected
//---------------------------------------------------------------------------//
static uint8_t Converter_SetVoltage(uint8_t channel, int32_t new_value, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : regulation_setting_p);
	uint8_t operating_channel_affected = 0;
								
	uint8_t result = SetRegulationValue(&r->voltage, new_value);
	if (err_code) *err_code = result;
	
	// Update hardware
	if (r == regulation_setting_p)
	{
		ApplyAnalogRegultaion(APPLY_VOLTAGE);
		operating_channel_affected = 1;
	}
	return operating_channel_affected;
}


//---------------------------------------------------------------------------//
// 	Set converter output current
//	input:
//		channel - converter channel to affect
//		current_range - 20A range or 40A range to modify
//		new_value - new value to check and set
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		non-zero if operating channel has been affected
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrent(uint8_t channel, uint8_t current_range, int32_t new_value, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : regulation_setting_p);
	reg_setting_t *s = (current_range == CURRENT_RANGE_LOW) ? r->current_low_range : 
					   ((current_range == CURRENT_RANGE_HIGH) ? r->current_high_range : r->current);
	uint8_t operating_channel_affected = 0;
	
	uint8_t result = SetRegulationValue(s, new_value);
	if (err_code) *err_code = result;
	
	// Update hanrdware
	if (s == regulation_setting_p->current)
	{
		ApplyAnalogRegultaion(APPLY_CURRENT);
		operating_channel_affected = 1;
	}
	return 	operating_channel_affected;			   
}


//---------------------------------------------------------------------------//
// 	Set converter voltage limit
//	input:
//		channel - converter channel to affect
//		limit_type - low or high limit to modify
//		new_value - new value to check and set
//		enable - set limit enabled or disabled
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		non-zero if operating channel has been affected
//---------------------------------------------------------------------------//
static uint8_t Converter_SetVoltageLimit(uint8_t channel, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : regulation_setting_p);
								
	uint8_t result = SetRegulationLimit(&r->voltage, limit_type, new_value, enable);
	if (err_code) *err_code = result;
	
	// Ensure that voltage setting lies inside new limits
	return Converter_SetVoltage(channel, r->voltage.setting);
}

//---------------------------------------------------------------------------//
// 	Set converter current limit
//	input:
//		channel - converter channel to affect
//		current_range - 20A range or 40A range to modify
//		limit_type - low or high limit to modify
//		new_value - new value to check and set
//		enable - set limit enabled or disabled
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		non-zero if operating channel has been affected
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : regulation_setting_p);
	reg_setting_t *s = (current_range == CURRENT_RANGE_LOW) ? r->current_low_range : 
					   ((current_range == CURRENT_RANGE_HIGH) ? r->current_high_range : r->current);
	
	uint8_t result = SetRegulationLimit(s, limit_type, new_value, enable);
	if (err_code) *err_code = result;
	
	// Ensure that current setting lies inside new limits
	return Converter_SetCurrent(channel, current_range, s->setting);
}


//---------------------------------------------------------------------------//
// 	Set converter overload protection parameters
//	input:
//		channel - converter channel to affect
//		protection_enable - set protection enabled or disabled
//		overload_timeout - specifies delay between overload discovering and turning OFF converter
//	output:
//		non-zero if operating channel has been affected
//---------------------------------------------------------------------------//
static uint8_t Converter_SetProtectionControl(uint8_t channel, uint8_t protection_enable, int16_t overload_timeout, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : regulation_setting_p);
	uint8_t operating_channel_affected = 0;
	uint8_t result = VALUE_OK;
	
	// Bound new value
	if (overload_timeout >= MAX_OVERLOAD_TIMEOUT)
	{
		overload_timeout = MAX_OVERLOAD_TIMEOUT;
		result = VALUE_BOUND_BY_ABS_MAX;
	}
	if (overload_timeout <= MIN_OVERLOAD_TIMEOUT)
	{
		overload_timeout = MIN_OVERLOAD_TIMEOUT;
		result = VALUE_BOUND_BY_ABS_MIN;
	}
		
	r->overload_protection_enable = protection_enable;
	r->overload_timeout = overload_timeout;
	if (err_code) *err_code = result;
	
	// Update hardware
	if (r == regulation_setting_p)
	{
		// Overload protection mechanism uses data from regulation_setting_p, so no
		// 	special settings are required.
		operating_channel_affected = 1;
	}
	return operating_channel_affected;
}





//---------------------------------------------------------------------------//
// 	Set converter channel
//	input:
//		new_value - converter channel to select
//	output:
//		non-zero if channel setting has been applied
//---------------------------------------------------------------------------//
static uint8_t Converter_SetChannel(uint8_t new_value)
{
	// Check that converter is disabled
	if (GetConverterState() == CONVERTER_ON)
		return 0;
	// Check argument - should be either CHANNEL_5V or CHANNEL_12V
	if ( (new_value != CHANNEL_5V) && (new_value != CHANNEL_12V) )
		return 0;
	
	regulation_setting_p = (channel == CHANNEL_5V) ? &channel_5v : &channel_12v;
	
	// Apply controls
	__disable_irq();
	SetFeedbackChannel(regulation_setting_p->CHANNEL);		// PORTF can be accessed from ISR
	__enable_irq();
	SetCurrentRange(regulation_setting_p->current->RANGE);
	SetOutputLoad(regulation_setting_p->load_state);
	ApplyAnalogRegulation( APPLY_VOLTAGE | APPLY_CURRENT );
	// Overload protection mechanism uses data from regulation_setting_p, so no
	// 	special settings are required.
	
	return 1;
}



//---------------------------------------------------------------------------//
// 	Set converter current range
//	input:
//		channel - converter channel to affect
//		new_value - new current range 
//	output:
//		non-zero if channel setting has been applied
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrentRange(uint8_t channel, uint8_t new_value)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : regulation_setting_p);

	// Check that converter is disabled
	if (GetConverterState() == CONVERTER_ON)
		return 0;
	// Check argument - should be either CURRENT_RANGE_LOW or CURRENT_RANGE_HIGH
	if ( (new_value != CURRENT_RANGE_LOW) && (new_value != CURRENT_RANGE_HIGH) )
		return 0;
	
	r->current = (new_value == CURRENT_RANGE_LOW) ? &r->current_low_range : &r->current_high_range;
	
	if (r == regulation_setting_p)
	{
		SetCurrentRange(regulation_setting_p->current->RANGE);
		ApplyAnalogRegulation( APPLY_CURRENT );
		return 1;
	}
	else
		return 0;
}



static uint8_t ValidateNewChannel(uint8_t channel)
{
	if ( ((channel == CHANNEL_5V) || (channel == CHANNEL_12V)) && (channel != regulation_setting_p->CHANNEL) )
		return 1;
	else
		return 0;
}


static uint8_t ValidateNewCurrentRange(uint8_t channel, uint8_t current_range)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : &channel_12v;
	if (r->current->RANGE != current_range)
		return 1;
	else
		return 0;
}



void Converter_Init(uint8_t default_channel)
{
	// Converter is powered off.
	// TODO: add restore from EEPROM
	
	//---------- Channel 5V ------------//
	
	// Common
	channel_5v.CHANNEL = CHANNEL_5V;
	channel_5v.load_state = LOAD_ENABLE;										// dummy - load at 5V channel can not be disabled
	channel_5v.overload_protection_enable = 1;									// TODO: EEPROM
	channel_5v.overload_timeout = (5*1);										// TODO: EEPROM
	
	// Voltage
	channel_5v.voltage.setting = 5000;											// TODO: EEPROM
	channel_5v.voltage.MINIMUM = CONV_MIN_VOLTAGE_5V_CHANNEL;					// Minimum voltage setting for channel
	channel_5v.voltage.MAXIMUM = CONV_MAX_VOLTAGE_5V_CHANNEL;					// Maximum voltage setting for channel
	channel_5v.voltage.limit_low = CONV_MIN_VOLTAGE_5V_CHANNEL;					// TODO: EEPROM
	channel_5v.voltage.limit_high = CONV_MAX_VOLTAGE_5V_CHANNEL;				// TODO: EEPROM
	channel_5v.voltage.LIMIT_MIN = CONV_MIN_VOLTAGE_5V_CHANNEL;					// Minimum voltage limit setting
	channel_5v.voltage.LIMIT_MAX = CONV_MAX_VOLTAGE_5V_CHANNEL;					// Maximum voltage limit setting
	channel_5v.voltage.enable_low_limit = 0;									// TODO: EEPROM
	channel_5v.voltage.enable_high_limit = 0;									// TODO: EEPROM
	
	// Current
	channel_5v.current_low_range.RANGE = CURRENT_RANGE_LOW;						// LOW current range
	channel_5v.current_low_range.setting = 4000;								// TODO: EEPROM
	channel_5v.current_low_range.MINIMUM = CONV_LOW_CURRENT_RANGE_MIN;			// Minimum current setting for specified current range
	channel_5v.current_low_range.MAXIMUM = CONV_LOW_CURRENT_RANGE_MAX;			// Maximum current setting for specified current range
	channel_5v.current_low_range.limit_low = CONV_LOW_CURRENT_RANGE_MIN;		// TODO: EEPROM
	channel_5v.current_low_range.limit_high = CONV_LOW_CURRENT_RANGE_MAX;		// TODO: EEPROM
	channel_5v.current_low_range.LIMIT_MIN = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current limit setting
	channel_5v.current_low_range.LIMIT_MAX = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current limit setting
	channel_5v.current_low_range.enable_low_limit = 0;							// TODO: EEPROM
	channel_5v.current_low_range.enable_high_limit = 0;							// TODO: EEPROM
	
	channel_5v.current_high_range.RANGE = CURRENT_RANGE_HIGH;					// HIGH current range
	channel_5v.current_high_range.setting = 4000;								// TODO: EEPROM
	channel_5v.current_high_range.MINIMUM = CONV_HIGH_CURRENT_RANGE_MIN;		// Minimum current setting for specified current range
	channel_5v.current_high_range.MAXIMUM = CONV_HIGH_CURRENT_RANGE_MAX;		// Maximum current setting for specified current range
	channel_5v.current_high_range.limit_low = CONV_HIGH_CURRENT_RANGE_MIN;		// TODO: EEPROM
	channel_5v.current_high_range.limit_high = CONV_HIGH_CURRENT_RANGE_MAX;		// TODO: EEPROM
	channel_5v.current_high_range.LIMIT_MIN = CONV_HIGH_CURRENT_RANGE_MIN;		// Minimum current limit setting
	channel_5v.current_high_range.LIMIT_MAX = CONV_HIGH_CURRENT_RANGE_MAX;		// Maximum current limit setting
	channel_5v.current_high_range.enable_low_limit = 0;							// TODO: EEPROM
	channel_5v.current_high_range.enable_high_limit = 0;						// TODO: EEPROM

	channel_5v.current = &channel_5v.current_low_range;
	
	//---------- Channel 12V -----------//
	
	// Common
	channel_12v.CHANNEL = CHANNEL_12V;
	channel_12v.load_state = LOAD_ENABLE;										
	channel_12v.overload_protection_enable = 1;									// TODO: EEPROM
	channel_12v.overload_timeout = (5*1);										// TODO: EEPROM
	
	// Voltage
	channel_12v.voltage.setting = 12000;										// TODO: EEPROM
	channel_12v.voltage.MINIMUM = CONV_MIN_VOLTAGE_12V_CHANNEL;					// Minimum voltage setting for channel
	channel_12v.voltage.MAXIMUM = CONV_MAX_VOLTAGE_12V_CHANNEL;					// Maximum voltage setting for channel
	channel_12v.voltage.limit_low = CONV_MIN_VOLTAGE_12V_CHANNEL;				// TODO: EEPROM
	channel_12v.voltage.limit_high = CONV_MAX_VOLTAGE_12V_CHANNEL;				// TODO: EEPROM
	channel_12v.voltage.LIMIT_MIN = CONV_MIN_VOLTAGE_12V_CHANNEL;				// Minimum voltage limit setting
	channel_12v.voltage.LIMIT_MAX = CONV_MAX_VOLTAGE_12V_CHANNEL;				// Maximum voltage limit setting
	channel_12v.voltage.enable_low_limit = 0;									// TODO: EEPROM
	channel_12v.voltage.enable_high_limit = 0;									// TODO: EEPROM
	
	// Current
	channel_12v.current_low_range.RANGE = CURRENT_RANGE_LOW;					// LOW current range
	channel_12v.current_low_range.setting = 2000;								// TODO: EEPROM
	channel_12v.current_low_range.MINIMUM = CONV_LOW_CURRENT_RANGE_MIN;			// Minimum current setting for specified current range
	channel_12v.current_low_range.MAXIMUM = CONV_LOW_CURRENT_RANGE_MAX;			// Maximum current setting for specified current range
	channel_12v.current_low_range.limit_low = CONV_LOW_CURRENT_RANGE_MIN;		// TODO: EEPROM
	channel_12v.current_low_range.limit_high = CONV_LOW_CURRENT_RANGE_MAX;		// TODO: EEPROM
	channel_12v.current_low_range.LIMIT_MIN = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current limit setting
	channel_12v.current_low_range.LIMIT_MAX = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current limit setting
	channel_12v.current_low_range.enable_low_limit = 0;							// TODO: EEPROM
	channel_12v.current_low_range.enable_high_limit = 0;						// TODO: EEPROM
	
	// 12V channel cannot provide currents > 20A (low range)
	channel_12v.current_high_range.RANGE = CURRENT_RANGE_HIGH;					// HIGH current range
	channel_12v.current_high_range.setting = 4000;								// TODO: EEPROM
	channel_12v.current_high_range.MINIMUM = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current setting for specified current range
	channel_12v.current_high_range.MAXIMUM = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current setting for specified current range
	channel_12v.current_high_range.limit_low = CONV_LOW_CURRENT_RANGE_MIN;		// TODO: EEPROM
	channel_12v.current_high_range.limit_high = CONV_LOW_CURRENT_RANGE_MAX;		// TODO: EEPROM
	channel_12v.current_high_range.LIMIT_MIN = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current limit setting
	channel_12v.current_high_range.LIMIT_MAX = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current limit setting
	channel_12v.current_high_range.enable_low_limit = 0;						// TODO: EEPROM
	channel_12v.current_high_range.enable_high_limit = 0;						// TODO: EEPROM

	channel_12v.current = &channel_12v.current_low_range;
	
	
	// Select default channel
	Converter_SetChannel(CHANNEL_12V);	// will apply other settings

	/*
	if (default_channel == CHANNEL_12V)
		regulation_setting_p = &channel_12v;
	else
		regulation_setting_p = &channel_5v;
	
	
	// Apply controls
	__disable_irq();
	SetFeedbackChannel(regulation_setting_p->CHANNEL);		// PORTF can be accessed from ISR
	__enable_irq();
	SetCurrentRange(regulation_setting_p->current->RANGE);
	SetOutputLoad(regulation_setting_p->load_state);
	apply_regulation();										// Apply voltage and current settings
	*/
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




static void SoundEvent_OnSettingChanged(uint8_t code)
{
	uint32_t sound_msg;
	if ((err_code == VALUE_BOUND_BY_SOFT_MAX) || (err_code == VALUE_BOUND_BY_SOFT_MIN))
		sound_msg = SND_CONV_SETTING_ILLEGAL;
	else if ((err_code == VALUE_BOUND_BY_ABS_MAX) || (err_code == VALUE_BOUND_BY_ABS_MIN))
		sound_msg = SND_CONV_SETTING_ILLEGAL;
	else
		sound_msg = SND_CONV_SETTING_OK;
	sound_msg |= SND_CONVERTER_PRIORITY_NORMAL;
	xQueueSendToBack(xQueueSound, &sound_msg, 0);
}



uint8_t taskConverter_Enable = 0;



//---------------------------------------------//
//	Main converter task
//	
//---------------------------------------------//
void vTaskConverter(void *pvParameters) 
{
	conveter_message_t msg;
	
	uint8_t err_code;
	uint8_t upd_gui;
	uint32_t adc_msg;
	
	
	// Initialize
	xQueueConverter = xQueueCreate( 5, sizeof( conveter_message_t ) );		// Queue can contain 5 elements of type conveter_message_t
	if( xQueueConverter == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	// Wait until task is started by dispatcher
	//vTaskSuspend(vTaskConverter);			
	
	while (taskConverter_Enable == 0)
		vTaskDelay(5);
	
	xQueueReset(xQueueConverter);
	
	
	while(1)
	{
		xQueueReceive(xQueueConverter, &msg, portMAX_DELAY);
	

		switch (msg.type)
		{
			case CONVERTER_SET_VOLTAGE:
				if (msg.voltage_setting.channel == OPERATING_CHANNEL) msg.voltage_setting.channel = regulation_setting_p->CHANNEL;
				Converter_SetVoltage(msg.voltage_setting.channel, msg.voltage_setting.value, &err_code);
				//----- Send notification to GUI -----//
				gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
				gui_msg.converter_event.spec = VOLTAGE_SETTING_CHANGED;
				gui_msg.converter_event.channel = msg.voltage_setting.channel;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				//------------------------------------//
				if (msg.sender != TID_UART)
					SoundEvent_OnSettingChanged(err_code);
				msg.type = 0;
				break;
			case CONVERTER_SET_CURRENT:
				if (msg.current_setting.channel == OPERATING_CHANNEL) msg.current_setting.channel = regulation_setting_p->CHANNEL;
				if (msg.current_setting.range == OPERATING_CURRENT_RANGE) msg.current_setting.range = regulation_setting_p->current->RANGE;
				Converter_SetCurrent(msg.current_setting.channel, msg.current_setting.range, msg.current_setting.value, &err_code);
				//----- Send notification to GUI -----//
				gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
				gui_msg.converter_event.spec = CURRENT_SETTING_CHANGED;
				gui_msg.converter_event.channel = msg.current_setting.channel;
				gui_msg.converter_event.current_range = msg.current_setting.range;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				//------------------------------------//
				if (msg.sender != TID_UART)
					SoundEvent_OnSettingChanged(err_code);
				msg.type = 0;
				break;
			case CONVERTER_SET_VOLTAGE_LIMIT:
				if (msg.voltage_limit_setting.channel == OPERATING_CHANNEL) msg.voltage_limit_setting.channel = regulation_setting_p->CHANNEL;
				Converter_SetVoltageLimit(msg.voltage_limit_setting.channel, msg.voltage_limit_setting.type, 
									msg.voltage_limit_setting.value, msg.voltage_limit_setting.enable, &err_code);
				//----- Send notification to GUI -----//					
				gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
				gui_msg.converter_event.spec = VOLTAGE_LIMIT_CHANGED;
				gui_msg.converter_event.channel = msg.voltage_limit_setting.channel;
				gui_msg.converter_event.type = msg.voltage_limit_setting.type;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				//------------------------------------//
				if (msg.sender != TID_UART)
					SoundEvent_OnSettingChanged(err_code);
				msg.type = 0;
				break;
			case CONVERTER_SET_CURRENT_LIMIT:
				if (msg.current_limit_setting.channel == OPERATING_CHANNEL) msg.current_limit_setting.channel = regulation_setting_p->CHANNEL;
				if (msg.current_limit_setting.range == OPERATING_CURRENT_RANGE) msg.current_limit_setting.range = regulation_setting_p->current->RANGE;
				Converter_SetCurrentLimit(msg.current_limit_setting.channel, msg.current_limit_setting.range, msg.current_limit_setting.type, 
									msg.current_limit_setting.value, msg.current_limit_setting.enable, &err_code);
				//----- Send notification to GUI -----//					
				gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
				gui_msg.converter_event.spec = CURRENT_LIMIT_CHANGED;
				gui_msg.converter_event.channel = msg.current_limit_setting.channel;
				gui_msg.converter_event.type = msg.current_limit_setting.type;
				gui_msg.converter_event.current_range = msg.current_limit_setting.range;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				//------------------------------------//
				if (msg.sender != TID_UART)
					SoundEvent_OnSettingChanged(err_code);
				msg.type = 0;
				break;
				
			// TODO: add overload and other settings
				
			
			case CONVERTER_TICK:		// Temporary! Will be special for charging mode
				// ADC task is responsible for sampling and filtering voltage and current
				adc_msg = ADC_GET_ALL_NORMAL;
				xQueueSendToBack(xQueueADC, &adc_msg, 0);
				msg.type = 0;
				break;
		}
		
		// Other messages get processing depending on converter state

		while(msg.type)
		{
			//------------------------------------//
			//------------------------------------//
			switch(conv_state & CONV_STATE_MASK)
			{
				case CONV_OFF:
					if (msg.type == CONVERTER_SWITCH_CHANNEL)
					{
						if (ValidateNewChannel(msg.channel_setting.new_channel))
						{
							Converter_SetChannel(msg.channel_setting.new_channel);
							ctrl_HWProcess = CMD_HW_RESTART_USER_TIMER;
							//----- Send notification to GUI -----//
							gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
							gui_msg.converter_event.spec = CHANNEL_CHANGED;
							gui_msg.converter_event.channel = msg.channel_setting.new_channel;
							gui_msg.converter_event.current_range = regulation_setting_p->current->RANGE;
							xQueueSendToBack(xQueueGUI, &gui_msg, 0);
							//------------------------------------//
							while(ctrl_HWProcess);
						}
						msg.type = 0;
						break;
					}
					if (msg.type == CONVERTER_SET_CURRENT_RANGE)
					{
						if (msg.current_range_setting.channel == OPERATING_CHANNEL) msg.current_range_setting.channel = regulation_setting_p->CHANNEL;
						if (msg.current_range_setting.range == OPERATING_CURRENT_RANGE) msg.current_range_setting.range = regulation_setting_p->current->RANGE;
						if (ValidateNewCurrentRange(msg.current_range_setting.channel, msg.current_range_setting.range)
						{
							Converter_SetCurrentRange(msg.current_range_setting.channel, msg.current_range_setting.new_range);
							ctrl_HWProcess = CMD_HW_RESTART_USER_TIMER;
							//----- Send notification to GUI -----//
							gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
							gui_msg.converter_event.spec = CURRENT_RANGE_CHANGED;
							gui_msg.converter_event.channel = msg.current_range_setting.channel;
							gui_msg.converter_event.current_range = msg.current_range_setting.new_range;
							xQueueSendToBack(xQueueGUI, &gui_msg, 0);
							//------------------------------------//
							while(ctrl_HWProcess);
						}
						msg.type = 0;
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
					if (msg.type == CONVERTER_SWITCH_CHANNEL)
					{
						if (ValidateNewChannel(msg.channel_setting.new_channel))
						{
							conv_state = disableConverterAndCheckHWState();
							vTaskDelay(4);
						}
						else
						{
							msg.type = 0;
						}
						break;
					}
					
					if (msg.type == CONVERTER_SET_CURRENT_RANGE)
					{
						// CONVERTER_SET_CURRENT_RANGE is ignored when converter is ON
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
					msg.type = 0;
					break;
			}
			//------------------------------------//
			//------------------------------------//
		}

	}	
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
	if ((state_HWProcess & (STATE_HW_OFF | STATE_HW_OFF_BY_ADC)) || (regulation_setting_p->overload_protection_enable == 0))
		overload_ignore_counter = OVERLOAD_IGNORE_TIMEOUT;
	else if (overload_ignore_counter != 0)
		overload_ignore_counter--;
	else
		overload_check_enable = 1;
		
	
	if (overload_check_enable)
		raw_overload_flag = GetOverloadStatus();
	else
		raw_overload_flag = NORMAL;
	
	if (raw_overload_flag == NORMAL)
		//overload_counter = OVERLOAD_TIMEOUT;
		overload_counter = regulation_setting_p->overload_timeout;
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















