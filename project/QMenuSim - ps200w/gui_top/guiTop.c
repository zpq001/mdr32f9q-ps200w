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
#include "eeprom.h"

// UART parser test
#include "uartParser.h"

// Callback functions
cbLogPtr addLogCallback;
cbLcdUpdatePtr updateLcdCallback;

// Temporary display buffers, used for splitting GUI buffer into two separate LCD's
uint8_t lcd0_buffer[DISPLAY_BUFFER_SIZE];
uint8_t lcd1_buffer[DISPLAY_BUFFER_SIZE];


uint8_t timeHours;
uint8_t timeMinutes;
uint8_t timeSeconds;

uint8_t gui_started = 0;

guiEvent_t guiEvent;


//=================================================================//
//=================================================================//
//                      Hardware emulation interface               //
//=================================================================//
uint16_t voltage_adc;		// [mV]
uint16_t set_voltage;
uint16_t current_adc;		// [mA]
uint16_t set_current;
uint32_t power_adc;			// [mW]
int16_t converter_temp_celsius;

uint8_t channel;            // feedback channel
uint8_t current_range;      // converter max current (20A/40A)
//=================================================================//





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


void guiUpdateTime(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    timeHours = hours;
    timeMinutes = minutes;
    timeSeconds = seconds;
}


void guiInitialize(void)
{
    timeHours = 0;
    timeMinutes = 0;
    timeSeconds = 0;
    uint8_t i;
    char profileName[50];

    set_voltage = 10000;        // mV
    voltage_adc = set_voltage;
    set_current = 2000;         // mA
    current_adc =  set_current;
    power_adc =       0;        // mW
    converter_temp_celsius = 25;        // Celsius
    current_range = GUI_CURRENT_RANGE_LOW;
    channel = GUI_CHANNEL_12V;

    guiMainForm_Initialize();
    guiCore_Init((guiGenericWidget_t *)&guiMainForm);
    // Profile list init
    for (i=0; i < EE_PROFILES_COUNT; i++)
    {
        sprintf(profileName, "Profile %d", i);
        setGuiProfileRecordState(i, EE_PROFILE_VALID, profileName);
    }


    // EEPROM
    guiEvent.type = GUI_EVENT_EEPROM_MESSAGE;
    //guiEvent.spec = 0;  // EEPROM FAIL
    guiEvent.spec = 1;  // EEPROM OK
    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
    guiCore_ProcessMessageQueue();


    guiTop_UpdateVoltageSetting(channel);

    guiTop_UpdateGuiVoltageIndicator();
    guiTop_UpdateCurrentIndicator();
    guiTop_UpdatePowerIndicator();
    guiTop_UpdateTemperatureIndicator();
    guiUpdateChannelSetting();
    guiTop_UpdateCurrentRange(channel);


    // Parser test
//    char *parse_strings[] = {
//        "key",
//        "down",
//        "btn_esc"
//    };
//    uart_parse(parse_strings, 3);

}



void guiDrawAll(void)
{
    guiCore_ProcessTimers();
    //addLogCallback(LOG_FROM_TOP, "Redrawing GUI");
    guiCore_RedrawAll();
    // Update display(s)
    guiDrawIsComplete();
}


// No touch support


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
// NEW HW interface
uint16_t getVoltageSetting(uint8_t channel)
{
    return set_voltage;
}

uint16_t getVoltageAbsMax(uint8_t channel)
{
    return 2000;
}

uint16_t getVoltageAbsMin(uint8_t channel)
{
    return 0;
}

uint16_t getVoltageLimitSetting(uint8_t channel, uint8_t limit_type)
{
    return 0;
}

uint8_t getVoltageLimitState(uint8_t channel, uint8_t limit_type)
{
    return 0;
}

uint16_t getCurrentSetting(uint8_t channel, uint8_t range)
{
    return set_current;
}

uint16_t getCurrentAbsMax(uint8_t channel, uint8_t range)
{
    return 4000;
}

uint16_t getCurrentAbsMin(uint8_t channel, uint8_t range)
{
    return 0;
}

uint16_t getCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    return 0;
}

uint8_t getCurrentLimitState(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    return 0;
}

uint8_t getOverloadProtectionState(void)
{
    return 0;
}

uint8_t getOverloadProtectionWarning(void)
{
    return 0;
}

uint16_t getOverloadProtectionThreshold(void)
{
    return 0;
}

uint8_t getCurrentRange(uint8_t channel)
{
    return 0;
}


//---------------------------------------------//
// Button functions emulation
//---------------------------------------------//
uint8_t BTN_GetExtSwitchMode(void)
{
    return EXTSW_TOGGLE;
}

uint8_t BTN_GetExtSwitchInversion(void)
{
    return 0;
}

uint8_t BTN_IsExtSwitchEnabled(void)
{
    return 1;
}



//---------------------------------------------//


//guiLogEvent("Reading voltage ADC");






//-----------------------------------//
// Current










//-----------------------------------//
// Feedback channel

// Read selected feedback channel and update LCD
void guiUpdateChannelSetting(void)
{
    guiLogEvent("Reading selected feedback channel");
    setGuiFeedbackChannel(channel);
}






