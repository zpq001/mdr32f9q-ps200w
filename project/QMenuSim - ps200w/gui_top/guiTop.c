#include <stdio.h>

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

// UART parser test
#include "taps.h"

// Callback functions
cbLogPtr addLogCallback;
cbLcdUpdatePtr updateLcdCallback;

// Temporary display buffers, used for splitting GUI buffer into two separate LCD's
uint8_t lcd0_buffer[DISPLAY_BUFFER_SIZE];
uint8_t lcd1_buffer[DISPLAY_BUFFER_SIZE];


uint8_t gui_started = 0;

guiEvent_t guiEvent;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Hardware state variables

converter_state_t converter_state;
uint8_t ee_saveRecentProfile = 0, ee_restoreRecentProfile = 1;
uint8_t extsw_enable = 1;
uint8_t extsw_inverse = 0;
uint8_t extsw_mode = EXTSW_DIRECT;
int8_t voltage_dac_offset = -5;
int8_t current_low_dac_offset = -10;
int8_t current_high_dac_offset = 20;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//



//-----------------------------------//
// Callbacks top->GUI
void registerLogCallback(cbLogPtr fptr)
{
    addLogCallback = fptr;
}

void registerLcdUpdateCallback(cbLcdUpdatePtr fptr)
{
    updateLcdCallback = fptr;
}

//-----------------------------------//
// Callbacks GUI->top
void guiLogEvent(char *string)
{
    addLogCallback(LOG_FROM_BOTTOM, string);
}




//-----------------------------------//
// Splitting GUI buffer into two separate LCD's and
// updating displays
//-----------------------------------//
static void guiDrawIsComplete(void)
{
    int i,j;
    int lcd_buf_index;
    int gui_buf_index;
    int num_pages = LCD_YSIZE / 8;
    if (LCD_YSIZE % 8) num_pages++;

    addLogCallback(LOG_FROM_BOTTOM, "GUI redraw completed!");

    // Split whole GUI buffer into two separate buffers per LCD
    lcd_buf_index = 0;
    gui_buf_index = 0;
    for (j=0; j<num_pages; j++)
    {
        for (i=0; i<DISPLAY_XSIZE; i++)
        {
            lcd0_buffer[lcd_buf_index] = (uint8_t)lcdBuffer[gui_buf_index];
            lcd1_buffer[lcd_buf_index] = (uint8_t)lcdBuffer[gui_buf_index + DISPLAY_XSIZE];
            lcd_buf_index++;
            gui_buf_index++;
        }
        gui_buf_index += DISPLAY_XSIZE;
    }

    updateLcdCallback(0,lcd0_buffer);
    updateLcdCallback(1,lcd1_buffer);
}



//-----------------------------------//
// Commands to GUI


void guiInitialize(void)
{
    // Initial state
    converter_state.channel = CHANNEL_12V;
    converter_state.current_range = CURRENT_RANGE_LOW;
    converter_state.set_voltage = 16000;
    converter_state.set_current = 4000;
    converter_state.converter_temp_celsius = 25;
    converter_state.power_adc = 0;
    converter_state.vdac_offset = -5;
    converter_state.cdac_offset = 20;
    converter_state.overload_prot_en = 1;
    converter_state.overload_warn_en = 0;
    converter_state.overload_threshold = 4;     // 2x [100 us]

    guiMainForm_Initialize();
    guiCore_Init((guiGenericWidget_t *)&guiMainForm);

    // Initial master panel update
    guiUpdateChannel();     // updates all master panel elements

    // Initial setup panel update
    // Voltage and current limit update is not required - these widgets will get proper values on show
    guiUpdateOverloadSettings();
    guiUpdateProfileSettings();
    guiUpdateExtswitchSettings();
    guiUpdateDacSettings();
    guiUpdateProfileList();

    // Simulation of ADC and service tasks work
    guiUpdateAdcIndicators();
    guiUpdateTemperatureIndicator();

    guiUpdateUartSettings(0);
}



void guiDrawAll(void)
{
    guiCore_ProcessTimers();
    guiCore_RedrawAll();
    guiDrawIsComplete();
}


//~~~~~~~ No touch support


void guiButtonEvent(uint16_t buttonCode, uint8_t eventType)
{
    if (gui_started)
    {
        guiCore_ProcessKeyEvent(buttonCode, eventType);
    }
    else
    {
        gui_started = 1;
        guiEvent.type = GUI_EVENT_START;
        guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
        guiCore_ProcessMessageQueue();
    }
}


