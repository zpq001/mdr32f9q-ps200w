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
#include "guiSelectTextBox.h"

// Other forms and panels
#include "guiMasterPanel.h"
#include "guiSetupPanel.h"
#include "guiMessagePanel1.h"
#include "guiEditPanel2.h"

#include "guiTop.h"
#include "guiConfig.h"

#ifdef _GUITESTPROJ_
#include "taps.h"
#else
#include "converter.h"
#include "eeprom.h"
#include "buttons_top.h"
#include "service.h"
#endif  //_GUITESTPROJ_



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

static uint8_t guiSelectTextBox_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);

static uint8_t onLowLimitChanged(void *widget, guiEvent_t *event);
static uint8_t onHighLimitChanged(void *widget, guiEvent_t *event);

static uint8_t onOverloadSettingsChanged(void *widget, guiEvent_t *event);
static uint8_t onProfileSettingsChanged(void *widget, guiEvent_t *event);
static uint8_t onExtSwitchSettingsChanged(void *widget, guiEvent_t *event);
static uint8_t onDacSettingsChanged(void *widget, guiEvent_t *event);
static uint8_t onUartSettingsChanged(void *widget, guiEvent_t *event);




struct {
    uint8_t channel;
    uint8_t current_range;
    uint8_t view;
    uint8_t profileAction;
    uint8_t uart_num;
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

static char profileName[EE_PROFILE_NAME_SIZE];


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

// UART section
guiCheckBox_t checkBox_UartEnable;
guiSelectTextBox_t selectTextBox_UartParity;
guiSelectTextBox_t selectTextBox_UartBaudrate;
guiTextLabel_t textLabel_Uart;

//-------------------------------------------------------//
//  Panel initialization
//
//-------------------------------------------------------//
void guiSetupPanel_Initialize(guiGenericWidget_t *parent)
{
    uint8_t i;

    // Initialize
    guiPanel_Initialize(&guiSetupPanel, parent);
    guiCore_AllocateWidgetCollection((guiGenericContainer_t *)&guiSetupPanel, 40);
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
    setupList.stringCount = 10;
    setupList.strings = guiCore_calloc(sizeof(char *) * setupList.stringCount);
    setupList.strings[0] = "Channel 5V";
    setupList.strings[1] = "Channel 12V";
    setupList.strings[2] = "Overload setup";
    setupList.strings[3] = "Profile load";
    setupList.strings[4] = "Profile save";
    setupList.strings[5] = "Profile setup";
    setupList.strings[6] = "External switch";
    setupList.strings[7] = "DAC offset";
    setupList.strings[8] = "UART 1";
    setupList.strings[9] = "UART 2";
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
    chSetupList.stringCount = 3;
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
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_OverloadProtect, CHECKBOX_CHECKED_CHANGED, onOverloadSettingsChanged);
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
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_OverloadThreshold, SPINBOX_VALUE_CHANGED, onOverloadSettingsChanged);
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
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_ProfileSetup1, CHECKBOX_CHECKED_CHANGED, onProfileSettingsChanged);
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
    guiCore_AllocateHandlers((guiGenericWidget_t *)&spinBox_VoltageDacOffset, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_VoltageDacOffset, SPINBOX_VALUE_CHANGED, onDacSettingsChanged);
    guiCore_AddHandler((guiGenericWidget_t *)&spinBox_VoltageDacOffset, GUI_EVENT_KEY, guiSetupList_ChildKeyHandler);
    spinBox_VoltageDacOffset.keyTranslator = guiSpinBoxLimit_KeyTranslator;

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

    //--------------- UART section ---------------//

    guiCheckBox_Initialize(&checkBox_UartEnable, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&checkBox_UartEnable, (guiGenericContainer_t *)&guiSetupPanel);
    checkBox_UartEnable.font = &font_h10;
    checkBox_UartEnable.x = 96 + 5;
    checkBox_UartEnable.y = 0;
    checkBox_UartEnable.width = 60;
    checkBox_UartEnable.height = 14;
    checkBox_UartEnable.isVisible = 0;
    checkBox_UartEnable.text = "Enable";
    checkBox_UartEnable.tabIndex = 1;
    guiCore_AllocateHandlers((guiGenericWidget_t *)&checkBox_UartEnable, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_UartEnable, GUI_EVENT_KEY, guiSetupList_ChildKeyHandler);
    guiCore_AddHandler((guiGenericWidget_t *)&checkBox_UartEnable, CHECKBOX_CHECKED_CHANGED, onUartSettingsChanged);
    checkBox_UartEnable.keyTranslator = guiCheckBoxLimit_KeyTranslator;        // CHECKME - name;

    guiSelectTextBox_Initialize(&selectTextBox_UartParity, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&selectTextBox_UartParity, (guiGenericContainer_t *)&guiSetupPanel);
    selectTextBox_UartParity.font = &font_h10;
    selectTextBox_UartParity.x = 96 + 2;
    selectTextBox_UartParity.y = 18;
    selectTextBox_UartParity.width = 92;
    selectTextBox_UartParity.height = 14;
    selectTextBox_UartParity.isVisible = 0;
    selectTextBox_UartParity.tabIndex = 2;
    selectTextBox_UartParity.restoreIndexOnEscape = 1;
    selectTextBox_UartParity.stringCount = 3;
    selectTextBox_UartParity.stringList = guiCore_calloc(sizeof(char *) * selectTextBox_UartParity.stringCount);
    selectTextBox_UartParity.stringList[0] = "Parity NO";
    selectTextBox_UartParity.stringList[1] = "Parity ODD";
    selectTextBox_UartParity.stringList[2] = "Parity EVEN";
    selectTextBox_UartParity.valueList = guiCore_calloc(sizeof(uint8_t) * selectTextBox_UartParity.stringCount);
    ((uint8_t *)selectTextBox_UartParity.valueList)[0] = PARITY_NO;
    ((uint8_t *)selectTextBox_UartParity.valueList)[1] = PARITY_ODD;
    ((uint8_t *)selectTextBox_UartParity.valueList)[2] = PARITY_EVEN;
    guiCore_AllocateHandlers((guiGenericWidget_t *)&selectTextBox_UartParity, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&selectTextBox_UartParity, GUI_EVENT_KEY, guiSetupList_ChildKeyHandler);
    guiCore_AddHandler((guiGenericWidget_t *)&selectTextBox_UartParity, SELECTTEXTBOX_ACTIVE_CHANGED, onUartSettingsChanged);
    selectTextBox_UartParity.keyTranslator = guiSelectTextBox_KeyTranslator;

    guiSelectTextBox_Initialize(&selectTextBox_UartBaudrate, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&selectTextBox_UartBaudrate, (guiGenericContainer_t *)&guiSetupPanel);
    selectTextBox_UartBaudrate.font = &font_h10;
    selectTextBox_UartBaudrate.x = 96 + 2;
    selectTextBox_UartBaudrate.y = 36;
    selectTextBox_UartBaudrate.width = 92;
    selectTextBox_UartBaudrate.height = 14;
    selectTextBox_UartBaudrate.isVisible = 0;
    selectTextBox_UartBaudrate.tabIndex = 3;
    selectTextBox_UartBaudrate.restoreIndexOnEscape = 1;
    selectTextBox_UartBaudrate.stringCount = 5;
    selectTextBox_UartBaudrate.stringList = guiCore_calloc(sizeof(char *) * selectTextBox_UartBaudrate.stringCount);
    selectTextBox_UartBaudrate.stringList[0] = "9600 bps";
    selectTextBox_UartBaudrate.stringList[1] = "19200 bps";
    selectTextBox_UartBaudrate.stringList[2] = "38400 bps";
    selectTextBox_UartBaudrate.stringList[3] = "57600 bps";
    selectTextBox_UartBaudrate.stringList[4] = "115.2 kbps";
    selectTextBox_UartBaudrate.valueList = guiCore_calloc(sizeof(uint32_t) * selectTextBox_UartBaudrate.stringCount);
    ((uint32_t *)selectTextBox_UartBaudrate.valueList)[0] = 9600;
    ((uint32_t *)selectTextBox_UartBaudrate.valueList)[1] = 19200;
    ((uint32_t *)selectTextBox_UartBaudrate.valueList)[2] = 38400;
    ((uint32_t *)selectTextBox_UartBaudrate.valueList)[3] = 57600;
    ((uint32_t *)selectTextBox_UartBaudrate.valueList)[4] = 115200;
    selectTextBox_UartBaudrate.handlers.elements = selectTextBox_UartParity.handlers.elements;  // Handlers are shared
    selectTextBox_UartBaudrate.handlers.count = selectTextBox_UartParity.handlers.count;
    selectTextBox_UartBaudrate.keyTranslator = guiSelectTextBox_KeyTranslator;

    guiTextLabel_Initialize(&textLabel_Uart, 0);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_Uart, (guiGenericContainer_t *)&guiSetupPanel);
    textLabel_Uart.x = 96;
    textLabel_Uart.y = 55;
    textLabel_Uart.width = 96;
    textLabel_Uart.height = 10;
    textLabel_Uart.textAlignment = ALIGN_LEFT;
    textLabel_Uart.text = "(8 bit, 1 stop)";
    textLabel_Uart.font = &font_h10;
    textLabel_Uart.textAlignment = ALIGN_CENTER;



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

    checkBox_UartEnable.tag = 19;     // takes tags 19,20
    selectTextBox_UartParity.tag = 19;
    selectTextBox_UartBaudrate.tag = 19;
    textLabel_Uart.tag = 19;

    // Other
    textLabel_hint.tag = 11;

    // Add other widgets
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&guiEditPanel2, (guiGenericContainer_t *)&guiSetupPanel);
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
        guiTextLabel_SetText(&textLabel_hint, "Ch. 5V setup ...");
        guiCore_SetVisible((guiGenericWidget_t *)&textLabel_hint, 1);
    }
    else if (setupList.selectedIndex == 1)
    {
        setupView.channel = CHANNEL_12V;
        guiTextLabel_SetText(&textLabel_hint, "Ch. 12V setup ...");
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
    else if (setupList.selectedIndex == 8)
    {
        setupView.uart_num = 1;
        guiCore_SetVisibleByTag(&guiSetupPanel.widgets, 19, 20, ITEMS_IN_RANGE_ARE_VISIBLE);
        guiUpdateUartSettings(setupView.uart_num);
    }
    else if (setupList.selectedIndex == 9)
    {
        setupView.uart_num = 2;
        guiCore_SetVisibleByTag(&guiSetupPanel.widgets, 19, 20, ITEMS_IN_RANGE_ARE_VISIBLE);
        guiUpdateUartSettings(setupView.uart_num);
    }
    else
    {
        guiCore_SetVisibleByTag(&guiSetupPanel.widgets, currTag, currTag, ITEMS_IN_RANGE_ARE_VISIBLE);
        // Update widgets that become visible
        if (setupList.selectedIndex == 2)
            {}//guiTop_UpdateOverloadSettings();
        else if (setupList.selectedIndex == 5)
            {}//guiTop_UpdateProfileSettings();
        else if (setupList.selectedIndex == 6)
            {}//guiTop_UpdateExtSwitchSettings();
        else if (setupList.selectedIndex == 7)
            {}//guiTop_UpdateDacSettings();
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
        guiSetupList_onIndexChanged(widget, event);     // CHECKME - is this necessary for widgets update?
        guiTextLabel_SetText(&textLabel_title, "Settings");
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
        guiUpdateVoltageLimit(setupView.channel, UPDATE_LOW_LIMIT | UPDATE_HIGH_LIMIT);
    }
    else if ((chSetupList.selectedIndex == 1) || (chSetupList.selectedIndex == 2))
    {
        guiCheckBox_SetText(&checkBox_ApplyLowLimit, "Low: [A]");
        guiCheckBox_SetText(&checkBox_ApplyHighLimit, "High: [A]");
        // Update widgets for current
        setupView.view = VIEW_CURRENT;
        setupView.current_range = (chSetupList.selectedIndex == 1) ? CURRENT_RANGE_LOW : CURRENT_RANGE_HIGH;
        guiUpdateCurrentLimit(setupView.channel, setupView.current_range, UPDATE_LOW_LIMIT | UPDATE_HIGH_LIMIT);
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
            guiTextLabel_SetText(&textLabel_title, "5V channel");
        else
            guiTextLabel_SetText(&textLabel_title, "12V channel");
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
        tkey->increment = (int16_t)event->lparam;

        if (widget == (guiGenericWidget_t *)&spinBox_OverloadThreshold)
        {
            if (spinBox_OverloadThreshold.activeDigit == 0)
                tkey->increment = (int16_t)event->lparam * 2;
        }
        else if (widget == (guiGenericWidget_t *)&spinBox_VoltageDacOffset)
        {
            if (spinBox_VoltageDacOffset.activeDigit == 0)
                tkey->increment = (int16_t)event->lparam * 5;
        }
        else if (widget == (guiGenericWidget_t *)&spinBox_CurrentLowDacOffset)
        {
            if (spinBox_CurrentLowDacOffset.activeDigit == 0)
                tkey->increment = (int16_t)event->lparam * 5;
        }
        else if (widget == (guiGenericWidget_t *)&spinBox_CurrentHighDacOffset)
        {
            if (spinBox_CurrentHighDacOffset.activeDigit == 0)
                tkey->increment = (int16_t)event->lparam * 10;
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
            guiTop_LoadProfile(profileList.selectedIndex);
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


static uint8_t guiSelectTextBox_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiSelectTextBoxTranslatedKey_t *tkey = (guiSelectTextBoxTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_OK)
            tkey->key = SELECTTEXTBOX_KEY_SELECT;
        else if (event->lparam == GUI_KEY_LEFT)
            tkey->key = SELECTTEXTBOX_KEY_PREV;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = SELECTTEXTBOX_KEY_NEXT;
    }
    else if (event->spec == GUI_KEY_EVENT_UP_SHORT)
    {
        if (event->lparam == GUI_KEY_ENCODER)
            tkey->key = SELECTTEXTBOX_KEY_SELECT;
        else if (event->lparam == GUI_KEY_ESC)
            tkey->key = SELECTTEXTBOX_KEY_EXIT;
    }
    else if (event->spec == GUI_KEY_EVENT_HOLD)
    {
        if (event->lparam == GUI_KEY_ENCODER)
            tkey->key = SELECTTEXTBOX_KEY_EXIT;
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        if ((int8_t)event->lparam < 0)
            tkey->key = SELECTTEXTBOX_KEY_PREV;
        else if ((int8_t)event->lparam > 0)
            tkey->key = SELECTTEXTBOX_KEY_NEXT;
    }
    return 0;
}


void hideEditPanel2(char *newProfileName)
{
    if (newProfileName)
    {
        // New name confirmed
        guiTop_SaveProfile(profileList.selectedIndex, newProfileName);
    }
    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiEditPanel2, &guiEvent_HIDE);
    guiCore_RequestFocusChange((guiGenericWidget_t *)&profileList);
}





