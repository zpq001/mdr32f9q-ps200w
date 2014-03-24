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
        updateGuiProfileListRecord(i, EE_PROFILE_VALID, profileName);
    }


    // EEPROM
    guiEvent.type = GUI_EVENT_EEPROM_MESSAGE;
    //guiEvent.spec = 0;  // EEPROM FAIL
    guiEvent.spec = 1;  // EEPROM OK
    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
    guiCore_ProcessMessageQueue();


    updateGuiVoltageSetting();

    updateGuiVoltageIndicator();
    updateGuiCurrentIndicator();
    guiUpdatePowerIndicator();
    guiUpdateTemperatureIndicator();
    guiUpdateChannelSetting();
    guiUpdateCurrentRange();


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


//-----------------------------------//
// Voltage

// Read ADC voltage and update GUI
void updateGuiVoltageIndicator(void)
{
    guiLogEvent("Reading voltage ADC");
    setGuiVoltageIndicator(voltage_adc);
}



// Voltage setting HW -> GUI
void updateGuiVoltageSetting(void)
{
    guiLogEvent("Reading voltage setting");
    setGuiVoltageSetting(channel, set_voltage);
}


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


// Voltage setting GUI -> HW
void applyGuiVoltageSetting(uint8_t channel, int16_t new_set_voltage)
{
    set_voltage = new_set_voltage;
    voltage_adc = set_voltage;

    //------ simulation of actual conveter work ------//
    setGuiVoltageIndicator(voltage_adc);
    setGuiVoltageSetting(channel,set_voltage);
}


// Voltage limit setting HW -> GUI
void updateGuiVoltageLimit(uint8_t channel, uint8_t limit_type)
{
    // TODO
}

// Voltage limit setting GUI -> HW
void applyGuiVoltageLimit(uint8_t channel, uint8_t limit_type, uint8_t enable, int16_t value)
{
    // TODO
}


//-----------------------------------//
// Current

// Read ADC current and update GUI
void updateGuiCurrentIndicator(void)
{
    guiLogEvent("Reading current ADC");
    setGuiCurrentIndicator(current_adc);
}


// Current setting GUI -> HW
void applyGuiCurrentSetting(uint8_t channel, uint8_t currentRange, int16_t new_set_current)
{
    guiLogEvent("Writing current setting");
    set_current = new_set_current;
    current_adc = set_current;

    //------ simulation of actual conveter work ------//
    setGuiCurrentSetting(channel,currentRange,set_current);
    setGuiCurrentIndicator(current_adc);
}


// Current limit setting HW -> GUI
void updateGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t limit_type)
{
    // TODO
}

// Current limit setting GUI -> HW
void applyGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t limit_type, uint8_t enable, int16_t value)
{
    // TODO
}




//-----------------------------------//
// Feedback channel

// Read selected feedback channel and update LCD
void guiUpdateChannelSetting(void)
{
    guiLogEvent("Reading selected feedback channel");
    setGuiFeedbackChannel(channel);
}

// Apply new selected feedback channel
/*void applyGuiChannelSetting(uint8_t new_channel)
{
    guiLogEvent("Writing selected feedback channel");
    channel = new_channel;

    // simulation of actual conveter work
    guiUpdateChannelSetting();
}*/


//-----------------------------------//
// Current range (20A / 40A)

// Read current limit and update LCD
void guiUpdateCurrentRange(void)
{
    guiLogEvent("Reading current range");
    setGuiCurrentRange(channel, current_range);
}

// Apply new selected feedback channel
void applyGuiCurrentRange(uint8_t channel, uint8_t new_current_range)
{
    guiLogEvent("Writing current range");
    current_range = new_current_range;

    //------ simulation of actual conveter work ------//
    guiUpdateCurrentRange();
}


//-----------------------------------//
// Power

// Read computed power and update LCD indicator
void guiUpdatePowerIndicator(void)
{
    guiLogEvent("Reading power ADC");
    setGuiPowerIndicator(power_adc);
}


//-----------------------------------//
// Temperature

// Read normalized temperature and update LCD indicator
void guiUpdateTemperatureIndicator(void)
{
    guiLogEvent("Reading temperature ADC");
    setGuiTemperatureIndicator(converter_temp_celsius);
}


//-----------------------------------//
// Other
void applyGuiOverloadSetting(uint8_t protectionEnable, uint8_t warningEnable, int32_t newThreshold)
{
}

void updateGuiOverloadSetting(void)
{

}


//---------------------------------------------//
// Requests for profile load
//---------------------------------------------//
void loadProfile(uint8_t index)
{
    if (index % 3 == 0)
    {
        guiMessagePanel1_Show(MESSAGE_TYPE_INFO, MESSAGE_PROFILE_LOADED, 0, 30);
    }
    else if (index % 3 == 1)
    {
        guiMessagePanel1_Show(MESSAGE_TYPE_WARNING, MESSAGE_PROFILE_CRC_ERROR, 0, 30);
    }
    else if (index % 3 == 2)
    {
        guiMessagePanel1_Show(MESSAGE_TYPE_ERROR, MESSAGE_PROFILE_HW_ERROR, 0, 30);
    }
}

//---------------------------------------------//
// Requests for profile save
//---------------------------------------------//
void saveProfile(uint8_t index, char *profileName)
{

}


//=================================================================//

