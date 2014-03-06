

#include "MDR32Fx.h" 

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#include "sound_driver.h"
#include "control.h"
#include "converter.h"
#include "converter_hw.h"
#include "adc.h"

#include "dispatcher.h"
//#include "guiTop.h"




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

//gui_msg_t gui_msg;
uint32_t adc_msg;
dispatch_msg_t dispatcher_msg;

converter_state_t converter_state;		// main converter control




//---------------------------------------------//
// 

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


//---------------------------------------------//






//---------------------------------------------------------------------------//
///	Checks and sets new value in regulation structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationValue(reg_setting_t *s, int32_t new_value)
{
	uint8_t errCode = VALUE_OK;
	// Check software limits
	if ((s->enable_low_limit) && (new_value <= (int32_t)s->limit_low))
	{	
		new_value = (int32_t)s->limit_low;
		errCode = VALUE_BOUND_BY_SOFT_MIN;
	}
	if ((s->enable_high_limit) && (new_value >= (int32_t)s->limit_high)) 
	{
		new_value = (int32_t)s->limit_high;
		errCode = VALUE_BOUND_BY_SOFT_MAX;
	} 
	// Check absolute limits
	if (new_value <= (int32_t)s->MINIMUM)
	{	
		new_value = (int32_t)s->MINIMUM;
		errCode = VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->MAXIMUM) 
	{
		new_value = (int32_t)s->MAXIMUM;
		errCode = VALUE_BOUND_BY_ABS_MAX;
	} 
	s->setting = new_value;
	return errCode;
}


//---------------------------------------------------------------------------//
///	Checks and sets new value limit in regultaion structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationLimit(reg_setting_t *s, uint8_t limit_type, int32_t new_value, uint8_t enable)
{
	uint8_t errCode = VALUE_OK;
	if (new_value <= (int32_t)s->LIMIT_MIN)
	{	
		new_value = (int32_t)s->LIMIT_MIN;
		errCode = VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->LIMIT_MAX) 
	{
		new_value = (int32_t)s->LIMIT_MAX;
		errCode = VALUE_BOUND_BY_ABS_MAX;
	} 
	if (limit_type == LIMIT_TYPE_LOW)
	{
		s->limit_low = new_value;
		s->enable_low_limit = enable;
	}
	else 
	{
		s->limit_high = new_value;
		s->enable_high_limit = enable;
	}
	return errCode;
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
	// Ensure that voltage setting lies inside new limits
	Converter_SetVoltage(channel, c->voltage.setting);
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
	// Ensure that current setting lies inside new limits
	Converter_SetCurrent(channel, current_range, s->setting);
	return err_code;
}


