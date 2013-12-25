#include <stdio.h>

#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "stdio.h"

#include "lcd_1202.h"		// physical driver

//------ GUI components ------//
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


void vTaskGUI(void *pvParameters) 
{
	uint32_t msg;
	uint8_t i;
	uint8_t button_spec;
	
	// Initialize
	xQueueGUI = xQueueCreate( 10, sizeof( uint32_t ) );		// GUI queue can contain 10 elements of type uint32_t
	if( xQueueGUI == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	// GUI initialize
	guiMainForm_Initialize();
    guiCore_Init((guiGenericWidget_t *)&guiMainForm);
	// TODO - add restoring values from EEPROM
	
	while(1)
	{
		xQueueReceive(xQueueGUI, &msg, portMAX_DELAY);
		switch (msg)
		{
			case GUI_TASK_REDRAW:
				// Draw GUI
				guiCore_RedrawAll();
				// Flush buffer to LCDs
				LcdUpdateBothByCore(lcdBuffer);
				break;
			case GUI_TASK_PROCESS_BUTTONS:
				// Serialize button events
				button_spec = GUI_KEY_EVENT_DOWN;
				if (buttons.action_down & BTN_ESC)
					guiCore_ProcessKeyEvent(GUI_KEY_ESC, button_spec);
				if (buttons.action_down & BTN_OK)
					guiCore_ProcessKeyEvent(GUI_KEY_OK, button_spec);
				if (buttons.action_down & BTN_LEFT)
					guiCore_ProcessKeyEvent(GUI_KEY_LEFT, button_spec);
				if (buttons.action_down & BTN_RIGHT)
					guiCore_ProcessKeyEvent(GUI_KEY_RIGHT, button_spec);
				if (buttons.action_down & BTN_ENCODER)
					guiCore_ProcessKeyEvent(GUI_KEY_ENCODER, button_spec);
				// Encoder
				if (encoder_delta)
					guiCore_ProcessEncoderEvent(delta);
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







