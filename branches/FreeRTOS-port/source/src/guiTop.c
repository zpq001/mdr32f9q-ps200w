#include <stdio.h>

#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "stdio.h"

#include "lcd_1202.h"		// physical driver

//------ GUI components ------//
#include "guiConfig.h"
#include "guiTop.h"
#include "guiFonts.h"
#include "guiGraphHAL.h"
#include "guiGraphPrimitives.h"
#include "guiGraphWidgets.h"
#include "guiCore.h"
#include "guiEvents.h"
#include "guiWidgets.h"
#include "guiTextLabel.h"
#include "guiMainForm.h"
#include "guiMasterPanel.h"
#include "guiSetupPanel.h"
#include "guiMessagePanel1.h"
//----------------------------//

#include "converter.h"	// voltage, current, etc
#include "adc.h"

#include "dispatcher.h"

#include "service.h"	// temperature
#include "control.h"	// some defines
#include "eeprom.h"

#include "buttons.h"
#include "buttons_top.h"
#include "encoder.h"

#include "uart_rx.h"

xQueueHandle xQueueGUI;

static dispatch_msg_t dispatcher_msg;
static converter_message_t converter_msg;
static buttons_msg_t buttons_msg;
static eeprom_message_t eeprom_msg;

static xSemaphoreHandle xSemaphoreEEPROM;
static xSemaphoreHandle xSemaphoreConverter;

static uint8_t profileOperationInProgress = 0;

const gui_msg_t gui_msg_redraw = {GUI_TASK_REDRAW};




// TODO: set proper min and max for voltage and current spinboxes




