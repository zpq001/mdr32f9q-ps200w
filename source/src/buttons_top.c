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



void vTaskButtons(void *pvParameters) 
{
	uint16_t mask;
	dispatch_msg_t dispatcher_msg;
	
	// Initialize
	portTickType lastExecutionTime = xTaskGetTickCount();
	
	while(1)
	{
		vTaskDelayUntil(&lastExecutionTime, ((portTickType)20 / portTICK_RATE_MS));
		
		ProcessButtons();	
		UpdateEncoderDelta();
		
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
		
		if ((buttons.action_down & BTN_OFF) || (buttons.action_up & SW_EXTERNAL))
		{
			// Send OFF mesage to converter task
			dispatcher_msg.converter_cmd.msg_type = CONVERTER_TURN_OFF;
			xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
		}
		else if ((buttons.action_down & BTN_ON) || (buttons.action_down & SW_EXTERNAL))
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
