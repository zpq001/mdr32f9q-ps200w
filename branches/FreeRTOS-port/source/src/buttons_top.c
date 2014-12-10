/**********************************************************
	Top-level module for processing buttons and encoder
	Button task executes once per T ms, reads buttons and encoder
	low-level driver state, serializes these events and 
	sends them to dispatcher task.
	Period is defined by period of tick message to this task.
	
	Note: this task also takes care for converter ON/OFF control by 
	special buttons and external switch
	
External switch mode:
	1. Direct ON (action down) /OFF (action up)
	2. Toggle (by action down)
	3. Toggle to OFF (by action down)
	Additional: Inversion ON/OFF, enabled/disabled

	Written by: AvegaWanderer 03.2014
**********************************************************/

#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"
#include "MDR32F9Qx_adc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

//#include "dispatcher.h"
#include "converter.h"
#include "buttons.h"
#include "encoder.h"
#include "global_def.h"
#include "buttons_top.h"
#include "guiTop.h"
#include "eeprom.h"


enum ExtSwActions {EXTSW_CMD_NONE, EXTSW_CMD_OFF, EXTSW_CMD_ON};

static struct extsw_mode_t {
	uint8_t enable;
	uint8_t inverse;
	uint8_t mode;
} extsw_mode;

static uint8_t operation_enable;
static uint8_t enable_on_off;
static converter_message_t converter_msg;
static gui_msg_t gui_msg;

xQueueHandle xQueueButtons;
const buttons_msg_t buttons_tick_msg = {BUTTONS_TICK};




uint8_t BTN_GetExtSwitchMode(void)
{
	return extsw_mode.mode;
}

uint8_t BTN_GetExtSwitchInversion(void)
{
	return extsw_mode.inverse;
}

uint8_t BTN_IsExtSwitchEnabled(void)
{
	return extsw_mode.enable;
}


// Read configuration from profile structure
static void BTN_LoadProfile(void)
{
	extsw_mode.enable = device_profile->buttons_profile.ext_switch_enable;
	extsw_mode.inverse = device_profile->buttons_profile.ext_switch_inverse;
	extsw_mode.mode = device_profile->buttons_profile.ext_switch_mode;
}

// Write configuration into profile structure
void BTN_SaveProfile(void)
{
	device_profile->buttons_profile.ext_switch_enable = extsw_mode.enable;
	device_profile->buttons_profile.ext_switch_inverse = extsw_mode.inverse;
	device_profile->buttons_profile.ext_switch_mode = extsw_mode.mode;
}




static uint8_t getExtSwAction(void)
{
	uint16_t sw_up;
	uint16_t sw_down;
	uint8_t ext_cmd = EXTSW_CMD_NONE;
	if (extsw_mode.enable)
	{
		sw_up = (extsw_mode.inverse == 0) ? (buttons.action_up & SW_EXTERNAL) : (buttons.action_down & SW_EXTERNAL);
		sw_down = (extsw_mode.inverse == 0) ? (buttons.action_down & SW_EXTERNAL) : (buttons.action_up & SW_EXTERNAL);
		switch (extsw_mode.mode)
		{
			case EXTSW_DIRECT:
				if (sw_down) 
					ext_cmd = EXTSW_CMD_ON;
				else if (sw_up)
					ext_cmd = EXTSW_CMD_OFF;
				break;
			case EXTSW_TOGGLE:
				if (sw_down)
					ext_cmd = (Converter_GetState()) ? EXTSW_CMD_OFF : EXTSW_CMD_ON;
				break;
			case EXTSW_TOGGLE_OFF:
				if (sw_down)
					ext_cmd = EXTSW_CMD_OFF;
				break;
		}
	}
	return ext_cmd;
}