//=================================================================//
//          Functions for widgets access
//
//=================================================================//


//------------------------------------------------------//
//              Voltage/Current limits                  //
//------------------------------------------------------//

static uint8_t onLowLimitChanged(void *widget, guiEvent_t *event)
{
    uint8_t limEnabled = checkBox_ApplyLowLimit.isChecked;
    if (setupView.view == VIEW_VOLTAGE)
        guiTop_ApplyGuiVoltageLimit(setupView.channel, LIMIT_TYPE_LOW, limEnabled, spinBox_LowLimit.value * 10);
    else if (setupView.view == VIEW_CURRENT)
        guiTop_ApplyGuiCurrentLimit(setupView.channel, setupView.current_range, LIMIT_TYPE_LOW, limEnabled, spinBox_LowLimit.value * 10);
    return 0;
}

static uint8_t onHighLimitChanged(void *widget, guiEvent_t *event)
{
    uint8_t limEnabled = checkBox_ApplyHighLimit.isChecked;
    if (setupView.view == VIEW_VOLTAGE)
        guiTop_ApplyGuiVoltageLimit(setupView.channel, LIMIT_TYPE_HIGH, limEnabled, spinBox_HighLimit.value * 10);
    else if (setupView.view == VIEW_CURRENT)
        guiTop_ApplyGuiCurrentLimit(setupView.channel, setupView.current_range, LIMIT_TYPE_HIGH, limEnabled, spinBox_HighLimit.value * 10);
    return 0;
}


