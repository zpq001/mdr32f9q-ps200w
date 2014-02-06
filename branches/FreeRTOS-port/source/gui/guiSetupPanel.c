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
static uint8_t guiSetupList_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);
static uint8_t guiSetupList_onIndexChanged(void *widget, guiEvent_t *event);
static uint8_t guiSetupList_onVisibleChanged(void *widget, guiEvent_t *event);
static uint8_t guiSetupList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, guiWidgetTranslatedKey_t *tkey);

static uint8_t guiChSetupList_onIndexChanged(void *widget, guiEvent_t *event);
static uint8_t guiChSetupList_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);
static uint8_t guiChSetupList_onVisibleChanged(void *widget, guiEvent_t *event);

static uint8_t onLowVoltageLimitChanged(void *widget, guiEvent_t *event);
static uint8_t onHighVoltageLimitChanged(void *widget, guiEvent_t *event);

static uint8_t guiChSetupList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, guiWidgetTranslatedKey_t *tkey);
static uint8_t guiCheckBoxLimit_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, guiWidgetTranslatedKey_t *tkey);
static uint8_t guiSpinBoxLimit_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, guiWidgetTranslatedKey_t *tkey);

static uint8_t guiChSetupList_ChildKeyHandler(void *widget, guiEvent_t *event);


//--------- Setup panel  ----------//
#define SETUP_PANEL_ELEMENTS_COUNT 8
guiPanel_t     guiSetupPanel;
static void *guiSetupPanelElements[SETUP_PANEL_ELEMENTS_COUNT];


//--------- Panel elements ---------//
// Title label
static guiTextLabel_t textLabel_title;        // Menu item list title


guiStringList_t setupList;
#define SETUP_LIST_ELEMENTS_COUNT 6
char *setupListElements[SETUP_LIST_ELEMENTS_COUNT];
guiWidgetHandler_t setupListHandlers[2];

// Channel setup list
guiStringList_t chSetupList;
#define CH_SETUP_LIST_ELEMENTS_COUNT 4
char *chSsetupListElements[CH_SETUP_LIST_ELEMENTS_COUNT];
guiWidgetHandler_t chSetupListHandlers[2];
uint8_t channelBeingSetup;

// Hint label
static guiTextLabel_t textLabel_hint;        // Menu item hint




// Voltage limit section
guiCheckBox_t checkBox_ApplyLowVoltageLimit;
guiCheckBox_t checkBox_ApplyHighVoltageLimit;
guiWidgetHandler_t LowVoltageLimit_handlers[3];

