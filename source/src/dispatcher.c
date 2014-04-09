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
#include "buttons_top.h"

xQueueHandle xQueueDispatcher;

static eeprom_message_t eeprom_msg;
static xSemaphoreHandle xSemaphoreEEPROM;
static xSemaphoreHandle xSemaphoreSync;


void vTaskDispatcher(void *pvParameters) 
{
	dispatch_msg_t msg;
	converter_message_t converter_msg;
	uart_transmiter_msg_t transmitter_msg;
	gui_msg_t gui_msg;
	uint8_t eepromState;
	uint32_t sound_msg;
	buttons_msg_t buttons_msg;
	
	// Initialize
	xQueueDispatcher = xQueueCreate( 10, sizeof( dispatch_msg_t ) );		// Queue can contain 5 elements of type uint32_t
	if( xQueueDispatcher == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	vSemaphoreCreateBinary( xSemaphoreEEPROM );
	if( xSemaphoreEEPROM == 0 )
    {
        while(1);
    }
	
	vSemaphoreCreateBinary( xSemaphoreSync );
	if( xSemaphoreSync == 0 )
    {
        while(1);
    }
	
	//---------- Task init sequence ----------//
	// Tasks suspended on this moment:
	//		
	
	// Provide some time for hardware to stay idle
	vTaskDelay( 100 / portTICK_RATE_MS);
	
	// Read EEPROM and restore global settings and recent profile
	eeprom_msg.type = EE_TASK_INITIAL_LOAD;
	eeprom_msg.initial_load.state = &eepromState;
	eeprom_msg.xSemaphorePtr = &xSemaphoreEEPROM;
	xSemaphoreTake(xSemaphoreEEPROM, 0);
	xQueueSendToBack(xQueueEEPROM, &eeprom_msg, portMAX_DELAY);
	// Wait for EEPROM task to complete
	xSemaphoreTake(xSemaphoreEEPROM, portMAX_DELAY);
	
	// EEPROM status 
	gui_msg.type = GUI_TASK_EEPROM_STATE;
	// 1 = OK, 0 = FAIL
	gui_msg.data.a = (eepromState == 0) ? 1 : 0;	
	xQueueSendToBack(xQueueGUI, &gui_msg, 0);
	
	// Load converter profile
	converter_msg.type = CONVERTER_LOAD_PROFILE;
	xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
	
	// Load profile for buttons
	buttons_msg.type = BUTTONS_LOAD_PROFILE;
	buttons_msg.pxSemaphore = 0;
	xQueueSendToBack(xQueueButtons, &buttons_msg, 0);
					
	// Wait a bit more
	vTaskDelay( 250 / portTICK_RATE_MS);
	
	// Send GUI task message to read all settings and converter setup and update it's widgets.
	gui_msg.type = GUI_TASK_RESTORE_ALL;
	xQueueSendToBack(xQueueGUI, &gui_msg, 0);
	
	// Display profile restore state
	gui_msg.type = GUI_TASK_PROFILE_EVENT;
	gui_msg.profile_event.event = PROFILE_LOAD_RECENT;
	gui_msg.profile_event.err_code = (eepromState & (EE_PROFILE_HW_ERROR | EE_PROFILE_CRC_ERROR));
	gui_msg.profile_event.index = msg.profile_load_response.index;
	xQueueSendToBack(xQueueGUI, &gui_msg, portMAX_DELAY);
	
	// Some tasks stay suspended. Start them.  - TODO
	// UART?
	
	// Sound notification
	sound_msg = SND_CONV_SETTING_OK | SND_CONVERTER_PRIORITY_NORMAL;
	xQueueSendToBack(xQueueSound, &sound_msg, 0);
	
	
	/* TODO: 
<done>	-  add GUI menu for external switch
		-  add GUI menu for UARTs ?
		-  add GUI menu for DAC offset (global settings) ?
<done>	-  check if additional EEPROM settings are required
<done>	-  add GUI input box for user profile name
		-  add various sound signals
		-  clean up GUI (bringing to initial state)
		-  !!! check and remove voltage spike upon power ON !!!
		-  check resource sharing - add critical sections (for example, current limit update in converter)
		-  check UART command interface
		-  Add percent setting of voltage or current
		-  Add charge function
		-  Improve cooler control
		-  Add overheat warning and auto OFF function
	*/
	
	
	
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
				transmitter_msg.type = UART_SEND_POWER_CYCLES_STAT;
				if (msg.sender == sender_UART1)
					xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0);
				else if (msg.sender == sender_UART2)
					xQueueSendToBack(xQueueUART2TX, &transmitter_msg, 0);
				break;
				
			//----------------- Load profile ---------------//	
			case DISPATCHER_LOAD_PROFILE:
				// Send to EEPROM task command to read profile data
				eeprom_msg.type = EE_TASK_LOAD_PROFILE;
				eeprom_msg.profile_load_request.index = msg.profile_load.number;
				xQueueSendToBack(xQueueEEPROM, &eeprom_msg, 0);
				break;
			case DISPATCHER_LOAD_PROFILE_RESPONSE:
				// EEPROM task has completed profile data loading
				// Check state
				if (msg.profile_load_response.profileState == EE_PROFILE_VALID)
				{
					// New profile data is loaded. 
					converter_msg.type = CONVERTER_LOAD_PROFILE;
					xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
					// Wait for conveter task to complete - TODO
					
					// Load profile for buttons
					buttons_msg.type = BUTTONS_LOAD_PROFILE;
					buttons_msg.pxSemaphore = &xSemaphoreSync;
					xSemaphoreTake(xSemaphoreSync, 0);
					xQueueSendToBack(xQueueButtons, &buttons_msg, 0);
					// Wait for task to complete
					xSemaphoreTake(xSemaphoreSync, portMAX_DELAY);
					
					// Send response to GUI
					gui_msg.type = GUI_TASK_PROFILE_EVENT;
					gui_msg.profile_event.event = PROFILE_LOAD;
					gui_msg.profile_event.err_code = msg.profile_load_response.profileState;
					gui_msg.profile_event.index = msg.profile_load_response.index;
					xQueueSendToBack(xQueueGUI, &gui_msg, portMAX_DELAY);
					// Send response to sound task
					sound_msg = SND_CONV_CMD_OK | SND_CONVERTER_PRIORITY_NORMAL;
					xQueueSendToBack(xQueueSound, &sound_msg, 0);
				}
				else
				{
					// Send response ERROR to GUI
					gui_msg.type = GUI_TASK_PROFILE_EVENT;
					gui_msg.profile_event.event = PROFILE_LOAD;
					gui_msg.profile_event.err_code = msg.profile_load_response.profileState;
					gui_msg.profile_event.index = msg.profile_load_response.index;
					xQueueSendToBack(xQueueGUI, &gui_msg, portMAX_DELAY);
					// Send response to sound task
					sound_msg = SND_CONV_CMD_ILLEGAL | SND_CONVERTER_PRIORITY_NORMAL;
					xQueueSendToBack(xQueueSound, &sound_msg, 0);
				}
				break;
			
			//----------------- Save profile ---------------//	
			case DISPATCHER_SAVE_PROFILE:
				// Prepare data structure (fills with FFs)
				EE_GetReadyForProfileSave();		 
				// Collect information from tasks
				// Converter task
				converter_msg.type = CONVERTER_SAVE_PROFILE;
				converter_msg.pxSemaphore = &xSemaphoreSync;
				xSemaphoreTake(xSemaphoreSync, 0);
				xQueueSendToBack(xQueueConverter, &converter_msg, 0);
				// Wait for task to complete
				xSemaphoreTake(xSemaphoreSync, portMAX_DELAY);
				
				// Buttons task
				buttons_msg.type = BUTTONS_SAVE_PROFILE;
				buttons_msg.pxSemaphore = &xSemaphoreSync;
				xSemaphoreTake(xSemaphoreSync, 0);
				xQueueSendToBack(xQueueButtons, &buttons_msg, 0);
				// Wait for task to complete
				xSemaphoreTake(xSemaphoreSync, portMAX_DELAY);
				
				// Ready to write profile data to EEPROM
				eeprom_msg.type = EE_TASK_SAVE_PROFILE;
				eeprom_msg.profile_save_request.index  = msg.profile_save.number;
				eeprom_msg.profile_save_request.newName = msg.profile_save.new_name;
				xQueueSendToBack(xQueueEEPROM, &eeprom_msg, 0);	
				break;
			case DISPATCHER_SAVE_PROFILE_RESPONSE:
				// EEPROM task has completed profile data saving
				// Check state
				if (msg.profile_save_response.profileState == EE_PROFILE_VALID)
				{
					// Profile data is saved
					// Send response to GUI
					gui_msg.type = GUI_TASK_PROFILE_EVENT;
					gui_msg.profile_event.event = PROFILE_SAVE;
					gui_msg.profile_event.err_code = msg.profile_save_response.profileState;
					gui_msg.profile_event.index = msg.profile_save_response.index;
					xQueueSendToBack(xQueueGUI, &gui_msg, portMAX_DELAY);
					// Send response to sound task
					sound_msg = SND_CONV_CMD_OK | SND_CONVERTER_PRIORITY_NORMAL;
					xQueueSendToBack(xQueueSound, &sound_msg, 0); 
				}
				else
				{
					// Profile data cannot be saved
					// Send response to GUI
					gui_msg.type = GUI_TASK_PROFILE_EVENT;
					gui_msg.profile_event.event = PROFILE_SAVE;
					gui_msg.profile_event.err_code = msg.profile_save_response.profileState;
					gui_msg.profile_event.index = msg.profile_save_response.index;
					xQueueSendToBack(xQueueGUI, &gui_msg, portMAX_DELAY);
					// Send response to sound task
					sound_msg = SND_CONV_CMD_ILLEGAL | SND_CONVERTER_PRIORITY_NORMAL;
					xQueueSendToBack(xQueueSound, &sound_msg, 0);
				}
				break;
				
			//---------------- Setup profile ---------------//		
			case DISPATCHER_PROFILE_SETUP:
				// Access directly
				EE_ApplyProfileSettings(msg.profile_setup.saveRecentProfile, msg.profile_setup.restoreRecentProfile);
				// Send response to GUI
				gui_msg.type = GUI_TASK_UPDATE_PROFILE_SETUP;
				xQueueSendToBack(xQueueGUI, &gui_msg, portMAX_DELAY);
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



