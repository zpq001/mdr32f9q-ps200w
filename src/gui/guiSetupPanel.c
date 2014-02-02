/**********************************************************
    Module guiSetupPanel




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
#include "guiStringList.h"
#include "guiCheckBox.h"

// Other forms and panels - in order to switch between them
#include "guiMasterPanel.h"
#include "guiSetupPanel.h"

#include "guiTop.h"



static uint8_t guiSetupPanel_ProcessEvents(struct guiGenericWidget_t *widget, guiEvent_t event);
static uint8_t guiSetupList_onIndexChanged(void *widget, guiEvent_t *event);
static uint8_t guiSetupList_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);

static uint8_t onLowVoltageLimitChanged(void *widget, guiEvent_t *event);
static uint8_t onHighVoltageLimitChanged(void *widget, guiEvent_t *event);




//--------- Setup panel  ----------//
#define SETUP_PANEL_ELEMENTS_COUNT 8
guiPanel_t     guiSetupPanel;
static void *guiSetupPanelElements[SETUP_PANEL_ELEMENTS_COUNT];


//--------- Panel elements ---------//
guiStringList_t setupList;
#define SETUP_LIST_ELEMENTS_COUNT 6
char *setupListElements[SETUP_LIST_ELEMENTS_COUNT];
guiWidgetHandler_t setupListHandlers[1];

// Voltage limit section
#define VOLTAGE_LIMIT_SECTION_TAG   0
guiCheckBox_t checkBox_ApplyLowVoltageLimit;
guiCheckBox_t checkBox_ApplyHighVoltageLimit;
guiWidgetHandler_t checkBoxHandlers_VoltageLimit[2];

guiSpinBox_t spinBox_LowVoltageLimit;
guiSpinBox_t spinBox_HighVoltageLimit;
guiWidgetHandler_t spinBoxHandlers_VoltageLimit[2];





//-------------------------------------------------------//
//  Panel initialization
//
//-------------------------------------------------------//
void guiSetupPanel_Initialize(guiGenericWidget_t *parent)
{
    // Initialize
    guiPanel_Initialize(&guiSetupPanel, parent);
    guiSetupPanel.processEvent = guiSetupPanel_ProcessEvents;     // redefine standard panel message processing funtion
    guiSetupPanel.widgets.count = SETUP_PANEL_ELEMENTS_COUNT;
    guiSetupPanel.widgets.elements = guiSetupPanelElements;
    guiSetupPanel.widgets.elements[0] = &setupList;
    guiSetupPanel.widgets.elements[1] = &checkBox_ApplyLowVoltageLimit;
    guiSetupPanel.widgets.elements[2] = &checkBox_ApplyHighVoltageLimit;
    guiSetupPanel.widgets.elements[3] = &spinBox_LowVoltageLimit;
    guiSetupPanel.widgets.elements[4] = &spinBox_HighVoltageLimit;
    guiSetupPanel.widgets.elements[5] = 0;
    guiSetupPanel.widgets.elements[6] = 0;
    guiSetupPanel.widgets.elements[7] = 0;
    guiSetupPanel.x = 0;
    guiSetupPanel.y = 0;
    guiSetupPanel.width = 96 * 2;
    guiSetupPanel.height = 68;
    guiSetupPanel.showFocus = 1;
    guiSetupPanel.focusFallsThrough = 0;

    // Main list
    guiStringList_Initialize(&setupList, (guiGenericWidget_t *)&guiSetupPanel );
    setupList.font = &font_h10;
    setupList.textAlignment = ALIGN_LEFT;
    setupList.hasFrame = 1;
    setupList.showFocus = 1;
    setupList.showStringFocus = 1;
    setupList.canWrap = 0;
    setupList.restoreIndexOnEscape = 1;
    setupList.x = 0;
    setupList.y = 11;
    setupList.width = 96;
    setupList.height = 68 - 13;
    setupList.stringCount = SETUP_LIST_ELEMENTS_COUNT;
    setupList.strings = setupListElements;
    setupList.strings[0] = " Voltage limit";
    setupList.strings[1] = " Current limit";
    setupList.strings[2] = " Overload time";
    setupList.strings[3] = " 3333";
    setupList.strings[4] = " 4444";
    setupList.strings[5] = "  ---- Exit ---- ";
    setupList.handlers.count = 1;
    setupList.handlers.elements = setupListHandlers;
    setupList.handlers.elements[0].eventType = STRINGLIST_INDEX_CHANGED;
    setupList.handlers.elements[0].handler = &guiSetupList_onIndexChanged;
    setupList.acceptFocusByTab = 0;
    setupList.tag = 255;
    setupList.processEvent = &guiSetupList_ProcessEvent;

    // Voltage limit section
    guiCheckBox_Initialize(&checkBox_ApplyLowVoltageLimit, (guiGenericWidget_t *)&guiSetupPanel);
    checkBox_ApplyLowVoltageLimit.font = &font_h10;
    checkBox_ApplyLowVoltageLimit.x = 96 + 4;
    checkBox_ApplyLowVoltageLimit.y = 0;
    checkBox_ApplyLowVoltageLimit.width = 66;
    checkBox_ApplyLowVoltageLimit.height = 14;
    checkBox_ApplyLowVoltageLimit.text = "Low: [V]";
    checkBox_ApplyLowVoltageLimit.tag = VOLTAGE_LIMIT_SECTION_TAG;
    checkBox_ApplyLowVoltageLimit.tabIndex = 1;
    checkBox_ApplyLowVoltageLimit.handlers.elements = &checkBoxHandlers_VoltageLimit[0];
    checkBox_ApplyLowVoltageLimit.handlers.count = 1;
    checkBox_ApplyLowVoltageLimit.handlers.elements[0].eventType = CHECKBOX_CHECKED_CHANGED;
    checkBox_ApplyLowVoltageLimit.handlers.elements[0].handler = onLowVoltageLimitChanged;


    guiCheckBox_Initialize(&checkBox_ApplyHighVoltageLimit, (guiGenericWidget_t *)&guiSetupPanel);
    checkBox_ApplyHighVoltageLimit.font = &font_h10;
    checkBox_ApplyHighVoltageLimit.x = 96 + 4;
    checkBox_ApplyHighVoltageLimit.y = 36;
    checkBox_ApplyHighVoltageLimit.width = 66;
    checkBox_ApplyHighVoltageLimit.height = 14;
    checkBox_ApplyHighVoltageLimit.text = "High: [V]";
    checkBox_ApplyHighVoltageLimit.tag = VOLTAGE_LIMIT_SECTION_TAG;
    checkBox_ApplyHighVoltageLimit.tabIndex = 3;
    checkBox_ApplyHighVoltageLimit.handlers.elements = &checkBoxHandlers_VoltageLimit[1];
    checkBox_ApplyHighVoltageLimit.handlers.count = 1;
    checkBox_ApplyHighVoltageLimit.handlers.elements[0].eventType = CHECKBOX_CHECKED_CHANGED;
    checkBox_ApplyHighVoltageLimit.handlers.elements[0].handler = onHighVoltageLimitChanged;

    guiSpinBox_Initialize(&spinBox_LowVoltageLimit, (guiGenericWidget_t *)&guiSetupPanel);
    spinBox_LowVoltageLimit.x = 96+10;
    spinBox_LowVoltageLimit.y = 14;
    spinBox_LowVoltageLimit.width = 60;
    spinBox_LowVoltageLimit.height = 18;
    spinBox_LowVoltageLimit.textRightOffset = -2;
    spinBox_LowVoltageLimit.textTopOffset = 2;
    spinBox_LowVoltageLimit.tabIndex = 2;
    spinBox_LowVoltageLimit.tag = VOLTAGE_LIMIT_SECTION_TAG;
    spinBox_LowVoltageLimit.font = &font_h11;
    spinBox_LowVoltageLimit.dotPosition = 2;
    spinBox_LowVoltageLimit.activeDigit = 2;
    spinBox_LowVoltageLimit.minDigitsToDisplay = 3;
    spinBox_LowVoltageLimit.restoreValueOnEscape = 1;
    spinBox_LowVoltageLimit.maxValue = 2100;
    spinBox_LowVoltageLimit.minValue = -1;
    spinBox_LowVoltageLimit.showFocus = 1;
    spinBox_LowVoltageLimit.value = 1;
    guiSpinBox_SetValue(&spinBox_LowVoltageLimit, 0, 0);
    spinBox_LowVoltageLimit.handlers.elements = &spinBoxHandlers_VoltageLimit[0];
    spinBox_LowVoltageLimit.handlers.count = 1;
    spinBox_LowVoltageLimit.handlers.elements[0].eventType = SPINBOX_VALUE_CHANGED;
    spinBox_LowVoltageLimit.handlers.elements[0].handler = onLowVoltageLimitChanged;

    guiSpinBox_Initialize(&spinBox_HighVoltageLimit, (guiGenericWidget_t *)&guiSetupPanel);
    spinBox_HighVoltageLimit.x = 96+10;
    spinBox_HighVoltageLimit.y = 50;
    spinBox_HighVoltageLimit.width = 60;
    spinBox_HighVoltageLimit.height = 18;
    spinBox_HighVoltageLimit.textRightOffset = -2;
    spinBox_HighVoltageLimit.textTopOffset = 2;
    spinBox_HighVoltageLimit.tabIndex = 4;
    spinBox_HighVoltageLimit.tag = VOLTAGE_LIMIT_SECTION_TAG;
    spinBox_HighVoltageLimit.font = &font_h11;
    spinBox_HighVoltageLimit.dotPosition = 2;
    spinBox_HighVoltageLimit.activeDigit = 2;
    spinBox_HighVoltageLimit.minDigitsToDisplay = 3;
    spinBox_HighVoltageLimit.restoreValueOnEscape = 1;
    spinBox_HighVoltageLimit.maxValue = 2100;
    spinBox_HighVoltageLimit.minValue = -1;
    spinBox_HighVoltageLimit.showFocus = 1;
    spinBox_HighVoltageLimit.value = 1;
    guiSpinBox_SetValue(&spinBox_HighVoltageLimit, 0, 0);
    spinBox_HighVoltageLimit.handlers.elements = &spinBoxHandlers_VoltageLimit[1];
    spinBox_HighVoltageLimit.handlers.count = 1;
    spinBox_HighVoltageLimit.handlers.elements[0].eventType = SPINBOX_VALUE_CHANGED;
    spinBox_HighVoltageLimit.handlers.elements[0].handler = onHighVoltageLimitChanged;

}




static uint8_t guiSetupPanel_ProcessEvents(struct guiGenericWidget_t *widget, guiEvent_t event)
{
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    switch (event.type)
    {
        case GUI_EVENT_DRAW:
            guiGraph_DrawPanel(&guiSetupPanel);
            if (guiSetupPanel.redrawForced)
            {
                // Draw static elements
                LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
                LCD_SetFont(&font_h10_bold);
                LCD_PrintString("Settings", 22, 0, IMAGE_MODE_NORMAL);
            }
            // Reset flags - redrawForced will be reset by core
            guiSetupPanel.redrawFocus = 0;
            guiSetupPanel.redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:
            processResult = guiPanel_ProcessEvent(widget, event);
            if (processResult == GUI_EVENT_ACCEPTED)
            {
                guiCore_RequestFocusChange((guiGenericWidget_t*)&setupList);
                event.type = STRINGLIST_EVENT_ACTIVATE;
                guiCore_AddMessageToQueue((guiGenericWidget_t*)&setupList, &event);
            }
            break;
        case GUI_EVENT_KEY:
            if ((event.spec == GUI_KEY_EVENT_DOWN) && (event.lparam == GUI_KEY_ESC))
            {
                guiCore_RequestFocusChange((guiGenericWidget_t*)&setupList);
                event.type = STRINGLIST_EVENT_ACTIVATE;
                guiCore_AddMessageToQueue((guiGenericWidget_t*)&setupList, &event);
                break;
            }
            /*if ((event.spec == GUI_KEY_EVENT_DOWN) && (event.lparam == GUI_KEY_ENCODER))
            {

            }*/
            // fall down to default
        default:
            processResult = guiPanel_ProcessEvent(widget, event);
    }

    return processResult;
}


