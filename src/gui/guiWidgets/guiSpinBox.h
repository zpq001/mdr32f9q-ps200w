#ifndef __GUI_SPINBOX_H_
#define __GUI_SPINBOX_H_

#include <stdint.h>
#include "guiWidgets.h"



// Widget-specific virtual keys
#define SPINBOX_KEY_SELECT     0x01
#define SPINBOX_KEY_EXIT       0x02
#define SPINBOX_KEY_RIGHT      0x03
#define SPINBOX_KEY_LEFT       0x04
#define SPINBOX_KEY_UP         0x05
#define SPINBOX_KEY_DOWN       0x06


// Widget-specific events
#define SPINBOX_EVENT_ACTIVATE  (0x40 + 0x00)

#define SPINBOX_ACTIVE_CHANGED          (0xC0 + 0x00)
#define SPINBOX_VALUE_CHANGED           (0xC0 + 0x01)
#define SPINBOX_ACTIVE_DIGIT_CHANGED    (0xC0 + 0x03)


// Widget-specific state checks
#define SPINBOX_ACCEPTS_FOCUS_EVENT(spinBox)  ( (spinBox->isVisible) && (1) )    // TODO - add isEnabled
#define SPINBOX_ACCEPTS_KEY_EVENT(spinBox)    ( (spinBox->isFocused) && \
                                                  (spinBox->isVisible) )         // TODO - add isEnabled
#define SPINBOX_ACCEPTS_ENCODER_EVENT(spinBox) ( (spinBox->isFocused) && \
                                               (spinBox->isVisible) )            // TODO - add isEnabled

uint8_t guiSpinBox_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);

void guiSpinBox_Initialize(guiSpinBox_t *spinBox, guiGenericWidget_t *parent);
void guiSpinBox_SetValue(guiSpinBox_t *spinBox, int32_t value);




#endif
