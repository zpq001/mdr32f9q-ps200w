/**********************************************************
	Module converter

	Converter settings are shared between global settings
	and profiles. Converter is initialized to some known
	default state which is safe. Then global settings and profiles can be loaded.
	On power off, data that came from global set. must be saved back
	as well as profile data.

**********************************************************/



#include <string.h>

#include "MDR32Fx.h" 

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


//#include "sound_driver.h"
#include "control.h"
#include "converter.h"
#include "converter_hw.h"
#include "adc.h"
#include "eeprom.h"

#include "dispatcher.h"



enum ChargeFSMCommands { CHARGE_START = 1, CHARGE_STOP, CHARGE_ABORT, CHARGE_TICK };





// OS stuff
xQueueHandle xQueueConverter;
xTaskHandle xTaskHandle_Converter;
const converter_message_t converter_overload_msg = {CONVERTER_OVERLOAD_EVENT};
const converter_message_t converter_tick_message = {CONVERTER_TICK};

// Global to allow whole message processing be split into several functions
static converter_message_t msg;

uint32_t adc_msg;
dispatch_msg_t dispatcher_msg;

converter_state_t converter_state;		// main converter control
//dac_settings_t dac_settings;			// DAC calibration offset

static void processStateControl (uint8_t cmd_code);
static void processParamControl (void);
static void processProfileControl (void);
static void processChargeControl(uint8_t cmd);


//---------------------------------------------//
// 

uint8_t Converter_GetState(void)
{
	return (converter_state.state == CONVERTER_STATE_ON) ? 1 : 0;
}

uint16_t Converter_GetVoltageSetting(uint8_t channel)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return c->voltage.setting;
}

uint16_t Converter_GetVoltageAbsMax(uint8_t channel)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return c->voltage.MAXIMUM;
}

uint16_t Converter_GetVoltageAbsMin(uint8_t channel)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return c->voltage.MINIMUM;
}

uint16_t Converter_GetVoltageLimitSetting(uint8_t channel, uint8_t limit_type)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return (limit_type == LIMIT_TYPE_LOW) ? c->voltage.limit_low : c->voltage.limit_high;
}

uint8_t Converter_GetVoltageLimitState(uint8_t channel, uint8_t limit_type)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return (limit_type == LIMIT_TYPE_LOW) ? c->voltage.enable_low_limit : c->voltage.enable_high_limit;
}

uint16_t Converter_GetCurrentSetting(uint8_t channel, uint8_t range)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
    return s->setting;
}

uint16_t Converter_GetCurrentAbsMax(uint8_t channel, uint8_t range)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
    return s->MAXIMUM;
}

uint16_t Converter_GetCurrentAbsMin(uint8_t channel, uint8_t range)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
    return s->MINIMUM;
}

uint16_t Converter_GetCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
	return (limit_type == LIMIT_TYPE_LOW) ? s->limit_low : s->limit_high;
}

uint8_t Converter_GetCurrentLimitState(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
	return (limit_type == LIMIT_TYPE_LOW) ? s->enable_low_limit : s->enable_high_limit;
}

uint8_t Converter_GetOverloadProtectionState(void)
{
	return converter_state.overload_protection_enable;
}

uint8_t Converter_GetOverloadProtectionWarning(void)
{
    return converter_state.overload_warning_enable;
}

uint16_t Converter_GetOverloadProtectionThreshold(void)
{
	// return value is in units of 100us
    return converter_state.overload_threshold;	
}

uint8_t Converter_GetCurrentRange(uint8_t channel)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return c->current->RANGE;
}

uint8_t Converter_GetFeedbackChannel(void)
{
	return converter_state.channel->CHANNEL;
}


int8_t Converter_GetVoltageDacOffset(void)
{
	return global_settings->dac_voltage_offset;
}

int8_t Converter_GetCurrentDacOffset(uint8_t range)
{
	return (range == CURRENT_RANGE_LOW) ? global_settings->dac_current_low_offset : global_settings->dac_current_high_offset;
}


//---------------------------------------------//






//---------------------------------------------------------------------------//
///	Checks and sets new value in regulation structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationValue(reg_setting_t *s, int32_t new_value)
{
	uint8_t errCode = 0;
	// Check software limits
	if ((s->enable_low_limit) && (new_value <= (int32_t)s->limit_low)) {	
		new_value = (int32_t)s->limit_low;
		errCode |= VALUE_BOUND_BY_SOFT_MIN;
	}
	if ((s->enable_high_limit) && (new_value >= (int32_t)s->limit_high)) {
		new_value = (int32_t)s->limit_high;
		errCode |= VALUE_BOUND_BY_SOFT_MAX;
	} 
	// Check absolute limits
	if (new_value <= (int32_t)s->MINIMUM) {	
		new_value = (int32_t)s->MINIMUM;
		errCode |= VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->MAXIMUM) {
		new_value = (int32_t)s->MAXIMUM;
		errCode |= VALUE_BOUND_BY_ABS_MAX;
	} 
	// Check if new value differes from old
	if (s->setting != new_value) {
		s->setting = new_value;
		errCode |= VALUE_UPDATED;
	}
	return errCode;
}


//---------------------------------------------------------------------------//
///	Checks and sets new value limit in regultaion structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationLimit(reg_setting_t *s, uint8_t limit_type, int32_t new_value, uint8_t enable)
{
	uint8_t errCode = 0;
	if (new_value <= (int32_t)s->LIMIT_MIN)
	{	
		new_value = (int32_t)s->LIMIT_MIN;
		errCode |= VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->LIMIT_MAX) 
	{
		new_value = (int32_t)s->LIMIT_MAX;
		errCode |= VALUE_BOUND_BY_ABS_MAX;
	} 
	if (limit_type == LIMIT_TYPE_LOW)
	{
		if ((s->limit_low != new_value) || (s->enable_low_limit != enable))
		{
			s->limit_low = new_value;
			s->enable_low_limit = enable;
			errCode |= VALUE_UPDATED;
		}
	}
	else 
	{
		if ((s->limit_high != new_value) || (s->enable_high_limit != enable))
		{
			s->limit_high = new_value;
			s->enable_high_limit = enable;
			errCode |= VALUE_UPDATED;
		}
	}
	return errCode;
}