void guiEncoderRotated(int32_t delta)
{
    addLogCallback(LOG_FROM_TOP, "Generated encoder event");
    guiCore_ProcessEncoderEvent((int16_t) delta);
}





//=================================================================//
//=================================================================//
//                      Hardware emulation interface               //
//=================================================================//


//---------------------------------------------//
// ADC functions emulation
//---------------------------------------------//
uint16_t ADC_GetVoltage(void)
{
    return converter_state.set_voltage;
}

uint16_t ADC_GetCurrent(void)
{
    return converter_state.set_current;
}

uint32_t ADC_GetPower(void)
{
    return 0;
}


//---------------------------------------------//
// Service functions emulation
//---------------------------------------------//
int16_t Service_GetTemperature(void)
{
    return converter_state.converter_temp_celsius;
}



//---------------------------------------------//
// Button functions emulation
//---------------------------------------------//
uint8_t BTN_GetExtSwitchMode(void)
{
    return extsw_mode;
}

uint8_t BTN_GetExtSwitchInversion(void)
{
    return extsw_inverse;
}

uint8_t BTN_IsExtSwitchEnabled(void)
{
    return extsw_enable;
}

//---------------------------------------------//
// EEPROM functions emulation
//---------------------------------------------//
uint8_t EE_IsRecentProfileSavingEnabled(void)
{
    return ee_saveRecentProfile;
}

uint8_t EE_IsRecentProfileRestoreEnabled(void)
{
    return ee_restoreRecentProfile;
}




//---------------------------------------------//
// Emulation of converter task
//---------------------------------------------//

uint16_t Converter_GetVoltageSetting(uint8_t channel)
{
    return converter_state.set_voltage;
}

uint16_t Converter_GetVoltageAbsMax(uint8_t channel)
{
    return 20000;
}

uint16_t Converter_GetVoltageAbsMin(uint8_t channel)
{
    return 0;
}

uint16_t Converter_GetVoltageLimitSetting(uint8_t channel, uint8_t limit_type)
{
    return 0;
}

uint8_t Converter_GetVoltageLimitState(uint8_t channel, uint8_t limit_type)
{
    return 0;
}

uint16_t Converter_GetCurrentSetting(uint8_t channel, uint8_t range)
{
    return converter_state.set_current;
}

uint16_t Converter_GetCurrentAbsMax(uint8_t channel, uint8_t range)
{
    return 40000;
}

uint16_t Converter_GetCurrentAbsMin(uint8_t channel, uint8_t range)
{
    return 0;
}

uint16_t Converter_GetCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    return 0;
}

uint8_t Converter_GetCurrentLimitState(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    return 0;
}

uint8_t Converter_GetOverloadProtectionState(void)
{
    return converter_state.overload_prot_en;
}

uint8_t Converter_GetOverloadProtectionWarning(void)
{
    return converter_state.overload_warn_en;
}

uint16_t Converter_GetOverloadProtectionThreshold(void)
{
    return converter_state.overload_threshold;
}

uint8_t Converter_GetCurrentRange(uint8_t channel)
{
    return converter_state.current_range;
}

uint8_t Converter_GetFeedbackChannel(void)
{
    return converter_state.channel;
}


int8_t Converter_GetVoltageDacOffset(void)
{
    return converter_state.vdac_offset;
}

int8_t Converter_GetCurrentDacOffset(uint8_t range)
{
    return converter_state.cdac_offset;
}

//---------------------------------------------//




//===========================================================================//
//===========================================================================//
//===========================================================================//
//===========================================================================//



//------------------------------------------------------//
//                  Voltage setting                     //
//------------------------------------------------------//

void guiTop_ApplyGuiVoltageSetting(uint8_t channel, int16_t new_set_voltage)
{
    converter_state.set_voltage = new_set_voltage;

    //------ simulation of actual conveter work ------//
    guiUpdateAdcIndicators();
    // Converter is done. Update voltage
    guiUpdateVoltageSetting(channel);
}



//------------------------------------------------------//
//                  Current setting                     //
//------------------------------------------------------//

// Current setting GUI -> HW
void guiTop_ApplyCurrentSetting(uint8_t channel, uint8_t currentRange, int16_t new_set_current)
{
    converter_state.set_current = new_set_current;

    //------ simulation of actual conveter work ------//
    guiUpdateAdcIndicators();
    // Converter is done. Update current
    guiUpdateCurrentSetting(channel, currentRange);
}