static void setGuiLimitWidgets(uint8_t limit_type, uint8_t isEnabled, int32_t value)
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

//------------------------------------------------------//
//                  Overload                            //
//------------------------------------------------------//

static uint8_t onOverloadSettingsChanged(void *widget, guiEvent_t *event)
{
    // Threshold in units of 100us
    uint8_t protectionEnabled = checkBox_OverloadProtect.isChecked;
    uint8_t warningEnabled = checkBox_OverloadWarning.isChecked;
    guiTop_ApplyGuiOverloadSettings( protectionEnabled, warningEnabled, spinBox_OverloadThreshold.value);
    return 0;
}

static void setGuiOverloadSettings(uint8_t protectionEnabled, uint8_t warningEnabled, int32_t threshold)
{
    // Threshold in units of 100us
    guiCheckbox_SetChecked(&checkBox_OverloadProtect, protectionEnabled, 0);
    guiCheckbox_SetChecked(&checkBox_OverloadWarning, warningEnabled, 0);
    guiSpinBox_SetValue(&spinBox_OverloadThreshold, threshold, 0);
}


//------------------------------------------------------//
//                  Profiles                            //
//------------------------------------------------------//

static uint8_t onProfileSettingsChanged(void *widget, guiEvent_t *event)
{
    uint8_t saveRecentProfile = checkBox_ProfileSetup1.isChecked;
    uint8_t restoreRecentProfile = checkBox_ProfileSetup2.isChecked;
    guiTop_ApplyGuiProfileSettings(saveRecentProfile, restoreRecentProfile);
    return 0;
}

