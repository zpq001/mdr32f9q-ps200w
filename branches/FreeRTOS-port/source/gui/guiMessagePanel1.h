#ifndef __GUI_MESSAGE_PANEL1_H_
#define __GUI_MESSAGE_PANEL1_H_

#include <stdint.h>
#include "guiWidgets.h"


typedef struct {
    uint8_t type;
    uint8_t code;
    guiGenericWidget_t *lastFocused;
} messageView_t;

enum messageViewTypes {
    MESSAGE_TYPE_INFO,
    MESSAGE_TYPE_WARNING,
    MESSAGE_TYPE_ERROR
};





extern messageView_t messageView;

extern guiPanel_t     guiMessagePanel1;


void guiMessagePanel1_Initialize(guiGenericWidget_t *parent);



#endif