guiSpinBox_t spinBox_LowVoltageLimit;
guiSpinBox_t spinBox_HighVoltageLimit;
guiWidgetHandler_t HighVoltageLimit_handlers[3];





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
    guiSetupPanel.widgets.elements[5] = &textLabel_hint;
    guiSetupPanel.widgets.elements[6] = &chSetupList;
    guiSetupPanel.widgets.elements[7] = &textLabel_title;
    guiSetupPanel.x = 0;
    guiSetupPanel.y = 0;
    guiSetupPanel.width = 96 * 2;
    guiSetupPanel.height = 68;
    guiSetupPanel.showFocus = 1;
    guiSetupPanel.focusFallsThrough = 0;

    // Initialize text label for menu list title
    guiTextLabel_Initialize(&textLabel_title, (guiGenericWidget_t *)&guiSetupPanel);
    textLabel_title.x = 0;
    textLabel_title.y = 0;
    textLabel_title.width = 95;
    textLabel_title.height = 10;
    textLabel_title.textAlignment = ALIGN_TOP;
    textLabel_title.text = "";
    textLabel_title.font = &font_h10_bold;
    textLabel_title.tag = 255;


    // Main list
    guiStringList_Initialize(&setupList, (guiGenericWidget_t *)&guiSetupPanel );
    setupList.font = &font_h10;
    setupList.textAlignment = ALIGN_LEFT;
    setupList.hasFrame = 1;
    setupList.showFocus = 1;
    setupList.isVisible = 0;
    setupList.showStringFocus = 1;
    setupList.canWrap = 0;
    setupList.restoreIndexOnEscape = 1;
    setupList.x = 0;
    setupList.y = 11;
    setupList.width = 96;
    setupList.height = 68 - 13;
    setupList.stringCount = SETUP_LIST_ELEMENTS_COUNT;
    setupList.strings = setupListElements;
    setupList.strings[0] = " Channel 5V";
    setupList.strings[1] = " Channel 12V";
    setupList.strings[2] = " 2222";
    setupList.strings[3] = " 3333";
    setupList.strings[4] = " 4444";
    setupList.strings[5] = "  ---- Exit ---- ";
    setupList.handlers.count = 2;
    setupList.handlers.elements = setupListHandlers;
    setupList.handlers.elements[0].eventType = STRINGLIST_INDEX_CHANGED;
    setupList.handlers.elements[0].handler = &guiSetupList_onIndexChanged;
    setupList.handlers.elements[1].eventType = GUI_ON_VISIBLE_CHANGED;
    setupList.handlers.elements[1].handler = &guiSetupList_onVisibleChanged;
    setupList.acceptFocusByTab = 0;
    setupList.processEvent = &guiSetupList_ProcessEvent;
    setupList.keyTranslator = guiSetupList_KeyTranslator;

    guiStringList_Initialize(&chSetupList, (guiGenericWidget_t *)&guiSetupPanel );
    chSetupList.font = &font_h10;
    chSetupList.textAlignment = ALIGN_LEFT;
    chSetupList.hasFrame = 1;
    chSetupList.showFocus = 1;
    chSetupList.isVisible = 0;
    chSetupList.showStringFocus = 1;
    chSetupList.canWrap = 0;
    chSetupList.restoreIndexOnEscape = 1;
    chSetupList.x = 0;
    chSetupList.y = 11;
    chSetupList.width = 96;
    chSetupList.height = 68 - 13;
    chSetupList.stringCount = CH_SETUP_LIST_ELEMENTS_COUNT;
    chSetupList.strings = chSsetupListElements;
    chSetupList.strings[0] = " Voltage limit";
    chSetupList.strings[1] = " Current limit";
    chSetupList.strings[2] = " Overload";
    chSetupList.strings[3] = " ****";
    chSetupList.handlers.count = 2;
    chSetupList.handlers.elements = chSetupListHandlers;
    chSetupList.handlers.elements[0].eventType = STRINGLIST_INDEX_CHANGED;
    chSetupList.handlers.elements[0].handler = &guiChSetupList_onIndexChanged;
    chSetupList.handlers.elements[1].eventType = GUI_ON_VISIBLE_CHANGED;
    chSetupList.handlers.elements[1].handler = &guiChSetupList_onVisibleChanged;
    chSetupList.acceptFocusByTab = 0;
    chSetupList.processEvent = &guiChSetupList_ProcessEvent;
    chSetupList.keyTranslator = guiChSetupList_KeyTranslator;


    // Initialize text label for menu item hint
    guiTextLabel_Initialize(&textLabel_hint, (guiGenericWidget_t *)&guiSetupPanel);
    textLabel_hint.x = 96+5;
    textLabel_hint.y = 14;
    textLabel_hint.width = 80;
    textLabel_hint.height = 32;
    textLabel_hint.textAlignment = ALIGN_TOP_LEFT;
    textLabel_hint.text = "";
    textLabel_hint.font = &font_h10;
    textLabel_hint.isVisible = 0;


    LowVoltageLimit_handlers[0].eventType = CHECKBOX_CHECKED_CHANGED;	// FIXME - can be equal
    LowVoltageLimit_handlers[0].handler = onLowVoltageLimitChanged;
    LowVoltageLimit_handlers[1].eventType = SPINBOX_VALUE_CHANGED;
    LowVoltageLimit_handlers[1].handler = onLowVoltageLimitChanged;
    LowVoltageLimit_handlers[2].eventType = GUI_EVENT_KEY;
    LowVoltageLimit_handlers[2].handler = guiChSetupList_ChildKeyHandler;
    HighVoltageLimit_handlers[0].eventType = CHECKBOX_CHECKED_CHANGED;
    HighVoltageLimit_handlers[0].handler = onHighVoltageLimitChanged;
    HighVoltageLimit_handlers[1].eventType = SPINBOX_VALUE_CHANGED;
    HighVoltageLimit_handlers[1].handler = onHighVoltageLimitChanged;
    HighVoltageLimit_handlers[2].eventType = GUI_EVENT_KEY;
    HighVoltageLimit_handlers[2].handler = guiChSetupList_ChildKeyHandler;

    // Voltage limit section
    guiCheckBox_Initialize(&checkBox_ApplyLowVoltageLimit, (guiGenericWidget_t *)&guiSetupPanel);
    checkBox_ApplyLowVoltageLimit.font = &font_h10;
    checkBox_ApplyLowVoltageLimit.x = 96 + 4;
    checkBox_ApplyLowVoltageLimit.y = 0;
    checkBox_ApplyLowVoltageLimit.width = 66;
    checkBox_ApplyLowVoltageLimit.height = 14;
    checkBox_ApplyLowVoltageLimit.isVisible = 0;
    checkBox_ApplyLowVoltageLimit.text = "Low: [V]";
    checkBox_ApplyLowVoltageLimit.tabIndex = 1;
    checkBox_ApplyLowVoltageLimit.handlers.elements = &LowVoltageLimit_handlers[0];
    checkBox_ApplyLowVoltageLimit.handlers.count = 3;
    //checkBox_ApplyLowVoltageLimit.handlers.elements[0].eventType = CHECKBOX_CHECKED_CHANGED;
    //checkBox_ApplyLowVoltageLimit.handlers.elements[0].handler = onLowVoltageLimitChanged;
    checkBox_ApplyLowVoltageLimit.keyTranslator = guiCheckBoxLimit_KeyTranslator;

    guiCheckBox_Initialize(&checkBox_ApplyHighVoltageLimit, (guiGenericWidget_t *)&guiSetupPanel);
    checkBox_ApplyHighVoltageLimit.font = &font_h10;
    checkBox_ApplyHighVoltageLimit.x = 96 + 4;
    checkBox_ApplyHighVoltageLimit.y = 36;
    checkBox_ApplyHighVoltageLimit.width = 66;
    checkBox_ApplyHighVoltageLimit.height = 14;
    checkBox_ApplyHighVoltageLimit.isVisible = 0;
    checkBox_ApplyHighVoltageLimit.text = "High: [V]";
    checkBox_ApplyHighVoltageLimit.tabIndex = 3;
    checkBox_ApplyHighVoltageLimit.handlers.elements = &HighVoltageLimit_handlers[1];
    checkBox_ApplyHighVoltageLimit.handlers.count = 3;
    //checkBox_ApplyHighVoltageLimit.handlers.elements[0].eventType = CHECKBOX_CHECKED_CHANGED;
    //checkBox_ApplyHighVoltageLimit.handlers.elements[0].handler = onHighVoltageLimitChanged;
    checkBox_ApplyHighVoltageLimit.keyTranslator = guiCheckBoxLimit_KeyTranslator;

    guiSpinBox_Initialize(&spinBox_LowVoltageLimit, (guiGenericWidget_t *)&guiSetupPanel);
    spinBox_LowVoltageLimit.x = 96+10;
    spinBox_LowVoltageLimit.y = 14;
    spinBox_LowVoltageLimit.width = 60;
    spinBox_LowVoltageLimit.height = 18;
    spinBox_LowVoltageLimit.textRightOffset = -2;
    spinBox_LowVoltageLimit.textTopOffset = 2;
    spinBox_LowVoltageLimit.tabIndex = 2;
    spinBox_LowVoltageLimit.font = &font_h11;
    spinBox_LowVoltageLimit.dotPosition = 2;
    spinBox_LowVoltageLimit.activeDigit = 2;
    spinBox_LowVoltageLimit.minDigitsToDisplay = 3;
    spinBox_LowVoltageLimit.restoreValueOnEscape = 1;
    spinBox_LowVoltageLimit.maxValue = 2100;
    spinBox_LowVoltageLimit.minValue = -1;
    spinBox_LowVoltageLimit.showFocus = 1;
    spinBox_LowVoltageLimit.isVisible = 0;
    spinBox_LowVoltageLimit.value = 1;
    guiSpinBox_SetValue(&spinBox_LowVoltageLimit, 0, 0);
    spinBox_LowVoltageLimit.handlers.elements = &LowVoltageLimit_handlers[0];
    spinBox_LowVoltageLimit.handlers.count = 3;
    //spinBox_LowVoltageLimit.handlers.elements[0].eventType = SPINBOX_VALUE_CHANGED;
    //spinBox_LowVoltageLimit.handlers.elements[0].handler = onLowVoltageLimitChanged;
    spinBox_LowVoltageLimit.keyTranslator = guiSpinBoxLimit_KeyTranslator;

    guiSpinBox_Initialize(&spinBox_HighVoltageLimit, (guiGenericWidget_t *)&guiSetupPanel);
    spinBox_HighVoltageLimit.x = 96+10;
    spinBox_HighVoltageLimit.y = 50;
    spinBox_HighVoltageLimit.width = 60;
    spinBox_HighVoltageLimit.height = 18;
    spinBox_HighVoltageLimit.textRightOffset = -2;
    spinBox_HighVoltageLimit.textTopOffset = 2;
    spinBox_HighVoltageLimit.tabIndex = 4;
    spinBox_HighVoltageLimit.font = &font_h11;
    spinBox_HighVoltageLimit.dotPosition = 2;
    spinBox_HighVoltageLimit.activeDigit = 2;
    spinBox_HighVoltageLimit.minDigitsToDisplay = 3;
    spinBox_HighVoltageLimit.restoreValueOnEscape = 1;
    spinBox_HighVoltageLimit.maxValue = 2100;
    spinBox_HighVoltageLimit.minValue = -1;
    spinBox_HighVoltageLimit.showFocus = 1;
    spinBox_HighVoltageLimit.isVisible = 0;
    spinBox_HighVoltageLimit.value = 1;
    guiSpinBox_SetValue(&spinBox_HighVoltageLimit, 0, 0);
    spinBox_HighVoltageLimit.handlers.elements = &HighVoltageLimit_handlers[1];
    spinBox_HighVoltageLimit.handlers.count = 3;
    //spinBox_HighVoltageLimit.handlers.elements[0].eventType = SPINBOX_VALUE_CHANGED;
    //spinBox_HighVoltageLimit.handlers.elements[0].handler = onHighVoltageLimitChanged;
    spinBox_HighVoltageLimit.keyTranslator = guiSpinBoxLimit_KeyTranslator;


    //---------- Tags ----------//

    // Group 1
    checkBox_ApplyLowVoltageLimit.tag = 1;
    checkBox_ApplyHighVoltageLimit.tag = 1;
    spinBox_LowVoltageLimit.tag = 1;
    spinBox_HighVoltageLimit.tag = 1;
    chSetupList.tag = 0;

    // Group 2
    setupList.tag = 10;
    textLabel_hint.tag = 11;
}




