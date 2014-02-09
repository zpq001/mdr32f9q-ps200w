#ifndef __GUI_CHECK_BOX_H_
#define __GUI_CHECK_BOX_H_

#include <stdint.h>
#include "guiWidgets.h"


// Widget-specific events
#define CHECKBOX_CHECKED_CHANGED    (0xC0 + 0x00)


// Widget-specific virtual keys
enum guiCheckboxVirtualKeys {
    CHECKBOX_KEY_SELECT = 0x01
};

// Translated key event struct
typedef struct {
    uint8_t key;
} guiCheckboxTranslatedKey_t;




// Widget-specific state checks
#define CHECKBOX_ACCEPTS_FOCUS_EVENT(checkBox)  ( (checkBox->isVisible) && (1) )    // TODO - add isEnabled
#define CHECKBOX_ACCEPTS_KEY_EVENT(checkBox)    ( (checkBox->isFocused) && \
                                                  (checkBox->isVisible) )           // TODO - add isEnabled
#define CHECKBOX_ACCEPTS_TOUCH_EVENT(checkBox)  ( (checkBox->isVisible) )           // TODO - add isEnabled



void guiCheckBox_Initialize(guiCheckBox_t *checkBox, guiGenericWidget_t *parent);
uint8_t guiCheckbox_SetChecked(guiCheckBox_t *checkBox, uint8_t newCheckedState, uint8_t callHandler);
uint8_t guiCheckbox_ProcessKey(guiCheckBox_t *checkBox, uint8_t key);
uint8_t guiCheckBox_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);

void guiCheckBox_SetText(guiCheckBox_t *checkBox, char *text);

#endif
