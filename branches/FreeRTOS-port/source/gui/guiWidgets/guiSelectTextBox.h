#ifndef __GUI_SELECT_TEXT_BOX_H_
#define __GUI_SELECT_TEXT_BOX_H_

#include "guiWidgets.h"
#include "guiFonts.h"

// Widget type ID - must be unique!
#define WT_SELECTTEXTBOX    0x09


typedef struct guiSelectTextBox_t {
    //----- Inherited from generic widget -----//
    GENERIC_WIDGET_PATTERN
    //------- Widget - specific fields --------//
    const tFont *font;
    uint8_t hasFrame : 1;
    uint8_t redrawText : 1;
    uint8_t isActive : 1;
    uint8_t restoreIndexOnEscape : 1;
    uint8_t newIndexAccepted : 1;

    uint8_t selectedIndex;
    uint8_t savedIndex;
    uint8_t stringCount;
    char **stringList;
    void *valueList;
} guiSelectTextBox_t;


// Widget-specific events
#define SELECTTEXTBOX_EVENT_ACTIVATE  (0x40 + 0x00)

#define SELECTTEXTBOX_ACTIVE_CHANGED          (0xC0 + 0x00)
#define SELECTTEXTBOX_INDEX_CHANGED           (0xC0 + 0x01)

// Widget-specific state checks
#define SELECTTEXTBOX_ACCEPTS_FOCUS_EVENT(sbox)  ( (sbox->isVisible) && (1) )    // TODO - add isEnabled
#define SELECTTEXTBOX_ACCEPTS_KEY_EVENT(sbox)    ( (sbox->isFocused) && \
                                                  (sbox->isVisible) )         // TODO - add isEnabled

// Widget-specific virtual keys
enum guiSelectTextBoxVirtualKeys {
    SELECTTEXTBOX_KEY_SELECT = 0x01,
    SELECTTEXTBOX_KEY_EXIT,
    SELECTTEXTBOX_KEY_NEXT,
    SELECTTEXTBOX_KEY_PREV
};

// Translated key event struct
typedef struct {
    uint8_t key;
} guiSelectTextBoxTranslatedKey_t;


void guiSelectTextBox_Initialize(guiSelectTextBox_t *selectTextBox, guiGenericWidget_t *parent);
uint8_t guiSelectTextBox_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);
void guiSelectTextBox_SetIndex(guiSelectTextBox_t *selectTextBox, int16_t index, uint8_t callHandler);

#endif
