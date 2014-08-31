#ifndef __GUI_TEXTSPINBOX_H_
#define __GUI_TEXTSPINBOX_H_

#include "guiWidgets.h"
#include "guiFonts.h"

// Widget type ID - must be unique!
#define WT_TEXTSPINBOX      0x08


typedef struct guiTextSpinBox_t {
    //----- Inherited from generic widget -----//
    GENERIC_WIDGET_PATTERN
    //------- Widget - specific fields --------//
    const tFont *font;
    uint8_t textAlignment;
    uint8_t hasFrame : 1;
    uint8_t redrawText : 1;
    uint8_t redrawCharSelection : 1;
    uint8_t isActive : 1;
    uint8_t activeChar;
    char *text;
    uint8_t textLength;
    int8_t textLeftOffset;
    int8_t textTopOffset;
} guiTextSpinBox_t;



// Widget-specific virtual keys
enum guiTextSpinboxVirtualKeys {
    TEXTSPINBOX_KEY_SELECT = 0x01,
    TEXTSPINBOX_KEY_EXIT,
    TEXTSPINBOX_KEY_RIGHT,
    TEXTSPINBOX_KEY_LEFT,
    TEXTSPINBOX_KEY_UP,
    TEXTSPINBOX_KEY_DOWN
};

// Translated key event struct
typedef struct {
    uint8_t key;
    int8_t increment;
} guiTextSpinBoxTranslatedKey_t;





// Widget-specific events
#define TEXTSPINBOX_EVENT_ACTIVATE  		(0x40 + 0x00)

#define TEXTSPINBOX_ACTIVE_CHANGED          (0xC0 + 0x00)
#define TEXTSPINBOX_TEXT_CHANGED           (0xC0 + 0x01)
#define TEXTSPINBOX_ACTIVE_CHAR_CHANGED   (0xC0 + 0x03)


// Widget-specific state checks
#define TEXTSPINBOX_ACCEPTS_FOCUS_EVENT(textSpinBox)  ( (textSpinBox->isVisible) && (1) )    // TODO - add isEnabled
#define TEXTSPINBOX_ACCEPTS_KEY_EVENT(textSpinBox)    ( (textSpinBox->isFocused) && \
                                                  (textSpinBox->isVisible) )         // TODO - add isEnabled



uint8_t guiTextSpinBox_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);
void guiTextSpinBox_Initialize(guiTextSpinBox_t *spinBox, guiGenericWidget_t *parent);
void guiTextSpinBox_SetText(guiTextSpinBox_t *textSpinBox, char *newText, uint8_t callHandler);
void guiTextSpinBox_SetActiveChar(guiTextSpinBox_t *textSpinBox, char newChar, uint8_t callHandler);

#endif
