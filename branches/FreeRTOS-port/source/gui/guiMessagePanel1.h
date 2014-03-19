#ifndef __GUI_MESSAGE_PANEL1_H_
#define __GUI_MESSAGE_PANEL1_H_

#include <stdint.h>
#include "guiWidgets.h"


typedef struct {
    uint8_t mode;
    guiGenericWidget_t *lastFocused;
} messageView_t;

extern messageView_t messageView;

extern guiPanel_t     guiMessagePanel1;


void guiMessagePanel1_Initialize(guiGenericWidget_t *parent);



#endif