void vTaskGUI(void *pvParameters) 
{
	gui_msg_t msg;
	guiEvent_t guiEvent;
	int32_t value;
	uint8_t state;
	uint8_t state2;
	uint8_t i;
    uint8_t profileState;
	uint32_t temp32u;
	
	// Initialize
	xQueueGUI = xQueueCreate( 10, sizeof( gui_msg_t ) );		// GUI queue can contain 10 elements of type gui_incoming_msg_t
	if( xQueueGUI == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	vSemaphoreCreateBinary( xSemaphoreEEPROM );
	if( xSemaphoreEEPROM == 0 )
    {
        while(1);
    }
	xSemaphoreTake(xSemaphoreEEPROM, 0);
	
	vSemaphoreCreateBinary( xSemaphoreConverter );
	if( xSemaphoreConverter == 0 )
    {
        while(1);
    }
	xSemaphoreTake(xSemaphoreConverter, 0);
	
	// Common fields
	eeprom_msg.sender = sender_GUI;
	eeprom_msg.xSemaphorePtr = &xSemaphoreEEPROM;
	
	// Create all GUI elements and prepare core
	guiMainForm_Initialize();
    guiCore_Init((guiGenericWidget_t *)&guiMainForm);
	// GUI is ready, but initial welcome screen is what will be displayed.
	// When all other systems init will be done, GUI should be sent 
	// a special message to start operate normally.
	
	
	while(1)
	{
		xQueueReceive(xQueueGUI, &msg, portMAX_DELAY);
		switch (msg.type)
		{
			case GUI_TASK_RESTORE_ALL:
				// Initial master panel update
				guiUpdateChannel();     
				// Initial setup panel update
				// Voltage and current limit update is not required - these widgets will get proper values on show
				guiUpdateOverloadSettings();
				guiUpdateProfileSettings();
				guiUpdateExtswitchSettings();
				guiUpdateDacSettings();
				guiUpdateProfileList();
				// Start normal GUI operation
				guiEvent.type = GUI_EVENT_START;
				guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
				guiCore_ProcessMessageQueue();	
				break;
			case GUI_TASK_REDRAW:
				guiCore_ProcessTimers();
				guiCore_RedrawAll();				// Draw GUI
				LcdUpdateBothByCore(lcdBuffer);		// Flush buffer to LCDs
				break;
			case GUI_TASK_EEPROM_STATE:
				guiEvent.type = GUI_EVENT_EEPROM_MESSAGE;
				guiEvent.spec = (uint8_t)msg.data.a;
				guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
				guiCore_ProcessMessageQueue();
				break;
			case GUI_TASK_PROCESS_BUTTONS:
				guiCore_ProcessKeyEvent(msg.key_event.code, msg.key_event.event);
				break;
			case GUI_TASK_PROCESS_ENCODER:
				guiCore_ProcessEncoderEvent(msg.encoder_event.delta);
				break;
			case GUI_TASK_UPDATE_CONVERTER_STATE:
				switch (msg.converter_event.param)
				{
					case param_VSET:
						guiUpdateVoltageSetting(msg.converter_event.channel);
						break;
					case param_CSET:
						guiUpdateCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range);
						break;
					case param_VLIMIT:
						guiUpdateVoltageLimit(msg.converter_event.channel, (msg.converter_event.limit_type == LIMIT_TYPE_LOW) ? UPDATE_LOW_LIMIT : UPDATE_HIGH_LIMIT);
						// Voltage setting may have been changed - update too
						//guiUpdateVoltageSetting(msg.converter_event.channel);	// Not required - CHECKME!!!
						break;
					case param_CLIMIT:
						guiUpdateCurrentLimit(msg.converter_event.channel, msg.converter_event.current_range, 
							(msg.converter_event.limit_type == LIMIT_TYPE_LOW) ? UPDATE_LOW_LIMIT : UPDATE_HIGH_LIMIT);
						// Current setting may have been changed - update too
						//guiUpdateCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range);	// Not required - CHECKME!!!
						break;
					case param_CRANGE:
						guiUpdateCurrentRange(msg.converter_event.channel);
						break;
					case param_CHANNEL:
						guiUpdateChannel();
						break;
					case param_OVERLOAD_PROTECTION:
						guiUpdateOverloadSettings();
						break;
				}
				break;
			case GUI_TASK_UPDATE_VOLTAGE_CURRENT:
				guiUpdateAdcIndicators();
				break;
			case GUI_TASK_UPDATE_TEMPERATURE_INDICATOR:
				guiUpdateTemperatureIndicator();
				break;
				
			case GUI_TASK_PROFILE_EVENT:
				switch (msg.profile_event.event)
				{
					case PROFILE_LOAD:
						if (msg.profile_event.err_code == EE_PROFILE_VALID)
						{
							// Profile succesfully loaded
							guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_LOADED, 0, 30);
							
							// Bring GUI to initial state - TODO
							
							// Update all widgets that display profile data
							guiUpdateChannel();
							guiUpdateVoltageLimit(CHANNEL_AUTO, UPDATE_LOW_LIMIT | UPDATE_HIGH_LIMIT);
							guiUpdateCurrentLimit(CHANNEL_AUTO, CURRENT_RANGE_AUTO, UPDATE_LOW_LIMIT | UPDATE_HIGH_LIMIT);
							guiUpdateOverloadSettings();
							guiUpdateExtswitchSettings();
						}
						else
						{
							// Profile load failed
							if (msg.profile_event.err_code == EE_PROFILE_CRC_ERROR) 
								guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_CRC_ERROR, 0, 30);
							else
								guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_HW_ERROR, 0, 30);	
							// Update single record 
							guiUpdateProfileListRecord(msg.profile_event.index);
						}
						break;
					case PROFILE_SAVE:
						if (msg.profile_event.err_code == EE_PROFILE_VALID)
						{
							// Profile succesfully saved
							guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_SAVED, 0, 30);
							// Update single record 
							guiUpdateProfileListRecord(msg.profile_event.index);
						}
						else
						{
							// Profile save failed
							if (msg.profile_event.err_code == EE_PROFILE_CRC_ERROR) 
								guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_CRC_ERROR, 0, 30);
							else
								guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_HW_ERROR, 0, 30);
							// Update single record 
							guiUpdateProfileListRecord(msg.profile_event.index);
						}
						break;
					case PROFILE_LOAD_RECENT:
						if (EE_IsRecentProfileRestoreEnabled())
						{
							if (msg.profile_event.err_code == EE_PROFILE_VALID)
							{
								// Profile succesfully loaded
								guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_RESTORED_RECENT, 0, 30);
							}
							else
							{
								// Default profile used due to error
								guiMessagePanel1_Show(MESSAGE_TYPE_WARNING, MESSAGE_PROFILE_LOADED_DEFAULT, 0, 30);
							}
						}
						else
						{
							// Default profile used according to settings
							guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_LOADED_DEFAULT, 0, 30);
						}
						break;
				}
				// Allow another profile load or save request
				profileOperationInProgress = 0;
				break;
				
			/*	
			case GUI_TASK_UPDATE_PROFILE_SETUP:
					// Read profile settings and update widgets
					guiUpdateProfileSettings();
				break;
			*/
		}
	}
}








//===========================================================================//
//===========================================================================//
//===========================================================================//
//===========================================================================//

enum {NO_SEMPHR = 0, USE_SEMPHR = 1};
	
