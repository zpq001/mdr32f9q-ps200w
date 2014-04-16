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


xQueueHandle xQueueGUI;

static dispatch_msg_t dispatcher_msg;
static buttons_msg_t buttons_msg;
static eeprom_message_t eeprom_msg;



static xSemaphoreHandle xSemaphoreEEPROM;


static char profileName[EE_PROFILE_NAME_SIZE];

static uint8_t profileOperationInProgress = 0;

const gui_msg_t gui_msg_redraw = {GUI_TASK_REDRAW};

// Blockinbg read from EEPROM task
static uint8_t readProfileListRecordName(uint8_t index, char *profileName)
{
	uint8_t profileState;
	eeprom_msg.type = EE_TASK_GET_PROFILE_NAME;
	eeprom_msg.profile_name_request.index = index;
	eeprom_msg.profile_name_request.state = &profileState;
	eeprom_msg.profile_name_request.name = profileName;
	xSemaphoreTake(xSemaphoreEEPROM, 0);
	xQueueSendToBack(xQueueEEPROM, &eeprom_msg, portMAX_DELAY);
	// Wait for EEPROM task to complete
	xSemaphoreTake(xSemaphoreEEPROM, portMAX_DELAY);
	return profileState;
}



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
				value = Converter_GetFeedbackChannel();
				setGuiFeedbackChannel(value);
				// Restore other values from EEPROM
				// TODO
				
				// Start normal GUI operation
				guiEvent.type = GUI_EVENT_START;
				guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
				guiCore_ProcessMessageQueue();

				// Populate GUI profile list
				for (i = 0; i < EE_PROFILES_COUNT; i++)
				{
					profileState = readProfileListRecordName(i, profileName);
					setGuiProfileRecordState(i, profileState, profileName);
				}
				
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
				switch (msg.converter_event.spec)
				{
					case VOLTAGE_SETTING_CHANGE:
						value = Converter_GetVoltageSetting(msg.converter_event.channel);
						setGuiVoltageSetting(msg.converter_event.channel, value);
						break;
					case CURRENT_SETTING_CHANGE:
						value = Converter_GetCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range);
						setGuiCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range, value);
						break;
					case VOLTAGE_LIMIT_CHANGE:
						value = Converter_GetVoltageLimitSetting(msg.converter_event.channel, msg.converter_event.type);
						state = Converter_GetVoltageLimitState(msg.converter_event.channel, msg.converter_event.type);
						setGuiVoltageLimitSetting(msg.converter_event.channel, msg.converter_event.type, state, value);
						// Update voltage setting too
						value = Converter_GetVoltageSetting(msg.converter_event.channel);
						setGuiVoltageSetting(msg.converter_event.channel, value);
						break;
					case CURRENT_LIMIT_CHANGE:
						value = Converter_GetCurrentLimitSetting(msg.converter_event.channel, msg.converter_event.current_range, msg.converter_event.type);
						state = Converter_GetCurrentLimitState(msg.converter_event.channel, msg.converter_event.current_range, msg.converter_event.type);
						setGuiCurrentLimitSetting(msg.converter_event.channel, msg.converter_event.current_range, msg.converter_event.type, state, value);	
						// Update current setting too
						value = Converter_GetCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range);
						setGuiCurrentSetting(msg.converter_event.channel, msg.converter_event.current_range, value);
						break;
					case CURRENT_RANGE_CHANGE:
						value = Converter_GetCurrentRange(msg.converter_event.channel);
						setGuiCurrentRange(msg.converter_event.channel, value);
						break;
					case CHANNEL_CHANGE:
						value = Converter_GetFeedbackChannel();
						setGuiFeedbackChannel(value);
						break;
					case OVERLOAD_SETTING_CHANGE:
						state = Converter_GetOverloadProtectionState();
						state2 = Converter_GetOverloadProtectionWarning();
						value = Converter_GetOverloadProtectionThreshold();
						setGuiOverloadSettings(state, state2, value);
						break;
				}
				break;
			case GUI_TASK_UPDATE_VOLTAGE_CURRENT:
				setGuiVoltageIndicator(voltage_adc);
				setGuiCurrentIndicator(current_adc);
				setGuiPowerIndicator(power_adc);
				break;
			case GUI_TASK_UPDATE_TEMPERATURE_INDICATOR:
				setGuiTemperatureIndicator(converter_temp_celsius);
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
							
							// Update widgets
							value = Converter_GetFeedbackChannel();
							setGuiFeedbackChannel(value);
						}
						else
						{
							// Profile load failed
							if (msg.profile_event.err_code == EE_PROFILE_CRC_ERROR) 
								guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_CRC_ERROR, 0, 30);
							else
								guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_HW_ERROR, 0, 30);	
							// Update single record 
							profileState = readProfileListRecordName(msg.profile_event.index, profileName);
							setGuiProfileRecordState(msg.profile_event.index, profileState, profileName);
						}
						break;
					case PROFILE_SAVE:
						if (msg.profile_event.err_code == EE_PROFILE_VALID)
						{
							// Profile succesfully saved
							guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_SAVED, 0, 30);
							// Update single record 
							profileState = readProfileListRecordName(msg.profile_event.index, profileName);
							setGuiProfileRecordState(msg.profile_event.index, profileState, profileName);
						}
						else
						{
							// Profile save failed
							if (msg.profile_event.err_code == EE_PROFILE_CRC_ERROR) 
								guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_CRC_ERROR, 0, 30);
							else
								guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_HW_ERROR, 0, 30);
							// Update single record 
							profileState = readProfileListRecordName(msg.profile_event.index, profileName);
							setGuiProfileRecordState(msg.profile_event.index, profileState, profileName);
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
				
				
			case GUI_TASK_UPDATE_PROFILE_SETUP:
					// Read profile settings and update widgets
					guiTop_UpdateProfileSettings();
				break;
			
		}
	}
}







