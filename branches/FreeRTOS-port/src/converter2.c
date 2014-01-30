

#include "MDR32Fx.h" 

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#include "sound_driver.h"
#include "control.h"
#include "converter2.h"
#include "converter2_hw.h"

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
#define ERROR_ARGS					0xFF
// these two can be set simultaneously
#define VOLTAGE_SETTING_APPLIED		0x01	
#define VOLTAGE_LIMIT_APPLIED		0x02
// these two can be set simultaneously
#define CURRENT_SETTING_APPLIED		0x01	
#define CURRENT_LIMIT_APPLIED		0x02

#define OVERLOAD_SETTINGS_APPLIED	0x03
#define CURRENT_RANGE_APPLIED		0x04
#define CHANNEL_SETTING_APPLIED		0x05

#define CONVERTER_CMD_OK			0x10
#define CONVERTER_IS_NOT_READY		0x11
#define CONVERTER_IS_OVERLOADED		0x12



xQueueHandle xQueueConverter;
uint8_t taskConverter_Enable = 0;

converter_state_t converter_state;		// main converter control


gui_msg_t gui_msg;


//---------------------------------------------------------------------------//
//	Checks and sets new value in regulation structure
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
// 	Set converter output voltage
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
		result = VOLTAGE_SETTING_APPLIED;
	}
	return result;
}


//---------------------------------------------------------------------------//
// 	Set converter output current
//	input:
//		channel - converter channel to affect: CHANNEL_5V / CHANNEL_12V / OPERATING_CHANNEL
//		current_range - 20A range or 40A range to modify. Can be OPERATING_CURRENT_RANGE.
//		new_value - new value to check and set
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		CURRENT_SETTING_APPLIED 	- if new setting has been set
//---------------------------------------------------------------------------//
uint8_t Converter_SetCurrent(uint8_t channel, uint8_t current_range, int32_t new_value, uint8_t *err_code)
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









uint8_t Converter_ValidateChannel(uint8_t *channel)
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

uint8_t Converter_ValidateCurrentRange(uint8_t *range)
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
	sound_msg = SND_CONV_OVERLOADED | SND_CONVERTER_PRIORITY_HIGHEST;	// FIXME
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
	converter_state.channel_5v.overload_protection_enable = 1;									// TODO: EEPROM
	converter_state.channel_5v.overload_timeout = (5*1);										// TODO: EEPROM
	
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
	converter_state.channel_12v.overload_protection_enable = 1;									// TODO: EEPROM
	converter_state.channel_12v.overload_timeout = (5*1);										// TODO: EEPROM
	
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
	
	
	// Select default channel
	//Converter_SetChannel(default_channel, 1);	// will also apply other settings
	
	converter_state.channel = (default_channel == CHANNEL_5V) ? &converter_state.channel_5v : 
								&converter_state.channel_12v;
	
	// Apply controls
	__disable_irq();
	SetFeedbackChannel(converter_state.channel->CHANNEL);		// PORTF can be accessed from ISR
	__enable_irq();
	SetCurrentRange(converter_state.channel->current->RANGE);
	SetOutputLoad(converter_state.channel->load_state);
	SetVoltageDAC(converter_state.channel->voltage.setting);
	SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
	
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
	//vTaskSuspend(vTaskConverter);			
	
	while (taskConverter_Enable == 0)
		vTaskDelay(5);
	
	xQueueReset(xQueueConverter);
	
	

	xQueueReceive(xQueueConverter, &msg, portMAX_DELAY);

	
	// Other messages get processing depending on converter state

	while(msg.type)
	{
		switch(msg.type)
		{
			case CONVERTER_SET_VOLTAGE:
				if (Converter_ValidateChannel(&msg.voltage_setting.channel))
				{
					res = Converter_SetVoltage(msg.voltage_setting.channel, msg.voltage_setting.value, &err_code);
					if (res & VOLTAGE_SETTING_APPLIED)
					{
						// Apply new setting to hardware
						// CHECKME: charge state
						if (msg.voltage_setting.channel == converter_state.channel->CHANNEL)
						{
							SetVoltageDAC(converter_state.channel->voltage.setting);
						}
						// Send notification to GUI
						gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
						gui_msg.converter_event.spec = VOLTAGE_SETTING_CHANGED;
						gui_msg.converter_event.channel = msg.voltage_setting.channel;
						xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					}
					// Sound response
					if (msg.sender != TID_UART)
						SoundEvent_OnSettingChanged(err_code);
				}
				else
				{
					// Wrong argument (channel)
				}
				msg.type = 0;
				break;
			case CONVERTER_SET_CURRENT:
				if (Converter_ValidateChannel(&msg.current_setting.channel) && Converter_ValidateCurrentRange(&msg.current_setting.range))
				{
					res = Converter_SetCurrent(msg.current_setting.channel, msg.current_setting.range, msg.current_setting.value, &err_code);
					if (res & CURRENT_SETTING_APPLIED)
					{
						// Apply new setting to hardware
						// CHECKME: charge state
						if ((msg.current_setting.channel == converter_state.channel->CHANNEL) && 
							(msg.current_setting.range == converter_state.channel->current->RANGE))
						{
							SetCurrentDAC(converter_state.channel->current->setting, converter_state.channel->current->RANGE);
						}
						// Send notification to GUI
						gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
						gui_msg.converter_event.spec = CURRENT_SETTING_CHANGED;
						gui_msg.converter_event.channel = msg.current_setting.channel;
						gui_msg.converter_event.current_range = msg.current_setting.range;
						xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					}
					// Sound response
					if (msg.sender != TID_UART)
						SoundEvent_OnSettingChanged(err_code);
				}
				else
				{
					// Wrong argument (channel or range)
				}
				msg.type = 0;
				break;
			case CONVERTER_TURN_ON:
				if (converter_state.state == CONVERTER_STATE_OFF)
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
				msg.type = 0;
				break;
			case CONVERTER_TURN_OFF:
				if (converter_state.state == CONVERTER_STATE_ON)
				{
					Converter_TurnOff();
					converter_state.state = CONVERTER_STATE_OFF;
				}
				msg.type = 0;
				break;
				
			default:
				msg.type = 0;
		}
		
		
		
		
	}
}






