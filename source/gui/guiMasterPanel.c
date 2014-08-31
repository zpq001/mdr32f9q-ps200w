/**********************************************************
    Module guiMasterPanel




**********************************************************/


#include <stdio.h>      // due to printf
#include <stdint.h>

#include "guiFonts.h"
#include "guiGraphHAL.h"
#include "guiGraphPrimitives.h"
#include "guiGraphWidgets.h"
#include "guiImages.h"

#include "guiCore.h"
#include "guiEvents.h"
#include "guiWidgets.h"
#include "guiPanel.h"
#include "guiTextLabel.h"
#include "guiSpinBox.h"

// Other forms and panels
#include "guiMasterPanel.h"
#include "guiSetupPanel.h"
#include "guiEditPanel1.h"
#include "guiMessagePanel1.h"

#include "guiTop.h"


#ifdef _GUITESTPROJ_
#include "taps.h"
#else
#include "converter.h"
#include "eeprom.h"
#include "adc.h"
#include "service.h"
#endif  //_GUITESTPROJ_


masterViev_t masterView;


static uint8_t guiMasterPanel_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
static uint8_t guiMasterPanel_onFocusChanged(void *widget, guiEvent_t *event);
static uint8_t guiMasterPanel_onDraw(void *widget, guiEvent_t *event);

static uint8_t spinBox_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);

static uint8_t onSpinBoxDrawEvent(void *sender, guiEvent_t *event);
static uint8_t onSpinBoxValueChanged(void *sender, guiEvent_t *event);

static uint8_t onTextLabelDrawEvent(void *sender, guiEvent_t *event);
static uint8_t onTextLabelKeyEncoderEvent(void *sender, guiEvent_t *event);




//--------- Master panel  ---------//
guiPanel_t     guiMasterPanel;

//--------- Panel elements --------//
static guiTextLabel_t textLabel_voltage;        // Measured voltage
static guiTextLabel_t textLabel_current;        // Measured current
static guiTextLabel_t textLabel_power;          // Measured power
static guiTextLabel_t textLabel_temperature;    // Converter temperature
static guiTextLabel_t textLabel_channel;        // Feedback channel
static guiTextLabel_t textLabel_currRange;      // Current limit
static guiSpinBox_t spinBox_voltage;            // Voltage setting
static guiSpinBox_t spinBox_current;            // Current setting



