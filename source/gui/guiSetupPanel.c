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

// Other forms and panels
#include "guiMasterPanel.h"
#include "guiSetupPanel.h"
#include "guiMessagePanel1.h"

#include "guiTop.h"
#include "converter.h"
#include "eeprom.h"


static uint8_t guiSetupPanel_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
static uint8_t guiSetupPanel_onVisibleChanged(void *widget, guiEvent_t *event);
static uint8_t guiSetupPanel_onFocusChanged(void *widget, guiEvent_t *event);

static uint8_t guiSetupList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
static uint8_t guiSetupList_onKeyEvent(void *widget, guiEvent_t *event);
static uint8_t guiSetupList_onIndexChanged(void *widget, guiEvent_t *event);
static uint8_t guiSetupList_onVisibleChanged(void *widget, guiEvent_t *event);
static uint8_t guiSetupList_onFocusChanged(void *widget, guiEvent_t *event);
static uint8_t guiSetupList_ChildKeyHandler(void *widget, guiEvent_t *event);


static uint8_t guiChSetupList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
static uint8_t guiChSetupList_onKeyEvent(void *widget, guiEvent_t *event);
static uint8_t guiChSetupList_onIndexChanged(void *widget, guiEvent_t *event);
static uint8_t guiChSetupList_onVisibleChanged(void *widget, guiEvent_t *event);
static uint8_t guiChSetupList_onFocusChanged(void *widget, guiEvent_t *event);
static uint8_t guiChSetupList_ChildKeyHandler(void *widget, guiEvent_t *event);

static uint8_t guiCheckBoxLimit_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
static uint8_t guiSpinBoxLimit_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);

static uint8_t guiProfileList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
static uint8_t guiProfileList_onKeyEvent(void *widget, guiEvent_t *event);
static uint8_t guiProfileList_onFocusChanged(void *widget, guiEvent_t *event);

static uint8_t onLowLimitChanged(void *widget, guiEvent_t *event);
static uint8_t onHighLimitChanged(void *widget, guiEvent_t *event);
static uint8_t onOverloadSettingChanged(void *widget, guiEvent_t *event);

static void updateVoltageLimit(uint8_t channel, uint8_t limit_type);
static void updateCurrentLimit(uint8_t channel, uint8_t range, uint8_t limit_type);
static void updateOverloadSetting(void);


//--------- Setup panel  ----------//
#define SETUP_PANEL_ELEMENTS_COUNT 20   // CHECKME - possibly too much
guiPanel_t     guiSetupPanel;
static void *guiSetupPanelElements[SETUP_PANEL_ELEMENTS_COUNT];
guiWidgetHandler_t setupPanelHandlers[2];

//--------- Panel elements ---------//
// Title label
static guiTextLabel_t textLabel_title;        // Menu item list title


guiStringList_t setupList;
#define SETUP_LIST_ELEMENTS_COUNT 6
char *setupListElements[SETUP_LIST_ELEMENTS_COUNT];
guiWidgetHandler_t setupListHandlers[4];

// Channel setup list
guiStringList_t chSetupList;
#define CH_SETUP_LIST_ELEMENTS_COUNT 4
char *chSsetupListElements[CH_SETUP_LIST_ELEMENTS_COUNT];
guiWidgetHandler_t chSetupListHandlers[4];

// Profile list
guiStringList_t profileList;
#define PROFILE_LIST_ELEMENTS_COUNT     EE_PROFILES_COUNT       // from eeprom module
char *profileListElements[PROFILE_LIST_ELEMENTS_COUNT];
char profileListStrings[PROFILE_LIST_ELEMENTS_COUNT][EE_PROFILE_NAME_SIZE];
guiWidgetHandler_t profileListHandlers[4];

struct {
    uint8_t channel;
    uint8_t currentRange;
    uint8_t view;
    uint8_t profileAction;
} setupView;

enum setupViewModes {
    VIEW_VOLTAGE,
    VIEW_CURRENT,
    VIEW_OTHER
};

enum setupViewProfileActions {
    PROFILE_SAVE,
    PROFILE_LOAD
};

// Hint label
static guiTextLabel_t textLabel_hint;        // Menu item hint

// Low and high limit section
guiCheckBox_t checkBox_ApplyLowLimit;
guiCheckBox_t checkBox_ApplyHighLimit;
guiWidgetHandler_t LowLimit_CheckBoxHandlers[2];
guiWidgetHandler_t HighLimit_CheckBoxHandlers[2];

guiSpinBox_t spinBox_LowLimit;
guiSpinBox_t spinBox_HighLimit;
guiWidgetHandler_t LowLimit_SpinBoxHandlers[2];
guiWidgetHandler_t HighLimit_SpinBoxHandlers[2];

// Overload section
guiCheckBox_t checkBox_OverloadProtect;
guiCheckBox_t checkBox_OverloadWarning;
guiSpinBox_t spinBox_OverloadThreshold;
guiTextLabel_t textLabel_overloadThresholdHint;
guiWidgetHandler_t Overload_CheckBoxHandlers[2];
guiWidgetHandler_t Overload_SpinBoxHandlers[2];


