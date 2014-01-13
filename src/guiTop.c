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



void vTaskGUI(void *pvParameters) 
{
	gui_incoming_msg_t msg;
	uint8_t guiKeyCode;
	uint8_t guiKeyEvent;
	int16_t encoder_delta;
	guiEvent_t guiEvent;
	
	// Initialize
	xQueueGUI = xQueueCreate( 10, sizeof( gui_incoming_msg_t ) );		// GUI queue can contain 10 elements of type gui_incoming_msg_t
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
				setVoltageSetting(regulation_setting_p->set_voltage);
				setCurrentSetting(regulation_setting_p->set_current);
				setCurrentLimitIndicator( (regulation_setting_p->current_limit == CURRENT_LIM_HIGH) ? GUI_CURRENT_LIM_HIGH : GUI_CURRENT_LIM_LOW );
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
				guiEvent.spec = (uint8_t)msg.data;
				guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
				guiCore_ProcessMessageQueue();
				break;
			case GUI_TASK_PROCESS_BUTTONS:
				// msg.data[31:16] = key code, 	msg.data[15:0] = key event type
				guiKeyCode = encodeGuiKey((uint16_t)msg.data);
				if (guiKeyCode == 0)
					break;
				guiKeyEvent = encodeGuiKeyEvent((uint16_t)(msg.data >> 16));
				if (guiKeyEvent == 0)
					break;
				guiCore_ProcessKeyEvent(guiKeyCode, guiKeyEvent);
				break;
			case GUI_TASK_PROCESS_ENCODER:
				encoder_delta = (int16_t)msg.data;
				if (encoder_delta)
					guiCore_ProcessEncoderEvent(encoder_delta);
				break;
			case GUI_TASK_UPDATE_VOLTAGE_CURRENT:
				setVoltageIndicator(voltage_adc);
				setCurrentIndicator(current_adc);
				setPowerIndicator(power_adc);
				break;
			case GUI_TASK_UPDATE_VOLTAGE_SETTING:
				setVoltageSetting(regulation_setting_p->set_voltage);
				break;
			case GUI_TASK_UPDATE_CURRENT_SETTING:
				setCurrentSetting(regulation_setting_p->set_current);
				break;
			case GUI_TASK_UPDATE_CURRENT_LIMIT:
				setCurrentLimitIndicator( (regulation_setting_p->current_limit == CURRENT_LIM_HIGH) ? GUI_CURRENT_LIM_HIGH : GUI_CURRENT_LIM_LOW );
				// CHECKME - possibly conveter module has to send GUI_TASK_UPDATE_CURRENT_SETTING message when updating limit
				setCurrentSetting(regulation_setting_p->set_current);			
				break;
			case GUI_TASK_UPDATE_FEEDBACK_CHANNEL:
				setFeedbackChannelIndicator(regulation_setting_p->CHANNEL);
				// CHECKME - same as above
				setVoltageSetting(regulation_setting_p->set_voltage);
				setCurrentSetting(regulation_setting_p->set_current);
				setCurrentLimitIndicator( (regulation_setting_p->current_limit == CURRENT_LIM_HIGH) ? GUI_CURRENT_LIM_HIGH : GUI_CURRENT_LIM_LOW );
				break;
			case GUI_TASK_UPDATE_TEMPERATURE_INDICATOR:
				setTemperatureIndicator(converter_temp_celsius);
				break;
		}
	}
}

//=================================================================//
//=================================================================//
//                      Hardware control interface                 //
//=================================================================//

// Apply voltage setting from GUI
void applyGuiVoltageSetting(uint16_t new_set_voltage)
{
	converter_msg.type = CONVERTER_SET_VOLTAGE;
	converter_msg.data_a = new_set_voltage;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}

// Apply current setting from GUI
void applyGuiCurrentSetting(uint16_t new_set_current)
{
	converter_msg.type = CONVERTER_SET_CURRENT;
	converter_msg.data_a = new_set_current;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}

// Apply new selected feedback channel
void applyGuiCurrentLimit(uint8_t new_current_limit)
{
	if (new_current_limit == GUI_CURRENT_LIM_HIGH)
		converter_msg.type = SET_CURRENT_LIMIT_40A;
	else
		converter_msg.type = SET_CURRENT_LIMIT_20A;
	xQueueSendToBack(xQueueConverter, &converter_msg, 0);
}