//-------------------------------------------------------//
//  Panel initialization
//
//-------------------------------------------------------//
void guiMasterPanel_Initialize(guiGenericWidget_t *parent)
{
    // Initialize form
    guiPanel_Initialize(&guiMasterPanel, parent);
    guiCore_AllocateWidgetCollection((guiGenericContainer_t *)&guiMasterPanel, 20);
    guiMasterPanel.x = 0;
    guiMasterPanel.y = 0;
    guiMasterPanel.width = 96 * 2;
    guiMasterPanel.height = 68;
    guiMasterPanel.showFocus = 0;
    guiMasterPanel.focusFallsThrough = 0;
    guiMasterPanel.keyTranslator = &guiMasterPanel_KeyTranslator;
    guiCore_AllocateHandlers(&guiMasterPanel, 2);
    guiCore_AddHandler(&guiMasterPanel, GUI_ON_FOCUS_CHANGED, guiMasterPanel_onFocusChanged);
    guiCore_AddHandler(&guiMasterPanel, GUI_EVENT_DRAW, guiMasterPanel_onDraw);

    // Initialize text label for measured voltage display
    guiTextLabel_Initialize(&textLabel_voltage, (guiGenericWidget_t *)&guiMasterPanel);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_voltage, (guiGenericContainer_t *)&guiMasterPanel);
    textLabel_voltage.x = 1;
    textLabel_voltage.y = 0;
    textLabel_voltage.width = 94;
    textLabel_voltage.height = 32;
    textLabel_voltage.textAlignment = ALIGN_TOP_RIGHT;
    textLabel_voltage.text = guiCore_calloc(10);
    textLabel_voltage.font = &font_h32;

    // Initialize text label for measured current display
    guiTextLabel_Initialize(&textLabel_current, (guiGenericWidget_t *)&guiMasterPanel);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_current, (guiGenericContainer_t *)&guiMasterPanel);
    textLabel_current.x = 96 + 1;
    textLabel_current.y = 0;
    textLabel_current.width = 94;
    textLabel_current.height = 32;
    textLabel_current.textAlignment = ALIGN_TOP_RIGHT;
    textLabel_current.text = guiCore_calloc(10);
    textLabel_current.font = &font_h32;

    // Initialize text label for measured power display
    guiTextLabel_Initialize(&textLabel_power, (guiGenericWidget_t *)&guiMasterPanel);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_power, (guiGenericContainer_t *)&guiMasterPanel);
    textLabel_power.x = 96 + 43;
    textLabel_power.y = 57;
    textLabel_power.width = 52;
    textLabel_power.height = 11;
    textLabel_power.textAlignment = ALIGN_TOP_RIGHT;
    textLabel_power.text = guiCore_calloc(10);
    textLabel_power.font = &font_h11;

    // Initialize text label for temperature display
    guiTextLabel_Initialize(&textLabel_temperature, (guiGenericWidget_t *)&guiMasterPanel);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_temperature, (guiGenericContainer_t *)&guiMasterPanel);
    textLabel_temperature.x = 55;
    textLabel_temperature.y = 57;
    textLabel_temperature.width = 40;
    textLabel_temperature.height = 11;
    textLabel_temperature.textAlignment = ALIGN_TOP_LEFT;
    textLabel_temperature.text = guiCore_calloc(10);
    textLabel_temperature.font = &font_h11;

    // Initialize text label for feedback channel display
    guiTextLabel_Initialize(&textLabel_channel, (guiGenericWidget_t *)&guiMasterPanel);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_channel, (guiGenericContainer_t *)&guiMasterPanel);
    textLabel_channel.x = 1;
    textLabel_channel.y = 57;
    textLabel_channel.width = 45;
    textLabel_channel.height = 11;
    textLabel_channel.textAlignment = ALIGN_TOP_LEFT;
    textLabel_channel.text = guiCore_calloc(10);
    textLabel_channel.font = &font_h11;

    // Initialize text label for current limit display and control
    guiTextLabel_Initialize(&textLabel_currRange, (guiGenericWidget_t *)&guiMasterPanel);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_currRange, (guiGenericContainer_t *)&guiMasterPanel);
    textLabel_currRange.x = 96+1;
    textLabel_currRange.y = 57;
    textLabel_currRange.width = 25;
    textLabel_currRange.height = 11;
    textLabel_currRange.textAlignment = ALIGN_TOP_LEFT;
    textLabel_currRange.text = guiCore_calloc(10);
    textLabel_currRange.font = &font_h11;
    textLabel_currRange.acceptFocusByTab = 1;
    textLabel_currRange.tabIndex = 13;
    textLabel_currRange.showFocus = 0;
    guiCore_AllocateHandlers((guiGenericWidget_t *)&textLabel_currRange, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&textLabel_currRange, GUI_EVENT_DRAW, onTextLabelDrawEvent);
    guiCore_AddHandler((guiGenericWidget_t *)&textLabel_currRange, GUI_EVENT_KEY, onTextLabelKeyEncoderEvent);

    // Initialize spinbox for voltage setting
    guiSpinBox_Initialize(&spinBox_voltage, (guiGenericWidget_t *)&guiMasterPanel);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&spinBox_voltage, (guiGenericContainer_t *)&guiMasterPanel);
    spinBox_voltage.x = 30;
    spinBox_voltage.y = 33;
    spinBox_voltage.width = 45;
    spinBox_voltage.height = 21;
    spinBox_voltage.textRightOffset = 0;
    spinBox_voltage.textTopOffset = 1;
    spinBox_voltage.tabIndex = 11;
    spinBox_voltage.font = &font_h16;
    spinBox_voltage.dotPosition = 2;
    spinBox_voltage.activeDigit = 2;
    spinBox_voltage.minDigitsToDisplay = 3;
    spinBox_voltage.restoreValueOnEscape = 0;
    spinBox_voltage.maxValue = 4100;
    spinBox_voltage.minValue = -1;
    spinBox_voltage.showFocus = 0;
    spinBox_voltage.keyTranslator = &spinBox_KeyTranslator;
    guiCore_AllocateHandlers((guiGenericWidget_t *)&spinBox_voltage, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_voltage, GUI_EVENT_DRAW, onSpinBoxDrawEvent);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_voltage, SPINBOX_VALUE_CHANGED, onSpinBoxValueChanged);

    // Initialize spinbox for current setting
    guiSpinBox_Initialize(&spinBox_current, (guiGenericWidget_t *)&guiMasterPanel);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&spinBox_current, (guiGenericContainer_t *)&guiMasterPanel);
    spinBox_current.x = 96+30;
    spinBox_current.y = 33;
    spinBox_current.width = 45;
    spinBox_current.height = 21;
    spinBox_current.textRightOffset = 0;
    spinBox_current.textTopOffset = 1;
    spinBox_current.tabIndex = 12;
    spinBox_current.font = &font_h16;
    spinBox_current.dotPosition = 2;
    spinBox_current.activeDigit = 2;
    spinBox_current.minDigitsToDisplay = 3;
    spinBox_current.restoreValueOnEscape = 0;
    spinBox_current.maxValue = 4100;
    spinBox_current.minValue = -1;
    spinBox_current.showFocus = 0;
    spinBox_current.keyTranslator = &spinBox_KeyTranslator;
    spinBox_current.handlers.count = spinBox_voltage.handlers.count;        // handlers are shared
    spinBox_current.handlers.elements = spinBox_voltage.handlers.elements;


    // Add other widgets
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&guiEditPanel1, (guiGenericContainer_t *)&guiMasterPanel);
}