//-------------------------------------------------------//
//  Panel initialization
//
//-------------------------------------------------------//
void guiSetupPanel_Initialize(guiGenericWidget_t *parent)
{
    // Initialize
    guiPanel_Initialize(&guiSetupPanel, parent);
    guiSetupPanel.widgets.count = SETUP_PANEL_ELEMENTS_COUNT;
    guiSetupPanel.widgets.elements = guiSetupPanelElements;
    guiSetupPanel.x = 0;
    guiSetupPanel.y = 0;
    guiSetupPanel.width = 96 * 2;
    guiSetupPanel.height = 68;
    guiSetupPanel.showFocus = 1;
    guiSetupPanel.focusFallsThrough = 0;
    guiSetupPanel.keyTranslator = &guiSetupPanel_KeyTranslator;
    guiSetupPanel.handlers.count = 2;
    guiSetupPanel.handlers.elements = setupPanelHandlers;
    guiSetupPanel.handlers.elements[0].eventType = GUI_ON_VISIBLE_CHANGED;
    guiSetupPanel.handlers.elements[0].handler = *guiSetupPanel_onVisibleChanged;
    guiSetupPanel.handlers.elements[1].eventType = GUI_ON_FOCUS_CHANGED;
    guiSetupPanel.handlers.elements[1].handler = *guiSetupPanel_onFocusChanged;

    // Initialize text label for menu list title
    guiTextLabel_Initialize(&textLabel_title, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_title, (guiGenericContainer_t *)&guiSetupPanel);
    textLabel_title.x = 0;
    textLabel_title.y = 0;
    textLabel_title.width = 95;
    textLabel_title.height = 10;
    textLabel_title.textAlignment = ALIGN_TOP;
    textLabel_title.text = "";
    textLabel_title.font = &font_h10_bold;
    textLabel_title.tag = 255;

    // Main list
    guiStringList_Initialize(&setupList, 0 );
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&setupList, (guiGenericContainer_t *)&guiSetupPanel);
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
    setupList.strings[2] = " Overload setup";
    setupList.strings[3] = " Profile load";
    setupList.strings[4] = " Profile save";
    setupList.strings[5] = " Profile setup";
    setupList.handlers.count = 4;
    setupList.handlers.elements = setupListHandlers;
    setupList.handlers.elements[0].eventType = STRINGLIST_INDEX_CHANGED;
    setupList.handlers.elements[0].handler = &guiSetupList_onIndexChanged;
    setupList.handlers.elements[1].eventType = GUI_ON_VISIBLE_CHANGED;
    setupList.handlers.elements[1].handler = &guiSetupList_onVisibleChanged;
    setupList.handlers.elements[2].eventType = GUI_ON_FOCUS_CHANGED;
    setupList.handlers.elements[2].handler = &guiSetupList_onFocusChanged;
    setupList.handlers.elements[3].eventType = GUI_EVENT_KEY;
    setupList.handlers.elements[3].handler = &guiSetupList_onKeyEvent;
    setupList.acceptFocusByTab = 0;
    setupList.keyTranslator = &guiSetupList_KeyTranslator;

    // Channel setup list
    guiStringList_Initialize(&chSetupList, 0 );
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&chSetupList, (guiGenericContainer_t *)&guiSetupPanel);
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
    chSetupList.strings[1] = " Current lim. 20A";
    chSetupList.strings[2] = " Current lim. 40A";
    chSetupList.strings[3] = " ******";
    chSetupList.handlers.count = 4;
    chSetupList.handlers.elements = chSetupListHandlers;
    chSetupList.handlers.elements[0].eventType = STRINGLIST_INDEX_CHANGED;
    chSetupList.handlers.elements[0].handler = &guiChSetupList_onIndexChanged;
    chSetupList.handlers.elements[1].eventType = GUI_ON_VISIBLE_CHANGED;
    chSetupList.handlers.elements[1].handler = &guiChSetupList_onVisibleChanged;
    chSetupList.handlers.elements[2].eventType = GUI_ON_FOCUS_CHANGED;
    chSetupList.handlers.elements[2].handler = &guiChSetupList_onFocusChanged;
    chSetupList.handlers.elements[3].eventType = GUI_EVENT_KEY;
    chSetupList.handlers.elements[3].handler = &guiChSetupList_onKeyEvent;
    chSetupList.acceptFocusByTab = 0;
    chSetupList.keyTranslator = guiChSetupList_KeyTranslator;


    // Initialize text label for menu item hint
    guiTextLabel_Initialize(&textLabel_hint, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_hint, (guiGenericContainer_t *)&guiSetupPanel);
    textLabel_hint.x = 96+5;
    textLabel_hint.y = 14;
    textLabel_hint.width = 80;
    textLabel_hint.height = 32;
    textLabel_hint.textAlignment = ALIGN_TOP_LEFT;
    textLabel_hint.text = "";
    textLabel_hint.font = &font_h10;
    textLabel_hint.isVisible = 0;

    // Common handlers
    LowLimit_CheckBoxHandlers[0].eventType = CHECKBOX_CHECKED_CHANGED;
    LowLimit_CheckBoxHandlers[0].handler = onLowLimitChanged;
    LowLimit_CheckBoxHandlers[1].eventType = GUI_EVENT_KEY;
    LowLimit_CheckBoxHandlers[1].handler = guiChSetupList_ChildKeyHandler;
    LowLimit_SpinBoxHandlers[0].eventType = SPINBOX_VALUE_CHANGED;
    LowLimit_SpinBoxHandlers[0].handler = onLowLimitChanged;
    LowLimit_SpinBoxHandlers[1].eventType = GUI_EVENT_KEY;
    LowLimit_SpinBoxHandlers[1].handler = guiChSetupList_ChildKeyHandler;

    HighLimit_CheckBoxHandlers[0].eventType = CHECKBOX_CHECKED_CHANGED;
    HighLimit_CheckBoxHandlers[0].handler = onHighLimitChanged;
    HighLimit_CheckBoxHandlers[1].eventType = GUI_EVENT_KEY;
    HighLimit_CheckBoxHandlers[1].handler = guiChSetupList_ChildKeyHandler;
    HighLimit_SpinBoxHandlers[0].eventType = SPINBOX_VALUE_CHANGED;
    HighLimit_SpinBoxHandlers[0].handler = onHighLimitChanged;
    HighLimit_SpinBoxHandlers[1].eventType = GUI_EVENT_KEY;
    HighLimit_SpinBoxHandlers[1].handler = guiChSetupList_ChildKeyHandler;


    //--------------- Limits section ---------------//

    guiCheckBox_Initialize(&checkBox_ApplyLowLimit, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&checkBox_ApplyLowLimit, (guiGenericContainer_t *)&guiSetupPanel);
    checkBox_ApplyLowLimit.font = &font_h10;
    checkBox_ApplyLowLimit.x = 96 + 4;
    checkBox_ApplyLowLimit.y = 0;
    checkBox_ApplyLowLimit.width = 66;
    checkBox_ApplyLowLimit.height = 14;
    checkBox_ApplyLowLimit.isVisible = 0;
    checkBox_ApplyLowLimit.text = "Low: [V]";
    checkBox_ApplyLowLimit.tabIndex = 1;
    checkBox_ApplyLowLimit.handlers.elements = LowLimit_CheckBoxHandlers;
    checkBox_ApplyLowLimit.handlers.count = 2;
    checkBox_ApplyLowLimit.keyTranslator = guiCheckBoxLimit_KeyTranslator;

    guiCheckBox_Initialize(&checkBox_ApplyHighLimit, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&checkBox_ApplyHighLimit, (guiGenericContainer_t *)&guiSetupPanel);
    checkBox_ApplyHighLimit.font = &font_h10;
    checkBox_ApplyHighLimit.x = 96 + 4;
    checkBox_ApplyHighLimit.y = 36;
    checkBox_ApplyHighLimit.width = 66;
    checkBox_ApplyHighLimit.height = 14;
    checkBox_ApplyHighLimit.isVisible = 0;
    checkBox_ApplyHighLimit.text = "High: [V]";
    checkBox_ApplyHighLimit.tabIndex = 3;
    checkBox_ApplyHighLimit.handlers.elements = HighLimit_CheckBoxHandlers;
    checkBox_ApplyHighLimit.handlers.count = 2;
    checkBox_ApplyHighLimit.keyTranslator = guiCheckBoxLimit_KeyTranslator;

    guiSpinBox_Initialize(&spinBox_LowLimit, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&spinBox_LowLimit, (guiGenericContainer_t *)&guiSetupPanel);
    spinBox_LowLimit.x = 96+10;
    spinBox_LowLimit.y = 14;
    spinBox_LowLimit.width = 60;
    spinBox_LowLimit.height = 18;
    spinBox_LowLimit.textRightOffset = -2;
    spinBox_LowLimit.textTopOffset = 2;
    spinBox_LowLimit.tabIndex = 2;
    spinBox_LowLimit.font = &font_h11;
    spinBox_LowLimit.dotPosition = 2;
    spinBox_LowLimit.activeDigit = 2;
    spinBox_LowLimit.minDigitsToDisplay = 3;
    spinBox_LowLimit.restoreValueOnEscape = 1;
    spinBox_LowLimit.maxValue = 5000;
    spinBox_LowLimit.minValue = -1;
    spinBox_LowLimit.showFocus = 1;
    spinBox_LowLimit.isVisible = 0;
    spinBox_LowLimit.value = 1;
    guiSpinBox_SetValue(&spinBox_LowLimit, 0, 0);
    spinBox_LowLimit.handlers.elements = LowLimit_SpinBoxHandlers;
    spinBox_LowLimit.handlers.count = 2;
    spinBox_LowLimit.keyTranslator = guiSpinBoxLimit_KeyTranslator;

    guiSpinBox_Initialize(&spinBox_HighLimit, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&spinBox_HighLimit, (guiGenericContainer_t *)&guiSetupPanel);
    spinBox_HighLimit.x = 96+10;
    spinBox_HighLimit.y = 50;
    spinBox_HighLimit.width = 60;
    spinBox_HighLimit.height = 18;
    spinBox_HighLimit.textRightOffset = -2;
    spinBox_HighLimit.textTopOffset = 2;
    spinBox_HighLimit.tabIndex = 4;
    spinBox_HighLimit.font = &font_h11;
    spinBox_HighLimit.dotPosition = 2;
    spinBox_HighLimit.activeDigit = 2;
    spinBox_HighLimit.minDigitsToDisplay = 3;
    spinBox_HighLimit.restoreValueOnEscape = 1;
    spinBox_HighLimit.maxValue = 5000;
    spinBox_HighLimit.minValue = -1;
    spinBox_HighLimit.showFocus = 1;
    spinBox_HighLimit.isVisible = 0;
    spinBox_HighLimit.value = 1;
    guiSpinBox_SetValue(&spinBox_HighLimit, 0, 0);
    spinBox_HighLimit.handlers.elements = HighLimit_SpinBoxHandlers;
    spinBox_HighLimit.handlers.count = 2;
    spinBox_HighLimit.keyTranslator = guiSpinBoxLimit_KeyTranslator;

    //--------------- Overload section ---------------//

    Overload_CheckBoxHandlers[0].eventType = CHECKBOX_CHECKED_CHANGED;
    Overload_CheckBoxHandlers[0].handler = &onOverloadSettingChanged;
    Overload_CheckBoxHandlers[1].eventType = GUI_EVENT_KEY;
    Overload_CheckBoxHandlers[1].handler = &guiSetupList_ChildKeyHandler;

    Overload_SpinBoxHandlers[0].eventType = SPINBOX_VALUE_CHANGED;
    Overload_SpinBoxHandlers[0].handler = &onOverloadSettingChanged;
    Overload_SpinBoxHandlers[1].eventType = GUI_EVENT_KEY;
    Overload_SpinBoxHandlers[1].handler = &guiSetupList_ChildKeyHandler;

    guiTextLabel_Initialize(&textLabel_overloadThresholdHint, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_overloadThresholdHint, (guiGenericContainer_t *)&guiSetupPanel);
    textLabel_overloadThresholdHint.x = 96+6;
    textLabel_overloadThresholdHint.y = 20;
    textLabel_overloadThresholdHint.width = 80;
    textLabel_overloadThresholdHint.height = 10;
    textLabel_overloadThresholdHint.textAlignment = ALIGN_LEFT;
    textLabel_overloadThresholdHint.text = "Threshold [ms]:";
    textLabel_overloadThresholdHint.font = &font_h10;
    textLabel_overloadThresholdHint.isVisible = 0;

    guiCheckBox_Initialize(&checkBox_OverloadProtect, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&checkBox_OverloadProtect, (guiGenericContainer_t *)&guiSetupPanel);
    checkBox_OverloadProtect.font = &font_h10;
    checkBox_OverloadProtect.x = 96 + 4;
    checkBox_OverloadProtect.y = 0;
    checkBox_OverloadProtect.width = 90;
    checkBox_OverloadProtect.height = 14;
    checkBox_OverloadProtect.isVisible = 0;
    checkBox_OverloadProtect.text = "Protection";
    checkBox_OverloadProtect.tabIndex = 1;
    checkBox_OverloadProtect.handlers.elements = Overload_CheckBoxHandlers;
    checkBox_OverloadProtect.handlers.count = 2;
    checkBox_OverloadProtect.keyTranslator = guiCheckBoxLimit_KeyTranslator;        // CHECKME - name

    guiCheckBox_Initialize(&checkBox_OverloadWarning, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&checkBox_OverloadWarning, (guiGenericContainer_t *)&guiSetupPanel);
    checkBox_OverloadWarning.font = &font_h10;
    checkBox_OverloadWarning.x = 96 + 4;
    checkBox_OverloadWarning.y = 52;
    checkBox_OverloadWarning.width = 90;
    checkBox_OverloadWarning.height = 14;
    checkBox_OverloadWarning.isVisible = 0;
    checkBox_OverloadWarning.text = "Warning";
    checkBox_OverloadWarning.tabIndex = 3;
    checkBox_OverloadWarning.handlers.elements = Overload_CheckBoxHandlers;
    checkBox_OverloadWarning.handlers.count = 2;
    checkBox_OverloadWarning.keyTranslator = guiCheckBoxLimit_KeyTranslator;        // CHECKME - name

    guiSpinBox_Initialize(&spinBox_OverloadThreshold, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&spinBox_OverloadThreshold, (guiGenericContainer_t *)&guiSetupPanel);
    spinBox_OverloadThreshold.x = 96+10;
    spinBox_OverloadThreshold.y = 32;
    spinBox_OverloadThreshold.width = 60;
    spinBox_OverloadThreshold.height = 18;
    spinBox_OverloadThreshold.textRightOffset = -2;
    spinBox_OverloadThreshold.textTopOffset = 2;
    spinBox_OverloadThreshold.tabIndex = 2;
    spinBox_OverloadThreshold.font = &font_h11;
    spinBox_OverloadThreshold.dotPosition = 1;
    spinBox_OverloadThreshold.activeDigit = 0;
    spinBox_OverloadThreshold.minDigitsToDisplay = 2;
    spinBox_OverloadThreshold.restoreValueOnEscape = 1;
    spinBox_OverloadThreshold.maxValue = 1000;
    spinBox_OverloadThreshold.minValue = -1;
    spinBox_OverloadThreshold.showFocus = 1;
    spinBox_OverloadThreshold.isVisible = 0;
    spinBox_OverloadThreshold.value = 1;
    guiSpinBox_SetValue(&spinBox_OverloadThreshold, 0, 0);
    spinBox_OverloadThreshold.handlers.elements = Overload_SpinBoxHandlers;
    spinBox_OverloadThreshold.handlers.count = 2;
    spinBox_OverloadThreshold.keyTranslator = guiSpinBoxLimit_KeyTranslator;


    //--------------- Profile section ---------------//

    guiStringList_Initialize(&profileList, 0 );
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&profileList, (guiGenericContainer_t *)&guiSetupPanel);
    profileList.font = &font_h10;
    profileList.textAlignment = ALIGN_LEFT;
    profileList.hasFrame = 1;
    profileList.showFocus = 1;
    profileList.isVisible = 0;
    profileList.acceptFocusByTab = 1;
    profileList.showStringFocus = 1;
    profileList.canWrap = 0;
    profileList.restoreIndexOnEscape = 1;
    profileList.x = 96;
    profileList.y = 11;
    profileList.width = 96;
    profileList.height = 68 - 13;
    profileList.strings = profileListElements;
    profileList.stringCount = 0;
    while (profileList.stringCount < PROFILE_LIST_ELEMENTS_COUNT)
    {
        profileList.strings[profileList.stringCount] = profileListStrings[profileList.stringCount];
        profileList.strings[profileList.stringCount++][0] = 0;
    }
    profileList.handlers.count = 2;
    profileList.handlers.elements = profileListHandlers;
    profileList.handlers.elements[0].eventType = GUI_EVENT_KEY;
    profileList.handlers.elements[0].handler = &guiProfileList_onKeyEvent;
    profileList.handlers.elements[1].eventType = GUI_ON_FOCUS_CHANGED;
    profileList.handlers.elements[1].handler = &guiProfileList_onFocusChanged;
    profileList.handlers.elements[2].eventType = 0;
    profileList.handlers.elements[2].handler = 0;
    profileList.handlers.elements[3].eventType = 0;
    profileList.handlers.elements[3].handler = 0;
    profileList.keyTranslator = &guiProfileList_KeyTranslator;


    //---------- Tags ----------//

    // Group 1
    checkBox_ApplyLowLimit.tag = 1;
    checkBox_ApplyHighLimit.tag = 1;
    spinBox_LowLimit.tag = 1;
    spinBox_HighLimit.tag = 1;
    chSetupList.tag = 0;


    // Group 2
    setupList.tag = 10;
    checkBox_OverloadProtect.tag = 13;
    checkBox_OverloadWarning.tag = 13;
    spinBox_OverloadThreshold.tag = 13;
    textLabel_overloadThresholdHint.tag = 13;
    profileList.tag = 14;   // takes tags 14,15

    // Other
    textLabel_hint.tag = 11;

}



