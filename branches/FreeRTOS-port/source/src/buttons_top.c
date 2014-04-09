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

#include "dispatcher.h"
#include "converter.h"
#include "buttons.h"
#include "encoder.h"
#include "global_def.h"
#include "buttons_top.h"
#include "eeprom.h"

// TODO: Mutex!!!

enum ExtSwActions {EXTSW_CMD_NONE, EXTSW_CMD_OFF, EXTSW_CMD_ON};

static struct extsw_mode_t {
	uint8_t enable;
	uint8_t inverse;
	uint8_t mode;
} extsw_mode;

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
void BTN_LoadProfile(void)
{
	//taskENTER_CRITICAL();
	extsw_mode.enable = device_profile->buttons_profile.ext_switch_enable;
	extsw_mode.inverse = device_profile->buttons_profile.ext_switch_inverse;
	extsw_mode.mode = device_profile->buttons_profile.ext_switch_mode;
	//taskEXIT_CRITICAL();
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
	dispatch_msg_t dispatcher_msg;
	buttons_msg_t msg;
	
	// Initialize
	xQueueButtons = xQueueCreate( 5, sizeof( buttons_msg_t ) );		
	if( xQueueButtons == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	// Default initialization
	extsw_mode.enable = 0;
	extsw_mode.inverse = 0;
	extsw_mode.mode = EXTSW_DIRECT;
	
	while(1)
	{
		xQueueReceive(xQueueButtons, &msg, portMAX_DELAY);
		switch(msg.type)
		{
			case BUTTONS_EXTSWITCH_SETTINGS:
				// Buttons task is the one who can write settings
				extsw_mode.enable = msg.extSwitchSetting.enable;
				extsw_mode.inverse = msg.extSwitchSetting.inverse;
				extsw_mode.mode = msg.extSwitchSetting.mode;
				// Confirm
				if (msg.pxSemaphore != 0)
					xSemaphoreGive(*msg.pxSemaphore);	
				break;
			case BUTTONS_LOAD_PROFILE:
				// Read settings from profile data structure
				BTN_LoadProfile();
				// Confirm
				if (msg.pxSemaphore != 0)
					xSemaphoreGive(*msg.pxSemaphore);
				break;
			case BUTTONS_SAVE_PROFILE:
				// Write settings to profile data structure
				BTN_SaveProfile();
				// Confirm
				if (msg.pxSemaphore != 0)
					xSemaphoreGive(*msg.pxSemaphore);
				break;
			case BUTTONS_TICK:
				ProcessButtons();	
				UpdateEncoderDelta();
				extsw_cmd = getExtSwAction();
				//---------- Converter control -----------//
				dispatcher_msg.type = DISPATCHER_CONVERTER;
				
				// Feedback channel select
				if (buttons.action_down & SW_CHANNEL)
				{
					// Send switch channel to 5V message
					dispatcher_msg.converter_cmd.msg_type = CONVERTER_SWITCH_CHANNEL;
					dispatcher_msg.converter_cmd.a.ch_set.new_channel = CHANNEL_5V;
					xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				}
				else if (buttons.action_up & SW_CHANNEL)
				{
					// Send switch channel to 12V message
					dispatcher_msg.converter_cmd.msg_type = CONVERTER_SWITCH_CHANNEL;
					dispatcher_msg.converter_cmd.a.ch_set.new_channel = CHANNEL_12V;
					xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				}
				
				if ((buttons.action_down & BTN_OFF) || (extsw_cmd == EXTSW_CMD_OFF))
				{
					// Send OFF mesage to converter task
					dispatcher_msg.converter_cmd.msg_type = CONVERTER_TURN_OFF;
					xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				}
				else if ((buttons.action_down & BTN_ON) || (extsw_cmd == EXTSW_CMD_ON))
				{
					// Send ON mesage to converter task
					dispatcher_msg.converter_cmd.msg_type = CONVERTER_TURN_ON;
					xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				}
				
				
				// Serialize button events
				dispatcher_msg.type = DISPATCHER_BUTTONS;
				mask = 0x0001;
				while(mask)
				{
					dispatcher_msg.key_event.code = mask;
					
					if (buttons.action_down & mask)
					{
						dispatcher_msg.key_event.event_type = BTN_EVENT_DOWN;
						xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
					}
				/*	if (buttons.action_up & mask)
					{
						dispatcher_msg.key_event.event_type = BTN_EVENT_UP;
						xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
					} */
					if (buttons.action_up_short & mask)
					{
						dispatcher_msg.key_event.event_type = BTN_EVENT_UP_SHORT;
						xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
					}
					if (buttons.action_hold & mask)
					{
						dispatcher_msg.key_event.event_type = BTN_EVENT_HOLD;
						xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
					}
					mask <<= 1;
				}
				
				// Send encoder events to GUI
				if (encoder_delta)
				{
					dispatcher_msg.type = DISPATCHER_ENCODER;
					dispatcher_msg.encoder_event.delta = encoder_delta;
					xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
				}
				break;
		}
	}
}
