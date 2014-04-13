/**********************************************************
    Module guiSetupPanel




**********************************************************/


#include <stdio.h>      // due to printf
#include <stdint.h>
#include <string.h>

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
#include "guiRadioButton.h"

// Other forms and panels
#include "guiMasterPanel.h"
#include "guiSetupPanel.h"
#include "guiMessagePanel1.h"
#include "guiEditPanel2.h"

#include "guiTop.h"
#include "converter.h"
#include "eeprom.h"
#include "buttons_top.h"


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
static uint8_t onProfileSetupChanged(void *widget, guiEvent_t *event);
static uint8_t onExtSwitchSettingsChanged(void *widget, guiEvent_t *event);

static void updateVoltageLimit(uint8_t channel, uint8_t limit_type);
static void updateCurrentLimit(uint8_t channel, uint8_t range, uint8_t limit_type);
static void updateOverloadSetting(void);


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
    PROFILE_ACTION_SAVE,
    PROFILE_ACTION_LOAD
};


//--------- Setup panel  ----------//
guiPanel_t     guiSetupPanel;


//--------- Panel elements ---------//
// Title label
static guiTextLabel_t textLabel_title;        // Menu item list title

// List of all settiings
guiStringList_t setupList;

// Channel setup list
guiStringList_t chSetupList;

// Profile list
guiStringList_t profileList;

// Hint label
static guiTextLabel_t textLabel_hint;        // Menu item hint

// Low and high limit section
guiCheckBox_t checkBox_ApplyLowLimit;
guiCheckBox_t checkBox_ApplyHighLimit;
guiSpinBox_t spinBox_LowLimit;
guiSpinBox_t spinBox_HighLimit;

// Overload section
guiCheckBox_t checkBox_OverloadProtect;
guiCheckBox_t checkBox_OverloadWarning;
guiSpinBox_t spinBox_OverloadThreshold;
guiTextLabel_t textLabel_overloadThresholdHint;

// Profile setup
guiCheckBox_t checkBox_ProfileSetup1;
guiCheckBox_t checkBox_ProfileSetup2;

// External switch section
guiCheckBox_t checkBox_ExtSwitchEnable;
guiCheckBox_t checkBox_ExtSwitchInverse;
guiRadioButton_t radioBtn_ExtSwitchMode1;
guiRadioButton_t radioBtn_ExtSwitchMode2;
guiRadioButton_t radioBtn_ExtSwitchMode3;

// DAC offset section
guiSpinBox_t spinBox_VoltageDacOffset;
guiSpinBox_t spinBox_CurrentLowDacOffset;
guiSpinBox_t spinBox_CurrentHighDacOffset;
guiTextLabel_t textLabel_VoltageDacOffset;
guiTextLabel_t textLabel_CurrentLowDacOffset;
guiTextLabel_t textLabel_CurrentHighDacOffset;

