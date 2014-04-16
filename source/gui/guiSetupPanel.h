#ifndef __GUI_SETUP_PANEL_H_
#define __GUI_SETUP_PANEL_H_

#include <stdint.h>
#include "guiWidgets.h"


extern guiPanel_t     guiSetupPanel;


void guiSetupPanel_Initialize(guiGenericWidget_t *parent);


// Voltage/Current limits
void setGuiVoltageLimitSetting(uint8_t channel, uint8_t limit_type, uint8_t isEnabled, int32_t value);
void setGuiCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type, uint8_t isEnabled, int32_t value);
// Overload
void setGuiOverloadSettings(uint8_t protectionEnable, uint8_t warningEnabled, int32_t threshold);
// Profiles
void setGuiProfileSettings(uint8_t saveRecentProfile, uint8_t restoreRecentProfile);
void setGuiProfileRecordState(uint8_t i, uint8_t profileState, char *name);
// External switch
void setGuiExtSwitchSettings(uint8_t enable, uint8_t inverse, uint8_t mode);
// DAC
void setGuiDacSettings(int8_t v_offset, int8_t c_low_offset, int8_t c_high_offset);


void hideEditPanel2(char *newProfileName);  // called by edit panel 2

#endif
