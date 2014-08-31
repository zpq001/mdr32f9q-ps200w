#ifndef __GUI_TEXT_LABEL_H_
#define __GUI_TEXT_LABEL_H_

#include "guiWidgets.h"
#include "guiFonts.h"


// Widget type ID - must be unique!
#define WT_TEXTLABEL    0x05


typedef struct guiTextLabel_t {
    //----- Inherited from generic widget -----//
    GENERIC_WIDGET_PATTERN
    //------- Widget - specific fields --------//
    char *text;
    const tFont *font;
    uint8_t textAlignment;
    uint8_t hasFrame : 1;
} guiTextLabel_t;

// Widget-specific state checks
#define TEXTLABEL_ACCEPTS_FOCUS_EVENT(label)  ( (label->isVisible) && (1) )    // TODO - add isEnabled


void guiTextLabel_Initialize(guiTextLabel_t *textLabel, guiGenericWidget_t *parent);
uint8_t guiTextLabel_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);
void guiTextLabel_SetText(guiTextLabel_t *textLabel, char *text);


#endif