static uint8_t guiSetupPanel_ProcessEvents(struct guiGenericWidget_t *widget, guiEvent_t event)
{
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    switch (event.type)
    {
        case GUI_EVENT_INIT:
            //guiCore_SetVisible((guiGenericWidget_t *)&setupList, 1);
            break;
        case GUI_EVENT_DRAW:
            guiGraph_DrawPanel(&guiSetupPanel);
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
                //guiSetupList_onIndexChanged(widget, 0);
            }
            break;
        case GUI_EVENT_SHOW:
            processResult = guiPanel_ProcessEvent(widget, event);
            if (processResult == GUI_EVENT_ACCEPTED)
            {
                // Bring GUI elements to initial state
                if (setupList.isVisible == 0)
                {
                    guiCore_SetVisible((guiGenericWidget_t *)&chSetupList, 0);
                    guiCore_SetVisible((guiGenericWidget_t *)&setupList, 1);
                }
            }
            break;
        case GUI_EVENT_KEY:
            /*if ((event.spec == GUI_KEY_EVENT_UP_SHORT) && (event.lparam == GUI_KEY_ESC))
            {
                if (setupList.isVisible)
                    guiCore_RequestFocusChange((guiGenericWidget_t*)&setupList);
                else if (chSetupList.isVisible)
                    guiCore_RequestFocusChange((guiGenericWidget_t*)&chSetupList);

                break;
            }*/
            if ((event.spec == GUI_KEY_EVENT_HOLD) && (event.lparam == GUI_KEY_ESC))
            {
                processResult = GUI_EVENT_DECLINE;
                break;
            }
            if ((event.spec == GUI_KEY_EVENT_DOWN) && (event.lparam == GUI_KEY_ESC))
            {
                break;
            }
            // fall down to default
        default:
            processResult = guiPanel_ProcessEvent(widget, event);
    }

    return processResult;
}