void vTaskButtons(void *pvParameters) 
{
	uint16_t mask;
	uint8_t extsw_cmd;
	buttons_msg_t msg;
	
	// Initialize
	xQueueButtons = xQueueCreate( 5, sizeof( buttons_msg_t ) );		
	if( xQueueButtons == 0 )
		while(1);
	
	// Default initialization
	extsw_mode.enable = 0;
	extsw_mode.inverse = 0;
	extsw_mode.mode = EXTSW_DIRECT;
	
	converter_msg.sender = sender_BUTTONS;
	converter_msg.pxSemaphore = 0;
	
	operation_enable = 0;
	enable_on_off = 0;
	
	while(1)
	{
		xQueueReceive(xQueueButtons, &msg, portMAX_DELAY);
		switch(msg.type)
		{
			case BUTTONS_EXTSWITCH_SETTINGS:
				taskENTER_CRITICAL();
				extsw_mode.enable = msg.extSwitchSetting.enable;
				extsw_mode.inverse = msg.extSwitchSetting.inverse;
				extsw_mode.mode = msg.extSwitchSetting.mode;
				taskEXIT_CRITICAL();
				// Confirm
				if (msg.pxSemaphore != 0)	xSemaphoreGive(*msg.pxSemaphore);	
				break;
			case BUTTONS_LOAD_PROFILE:
				// Read settings from profile data structure
				taskENTER_CRITICAL();
				BTN_LoadProfile();
				taskEXIT_CRITICAL();
				// Confirm
				if (msg.pxSemaphore != 0)	xSemaphoreGive(*msg.pxSemaphore);
				break;
			case BUTTONS_SAVE_PROFILE:
				// Write settings to profile data structure
				BTN_SaveProfile();
				// Confirm
				if (msg.pxSemaphore != 0)	xSemaphoreGive(*msg.pxSemaphore);
				break;
			case BUTTONS_START_OPERATION:
				operation_enable = 1;
				break;
			case BUTTONS_ENABLE_ON_OFF_CONTROL:
				enable_on_off = 1;
				break;
			case BUTTONS_TICK:
				if (!operation_enable)
					break;
				ProcessButtons();	
				UpdateEncoderDelta();
				extsw_cmd = getExtSwAction();
				
				//---------- Converter control -----------//
				converter_msg.type = CONVERTER_CONTROL;
				
				// Feedback channel select
				converter_msg.param = param_CHANNEL;
				if (buttons.action_down & SW_CHANNEL) {
					converter_msg.a.ch_set.new_channel = CHANNEL_5V;
					xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
				} else if (buttons.action_up & SW_CHANNEL) {
					converter_msg.a.ch_set.new_channel = CHANNEL_12V;
					xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
				}
				
				// ON/OFF
				converter_msg.param = param_STATE;
				if (enable_on_off) {				
					if ((buttons.action_down & BTN_OFF) || (extsw_cmd == EXTSW_CMD_OFF)) {
						converter_msg.a.state_set.command = cmd_TURN_OFF;
						xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
					} else if ((buttons.action_down & BTN_ON) || (extsw_cmd == EXTSW_CMD_ON)) {
						converter_msg.a.state_set.command = cmd_TURN_ON;
						xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
					}
				}
				
				// Serialize button events
				gui_msg.type = GUI_TASK_PROCESS_BUTTONS;
				mask = 0x0001;
				while(mask)
				{
					gui_msg.key_event.code = mask;
					
					if (buttons.action_down & mask)
					{
						gui_msg.key_event.event = BTN_EVENT_DOWN;
						xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					}
					if (buttons.action_up_short & mask)
					{
						gui_msg.key_event.event = BTN_EVENT_UP_SHORT;
						xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					}
					if (buttons.action_hold & mask)
					{
						gui_msg.key_event.event = BTN_EVENT_HOLD;
						xQueueSendToBack(xQueueGUI, &gui_msg, 0);
					}
					mask <<= 1;
				}
				
				// Send encoder events to GUI
				if (encoder_delta)
				{
					gui_msg.type = GUI_TASK_PROCESS_ENCODER;
					gui_msg.encoder_event.delta = encoder_delta;
					xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				}
				break;
		}
	}
}
