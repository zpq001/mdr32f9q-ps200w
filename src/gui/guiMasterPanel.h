#ifndef __GUI_MASTER_PANEL_H_
#define __GUI_MASTER_PANEL_H_

#include <stdint.h>
#include "guiWidgets.h"




extern guiPanel_t     guiMasterPanel;


void guiMasterPanel_Initialize(guiGenericWidget_t *parent);


void setVoltageIndicator(uint16_t value);
void setVoltageSetting(uint16_t value);

void setCurrentIndicator(uint16_t value);
void setCurrentSetting(uint16_t value);

void setPowerIndicator(uint32_t value);
void setTemperatureIndicator(int16_t value);
void setFeedbackChannelIndicator(uint8_t channel);
void setCurrentLimitIndicator(uint8_t current_limit);


#endif
