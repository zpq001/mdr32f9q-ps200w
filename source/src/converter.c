

#include "MDR32Fx.h" 

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#include "sound_driver.h"
#include "control.h"
#include "converter.h"
#include "converter_hw.h"
#include "adc.h"

#include "guiTop.h"

//-------------------------------------------------------//
// Voltage and current setting return codes
#define VALUE_OK					0x00
#define VALUE_BOUND_BY_SOFT_MAX		0x01
#define VALUE_BOUND_BY_SOFT_MIN		0x02
#define VALUE_BOUND_BY_ABS_MAX		0x04
#define VALUE_BOUND_BY_ABS_MIN		0x08


//-------------------------------------------------------//
// Converter control functions return values

// these can be set simultaneously
#define VOLTAGE_SETTING_APPLIED			0x01	
#define VOLTAGE_LIMIT_APPLIED			0x02
//#define VOLTAGE_SETTING_ARG_MODIFIED	0x04
//#define VOLTAGE_LIMIT_ARG_MODIFIED		0x08
// these can be set simultaneously
#define CURRENT_SETTING_APPLIED			0x01	
#define CURRENT_LIMIT_APPLIED			0x02
//#define CURRENT_SETTING_ARG_MODIFIED	0x04
//#define CURRENT_LIMIT_ARG_MODIFIED		0x08

//-------------------------------------------------------//
// Arguments of Converter_ApplyControls
//#define APPLY_CHANNEL			0x01
//#define APPLY_CURRENT_RANGE		0x02
//#define APPLY_OUTPUT_LOAD		0x04
//#define APPLY_VOLTAGE			0x10
//#define APPLY_CURRENT			0x20


// OS stuff
xQueueHandle xQueueConverter;
xTaskHandle xTaskHandle_Converter;
const converter_message_t converter_overload_msg = {CONVERTER_OVERLOADED};
const converter_message_t converter_tick_message = {CONVERTER_TICK};

gui_msg_t gui_msg;
uint32_t adc_msg;

converter_state_t converter_state;		// main converter control



//---------------------------------------------------------------------------//
///	Checks and sets new value in regulation structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationValue(reg_setting_t *s, int32_t new_value, uint8_t *errCode)
{
	uint8_t result = 0;
	*errCode = VALUE_OK;
	// Check software limits
	if ((s->enable_low_limit) && (new_value <= (int32_t)s->limit_low))
	{	
		new_value = (int32_t)s->limit_low;
		if (errCode) *errCode = VALUE_BOUND_BY_SOFT_MIN;
	}
	if ((s->enable_high_limit) && (new_value >= (int32_t)s->limit_high)) 
	{
		new_value = (int32_t)s->limit_high;
		if (errCode) *errCode = VALUE_BOUND_BY_SOFT_MAX;
	} 
	// Check absolute limits
	if (new_value <= (int32_t)s->MINIMUM)
	{	
		new_value = (int32_t)s->MINIMUM;
		if (errCode) *errCode = VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->MAXIMUM) 
	{
		new_value = (int32_t)s->MAXIMUM;
		if (errCode) *errCode = VALUE_BOUND_BY_ABS_MAX;
	} 
	if (s->setting != new_value)
	{
		s->setting = new_value;
		result = 1;
	}
	return result;
}


//---------------------------------------------------------------------------//
///	Checks and sets new value limit in regultaion structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationLimit(reg_setting_t *s, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *errCode)
{
	uint8_t result = 0;
	*errCode = VALUE_OK;
	if (new_value <= (int32_t)s->LIMIT_MIN)
	{	
		new_value = (int32_t)s->LIMIT_MIN;
		if (errCode) *errCode = VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->LIMIT_MAX) 
	{
		new_value = (int32_t)s->LIMIT_MAX;
		if (errCode) *errCode = VALUE_BOUND_BY_ABS_MAX;
	} 
	if (limit_type == LIMIT_TYPE_LOW)
	{
		if ((s->limit_low != new_value) || (s->enable_low_limit != enable))
		{
			s->limit_low = new_value;
			s->enable_low_limit = enable;
			result = 1;
		}
	}
	else if (limit_type == LIMIT_TYPE_HIGH)
	{
		if ((s->limit_high != new_value) || (s->enable_high_limit != enable))
		{
			s->limit_high = new_value;
			s->enable_high_limit = enable;
			result = 1;
		}
	}
	return result;
}