//===========================================================================//
//===========================================================================//
//===========================================================================//
//===========================================================================//


//------------------------------------------------------//
//                  Indicators 		                    //
//------------------------------------------------------//
void guiTop_UpdateGuiVoltageIndicator(void)
{
    setGuiVoltageIndicator(voltage_adc);
}

void guiTop_UpdateCurrentIndicator(void)
{
    setGuiCurrentIndicator(current_adc);
}

void guiTop_UpdatePowerIndicator(void)
{
    setGuiPowerIndicator(power_adc);
}

void guiTop_UpdateTemperatureIndicator(void)
{
    setGuiTemperatureIndicator(converter_temp_celsius);
}




//------------------------------------------------------//
//                  Voltage setting                     //
//------------------------------------------------------//

void guiTop_ApplyGuiVoltageSetting(uint8_t channel, int16_t new_set_voltage)
{
    set_voltage = new_set_voltage;
    voltage_adc = set_voltage;

    //------ simulation of actual conveter work ------//
    guiTop_UpdateGuiVoltageIndicator();
}

void guiTop_UpdateVoltageSetting(uint8_t channel)
{
    setGuiVoltageSetting(channel, set_voltage);
}




//------------------------------------------------------//
//                  Current setting                     //
//------------------------------------------------------//

// Current setting GUI -> HW
void guiTop_ApplyCurrentSetting(uint8_t channel, uint8_t currentRange, int16_t new_set_current)
{
    set_current = new_set_current;
    current_adc = set_current;

    //------ simulation of actual conveter work ------//
    guiTop_UpdateCurrentIndicator();
}

void guiTop_UpdateCurrentSetting(uint8_t channel, uint8_t currentRange)
{
    setGuiCurrentSetting(channel, currentRange, set_voltage);
}



//------------------------------------------------------//
//              Voltage/Current limits                  //
//------------------------------------------------------//
//struct limSet_t {
//    uint8_t enabled;
//    int32_t value;
//} vlim_low, vlim_high, clim_low, clim_high;

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

void guiTop_UpdateVoltageLimit(uint8_t channel, uint8_t limit_type)
{
    setGuiVoltageLimitSetting(channel, limit_type, 0, 0);
}

void guiTop_UpdateCurrentLimit(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    setGuiCurrentLimitSetting(channel, range, limit_type, 0, 0);
}



//------------------------------------------------------//
//                  Current range                       //
//------------------------------------------------------//

void guiTop_ApplyCurrentRange(uint8_t channel, uint8_t new_current_range)
{
    current_range = new_current_range;
    //------ simulation of actual conveter work ------//
    guiTop_UpdateCurrentRange(channel);
}

void guiTop_UpdateCurrentRange(uint8_t channel)
{
    setGuiCurrentRange(channel, current_range);
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

void guiTop_UpdateOverloadSettings(void)
{
    setGuiOverloadSettings(ovld_protectionEnable, ovld_warningEnable, ovld_threshold);
}



//------------------------------------------------------//
//                  Profiles                            //
//------------------------------------------------------//
uint8_t ee_saveRecentProfile, ee_restoreRecentProfile;

void guiTop_ApplyGuiProfileSettings(uint8_t saveRecentProfile, uint8_t restoreRecentProfile)
{
    ee_saveRecentProfile = saveRecentProfile;
    ee_restoreRecentProfile = restoreRecentProfile;
}

void guiTop_UpdateProfileSettings(void)
{
    setGuiProfileSettings(ee_saveRecentProfile, ee_restoreRecentProfile);
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
}


// Profile save
void guiTop_SaveProfile(uint8_t index, char *profileName)
{

}




//------------------------------------------------------//
//			External switch settings					//
//------------------------------------------------------//
uint8_t extsw_enable = 1;
uint8_t extsw_inverse = 1;
uint8_t extsw_mode = EXTSW_DIRECT;

void guiTop_ApplyExtSwitchSettings(uint8_t enable, uint8_t inverse, uint8_t mode)
{
    extsw_enable = enable;
    extsw_inverse = inverse;
    extsw_mode = mode;
}

void guiTop_UpdateExtSwitchSettings(void)
{
    setGuiExtSwitchSettings(extsw_enable, extsw_inverse, extsw_mode);
}



//------------------------------------------------------//
//			DAC offset settings							//
//------------------------------------------------------//
int8_t voltage_dac_offset = -5;
int8_t current_low_dac_offset = -10;
int8_t current_high_dac_offset = 20;

// Applies GUI DAC offset settings to hardware
// Called by GUI low level
void guiTop_ApplyDacSettings(int8_t v_offset, int8_t c_low_offset, int8_t c_high_offset)
{
    voltage_dac_offset = v_offset;
    current_low_dac_offset = c_low_offset;
    current_high_dac_offset = c_high_offset;
}

// Reads DAC offset settings and updates GUI widgets
// Called from both GUI top and low levels
void guiTop_UpdateDacSettings(void)
{
    setGuiDacSettings(voltage_dac_offset, current_low_dac_offset, current_high_dac_offset);
}