static void setGuiProfileSettings(uint8_t saveRecentProfile, uint8_t restoreRecentProfile)
{
    guiCheckbox_SetChecked(&checkBox_ProfileSetup1, saveRecentProfile, 0);
    guiCheckbox_SetChecked(&checkBox_ProfileSetup2, restoreRecentProfile, 0);
}

static void setGuiProfileRecordState(uint8_t index, uint8_t profileState, char *name)
{
    switch (profileState)
    {
        case EE_PROFILE_CRC_ERROR:
            snprintf(profileList.strings[index], EE_PROFILE_NAME_SIZE, "<empty>");
            break;
        case EE_PROFILE_HW_ERROR:
            snprintf(profileList.strings[index], EE_PROFILE_NAME_SIZE, "<hw n/a>");
            break;
        default:
            snprintf(profileList.strings[index], EE_PROFILE_NAME_SIZE, "%s", name);
            break;
    }
    profileList.redrawRequired = 1;
    profileList.redrawForced = 1;
}



//------------------------------------------------------//
//			External switch settings					//
//------------------------------------------------------//
static uint8_t onExtSwitchSettingsChanged(void *widget, guiEvent_t *event)
{
    uint8_t enable, inverse, mode;
    // Skip radioButton uncheck
    if ( (((guiGenericWidget_t *)widget)->type != WT_RADIOBUTTON) || (((guiRadioButton_t *)widget)->isChecked != 0) )
    {
        enable = checkBox_ExtSwitchEnable.isChecked;
        inverse = checkBox_ExtSwitchInverse.isChecked;
        mode = (radioBtn_ExtSwitchMode1.isChecked) ? EXTSW_DIRECT :
                    (radioBtn_ExtSwitchMode2.isChecked ? EXTSW_TOGGLE : EXTSW_TOGGLE_OFF);
        guiTop_ApplyExtSwitchSettings(enable, inverse, mode);
    }
    return 0;
}

