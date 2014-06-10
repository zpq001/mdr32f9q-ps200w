#ifndef __GUI_SETUP_PANEL_H_
#define __GUI_SETUP_PANEL_H_

#include <stdint.h>
#include "guiWidgets.h"



// These values can be ORed and specify limit types to update
#define UPDATE_LOW_LIMIT    0x01
#define UPDATE_HIGH_LIMIT   0x02

// These special values are used as arguments for voltage or current limit update when
// limits for view.channel and view.current_range should be updated
#define CHANNEL_AUTO        0x10    // any value but not defined channel values
#define CURRENT_RANGE_AUTO  0x10    // same - any value but not defined current range values


extern guiPanel_t     guiSetupPanel;

void guiSetupPanel_Initialize(guiGenericWidget_t *parent);
void hideEditPanel2(char *newProfileName);  // called by edit panel 2

// Voltage/Current limits
void guiUpdateVoltageLimit(uint8_t channel, uint8_t limit_type);
void guiUpdateCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t limit_type);
// Overload
void guiUpdateOverloadSettings(void);
// Profiles
void guiUpdateProfileSettings(void);
void guiUpdateProfileListRecord(uint8_t index);
void guiUpdateProfileList(void);
// External switch
void guiUpdateExtswitchSettings(void);
// DAC
void guiUpdateDacSettings(void);
// UART
void guiUpdateUartSettings(uint8_t uart_num);


#endif
