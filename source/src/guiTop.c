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

#include "service.h"	// temperature
#include "control.h"	// some defines

#include "buttons.h"
#include "encoder.h"


xQueueHandle xQueueGUI;

static converter_message_t converter_msg;	// to save stack


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



static channel_state_t *c;
static uint8_t converter_current_range;



//-------------------------------------------------------//
//	Voltage

static void updateGuiVoltageSetting(uint8_t channel)
{
	if (channel == c->CHANNEL)
	{
		setVoltageSetting(c->voltage.setting);
	}
} 

void updateGuiVoltageLimit(uint8_t channel, uint8_t type)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	
		if (type == LIMIT_TYPE_LOW)
			setLowVoltageLimitSetting(channel, c->voltage.enable_low_limit, c->voltage.limit_low);
		else
			setHighVoltageLimitSetting(channel, c->voltage.enable_high_limit, c->voltage.limit_high);
} 

//-------------------------------------------------------//
//	Current
static void updateGuiCurrentSetting(uint8_t channel, uint8_t current_range)
{
	if ((channel == c->CHANNEL) && (converter_current_range == current_range))
	{
		reg_setting_t *s = (converter_current_range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
		setCurrentSetting(s->setting);
	}
} 

void updateGuiCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t type)
{
	channel_state_t *c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	reg_setting_t *r = (current_range == CURRENT_RANGE_LOW) ? &c->current_low_range : &c->current_high_range;
	
	if (type == LIMIT_TYPE_LOW)
		setLowCurrentLimitSetting(channel, current_range, r->enable_low_limit, r->limit_low);
	else
		setHighCurrentLimitSetting(channel, current_range, r->enable_high_limit, r->limit_high);
} 

//-------------------------------------------------------//
//	Other
static void UpdateConverterCurrentRange(uint8_t channel, uint8_t current_range)
{
	if (channel == c->CHANNEL)
	{
		converter_current_range = current_range;
		setCurrentRangeIndicator(current_range);
		updateGuiCurrentSetting(channel, current_range);
	}
}

static void UpdateConverterChannel(uint8_t channel, uint8_t current_range)
{
	c = (channel == CHANNEL_5V) ? &converter_state.channel_5v : &converter_state.channel_12v;
	setFeedbackChannelIndicator( (c->CHANNEL == CHANNEL_5V) ? GUI_CHANNEL_5V : GUI_CHANNEL_12V );
	UpdateConverterCurrentRange(channel, current_range);
	setVoltageSetting(c->voltage.setting);
}




// TODO: set proper min and max for voltage and current spinboxes


void vTaskGUI(void *pvParameters) 
{
	gui_msg_t msg;
	uint8_t guiKeyCode;
	uint8_t guiKeyEvent;
//	int16_t encoder_delta;
	guiEvent_t guiEvent;
	
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
				UpdateConverterChannel(converter_state.channel->CHANNEL, converter_state.channel->current->RANGE);
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
				guiKeyCode = encodeGuiKey(msg.key_event.code);
				guiKeyEvent = encodeGuiKeyEvent(msg.key_event.event);
				if ((guiKeyEvent != 0) && (guiKeyCode != 0))
					guiCore_ProcessKeyEvent(guiKeyCode, guiKeyEvent);
				break;
			case GUI_TASK_PROCESS_ENCODER:
				if (msg.encoder_event.delta)
					guiCore_ProcessEncoderEvent(msg.encoder_event.delta);
				break;
			case GUI_TASK_UPDATE_CONVERTER_STATE:
				if (msg.converter_event.spec & VOLTAGE_SETTING_CHANGED)
					updateGuiVoltageSetting(msg.converter_event.channel);
				if (msg.converter_event.spec & CURRENT_SETTING_CHANGED)
					updateGuiCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range);
				if (msg.converter_event.spec & VOLTAGE_LIMIT_CHANGED)
					updateGuiVoltageLimit(msg.converter_event.channel, msg.converter_event.type);
				if (msg.converter_event.spec & CURRENT_LIMIT_CHANGED)
					updateGuiCurrentLimit(msg.converter_event.channel, msg.converter_event.current_range, msg.converter_event.type);	
				if (msg.converter_event.spec & CURRENT_RANGE_CHANGED)
					UpdateConverterCurrentRange(msg.converter_event.channel, msg.converter_event.current_range);
				if (msg.converter_event.spec & CHANNEL_CHANGED)
					UpdateConverterChannel(msg.converter_event.channel, msg.converter_event.current_range);
				break;
			
			case GUI_TASK_UPDATE_VOLTAGE_CURRENT:
				setVoltageIndicator(voltage_adc);
				setCurrentIndicator(current_adc);
				setPowerIndicator(power_adc);
				break;
			case GUI_TASK_UPDATE_TEMPERATURE_INDICATOR:
				setTemperatureIndicator(converter_temp_celsius);
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
void applyGuiVoltageSetting(int32_t new_set_voltage)
{
	converter_msg.type = CONVERTER_SET_VOLTAGE;
	converter_msg.voltage_setting.channel = c->CHANNEL;
	converter_msg.voltage_setting.value = new_set_voltage;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}

void applyGuiVoltageLimit(uint8_t channel, uint8_t type, uint8_t enable, int32_t value)
{
	converter_msg.type = CONVERTER_SET_VOLTAGE_LIMIT;
	converter_msg.voltage_limit_setting.channel = channel;
	converter_msg.voltage_limit_setting.type = type;	
	converter_msg.voltage_limit_setting.enable = enable;
	converter_msg.voltage_limit_setting.value = value;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}

//-------------------------------------------------------//
//	Current
void applyGuiCurrentSetting(int32_t new_set_current)
{
	converter_msg.type = CONVERTER_SET_CURRENT;
	converter_msg.current_setting.channel = c->CHANNEL;
	converter_msg.current_setting.range = converter_current_range;
	converter_msg.current_setting.value = new_set_current;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}

void applyGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t type, uint8_t enable, int32_t value)
{
	converter_msg.type = CONVERTER_SET_CURRENT_LIMIT;
	converter_msg.current_limit_setting.channel = channel;
	converter_msg.current_limit_setting.range = currentRange;
	converter_msg.current_limit_setting.type = type;	
	converter_msg.current_limit_setting.enable = enable;
	converter_msg.current_limit_setting.value = value;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0); 
}

//-------------------------------------------------------//
//	Other
void applyGuiCurrentRange(uint8_t new_range)
{
	converter_msg.type = CONVERTER_SET_CURRENT_RANGE;
	converter_msg.current_range_setting.channel = c->CHANNEL;
	converter_msg.current_range_setting.new_range = new_range;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}







