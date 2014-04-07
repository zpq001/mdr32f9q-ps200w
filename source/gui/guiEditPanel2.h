#ifndef __GUI_EDIT_PANEL2_H_
#define __GUI_EDIT_PANEL2_H_

#include <stdint.h>
#include "guiWidgets.h"



extern guiPanel_t     guiEditPanel2;
void guiEditPanel2_Initialize(guiGenericWidget_t *parent);


void guiEditPanel2_Show(char *textToEdit);


#endif
