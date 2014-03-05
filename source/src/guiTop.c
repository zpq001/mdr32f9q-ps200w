#include <stdio.h>

#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "stdio.h"

#include "lcd_1202.h"		// physical driver

//------ GUI components ------//
#include "guiConfig.h"
#include "guiTop.h"
#include "guiFonts.h"
#include "guiGraphHAL.h"
#include "guiGraphPrimitives.h"
#include "guiGraphWidgets.h"
#include "guiCore.h"
#include "guiEvents.h"
#include "guiWidgets.h"
#include "guiTextLabel.h"
#include "guiMainForm.h"
#include "guiMasterPanel.h"
#include "guiSetupPanel.h"
//----------------------------//

#include "converter.h"	// voltage, current, etc
//#include "converter_hw.h"
#include "adc.h"

#include "dispatcher.h"

#include "service.h"	// temperature
#include "control.h"	// some defines

#include "buttons.h"
#include "encoder.h"


xQueueHandle xQueueGUI;

static dispatch_msg_t dispatcher_msg;


// TODO: move to some .h file common definitions - such as channel, curr. range, etc


// Encode physical buttons into GUI virtual keys
static uint8_t encodeGuiKey(uint16_t btnCode)
{
	switch (btnCode)
	{
		case BTN_ESC:
			return GUI_KEY_ESC;
		case BTN_OK:
			return GUI_KEY_OK;
		case BTN_LEFT:
			return GUI_KEY_LEFT;
		case BTN_RIGHT:
			return GUI_KEY_RIGHT;
		case BTN_ENCODER:
			return GUI_KEY_ENCODER;
		default: 
			return 0;
	}
}

// Encode physical buttons events into GUI virtual events
static uint8_t encodeGuiKeyEvent(uint16_t btnEvent)
{
	return (uint8_t)btnEvent;
}





//---------------------------------------------//
// 

uint16_t getVoltageSetting(uint8_t channel)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return c->voltage.setting;
}

uint16_t getVoltageAbsMax(uint8_t channel)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return c->voltage.MAXIMUM;
}

uint16_t getVoltageAbsMin(uint8_t channel)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return c->voltage.MINIMUM;
}

uint16_t getVoltageLimitSetting(uint8_t channel, uint8_t limit_type)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return (limit_type == GUI_LIMIT_TYPE_LOW) ? c->voltage.limit_low : c->voltage.limit_high;
}

uint8_t getVoltageLimitState(uint8_t channel, uint8_t limit_type)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return (limit_type == GUI_LIMIT_TYPE_LOW) ? c->voltage.enable_low_limit : c->voltage.enable_high_limit;
}

uint16_t getCurrentSetting(uint8_t channel, uint8_t range)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
    return s->setting;
}

uint16_t getCurrentAbsMax(uint8_t channel, uint8_t range)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
    return s->MAXIMUM;
}

uint16_t getCurrentAbsMin(uint8_t channel, uint8_t range)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
    return s->MINIMUM;
}

uint16_t getCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
	return (limit_type == GUI_LIMIT_TYPE_LOW) ? s->limit_low : s->limit_high;
}

uint8_t getCurrentLimitState(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *s = (range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
	return (limit_type == GUI_LIMIT_TYPE_LOW) ? s->enable_low_limit : s->enable_high_limit;
}

uint8_t getOverloadProtectionState(void)
{
	return converter_state.overload_protection_enable;
}

uint8_t getOverloadProtectionWarning(void)
{
    return converter_state.overload_warning_enable;
}

uint16_t getOverloadProtectionThreshold(void)
{
    return converter_state.overload_threshold;
}

uint8_t getCurrentRange(uint8_t channel)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
    return c->current->RANGE;
}

uint8_t getFeedbackChannel(void)
{
	return converter_state.channel->CHANNEL;
}


//---------------------------------------------//






// TODO: set proper min and max for voltage and current spinboxes


