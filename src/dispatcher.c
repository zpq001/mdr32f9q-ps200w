/********************************************************************
	Module		Dispatcher
	
	Purpose:
		
					
	Globals used:

		
	Operation: FIXME

********************************************************************/


#include "MDR32Fx.h" 

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "buttons.h"
#include "encoder.h"
#include "converter.h"
#include "control.h"
//#include "gui_top.h"
#include "guiTop.h"
#include "dispatcher.h"
#include "sound_driver.h"


xQueueHandle xQueueDispatcher;

const dispatch_msg_t dispatcher_tick_msg = {DISPATCHER_TICK, 0};




void vTaskDispatcher(void *pvParameters) 
{
	dispatch_incoming_msg_t income_msg;
	conveter_message_t converter_msg;
	gui_incoming_msg_t gui_msg;
	uint16_t mask;
	uint32_t sound_msg;
	
	// Initialize
	xQueueDispatcher = xQueueCreate( 10, sizeof( dispatch_incoming_msg_t ) );		// Queue can contain 5 elements of type uint32_t
	if( xQueueDispatcher == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	
	//---------- Task init sequence ----------//
	// Tasks suspended on this moment:
	//		- vTaskConverter
	
	// Provide some time for hardware to stay idle
	vTaskDelay( 500 / portTICK_RATE_MS);
	
	// EEPROM status (fake for now)
	gui_msg.type = GUI_TASK_EEPROM_STATE;
	gui_msg.data = 1;	// 1 = OK, 0 = FAIL
	xQueueSendToBack(xQueueGUI, &gui_msg, 0);
	
	// Init and start converter
	ProcessButtons();
	if (buttons.raw_state & SW_CHANNEL)
		Converter_Init(CHANNEL_5V);
	else
		Converter_Init(CHANNEL_12V);
	//vTaskResume(vTaskConverter);	
	taskConverter_Enable = 1;
	
	// Wait a bit more
	vTaskDelay( 200 / portTICK_RATE_MS);
	
	// Send GUI task message to read all settings and converter setup and update it's widgets.
	gui_msg.type = GUI_TASK_RESTORE_ALL;
	xQueueSendToBack(xQueueGUI, &gui_msg, 0);
	
	// Some tasks stay suspended. Start them.  - TODO
	
	// Sound notification
	sound_msg = SND_CONV_SETTING_OK;
	xQueueSendToBack(xQueueSound, &sound_msg, 0);
	
	// Clear message queue from tick messages and start normal operation
	xQueueReset(xQueueDispatcher);
	
	while(1)
	{
		xQueueReceive(xQueueDispatcher, &income_msg, portMAX_DELAY);
		
		switch (income_msg.type)
		{
			case DISPATCHER_TICK:
			
				ProcessButtons();	
				UpdateEncoderDelta();
				
				//---------- Converter control -----------//
				
				// Feedback channel select
				if (buttons.action_down & SW_CHANNEL)
				{
					// Send switch channel to 5V message
					converter_msg.type = CONVERTER_SWITCH_CHANNEL;
					converter_msg.channel_setting.new_channel = CHANNEL_5V;
					xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				}
				else if (buttons.action_up & SW_CHANNEL)
				{
					// Send switch channel to 12V message
					converter_msg.type = CONVERTER_SWITCH_CHANNEL;
					converter_msg.channel_setting.new_channel = CHANNEL_12V;
					xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				}
				
				if ((buttons.action_down & BTN_OFF) || (buttons.action_up & SW_EXTERNAL))
				{
					// Send OFF mesage to converter task
					converter_msg.type = CONVERTER_TURN_OFF;
					xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				}
				else if ((buttons.action_down & BTN_ON) || (buttons.action_down & SW_EXTERNAL))
				{
					// Send ON mesage to converter task
					converter_msg.type = CONVERTER_TURN_ON;
					xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				}
				
				
				//------------- GUI control --------------//
				
				// Serialize button events to GUI
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
					if (buttons.action_up & mask)
					{
						gui_msg.key_event.event = BTN_EVENT_UP;
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
				
			//----- button and encoder emulation -----//
			case DISPATCHER_EMULATE_BUTTON:
				gui_msg.type = GUI_TASK_PROCESS_BUTTONS;
				gui_msg.key_event.event = (uint16_t)income_msg.data;
				gui_msg.key_event.code = (uint16_t)(income_msg.data >> 16);
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				break;
			case DP_EMU_ENC_DELTA:
				gui_msg.type = GUI_TASK_PROCESS_ENCODER;
				gui_msg.encoder_event.delta = (int16_t)income_msg.data;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				break;
			
			//----- converter control -----//
			case DP_CONVERTER_TURN_ON:
				converter_msg.type = CONVERTER_TURN_ON;
				xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				break;
			case DP_CONVERTER_TURN_OFF:
				converter_msg.type = CONVERTER_TURN_OFF;
				xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				break;
			case DP_CONVERTER_SET_VOLTAGE:
				converter_msg.type = CONVERTER_SET_VOLTAGE;
				converter_msg.voltage_setting.channel = OPERATING_CHANNEL;		// TODO - move channel specifier to UART parser
				converter_msg.voltage_setting.value = income_msg.data;
				xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				break;
			case DP_CONVERTER_SET_CURRENT:
				converter_msg.type = CONVERTER_SET_CURRENT;
				converter_msg.current_setting.channel = OPERATING_CHANNEL;		
				converter_msg.current_setting.range = OPERATING_CURRENT_RANGE;
				converter_msg.current_setting.value = income_msg.data;
				xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				break;
			case DP_CONVERTER_SET_CURRENT_LIMIT:
				converter_msg.type = CONVERTER_SET_CURRENT_RANGE;
				converter_msg.current_range_setting.channel = OPERATING_CHANNEL;		
				converter_msg.current_range_setting.new_range = (income_msg.data == 20) ? CURRENT_RANGE_LOW : CURRENT_RANGE_HIGH;
				xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				break;
		}
		
		
	}
	
}



