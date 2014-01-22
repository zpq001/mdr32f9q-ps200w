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
#include "service.h"	// temperature
#include "control.h"	// some defines

#include "buttons.h"
#include "encoder.h"


xQueueHandle xQueueGUI;

static conveter_message_t converter_msg;	// to save stack


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


// GUI initialization
void GUI_Init(void)
{

	
	// Get values from converter - must be already initialized
	
	
	// Restore values from settings

}

static uint8_t converter_channel;
static uint8_t converter_current_range;


static void UpdateConverterChannel(uint8_t channel)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : &channel_12v;
	converter_channel = channel;
	setFeedbackChannelIndicator(channel);
	converter_current_range = r->current->RANGE;
	setCurrentLimitIndicator(range);
	setVoltageSetting(r->voltage.setting);
	setCurrentSetting(r->current->setting);
}

static void UpdateConverterCurrentRange(uint8_t range)
{
	converter_current_range = range;
	setCurrentLimitIndicator(range);
}


static void UpdateVoltageSetting(uint8_t channel)
{
	if (channel == converter_channel)
	{
		converter_regulation_t *r = (converter_channel == CHANNEL_5V) ? &channel_5v : &channel_12v;
		setVoltageSetting(r->voltage.setting);
	}
} 

static void UpdateCurrentSetting(uint8_t channel, uint8_t current_range)
{
	if ((channel == converter_channel) && (converter_current_range == current_range))
	{
		converter_regulation_t *r = (converter_channel == CHANNEL_5V) ? &channel_5v : &channel_12v;
		reg_setting_t *s = (converter_current_range == CURRENT_RANGE_LOW) ? r->current_low_range : r->current_high_range;
		setCurrentSetting(s->setting);
	}
} 






// TODO: set proper min and max for voltage and current spinboxes