//-------------------------//

static uint8_t guiSetupList_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    switch (event.type)
    {
        case GUI_EVENT_FOCUS:
            processResult = guiStringList_ProcessEvent(widget, event);
            guiStringList_SetActive(&setupList, 1, 0);  // will call handler
            break;
        case GUI_EVENT_KEY:
            if (((event.spec == GUI_KEY_EVENT_DOWN) && (event.lparam == GUI_KEY_OK)) ||
                ((event.spec == GUI_KEY_EVENT_UP_SHORT) && (event.lparam == GUI_KEY_ENCODER)))
            {
                if (setupList.selectedIndex == 0)
                {
                    // Show the channel setup list
                    channelBeingSetup = GUI_CHANNEL_5V;
                    guiCore_SetVisible((guiGenericWidget_t*)&setupList, 0);
                    guiCore_SetVisible((guiGenericWidget_t*)&chSetupList, 1);
                    guiCore_RequestFocusChange((guiGenericWidget_t *)&chSetupList);
					// Update widgets
					UpdateVoltageLimitSetting(channelBeingSetup, GUI_LIMIT_TYPE_LOW);
					UpdateVoltageLimitSetting(channelBeingSetup, GUI_LIMIT_TYPE_HIGH);
                }
                else if (setupList.selectedIndex == 1)
                {
                    // Show the channel setup list
                    channelBeingSetup = GUI_CHANNEL_12V;
                    guiCore_SetVisible((guiGenericWidget_t*)&setupList, 0);
                    guiCore_SetVisible((guiGenericWidget_t*)&chSetupList, 1);
                    guiCore_RequestFocusChange((guiGenericWidget_t *)&chSetupList);
					// Update widgets
					UpdateVoltageLimitSetting(channelBeingSetup, GUI_LIMIT_TYPE_LOW);
					UpdateVoltageLimitSetting(channelBeingSetup, GUI_LIMIT_TYPE_HIGH);
                }
                else
                {
                    guiCore_RequestFocusNextWidget((guiGenericContainer_t *)&guiSetupPanel, 1);
                }
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
    uint8_t minTag = setupList.tag + 1;
    uint8_t maxTag = minTag + 9;
    uint8_t currTag = setupList.tag + setupList.selectedIndex + 1;
    guiCore_SetVisibleByTag(&guiSetupPanel.widgets, minTag, maxTag, ITEMS_IN_RANGE_ARE_INVISIBLE);
    if (setupList.selectedIndex == 0)
    {
        textLabel_hint.isVisible = 1;
        textLabel_hint.text = "Ch. 5V setup ...";
        textLabel_hint.redrawRequired = 1;
        textLabel_hint.redrawText = 1;
        guiCore_SetVisible((guiGenericWidget_t *)&textLabel_hint, 1);
    }
    else if (setupList.selectedIndex == 1)
    {
        textLabel_hint.isVisible = 1;
        textLabel_hint.text = "Ch. 12V setup ...";
        textLabel_hint.redrawRequired = 1;
        textLabel_hint.redrawText = 1;
        guiCore_SetVisible((guiGenericWidget_t *)&textLabel_hint, 1);
    }
    else
    {
        guiCore_SetVisibleByTag(&guiSetupPanel.widgets, currTag, currTag, ITEMS_IN_RANGE_ARE_VISIBLE);
    }

    return 0;
}


static uint8_t guiSetupList_onVisibleChanged(void *widget, guiEvent_t *event)
{
    if (setupList.isVisible == 0)
    {
        // Hide all related elements
        guiCore_SetVisibleByTag(&guiSetupPanel.widgets, 11, 19, ITEMS_IN_RANGE_ARE_INVISIBLE);
    }
    else
    {
        guiSetupList_onIndexChanged(widget, event);
        textLabel_title.text = "Settings";
        textLabel_title.redrawRequired = 1;
        textLabel_title.redrawText = 1;
    }
    return 0;
}


static uint8_t guiSetupList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, guiWidgetTranslatedKey_t *tkey)
{
    tkey->spec = 0;
    tkey->key = 0;
    if (event->type == GUI_EVENT_KEY)
    {
        if (event->spec == GUI_KEY_EVENT_DOWN)
        {
            if (event->lparam == GUI_KEY_LEFT)
                tkey->key = STRINGLIST_KEY_UP;
            else if (event->lparam == GUI_KEY_RIGHT)
                tkey->key = STRINGLIST_KEY_DOWN;
        }
        if (tkey->key != 0)
            tkey->spec = STRINGLIST_KEY;
    }
    else if (event->type == GUI_EVENT_ENCODER)
    {
        tkey->spec = STRINGLIST_INCREMENT;
        tkey->data = (int16_t)event->lparam;
    }
    return tkey->spec;
}