static uint8_t guiSetupPanel_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
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
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        tkey->key = (int16_t)event->lparam < 0 ? PANEL_KEY_PREV :
              ((int16_t)event->lparam > 0 ? PANEL_KEY_NEXT : 0);
    }
    return 0;
}


static uint8_t guiSetupPanel_onVisibleChanged(void *widget, guiEvent_t *event)
{
    // Bring GUI elements to initial state
    if (guiSetupPanel.isVisible == 1)
    {
        guiCore_SetVisible((guiGenericWidget_t *)&chSetupList, 0);      // list's onVisible handler will also
        guiCore_SetVisible((guiGenericWidget_t *)&setupList, 1);        // hide or show related widgets
    }
    return 0;
}


static uint8_t guiSetupPanel_onFocusChanged(void *widget, guiEvent_t *event)
{
    if (guiSetupPanel.isFocused)
    {
        guiCore_RequestFocusChange((guiGenericWidget_t*)&setupList);
        event->type = STRINGLIST_EVENT_ACTIVATE;
        guiCore_AddMessageToQueue((guiGenericWidget_t*)&setupList, event);
    }
    return 0;
}


//-------------------------//
// guiSetupList

static uint8_t guiSetupList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiStringlistTranslatedKey_t *tkey = (guiStringlistTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_LEFT)
            tkey->key = STRINGLIST_KEY_UP;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = STRINGLIST_KEY_DOWN;
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        tkey->key = (int16_t)event->lparam < 0 ? STRINGLIST_KEY_UP :
              ((int16_t)event->lparam > 0 ? STRINGLIST_KEY_DOWN : 0);
    }
    return 0;
}


