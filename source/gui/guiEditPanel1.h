#ifndef __GUI_EDIT_PANEL1_H_
#define __GUI_EDIT_PANEL1_H_

#include <stdint.h>
#include "guiWidgets.h"


#define EDIT_MODE_VOLTAGE 0
#define EDIT_MODE_CURRENT 1

typedef struct {
    uint8_t mode;
    uint8_t channel;
    uint8_t current_range;
    uint8_t active_digit;
} editView_t;

extern editView_t editView;

extern guiPanel_t     guiEditPanel1;


void guiEditPanel1_Initialize(guiGenericWidget_t *parent);



#endif