//-------------------------//

static uint8_t guiChSetupList_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    switch (event.type)
    {
        case GUI_EVENT_FOCUS:
            processResult = guiStringList_ProcessEvent(widget, event);
            guiStringList_SetActive(&chSetupList, 1, 0);  // will call handler
            break;
        case GUI_EVENT_KEY:
            if (((event.spec == GUI_KEY_EVENT_DOWN) && (event.lparam == GUI_KEY_OK)) ||
                ((event.spec == GUI_KEY_EVENT_UP_SHORT) && (event.lparam == GUI_KEY_ENCODER)))
            {
                guiCore_RequestFocusNextWidget((guiGenericContainer_t *)&guiSetupPanel, 1);
                break;
            }
            else if (((event.spec == GUI_KEY_EVENT_UP_SHORT) && (event.lparam == GUI_KEY_ESC)) ||
                     ((event.spec == GUI_KEY_EVENT_HOLD) && (event.lparam == GUI_KEY_ENCODER)))
            {
                guiCore_SetVisible((guiGenericWidget_t*)&chSetupList, 0);
                guiCore_SetVisible((guiGenericWidget_t*)&setupList, 1);
                guiCore_RequestFocusChange((guiGenericWidget_t*)&setupList);
                break;
            }

            // fall down to default
        default:
            processResult = guiStringList_ProcessEvent(widget, event);
    }
    return processResult;
}