static void setGuiExtSwitchSettings(uint8_t enable, uint8_t inverse, uint8_t mode)
{
    guiRadioButton_t *btn;
    guiCheckbox_SetChecked(&checkBox_ExtSwitchEnable, enable, 0);
    guiCheckbox_SetChecked(&checkBox_ExtSwitchInverse, inverse, 0);
    btn = (mode == EXTSW_DIRECT) ? &radioBtn_ExtSwitchMode1 :
          ((mode == EXTSW_TOGGLE) ? &radioBtn_ExtSwitchMode2 : &radioBtn_ExtSwitchMode3);
    guiRadioButton_CheckExclusive(btn, 0);
}


//------------------------------------------------------//
//			DAC offset settings							//
//------------------------------------------------------//
static uint8_t onDacSettingsChanged(void *widget, guiEvent_t *event)
{
	guiTop_ApplyDacSettings(spinBox_VoltageDacOffset.value,
							spinBox_CurrentLowDacOffset.value,
							spinBox_CurrentHighDacOffset.value);
    return 0;
}

static void setGuiDacSettings(int8_t v_offset, int8_t c_low_offset, int8_t c_high_offset)
{
    guiSpinBox_SetValue(&spinBox_VoltageDacOffset, v_offset, 0);			// [mV]
    guiSpinBox_SetValue(&spinBox_CurrentLowDacOffset, c_low_offset, 0);		// [mA]
    guiSpinBox_SetValue(&spinBox_CurrentHighDacOffset, c_high_offset, 0);	// [mA]
}



