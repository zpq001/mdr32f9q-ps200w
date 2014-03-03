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

//#include "buttons.h"
//#include "encoder.h"
#include "key_def.h"
#include "converter.h"
#include "control.h"
//#include "gui_top.h"
#include "guiTop.h"
#include "dispatcher.h"
#include "sound_driver.h"

#include "uart_tx.h"

xQueueHandle xQueueDispatcher;

//const dispatch_msg_t dispatcher_tick_msg = {DISPATCHER_TICK, 0};




void vTaskDispatcher(void *pvParameters) 
{
	dispatch_msg_t msg;
	converter_message_t converter_msg;
	uart_transmiter_msg_t transmitter_msg;
	gui_msg_t gui_msg;
	
	uint32_t sound_msg;
	
	// Initialize
	xQueueDispatcher = xQueueCreate( 10, sizeof( dispatch_msg_t ) );		// Queue can contain 5 elements of type uint32_t
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
	gui_msg.data.a = 1;	// 1 = OK, 0 = FAIL
	xQueueSendToBack(xQueueGUI, &gui_msg, 0);
	
	// Init and start converter
	//ProcessButtons();
	//if (buttons.raw_state & SW_CHANNEL)
	//	Converter_Init(CHANNEL_5V);
	//else
		Converter_Init(CHANNEL_12V);
	vTaskResume(xTaskHandle_Converter);	
	
	// Wait a bit more
	vTaskDelay( 200 / portTICK_RATE_MS);
	
	// Send GUI task message to read all settings and converter setup and update it's widgets.
	gui_msg.type = GUI_TASK_RESTORE_ALL;
	xQueueSendToBack(xQueueGUI, &gui_msg, 0);
	
	// Some tasks stay suspended. Start them.  - TODO
	// UART?
	
	// Sound notification
	sound_msg = SND_CONV_SETTING_OK;
	xQueueSendToBack(xQueueSound, &sound_msg, 0);
	
	// Clear message queue from tick messages and start normal operation
	//xQueueReset(xQueueDispatcher);
	
	while(1)
	{
		xQueueReceive(xQueueDispatcher, &msg, portMAX_DELAY);
		
		switch (msg.type)
		{
			//---------------- Button event ----------------//	
			case DISPATCHER_BUTTONS:
				// Send button events to GUI
				gui_msg.type = GUI_TASK_PROCESS_BUTTONS;
				gui_msg.key_event.event = msg.key_event.event_type;
				gui_msg.key_event.code = msg.key_event.code;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				break;
				
			//---------------- Encoder event ---------------//	
			case DISPATCHER_ENCODER:
				// Send encoder events to GUI
				gui_msg.type = GUI_TASK_PROCESS_ENCODER;
				gui_msg.encoder_event.delta = msg.encoder_event.delta;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				break;
			
			//-------------- Converter command -------------//
			case DISPATCHER_CONVERTER:
				converter_msg.type = msg.converter_cmd.type;
				converter_msg.sender = msg.sender;
				converter_msg.channel = msg.converter_cmd.channel;
				converter_msg.range = msg.converter_cmd.range;
				converter_msg.limit_type = msg.converter_cmd.limit_type;
				converter_msg.enable = msg.converter_cmd.enable;
				converter_msg.value = msg.converter_cmd.value;
				xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				break;
			
			//------------- Converter response -------------//
			case DISPATCHER_CONVERTER_RESPONSE:
				// Converter has processed a message and returned feedback:
				// income_msg.converter_resp.type provides information about message been processed.
				// income_msg.converter_resp.sender provides information about message origin.
				// Other fields provide information about converter message process status
				
				// Always notify GUI
				gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
				gui_msg.converter_event.spec = msg.converter_resp.spec;
				gui_msg.converter_event.channel = msg.converter_resp.channel;
				gui_msg.converter_event.current_range = msg.converter_resp.range;
				gui_msg.converter_event.type = msg.converter_resp.limit_type;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				
				// Sound feedback
				if ( (msg.converter_resp.msg_sender != sender_UART1) && (msg.converter_resp.msg_sender != sender_UART2) )
				{
					// CHECKME - this can be actualy decided by sound task itself, depending on settings
					if (msg.converter_resp.err_code == 0)
						sound_msg = SND_CONV_SETTING_OK;
					else
						sound_msg = SND_CONV_SETTING_ILLEGAL;
					sound_msg |= SND_CONVERTER_PRIORITY_NORMAL;
					xQueueSendToBack(xQueueSound, &sound_msg, 0); 
				}
			
				// Serial feedback
				if ( (msg.converter_resp.msg_sender == sender_UART1) || (msg.converter_resp.msg_sender == sender_UART2) )
				{
					transmitter_msg.type = UART_RESPONSE_OK;
					if (msg.converter_resp.msg_sender == sender_UART1)
						xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0);
					else
						xQueueSendToBack(xQueueUART2TX, &transmitter_msg, 0);
				}
			
			
				break;
			
				
		}
		
		
	}
	
}