//-------------------------------------------------------//
//  Panel initialization
//
//-------------------------------------------------------//
void guiSetupPanel_Initialize(guiGenericWidget_t *parent)
{
    uint8_t i;

    // Initialize
    guiPanel_Initialize(&guiSetupPanel, parent);
    guiCore_AllocateWidgetCollection((guiGenericContainer_t *)&guiSetupPanel, 30);
    guiSetupPanel.x = 0;
    guiSetupPanel.y = 0;
    guiSetupPanel.width = 96 * 2;
    guiSetupPanel.height = 68;
    guiSetupPanel.showFocus = 1;
    guiSetupPanel.focusFallsThrough = 0;
    guiSetupPanel.keyTranslator = &guiSetupPanel_KeyTranslator;
    guiCore_AllocateHandlers((guiGenericWidget_t *)&guiSetupPanel, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&guiSetupPanel, GUI_ON_VISIBLE_CHANGED, guiSetupPanel_onVisibleChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&guiSetupPanel, GUI_ON_FOCUS_CHANGED, guiSetupPanel_onFocusChanged);

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
    setupList.stringCount = 8;
    setupList.strings = guiCore_calloc(sizeof(char *) * setupList.stringCount);
    setupList.strings[0] = "Channel 5V";
    setupList.strings[1] = "Channel 12V";
    setupList.strings[2] = "Overload setup";
    setupList.strings[3] = "Profile load";
    setupList.strings[4] = "Profile save";
    setupList.strings[5] = "Profile setup";
    setupList.strings[6] = "External switch";
    setupList.strings[7] = "DAC offset";
    guiCore_AllocateHandlers((guiGenericWidget_t *)&setupList, 4);
    guiCore_AddHandler((guiGenericWidget_t *)&setupList, STRINGLIST_INDEX_CHANGED, guiSetupList_onIndexChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&setupList, GUI_ON_VISIBLE_CHANGED, guiSetupList_onVisibleChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&setupList, GUI_ON_FOCUS_CHANGED, guiSetupList_onFocusChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&setupList, GUI_EVENT_KEY, guiSetupList_onKeyEvent);
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
    chSetupList.stringCount = 4;
    chSetupList.strings = guiCore_calloc(sizeof(char *) * chSetupList.stringCount);
    chSetupList.strings[0] = "Voltage limit";
    chSetupList.strings[1] = "Current lim. 20A";
    chSetupList.strings[2] = "Current lim. 40A";
    guiCore_AllocateHandlers((guiGenericWidget_t *)&chSetupList, 4);
    guiCore_AddHandler((guiGenericWidget_t *)&chSetupList, STRINGLIST_INDEX_CHANGED, guiChSetupList_onIndexChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&chSetupList, GUI_ON_VISIBLE_CHANGED, guiChSetupList_onVisibleChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&chSetupList, GUI_ON_FOCUS_CHANGED, guiChSetupList_onFocusChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&chSetupList, GUI_EVENT_KEY, guiChSetupList_onKeyEvent);
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
    guiCore_AllocateHandlers((guiGenericWidget_t *)&checkBox_ApplyLowLimit, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_ApplyLowLimit, CHECKBOX_CHECKED_CHANGED, onLowLimitChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_ApplyLowLimit, GUI_EVENT_KEY, guiChSetupList_ChildKeyHandler);
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
    guiCore_AllocateHandlers((guiGenericWidget_t *)&checkBox_ApplyHighLimit, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_ApplyHighLimit, CHECKBOX_CHECKED_CHANGED, onHighLimitChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_ApplyHighLimit, GUI_EVENT_KEY, guiChSetupList_ChildKeyHandler);
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
    guiCore_AllocateHandlers((guiGenericWidget_t *)&spinBox_LowLimit, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_LowLimit, SPINBOX_VALUE_CHANGED, onLowLimitChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_LowLimit, GUI_EVENT_KEY, guiChSetupList_ChildKeyHandler);
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
    guiCore_AllocateHandlers((guiGenericWidget_t *)&spinBox_HighLimit, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_HighLimit, SPINBOX_VALUE_CHANGED, onHighLimitChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_HighLimit, GUI_EVENT_KEY, guiChSetupList_ChildKeyHandler);
    spinBox_HighLimit.keyTranslator = guiSpinBoxLimit_KeyTranslator;

    //--------------- Overload section ---------------//

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
    guiCore_AllocateHandlers((guiGenericWidget_t *)&checkBox_OverloadProtect, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_OverloadProtect, CHECKBOX_CHECKED_CHANGED, onOverloadSettingChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_OverloadProtect, GUI_EVENT_KEY, guiSetupList_ChildKeyHandler);
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
    checkBox_OverloadWarning.handlers.count = checkBox_OverloadProtect.handlers.count;      // Handlers are shared
    checkBox_OverloadWarning.handlers.elements = checkBox_OverloadProtect.handlers.elements;
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
    guiCore_AllocateHandlers((guiGenericWidget_t *)&spinBox_OverloadThreshold, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_OverloadThreshold, SPINBOX_VALUE_CHANGED, onOverloadSettingChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_OverloadThreshold, GUI_EVENT_KEY, guiSetupList_ChildKeyHandler);
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
    profileList.y = 0;
    profileList.width = 96;
    profileList.height = 68;
    profileList.stringCount = EE_PROFILES_COUNT;
    profileList.strings = guiCore_malloc(sizeof(char *) * profileList.stringCount);
    for (i=0; i<profileList.stringCount; i++)
        profileList.strings[i] = guiCore_calloc(EE_PROFILE_NAME_SIZE);
    guiCore_AllocateHandlers((guiGenericWidget_t *)&profileList, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&profileList, GUI_EVENT_KEY, guiProfileList_onKeyEvent);
    guiCore_AddHandler((guiGenericWidget_t *)&profileList, GUI_ON_FOCUS_CHANGED, guiProfileList_onFocusChanged);
    profileList.keyTranslator = &guiProfileList_KeyTranslator;

    guiCheckBox_Initialize(&checkBox_ProfileSetup1, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&checkBox_ProfileSetup1, (guiGenericContainer_t *)&guiSetupPanel);
    checkBox_ProfileSetup1.font = &font_h10;
    checkBox_ProfileSetup1.x = 96 + 2;
    checkBox_ProfileSetup1.y = 5;
    checkBox_ProfileSetup1.width = 90;
    checkBox_ProfileSetup1.height = 14;
    checkBox_ProfileSetup1.isVisible = 0;
    checkBox_ProfileSetup1.text = "Save recent";
    checkBox_ProfileSetup1.tabIndex = 1;
    guiCore_AllocateHandlers((guiGenericWidget_t *)&checkBox_ProfileSetup1, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_ProfileSetup1, CHECKBOX_CHECKED_CHANGED, onProfileSetupChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_ProfileSetup1, GUI_EVENT_KEY, guiSetupList_ChildKeyHandler);
    checkBox_ProfileSetup1.keyTranslator = guiCheckBoxLimit_KeyTranslator;        // CHECKME - name;

    guiCheckBox_Initialize(&checkBox_ProfileSetup2, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&checkBox_ProfileSetup2, (guiGenericContainer_t *)&guiSetupPanel);
    checkBox_ProfileSetup2.font = &font_h10;
    checkBox_ProfileSetup2.x = 96 + 2;
    checkBox_ProfileSetup2.y = 30;
    checkBox_ProfileSetup2.width = 90;
    checkBox_ProfileSetup2.height = 14;
    checkBox_ProfileSetup2.isVisible = 0;
    checkBox_ProfileSetup2.text = "Restore";
    checkBox_ProfileSetup2.tabIndex = 2;
    checkBox_ProfileSetup2.handlers.count = checkBox_ProfileSetup1.handlers.count;      // Handlers are shared
    checkBox_ProfileSetup2.handlers.elements = checkBox_ProfileSetup1.handlers.elements;
    checkBox_ProfileSetup2.keyTranslator = guiCheckBoxLimit_KeyTranslator;        // CHECKME - name;

    //--------------- External switch section ---------------//

    guiCheckBox_Initialize(&checkBox_ExtSwitchEnable, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&checkBox_ExtSwitchEnable, (guiGenericContainer_t *)&guiSetupPanel);
    checkBox_ExtSwitchEnable.font = &font_h10;
    checkBox_ExtSwitchEnable.x = 96 + 0;
    checkBox_ExtSwitchEnable.y = 0;
    checkBox_ExtSwitchEnable.width = 60;
    checkBox_ExtSwitchEnable.height = 14;
    checkBox_ExtSwitchEnable.isVisible = 0;
    checkBox_ExtSwitchEnable.text = "Enable";
    checkBox_ExtSwitchEnable.tabIndex = 1;
    guiCore_AllocateHandlers((guiGenericWidget_t *)&checkBox_ExtSwitchEnable, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_ExtSwitchEnable, CHECKBOX_CHECKED_CHANGED, onExtSwitchSettingsChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_ExtSwitchEnable, GUI_EVENT_KEY, guiSetupList_ChildKeyHandler);
    checkBox_ExtSwitchEnable.keyTranslator = guiCheckBoxLimit_KeyTranslator;        // CHECKME - name;

    guiCheckBox_Initialize(&checkBox_ExtSwitchInverse, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&checkBox_ExtSwitchInverse, (guiGenericContainer_t *)&guiSetupPanel);
    checkBox_ExtSwitchInverse.font = &font_h10;
    checkBox_ExtSwitchInverse.x = 96 + 60;
    checkBox_ExtSwitchInverse.y = 0;
    checkBox_ExtSwitchInverse.width = 36;
    checkBox_ExtSwitchInverse.height = 14;
    checkBox_ExtSwitchInverse.isVisible = 0;
    checkBox_ExtSwitchInverse.text = "Inv";
    checkBox_ExtSwitchInverse.tabIndex = 2;
    checkBox_ExtSwitchInverse.handlers.count = checkBox_ExtSwitchEnable.handlers.count;      // Handlers are shared
    checkBox_ExtSwitchInverse.handlers.elements = checkBox_ExtSwitchEnable.handlers.elements;
    checkBox_ExtSwitchInverse.keyTranslator = guiCheckBoxLimit_KeyTranslator;        // CHECKME - name;

    guiRadioButton_Initialize(&radioBtn_ExtSwitchMode1, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&radioBtn_ExtSwitchMode1, (guiGenericContainer_t *)&guiSetupPanel);
    radioBtn_ExtSwitchMode1.x = 96 + 0;
    radioBtn_ExtSwitchMode1.y = 20;
    radioBtn_ExtSwitchMode1.width = 96;
    radioBtn_ExtSwitchMode1.height = 15;
    radioBtn_ExtSwitchMode1.isVisible = 0;
    radioBtn_ExtSwitchMode1.font = &font_h10;
    radioBtn_ExtSwitchMode1.text = "Direct on/off";
    radioBtn_ExtSwitchMode1.tabIndex = 3;
    guiCore_AllocateHandlers((guiGenericWidget_t *)&radioBtn_ExtSwitchMode1, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&radioBtn_ExtSwitchMode1, RADIOBUTTON_CHECKED_CHANGED, onExtSwitchSettingsChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&radioBtn_ExtSwitchMode1, GUI_EVENT_KEY, guiSetupList_ChildKeyHandler);

    guiRadioButton_Initialize(&radioBtn_ExtSwitchMode2, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&radioBtn_ExtSwitchMode2, (guiGenericContainer_t *)&guiSetupPanel);
    radioBtn_ExtSwitchMode2.x = 96 + 0;
    radioBtn_ExtSwitchMode2.y = 35;
    radioBtn_ExtSwitchMode2.width = 96;
    radioBtn_ExtSwitchMode2.height = 15;
    radioBtn_ExtSwitchMode2.isVisible = 0;
    radioBtn_ExtSwitchMode2.font = &font_h10;
    radioBtn_ExtSwitchMode2.text = "Toggle";
    radioBtn_ExtSwitchMode2.tabIndex = 4;
    radioBtn_ExtSwitchMode2.handlers.count = radioBtn_ExtSwitchMode1.handlers.count;      // Handlers are shared
    radioBtn_ExtSwitchMode2.handlers.elements = radioBtn_ExtSwitchMode1.handlers.elements;

    guiRadioButton_Initialize(&radioBtn_ExtSwitchMode3, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&radioBtn_ExtSwitchMode3, (guiGenericContainer_t *)&guiSetupPanel);
    radioBtn_ExtSwitchMode3.x = 96 + 0;
    radioBtn_ExtSwitchMode3.y = 50;
    radioBtn_ExtSwitchMode3.width = 96;
    radioBtn_ExtSwitchMode3.height = 15;
    radioBtn_ExtSwitchMode3.isVisible = 0;
    radioBtn_ExtSwitchMode3.font = &font_h10;
    radioBtn_ExtSwitchMode3.text = "Toggle OFF";
    radioBtn_ExtSwitchMode3.tabIndex = 5;
    radioBtn_ExtSwitchMode3.handlers.count = radioBtn_ExtSwitchMode1.handlers.count;      // Handlers are shared
    radioBtn_ExtSwitchMode3.handlers.elements = radioBtn_ExtSwitchMode1.handlers.elements;


    //--------------- DAC offset section ---------------//
    guiSpinBox_Initialize(&spinBox_VoltageDacOffset, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&spinBox_VoltageDacOffset, (guiGenericContainer_t *)&guiSetupPanel);
    spinBox_VoltageDacOffset.x = 96+70;
    spinBox_VoltageDacOffset.y = 0;
    spinBox_VoltageDacOffset.width = 26;
    spinBox_VoltageDacOffset.height = 17;
    spinBox_VoltageDacOffset.textRightOffset = -2;
    spinBox_VoltageDacOffset.textTopOffset = 2;
    spinBox_VoltageDacOffset.tabIndex = 1;
    spinBox_VoltageDacOffset.font = &font_h10;
    spinBox_VoltageDacOffset.dotPosition = -1;
    spinBox_VoltageDacOffset.activeDigit = 0;
    spinBox_VoltageDacOffset.minDigitsToDisplay = 1;
    spinBox_VoltageDacOffset.restoreValueOnEscape = 1;
    spinBox_VoltageDacOffset.maxValue = 100;
    spinBox_VoltageDacOffset.minValue = -100;
    spinBox_VoltageDacOffset.showFocus = 1;
    spinBox_VoltageDacOffset.value = 55;      // anything different from value being set by guiSpinBox_SetValue()
    guiSpinBox_SetValue(&spinBox_VoltageDacOffset, -100, 0);
    //guiCore_AllocateHandlers((guiGenericWidget_t *)&spinBox_VoltageDacOffset, 2);
    //guiCore_AddHandler((guiGenericWidget_t *)&spinBox_VoltageDacOffset, SPINBOX_VALUE_CHANGED, onOverloadSettingChanged);
    //guiCore_AddHandler((guiGenericWidget_t *)&spinBox_VoltageDacOffset, GUI_EVENT_KEY, guiSetupList_ChildKeyHandler);
    //spinBox_VoltageDacOffset.keyTranslator = guiSpinBoxLimit_KeyTranslator;

    memcpy(&spinBox_CurrentLowDacOffset, &spinBox_VoltageDacOffset, sizeof(spinBox_VoltageDacOffset));
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&spinBox_CurrentLowDacOffset, (guiGenericContainer_t *)&guiSetupPanel);
    spinBox_CurrentLowDacOffset.y = 20;
    spinBox_CurrentLowDacOffset.tabIndex = 2;

    memcpy(&spinBox_CurrentHighDacOffset, &spinBox_VoltageDacOffset, sizeof(spinBox_VoltageDacOffset));
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&spinBox_CurrentHighDacOffset, (guiGenericContainer_t *)&guiSetupPanel);
    spinBox_CurrentHighDacOffset.y = 40;
    spinBox_CurrentHighDacOffset.tabIndex = 3;

    guiTextLabel_Initialize(&textLabel_VoltageDacOffset, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_VoltageDacOffset, (guiGenericContainer_t *)&guiSetupPanel);
    textLabel_VoltageDacOffset.x = 96+0;
    textLabel_VoltageDacOffset.y = 2;
    textLabel_VoltageDacOffset.width = 40;
    textLabel_VoltageDacOffset.height = 10;
    textLabel_VoltageDacOffset.textAlignment = ALIGN_LEFT;
    textLabel_VoltageDacOffset.text = "Voltage [mV]:";
    textLabel_VoltageDacOffset.font = &font_h10;

    memcpy(&textLabel_CurrentLowDacOffset, &textLabel_VoltageDacOffset, sizeof(textLabel_VoltageDacOffset));
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_CurrentLowDacOffset, (guiGenericContainer_t *)&guiSetupPanel);
    textLabel_CurrentLowDacOffset.text = "Cur.low [mA]:";
    textLabel_CurrentLowDacOffset.y = 22;

    memcpy(&textLabel_CurrentHighDacOffset, &textLabel_VoltageDacOffset, sizeof(textLabel_VoltageDacOffset));
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_CurrentHighDacOffset, (guiGenericContainer_t *)&guiSetupPanel);
    textLabel_CurrentHighDacOffset.text = "Cur.high [mA]:";
    textLabel_CurrentHighDacOffset.y = 42;

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

    checkBox_ProfileSetup1.tag = 16;
    checkBox_ProfileSetup2.tag = 16;

    checkBox_ExtSwitchEnable.tag = 17;
    checkBox_ExtSwitchInverse.tag = 17;
    radioBtn_ExtSwitchMode1.tag = 17;
    radioBtn_ExtSwitchMode2.tag = 17;
    radioBtn_ExtSwitchMode3.tag = 17;

    spinBox_VoltageDacOffset.tag = 18;
    spinBox_CurrentLowDacOffset.tag = 18;
    spinBox_CurrentHighDacOffset.tag = 18;
    textLabel_VoltageDacOffset.tag = 18;
    textLabel_CurrentLowDacOffset.tag = 18;
    textLabel_CurrentHighDacOffset.tag = 18;

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
        setupView.profileAction = PROFILE_ACTION_LOAD;
        guiCore_SetVisible((guiGenericWidget_t *)&profileList, 1);
    }
    else if (setupList.selectedIndex == 4)
    {
        setupView.profileAction = PROFILE_ACTION_SAVE;
        guiCore_SetVisible((guiGenericWidget_t *)&profileList, 1);
    }
    else
    {
        guiCore_SetVisibleByTag(&guiSetupPanel.widgets, currTag, currTag, ITEMS_IN_RANGE_ARE_VISIBLE);
        // Update widgets that become visible
        if (setupList.selectedIndex == 2)
            updateOverloadSetting();
        else if (setupList.selectedIndex == 5)
            updateProfileSetup();
        else if (setupList.selectedIndex == 6)
            updateExtSwitchSettings(1);     // update forced
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
        if (setupView.profileAction == PROFILE_ACTION_LOAD)
        {
            // Load profile data from EEPROM
            loadProfile(profileList.selectedIndex);
        }
        else
        {
            // Save profile data to EEPROM (edit name first)
            guiEditPanel2_Show(profileList.strings[profileList.selectedIndex]);
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

void hideEditPanel2(char *newProfileName)
{
    if (newProfileName)
    {
        // New name confirmed
        saveProfile(profileList.selectedIndex, newProfileName);
    }
    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiEditPanel2, &guiEvent_HIDE);
    guiCore_RequestFocusChange((guiGenericWidget_t *)&profileList);
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

static uint8_t onProfileSetupChanged(void *widget, guiEvent_t *event)
{
    uint8_t saveRecentProfile = checkBox_ProfileSetup1.isChecked;
    uint8_t restoreRecentProfile = checkBox_ProfileSetup2.isChecked;
    applyGuiProfileSettings(saveRecentProfile, restoreRecentProfile);
    return 0;
}

static uint8_t onExtSwitchSettingsChanged(void *widget, guiEvent_t *event)
{
    uint8_t swEnabled;
    uint8_t swInverse;
    uint8_t swMode;

    // Skip radioButton uncheck
    if ( (((guiGenericWidget_t *)widget)->type != WT_RADIOBUTTON) || (((guiRadioButton_t *)widget)->isChecked != 0) )
    {
        swEnabled = checkBox_ExtSwitchEnable.isChecked;
        swInverse = checkBox_ExtSwitchInverse.isChecked;
        swMode = (radioBtn_ExtSwitchMode1.isChecked) ? EXTSW_DIRECT :
                    (radioBtn_ExtSwitchMode2.isChecked ? EXTSW_TOGGLE : EXTSW_TOGGLE_OFF);
        applyGuiExtSwitchSettings(swEnabled, swInverse, swMode);
    }
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

//---------------------------------------------//
// Called by GUI itself and by top-level
// Reads profile setup settings and updates widgets
//---------------------------------------------//
void updateProfileSetup(void)
{
    uint8_t saveRecentProfile = EE_IsRecentProfileSavingEnabled();
    uint8_t restoreRecentProfile = EE_IsRecentProfileRestoreEnabled();
    guiCheckbox_SetChecked(&checkBox_ProfileSetup1, saveRecentProfile, 0);
    guiCheckbox_SetChecked(&checkBox_ProfileSetup2, restoreRecentProfile, 0);
}


//---------------------------------------------//
// Called by GUI itself and by top-level
// Reads external switch settings and updates widgets
//---------------------------------------------//
void updateExtSwitchSettings(uint8_t updateForced)      //<-------- CHECKME - correct variant for all GUI<->HW interaction ?
{
    uint8_t swEnabled;
    uint8_t swInverse;
    uint8_t swMode;
    // Check any of widgets representing external switch settings
    if ((updateForced) || (guiCore_IsWidgetVisible((guiGenericWidget_t *)&checkBox_ExtSwitchEnable)))
    {
        // Get information from system
        swEnabled = BTN_IsExtSwitchEnabled();
        swInverse = BTN_GetExtSwitchInversion();
        swMode = BTN_GetExtSwitchMode();
        // Update widgets
        guiCheckbox_SetChecked(&checkBox_ExtSwitchEnable, swEnabled, 0);
        guiCheckbox_SetChecked(&checkBox_ExtSwitchInverse, swInverse, 0);
        if (swMode == EXTSW_DIRECT)
            guiRadioButton_CheckExclusive(&radioBtn_ExtSwitchMode1, 0);
        else if (swMode == EXTSW_TOGGLE)
            guiRadioButton_CheckExclusive(&radioBtn_ExtSwitchMode2, 0);
        else if (swMode == EXTSW_TOGGLE_OFF)
            guiRadioButton_CheckExclusive(&radioBtn_ExtSwitchMode3, 0);
    }
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
// Updates profile record
//---------------------------------------------//
void updateGuiProfileListRecord(uint8_t i, uint8_t profileState, char *name)
{
	switch (profileState)
	{
		case EE_PROFILE_CRC_ERROR:
            snprintf(profileList.strings[i], EE_PROFILE_NAME_SIZE, "<empty>");
			break;
		case EE_PROFILE_HW_ERROR:
            snprintf(profileList.strings[i], EE_PROFILE_NAME_SIZE, "<hw n/a>");
			break;
		default:
            snprintf(profileList.strings[i], EE_PROFILE_NAME_SIZE, "%s", name);
			break;
	}
	profileList.redrawRequired = 1;
	profileList.redrawForced = 1;
}





