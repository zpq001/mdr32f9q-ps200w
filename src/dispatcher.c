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



xQueueHandle xQueueDispatcher;



const dispatch_incoming_msg_t dispatcher_tick_msg = {DISPATCHER_TICK, 0};



void vTaskDispatcher(void *pvParameters) 
{
	dispatch_incoming_msg_t income_msg;
	int16_t encoder_delta;
	conveter_message_t converter_msg;
	uint32_t gui_msg;
	
	uint16_t button_emulation = 0;
	int16_t encoder_emulation = 0;
	
	// Initialize
	xQueueDispatcher = xQueueCreate( 10, sizeof( dispatch_incoming_msg_t ) );		// Queue can contain 5 elements of type uint32_t
	if( xQueueDispatcher == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	
	
	while(1)
	{
		xQueueReceive(xQueueDispatcher, &income_msg, portMAX_DELAY);
		converter_msg.type = 0;
		
		switch (income_msg.type)
		{
			case DISPATCHER_TICK:
				ProcessButtons();
				//encoder_delta = GetEncoderDelta();	
				UpdateEncoderDelta();
				
				// Apply emulation
				buttons.action_down |= button_emulation;
				encoder_delta += encoder_emulation;
				button_emulation = 0;
				encoder_emulation = 0;
			
				
			
				// Feedback channel select
				if (buttons.action_down & SW_CHANNEL)
				{
					// Send switch channel to 5V message
					converter_msg.type = CONVERTER_SWITCH_TO_5VCH;
				}
				else if (buttons.action_up & SW_CHANNEL)
				{
					// Send switch channel to 12V message
					converter_msg.type = CONVERTER_SWITCH_TO_12VCH;
				}
				else if ((buttons.action_down & BTN_OFF) || (buttons.action_up & SW_EXTERNAL))
				{
					// Send OFF mesage to converter task
					converter_msg.type = CONVERTER_TURN_OFF;
				}
				else if ((buttons.action_down & BTN_ON) || (buttons.action_down & SW_EXTERNAL))
				{
					// Send ON mesage to converter task
					converter_msg.type = CONVERTER_TURN_ON;
				}
			
				
					
				//---------------------------//	
				//---------------------------//

				// Make GUI process buttons and encoder
				gui_msg = GUI_TASK_PROCESS_BUTTONS;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
			
				break;
				
			//----- button and encoder emulation -----//
			case DP_EMU_BTN_DOWN:
				button_emulation |= income_msg.data;
				break;
			case DP_EMU_ENC_DELTA:
				encoder_emulation += (int16_t)income_msg.data;
				break;
			
				
			//----- converter control -----//
			case DP_CONVERTER_TURN_ON:
				converter_msg.type = CONVERTER_TURN_ON;
				break;
			case DP_CONVERTER_TURN_OFF:
				converter_msg.type = CONVERTER_TURN_OFF;
				break;
			case DP_CONVERTER_SET_VOLTAGE:
				converter_msg.type = CONVERTER_SET_VOLTAGE;
				converter_msg.data_a = income_msg.data;
				break;
			case DP_CONVERTER_SET_CURRENT:
				converter_msg.type = CONVERTER_SET_CURRENT;
				converter_msg.data_a = income_msg.data;
				break;
			case DP_CONVERTER_SET_CURRENT_LIMIT:
				if (income_msg.data == 20)
					converter_msg.type = SET_CURRENT_LIMIT_20A;
				else if (income_msg.data == 40)
					converter_msg.type = SET_CURRENT_LIMIT_40A;
				break;
		}
		
		
		if (converter_msg.type)
			xQueueSendToBack(xQueueConverter, &converter_msg, 0);
	}
	
}



