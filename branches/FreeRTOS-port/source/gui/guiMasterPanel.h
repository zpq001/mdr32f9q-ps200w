#ifndef __GUI_MASTER_PANEL_H_
#define __GUI_MASTER_PANEL_H_

#include <stdint.h>
#include "guiWidgets.h"




typedef struct {
    uint8_t channel;
    uint8_t current_range;

} masterViev_t;

extern masterViev_t masterView;
extern guiPanel_t     guiMasterPanel;


void guiMasterPanel_Initialize(guiGenericWidget_t *parent);

void setGuiVoltageIndicator(uint16_t value);
void setGuiCurrentIndicator(uint16_t value);
void setGuiPowerIndicator(uint32_t value);
void setGuiTemperatureIndicator(int16_t value);
void setGuiVoltageSetting(uint8_t channel, int32_t value);
void setGuiCurrentSetting(uint8_t channel, uint8_t range, int32_t value);
void setGuiFeedbackChannel(uint8_t channel);
void setGuiCurrentRange(uint8_t channel, uint8_t range);


void showEditPanel1(void);
void hideEditPanel1(void);


#endif