static uint8_t guiSetupList_onKeyEvent(void *widget, guiEvent_t *event)
{
    // Here unhandled key events are caught
    if (((event->spec == GUI_KEY_EVENT_DOWN) && (event->lparam == GUI_KEY_OK)) ||
        ((event->spec == GUI_KEY_EVENT_UP_SHORT) && (event->lparam == GUI_KEY_ENCODER)))
    {
        if (setupList.selectedIndex == 0)
        {
            // Show the channel setup list
            guiCore_SetVisible((guiGenericWidget_t*)&setupList, 0);
            guiCore_SetVisible((guiGenericWidget_t*)&chSetupList, 1);
            guiCore_RequestFocusChange((guiGenericWidget_t *)&chSetupList);

        }
        else if (setupList.selectedIndex == 1)
        {
            // Show the channel setup list
            guiCore_SetVisible((guiGenericWidget_t*)&setupList, 0);
            guiCore_SetVisible((guiGenericWidget_t*)&chSetupList, 1);
            guiCore_RequestFocusChange((guiGenericWidget_t *)&chSetupList);
        }
        else
        {
            guiCore_RequestFocusNextWidget((guiGenericContainer_t *)&guiSetupPanel, 1);
        }
        return GUI_EVENT_ACCEPTED;
    }

    return GUI_EVENT_DECLINE;
}