static uint8_t guiChSetupList_onIndexChanged(void *widget, guiEvent_t *event)
{
    uint8_t currTag = chSetupList.tag + chSetupList.selectedIndex + 1;
    uint8_t minTag = chSetupList.tag + 1;
    uint8_t maxTag = minTag + 9;
    guiCore_SetVisibleByTag(&guiSetupPanel.widgets, minTag, maxTag, ITEMS_IN_RANGE_ARE_INVISIBLE);
    guiCore_SetVisibleByTag(&guiSetupPanel.widgets, currTag, currTag, ITEMS_IN_RANGE_ARE_VISIBLE);
    return 0;
}


static uint8_t guiChSetupList_onVisibleChanged(void *widget, guiEvent_t *event)
{
    if (chSetupList.isVisible == 0)
    {
        // Hide all related elements
        guiCore_SetVisibleByTag(&guiSetupPanel.widgets, 1, 9, ITEMS_IN_RANGE_ARE_INVISIBLE);
    }
    else
    {
        guiChSetupList_onIndexChanged(widget, event);
        if (channelBeingSetup == GUI_CHANNEL_5V)
            textLabel_title.text = "5V channel";
        else
            textLabel_title.text = "12V channel";
        textLabel_title.redrawRequired = 1;
        textLabel_title.redrawText = 1;
    }
    return 0;
}


static uint8_t guiChSetupList_ChildKeyHandler(void *widget, guiEvent_t *event)
{
    uint8_t res = GUI_EVENT_DECLINE;
    if (event->type == GUI_EVENT_KEY)
    {
        if (event->spec == GUI_KEY_EVENT_UP_SHORT)
        {
            if (event->lparam == GUI_KEY_ESC)
            {
                guiCore_RequestFocusChange((guiGenericWidget_t*)&chSetupList);
                res = GUI_EVENT_ACCEPTED;
            }
        }
        if (event->spec == GUI_KEY_EVENT_HOLD)
        {
            if (event->lparam == GUI_KEY_ENCODER)
            {
                guiCore_RequestFocusChange((guiGenericWidget_t*)&chSetupList);
                res = GUI_EVENT_ACCEPTED;
            }
        }
    }
    return res;
}

//-------------------------//

static uint8_t guiChSetupList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, guiWidgetTranslatedKey_t *tkey)
{
    tkey->spec = 0;
    tkey->key = 0;
    if (event->type == GUI_EVENT_KEY)
    {
        if (event->spec == GUI_KEY_EVENT_DOWN)
        {
            if (event->lparam == GUI_KEY_LEFT)
                tkey->key = STRINGLIST_KEY_UP;
            else if (event->lparam == GUI_KEY_RIGHT)
                tkey->key = STRINGLIST_KEY_DOWN;
        }
        if (tkey->key != 0)
            tkey->spec = STRINGLIST_KEY;
    }
    else if (event->type == GUI_EVENT_ENCODER)
    {
        tkey->spec = STRINGLIST_INCREMENT;
        tkey->data = (int16_t)event->lparam;
    }
    return tkey->spec;
}

