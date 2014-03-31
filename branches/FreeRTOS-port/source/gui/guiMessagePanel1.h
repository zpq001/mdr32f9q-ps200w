#ifndef __GUI_MESSAGE_PANEL1_H_
#define __GUI_MESSAGE_PANEL1_H_

#include <stdint.h>
#include "guiWidgets.h"


typedef struct {
    uint8_t type;
    uint8_t code;
    guiGenericWidget_t *lastFocused;
} messageView_t;

enum messageViewTypes {
    MESSAGE_TYPE_INFO,
    MESSAGE_TYPE_WARNING,
    MESSAGE_TYPE_ERROR
};

enum messageViewCodes {
    MESSAGE_PROFILE_LOADED,
    MESSAGE_PROFILE_SAVED,
    MESSAGE_PROFILE_CRC_ERROR,
    MESSAGE_PROFILE_HW_ERROR,
    MESSAGE_PROFILE_LOADED_DEFAULT,
    MESSAGE_PROFILE_RESTORED_RECENT
};



extern messageView_t messageView;

extern guiPanel_t     guiMessagePanel1;


void guiMessagePanel1_Initialize(guiGenericWidget_t *parent);
void guiMessagePanel1_Show(uint8_t msgType, uint8_t msgCode, uint8_t takeFocus, uint8_t timeout);


#endif