static uint8_t guiSetupList_onIndexChanged(void *widget, guiEvent_t *event)
{
    uint8_t minTag = setupList.tag + 1;
    uint8_t maxTag = minTag + 9;
    uint8_t currTag = setupList.tag + setupList.selectedIndex + 1;
    guiCore_SetVisibleByTag(&guiSetupPanel.widgets, minTag, maxTag, ITEMS_IN_RANGE_ARE_INVISIBLE);
    if (setupList.selectedIndex == 0)
    {
        setupView.channel = CHANNEL_5V;
        textLabel_hint.isVisible = 1;
        textLabel_hint.text = "Ch. 5V setup ...";
        textLabel_hint.redrawRequired = 1;
        textLabel_hint.redrawText = 1;
        guiCore_SetVisible((guiGenericWidget_t *)&textLabel_hint, 1);
    }
    else if (setupList.selectedIndex == 1)
    {
        setupView.channel = CHANNEL_12V;
        textLabel_hint.isVisible = 1;
        textLabel_hint.text = "Ch. 12V setup ...";
        textLabel_hint.redrawRequired = 1;
        textLabel_hint.redrawText = 1;
        guiCore_SetVisible((guiGenericWidget_t *)&textLabel_hint, 1);
    }
    else if (setupList.selectedIndex == 3)
    {
        setupView.profileAction = PROFILE_LOAD;
        guiCore_SetVisible((guiGenericWidget_t *)&profileList, 1);
    }
    else if (setupList.selectedIndex == 4)
    {
        setupView.profileAction = PROFILE_SAVE;
        guiCore_SetVisible((guiGenericWidget_t *)&profileList, 1);
    }
    else
    {
        guiCore_SetVisibleByTag(&guiSetupPanel.widgets, currTag, currTag, ITEMS_IN_RANGE_ARE_VISIBLE);
        if (setupList.selectedIndex == 2)
            updateOverloadSetting();
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


static uint8_t guiSetupList_onFocusChanged(void *widget, guiEvent_t *event)
{
    if (setupList.isFocused)
    {
        guiStringList_SetActive(&setupList, 1, 0);  // will call handler
    }
    return 0;
}


static uint8_t guiSetupList_ChildKeyHandler(void *widget, guiEvent_t *event)
{
    uint8_t res = GUI_EVENT_DECLINE;
    if (event->type == GUI_EVENT_KEY)
    {
        if (event->spec == GUI_KEY_EVENT_UP_SHORT)
        {
            if (event->lparam == GUI_KEY_ESC)
            {
                guiCore_RequestFocusChange((guiGenericWidget_t*)&setupList);
                res = GUI_EVENT_ACCEPTED;
            }
        }
        if (event->spec == GUI_KEY_EVENT_HOLD)
        {
            if (event->lparam == GUI_KEY_ENCODER)
            {
                guiCore_RequestFocusChange((guiGenericWidget_t*)&setupList);
                res = GUI_EVENT_ACCEPTED;
            }
        }
    }
    return res;
}


//-------------------------//
// guiChSetupList


static uint8_t guiChSetupList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiStringlistTranslatedKey_t *tkey = (guiStringlistTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_LEFT)
            tkey->key = STRINGLIST_KEY_UP;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = STRINGLIST_KEY_DOWN;
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        tkey->key = (int16_t)event->lparam < 0 ? STRINGLIST_KEY_UP :
              ((int16_t)event->lparam > 0 ? STRINGLIST_KEY_DOWN : 0);
    }
    return 0;
}