void vTaskGUI(void *pvParameters) 
{
	gui_msg_t msg;
	uint8_t guiKeyCode;
	uint8_t guiKeyEvent;
//	int16_t encoder_delta;
	guiEvent_t guiEvent;
	int32_t value;
	uint8_t state;
	uint8_t state2;
	
	// Initialize
	xQueueGUI = xQueueCreate( 10, sizeof( gui_msg_t ) );		// GUI queue can contain 10 elements of type gui_incoming_msg_t
	if( xQueueGUI == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	// Create all GUI elements and prepare core
	guiMainForm_Initialize();
    guiCore_Init((guiGenericWidget_t *)&guiMainForm);
	// GUI is ready, but initial welcome screen is what will be displayed.
	// When all other systems init will be done, GUI should be sent 
	// a special message to start operate normally.
	
	
	while(1)
	{
		xQueueReceive(xQueueGUI, &msg, portMAX_DELAY);
		switch (msg.type)
		{
			case GUI_TASK_RESTORE_ALL:
				value = getFeedbackChannel();
				setGuiFeedbackChannel(value);
				// Retore other values from EEPROM
				// TODO
				
				// Start normal GUI operation
				guiEvent.type = GUI_EVENT_START;
				guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
				guiCore_ProcessMessageQueue();
				break;
			case GUI_TASK_REDRAW:
				guiCore_RedrawAll();				// Draw GUI
				LcdUpdateBothByCore(lcdBuffer);		// Flush buffer to LCDs
				break;
			case GUI_TASK_EEPROM_STATE:
				guiEvent.type = GUI_EVENT_EEPROM_MESSAGE;
				guiEvent.spec = (uint8_t)msg.data.a;
				guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
				guiCore_ProcessMessageQueue();
				break;
			case GUI_TASK_PROCESS_BUTTONS:
				//guiKeyCode = encodeGuiKey(msg.key_event.code);
				//guiKeyEvent = encodeGuiKeyEvent(msg.key_event.event);
				//if ((guiKeyEvent != 0) && (guiKeyCode != 0))
				//	guiCore_ProcessKeyEvent(guiKeyCode, guiKeyEvent);
				guiCore_ProcessKeyEvent(msg.key_event.code, msg.key_event.event);
				break;
			case GUI_TASK_PROCESS_ENCODER:
				if (msg.encoder_event.delta)
					guiCore_ProcessEncoderEvent(msg.encoder_event.delta);
				break;
			case GUI_TASK_UPDATE_CONVERTER_STATE:
				switch (msg.converter_event.spec)
				{
					case VOLTAGE_SETTING_CHANGED:
						value = getVoltageSetting(msg.converter_event.channel);
						setGuiVoltageSetting(msg.converter_event.channel, value);
						break;
					case CURRENT_SETTING_CHANGED:
						value = getCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range);
						setGuiCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range, value);
						break;
					case VOLTAGE_LIMIT_CHANGED:
						value = getVoltageLimitSetting(msg.converter_event.channel, msg.converter_event.type);
						state = getVoltageLimitState(msg.converter_event.channel, msg.converter_event.type);
						setGuiVoltageLimitSetting(msg.converter_event.channel, msg.converter_event.type, state, value);
						// Update voltage setting too
						value = getVoltageSetting(msg.converter_event.channel);
						setGuiVoltageSetting(msg.converter_event.channel, value);
						break;
					case CURRENT_LIMIT_CHANGED:
						value = getCurrentLimitSetting(msg.converter_event.channel, msg.converter_event.current_range, msg.converter_event.type);
						state = getCurrentLimitState(msg.converter_event.channel, msg.converter_event.current_range, msg.converter_event.type);
						setGuiCurrentLimitSetting(msg.converter_event.channel, msg.converter_event.current_range, msg.converter_event.type, state, value);	
						// Update current setting too
						value = getCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range);
						setGuiCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range, value);
						break;
					case CURRENT_RANGE_CHANGED:
						value = getCurrentRange(msg.converter_event.channel);
						setGuiCurrentRange(msg.converter_event.channel, value);
						break;
					case CHANNEL_CHANGED:
						value = getFeedbackChannel();
						setGuiFeedbackChannel(value);
						break;
					case OVERLOAD_SETTING_CHANGED:
						state = getOverloadProtectionState();
						state2 = getOverloadProtectionWarning();
						value = getOverloadProtectionThreshold();
						setGuiOverloadSetting(state, state2, value);
						break;
				}
				break;
			case GUI_TASK_UPDATE_VOLTAGE_CURRENT:
				setGuiVoltageIndicator(voltage_adc);
				setGuiCurrentIndicator(current_adc);
				setGuiPowerIndicator(power_adc);
				break;
			case GUI_TASK_UPDATE_TEMPERATURE_INDICATOR:
				setGuiTemperatureIndicator(converter_temp_celsius);
				break;
		}
	}
}


//=================================================================//
//=================================================================//
//                   Hardware control interface                    //
//						GUI -> converter						   //
//=================================================================//


//-------------------------------------------------------//
//	Voltage
void applyGuiVoltageSetting(uint8_t channel, int32_t new_set_voltage)
{
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_VOLTAGE;
	dispatcher_msg.converter_cmd.a.v_set.channel = channel;	
	dispatcher_msg.converter_cmd.a.v_set.value = new_set_voltage;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
}

void applyGuiVoltageLimit(uint8_t channel, uint8_t type, uint8_t enable, int32_t value)
{ 
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_VOLTAGE_LIMIT;
	dispatcher_msg.converter_cmd.a.vlim_set.channel = channel;
	dispatcher_msg.converter_cmd.a.vlim_set.type = type;	
	dispatcher_msg.converter_cmd.a.vlim_set.enable = enable;
	dispatcher_msg.converter_cmd.a.vlim_set.value = value;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
}

//-------------------------------------------------------//
//	Current
void applyGuiCurrentSetting(uint8_t channel, uint8_t range, int32_t new_set_current)
{ 
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_CURRENT;
	dispatcher_msg.converter_cmd.a.c_set.channel = channel;	
	dispatcher_msg.converter_cmd.a.c_set.range = range;	
	dispatcher_msg.converter_cmd.a.c_set.value = new_set_current;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
}

void applyGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t type, uint8_t enable, int32_t value)
{ 
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_CURRENT_LIMIT;
	dispatcher_msg.converter_cmd.a.clim_set.channel = channel;
	dispatcher_msg.converter_cmd.a.clim_set.range = currentRange;
	dispatcher_msg.converter_cmd.a.clim_set.type = type;	
	dispatcher_msg.converter_cmd.a.clim_set.enable = enable;
	dispatcher_msg.converter_cmd.a.clim_set.value = value;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
}

void applyGuiCurrentRange(uint8_t channel, uint8_t new_range)
{
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_CURRENT_RANGE;
	dispatcher_msg.converter_cmd.a.crange_set.channel = channel;	
	dispatcher_msg.converter_cmd.a.crange_set.new_range = new_range;	
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
}

//-------------------------------------------------------//
//	Other

void applyGuiOverloadSetting(uint8_t protection_enable, uint8_t warning_enable, int32_t threshold)
{ 
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_CURRENT_RANGE;
	dispatcher_msg.converter_cmd.a.overload_set.protection_enable = protection_enable;
	dispatcher_msg.converter_cmd.a.overload_set.warning_enable = warning_enable;
	dispatcher_msg.converter_cmd.a.overload_set.threshold = threshold;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);		
}