static uint8_t guiMasterPanel_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiPanelTranslatedKey_t *tkey = (guiPanelTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_LEFT)
            tkey->key = PANEL_KEY_PREV;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = PANEL_KEY_NEXT;
    }
    else if (event->spec == GUI_KEY_EVENT_UP_SHORT)
    {
        if (event->lparam == GUI_KEY_ENCODER)
        {
            // Hook encoder push
            if (spinBox_voltage.isFocused)
            {
                guiCore_RequestFocusChange((guiGenericWidget_t *)&spinBox_current);
                event->type = SPINBOX_EVENT_ACTIVATE;
                guiCore_AddMessageToQueue((guiGenericWidget_t *)&spinBox_current,event);   // activate
            }
            else
            {
                guiCore_RequestFocusChange((guiGenericWidget_t *)&spinBox_voltage);
                event->type = SPINBOX_EVENT_ACTIVATE;
                guiCore_AddMessageToQueue((guiGenericWidget_t *)&spinBox_voltage,event);   // activate
            }
            return GUI_EVENT_ACCEPTED;  // do not process this event any more
        }

    }
    else if (event->spec == GUI_KEY_EVENT_HOLD)
    {
        if (event->lparam == GUI_KEY_ENCODER)
        {
            showEditPanel1();           // edit panel may be not shown if focus has "wrong" widget
            return GUI_EVENT_ACCEPTED;  // do not process this event any more
        }
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        tkey->key = (int16_t)event->lparam < 0 ? PANEL_KEY_PREV :
              ((int16_t)event->lparam > 0 ? PANEL_KEY_NEXT : 0);
    }
    return 0;
}


static uint8_t guiMasterPanel_onFocusChanged(void *widget, guiEvent_t *event)
{
    if (guiMasterPanel.isFocused)
    {
        guiCore_RequestFocusNextWidget((guiGenericContainer_t *)&guiMasterPanel,1);
    }
    return 0;
}


static uint8_t guiMasterPanel_onDraw(void *widget, guiEvent_t *event)
{
    if (guiMasterPanel.redrawForced)
    {
        // Draw static elements
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_SetLineStyle(LINE_STYLE_SOLID);
        LCD_DrawHorLine(0, 55, 96*2, 1);
        LCD_DrawVertLine(48, 56, 13, 1);
        LCD_DrawVertLine(96+42, 56 , 13, 1);
        LCD_SetFont(&font_h11);
        LCD_PrintString("SET:", 2, 41, IMAGE_MODE_NORMAL);
        LCD_PrintString("V", 75, 41, IMAGE_MODE_NORMAL);
        LCD_PrintString("SET:", 96+2, 41, IMAGE_MODE_NORMAL);
        LCD_PrintString("A", 96+75, 41, IMAGE_MODE_NORMAL);
    }
    return 0;
}



//-------------------------//

static uint8_t spinBox_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiSpinboxTranslatedKey_t *tkey = (guiSpinboxTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    tkey->increment = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_OK)
            tkey->key = SPINBOX_KEY_SELECT;
        else if (event->lparam == GUI_KEY_ESC)
            tkey->key = SPINBOX_KEY_EXIT;
        else if (event->lparam == GUI_KEY_LEFT)
            tkey->key = SPINBOX_KEY_LEFT;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = SPINBOX_KEY_RIGHT;
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        if (((guiSpinBox_t *)widget)->isActive == 0)
        {
            event->type = SPINBOX_EVENT_ACTIVATE;
            guiCore_AddMessageToQueue(widget,event);   // activate
            return GUI_EVENT_ACCEPTED;
        }
        tkey->increment = (int16_t)event->lparam;
    }
    return 0;
}


