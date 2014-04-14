#ifndef __GUI_SETUP_PANEL_H_
#define __GUI_SETUP_PANEL_H_

#include <stdint.h>
#include "guiWidgets.h"


extern guiPanel_t     guiSetupPanel;


void guiSetupPanel_Initialize(guiGenericWidget_t *parent);



void setGuiVoltageLimitSetting(uint8_t channel, uint8_t limit_type, uint8_t isEnabled, int32_t value);
void setGuiCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type, uint8_t isEnabled, int32_t value);
void setGuiOverloadSetting(uint8_t protectionEnable, uint8_t warningEnabled, int32_t threshold);
void updateGuiProfileListRecord(uint8_t i, uint8_t profileState, char *name);

void updateProfileSetup(void);
void updateExtSwitchSettings(uint8_t updateForced);


void setGuiDacSettings(int8_t v_offset, int8_t c_low_offset, int8_t c_high_offset);


void hideEditPanel2(char *newProfileName);  // called by edit panel 2

#endif