static uint8_t guiChSetupList_onKeyEvent(void *widget, guiEvent_t *event)
{
    // Here unhandled key events are caught
    if (((event->spec == GUI_KEY_EVENT_DOWN) && (event->lparam == GUI_KEY_OK)) ||
        ((event->spec == GUI_KEY_EVENT_UP_SHORT) && (event->lparam == GUI_KEY_ENCODER)))
    {
        guiCore_RequestFocusNextWidget((guiGenericContainer_t *)&guiSetupPanel, 1);
        return GUI_EVENT_ACCEPTED;
    }
    else if (((event->spec == GUI_KEY_EVENT_UP_SHORT) && (event->lparam == GUI_KEY_ESC)) ||
             ((event->spec == GUI_KEY_EVENT_HOLD) && (event->lparam == GUI_KEY_ENCODER)))
    {
        guiCore_SetVisible((guiGenericWidget_t*)&chSetupList, 0);
        guiCore_SetVisible((guiGenericWidget_t*)&setupList, 1);
        guiCore_RequestFocusChange((guiGenericWidget_t*)&setupList);
        return GUI_EVENT_ACCEPTED;
    }

    return GUI_EVENT_DECLINE;
}


static uint8_t guiChSetupList_onIndexChanged(void *widget, guiEvent_t *event)
{
    uint8_t currTag = chSetupList.tag + chSetupList.selectedIndex + 1;
    uint8_t minTag = chSetupList.tag + 1;
    uint8_t maxTag = minTag + 9;
    guiCore_SetVisibleByTag(&guiSetupPanel.widgets, minTag, maxTag, ITEMS_IN_RANGE_ARE_INVISIBLE);

    if (chSetupList.selectedIndex <= 2)
    {
        guiCore_SetVisible((guiGenericWidget_t *)&checkBox_ApplyLowLimit, 1);
        guiCore_SetVisible((guiGenericWidget_t *)&checkBox_ApplyHighLimit, 1);
        guiCore_SetVisible((guiGenericWidget_t *)&spinBox_LowLimit, 1);
        guiCore_SetVisible((guiGenericWidget_t *)&spinBox_HighLimit, 1);
    }

    if (chSetupList.selectedIndex == 0)
    {
        guiCheckBox_SetText(&checkBox_ApplyLowLimit, "Low: [V]");
        guiCheckBox_SetText(&checkBox_ApplyHighLimit, "High: [V]");
        // Update widgets for voltage
        setupView.view = VIEW_VOLTAGE;
        updateVoltageLimit(setupView.channel, LIMIT_TYPE_LOW);
        updateVoltageLimit(setupView.channel, LIMIT_TYPE_HIGH);
    }
    else if ((chSetupList.selectedIndex == 1) || (chSetupList.selectedIndex == 2))
    {
        guiCheckBox_SetText(&checkBox_ApplyLowLimit, "Low: [A]");
        guiCheckBox_SetText(&checkBox_ApplyHighLimit, "High: [A]");
        // Update widgets for current
        setupView.view = VIEW_CURRENT;
        setupView.currentRange = (chSetupList.selectedIndex == 1) ? CURRENT_RANGE_LOW : CURRENT_RANGE_HIGH;
        updateCurrentLimit(setupView.channel, setupView.currentRange, LIMIT_TYPE_LOW);
        updateCurrentLimit(setupView.channel, setupView.currentRange, LIMIT_TYPE_HIGH);
    }
    else
    {
        setupView.view = VIEW_OTHER;
        guiCore_SetVisibleByTag(&guiSetupPanel.widgets, currTag, currTag, ITEMS_IN_RANGE_ARE_VISIBLE);
    }


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
        if (setupView.channel == CHANNEL_5V)
            textLabel_title.text = "5V channel";
        else
            textLabel_title.text = "12V channel";
        textLabel_title.redrawRequired = 1;
        textLabel_title.redrawText = 1;
    }
    return 0;
}


