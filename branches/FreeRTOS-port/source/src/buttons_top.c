/**********************************************************
	Top-level module for processing buttons and encoder
	Button task executes once per 10ms, reads button and encoder
	low-level driver state, serializes these events and 
	send them to dispatcher task.
	
	Note: this task also takes care for converter ON/OFF control by 
	special buttons and external switch
	
	Written by: AvegaWanderer 03.2014
**********************************************************/

#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"
#include "MDR32F9Qx_adc.h"

#include "FreeRTOS.h"
#include "task.h"

#include "dispatcher.h"
#include "converter.h"
#include "buttons.h"
#include "encoder.h"
#include "global_def.h"
#include "buttons_top.h"

// TODO: Mutex!!!


/*
External switch mode:
	1. Direct ON (action down) /OFF (action up)
	2. Toggle (by action down)
	3. Toggle to OFF (by action down)
Additional: Inversion ON/OFF, enabled/disabled
*/

extsw_mode_t extsw_mode;

enum ExtSwActions {EXTSW_CMD_NONE, EXTSW_CMD_OFF, EXTSW_CMD_ON};

/*
void BTN_SetExtSwMode(uint8_t newMode, uint8_t inversionEnable)
{
	taskENTER_CRITICAL();
	extsw_mode.mode = newMode;
	extsw_mode.inv = inversionEnable;
	taskEXIT_CRITICAL();
}
*/
uint8_t BTN_GetExtSwMode(void)
{
	return extsw_mode.mode;
}

uint8_t BTN_GetExtSwInversion(void)
{
	return extsw_mode.inv;
}

uint8_t BTN_IsExtSwEnabled(void)
{
	return extsw_mode.enabled;
}


void BTN_LoadProfile()
{
	taskENTER_CRITICAL();
	//device_profile->
	taskEXIT_CRITICAL();
}

void BTN_SaveProfile()
{


}




static uint8_t getExtSwAction(void)
{
	uint16_t sw_up;
	uint16_t sw_down;
	uint8_t ext_cmd = EXTSW_CMD_NONE;
	
	taskENTER_CRITICAL();
	sw_up = (extsw_mode.inv == 0) ? (buttons.action_up & SW_EXTERNAL) : (buttons.action_down & SW_EXTERNAL);
	sw_down = (extsw_mode.inv == 0) ? (buttons.action_down & SW_EXTERNAL) : (buttons.action_up & SW_EXTERNAL);
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
	taskEXIT_CRITICAL();
	return ext_cmd;
}



void vTaskButtons(void *pvParameters) 
{
	uint16_t mask;
	uint8_t extsw_cmd;
	dispatch_msg_t dispatcher_msg;
	
	// Initialize
	portTickType lastExecutionTime = xTaskGetTickCount();
	extsw_mode.inv = 0;
	extsw_mode.mode = EXTSW_TOGGLE;//EXTSW_DIRECT;
	
	while(1)
	{
		vTaskDelayUntil(&lastExecutionTime, ((portTickType)20 / portTICK_RATE_MS));
		
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
	}
	

}