//=================================================================//
//=================================================================//
//                   Hardware control interface                    //
//						GUI -> converter						   //
//=================================================================//


//-------------------------------------------------------//
//	Voltage
void applyGuiVoltageSetting(uint8_t channel, int32_t new_set_voltage)
{
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_VOLTAGE;
	dispatcher_msg.converter_cmd.a.v_set.channel = channel;	
	dispatcher_msg.converter_cmd.a.v_set.value = new_set_voltage;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
}



//-------------------------------------------------------//
//	Current
void applyGuiCurrentSetting(uint8_t channel, uint8_t range, int32_t new_set_current)
{ 
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_CURRENT;
	dispatcher_msg.converter_cmd.a.c_set.channel = channel;	
	dispatcher_msg.converter_cmd.a.c_set.range = range;	
	dispatcher_msg.converter_cmd.a.c_set.value = new_set_current;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
}


void applyGuiCurrentRange(uint8_t channel, uint8_t new_range)
{
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_CURRENT_RANGE;
	dispatcher_msg.converter_cmd.a.crange_set.channel = channel;	
	dispatcher_msg.converter_cmd.a.crange_set.new_range = new_range;	
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
}















//===========================================================================//
//===========================================================================//
//===========================================================================//
//===========================================================================//



//------------------------------------------------------//
//              Voltage/Current limits                  //
//------------------------------------------------------//

void guiTop_ApplyGuiVoltageLimit(uint8_t channel, uint8_t limit_type, uint8_t enable, int16_t value)
{
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_VOLTAGE_LIMIT;
	dispatcher_msg.converter_cmd.a.vlim_set.channel = channel;
	dispatcher_msg.converter_cmd.a.vlim_set.type = limit_type;	
	dispatcher_msg.converter_cmd.a.vlim_set.enable = enable;
	dispatcher_msg.converter_cmd.a.vlim_set.value = value;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
    // TODO - should update widgets after processing command by converter - both limit and setting widgets
}

void guiTop_ApplyGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t limit_type, uint8_t enable, int16_t value)
{
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_CURRENT_LIMIT;
	dispatcher_msg.converter_cmd.a.clim_set.channel = channel;
	dispatcher_msg.converter_cmd.a.clim_set.range = currentRange;
	dispatcher_msg.converter_cmd.a.clim_set.type = limit_type;	
	dispatcher_msg.converter_cmd.a.clim_set.enable = enable;
	dispatcher_msg.converter_cmd.a.clim_set.value = value;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
    // TODO - should update widgets after processing command by converter - both limit and setting widgets
}

void guiTop_UpdateVoltageLimit(uint8_t channel, uint8_t limit_type)
{
	uint8_t isEnabled;
	uint16_t value;
	taskENTER_CRITICAL();
    isEnabled = Converter_GetVoltageLimitState(channel, limit_type);
    value = Converter_GetVoltageLimitSetting(channel, limit_type);
	taskEXIT_CRITICAL();
    setGuiVoltageLimitSetting(channel, limit_type, isEnabled, value);
}