//------------------------------------------------------//
//              UART settings							//
//------------------------------------------------------//
static uint8_t onUartSettingsChanged(void *widget, guiEvent_t *event)
{
    reqUartSettings_t s;
    guiSelectTextBox_t *box = (guiSelectTextBox_t *)widget;
    if ((((guiGenericWidget_t*)widget)->type == WT_CHECKBOX) || ((box->isActive == 0) && (box->newIndexAccepted)))
	//if (((box->isActive == 0) && (box->newIndexAccepted)))
    {
        s.enable = checkBox_UartEnable.isChecked;
        s.parity = ((uint8_t *)selectTextBox_UartParity.valueList)[selectTextBox_UartParity.selectedIndex];
        s.brate = ((uint32_t *)selectTextBox_UartBaudrate.valueList)[selectTextBox_UartBaudrate.selectedIndex];
        s.uart_num = setupView.uart_num;
        guiTop_ApplyUartSettings(&s);
    }
    return 0;
}



//==========================================================================//
//==========================================================================//
//                    The NEW interface to TOP level                        //
//                                                                          //
//==========================================================================//


void guiUpdateVoltageLimit(uint8_t channel, uint8_t limit_type)
{
    uint16_t value;
    uint8_t enable;
    // CHECKME - add visibility check ?
    if (channel == setupView.channel)
    {
        taskENTER_CRITICAL();
        if (limit_type & UPDATE_LOW_LIMIT)
        {
            value = Converter_GetVoltageLimitSetting(setupView.channel, LIMIT_TYPE_LOW);
            enable = Converter_GetVoltageLimitState(setupView.channel, LIMIT_TYPE_LOW);
            setGuiLimitWidgets(LIMIT_TYPE_LOW, enable, value);
        }
        if (limit_type & UPDATE_HIGH_LIMIT)
        {
            value = Converter_GetVoltageLimitSetting(setupView.channel, LIMIT_TYPE_HIGH);
            enable = Converter_GetVoltageLimitState(setupView.channel, LIMIT_TYPE_HIGH);
            setGuiLimitWidgets(LIMIT_TYPE_HIGH, enable, value);
        }
        taskEXIT_CRITICAL();
    }
}


void guiUpdateCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t limit_type)
{
    uint16_t value;
    uint8_t enable;
    // CHECKME - add visibility check ?
    if ((channel == setupView.channel) && (current_range == setupView.current_range))
    {
        taskENTER_CRITICAL();
        if (limit_type & UPDATE_LOW_LIMIT)
        {
            value = Converter_GetCurrentLimitSetting(setupView.channel, setupView.current_range, LIMIT_TYPE_LOW);
            enable = Converter_GetCurrentLimitState(setupView.channel, setupView.current_range, LIMIT_TYPE_LOW);
            setGuiLimitWidgets(LIMIT_TYPE_LOW, enable, value);
        }
        if (limit_type & UPDATE_HIGH_LIMIT)
        {
            value = Converter_GetCurrentLimitSetting(setupView.channel, setupView.current_range, LIMIT_TYPE_HIGH);
            enable = Converter_GetCurrentLimitState(setupView.channel, setupView.current_range, LIMIT_TYPE_HIGH);
            setGuiLimitWidgets(LIMIT_TYPE_HIGH, enable, value);
        }
        taskEXIT_CRITICAL();
    }
}


void guiUpdateOverloadSettings(void)
{
    uint8_t protection_enable, warning_enable;
    uint16_t threshold;
    taskENTER_CRITICAL();
    protection_enable = Converter_GetOverloadProtectionState();
    warning_enable = Converter_GetOverloadProtectionWarning();
    threshold = Converter_GetOverloadProtectionThreshold();
    taskEXIT_CRITICAL();
    setGuiOverloadSettings(protection_enable, warning_enable, threshold);
}


void guiUpdateProfileSettings(void)
{
    uint8_t saveRecentProfile, restoreRecentProfile;
    taskENTER_CRITICAL();
    saveRecentProfile = EE_IsRecentProfileSavingEnabled();
    restoreRecentProfile = EE_IsRecentProfileRestoreEnabled();
    taskEXIT_CRITICAL();
    setGuiProfileSettings(saveRecentProfile, restoreRecentProfile);
}


void guiUpdateProfileListRecord(uint8_t index)
{
    uint8_t profileState;
    profileState = readProfileListRecordName(index, profileName);
    setGuiProfileRecordState(index, profileState, profileName);
}


void guiUpdateProfileList(void)
{
    uint8_t i, profileState;;
    for (i = 0; i < EE_PROFILES_COUNT; i++)
    {
        profileState = readProfileListRecordName(i, profileName);
        setGuiProfileRecordState(i, profileState, profileName);
    }
}


void guiUpdateExtswitchSettings(void)
{
    uint8_t enable, inverse, mode;
    taskENTER_CRITICAL();
    enable = BTN_IsExtSwitchEnabled();
    inverse = BTN_GetExtSwitchInversion();
    mode = BTN_GetExtSwitchMode();
    taskEXIT_CRITICAL();
    setGuiExtSwitchSettings(enable, inverse, mode);
}


void guiUpdateDacSettings(void)
{
    int8_t v_offset, c_low_offset, c_high_offset;
    taskENTER_CRITICAL();
    v_offset = Converter_GetVoltageDacOffset();
    c_low_offset = Converter_GetCurrentDacOffset(CURRENT_RANGE_LOW);
    c_high_offset = Converter_GetCurrentDacOffset(CURRENT_RANGE_HIGH);
    taskEXIT_CRITICAL();
    setGuiDacSettings(v_offset, c_low_offset, c_high_offset);
}



//-------------------------------------------------------//

void guiUpdateUartSettings(uint8_t uart_num)
{
    reqUartSettings_t req;
    void *p_temp;
    uint8_t i;
    if (uart_num == setupView.uart_num)
    {
        req.uart_num = uart_num;
        guiTop_GetUartSettings(&req);
        guiCheckbox_SetChecked(&checkBox_UartEnable, req.enable, 0);
        // Set parity
        p_temp = selectTextBox_UartParity.valueList;
        for (i = 0; i < selectTextBox_UartParity.stringCount; i++)
        {
            if (((uint8_t *)p_temp)[i] == req.parity)
            {
                guiSelectTextBox_SetIndex(&selectTextBox_UartParity, i, 0);
                break;
            }
        }
        // Set baudrate
        p_temp = selectTextBox_UartBaudrate.valueList;
        for (i = 0; i < selectTextBox_UartBaudrate.stringCount; i++)
        {
            if (((uint32_t *)p_temp)[i] == req.brate)
            {
                guiSelectTextBox_SetIndex(&selectTextBox_UartBaudrate, i, 0);
                break;
            }
        }
    }
}