static void fillConverterMessage(uint8_t param, uint8_t use_semaphore)
{
	converter_msg.type = CONVERTER_CONTROL;
	converter_msg.param = param;
	converter_msg.sender = sender_GUI;
	if (use_semaphore == USE_SEMPHR)
		converter_msg.pxSemaphore = &xSemaphoreConverter;
	else
		converter_msg.pxSemaphore = 0;
}


//------------------------------------------------------//
//                  Current range                       //
//------------------------------------------------------//

void guiTop_ApplyCurrentRange(uint8_t channel, uint8_t new_current_range)
{
	fillConverterMessage(param_CRANGE, USE_SEMPHR);
	converter_msg.a.crange_set.channel = channel;
	converter_msg.a.crange_set.new_range = new_current_range;
	xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
	// Wait
	xSemaphoreTake(xSemaphoreConverter, portMAX_DELAY);
	// Converter is done. Update current range
	guiUpdateCurrentRange(channel);
}


//------------------------------------------------------//
//                  Voltage setting                     //
//------------------------------------------------------//


void guiTop_ApplyGuiVoltageSetting(uint8_t channel, int16_t new_set_voltage)
{
	fillConverterMessage(param_VSET, USE_SEMPHR);
	converter_msg.a.v_set.channel = channel;
	converter_msg.a.v_set.value = new_set_voltage;
	xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
	// Wait
	xSemaphoreTake(xSemaphoreConverter, portMAX_DELAY);
	// Converter is done. Update voltage
	guiUpdateVoltageSetting(channel);
}


//------------------------------------------------------//
//                  Current setting                     //
//------------------------------------------------------//

// Current setting GUI -> HW
void guiTop_ApplyCurrentSetting(uint8_t channel, uint8_t range, int16_t new_set_current)
{
	fillConverterMessage(param_CSET, USE_SEMPHR);
	converter_msg.a.c_set.channel = channel;
	converter_msg.a.c_set.range = range;
	converter_msg.a.c_set.value = new_set_current;
	xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
	// Wait
	xSemaphoreTake(xSemaphoreConverter, portMAX_DELAY);
	// Converter is done. Update current
	guiUpdateCurrentSetting(channel, range);
}


//------------------------------------------------------//
//              Voltage/Current limits                  //
//------------------------------------------------------//

void guiTop_ApplyGuiVoltageLimit(uint8_t channel, uint8_t limit_type, uint8_t enable, int16_t value)
{
	fillConverterMessage(param_VLIMIT, USE_SEMPHR);
	converter_msg.a.vlim_set.channel = channel;
	converter_msg.a.vlim_set.type = limit_type;
	converter_msg.a.vlim_set.enable = enable;
	converter_msg.a.vlim_set.value = value;
	xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
	// Wait
	xSemaphoreTake(xSemaphoreConverter, portMAX_DELAY);
	// Converter is done. Update both voltage setting and limit
	guiUpdateVoltageLimit(channel,  (limit_type == LIMIT_TYPE_LOW) ? UPDATE_LOW_LIMIT : UPDATE_HIGH_LIMIT);
	guiUpdateVoltageSetting(channel);
}

void guiTop_ApplyGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t limit_type, uint8_t enable, int16_t value)
{
	fillConverterMessage(param_CLIMIT, USE_SEMPHR);
	converter_msg.a.clim_set.channel = channel;
	converter_msg.a.clim_set.range = currentRange;
	converter_msg.a.clim_set.type = limit_type;
	converter_msg.a.clim_set.enable = enable;
	converter_msg.a.clim_set.value = value;
	xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
	// Wait
	xSemaphoreTake(xSemaphoreConverter, portMAX_DELAY);
	// Converter is done. Update both current setting and limit
	guiUpdateCurrentLimit(channel, currentRange, (limit_type == LIMIT_TYPE_LOW) ? UPDATE_LOW_LIMIT : UPDATE_HIGH_LIMIT);
	guiUpdateCurrentSetting(channel, currentRange);
}



//------------------------------------------------------//
//                  Overload                            //
//------------------------------------------------------//

void guiTop_ApplyGuiOverloadSettings(uint8_t protection_enable, uint8_t warning_enable, int32_t threshold)
{ 
	fillConverterMessage(param_OVERLOAD_PROTECTION, USE_SEMPHR);
	converter_msg.a.overload_set.protection_enable = protection_enable;
	converter_msg.a.overload_set.warning_enable = warning_enable;
	converter_msg.a.overload_set.threshold = threshold;
	xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
	// Wait
	xSemaphoreTake(xSemaphoreConverter, portMAX_DELAY);
	// Converter is done. Update overload settings
	guiUpdateOverloadSettings();
}