//---------------------------------------------------------------------------//
///	Set converter output voltage
//	input:
//		channel - converter channel to affect: CHANNEL_5V / CHANNEL_12V / OPERATING_CHANNEL
//		new_value - new value to check and set
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		VOLTAGE_SETTING_APPLIED 	- if new setting has been set
//---------------------------------------------------------------------------//
static uint8_t Converter_SetVoltage(uint8_t channel, int32_t new_value, uint8_t *err_code)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : 
						 ((channel == CHANNEL_12V) ? &converter_state.channel_12v : converter_state.channel);
	uint8_t result = 0x00;		
	if (SetRegulationValue(&c->voltage, new_value, err_code))
	{
		result |= VOLTAGE_SETTING_APPLIED;
	}
	return result;
}


//---------------------------------------------------------------------------//
///	Set converter voltage limit
//	input:
//		channel - converter channel to affect: CHANNEL_5V / CHANNEL_12V / OPERATING_CHANNEL
//		limit_type - low or high limit to modify
//		new_value - new value to check and set
//		enable - set limit enabled or disabled
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		VOLTAGE_SETTING_APPLIED - if new voltage setting has been applied
//		VOLTAGE_LIMIT_APPLIED	- if new limit setting has been applied
//---------------------------------------------------------------------------//
static uint8_t Converter_SetVoltageLimit(uint8_t channel, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : 
					 ((channel == CHANNEL_12V) ? &converter_state.channel_12v : converter_state.channel);
	uint8_t result = 0;
	if (SetRegulationLimit(&c->voltage, limit_type, new_value, enable, err_code))
	{
		result |= VOLTAGE_LIMIT_APPLIED;
		// Ensure that voltage setting lies inside new limits
		result |= Converter_SetVoltage(channel, c->voltage.setting, 0);
	}
	return result;
}


//---------------------------------------------------------------------------//
///	Set converter output current
//	input:
//		channel - converter channel to affect: CHANNEL_5V / CHANNEL_12V / OPERATING_CHANNEL
//		current_range - 20A range or 40A range to modify. Can be OPERATING_CURRENT_RANGE.
//		new_value - new value to check and set
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		CURRENT_SETTING_APPLIED 	- if new setting has been set
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrent(uint8_t channel, uint8_t current_range, int32_t new_value, uint8_t *err_code)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : 
						 ((channel == CHANNEL_12V) ? &converter_state.channel_12v : converter_state.channel);
	reg_setting_t *s = (current_range == CURRENT_RANGE_LOW) ? &c->current_low_range : 
						((current_range == CURRENT_RANGE_HIGH) ? &c->current_high_range : c->current);
	uint8_t result = 0;
	if (SetRegulationValue(s, new_value, err_code))
	{
		result = CURRENT_SETTING_APPLIED;
	}
	return 	result;			   
}


//---------------------------------------------------------------------------//
/// Set converter current limit
//	input:
//		channel - converter channel to affect: CHANNEL_5V / CHANNEL_12V / OPERATING_CHANNEL
//		current_range - 20A range or 40A range to modify. Can be OPERATING_CURRENT_RANGE.
//		limit_type - low or high limit to modify
//		new_value - new value to check and set
//		enable - set limit enabled or disabled
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		CURRENT_SETTING_APPLIED - if new current setting has been applied
//		CURRENT_LIMIT_APPLIED	- if new limit setting has been applied
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : 
						 ((channel == CHANNEL_12V) ? &converter_state.channel_12v : converter_state.channel);
	reg_setting_t *s = (current_range == CURRENT_RANGE_LOW) ? &c->current_low_range : 
						((current_range == CURRENT_RANGE_HIGH) ? &c->current_high_range : c->current);
	uint8_t result = 0;
	if (SetRegulationLimit(s, limit_type, new_value, enable, err_code))
	{
		result = CURRENT_LIMIT_APPLIED;
		// Ensure that current setting lies inside new limits
		result |= Converter_SetCurrent(channel, current_range, s->setting, 0);
	}
	return result;
}