static uint8_t onSpinBoxDrawEvent(void *sender, guiEvent_t *event)
{
    guiSpinBox_t *spinBox = (guiSpinBox_t *)sender;

    if (((spinBox->redrawFocus) || (spinBox->redrawForced)) && (spinBox == &spinBox_voltage))
    {
        if (spinBox->isFocused)
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_OR);
            LCD_DrawImage((uint8_t*)&selector_tri, 88, 39, 6, 12, IMAGE_MODE_NORMAL);
        }
        else
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_AND);
            LCD_DrawImage((uint8_t*)&selector_tri, 88, 39, 6, 12, IMAGE_MODE_INVERSE);
        }
    }
    if (((spinBox->redrawFocus) || (spinBox->redrawForced)) && (spinBox == &spinBox_current))
    {
        if (spinBox->isFocused)
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_OR);
            LCD_DrawImage((uint8_t*)&selector_tri, 96+88, 39, 6, 12, IMAGE_MODE_NORMAL);
        }
        else
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_AND);
            LCD_DrawImage((uint8_t*)&selector_tri, 96+88, 39, 6, 12, IMAGE_MODE_INVERSE);
        }
    }
    return 0;   // doesn't matter
}


static uint8_t onSpinBoxValueChanged(void *sender, guiEvent_t *event)
{
    guiSpinBox_t *spinBox = (guiSpinBox_t *)sender;
    if (spinBox == &spinBox_voltage)
    {
        guiTop_ApplyGuiVoltageSetting(masterView.channel, spinBox_voltage.value * 10);
    }
    else if (spinBox == &spinBox_current)
    {
        guiTop_ApplyCurrentSetting(masterView.channel, masterView.current_range, spinBox_current.value * 10);
    }
    return 0;   // doesn't matter
}


//-------------------------//


static uint8_t onTextLabelDrawEvent(void *sender, guiEvent_t *event)
{
    guiTextLabel_t *label = (guiTextLabel_t *)sender;
    if ((label->redrawFocus) && (label == &textLabel_currRange))
    {
        if (label->isFocused)
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_OR);
            LCD_DrawImage((uint8_t*)&selector_tri, 96 + 32, 56, 6, 12, IMAGE_MODE_NORMAL);
        }
        else
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_AND);
            LCD_DrawImage((uint8_t*)&selector_tri, 96 + 32, 56, 6, 12, IMAGE_MODE_INVERSE);
        }
    }
    return 0;   // doesn't matter
}


static uint8_t onTextLabelKeyEncoderEvent(void *sender, guiEvent_t *event)
{
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    if (event->spec == GUI_ENCODER_EVENT)
    {
        if ((int16_t)event->lparam < 0)
        {
            guiTop_ApplyCurrentRange(masterView.channel, CURRENT_RANGE_LOW);
        }
        else if ((int16_t)event->lparam > 0)
        {
            guiTop_ApplyCurrentRange(masterView.channel, CURRENT_RANGE_HIGH);
        }
    }
    else
    {
        processResult = GUI_EVENT_DECLINE;
    }
    return processResult;
}



//-------------------------//

void showEditPanel1(void)
{
    uint8_t show_panel = 0;
    if (spinBox_voltage.isFocused)
    {
        editView.mode = EDIT_MODE_VOLTAGE;
        editView.active_digit = spinBox_voltage.activeDigit;
        guiEditPanel1.x = 5;
        show_panel = 1;
    }
    else if (spinBox_current.isFocused)
    {
        editView.mode = EDIT_MODE_CURRENT;
        editView.active_digit = spinBox_current.activeDigit;
        guiEditPanel1.x = 96 + 5;
        show_panel = 1;
    }
    editView.channel = masterView.channel;
    editView.current_range = masterView.current_range;
    if (show_panel)
    {
        guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiEditPanel1, &guiEvent_SHOW);
        guiCore_RequestFocusChange((guiGenericWidget_t *)&guiEditPanel1);
    }
}

void hideEditPanel1(void)
{
    guiGenericWidget_t * widget;
    guiEvent_t event;
    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiEditPanel1, &guiEvent_HIDE);
    if (editView.mode == EDIT_MODE_VOLTAGE)
        widget = (guiGenericWidget_t *)&spinBox_voltage;
    else
        widget = (guiGenericWidget_t *)&spinBox_current;

    event.type = SPINBOX_EVENT_ACTIVATE;
    guiCore_RequestFocusChange(widget);
    guiCore_AddMessageToQueue(widget, &event);   // activate
}




//=================================================================//
//          Functions for widgets access
//
//=================================================================//


