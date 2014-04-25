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
void showEditPanel1(void);
void hideEditPanel1(void);


void guiUpdateVoltageSetting(uint8_t channel);
void guiUpdateCurrentSetting(uint8_t channel, uint8_t current_range);
void guiUpdateCurrentRange(uint8_t channel);
void guiUpdateChannel(void);
void guiUpdateAdcIndicators(void);
void guiUpdateTemperatureIndicator(void);





#endif
