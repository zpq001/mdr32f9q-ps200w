#ifndef __GUI_STRINGLIST_H_
#define __GUI_STRINGLIST_H_

#include <stdint.h>
#include "guiWidgets.h"


// Widget-specific virtual keys
/*
#define STRINGLIST_KEY_SELECT     0x01
#define STRINGLIST_KEY_EXIT       0x02
#define STRINGLIST_KEY_UP         0x03
#define STRINGLIST_KEY_DOWN       0x04
*/
enum StringListVirtualKeys {
    STRINGLIST_KEY_SELECT = 0x01,
    STRINGLIST_KEY_EXIT,
    STRINGLIST_KEY_UP,
    STRINGLIST_KEY_DOWN
};


// Widget-specific events
#define STRINGLIST_EVENT_ACTIVATE  (0x40 + 0x00)

#define STRINGLIST_ACTIVE_CHANGED          (0xC0 + 0x00)
#define STRINGLIST_INDEX_CHANGED           (0xC0 + 0x01)



// Widget-specific state checks
#define STRINGLIST_ACCEPTS_FOCUS_EVENT(list)  ( (list->isVisible) && (1) )    // TODO - add isEnabled
#define STRINGLIST_ACCEPTS_KEY_EVENT(list)    ( (list->isFocused) && \
                                                  (list->isVisible) )         // TODO - add isEnabled
#define STRINGLIST_ACCEPTS_ENCODER_EVENT(list) ( (list->isFocused) && \
                                               (list->isVisible) )            // TODO - add isEnabled





uint8_t guiStringList_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);
void guiStringList_Initialize(guiStringList_t *list, guiGenericWidget_t *parent);



#endif