static uint8_t guiCheckBoxLimit_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, guiWidgetTranslatedKey_t *tkey)
{
    tkey->spec = 0;
    tkey->key = 0;
    if (event->type == GUI_EVENT_KEY)
    {
        if (event->spec == GUI_KEY_EVENT_DOWN)
        {
            if (event->lparam == GUI_KEY_OK)
                tkey->key = CHECKBOX_KEY_SELECT;
        }
        else if (event->spec == GUI_KEY_EVENT_UP_SHORT)
        {
            if (event->lparam == GUI_KEY_ENCODER)
                tkey->key = CHECKBOX_KEY_SELECT;
        }
        else if (event->spec == GUI_KEY_EVENT_HOLD)
        {
        }

        if (tkey->key != 0)
            tkey->spec = CHECKBOX_KEY;
    }
    return tkey->spec;
}

static uint8_t guiSpinBoxLimit_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, guiWidgetTranslatedKey_t *tkey)
{
    tkey->spec = 0;
    tkey->key = 0;
    if (event->type == GUI_EVENT_KEY)
    {
        if (event->spec == GUI_KEY_EVENT_DOWN)
        {
            if (event->lparam == GUI_KEY_OK)
                tkey->key = SPINBOX_KEY_SELECT;
            else if (event->lparam == GUI_KEY_LEFT)
                tkey->key = SPINBOX_KEY_LEFT;
            else if (event->lparam == GUI_KEY_RIGHT)
                tkey->key = SPINBOX_KEY_RIGHT;
        }
        else if (event->spec == GUI_KEY_EVENT_UP_SHORT)
        {
            if (event->lparam == GUI_KEY_ENCODER)
                tkey->key = SPINBOX_KEY_SELECT;
            else if (event->lparam == GUI_KEY_ESC)
                tkey->key = SPINBOX_KEY_EXIT;
        }
        else if (event->spec == GUI_KEY_EVENT_HOLD)
        {
            if (event->lparam == GUI_KEY_ENCODER)
                tkey->key = SPINBOX_KEY_EXIT;
        }
        if (tkey->key != 0)
            tkey->spec = SPINBOX_KEY;
    }
    else if (event->type == GUI_EVENT_ENCODER)
    {
        tkey->spec = SPINBOX_INCREMENT;
        tkey->data = (int16_t)event->lparam;
    }

    return tkey->spec;
}



static uint8_t onLowVoltageLimitChanged(void *widget, guiEvent_t *event)
{
    uint8_t limEnabled = 0;
    if (checkBox_ApplyLowVoltageLimit.isChecked)
        limEnabled = 1;
    applyGuiVoltageLimit(channelBeingSetup, GUI_LIMIT_TYPE_LOW, limEnabled, spinBox_LowVoltageLimit.value * 10);
    return 0;
}

void setLowVoltageLimitSetting(uint8_t channel, uint8_t isEnabled, int16_t value)
{
	if (channel == channelBeingSetup)
	{
        guiCheckbox_SetChecked(&checkBox_ApplyLowVoltageLimit, isEnabled, 0);
		guiSpinBox_SetValue(&spinBox_LowVoltageLimit, value / 10, 0);
	}
}

static uint8_t onHighVoltageLimitChanged(void *widget, guiEvent_t *event)
{
    uint8_t limEnabled = 0;
    if (checkBox_ApplyHighVoltageLimit.isChecked)
        limEnabled = 1;
    applyGuiVoltageLimit(channelBeingSetup, GUI_LIMIT_TYPE_HIGH, limEnabled, spinBox_HighVoltageLimit.value * 10);
    return 0;
}

void setHighVoltageLimitSetting(uint8_t channel, uint8_t isEnabled, int16_t value)
{
	if (channel == channelBeingSetup)
	{
        guiCheckbox_SetChecked(&checkBox_ApplyHighVoltageLimit, isEnabled, 0);
		guiSpinBox_SetValue(&spinBox_HighVoltageLimit, value / 10, 0);
	}
}