void vTaskGUI(void *pvParameters) 
{
	gui_msg_t msg;
	uint8_t guiKeyCode;
	uint8_t guiKeyEvent;
	int16_t encoder_delta;
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
				setVoltageSetting(regulation_setting_p->voltage.setting);
				setCurrentSetting(regulation_setting_p->current->setting);
				setCurrentLimitIndicator( (regulation_setting_p->current->RANGE == CURRENT_RANGE_HIGH) ? GUI_CURRENT_RANGE_HIGH : GUI_CURRENT_RANGE_LOW );
				//	FIXME - GUI_CURRENT_LIM_HIGH
				setFeedbackChannelIndicator(regulation_setting_p->CHANNEL);
				// Retore other values from EEPROM
				// TODO
				
				// Start normal GUI operation
				guiEvent.type = GUI_EVENT_START;
				guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
				guiCore_ProcessMessageQueue();
				break;
			case GUI_TASK_REDRAW:
				// Draw GUI
				guiCore_RedrawAll();
				// Flush buffer to LCDs
				LcdUpdateBothByCore(lcdBuffer);
				break;
			case GUI_TASK_EEPROM_STATE:
				guiEvent.type = GUI_EVENT_EEPROM_MESSAGE;
				guiEvent.spec = (uint8_t)msg.data.a;
				guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
				guiCore_ProcessMessageQueue();
				break;
			case GUI_TASK_PROCESS_BUTTONS:
				// msg.data[31:16] = key code, 	msg.data[15:0] = key event type
				guiKeyCode = encodeGuiKey((uint16_t)msg.data.a);
				if (guiKeyCode == 0)
					break;
				guiKeyEvent = encodeGuiKeyEvent((uint16_t)(msg.data.a >> 16));
				if (guiKeyEvent == 0)
					break;
				guiCore_ProcessKeyEvent(guiKeyCode, guiKeyEvent);
				break;
			case GUI_TASK_PROCESS_ENCODER:
				encoder_delta = (int16_t)msg.data.a;
				if (encoder_delta)
					guiCore_ProcessEncoderEvent(encoder_delta);
				break;
			case GUI_TASK_UPDATE_CONVERTER_STATE:
				if (msg.converter_event.spec = VOLTAGE_SETTING_CHANGED)
					UpdateVoltageSetting(msg.converter_event.channel);
				else if (msg.converter_event.spec = CURRENT_SETTING_CHANGED)
					UpdateCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range);
				else if (msg.converter_event.spec = CHANNEL_CHANGED)
				{
					UpdateConverterChannel(msg.converter_event.channel);
					
				}
					
				break;
			
			/*	
			case GUI_TASK_UPDATE_VOLTAGE_CURRENT:
				setVoltageIndicator(voltage_adc);
				setCurrentIndicator(current_adc);
				setPowerIndicator(power_adc);
				break;
			case GUI_TASK_UPDATE_VOLTAGE_SETTING:
				setVoltageSetting(regulation_setting_p->voltage.setting);
				break;
			case GUI_TASK_UPDATE_CURRENT_SETTING:
				setCurrentSetting(regulation_setting_p->current->setting);
				break;
			case GUI_TASK_UPDATE_CURRENT_LIMIT:
				setCurrentLimitIndicator( (regulation_setting_p->current->RANGE == CURRENT_RANGE_HIGH) ? GUI_CURRENT_RANGE_HIGH : GUI_CURRENT_RANGE_LOW );
				// CHECKME - possibly conveter module has to send GUI_TASK_UPDATE_CURRENT_SETTING message when updating limit
				setCurrentSetting(regulation_setting_p->current->setting);			
				break;
			case GUI_TASK_UPDATE_SOFT_LIMIT_SETTINGS:
				setLowVoltageLimitSetting(regulation_setting_p->voltage.enable_low_limit, regulation_setting_p->voltage.limit_low);
				setHighVoltageLimitSetting(regulation_setting_p->voltage.enable_high_limit, regulation_setting_p->voltage.limit_high);
				break;
			case GUI_TASK_UPDATE_FEEDBACK_CHANNEL:
				setFeedbackChannelIndicator( (regulation_setting_p->CHANNEL == CHANNEL_5V) ? GUI_CHANNEL_5V : GUI_CHANNEL_12V );
				// CHECKME - same as above
				setVoltageSetting(regulation_setting_p->voltage.setting);
				setCurrentSetting(regulation_setting_p->current->setting);
				setCurrentLimitIndicator( (regulation_setting_p->current->RANGE == CURRENT_RANGE_HIGH) ? GUI_CURRENT_RANGE_HIGH : GUI_CURRENT_RANGE_LOW );
				setLowVoltageLimitSetting(regulation_setting_p->voltage.enable_low_limit, regulation_setting_p->voltage.limit_low);
				setHighVoltageLimitSetting(regulation_setting_p->voltage.enable_high_limit, regulation_setting_p->voltage.limit_high);
				break;
				*/
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

// Apply voltage setting
void applyGuiVoltageSetting(uint16_t new_set_voltage)
{
	converter_msg.type = CONVERTER_SET_VOLTAGE;
	converter_msg.data.a = new_set_voltage;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}

void applyGuiVoltageLimit(uint8_t type, uint8_t enable, uint16_t value)
{
	converter_msg.type = CONVERTER_SET_VOLTAGE_LIMIT;
	converter_msg.voltage_limit_setting.mode = type;		// 1 - high, 0 - low
	converter_msg.voltage_limit_setting.enable = enable;
	converter_msg.voltage_limit_setting.value = value;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}

// Apply current setting
void applyGuiCurrentSetting(uint16_t new_set_current)
{
	converter_msg.type = CONVERTER_SET_CURRENT;
	converter_msg.data.a = new_set_current;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}

// Apply new current range
void applyGuiCurrentRange(uint8_t new_range)
{
	converter_msg.type = CONVERTER_SET_CURRENT_RANGE;
	if (new_range == GUI_CURRENT_RANGE_HIGH)
		converter_msg.data.a = CURRENT_RANGE_HIGH;
	else
		converter_msg.data.a = CURRENT_RANGE_LOW;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}