static uint8_t guiChSetupList_onFocusChanged(void *widget, guiEvent_t *event)
{
    if (chSetupList.isFocused)
    {
        guiStringList_SetActive(&chSetupList, 1, 0);  // will call handler
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


static uint8_t guiCheckBoxLimit_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiCheckboxTranslatedKey_t *tkey = (guiCheckboxTranslatedKey_t *)translatedKey;
    tkey->key = 0;
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
    return 0;
}


static uint8_t guiSpinBoxLimit_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiSpinboxTranslatedKey_t *tkey = (guiSpinboxTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    tkey->increment = 0;
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
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        if (widget == (guiGenericWidget_t *)&spinBox_OverloadThreshold)
        {
            if (spinBox_OverloadThreshold.activeDigit == 0)
                tkey->increment = (int16_t)event->lparam * 2;
            else
                tkey->increment = (int16_t)event->lparam;
        }
        else
        {
            tkey->increment = (int16_t)event->lparam;
        }
    }
    return 0;
}




//-------------------------//
// guiProfileList

static uint8_t guiProfileList_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiStringlistTranslatedKey_t *tkey = (guiStringlistTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_LEFT)
            tkey->key = STRINGLIST_KEY_UP;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = STRINGLIST_KEY_DOWN;
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        tkey->key = (int16_t)event->lparam < 0 ? STRINGLIST_KEY_UP :
              ((int16_t)event->lparam > 0 ? STRINGLIST_KEY_DOWN : 0);
    }
    return 0;
}


static uint8_t guiProfileList_onKeyEvent(void *widget, guiEvent_t *event)
{
    // Here unhandled key events are caught
    if (((event->spec == GUI_KEY_EVENT_DOWN) && (event->lparam == GUI_KEY_OK)) ||
        ((event->spec == GUI_KEY_EVENT_UP_SHORT) && (event->lparam == GUI_KEY_ENCODER)))
    {
        // All checks are done by dispatcher
        if (setupView.profileAction == PROFILE_LOAD)
        {
            // Load profile data from EEPROM
            loadProfile(profileList.selectedIndex);
        }
        else
        {
            // Save profile data to EEPROM
            saveProfile(profileList.selectedIndex, "");   // TODO: ask for new name before saving

        }
        return GUI_EVENT_ACCEPTED;
    }
    else if (((event->spec == GUI_KEY_EVENT_UP_SHORT) && (event->lparam == GUI_KEY_ESC)) ||
             ((event->spec == GUI_KEY_EVENT_HOLD) && (event->lparam == GUI_KEY_ENCODER)))
    {
        guiCore_RequestFocusChange((guiGenericWidget_t*)&setupList);
        return GUI_EVENT_ACCEPTED;
    }

    return GUI_EVENT_DECLINE;
}


static uint8_t guiProfileList_onFocusChanged(void *widget, guiEvent_t *event)
{
    if (profileList.isFocused)
    {
        guiStringList_SetActive(&profileList, 1, 0);  // will call handler
    }
    return 0;
}



//===========================================================================//
//  Hardware interface functions
//===========================================================================//

//-----------------------------------//
//  GUI -> HW
//-----------------------------------//

static uint8_t onLowLimitChanged(void *widget, guiEvent_t *event)
{
    uint8_t limEnabled = 0;
    if (checkBox_ApplyLowLimit.isChecked)
        limEnabled = 1;
    if (setupView.view == VIEW_VOLTAGE)
        applyGuiVoltageLimit(setupView.channel, LIMIT_TYPE_LOW, limEnabled, spinBox_LowLimit.value * 10);
    else if (setupView.view == VIEW_CURRENT)
        applyGuiCurrentLimit(setupView.channel, setupView.currentRange, LIMIT_TYPE_LOW, limEnabled, spinBox_LowLimit.value * 10);
    return 0;
}

static uint8_t onHighLimitChanged(void *widget, guiEvent_t *event)
{
    uint8_t limEnabled = 0;
    if (checkBox_ApplyHighLimit.isChecked)
        limEnabled = 1;
    if (setupView.view == VIEW_VOLTAGE)
        applyGuiVoltageLimit(setupView.channel, LIMIT_TYPE_HIGH, limEnabled, spinBox_HighLimit.value * 10);
    else if (setupView.view == VIEW_CURRENT)
        applyGuiCurrentLimit(setupView.channel, setupView.currentRange, LIMIT_TYPE_HIGH, limEnabled, spinBox_HighLimit.value * 10);
    return 0;
}