static reg_setting_t *getVoltageRegData(uint8_t channel)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	return &c->voltage;
}

static reg_setting_t *getCurrentRegData(uint8_t channel, uint8_t current_range)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	return (current_range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
}



//---------------------------------------------------------------------------//
///	Set converter output voltage
//	input:
//		channel - converter channel to affect: CHANNEL_5V / CHANNEL_12V
//		new_value - new value to check and set
//	output:
//		err_code - return code providing information about 
//				   limiting new value by absolute and software limits
//---------------------------------------------------------------------------//
static uint8_t Converter_SetVoltage(uint8_t channel, int32_t new_value)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	uint8_t err_code = SetRegulationValue(&c->voltage, new_value);
	return err_code;
}


//---------------------------------------------------------------------------//
///	Set converter voltage limit
//	input:
//		channel - converter channel to affect: CHANNEL_5V / CHANNEL_12V
//		limit_type - low or high limit to modify
//		new_value - new value to check and set
//		enable - set limit enabled or disabled
//	output:
//		err_code - return code providing information about 
//				   limiting new value by absolute limits
//---------------------------------------------------------------------------//
static uint8_t Converter_SetVoltageLimit(uint8_t channel, uint8_t limit_type, int32_t new_value, uint8_t enable)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	uint8_t err_code = SetRegulationLimit(&c->voltage, limit_type, new_value, enable);
	return err_code;
}


//---------------------------------------------------------------------------//
///	Set converter output current
//	input:
//		channel - converter channel to affect: CHANNEL_5V / CHANNEL_12V
//		current_range - 20A range or 40A range to modify.
//		new_value - new value to check and set
//	output:
//		err_code - return code providing information about 
//				   limiting new value by absolute and software limits
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrent(uint8_t channel, uint8_t current_range, int32_t new_value)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (current_range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
	uint8_t err_code = SetRegulationValue(s, new_value);
	return 	err_code;
}


//---------------------------------------------------------------------------//
/// Set converter current limit
//	input:
//		channel - converter channel to affect: CHANNEL_5V / CHANNEL_12V
//		current_range - 20A range or 40A range to modify.
//		limit_type - low or high limit to modify
//		new_value - new value to check and set
//		enable - set limit enabled or disabled
//	output:
//		err_code - return code providing information about 
//				   limiting new value by absolute limits
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t limit_type, int32_t new_value, uint8_t enable)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (current_range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
	uint8_t err_code = SetRegulationLimit(s, limit_type, new_value, enable);	
	return err_code;
}


//---------------------------------------------------------------------------//
/// Set converter overload parameters
// new_threshold in units of 100us
//---------------------------------------------------------------------------//
static uint8_t Converter_SetOverloadProtection(uint8_t protectionEnable, uint8_t warningEnable, int32_t new_threshold)
{
	uint8_t err_code;
	if (new_threshold < CONV_MIN_OVERLOAD_THRESHOLD)
	{
		new_threshold = CONV_MIN_OVERLOAD_THRESHOLD;
		err_code = VALUE_BOUND_BY_ABS_MIN;
	}
	else if (new_threshold > CONV_MAX_OVERLOAD_THRESHOLD)
	{
		new_threshold = CONV_MAX_OVERLOAD_THRESHOLD;
		err_code = VALUE_BOUND_BY_ABS_MAX;
	}
	else
	{
		err_code = VALUE_UPDATED;
	}
	
	converter_state.overload_protection_enable = protectionEnable;
	converter_state.overload_warning_enable = warningEnable;
	converter_state.overload_threshold = new_threshold;
	return err_code;
}	


//---------------------------------------------------------------------------//
//	If *channel = OPERATING_CHANNEL (special case), 
//	functions modifies channel to currently operating.
//---------------------------------------------------------------------------//
static uint8_t Converter_ValidateChannel(uint8_t channel)
{
	if ((channel != CHANNEL_5V) && (channel != CHANNEL_12V))
		channel = converter_state.channel->CHANNEL;	// OPERATING_CHANNEL
	return channel;
}


//---------------------------------------------------------------------------//
//	If *range = OPERATING_RANGE (special case), 
//	functions modifies range to currently operating.
//---------------------------------------------------------------------------//
static uint8_t Converter_ValidateCurrentRange(uint8_t range)
{
	if ((range != CURRENT_RANGE_LOW) && (range != CURRENT_RANGE_HIGH))
		range = converter_state.channel->current->RANGE;	//OPERATING_CURRENT_RANGE	
	return range;
}




//===========================================================================//
//===========================================================================//

// Converter data is shared between hardcoded constants and 
// profile data which is loaded from EEPROM