//------------------------------------------------------//
//			DAC offset settings							//
//------------------------------------------------------//

void guiTop_ApplyDacSettings(int8_t v_offset, int8_t c_low_offset, int8_t c_high_offset)
{	
	fillConverterMessage(param_DAC_OFFSET, NO_SEMPHR);
	converter_msg.a.dac_set.voltage_offset = v_offset;
	converter_msg.a.dac_set.current_low_offset = c_low_offset;
	converter_msg.a.dac_set.current_high_offset = c_high_offset;
	xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
}



//------------------------------------------------------//
//                  Profiles                            //
//------------------------------------------------------//

void guiTop_ApplyGuiProfileSettings(uint8_t saveRecentProfile, uint8_t restoreRecentProfile)
{	
	eeprom_msg.type = EE_TASK_PROFILE_SETTINGS;
	eeprom_msg.profile_settings.saveRecentProfile = saveRecentProfile;
	eeprom_msg.profile_settings.restoreRecentProfile = restoreRecentProfile;
	xQueueSendToBack(xQueueEEPROM, &eeprom_msg, portMAX_DELAY);
	// Wait for EEPROM task to complete
	xSemaphoreTake(xSemaphoreEEPROM, portMAX_DELAY);
	// Settings may have been applied not exactly - update
	guiUpdateProfileSettings();
}

void guiTop_LoadProfile(uint8_t index)
{
	if (profileOperationInProgress == 0)
	{
		dispatcher_msg.type = DISPATCHER_LOAD_PROFILE;
		dispatcher_msg.profile_load.number = index;
		xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);	
		profileOperationInProgress = 1;		// Preserve repeating until answer is received
	}
}

void guiTop_SaveProfile(uint8_t index, char *profileName)
{
	if (profileOperationInProgress == 0)
	{
		dispatcher_msg.type = DISPATCHER_SAVE_PROFILE;
		dispatcher_msg.profile_save.number = index;
		dispatcher_msg.profile_save.new_name = profileName;
		xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);		
		profileOperationInProgress = 1;		// Preserve repeating until answer is received
	}
}

// Blocking read from EEPROM task
// Possibly should be moved to setupPanel module (from where it's called)
uint8_t readProfileListRecordName(uint8_t index, char *profileName)
{
	uint8_t profileState;
	eeprom_msg.type = EE_TASK_GET_PROFILE_NAME;
	eeprom_msg.profile_name_request.index = index;
	eeprom_msg.profile_name_request.state = &profileState;
	eeprom_msg.profile_name_request.name = profileName;
	xQueueSendToBack(xQueueEEPROM, &eeprom_msg, portMAX_DELAY);
	// Wait for EEPROM task to complete
	xSemaphoreTake(xSemaphoreEEPROM, portMAX_DELAY);
	return profileState;
}



//------------------------------------------------------//
//			External switch settings					//
//------------------------------------------------------//

void guiTop_ApplyExtSwitchSettings(uint8_t enable, uint8_t inverse, uint8_t mode)
{
    buttons_msg.type = BUTTONS_EXTSWITCH_SETTINGS;
	buttons_msg.extSwitchSetting.enable = enable;
	buttons_msg.extSwitchSetting.inverse = inverse;
	buttons_msg.extSwitchSetting.mode = mode;
	xQueueSendToBack(xQueueButtons, &buttons_msg, portMAX_DELAY);
}









//===========================================================================//
//===========================================================================//
//===========================================================================//
//===========================================================================//


void guiTop_ApplyUartSettings(reqUartSettings_t *s)
{
	uart_receiver_msg_t uart_msg;
	uart_msg.type = UART_APPLY_NEW_SETTINGS;
	uart_msg.enable = s->enable;
	uart_msg.parity = s->parity;
	uart_msg.brate = s->brate;
	if (s->uart_num == 1)
		xQueueSendToBack(xQueueUART1RX, &uart_msg, portMAX_DELAY);
	else
		xQueueSendToBack(xQueueUART2RX, &uart_msg, portMAX_DELAY);
}

//------------------------------------------------------//
//                          UART                        //
//------------------------------------------------------//
void guiTop_GetUartSettings(reqUartSettings_t *req)
{
	uart_param_request_t params;
	taskENTER_CRITICAL();
	UART_Get_comm_params(req->uart_num,  &params);
	taskEXIT_CRITICAL();
	req->enable = params.enable;
	req->parity = params.parity;
	req->brate = params.brate;
}