//---------------------------------------------------------------------------//
/// Set converter overload parameters
// new_threshold in units of 100us
// Converter HW overload threshold in units of 200us
//---------------------------------------------------------------------------//
static void Converter_SetOverloadProtection(uint8_t protectionEnable, uint8_t warningEnable, int32_t new_threshold, uint8_t *err_code)
{
	new_threshold /= 2;
	if (new_threshold < CONV_MIN_OVERLOAD_THRESHOLD)
	{
		new_threshold = CONV_MIN_OVERLOAD_THRESHOLD;
		*err_code = VALUE_BOUND_BY_ABS_MIN;
	}
	else if (new_threshold > CONV_MAX_OVERLOAD_THRESHOLD)
	{
		new_threshold = CONV_MAX_OVERLOAD_THRESHOLD;
		*err_code = VALUE_BOUND_BY_ABS_MAX;
	}
	else
	{
		*err_code = 0;
	}
	
	converter_state.overload_protection_enable = protectionEnable;
	converter_state.overload_warning_enable = warningEnable;
	converter_state.overload_threshold = new_threshold;
}	


//---------------------------------------------------------------------------//
/// Checks if channel has a valid value
// Side-effect: if *channel = OPERATING_CHANNEL (special case), 
//	functions modifies channel to currently operating.
//---------------------------------------------------------------------------//
static uint8_t Converter_ValidateChannel(uint8_t *channel)
{
	if (*channel == OPERATING_CHANNEL)
	{
		*channel = converter_state.channel->CHANNEL;
		return 1;
	}
	if ((*channel == CHANNEL_5V) || (*channel == CHANNEL_12V))
		return 1;
	else
		return 0;
}


//---------------------------------------------------------------------------//
/// Checks if current range has a valid value
// Side-effect: if *range = OPERATING_RANGE (special case), 
//	functions modifies range to currently operating.
//---------------------------------------------------------------------------//
static uint8_t Converter_ValidateCurrentRange(uint8_t *range)
{
	if (*range == OPERATING_CURRENT_RANGE)
	{
		*range = converter_state.channel->current->RANGE;
		return 1;
	}
	if ((*range == CURRENT_RANGE_LOW) || (*range == CURRENT_RANGE_HIGH))
		return 1;
	else
		return 0;
	
}


//---------------------------------------------------------------------------//
/// Checks if limit type has a valid value (LOW or HIGH)
//---------------------------------------------------------------------------//
static uint8_t Converter_ValidateLimit(uint8_t limit)
{
	if ((limit == LIMIT_TYPE_LOW) || (limit == LIMIT_TYPE_HIGH))
		return 1;
	else
		return 0;
}




static void SoundEvent_OnSettingChanged(uint8_t err_code)
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

static void SoundEvent_OnOverload(void)
{
	uint32_t sound_msg;
	sound_msg = SND_CONV_OVERLOADED | SND_CONVERTER_PRIORITY_HIGHEST;
	xQueueSendToBack(xQueueSound, &sound_msg, 0); 
}

static void SoundEvent_OnGenericError(void)
{
	uint32_t sound_msg;
	sound_msg = SND_CONV_CMD_ILLEGAL | SND_CONVERTER_PRIORITY_HIGHEST;	// FIXME
	xQueueSendToBack(xQueueSound, &sound_msg, 0);
}