static uint8_t onOverloadSettingChanged(void *widget, guiEvent_t *event)
{
	// Threshold in units of 100us
    uint8_t protectionEnabled = checkBox_OverloadProtect.isChecked;
    uint8_t warningEnabled = checkBox_OverloadWarning.isChecked;
    applyGuiOverloadSetting( protectionEnabled, warningEnabled, spinBox_OverloadThreshold.value);
	return 0;
}






//-----------------------------------//
// HW -> GUI
//-----------------------------------//



/********************************************************************
  Internal GUI functions
*********************************************************************/

// Helper fucntion
static void updateLimitWidgets(uint8_t limit_type, uint8_t isEnabled, int32_t value)
{
    if (limit_type == LIMIT_TYPE_LOW)
    {
        guiCheckbox_SetChecked(&checkBox_ApplyLowLimit, isEnabled, 0);
        guiSpinBox_SetValue(&spinBox_LowLimit, value / 10, 0);
    }
    else
    {
        guiCheckbox_SetChecked(&checkBox_ApplyHighLimit, isEnabled, 0);
        guiSpinBox_SetValue(&spinBox_HighLimit, value / 10, 0);
    }
}

// Helper fucntion
static void updateOverloadWidgets(uint8_t protectionEnabled, uint8_t warningEnabled, int32_t threshold)
{
    // Threshold in units of 100us
    guiCheckbox_SetChecked(&checkBox_OverloadProtect, protectionEnabled, 0);
    guiCheckbox_SetChecked(&checkBox_OverloadWarning, warningEnabled, 0);
    guiSpinBox_SetValue(&spinBox_OverloadThreshold, threshold * 2, 0);
}




//---------------------------------------------//
// Called by GUI itself
// Reads voltage limit and updates widgets
//---------------------------------------------//
static void updateVoltageLimit(uint8_t channel, uint8_t limit_type)
{
    uint8_t isEnabled = Converter_GetVoltageLimitState(channel, limit_type);
    uint16_t value = Converter_GetVoltageLimitSetting(channel, limit_type);
    updateLimitWidgets(limit_type, isEnabled, value);
}


//---------------------------------------------//
// Called by GUI itself
// Reads current limit and updates widgets
//---------------------------------------------//
static void updateCurrentLimit(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    uint8_t isEnabled = Converter_GetCurrentLimitState(channel, range, limit_type);
    uint16_t value = Converter_GetCurrentLimitSetting(channel, range, limit_type);
    updateLimitWidgets(limit_type, isEnabled, value);
}


//---------------------------------------------//
// Called by GUI itself
// Reads overload settings and updates widgets
//---------------------------------------------//
static void updateOverloadSetting(void)
{
    uint8_t protectionEnabled = Converter_GetOverloadProtectionState();
    uint8_t warningEnabled = Converter_GetOverloadProtectionWarning();
    uint16_t threshold = Converter_GetOverloadProtectionThreshold();
    updateOverloadWidgets(protectionEnabled, warningEnabled, threshold);
}





/********************************************************************
  Top-level GUI functions
*********************************************************************/


//---------------------------------------------//
// Called by GUI top-level
// Sets voltage limit widgets to new value
//---------------------------------------------//
void setGuiVoltageLimitSetting(uint8_t channel, uint8_t limit_type, uint8_t isEnabled, int32_t value)
{
    // Check if widgets update is required
    if ((channel == setupView.channel) && (setupView.view == VIEW_VOLTAGE))
    {
        updateLimitWidgets(limit_type, isEnabled, value);
    }
}


//---------------------------------------------//
// Called by GUI top-level
// Sets current limit widgets to new value
//---------------------------------------------//
void setGuiCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type, uint8_t isEnabled, int32_t value)
{
    // Check if widgets update is required
    if ((channel == setupView.channel) && (setupView.view == VIEW_CURRENT) && (range == setupView.currentRange))
    {
        updateLimitWidgets(limit_type, isEnabled, value);
    }
}


//---------------------------------------------//
// Called by GUI top-level
// Sets overload widgets to new value
//---------------------------------------------//
void setGuiOverloadSetting(uint8_t protectionEnabled, uint8_t warningEnabled, int32_t threshold)
{
    // Check if widgets update is required
    if (1)
    {
        updateOverloadWidgets(protectionEnabled, warningEnabled, threshold);
    }
}



//---------------------------------------------//
// Called by GUI top-level
// Reads profile data and populates list
//---------------------------------------------//
void updateGuiProfileList(void)
{
    uint8_t i;
    uint8_t profileState;
    char *profileName;
    for (i = 0; i < PROFILE_LIST_ELEMENTS_COUNT; i++)
    {
        profileState = EE_GetProfileState(i);
        if ((profileState == EE_PROFILE_CRC_ERROR) || (profileState == EE_PROFILE_HW_ERROR))
        {
            snprintf(profileList.strings[i], EE_PROFILE_NAME_SIZE, "<empty>");
        }
        else
        {
            //profileName = EE_GetProfileName(i);
            //snprintf(profileList.strings[i], EE_PROFILE_NAME_SIZE, profileName);
            snprintf(profileList.strings[i], EE_PROFILE_NAME_SIZE, " Profile %d", i);
        }
    }
}