void guiTop_UpdateCurrentLimit(uint8_t channel, uint8_t range, uint8_t limit_type)
{
	uint8_t isEnabled;
	uint16_t value;
	taskENTER_CRITICAL();
    isEnabled = Converter_GetCurrentLimitState(channel, range, limit_type);
    value = Converter_GetCurrentLimitSetting(channel, range, limit_type);
    taskEXIT_CRITICAL();
    setGuiCurrentLimitSetting(channel, range, limit_type, isEnabled, value);
}


//------------------------------------------------------//
//                  Overload                            //
//------------------------------------------------------//

void guiTop_ApplyGuiOverloadSettings(uint8_t protection_enable, uint8_t warning_enable, int32_t threshold)
{ 
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_OVERLOAD_PARAMS;
	dispatcher_msg.converter_cmd.a.overload_set.protection_enable = protection_enable;
	dispatcher_msg.converter_cmd.a.overload_set.warning_enable = warning_enable;
	dispatcher_msg.converter_cmd.a.overload_set.threshold = threshold;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);		
}

void guiTop_UpdateOverloadSettings(void)
{
	uint8_t protectionEnabled, warningEnabled;
	uint16_t threshold;
	taskENTER_CRITICAL();
    protectionEnabled = Converter_GetOverloadProtectionState();
    warningEnabled = Converter_GetOverloadProtectionWarning();
    threshold = Converter_GetOverloadProtectionThreshold();
    taskEXIT_CRITICAL();
    setGuiOverloadSettings(protectionEnabled, warningEnabled, threshold);
}



//------------------------------------------------------//
//                  Profiles                            //
//------------------------------------------------------//

void guiTop_ApplyGuiProfileSettings(uint8_t saveRecentProfile, uint8_t restoreRecentProfile)
{
	dispatcher_msg.type = DISPATCHER_PROFILE_SETUP;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.profile_setup.saveRecentProfile = saveRecentProfile;
	dispatcher_msg.profile_setup.restoreRecentProfile = restoreRecentProfile;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);
}

void guiTop_UpdateProfileSettings(void)
{
	uint8_t saveRecentProfile, restoreRecentProfile;
	taskENTER_CRITICAL();
	saveRecentProfile = EE_IsRecentProfileSavingEnabled();
    restoreRecentProfile = EE_IsRecentProfileRestoreEnabled();
	taskEXIT_CRITICAL();
    setGuiProfileSettings(saveRecentProfile, restoreRecentProfile);
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

void guiTop_UpdateExtSwitchSettings(void)
{
	uint8_t enable, inverse, mode;
    // Read settings
	taskENTER_CRITICAL();
    enable = BTN_IsExtSwitchEnabled();
    inverse = BTN_GetExtSwitchInversion();
    mode = BTN_GetExtSwitchMode();
	taskEXIT_CRITICAL();
    // Update widgets
    setGuiExtSwitchSettings(enable, inverse, mode);
}



//------------------------------------------------------//
//			DAC offset settings							//
//------------------------------------------------------//

// Applies GUI DAC offset settings to hardware
// Called by GUI low level
void guiTop_ApplyDacSettings(int8_t v_offset, int8_t c_low_offset, int8_t c_high_offset)
{
	dispatcher_msg.type = DISPATCHER_CONVERTER;
	dispatcher_msg.sender = sender_GUI;
	dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_DAC_PARAMS;
	dispatcher_msg.converter_cmd.a.dac_set.voltage_offset = v_offset;
	dispatcher_msg.converter_cmd.a.dac_set.current_low_offset = c_low_offset;
	dispatcher_msg.converter_cmd.a.dac_set.current_high_offset = c_high_offset;
	xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
}

// Reads DAC offset settings and updates GUI widgets
// Called from both GUI top and low levels
void guiTop_UpdateDacSettings(void)
{
	int8_t v_offset;
	int8_t c_low_offset;
	int8_t c_high_offset;
	// Using critical section because modifying task has higher priority
	taskENTER_CRITICAL();
	v_offset = Converter_GetVoltageDacOffset();
	c_low_offset = Converter_GetCurrentDacOffset(CURRENT_RANGE_LOW);
	c_high_offset = Converter_GetCurrentDacOffset(CURRENT_RANGE_HIGH);
	taskEXIT_CRITICAL();
	setGuiDacSettings(v_offset, c_low_offset, c_high_offset);
}
