void Converter_Init(uint8_t default_channel)
{
	// Converter is powered off.
	// TODO: add restore from EEPROM
	
	//---------- Channel 5V ------------//
	
	converter_state.channel_5v.CHANNEL = CHANNEL_5V;
	converter_state.channel_5v.load_state = LOAD_ENABLE;
	
	
	// Common
	converter_state.channel_5v.CHANNEL = CHANNEL_5V;
	converter_state.channel_5v.load_state = LOAD_ENABLE;										// dummy - load at 5V channel can not be disabled
	
	// Voltage
	converter_state.channel_5v.voltage.setting = 5000;											// TODO: EEPROM
	converter_state.channel_5v.voltage.MINIMUM = CONV_MIN_VOLTAGE_5V_CHANNEL;					// Minimum voltage setting for channel
	converter_state.channel_5v.voltage.MAXIMUM = CONV_MAX_VOLTAGE_5V_CHANNEL;					// Maximum voltage setting for channel
	converter_state.channel_5v.voltage.limit_low = CONV_MIN_VOLTAGE_5V_CHANNEL;					// TODO: EEPROM
	converter_state.channel_5v.voltage.limit_high = CONV_MAX_VOLTAGE_5V_CHANNEL;				// TODO: EEPROM
	converter_state.channel_5v.voltage.LIMIT_MIN = CONV_MIN_VOLTAGE_5V_CHANNEL;					// Minimum voltage limit setting
	converter_state.channel_5v.voltage.LIMIT_MAX = CONV_MAX_VOLTAGE_5V_CHANNEL;					// Maximum voltage limit setting
	converter_state.channel_5v.voltage.enable_low_limit = 0;									// TODO: EEPROM
	converter_state.channel_5v.voltage.enable_high_limit = 0;									// TODO: EEPROM
	
	// Current
	converter_state.channel_5v.current_low_range.RANGE = CURRENT_RANGE_LOW;						// LOW current range
	converter_state.channel_5v.current_low_range.setting = 4000;								// TODO: EEPROM
	converter_state.channel_5v.current_low_range.MINIMUM = CONV_LOW_CURRENT_RANGE_MIN;			// Minimum current setting for specified current range
	converter_state.channel_5v.current_low_range.MAXIMUM = CONV_LOW_CURRENT_RANGE_MAX;			// Maximum current setting for specified current range
	converter_state.channel_5v.current_low_range.limit_low = CONV_LOW_CURRENT_RANGE_MIN;		// TODO: EEPROM
	converter_state.channel_5v.current_low_range.limit_high = CONV_LOW_CURRENT_RANGE_MAX;		// TODO: EEPROM
	converter_state.channel_5v.current_low_range.LIMIT_MIN = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current limit setting
	converter_state.channel_5v.current_low_range.LIMIT_MAX = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current limit setting
	converter_state.channel_5v.current_low_range.enable_low_limit = 0;							// TODO: EEPROM
	converter_state.channel_5v.current_low_range.enable_high_limit = 0;							// TODO: EEPROM
	
	converter_state.channel_5v.current_high_range.RANGE = CURRENT_RANGE_HIGH;					// HIGH current range
	converter_state.channel_5v.current_high_range.setting = 4000;								// TODO: EEPROM
	converter_state.channel_5v.current_high_range.MINIMUM = CONV_HIGH_CURRENT_RANGE_MIN;		// Minimum current setting for specified current range
	converter_state.channel_5v.current_high_range.MAXIMUM = CONV_HIGH_CURRENT_RANGE_MAX;		// Maximum current setting for specified current range
	converter_state.channel_5v.current_high_range.limit_low = CONV_HIGH_CURRENT_RANGE_MIN;		// TODO: EEPROM
	converter_state.channel_5v.current_high_range.limit_high = CONV_HIGH_CURRENT_RANGE_MAX;		// TODO: EEPROM
	converter_state.channel_5v.current_high_range.LIMIT_MIN = CONV_HIGH_CURRENT_RANGE_MIN;		// Minimum current limit setting
	converter_state.channel_5v.current_high_range.LIMIT_MAX = CONV_HIGH_CURRENT_RANGE_MAX;		// Maximum current limit setting
	converter_state.channel_5v.current_high_range.enable_low_limit = 0;							// TODO: EEPROM
	converter_state.channel_5v.current_high_range.enable_high_limit = 0;						// TODO: EEPROM

	converter_state.channel_5v.current = &converter_state.channel_5v.current_low_range;
	
	//---------- Channel 12V -----------//
	
	// Common
	converter_state.channel_12v.CHANNEL = CHANNEL_12V;
	converter_state.channel_12v.load_state = LOAD_ENABLE;										
	
	// Voltage
	converter_state.channel_12v.voltage.setting = 12000;										// TODO: EEPROM
	converter_state.channel_12v.voltage.MINIMUM = CONV_MIN_VOLTAGE_12V_CHANNEL;					// Minimum voltage setting for channel
	converter_state.channel_12v.voltage.MAXIMUM = CONV_MAX_VOLTAGE_12V_CHANNEL;					// Maximum voltage setting for channel
	converter_state.channel_12v.voltage.limit_low = CONV_MIN_VOLTAGE_12V_CHANNEL;				// TODO: EEPROM
	converter_state.channel_12v.voltage.limit_high = CONV_MAX_VOLTAGE_12V_CHANNEL;				// TODO: EEPROM
	converter_state.channel_12v.voltage.LIMIT_MIN = CONV_MIN_VOLTAGE_12V_CHANNEL;				// Minimum voltage limit setting
	converter_state.channel_12v.voltage.LIMIT_MAX = CONV_MAX_VOLTAGE_12V_CHANNEL;				// Maximum voltage limit setting
	converter_state.channel_12v.voltage.enable_low_limit = 0;									// TODO: EEPROM
	converter_state.channel_12v.voltage.enable_high_limit = 0;									// TODO: EEPROM
	
	// Current
	converter_state.channel_12v.current_low_range.RANGE = CURRENT_RANGE_LOW;					// LOW current range
	converter_state.channel_12v.current_low_range.setting = 2000;								// TODO: EEPROM
	converter_state.channel_12v.current_low_range.MINIMUM = CONV_LOW_CURRENT_RANGE_MIN;			// Minimum current setting for specified current range
	converter_state.channel_12v.current_low_range.MAXIMUM = CONV_LOW_CURRENT_RANGE_MAX;			// Maximum current setting for specified current range
	converter_state.channel_12v.current_low_range.limit_low = CONV_LOW_CURRENT_RANGE_MIN;		// TODO: EEPROM
	converter_state.channel_12v.current_low_range.limit_high = CONV_LOW_CURRENT_RANGE_MAX;		// TODO: EEPROM
	converter_state.channel_12v.current_low_range.LIMIT_MIN = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current limit setting
	converter_state.channel_12v.current_low_range.LIMIT_MAX = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current limit setting
	converter_state.channel_12v.current_low_range.enable_low_limit = 0;							// TODO: EEPROM
	converter_state.channel_12v.current_low_range.enable_high_limit = 0;						// TODO: EEPROM
	
	// 12V channel cannot provide currents > 20A (low range)
	converter_state.channel_12v.current_high_range.RANGE = CURRENT_RANGE_HIGH;					// HIGH current range
	converter_state.channel_12v.current_high_range.setting = 4000;								// TODO: EEPROM
	converter_state.channel_12v.current_high_range.MINIMUM = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current setting for specified current range
	converter_state.channel_12v.current_high_range.MAXIMUM = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current setting for specified current range
	converter_state.channel_12v.current_high_range.limit_low = CONV_LOW_CURRENT_RANGE_MIN;		// TODO: EEPROM
	converter_state.channel_12v.current_high_range.limit_high = CONV_LOW_CURRENT_RANGE_MAX;		// TODO: EEPROM
	converter_state.channel_12v.current_high_range.LIMIT_MIN = CONV_LOW_CURRENT_RANGE_MIN;		// Minimum current limit setting
	converter_state.channel_12v.current_high_range.LIMIT_MAX = CONV_LOW_CURRENT_RANGE_MAX;		// Maximum current limit setting
	converter_state.channel_12v.current_high_range.enable_low_limit = 0;						// TODO: EEPROM
	converter_state.channel_12v.current_high_range.enable_high_limit = 0;						// TODO: EEPROM

	converter_state.channel_12v.current = &converter_state.channel_12v.current_low_range;
	
	converter_state.overload_protection_enable = 1;									// TODO: EEPROM
	converter_state.overload_warning_enable = 0;									// TODO: EEPROM
	converter_state.overload_threshold = (5*1);										// TODO: EEPROM
	
	// Select default channel
	converter_state.channel = (default_channel == CHANNEL_5V) ? &converter_state.channel_5v : 
								&converter_state.channel_12v;
	
	Converter_TurnOff();
	while(Converter_IsReady() == 0);
	SetFeedbackChannel(converter_state.channel->CHANNEL);
	SetCurrentRange(converter_state.channel->current->RANGE);
	SetVoltageDAC(converter_state.channel->voltage.setting);
	SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
	SetOutputLoad(converter_state.channel->load_state);

	// Overload protection mechanism uses data from converter_regulation, so no
	// 	special settings are required.
}