static void Converter_LoadProfile(void)
{
	//---------- Channel 5V ------------//
	// Voltage
	converter_state.channel_5v.voltage.setting = device_profile->converter_profile.ch5v_voltage.setting;
	converter_state.channel_5v.voltage.limit_low = device_profile->converter_profile.ch5v_voltage.limit_low;
	converter_state.channel_5v.voltage.limit_high = device_profile->converter_profile.ch5v_voltage.limit_high;
	converter_state.channel_5v.voltage.enable_low_limit = device_profile->converter_profile.ch5v_voltage.enable_low_limit;
	converter_state.channel_5v.voltage.enable_high_limit = device_profile->converter_profile.ch5v_voltage.enable_high_limit;
	// Current low range
	converter_state.channel_5v.current_low_range.setting = device_profile->converter_profile.ch5v_current.low_range.setting;
	converter_state.channel_5v.current_low_range.limit_low = device_profile->converter_profile.ch5v_current.low_range.limit_low;
	converter_state.channel_5v.current_low_range.limit_high = device_profile->converter_profile.ch5v_current.low_range.limit_high;
	converter_state.channel_5v.current_low_range.enable_low_limit = device_profile->converter_profile.ch5v_current.low_range.enable_low_limit;
	converter_state.channel_5v.current_low_range.enable_high_limit = device_profile->converter_profile.ch5v_current.low_range.enable_high_limit;
	// Current high range
	converter_state.channel_5v.current_high_range.setting = device_profile->converter_profile.ch5v_current.high_range.setting;
	converter_state.channel_5v.current_high_range.limit_low = device_profile->converter_profile.ch5v_current.high_range.limit_low;
	converter_state.channel_5v.current_high_range.limit_high = device_profile->converter_profile.ch5v_current.high_range.limit_high;
	converter_state.channel_5v.current_high_range.enable_low_limit = device_profile->converter_profile.ch5v_current.high_range.enable_low_limit;
	converter_state.channel_5v.current_high_range.enable_high_limit = device_profile->converter_profile.ch5v_current.high_range.enable_high_limit;
	// Apply current range
	converter_state.channel_5v.current = (device_profile->converter_profile.ch5v_current.selected_range == CURRENT_RANGE_LOW) ? 
		&converter_state.channel_5v.current_low_range : &converter_state.channel_5v.current_high_range;
	
	
	//---------- Channel 12V -----------//
	// Voltage
	converter_state.channel_12v.voltage.setting = device_profile->converter_profile.ch12v_voltage.setting;
	converter_state.channel_12v.voltage.limit_low = device_profile->converter_profile.ch12v_voltage.limit_low;
	converter_state.channel_12v.voltage.limit_high = device_profile->converter_profile.ch12v_voltage.limit_high;
	converter_state.channel_12v.voltage.enable_low_limit = device_profile->converter_profile.ch12v_voltage.enable_low_limit;
	converter_state.channel_12v.voltage.enable_high_limit = device_profile->converter_profile.ch12v_voltage.enable_high_limit;
	// Current low range
	converter_state.channel_12v.current_low_range.setting = device_profile->converter_profile.ch12v_current.low_range.setting;
	converter_state.channel_12v.current_low_range.limit_low = device_profile->converter_profile.ch12v_current.low_range.limit_low;
	converter_state.channel_12v.current_low_range.limit_high = device_profile->converter_profile.ch12v_current.low_range.limit_high;
	converter_state.channel_12v.current_low_range.enable_low_limit = device_profile->converter_profile.ch12v_current.low_range.enable_low_limit;
	converter_state.channel_12v.current_low_range.enable_high_limit = device_profile->converter_profile.ch12v_current.low_range.enable_high_limit;
	// Current high range
	converter_state.channel_12v.current_high_range.setting = device_profile->converter_profile.ch12v_current.high_range.setting;
	converter_state.channel_12v.current_high_range.limit_low = device_profile->converter_profile.ch12v_current.high_range.limit_low;
	converter_state.channel_12v.current_high_range.limit_high = device_profile->converter_profile.ch12v_current.high_range.limit_high;
	converter_state.channel_12v.current_high_range.enable_low_limit = device_profile->converter_profile.ch12v_current.high_range.enable_low_limit;
	converter_state.channel_12v.current_high_range.enable_high_limit = device_profile->converter_profile.ch12v_current.high_range.enable_high_limit;
	// Apply current range
	converter_state.channel_12v.current = (device_profile->converter_profile.ch12v_current.selected_range == CURRENT_RANGE_LOW) ? 
		&converter_state.channel_12v.current_low_range : &converter_state.channel_12v.current_high_range;
		
	// Overload
	converter_state.overload_protection_enable = device_profile->converter_profile.overload.protection_enable;
	converter_state.overload_warning_enable = device_profile->converter_profile.overload.warning_enable;
	converter_state.overload_threshold = device_profile->converter_profile.overload.threshold;
	
}