//---------------------------------------------------------------------------//
/// Set converter overload parameters
// new_threshold in units of 100us
// Converter HW overload threshold in units of 200us
//---------------------------------------------------------------------------//
static uint8_t Converter_SetOverloadProtection(uint8_t protectionEnable, uint8_t warningEnable, int32_t new_threshold)
{
	uint8_t err_code;
	new_threshold /= 2;
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
		err_code = VALUE_OK;
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


void fillDispatchMessage(converter_message_t *converter_msg, dispatch_msg_t *dispatcher_msg)
{
	dispatcher_msg->type = DISPATCHER_CONVERTER_EVENT;
	dispatcher_msg->sender = sender_CONVERTER;
	dispatcher_msg->converter_event.msg_type = converter_msg->type;
	dispatcher_msg->converter_event.msg_sender = converter_msg->sender;
	dispatcher_msg->converter_event.spec = 0;		// need to be filled
	dispatcher_msg->converter_event.err_code = 0;	// need to be filled
}



//---------------------------------------------//
//	Main converter task
//	
//---------------------------------------------//
void vTaskConverter(void *pvParameters) 
{
	converter_message_t msg;
	channel_state_t *c;
	
	uint8_t err_code;
	uint8_t temp8u;
	
	
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
				msg.a.v_set.channel = Converter_ValidateChannel(msg.a.v_set.channel);
				err_code = Converter_SetVoltage(msg.a.v_set.channel, msg.a.v_set.value);
				// Apply new setting to hardware. CHECKME: charge state
				if (msg.a.v_set.channel == converter_state.channel->CHANNEL)
				{
					SetVoltageDAC(converter_state.channel->voltage.setting);
				}
				// Send notification to dispatcher
				fillDispatchMessage(&msg, &dispatcher_msg);
				dispatcher_msg.converter_event.spec = VOLTAGE_SETTING_CHANGE;
				dispatcher_msg.converter_event.channel = msg.a.v_set.channel;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break;
			//=========================================================================//
			// Setting current
			case CONVERTER_SET_CURRENT:
				msg.a.c_set.channel = Converter_ValidateChannel(msg.a.c_set.channel);
				msg.a.c_set.range = Converter_ValidateCurrentRange(msg.a.c_set.range);
				err_code = Converter_SetCurrent(msg.a.c_set.channel, msg.a.c_set.range, msg.a.c_set.value);
				// Apply new setting to hardware. CHECKME: charge state
				if ((msg.a.c_set.channel == converter_state.channel->CHANNEL) && 
					(msg.a.c_set.range == converter_state.channel->current->RANGE))
				{
					SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
				}
				// Send notification to dispatcher
				fillDispatchMessage(&msg, &dispatcher_msg);
				dispatcher_msg.converter_event.spec = CURRENT_SETTING_CHANGE;
				dispatcher_msg.converter_event.channel = msg.a.c_set.channel;
				dispatcher_msg.converter_event.range = msg.a.c_set.range;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break;
			//=========================================================================//
			// Setting voltage limit
			case CONVERTER_SET_VOLTAGE_LIMIT:
				msg.a.vlim_set.channel = Converter_ValidateChannel(msg.a.vlim_set.channel);
				err_code = Converter_SetVoltageLimit(msg.a.vlim_set.channel, msg.a.vlim_set.type, 
									msg.a.vlim_set.value, msg.a.vlim_set.enable);
				// Apply new setting to hardware. CHECKME: charge state				
				if (msg.a.vlim_set.channel == converter_state.channel->CHANNEL)
				{
					SetVoltageDAC(converter_state.channel->voltage.setting);
				}
				// Send notification to dispatcher
				fillDispatchMessage(&msg, &dispatcher_msg);
				dispatcher_msg.converter_event.spec = VOLTAGE_LIMIT_CHANGE;
				dispatcher_msg.converter_event.channel = msg.a.vlim_set.channel;
				dispatcher_msg.converter_event.limit_type = msg.a.vlim_set.type;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break; 
			//=========================================================================//
			// Setting current limit
			case CONVERTER_SET_CURRENT_LIMIT:
				msg.a.clim_set.channel = Converter_ValidateChannel(msg.a.clim_set.channel);
				msg.a.clim_set.range = Converter_ValidateCurrentRange(msg.a.clim_set.range);				
				err_code = Converter_SetCurrentLimit(msg.a.clim_set.channel, msg.a.clim_set.range, msg.a.clim_set.type, 
									msg.a.clim_set.value, msg.a.clim_set.enable);
				// Apply new setting to hardware. CHECKME: charge state	
				if ((msg.a.clim_set.channel == converter_state.channel->CHANNEL) && 
					(msg.a.clim_set.range == converter_state.channel->current->RANGE))
				{
					SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
				}
				// Send notification to dispatcher
				fillDispatchMessage(&msg, &dispatcher_msg);
				dispatcher_msg.converter_event.spec = CURRENT_LIMIT_CHANGE;
				dispatcher_msg.converter_event.channel = msg.a.clim_set.channel;
				dispatcher_msg.converter_event.range = msg.a.clim_set.range;
				dispatcher_msg.converter_event.limit_type = msg.a.clim_set.type;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break; 
			//=========================================================================//
			// Setting channel
			case CONVERTER_SWITCH_CHANNEL:
				msg.a.ch_set.new_channel = Converter_ValidateChannel(msg.a.ch_set.new_channel);
				if (msg.a.ch_set.new_channel != converter_state.channel->CHANNEL)
				{
					// Disable converter if enabled
					if (converter_state.state == CONVERTER_STATE_ON)	// TODO: analyze charge state here
					{
						Converter_TurnOff();
						converter_state.state = CONVERTER_STATE_OFF;
					}
					// Wait until voltage feedback switching is allowed
					while(Converter_IsReady() == 0) 
						vTaskDelay(1);
					// Apply new channel setting to hardware
					temp8u = Converter_SetFeedBackChannel(msg.a.ch_set.new_channel);
					if (temp8u == CONVERTER_CMD_OK)
					{
						converter_state.channel = (msg.a.ch_set.new_channel == CHANNEL_5V) ? &converter_state.channel_5v : 
												&converter_state.channel_12v;
						while(Converter_IsReady() == 0) 
							vTaskDelay(1);	
						// Apply new channel current range setting to hardware
						temp8u = Converter_SetCurrentRange(converter_state.channel->current->RANGE);
						if (temp8u == CONVERTER_CMD_OK)
						{
							SetVoltageDAC(converter_state.channel->voltage.setting);
							SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
							SetOutputLoad(converter_state.channel->load_state);
							err_code = CMD_OK;
						}
						else
						{
							// Unexpected error for current range setting
							err_code = CMD_ERROR;
							Converter_TurnOff();
						} 
					}
					else
					{
						// Unexpected error for channel setting
						err_code = CMD_ERROR;
						Converter_TurnOff();
					}
				}
				else
				{
					// Trying to set channel that is already selected.
					err_code = VALUE_BOUND_BY_ABS_MAX;
				}
				
				// Send notification to dispatcher
				fillDispatchMessage(&msg, &dispatcher_msg);
				dispatcher_msg.converter_event.spec = CHANNEL_CHANGE;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break;
			//=========================================================================//
			// Setting current range
			case CONVERTER_SET_CURRENT_RANGE:
				
				msg.a.crange_set.channel = Converter_ValidateChannel(msg.a.crange_set.channel);	
				msg.a.crange_set.new_range = Converter_ValidateCurrentRange(msg.a.crange_set.new_range);
				
				c = (msg.a.crange_set.channel == CHANNEL_5V) ? &converter_state.channel_5v : 
											&converter_state.channel_12v;
								
				if (msg.a.crange_set.new_range != c->current->RANGE)
				{
					// New current range differs from current
					if (msg.a.crange_set.channel == converter_state.channel->CHANNEL)
					{
						// Setting current range of the active channel
						if (converter_state.state != CONVERTER_STATE_ON)	// TODO: analyze charge state here
						{
							// Wait until current feedback switching is allowed
							while(Converter_IsReady() == 0) 
								vTaskDelay(1);	
							// Apply new channel current range setting to hardware
							temp8u = Converter_SetCurrentRange(msg.a.crange_set.new_range);
							if (temp8u == CONVERTER_CMD_OK)
							{
								c->current = (msg.a.crange_set.new_range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
								SetCurrentDAC(c->current->setting, c->current->RANGE);
								err_code = CMD_OK;
							}
							else
							{
								// Unexpected error
								Converter_TurnOff();
								err_code = CMD_ERROR;
							}
						}
						else
						{
							// Converter in ON and current range cannot be changed
							err_code = CMD_ERROR;
						}
					}
					else
					{
						// Setting current range of the non-active channel
						c->current = (msg.a.crange_set.new_range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
						err_code = CMD_OK;
					}
				}
				else
				{
					// Trying to set current range that is already selected.
					err_code = VALUE_BOUND_BY_ABS_MAX;
				}
				
				// Send notification to dispatcher
				fillDispatchMessage(&msg, &dispatcher_msg);
				dispatcher_msg.converter_event.spec = CURRENT_RANGE_CHANGE;
				dispatcher_msg.converter_event.channel = msg.a.crange_set.channel;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break;
			//=========================================================================//
			// Setting overload protection parameters
			case CONVERTER_SET_OVERLOAD_PARAMS:
				err_code = Converter_SetOverloadProtection( msg.a.overload_set.protection_enable, msg.a.overload_set.warning_enable, 
													msg.a.overload_set.threshold);
				// Send notification to dispatcher
				fillDispatchMessage(&msg, &dispatcher_msg);
				dispatcher_msg.converter_event.spec = OVERLOAD_SETTING_CHANGE;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break; 
			//=========================================================================//
			// Turn ON
			case CONVERTER_TURN_ON:
				if ((converter_state.state == CONVERTER_STATE_OFF) || (converter_state.state == CONVERTER_STATE_OVERLOADED))
				{
					temp8u = Converter_TurnOn();
					if (temp8u == CONVERTER_CMD_OK)
					{
						converter_state.state = CONVERTER_STATE_ON;
						err_code = CMD_OK;
					}
					else
					{
						err_code = CMD_ERROR;
					}
				}
				else
				{
					// Converter is already ON
					err_code = CMD_OK;
				}
				// Send notification to dispatcher
				fillDispatchMessage(&msg, &dispatcher_msg);
				dispatcher_msg.converter_event.spec = STATE_CHANGE_TO_ON;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break;
			//=========================================================================//
			// Turn OFF
			case CONVERTER_TURN_OFF:
				Converter_TurnOff();
				converter_state.state = CONVERTER_STATE_OFF;	// Reset overloaded state
				err_code = CMD_OK;
				// Send notification to dispatcher
				fillDispatchMessage(&msg, &dispatcher_msg);
				dispatcher_msg.converter_event.spec = STATE_CHANGE_TO_OFF;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break;
			//=========================================================================//
			// Overload
			case CONVERTER_OVERLOADED:
				// Hardware has been overloaded and disabled by low-level interrupt-driven routine.
				converter_state.state = CONVERTER_STATE_OVERLOADED;
				// Reset that routine FSM flag
				if (Converter_ClearOverloadFlag() == CONVERTER_CMD_OK)
				{
					// OK, flag has been reset.
					err_code = EVENT_OK;
				}	
				else
				{
					// Error, we should never get here.
					// Kind of restore attempt
					Converter_TurnOff();
					err_code = EVENT_ERROR;							
				}
				// Send notification to dispatcher
				dispatcher_msg.type = DISPATCHER_CONVERTER_EVENT;
				dispatcher_msg.sender = sender_CONVERTER;
				dispatcher_msg.converter_event.msg_type = 0;
				dispatcher_msg.converter_event.msg_sender = 0;
				dispatcher_msg.converter_event.spec = STATE_CHANGE_TO_OVERLOAD;
				dispatcher_msg.converter_event.err_code = err_code;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				break; 
			//=========================================================================//
			// Tick
			case CONVERTER_TICK:		// Temporary! Will be special for charging mode
				// ADC task is responsible for sampling and filtering voltage and current
				adc_msg = ADC_GET_ALL_NORMAL;
				xQueueSendToBack(xQueueADC, &adc_msg, 0);
				break;
				
			default:
				break;
		}
	}

}