//------------------------------------------------------//
//              Voltage/Current limits                  //
//------------------------------------------------------//

void guiTop_ApplyGuiVoltageLimit(uint8_t channel, uint8_t limit_type, uint8_t enable, int16_t value)
{
    // TODO
    // Should update widgets after processing command by converter - both limit and setting widgets
}

void guiTop_ApplyGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t limit_type, uint8_t enable, int16_t value)
{
    // TODO
    // Should update widgets after processing command by converter - both limit and setting widgets
}




//------------------------------------------------------//
//                  Current range                       //
//------------------------------------------------------//

void guiTop_ApplyCurrentRange(uint8_t channel, uint8_t new_current_range)
{
    converter_state.current_range = new_current_range;

    //------ simulation of actual conveter work ------//
    guiUpdateCurrentRange(channel);
}



//------------------------------------------------------//
//                  Overload                            //
//------------------------------------------------------//
uint8_t ovld_protectionEnable, ovld_warningEnable;
int32_t ovld_threshold;

void guiTop_ApplyGuiOverloadSettings(uint8_t protectionEnable, uint8_t warningEnable, int32_t newThreshold)
{
    ovld_protectionEnable = protectionEnable;
    ovld_warningEnable = warningEnable;
    ovld_threshold = newThreshold;
}




//------------------------------------------------------//
//                  Profiles                            //
//------------------------------------------------------//

void guiTop_ApplyGuiProfileSettings(uint8_t saveRecentProfile, uint8_t restoreRecentProfile)
{
    ee_saveRecentProfile = saveRecentProfile;
    ee_restoreRecentProfile = restoreRecentProfile;
}


// Blockinbg read from EEPROM task
uint8_t readProfileListRecordName(uint8_t index, char *profileName)
{
    sprintf(profileName, "Profile %d", index);
    return EE_PROFILE_VALID;
}

// Profile load
void guiTop_LoadProfile(uint8_t index)
{
    if (index % 5 == 0)
    {
        guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_LOADED, 0, 30);
    }
    else if (index % 5 == 1)
    {
        guiMessagePanel1_Show(MESSAGE_TYPE_WARNING, MESSAGE_PROFILE_CRC_ERROR, 0, 30);
    }
    else if (index % 5 == 2)
    {
        guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_HW_ERROR, 0, 30);
    }
    else if (index % 5 == 3)
    {
        guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_LOADED_DEFAULT, 0, 30);
    }
    else if (index % 5 == 4)
    {
        guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_RESTORED_RECENT, 0, 30);
    }

    // Update all widgets that display profile data
    guiUpdateChannel();
    guiUpdateVoltageLimit(CHANNEL_AUTO, UPDATE_LOW_LIMIT | UPDATE_HIGH_LIMIT);
    guiUpdateCurrentLimit(CHANNEL_AUTO, CURRENT_RANGE_AUTO, UPDATE_LOW_LIMIT | UPDATE_HIGH_LIMIT);
    guiUpdateOverloadSettings();
    guiUpdateExtswitchSettings();
}


// Profile save
void guiTop_SaveProfile(uint8_t index, char *profileName)
{
    guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_SAVED, 0, 30);
}




//------------------------------------------------------//
//			External switch settings					//
//------------------------------------------------------//

void guiTop_ApplyExtSwitchSettings(uint8_t enable, uint8_t inverse, uint8_t mode)
{
    extsw_enable = enable;
    extsw_inverse = inverse;
    extsw_mode = mode;
}


//------------------------------------------------------//
//			DAC offset settings							//
//------------------------------------------------------//

void guiTop_ApplyDacSettings(int8_t v_offset, int8_t c_low_offset, int8_t c_high_offset)
{
    voltage_dac_offset = v_offset;
    current_low_dac_offset = c_low_offset;
    current_high_dac_offset = c_high_offset;
}



//===========================================================================//
//===========================================================================//
//===========================================================================//
//===========================================================================//



//------------------------------------------------------//
//                          UART                        //
//------------------------------------------------------//
void guiTop_ApplyUartSettings(reqUartSettings_t *s)
{

}


void guiTop_GetUartSettings(reqUartSettings_t *req)
{
    if (req->uart_num == 1)
    {
        req->enable = 1;
        req->parity = PARITY_ODD;
        req->brate = 19200;
    }
    else
    {
        req->enable = 0;
        req->parity = PARITY_EVEN;
        req->brate = 115200;
    }
}