// Function should be accesible from outside
void Converter_SaveProfile(void)
{
	// 5V channel voltage
	device_profile->converter_profile.ch5v_voltage.setting = Converter_GetVoltageSetting(CHANNEL_5V);
	device_profile->converter_profile.ch5v_voltage.limit_low = Converter_GetVoltageLimitSetting(CHANNEL_5V, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_voltage.limit_high = Converter_GetVoltageLimitSetting(CHANNEL_5V, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_voltage.enable_low_limit = Converter_GetVoltageLimitState(CHANNEL_5V, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_voltage.enable_high_limit = Converter_GetVoltageLimitState(CHANNEL_5V, LIMIT_TYPE_HIGH);
	// Current
	device_profile->converter_profile.ch5v_current.low_range.setting = Converter_GetCurrentSetting(CHANNEL_5V, CURRENT_RANGE_LOW);
	device_profile->converter_profile.ch5v_current.low_range.limit_low = Converter_GetCurrentLimitSetting(CHANNEL_5V, CURRENT_RANGE_LOW, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_current.low_range.limit_high = Converter_GetCurrentLimitSetting(CHANNEL_5V, CURRENT_RANGE_LOW, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_current.low_range.enable_low_limit = Converter_GetCurrentLimitState(CHANNEL_5V, CURRENT_RANGE_LOW, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_current.low_range.enable_high_limit = Converter_GetCurrentLimitState(CHANNEL_5V, CURRENT_RANGE_LOW, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_current.high_range.setting = Converter_GetCurrentSetting(CHANNEL_5V, CURRENT_RANGE_HIGH);
	device_profile->converter_profile.ch5v_current.high_range.limit_low = Converter_GetCurrentLimitSetting(CHANNEL_5V, CURRENT_RANGE_HIGH, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_current.high_range.limit_high = Converter_GetCurrentLimitSetting(CHANNEL_5V, CURRENT_RANGE_HIGH, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_current.high_range.enable_low_limit = Converter_GetCurrentLimitState(CHANNEL_5V, CURRENT_RANGE_HIGH, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_current.high_range.enable_high_limit = Converter_GetCurrentLimitState(CHANNEL_5V, CURRENT_RANGE_HIGH, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_current.selected_range = Converter_GetCurrentRange(CHANNEL_5V);
	
	// 12V channel voltage
	device_profile->converter_profile.ch12v_voltage.setting = Converter_GetVoltageSetting(CHANNEL_12V);
	device_profile->converter_profile.ch12v_voltage.limit_low = Converter_GetVoltageLimitSetting(CHANNEL_12V, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_voltage.limit_high = Converter_GetVoltageLimitSetting(CHANNEL_12V, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_voltage.enable_low_limit = Converter_GetVoltageLimitState(CHANNEL_12V, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_voltage.enable_high_limit = Converter_GetVoltageLimitState(CHANNEL_12V, LIMIT_TYPE_HIGH);
	// Current
	device_profile->converter_profile.ch12v_current.low_range.setting = Converter_GetCurrentSetting(CHANNEL_12V, CURRENT_RANGE_LOW);
	device_profile->converter_profile.ch12v_current.low_range.limit_low = Converter_GetCurrentLimitSetting(CHANNEL_12V, CURRENT_RANGE_LOW, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_current.low_range.limit_high = Converter_GetCurrentLimitSetting(CHANNEL_12V, CURRENT_RANGE_LOW, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_current.low_range.enable_low_limit = Converter_GetCurrentLimitState(CHANNEL_12V, CURRENT_RANGE_LOW, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_current.low_range.enable_high_limit = Converter_GetCurrentLimitState(CHANNEL_12V, CURRENT_RANGE_LOW, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_current.high_range.setting = Converter_GetCurrentSetting(CHANNEL_12V, CURRENT_RANGE_HIGH);
	device_profile->converter_profile.ch12v_current.high_range.limit_low = Converter_GetCurrentLimitSetting(CHANNEL_12V, CURRENT_RANGE_HIGH, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_current.high_range.limit_high = Converter_GetCurrentLimitSetting(CHANNEL_12V, CURRENT_RANGE_HIGH, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_current.high_range.enable_low_limit = Converter_GetCurrentLimitState(CHANNEL_12V, CURRENT_RANGE_HIGH, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_current.high_range.enable_high_limit = Converter_GetCurrentLimitState(CHANNEL_12V, CURRENT_RANGE_HIGH, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_current.selected_range = Converter_GetCurrentRange(CHANNEL_12V);
	
	// Overload
	device_profile->converter_profile.overload.protection_enable = Converter_GetOverloadProtectionState();
	device_profile->converter_profile.overload.warning_enable = Converter_GetOverloadProtectionWarning();
	device_profile->converter_profile.overload.threshold = Converter_GetOverloadProtectionThreshold();
	
	// Other fields
	// ...
}


/*
static void Converter_LoadGlobalSettings(void)
{
	// Read data from global settings
	dac_settings.voltage_offset = global_settings->dac_voltage_offset;
	dac_settings.current_low_offset = global_settings->dac_current_low_offset;
	dac_settings.current_high_offset = global_settings->dac_current_high_offset;
	// Update DAC output
	SetVoltageDAC(converter_state.channel->voltage.setting);
	SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
}


void Converter_SaveGlobalSettings(void)
{
	global_settings->dac_voltage_offset = dac_settings.voltage_offset;
	global_settings->dac_current_low_offset = dac_settings.current_low_offset;
	global_settings->dac_current_high_offset = dac_settings.current_high_offset;
}
*/
	
// Converter initialization
void Converter_Init(void)
{
	// Set everything to 0
	memset(&converter_state, 0, sizeof(converter_state));
	//memset(&dac_settings, 0, sizeof(dac_settings));
	
	//---------- Channel 5V ------------//
	// Common
	converter_state.channel_5v.CHANNEL = CHANNEL_5V;
	converter_state.channel_5v.load_state = LOAD_ENABLE;										// dummy - load at 5V channel can not be disabled
	
	// Voltage
	converter_state.channel_5v.voltage.MINIMUM = CONV_MIN_VOLTAGE_5V_CHANNEL;					// Minimum voltage setting for channel
	converter_state.channel_5v.voltage.MAXIMUM = CONV_MAX_VOLTAGE_5V_CHANNEL;					// Maximum voltage setting for channel
	converter_state.channel_5v.voltage.LIMIT_MIN = CONV_MIN_VOLTAGE_5V_CHANNEL;					// Minimum voltage limit setting
	converter_state.channel_5v.voltage.LIMIT_MAX = CONV_MAX_VOLTAGE_5V_CHANNEL;					// Maximum voltage limit setting
	
	// Current
	converter_state.channel_5v.current_low_range.RANGE = CURRENT_RANGE_LOW;						// LOW current range
	converter_state.channel_5v.current_low_range.MINIMUM = CONV_LOW_CURRENT_RANGE_MIN;			// Minimum current setting for specified current range
	converter_state.channel_5v.current_low_range.MAXIMUM = CONV_LOW_CURRENT_RANGE_MAX;			// Maximum current setting for specified current range
	converter_state.channel_5v.current_low_range.LIMIT_MIN = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current limit setting
	converter_state.channel_5v.current_low_range.LIMIT_MAX = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current limit setting
	converter_state.channel_5v.current_high_range.RANGE = CURRENT_RANGE_HIGH;					// HIGH current range
	converter_state.channel_5v.current_high_range.MINIMUM = CONV_HIGH_CURRENT_RANGE_MIN;		// Minimum current setting for specified current range
	converter_state.channel_5v.current_high_range.MAXIMUM = CONV_HIGH_CURRENT_RANGE_MAX;		// Maximum current setting for specified current range
	converter_state.channel_5v.current_high_range.LIMIT_MIN = CONV_HIGH_CURRENT_RANGE_MIN;		// Minimum current limit setting
	converter_state.channel_5v.current_high_range.LIMIT_MAX = CONV_HIGH_CURRENT_RANGE_MAX;		// Maximum current limit setting
	
	//---------- Channel 12V -----------//
	
	// Common
	converter_state.channel_12v.CHANNEL = CHANNEL_12V;
	converter_state.channel_12v.load_state = LOAD_ENABLE;										
	
	// Voltage
	converter_state.channel_12v.voltage.MINIMUM = CONV_MIN_VOLTAGE_12V_CHANNEL;					// Minimum voltage setting for channel
	converter_state.channel_12v.voltage.MAXIMUM = CONV_MAX_VOLTAGE_12V_CHANNEL;					// Maximum voltage setting for channel
	converter_state.channel_12v.voltage.LIMIT_MIN = CONV_MIN_VOLTAGE_12V_CHANNEL;				// Minimum voltage limit setting
	converter_state.channel_12v.voltage.LIMIT_MAX = CONV_MAX_VOLTAGE_12V_CHANNEL;				// Maximum voltage limit setting	
	
	// Current
	converter_state.channel_12v.current_low_range.RANGE = CURRENT_RANGE_LOW;					// LOW current range
	converter_state.channel_12v.current_low_range.MINIMUM = CONV_LOW_CURRENT_RANGE_MIN;			// Minimum current setting for specified current range
	converter_state.channel_12v.current_low_range.MAXIMUM = CONV_LOW_CURRENT_RANGE_MAX;			// Maximum current setting for specified current range
	converter_state.channel_12v.current_low_range.LIMIT_MIN = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current limit setting
	converter_state.channel_12v.current_low_range.LIMIT_MAX = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current limit setting
	// 12V channel cannot provide currents > 20A (low range)
	converter_state.channel_12v.current_high_range.RANGE = CURRENT_RANGE_HIGH;					// HIGH current range
	converter_state.channel_12v.current_high_range.MINIMUM = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current setting for specified current range
	converter_state.channel_12v.current_high_range.MAXIMUM = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current setting for specified current range
	converter_state.channel_12v.current_high_range.LIMIT_MIN = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current limit setting
	converter_state.channel_12v.current_high_range.LIMIT_MAX = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current limit setting
	
	
	// Overload protection
	converter_state.overload_protection_enable = 1;
	converter_state.overload_warning_enable = 0;
	converter_state.overload_threshold = (10*1);
	
	// Default settings for channel and current ranges
	converter_state.channel_5v.current = &converter_state.channel_5v.current_low_range;
	converter_state.channel_12v.current = &converter_state.channel_12v.current_low_range;
	converter_state.channel = &converter_state.channel_12v;
	
	
	// Apply default configuration to hardware
	// Using functions without additional checks - safe here.
	Converter_TurnOff();
	while(Converter_IsReady() == 0);
	SetFeedbackChannel(converter_state.channel->CHANNEL);
	SetCurrentRange(converter_state.channel->current->RANGE);
	SetVoltageDAC(converter_state.channel->voltage.setting);
	SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
	SetOutputLoad(converter_state.channel->load_state);
	// Overload protection mechanism uses data from converter_state, so no
	// 	special settings are required.
	
	// Initialize charge framework
	converter_state.charge_status.status = CHARGE_INACTIVE;
}



// Helpers

void fillDispatchMessage(converter_message_t *msg, dispatch_msg_t *dispatcher_msg, uint8_t param)
{
	dispatcher_msg->type = DISPATCHER_CONVERTER_EVENT;
	dispatcher_msg->sender = sender_CONVERTER;
	if (msg)
		dispatcher_msg->converter_event.msg_sender = msg->sender;
	else
		dispatcher_msg->converter_event.msg_sender = sender_CONVERTER;
	// CHECKME - is that required?
	dispatcher_msg->converter_event.param = param;
	dispatcher_msg->converter_event.spec = 0;
	dispatcher_msg->converter_event.err_code = ERROR_NONE;			
}


// This function is called when converter state is changed - turned ON/OFF, started/stopped charging
// Arguments are:
//		err_code - error code
//		state_event - converter state change

static void stateResponse(converter_message_t *msg, uint8_t err_code, uint8_t state_event)
{	
	// Send notification to dispatcher
	dispatcher_msg.type = DISPATCHER_CONVERTER_EVENT;
	dispatcher_msg.sender = sender_CONVERTER;
	if (msg)
		dispatcher_msg.converter_event.msg_sender = msg->sender;
	else
		dispatcher_msg.converter_event.msg_sender = sender_CONVERTER;
	dispatcher_msg.converter_event.param = param_STATE;
	dispatcher_msg.converter_event.spec = state_event;
	dispatcher_msg.converter_event.err_code = err_code;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
}


static void msgConfirm(converter_message_t *msg)
{
	// Confirm
	if (msg->pxSemaphore)	xSemaphoreGive(*msg->pxSemaphore);
}









//---------------------------------------------//
//	Main converter task
//	
//---------------------------------------------//
void vTaskConverter(void *pvParameters) 
{	
	// Initialize
	xQueueConverter = xQueueCreate( 5, sizeof( converter_message_t ) );
	if( xQueueConverter == 0 )
		while(1);
	
	// Wait until task is started by dispatcher		
	//vTaskSuspend(0);
	
	xQueueReset(xQueueConverter);
	while(1) {
		xQueueReceive(xQueueConverter, &msg, portMAX_DELAY);	
		switch (msg.type) {
			case CONVERTER_CONTROL:
				if (msg.param == param_STATE)
					processStateControl(msg.a.state_set.command);
				else
					processParamControl();
				break;
			case CONVERTER_PROFILE_CMD:
				processProfileControl();
				break;
			case CONVERTER_ADC_MEASUREMENT_READY:

				break;
			case CONVERTER_TICK:
				processStateControl(event_TICK);
				break;
			case CONVERTER_OVERLOAD_EVENT:
				processStateControl(event_OVERLOAD);
				break;
			default:	// unknown
				break;
		}
	}
}	
		
		

//=========================================================================//
//	Control command processor
//=========================================================================//
static void processStateControl (uint8_t cmd_code) {
	uint8_t err_code = ERROR_NONE;
	uint8_t state_event = CONV_NO_STATE_CHANGE;
	uint8_t temp8u;
	
	switch (cmd_code) {
		case cmd_GET_READY_FOR_CHANNEL_SWITCH:
		case cmd_GET_READY_FOR_PROFILE_LOAD:
			if (converter_state.state == CONVERTER_STATE_ON) {
				Converter_TurnOff();
				converter_state.state = CONVERTER_STATE_OFF;
				stateResponse(0, ERROR_NONE, CONV_TURNED_OFF);				
			}
			else if (converter_state.state == CONVERTER_STATE_CHARGING)	{
				// Stop charge FSM
				Converter_TurnOff();
				SetOutputLoad(converter_state.channel->load_state);
				converter_state.state = CONVERTER_STATE_OFF;
				processChargeControl(cmd_STOP_CHARGE);
				stateResponse(0, ERROR_NONE, CONV_ABORTED_CHARGE);
			}
			else if (converter_state.state == CONVERTER_STATE_OVERLOADED) {
				// Just reset flag
				converter_state.state = CONVERTER_STATE_OFF;
				// stateResponse(ERROR_NONE, CONV_RESET_OVERLOAD_FLAG);		CHECKME
			}
			else {
				// OFF - do nothing
			}
			break;
		case cmd_TURN_ON:
			if ((converter_state.state == CONVERTER_STATE_OFF) || (converter_state.state == CONVERTER_STATE_OVERLOADED)) {
				// Resistive output load could be disabled by charger - enable it
				SetOutputLoad(converter_state.channel->load_state);
				// Command to low-level
				temp8u = Converter_TurnOn();
				if (temp8u == CONVERTER_CMD_OK) {
					converter_state.state = CONVERTER_STATE_ON;
					state_event = CONV_TURNED_ON;
				} 
				else {
					err_code = ERROR_INTERNAL;
				}
				msgConfirm(&msg);
				stateResponse(&msg, err_code, state_event);
			} else {
				msgConfirm(&msg);	// Converter is already ON or CHARGING - only give semaphore
			}
			break;
		case cmd_START_CHARGE:
			if (converter_state.state != CONVERTER_STATE_CHARGING) {
				if (converter_state.channel->CHANNEL == CHANNEL_12V) {				
					if (converter_state.state != CONVERTER_STATE_ON) {
						// Command to low-level
						temp8u = Converter_TurnOn();
						if (temp8u != CONVERTER_CMD_OK) {
							err_code = ERROR_INTERNAL;
						}
					}
					if (err_code == ERROR_NONE) {
						// Resistive output load must be disabled
						SetOutputLoad(0);
						// Process charge FSM
						processChargeControl(cmd_START_CHARGE);
						converter_state.state = CONVERTER_STATE_CHARGING;
						state_event = CONV_STARTED_CHARGE;
					}
				}
				else {
					// Can charge only by 12V channel
					err_code = ERROR_CMD;	
				}
			}
			else {
				// CHARGING already - restart?
			}
			msgConfirm(&msg);
			stateResponse(&msg, err_code, state_event);
			break;
		case cmd_TURN_OFF:
			temp8u = converter_state.state;
			Converter_TurnOff();
			converter_state.state = CONVERTER_STATE_OFF;
			if (temp8u == CONVERTER_STATE_CHARGING) {
				processChargeControl(cmd_STOP_CHARGE);
				state_event = CONV_ABORTED_CHARGE;
			} else {
				// Resistive output load was disabled by charger and it is second STOP command - enable it
				SetOutputLoad(converter_state.channel->load_state);
				state_event = CONV_TURNED_OFF;
			}
			msgConfirm(&msg);
			stateResponse(&msg, err_code, state_event);
			break;
		case event_OVERLOAD:
			// Hardware has been overloaded and disabled by low-level interrupt-driven routine.
			temp8u = converter_state.state;
			converter_state.state = CONVERTER_STATE_OVERLOADED;
			// Reset that routine FSM flag
			if (Converter_ClearOverloadFlag() != CONVERTER_CMD_OK) {
				// Error, we should never get here.
				// Kind of restore attempt
				Converter_TurnOff();
				err_code = ERROR_INTERNAL;
			}
			stateResponse(0, err_code, CONV_OVERLOADED);
			if (temp8u == CONVERTER_STATE_CHARGING) {
				// Stop charge FSM
				processChargeControl(cmd_STOP_CHARGE);
				stateResponse(0, ERROR_NONE, CONV_ABORTED_CHARGE);
				// Do not enable resistive load here
			}			
			break;
		case event_TICK:		// Temporary! Will be special for charging mode
			// ADC task is responsible for sampling and filtering voltage and current
			adc_msg = ADC_GET_ALL_NORMAL;
			xQueueSendToBack(xQueueADC, &adc_msg, 0);
			break;
	}
}


//=========================================================================//
//	Parameters setting processor
//=========================================================================//
static void processParamControl (void)
{
	uint8_t err_code = ERROR_NONE;
	uint8_t op_result = 0;
	uint8_t op_result2 = 0;
	uint8_t temp8u;
	channel_state_t *c;
	switch (msg.param) {
		//-------------------- Setting voltage --------------------//
		case param_VSET:
			msg.a.v_set.channel = Converter_ValidateChannel(msg.a.v_set.channel);
			taskENTER_CRITICAL();
			op_result = Converter_SetVoltage(msg.a.v_set.channel, msg.a.v_set.value);
			taskEXIT_CRITICAL();
			// Apply new setting to hardware. CHECKME: charge state
			if ((op_result & VALUE_UPDATED) && (msg.a.v_set.channel == converter_state.channel->CHANNEL)) {
				SetVoltageDAC(converter_state.channel->voltage.setting);
			}
			msgConfirm(&msg);
			fillDispatchMessage(&msg, &dispatcher_msg, param_VSET);
			dispatcher_msg.converter_event.channel = msg.a.v_set.channel;
			dispatcher_msg.converter_event.spec = op_result;
			xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			break;	
		//-------------------- Setting current --------------------//
		case param_CSET:
			msg.a.c_set.channel = Converter_ValidateChannel(msg.a.c_set.channel);
			msg.a.c_set.range = Converter_ValidateCurrentRange(msg.a.c_set.range);
			taskENTER_CRITICAL();
			op_result = Converter_SetCurrent(msg.a.c_set.channel, msg.a.c_set.range, msg.a.c_set.value);
			taskEXIT_CRITICAL();
			// Apply new setting to hardware. CHECKME: charge state
			if ((op_result & VALUE_UPDATED) && (msg.a.c_set.channel == converter_state.channel->CHANNEL) && 
				(msg.a.c_set.range == converter_state.channel->current->RANGE))	{
				SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
			}
			msgConfirm(&msg);
			fillDispatchMessage(&msg, &dispatcher_msg, param_CSET);
			dispatcher_msg.converter_event.channel = msg.a.c_set.channel;
			dispatcher_msg.converter_event.range = msg.a.c_set.range;
			dispatcher_msg.converter_event.spec = op_result;
			xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			break;
		//----------------- Setting voltage limit -----------------//
		case param_VLIMIT:
			msg.a.vlim_set.channel = Converter_ValidateChannel(msg.a.vlim_set.channel);
			taskENTER_CRITICAL();
			op_result = Converter_SetVoltageLimit(msg.a.vlim_set.channel, msg.a.vlim_set.type, 
								msg.a.vlim_set.value, msg.a.vlim_set.enable);
			// Ensure that voltage setting lies inside new limits
			if (op_result & VALUE_UPDATED)
				op_result2 = Converter_SetVoltage(msg.a.vlim_set.channel, 
									getVoltageRegData(msg.a.vlim_set.channel)->setting);
			taskEXIT_CRITICAL();
			// Apply new setting to hardware. CHECKME: charge state				
			if ((op_result2 & VALUE_UPDATED) && (msg.a.vlim_set.channel == converter_state.channel->CHANNEL)) {
				SetVoltageDAC(converter_state.channel->voltage.setting);
			}
			msgConfirm(&msg);
			fillDispatchMessage(&msg, &dispatcher_msg, param_VLIMIT);
			dispatcher_msg.converter_event.channel = msg.a.vlim_set.channel;
			dispatcher_msg.converter_event.limit_type = msg.a.vlim_set.type;
			dispatcher_msg.converter_event.spec = op_result;
			xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			if (op_result2 & VALUE_UPDATED) {
				fillDispatchMessage(0, &dispatcher_msg, param_VSET);
				dispatcher_msg.converter_event.channel = msg.a.vlim_set.channel;
				dispatcher_msg.converter_event.spec = op_result2 | VALUE_UPFORCED;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			}
			break; 
		//----------------- Setting current limit -----------------//
		case param_CLIMIT:
			msg.a.clim_set.channel = Converter_ValidateChannel(msg.a.clim_set.channel);
			msg.a.clim_set.range = Converter_ValidateCurrentRange(msg.a.clim_set.range);
			taskENTER_CRITICAL();			
			op_result = Converter_SetCurrentLimit(msg.a.clim_set.channel, msg.a.clim_set.range, msg.a.clim_set.type, 
								msg.a.clim_set.value, msg.a.clim_set.enable);
								
			// Ensure that current setting lies inside new limits
			if (op_result & VALUE_UPDATED)
				op_result2 = Converter_SetCurrent(msg.a.clim_set.channel, msg.a.clim_set.range, 
									getCurrentRegData(msg.a.clim_set.channel, msg.a.clim_set.range)->setting);
			taskEXIT_CRITICAL();
			// Apply new setting to hardware. CHECKME: charge state	
			if ((op_result2 & VALUE_UPDATED) && (msg.a.clim_set.channel == converter_state.channel->CHANNEL) && 
				(msg.a.clim_set.range == converter_state.channel->current->RANGE)) {
				SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
			}
			msgConfirm(&msg);
			fillDispatchMessage(&msg, &dispatcher_msg, param_CLIMIT);
			dispatcher_msg.converter_event.channel = msg.a.clim_set.channel;
			dispatcher_msg.converter_event.range = msg.a.clim_set.range;
			dispatcher_msg.converter_event.limit_type = msg.a.clim_set.type;
			dispatcher_msg.converter_event.spec = op_result;
			xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			if (op_result2 & VALUE_UPDATED) {
				fillDispatchMessage(0, &dispatcher_msg, param_CSET);
				dispatcher_msg.converter_event.channel = msg.a.clim_set.channel;
				dispatcher_msg.converter_event.range = msg.a.clim_set.range;
				dispatcher_msg.converter_event.spec = op_result2 | VALUE_UPFORCED;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			}
			break; 
		//-------------------- Setting channel --------------------//
		case param_CHANNEL:
			msg.a.ch_set.new_channel = Converter_ValidateChannel(msg.a.ch_set.new_channel);
			if (msg.a.ch_set.new_channel != converter_state.channel->CHANNEL)
			{
				// Disable converter if enabled
				processStateControl(cmd_GET_READY_FOR_CHANNEL_SWITCH);
				// Wait until voltage feedback switching is allowed
				while(Converter_IsReady() == 0) 
					vTaskDelay(1);
				// Apply new channel setting to hardware
				temp8u = Converter_SetFeedBackChannel(msg.a.ch_set.new_channel);
				if (temp8u == CONVERTER_CMD_OK)	{
					taskENTER_CRITICAL();	
					converter_state.channel = (msg.a.ch_set.new_channel == CHANNEL_5V) ? &converter_state.channel_5v : 
											&converter_state.channel_12v;
					taskEXIT_CRITICAL();
					while(Converter_IsReady() == 0) 
						vTaskDelay(1);	
					// Apply new channel current range setting to hardware
					temp8u = Converter_SetCurrentRange(converter_state.channel->current->RANGE);
					op_result = VALUE_UPDATED;
					if (temp8u == CONVERTER_CMD_OK) {
						SetVoltageDAC(converter_state.channel->voltage.setting);
						SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
						SetOutputLoad(converter_state.channel->load_state);
					} else {
						// Unexpected error for current range setting
						err_code = ERROR_INTERNAL;
					} 
				} else {
					// Unexpected error for channel setting
					err_code = ERROR_INTERNAL;
				}
			} else {
				// Trying to set channel that is already selected.
				op_result = VALUE_BOUND_MASK;
			}
			msgConfirm(&msg);
			fillDispatchMessage(&msg, &dispatcher_msg, param_CHANNEL);
			dispatcher_msg.converter_event.spec = op_result;
			dispatcher_msg.converter_event.err_code = err_code;
			xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			break;
		//----------------- Setting current range -----------------//
		case param_CRANGE:
			msg.a.crange_set.channel = Converter_ValidateChannel(msg.a.crange_set.channel);	
			msg.a.crange_set.new_range = Converter_ValidateCurrentRange(msg.a.crange_set.new_range);
			c = (msg.a.crange_set.channel == CHANNEL_5V) ? &converter_state.channel_5v : 
										&converter_state.channel_12v;	
			if (msg.a.crange_set.new_range != c->current->RANGE) {
				// New current range differs from current
				if (msg.a.crange_set.channel == converter_state.channel->CHANNEL) {
					// Setting current range of the active channel
					if ((converter_state.state != CONVERTER_STATE_ON) && (converter_state.state != CONVERTER_STATE_CHARGING)) {
						// Wait until current feedback switching is allowed
						while(Converter_IsReady() == 0) 
							vTaskDelay(1);	
						// Apply new channel current range setting to hardware
						temp8u = Converter_SetCurrentRange(msg.a.crange_set.new_range);
						if (temp8u == CONVERTER_CMD_OK) {
							taskENTER_CRITICAL();
							c->current = (msg.a.crange_set.new_range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
							taskEXIT_CRITICAL();
							SetCurrentDAC(c->current->setting, c->current->RANGE);
							op_result = VALUE_UPDATED;
						} else {
							// Unexpected error
							err_code = ERROR_INTERNAL;
						}
					} else {
						// Converter in ON and current range cannot be changed
						err_code = ERROR_CMD;
					}
				} else {
					// Setting current range of the inactive channel
					taskENTER_CRITICAL();
					c->current = (msg.a.crange_set.new_range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
					taskEXIT_CRITICAL();
					op_result = VALUE_UPDATED;
				}
			} else {
				// Trying to set current range that is already selected.
				op_result = VALUE_BOUND_MASK;
			}
			msgConfirm(&msg);
			fillDispatchMessage(&msg, &dispatcher_msg, param_CRANGE);
			dispatcher_msg.converter_event.channel = msg.a.crange_set.channel;
			dispatcher_msg.converter_event.spec = op_result;
			dispatcher_msg.converter_event.err_code = err_code;
			xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			break;
		//-------- Setting overload protection parameters ---------//
		case param_OVERLOAD_PROTECTION:
			taskENTER_CRITICAL();
			op_result = Converter_SetOverloadProtection( msg.a.overload_set.protection_enable, msg.a.overload_set.warning_enable, 
												msg.a.overload_set.threshold);
			taskEXIT_CRITICAL();
			msgConfirm(&msg);
			fillDispatchMessage(&msg, &dispatcher_msg, param_OVERLOAD_PROTECTION);
			dispatcher_msg.converter_event.spec = op_result;
			xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			break; 
		//-------------------- Setting DAC offset -----------------//
		case param_DAC_OFFSET:
			taskENTER_CRITICAL();
			global_settings->dac_voltage_offset = msg.a.dac_set.voltage_offset;
			global_settings->dac_current_low_offset = msg.a.dac_set.current_low_offset;
			global_settings->dac_current_high_offset = msg.a.dac_set.current_high_offset;
			taskEXIT_CRITICAL();
			// Update DAC output
			SetVoltageDAC(converter_state.channel->voltage.setting);
			SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
			msgConfirm(&msg);
			fillDispatchMessage(&msg, &dispatcher_msg, param_DAC_OFFSET);
			dispatcher_msg.converter_event.spec = VALUE_UPDATED;
			xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			break;
	}
}


//=========================================================================//
//	Profile command processor
//=========================================================================//
static void processProfileControl (void)
{
	uint8_t temp8u;
	//uint8_t err_code = ERROR_NONE;
	//uint8_t op_result = 0;
	switch (msg.param) {
		//---------------- Loading profile ------------------------//
		case cmd_LOAD_PROFILE:
			// Disable converter if enabled
			processStateControl(cmd_GET_READY_FOR_PROFILE_LOAD);
			// Load profile data
			taskENTER_CRITICAL();
			Converter_LoadProfile();
			taskEXIT_CRITICAL();
			// Wait until current feedback switching is allowed
			while(Converter_IsReady() == 0) 
				vTaskDelay(1);	
			// Apply new channel current range setting to hardware
			temp8u = Converter_SetCurrentRange(converter_state.channel->current->RANGE);
			if (temp8u == CONVERTER_CMD_OK) {
				SetVoltageDAC(converter_state.channel->voltage.setting);
				SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
				SetOutputLoad(converter_state.channel->load_state);
				//op_result = VALUE_UPDATED;
			}
			else {
				//err_code = ERROR_INTERNAL;
			} 
			msgConfirm(&msg);
		break;
		
		//----------------- Saving profile ------------------------//
		case cmd_SAVE_PROFILE:
			Converter_SaveProfile();
			msgConfirm(&msg);
			break;
/*			//=========================================================================//
			// Loading global settings
			case CONVERTER_LOAD_GLOBAL_SETTINGS:
				Converter_LoadGlobalSettings();
				// Confirm
				if (msg.pxSemaphore != 0)
					xSemaphoreGive(*msg.pxSemaphore);
				break;
			//=========================================================================//
			// Saving global settings
			case CONVERTER_SAVE_GLOBAL_SETTINGS:
				Converter_SaveGlobalSettings();
				// Confirm
				if (msg.pxSemaphore != 0)
					xSemaphoreGive(*msg.pxSemaphore);
				break;
			//=========================================================================//
*/
		default:	// unknown
			break;
	}
}



//---------------------------------------------//
//	Charge process controller
//	
//---------------------------------------------//
static void processChargeControl(uint8_t cmd)
{
	switch (cmd)
	{
	/*	case CHARGE_START:
			break;
		case cmd_SHARGE_STOP:
			break;
		case CHARGE_ABORT:
			break;
		case CHARGE_TICK:
			break; */
	}
}



		
				
				



