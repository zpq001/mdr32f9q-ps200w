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
    // EEPROM
    guiEvent.type = GUI_EVENT_EEPROM_MESSAGE;
    //guiEvent.spec = 0;  // EEPROM FAIL
    guiEvent.spec = 1;  // EEPROM OK
    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMainForm, &guiEvent);
    guiCore_ProcessMessageQueue();


    guiUpdateVoltageSetting();
    guiUpdateCurrentSetting();

    guiUpdateVoltageIndicator();
    guiUpdateCurrentIndicator();
    guiUpdatePowerIndicator();
    guiUpdateTemperatureIndicator();
    guiUpdateChannelSetting();
    guiUpdateCurrentRange();


    // Parser test
    //char *parse_strings[] = {
    //    "key",
    //    "down",
    //    "btn_esc"
    //};
    //uart_parse(parse_strings, 3);

}



void guiDrawAll(void)
{
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

// Read ADC voltage and update LCD indicator
void guiUpdateVoltageIndicator(void)
{
    guiLogEvent("Reading voltage ADC");
    setVoltageIndicator(voltage_adc);
}

// Read voltage setting and update LCD indicator
void guiUpdateVoltageSetting(void)
{
    guiLogEvent("Reading voltage setting");
    setVoltageSetting(set_voltage);
}

// Apply voltage setting from GUI
void applyGuiVoltageSetting(int16_t new_set_voltage)
{
    guiLogEvent("Writing voltage setting");
    set_voltage = new_set_voltage;
    voltage_adc = set_voltage;

    //------ simulation of actual conveter work ------//
    guiUpdateVoltageIndicator();

/*
    conveter_message_t msg;
    const conveter_message_t converter_tick_message = 	{25};
    msg.type = 1;
    msg.data.a = 12;
    msg.data.b = 14;


    msg.voltage_limit_setting.limit = 0;
    msg.voltage_limit_setting.enable = 1;
    msg.voltage_limit_setting.value = 20000; */
}

// Apply voltage limit setting from GUI
void applyGuiVoltageLimit(uint8_t type, uint8_t enable, int16_t value)
{
    if (type == 0)
    {
        guiLogEvent("Writing LOW voltage limit ");
        setLowVoltageLimitSetting(enable, value);
    }
    else
    {
        guiLogEvent("Writing HIGH voltage limit ");
        setHighVoltageLimitSetting(enable, value);
    }
}

//-----------------------------------//
// Current

// Read ADC current and update LCD indicator
void guiUpdateCurrentIndicator(void)
{
    guiLogEvent("Reading current ADC");
    setCurrentIndicator(current_adc);
}

// Read current setting and update LCD indicator
void guiUpdateCurrentSetting(void)
{
    guiLogEvent("Reading current setting");
    setCurrentSetting(set_current);
}

// Apply current setting from GUI
void applyGuiCurrentSetting(int16_t new_set_current)
{
    guiLogEvent("Writing current setting");
    set_current = new_set_current;
    current_adc = set_current;

    //------ simulation of actual conveter work ------//
    guiUpdateCurrentIndicator();
}


//-----------------------------------//
// Feedback channel

// Read selected feedback channel and update LCD
void guiUpdateChannelSetting(void)
{
    guiLogEvent("Reading selected feedback channel");
    setFeedbackChannelIndicator(channel);
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
    setCurrentRangeIndicator(current_range);
}

// Apply new selected feedback channel
void applyGuiCurrentRange(uint8_t new_current_range)
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
    setPowerIndicator(power_adc);
}


//-----------------------------------//
// Temperature

// Read normalized temperature and update LCD indicator
void guiUpdateTemperatureIndicator(void)
{
    guiLogEvent("Reading temperature ADC");
    setTemperatureIndicator(converter_temp_celsius);
}

//=================================================================//

