#ifndef __GUI_SPINBOX_H_
#define __GUI_SPINBOX_H_

#include "guiWidgets.h"
#include "guiFonts.h"

// Widget type ID - must be unique!
#define WT_SPINBOX      0x06

#define SPINBOX_STRING_LENGTH  12  // long enough to hold INT32_MAX and INT32_MIN + \0

typedef struct guiSpinBox_t {
    //----- Inherited from generic widget -----//
    GENERIC_WIDGET_PATTERN
    //------- Widget - specific fields --------//
    //char text[SPINBOX_STRING_LENGTH];
    const tFont *font;
    //uint8_t textAlignment;
    uint8_t hasFrame : 1;
    uint8_t redrawValue : 1;
    uint8_t redrawDigitSelection : 1;
    uint8_t isActive : 1;
    uint8_t restoreValueOnEscape : 1;
    uint8_t newValueAccepted : 1;
    uint8_t minDigitsToDisplay;
    uint8_t activeDigit;
    int8_t dotPosition;
    int32_t value;
    int32_t savedValue;
    int32_t maxValue;
    int32_t minValue;
    char text[SPINBOX_STRING_LENGTH];
    uint8_t digitsToDisplay;
    int8_t textRightOffset;
    int8_t textTopOffset;
} guiSpinBox_t;


// Widget-specific virtual keys
enum guiSpinboxVirtualKeys {
    SPINBOX_KEY_SELECT = 0x01,
    SPINBOX_KEY_EXIT,
    SPINBOX_KEY_RIGHT,
    SPINBOX_KEY_LEFT
};

// Translated key event struct
typedef struct {
    uint8_t key;
    int16_t increment;
} guiSpinboxTranslatedKey_t;





// Widget-specific events
#define SPINBOX_EVENT_ACTIVATE  (0x40 + 0x00)

#define SPINBOX_ACTIVE_CHANGED          (0xC0 + 0x00)
#define SPINBOX_VALUE_CHANGED           (0xC0 + 0x01)
#define SPINBOX_ACTIVE_DIGIT_CHANGED    (0xC0 + 0x03)


// Widget-specific state checks
#define SPINBOX_ACCEPTS_FOCUS_EVENT(spinBox)  ( (spinBox->isVisible) && (1) )    // TODO - add isEnabled
#define SPINBOX_ACCEPTS_KEY_EVENT(spinBox)    ( (spinBox->isFocused) && \
                                                  (spinBox->isVisible) )         // TODO - add isEnabled




uint8_t guiSpinBox_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);

void guiSpinBox_Initialize(guiSpinBox_t *spinBox, guiGenericWidget_t *parent);
void guiSpinBox_SetValue(guiSpinBox_t *spinBox, int32_t value, uint8_t callHandler);
void guiSpinBox_SetActiveDigit(guiSpinBox_t *spinBox, int8_t num);



#endif