static uint8_t guiSetupList_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    switch (event.type)
    {
        case GUI_EVENT_KEY:
            if ((event.spec == GUI_KEY_EVENT_DOWN) && (event.lparam == GUI_KEY_OK))
            {
                guiCore_RequestFocusNextWidget((guiGenericContainer_t *)&guiSetupPanel, 1);
                break;
            }
            else if ((event.spec == GUI_KEY_EVENT_DOWN) && (event.lparam == GUI_KEY_ESC))
            {
                // Do nothing
                break;
            }
            // fall down to default
        default:
            processResult = guiStringList_ProcessEvent(widget, event);
    }
    return processResult;
}

static uint8_t guiSetupList_onIndexChanged(void *widget, guiEvent_t *event)
{
    uint8_t minTag = setupList.selectedIndex * 10;
    uint8_t maxTag = minTag + 10 - 1;
    guiCore_SetVisibleByTag(&guiSetupPanel.widgets, 0, 250, ITEMS_IN_RANGE_ARE_INVISIBLE);
    guiCore_SetVisibleByTag(&guiSetupPanel.widgets, minTag, maxTag, ITEMS_IN_RANGE_ARE_VISIBLE);
    return 0;
}


static uint8_t onLowVoltageLimitChanged(void *widget, guiEvent_t *event)
{
    uint8_t limEnabled = 0;
    if (checkBox_ApplyLowVoltageLimit.isChecked)
        limEnabled = 1;
    applyGuiVoltageLimit(0, limEnabled, spinBox_LowVoltageLimit.value * 10);
    return 0;
}

void setLowVoltageLimitSetting(uint8_t isEnabled, int16_t value)
{
    guiCheckbox_SetChecked(&checkBox_ApplyLowVoltageLimit, isEnabled);
    guiSpinBox_SetValue(&spinBox_LowVoltageLimit, value / 10, 0);
}

static uint8_t onHighVoltageLimitChanged(void *widget, guiEvent_t *event)
{
    uint8_t limEnabled = 0;
    if (checkBox_ApplyHighVoltageLimit.isChecked)
        limEnabled = 1;
    applyGuiVoltageLimit(1, limEnabled, spinBox_HighVoltageLimit.value * 10);
    return 0;
}

void setHighVoltageLimitSetting(uint8_t isEnabled, int16_t value)
{
    guiCheckbox_SetChecked(&checkBox_ApplyHighVoltageLimit, isEnabled);
    guiSpinBox_SetValue(&spinBox_HighVoltageLimit, value / 10, 0);
}