static void setGuiVoltageIndicator(uint16_t value)
{
    sprintf(textLabel_voltage.text, "%2.2fv", (float)value/1000);
    textLabel_voltage.redrawRequired = 1;
    textLabel_voltage.redrawForced = 1;
}

static void setGuiCurrentIndicator(uint16_t value)
{
    sprintf(textLabel_current.text, "%2.2fa", (float)value/1000);
    textLabel_current.redrawRequired = 1;
    textLabel_current.redrawForced = 1;
}

static void setGuiPowerIndicator(uint32_t value)
{
    sprintf(textLabel_power.text, "%3.2fW", (float)value/1000 );
    textLabel_power.redrawRequired = 1;
    textLabel_power.redrawForced = 1;
}

static void setGuiTemperatureIndicator(int16_t value)
{
    sprintf(textLabel_temperature.text, "%2d%cC", value, 0xb0);
    textLabel_temperature.redrawRequired = 1;
    textLabel_temperature.redrawForced = 1;
}

static void setGuiVoltageSetting(int32_t value)
{
    guiSpinBox_SetValue(&spinBox_voltage, value/10, 0);     // do not call handler
}

static void setGuiCurrentSetting(int32_t value)
{
    guiSpinBox_SetValue(&spinBox_current, value/10, 0);     // do not call handler
}

static void setGuiCurrentRange(uint8_t range)
{
    if (range == CURRENT_RANGE_LOW)
        guiTextLabel_SetText(&textLabel_currRange, "20A");
    else
        guiTextLabel_SetText(&textLabel_currRange, "40A");
}

static void setGuiFeedbackChannel(uint8_t channel)
{
    if (channel == CHANNEL_5V)
        guiTextLabel_SetText(&textLabel_channel, "Ch.5V");
    else
        guiTextLabel_SetText(&textLabel_channel, "Ch.12V");
}


//==========================================================================//
//==========================================================================//
//                    The NEW interface to TOP level                        //
//                                                                          //
//==========================================================================//

void guiUpdateVoltageSetting(uint8_t channel)
{
    int32_t value;
    if (channel == masterView.channel)
    {
        taskENTER_CRITICAL();
        value = Converter_GetVoltageSetting(masterView.channel);
        taskEXIT_CRITICAL();
        setGuiVoltageSetting(value);
    }
}

void guiUpdateCurrentSetting(uint8_t channel, uint8_t current_range)
{
    int32_t value;
    if ((channel == masterView.channel) && (current_range == masterView.current_range))
    {
        taskENTER_CRITICAL();
        value = Converter_GetCurrentSetting(masterView.channel, masterView.current_range);
        taskEXIT_CRITICAL();
        setGuiCurrentSetting(value);
    }
}

void guiUpdateCurrentRange(uint8_t channel)
{
    int32_t value;
    if (channel == masterView.channel)
    {
        taskENTER_CRITICAL();
        masterView.current_range = Converter_GetCurrentRange(masterView.channel);
        value = Converter_GetCurrentSetting(masterView.channel, masterView.current_range);
        taskEXIT_CRITICAL();
        setGuiCurrentRange(masterView.current_range);
        setGuiCurrentSetting(value);
        // Hide edit panel
        if (guiEditPanel1.isVisible)
            hideEditPanel1();
    }
}

void guiUpdateChannel(void)
{
    int32_t value1, value2;
    taskENTER_CRITICAL();
    masterView.channel = Converter_GetFeedbackChannel();
    masterView.current_range = Converter_GetCurrentRange(masterView.channel);
    value1 = Converter_GetVoltageSetting(masterView.channel);
    value2 = Converter_GetCurrentSetting(masterView.channel, masterView.current_range);
    taskEXIT_CRITICAL();
    setGuiFeedbackChannel(masterView.channel);
    setGuiCurrentRange(masterView.current_range);
    setGuiVoltageSetting(value1);
    setGuiCurrentSetting(value2);
    // Hide edit panel
    if (guiEditPanel1.isVisible)
        hideEditPanel1();
}

void guiUpdateAdcIndicators(void)
{
    taskENTER_CRITICAL();
    setGuiVoltageIndicator( ADC_GetVoltage() );
    setGuiCurrentIndicator( ADC_GetCurrent() );
    setGuiPowerIndicator( ADC_GetPower() );
    taskEXIT_CRITICAL();
}

void guiUpdateTemperatureIndicator(void)
{
    taskENTER_CRITICAL();
    setGuiTemperatureIndicator( Service_GetTemperature() );
    taskEXIT_CRITICAL();
}