//---------------------------------------------//
//	Main converter task
//	
//---------------------------------------------//
void vTaskConverter(void *pvParameters) 
{
	converter_message_t msg;
	
	uint8_t err_code;
	uint8_t res;
	
	
	// Initialize
	xQueueConverter = xQueueCreate( 5, sizeof( converter_message_t ) );		// Queue can contain 5 elements of type converter_message_t
	if( xQueueConverter == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	// Wait until task is started by dispatcher		
	vTaskSuspend(0);			
	
	xQueueReset(xQueueConverter);
	while(1)
	{

		xQueueReceive(xQueueConverter, &msg, portMAX_DELAY);


		switch(msg.type)
		{
			//=========================================================================//
			// Setting voltage
			case CONVERTER_SET_VOLTAGE:
				if (Converter_ValidateChannel(&msg.voltage_setting.channel))
				{
					res = Converter_SetVoltage(msg.voltage_setting.channel, msg.voltage_setting.value, &err_code);
					// Apply new setting to hardware
					if (res & VOLTAGE_SETTING_APPLIED)
					{
						// CHECKME: charge state
						if (msg.voltage_setting.channel == converter_state.channel->CHANNEL)
						{
							SetVoltageDAC(converter_state.channel->voltage.setting);
						}
					}
					// Send notification to GUI
					gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
					gui_msg.converter_event.spec = VOLTAGE_SETTING_CHANGED;
					gui_msg.converter_event.channel = msg.voltage_setting.channel;
					xQueueSendToBack(xQueueGUI, &gui_msg, 0);

					// Sound feedback
					SoundEvent_OnSettingChanged(err_code);
				}
				else
				{
					// Wrong argument (channel)
				}
				break;
			//=========================================================================//
			// Setting voltage limit
			case CONVERTER_SET_VOLTAGE_LIMIT:
				if (Converter_ValidateChannel(&msg.voltage_limit_setting.channel) && Converter_ValidateLimit(msg.voltage_limit_setting.type))
				{
					res = Converter_SetVoltageLimit(msg.voltage_limit_setting.channel, msg.voltage_limit_setting.type, 
										msg.voltage_limit_setting.value, msg.voltage_limit_setting.enable, &err_code);
					// Apply new setting to hardware
					if (res & VOLTAGE_SETTING_APPLIED)
					{
						// CHECKME: charge state
						if (msg.voltage_limit_setting.channel == converter_state.channel->CHANNEL)
						{
							SetVoltageDAC(converter_state.channel->voltage.setting);
						}
					}
					// Send notification to GUI
					gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
					gui_msg.converter_event.spec = VOLTAGE_LIMIT_CHANGED | VOLTAGE_SETTING_CHANGED;
					gui_msg.converter_event.channel = msg.voltage_limit_setting.channel;
					gui_msg.converter_event.type = msg.voltage_limit_setting.type;
					xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					
					// Sound feedback
					SoundEvent_OnSettingChanged(err_code);
				}
				else
				{
					// Wrong argument (channel or limit type)
				}
				break;
			//=========================================================================//
			// Setting current
			case CONVERTER_SET_CURRENT:
				if (Converter_ValidateChannel(&msg.current_setting.channel) && Converter_ValidateCurrentRange(&msg.current_setting.range))
				{
					res = Converter_SetCurrent(msg.current_setting.channel, msg.current_setting.range, msg.current_setting.value, &err_code);
					// Apply new setting to hardware
					if (res & CURRENT_SETTING_APPLIED)
					{
						// CHECKME: charge state
						if ((msg.current_setting.channel == converter_state.channel->CHANNEL) && 
							(msg.current_setting.range == converter_state.channel->current->RANGE))
						{
							SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
						}
					}
					// Send notification to GUI
					gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
					gui_msg.converter_event.spec = CURRENT_SETTING_CHANGED;
					gui_msg.converter_event.channel = msg.current_setting.channel;
					gui_msg.converter_event.current_range = msg.current_setting.range;
					xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				
					// Sound feedback
					SoundEvent_OnSettingChanged(err_code);
				}
				else
				{
					// Wrong argument (channel or range)
				}
				break;
			//=========================================================================//
			// Setting current limit
			case CONVERTER_SET_CURRENT_LIMIT:
				if (Converter_ValidateChannel(&msg.current_limit_setting.channel) && Converter_ValidateCurrentRange(&msg.current_limit_setting.range) &&
					Converter_ValidateLimit(msg.current_limit_setting.type))
				{
					res = Converter_SetCurrentLimit(msg.current_limit_setting.channel, msg.current_limit_setting.range, msg.current_limit_setting.type, 
										msg.current_limit_setting.value, msg.current_limit_setting.enable, &err_code);
					if (res & CURRENT_SETTING_APPLIED)
					{
						// Apply new setting to hardware
						// CHECKME: charge state
						if ((msg.current_limit_setting.channel == converter_state.channel->CHANNEL) && 
							(msg.current_setting.range == converter_state.channel->current->RANGE))
						{
							SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
						}
					}
					// Send notification to GUI
					gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
					gui_msg.converter_event.spec = CURRENT_LIMIT_CHANGED | CURRENT_SETTING_CHANGED;
					gui_msg.converter_event.channel = msg.current_limit_setting.channel;
					gui_msg.converter_event.type = msg.current_limit_setting.type;
					gui_msg.converter_event.current_range = msg.current_limit_setting.range;
					xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					
					// Sound feedback
					SoundEvent_OnSettingChanged(err_code);
				}
				{
					// Wrong argument (channel or range or limit type)
				}
				break;
			//=========================================================================//
			// Setting channel
			case CONVERTER_SWITCH_CHANNEL:
				// Check arguments
				if (! Converter_ValidateChannel(&msg.channel_setting.new_channel) )
				{
					break;		// Wrong argument (channel)
				}
				if (msg.channel_setting.new_channel == converter_state.channel->CHANNEL)
				{
					SoundEvent_OnGenericError();
					break;		// Trying to set channel that is already selected.
				}
				// Disable converter if enabled
				if (converter_state.state == CONVERTER_STATE_ON)	// TODO: analyze charge state here
				{
					Converter_TurnOff();
					converter_state.state = CONVERTER_STATE_OFF;
				}
				
				// Wait until voltage feedback switching is allowed
				while(Converter_IsReady() == 0) 
					vTaskDelay(1);
				res = Converter_SetFeedBackChannel(msg.channel_setting.new_channel);
				if (res != CONVERTER_CMD_OK)
				{
					// Unexpected error
					Converter_TurnOff();
					SoundEvent_OnGenericError();
					break;
				}
				converter_state.channel = (msg.channel_setting.new_channel == CHANNEL_5V) ? &converter_state.channel_5v : 
											&converter_state.channel_12v;
				while(Converter_IsReady() == 0) 
					vTaskDelay(1);	
				res = Converter_SetCurrentRange(converter_state.channel->current->RANGE);
				if (res != CONVERTER_CMD_OK)
				{
					// Unexpected error
					Converter_TurnOff();
					SoundEvent_OnGenericError();
					break;
				} 
				//SetCurrentRange(converter_state.channel->current->RANGE);
				SetVoltageDAC(converter_state.channel->voltage.setting);
				SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
				SetOutputLoad(converter_state.channel->load_state);

				// Send notification to GUI
				gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
				gui_msg.converter_event.spec = CHANNEL_CHANGED;
				gui_msg.converter_event.channel = msg.channel_setting.new_channel;
				gui_msg.converter_event.current_range = converter_state.channel->current->RANGE;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				// No sound feeback (a relay is ticking)

				break;
			//=========================================================================//
			// Setting current range
			case CONVERTER_SET_CURRENT_RANGE:
				if (converter_state.state == CONVERTER_STATE_ON)	// TODO: analyze charge state here
				{
					SoundEvent_OnGenericError();
					break;			// Ignore this when converter is enabled
				}
				if (! Converter_ValidateChannel(&msg.current_range_setting.channel) )
				{
					break;			// Wrong argument (channel)
				}	
				if (! Converter_ValidateCurrentRange(&msg.current_range_setting.new_range) )
				{
					break;			// Wrong argument (new_range)
				}
				if ( (msg.current_range_setting.new_range == converter_state.channel->current->RANGE) && 
					 (msg.current_range_setting.channel == converter_state.channel->CHANNEL) )
				{
					SoundEvent_OnSettingChanged(VALUE_BOUND_BY_ABS_MAX);
					break;			// Trying to set current range that is already selected.
				}
				
				// Wait until current feedback switching is allowed
				while(Converter_IsReady() == 0) 
					vTaskDelay(1);	
				
				res = Converter_SetCurrentRange(msg.current_range_setting.new_range);
				if (res != CONVERTER_CMD_OK)
				{
					// Unexpected error
					Converter_TurnOff();
					SoundEvent_OnGenericError();
					break;
				}
				
				converter_state.channel->current = (msg.current_range_setting.new_range == CURRENT_RANGE_LOW) ? &converter_state.channel->current_low_range : 
														&converter_state.channel->current_high_range;
				SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
		
				// Send notification to GUI
				gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
				gui_msg.converter_event.spec = CURRENT_RANGE_CHANGED;
				gui_msg.converter_event.channel = msg.current_range_setting.channel;
				gui_msg.converter_event.current_range = msg.current_range_setting.new_range;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				// Sound feedback
				//SoundEvent_OnSettingChanged(VALUE_OK);
				break;
			//=========================================================================//
			// Setting overload protection parameters
			case CONVERTER_SET_OVERLOAD_PARAMS:
				
				Converter_SetOverloadProtection( msg.overload_setting.protection_enable, msg.overload_setting.warning_enable, 
													msg.overload_setting.threshold,  &err_code);
				// Send notification to GUI
				gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
				gui_msg.converter_event.spec = OVERLOAD_SETTING_CHANGED;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);					
				
				// Sound feedback
				SoundEvent_OnSettingChanged(err_code);
			
				break;
			//=========================================================================//
			// Turn ON
			case CONVERTER_TURN_ON:
				if ((converter_state.state == CONVERTER_STATE_OFF) || (converter_state.state == CONVERTER_STATE_OVERLOADED))
				{
					res = Converter_TurnOn();
					if (res == CONVERTER_CMD_OK)
					{
						converter_state.state = CONVERTER_STATE_ON;
					}
					else
					{
						SoundEvent_OnGenericError();
					}
				}
				break;
			//=========================================================================//
			// Turn OFF
			case CONVERTER_TURN_OFF:
				if (converter_state.state == CONVERTER_STATE_ON)
				{
					Converter_TurnOff();
				}
				converter_state.state = CONVERTER_STATE_OFF;	// Reset overloaded state
				break;
			//=========================================================================//
			// Overload
			case CONVERTER_OVERLOADED:
				// Converter has been overloaded and disabled by low-level interrupt-driven routine.
				// Reset that routine FSM flag
				if (Converter_ClearOverloadFlag() == CONVERTER_CMD_OK)
				{
					// OK, flag has been reset.
					SoundEvent_OnOverload();
					converter_state.state = CONVERTER_STATE_OVERLOADED;
				}	
				else
				{
					// Error, we should never get here.
					SoundEvent_OnGenericError();
					// Kind of restore attempt
					Converter_TurnOff();
					converter_state.state = CONVERTER_STATE_OFF;
				}
				break;
			//=========================================================================//
			// Tick
			case CONVERTER_TICK:		// Temporary! Will be special for charging mode
				// ADC task is responsible for sampling and filtering voltage and current
				adc_msg = ADC_GET_ALL_NORMAL;
				xQueueSendToBack(xQueueADC, &adc_msg, 0);
				break;
				
			default:;
		}
	}

}






