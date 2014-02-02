#ifndef __GUI_TEXT_LABEL_H_
#define __GUI_TEXT_LABEL_H_

#include <stdint.h>
#include "guiWidgets.h"



// Widget-specific state checks
#define TEXTLABEL_ACCEPTS_FOCUS_EVENT(label)  ( (label->isVisible) && (1) )    // TODO - add isEnabled


void guiTextLabel_Initialize(guiTextLabel_t *textLabel, guiGenericWidget_t *parent);
uint8_t guiTextLabel_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);



#endif
