#ifndef __GUI_SETUP_PANEL_H_
#define __GUI_SETUP_PANEL_H_

#include <stdint.h>
#include "guiWidgets.h"


extern guiPanel_t     guiSetupPanel;


void guiSetupPanel_Initialize(guiGenericWidget_t *parent);

void setLowVoltageLimitSetting(uint8_t channel, uint8_t isEnabled, int32_t value);
void setHighVoltageLimitSetting(uint8_t channel, uint8_t isEnabled, int32_t value);
void setLowCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t isEnabled, int32_t value);
void setHighCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t isEnabled, int32_t value);


#endif
