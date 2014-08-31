#ifndef __GUI_RADIO_BUTTON_H_
#define __GUI_RADIO_BUTTON_H_

#include "guiWidgets.h"
#include "guiFonts.h"

// Widget type ID - must be unique!
#define WT_RADIOBUTTON  0x04


typedef struct guiRadioButton_t {
    //----- Inherited from generic widget -----//
    GENERIC_WIDGET_PATTERN
    //------- Widget - specific fields --------//
    char *text;
    const tFont *font;
    uint8_t textAlignment;
    uint8_t redrawCheckedState : 1;
    uint8_t radioIndex;
    uint8_t isChecked : 1;
} guiRadioButton_t;


// Widget-specific virtual keys
enum guiRadioButtonVirtualKeys {
    RADIOBUTTON_KEY_SELECT = 0x01
};

// Translated key event struct
typedef struct {
    uint8_t key;
} guiRadioButtonTranslatedKey_t;


// Widget-specific events
#define RADIOBUTTON_CHECKED_CHANGED    (0xC0 + 0x00)



// Widget-specific virtual keys
#define RADIOBUTTON_KEY_SELECT     0x01




// Widget-specific state checks
#define RADIOBUTTON_ACCEPTS_FOCUS_EVENT(button)  ( (button->isVisible) && (1) )    // TODO - add isEnabled
#define RADIOBUTTON_ACCEPTS_KEY_EVENT(button)    ( (button->isFocused) && \
                                                   (button->isVisible) )           // TODO - add isEnabled
#define RADIOBUTTON_ACCEPTS_TOUCH_EVENT(button)  ( (button->isVisible) )           // TODO - add isEnabled




void guiRadioButton_Initialize(guiRadioButton_t *button, guiGenericWidget_t *parent);
uint8_t guiRadioButton_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);
void guiRadioButton_CheckExclusive(guiRadioButton_t *button, uint8_t callHandler);





#endif
