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
#include "converter_hw.h"
#include "sound_driver.h"


#include "guiTop.h"



static gui_msg_t gui_msg;










const conveter_message_t converter_tick_message = 	{	CONVERTER_TICK, 0	};
const conveter_message_t converter_update_message = {	CONVERTER_UPDATE, 0 };
//const conveter_message_t converter_init_message = {	CONVERTER_INITIALIZE, 0, 0 };


xQueueHandle xQueueConverter;


uint32_t conv_state = CONV_OFF;








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
	Converter_SetChannel(default_channel, 1);	// will also apply other settings
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


uint8_t taskConverter_Enable = 0;



//---------------------------------------------//
//	Main converter task
//	
//---------------------------------------------//
void vTaskConverter(void *pvParameters) 
{
	conveter_message_t msg;
	
	uint8_t err_code;
	uint8_t res;
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
				res = Converter_SetVoltage(msg.voltage_setting.channel, msg.voltage_setting.value, &err_code);
				// Check result
				if (res != ERROR_ARGS)
				{
					// Send notification to GUI
					if (res & VOLTAGE_SETTING_APPLIED)
					{
						gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
						gui_msg.converter_event.spec = VOLTAGE_SETTING_CHANGED;
						gui_msg.converter_event.channel = GetActualChannel(msg.voltage_setting.channel);
						xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					}
					// Sound response
					if (msg.sender != TID_UART)
						SoundEvent_OnSettingChanged(err_code);
				}	
				else
				{
					SoundEvent_OnGenericError());
				}
				msg.type = 0;
				break;
			case CONVERTER_SET_VOLTAGE_LIMIT:
				res = Converter_SetVoltageLimit(msg.voltage_limit_setting.channel, msg.voltage_limit_setting.type, 
									msg.voltage_limit_setting.value, msg.voltage_limit_setting.enable, &err_code);
				// Check result
				if (res != ERROR_ARGS)
				{
					// Send notification to GUI
					if (res & VOLTAGE_LIMIT_APPLIED)
					{
						gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
						gui_msg.converter_event.spec = VOLTAGE_LIMIT_CHANGED;
						if (res & VOLTAGE_SETTING_APPLIED)
							gui_msg.converter_event.spec |= VOLTAGE_SETTING_CHANGED;
						gui_msg.converter_event.channel = = GetActualChannel(msg.voltage_limit_setting.channel);
						gui_msg.converter_event.type = msg.voltage_limit_setting.type;
						xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					}
					// Sound response
					if (msg.sender != TID_UART)
						SoundEvent_OnSettingChanged(err_code);
				}	
				else
				{
					SoundEvent_OnGenericError());
				}					
				msg.type = 0;
				break;
			case CONVERTER_SET_CURRENT:
				res = Converter_SetCurrent(msg.current_setting.channel, msg.current_setting.range, msg.current_setting.value, &err_code);
				// Check result
				if (res != ERROR_ARGS)
				{
					// Send notification to GUI
					if (res & CURRENT_SETTING_APPLIED)
					{
						gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
						gui_msg.converter_event.spec = CURRENT_SETTING_CHANGED;
						gui_msg.converter_event.channel = GetActualChannel(msg.current_setting.channel);
						gui_msg.converter_event.current_range = GetActualCurrentRange(msg.current_setting.range);
						xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					}
					// Sound response
					if (msg.sender != TID_UART)
						SoundEvent_OnSettingChanged(err_code);
				}	
				else
				{
					SoundEvent_OnGenericError());
				}					
				msg.type = 0;
				break;
			case CONVERTER_SET_CURRENT_LIMIT:
				res = Converter_SetCurrentLimit(msg.current_limit_setting.channel, msg.current_limit_setting.range, msg.current_limit_setting.type, 
									msg.current_limit_setting.value, msg.current_limit_setting.enable, &err_code);
				// Check result
				if (res != ERROR_ARGS)
				{
					// Send notification to GUI
					if (res & CURRENT_LIMIT_APPLIED)
					{
						gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
						gui_msg.converter_event.spec = CURRENT_LIMIT_CHANGED;
						if (res & CURRENT_SETTING_APPLIED)
							gui_msg.converter_event.spec |= CURRENT_SETTING_CHANGED;
						gui_msg.converter_event.channel = GetActualChannel(msg.current_limit_setting.channel);
						gui_msg.converter_event.type = msg.current_limit_setting.type;
						gui_msg.converter_event.current_range = GetActualCurrentRange(msg.current_limit_setting.range);
						xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					}
					// Sound response
					if (msg.sender != TID_UART)
						SoundEvent_OnSettingChanged(err_code);
				}	
				else
				{
					SoundEvent_OnGenericError());
				}					
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
						while(Converter_IsReady() == 0);
						res = Converter_SetChannel(msg.channel_setting.new_channel);
						if (res == CONVERTER_CMD_OK)
						{
							// Send notification to GUI
							gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
							gui_msg.converter_event.spec = CHANNEL_CHANGED;
							gui_msg.converter_event.channel = msg.channel_setting.new_channel;
							gui_msg.converter_event.current_range = regulation_setting_p->current->RANGE;
							xQueueSendToBack(xQueueGUI, &gui_msg, 0);
						}
						else
						{
							SoundEvent_OnGenericError());
						}
						msg.type = 0;
						break;
					}
					if (msg.type == CONVERTER_SET_CURRENT_RANGE)
					{
						res = Converter_SetCurrentRange(msg.current_range_setting.channel, msg.current_range_setting.new_range);
						if (res == CONVERTER_CMD_OK)
						{
							// Send notification to GUI
							gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
							gui_msg.converter_event.spec = CURRENT_RANGE_CHANGED;
							gui_msg.converter_event.channel = msg.current_range_setting.channel;
							gui_msg.converter_event.current_range = msg.current_range_setting.new_range;
							xQueueSendToBack(xQueueGUI, &gui_msg, 0);
						}
						else
						{
							SoundEvent_OnGenericError());
						}
						msg.type = 0;
						break;
					}

					if (msg.type == CONVERTER_TURN_OFF)
					{
						conv_state = CONV_OFF;			// Reset overload flag
						msg.type = 0;
						break;
					}
					if (msg.type == CONVERTER_TURN_ON)
					{
						res = Converter_TurnOn();
						if (res == CONVERTER_CMD_OK)
						{
							conv_state = CONV_ON;
						}
						else
						{
							SoundEvent_OnGenericError());
						}
						msg.type = 0;
						break;
					}
					msg.type = 0;
					break;
				case CONV_ON:
					if (msg.type == CONVERTER_TURN_OFF)
					{
						Converter_TurnOff();
						conv_state = CONV_OFF;
						msg.type = 0;
						break;
					}
					if (msg.type == CONVERTER_SWITCH_CHANNEL)
					{
						Converter_TurnOff();
						conv_state = CONV_OFF;
						vTaskDelay(4);
						break;
					}
					
					if (msg.type == CONVERTER_SET_CURRENT_RANGE)
					{
						// CONVERTER_SET_CURRENT_RANGE is ignored when converter is ON
					}
					/*
					if (state_HWProcess & STATE_HW_OFF)
					{
						// Some hardware error has happened and converter had been switched off
						conv_state = CONV_OFF;
						conv_state |= analyzeAndResetHWErrorState();
						if (conv_state & CONV_OVERLOAD)
						{
							// Send message to sound driver
							// TODO: send this message in other cases (when overload is detected simultaneously with button)
							SoundEvent_OnOverload();
						}
						break;
					} */
					msg.type = 0;
					break;
			}
			//------------------------------------//
			//------------------------------------//
		}

	}	
}


