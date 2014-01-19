#ifndef __GUI_SETUP_PANEL_H_
#define __GUI_SETUP_PANEL_H_

#include <stdint.h>
#include "guiWidgets.h"


extern guiPanel_t     guiSetupPanel;


void guiSetupPanel_Initialize(guiGenericWidget_t *parent);

void setLowVoltageLimitSetting(uint8_t isEnabled, uint16_t value);
void setHighVoltageLimitSetting(uint8_t isEnabled, uint16_t value);



#endif
