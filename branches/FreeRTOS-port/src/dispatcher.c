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
#include "gui_top.h"
#include "dispatcher.h"



xQueueHandle xQueueDispatcher;





void vTaskDispatcher(void *pvParameters) 
{
	uint32_t msg;
	int16_t encoder_delta;
	conveter_message_t converter_msg;
	uint32_t gui_msg;

	
	// Initialize
	xQueueDispatcher = xQueueCreate( 5, sizeof( uint32_t ) );		// Queue can contain 5 elements of type uint32_t
	if( xQueueDispatcher == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	
	
	while(1)
	{
		xQueueReceive(xQueueDispatcher, &msg, portMAX_DELAY);
		switch (msg)
		{
			case DISPATCHER_TICK:
				ProcessButtons();
				//encoder_delta = GetEncoderDelta();	
				UpdateEncoderDelta();
			
				converter_msg.type = 0;
			
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
				else if (buttons.action_down & BTN_OFF)
				{
					// Send OFF mesage to converter task
					converter_msg.type = CONVERTER_TURN_OFF;
				}
				else if (buttons.action_down & BTN_ON)
				{
					// Send ON mesage to converter task
					converter_msg.type = CONVERTER_TURN_ON;
				}
			
				if (converter_msg.type)
					xQueueSendToBack(xQueueConverter, &converter_msg, 0);
					
				//---------------------------//	
				//---------------------------//

				// Make GUI process buttons and encoder
				gui_msg = GUI_PROCESS_BUTTONS;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
			
			break;
		}
		
	}
	
}



