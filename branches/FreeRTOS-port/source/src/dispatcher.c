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

#include <string.h>


#include "key_def.h"
#include "converter.h"
#include "control.h"
#include "guiTop.h"
#include "dispatcher.h"
#include "sound_driver.h"
#include "eeprom.h"
#include "uart_tx.h"

xQueueHandle xQueueDispatcher;


void vTaskDispatcher(void *pvParameters) 
{
	dispatch_msg_t msg;
	converter_message_t converter_msg;
	uart_transmiter_msg_t transmitter_msg;
	gui_msg_t gui_msg;
	uint8_t temp8u;
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
	//		
	
	// Provide some time for hardware to stay idle
	vTaskDelay( 200 / portTICK_RATE_MS);
	
	// Read EEPROM and restore global settings and recent profile
	temp8u = EE_InitialLoad();
	
	// EEPROM status 
	gui_msg.type = GUI_TASK_EEPROM_STATE;
	// 1 = OK, 0 = FAIL
	gui_msg.data.a = (temp8u == 0) ? 1 : 0;	
	xQueueSendToBack(xQueueGUI, &gui_msg, 0);
	
	// Load converter profile
	converter_msg.type = CONVERTER_LOAD_PROFILE;
	xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
	
	// Wait a bit more
	vTaskDelay( 500 / portTICK_RATE_MS);
	
	// Send GUI task message to read all settings and converter setup and update it's widgets.
	gui_msg.type = GUI_TASK_RESTORE_ALL;
	xQueueSendToBack(xQueueGUI, &gui_msg, 0);
	
	// Some tasks stay suspended. Start them.  - TODO
	// UART?
	
	// Sound notification
	sound_msg = SND_CONV_SETTING_OK | SND_CONVERTER_PRIORITY_NORMAL;
	xQueueSendToBack(xQueueSound, &sound_msg, 0);
	
	
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
			
			//---------------- Test function  --------------//	
			case DISPATCHER_TEST_FUNC1:
				//EE_SaveGlobalSettings();
				//EE_SaveRecentProfile();
				transmitter_msg.type = UART_SEND_POWER_CYCLES_STAT;
				if (msg.sender == sender_UART1)
					xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0);
				else if (msg.sender == sender_UART2)
					xQueueSendToBack(xQueueUART2TX, &transmitter_msg, 0);
				break;
				
			//----------------- Load profile ---------------//	
			case DISPATCHER_LOAD_PROFILE:
				// Check profile state
				if (EE_GetProfileState(msg.profile_load.number) == EE_PROFILE_VALID)
				{
					// State OK. Try to load data
					if (EE_LoadDeviceProfile(msg.profile_load.number) == EE_OK)
					{
						// New profile data is loaded
						converter_msg.type = CONVERTER_LOAD_PROFILE;
						xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
						// Send response to GUI
						// TODO
						// Bring GUI to initial state
						// TODO
					}
					else
					{
						// Send response ERROR to GUI
						// TODO
						// Repopulate GUI profile list (or update exact profile record)
						// TODO
					}
				}
				else
				{	
					// Send response ERROR to GUI
					// TODO
				}
				break;
			
			//----------------- Save profile ---------------//	
			case DISPATCHER_SAVE_PROFILE:
				// Try to save data
				if (EE_SaveDeviceProfile(msg.profile_save.number, msg.profile_save.new_name) == EE_OK)
				{
					// Profile data is saved
					// Send response to GUI
					// TODO
					// Repopulate GUI profile list (or update exact profile record)
					// TODO
				}
				else
				{
					// Profile data cannot be saved
					// Send response to GUI
					// TODO
					// Repopulate GUI profile list (or update exact profile record)
					// TODO
				}
				break;
			
			//-------------- Converter command -------------//
			case DISPATCHER_CONVERTER:
				converter_msg.type = msg.converter_cmd.msg_type; 
				converter_msg.sender = msg.sender;
				memcpy(&converter_msg.a, &msg.converter_cmd.a, sizeof(converter_arguments_t));
				// TODO: disable turn ON to converter for N ms after power-on
				xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				break;
			
			//------------- Converter response -------------//
			case DISPATCHER_CONVERTER_EVENT:
				// Converter has processed a message and returned feedback (or generated message itself):
				// income_msg.converter_event.type provides information about message been processed.
				// income_msg.converter_event.sender provides information about message origin.
				// Other fields provide information about converter message process status
				
				// Always notify GUI
				gui_msg.type = GUI_TASK_UPDATE_CONVERTER_STATE;
				gui_msg.converter_event.spec = msg.converter_event.spec;
				gui_msg.converter_event.channel = msg.converter_event.channel;
				gui_msg.converter_event.current_range = msg.converter_event.range;
				gui_msg.converter_event.type = msg.converter_event.limit_type;
				xQueueSendToBack(xQueueGUI, &gui_msg, 0);
				
				// Sound feedback
				if ( (msg.converter_event.msg_sender != sender_UART1) && (msg.converter_event.msg_sender != sender_UART2) )
				{
					// CHECKME - this can be actualy decided by sound task itself, depending on settings
					sound_msg = 0;
					switch (msg.converter_event.spec)
					{
						case VOLTAGE_SETTING_CHANGE:
						case CURRENT_SETTING_CHANGE:
						case VOLTAGE_LIMIT_CHANGE:
						case CURRENT_LIMIT_CHANGE:
						case OVERLOAD_SETTING_CHANGE:
							if (msg.converter_event.err_code == VALUE_OK)
								sound_msg = SND_CONV_SETTING_OK | SND_CONVERTER_PRIORITY_NORMAL;
							else
								sound_msg = SND_CONV_SETTING_ILLEGAL | SND_CONVERTER_PRIORITY_NORMAL;	
							break;
						//---------------------//
						case CHANNEL_CHANGE:
						case CURRENT_RANGE_CHANGE:
							if (msg.converter_event.err_code == VALUE_BOUND_BY_ABS_MAX)
								sound_msg = SND_CONV_SETTING_ILLEGAL | SND_CONVERTER_PRIORITY_NORMAL;
							else if (msg.converter_event.err_code == CMD_ERROR)
								sound_msg = SND_CONV_CMD_ILLEGAL | SND_CONVERTER_PRIORITY_NORMAL;	
							break;
						//---------------------//
						case STATE_CHANGE_TO_OVERLOAD:
							if (msg.converter_event.err_code != EVENT_ERROR)
								sound_msg = SND_CONV_OVERLOADED | SND_CONVERTER_PRIORITY_HIGHEST;
							else
								sound_msg = SND_CONV_CMD_ILLEGAL | SND_CONVERTER_PRIORITY_HIGHEST;
							break;
						//---------------------//
						default:
							if ((msg.converter_event.err_code == CMD_ERROR) || (msg.converter_event.err_code == EVENT_ERROR))
								sound_msg = SND_CONV_CMD_ILLEGAL | SND_CONVERTER_PRIORITY_NORMAL;	
							break;
					}					
					if (sound_msg)
						xQueueSendToBack(xQueueSound, &sound_msg, 0); 
				}
			
				// Serial feedback
				if ( (msg.converter_event.msg_sender == sender_UART1) || (msg.converter_event.msg_sender == sender_UART2) )
				{
					transmitter_msg.type = UART_RESPONSE_OK;
					if (msg.converter_event.msg_sender == sender_UART1)
						xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0);
					else
						xQueueSendToBack(xQueueUART2TX, &transmitter_msg, 0);
				}
			
			
				break;
			
				
		}
		
		
	}
	
}



