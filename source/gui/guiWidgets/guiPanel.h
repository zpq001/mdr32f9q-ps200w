#ifndef __GUI_PANEL_H_
#define __GUI_PANEL_H_

#include <stdint.h>
#include "guiWidgets.h"



// Widget-specific virtual keys
enum guiPanelVirtualKeys {
    PANEL_KEY_SELECT = 0x01,
    PANEL_KEY_ESC,
    PANEL_KEY_PREV,
    PANEL_KEY_NEXT
};

// Translated key event struct
typedef struct {
    uint8_t key;
} guiPanelTranslatedKey_t;


// Widget-specific state checks
#define PANEL_ACCEPTS_FOCUS_EVENT(panel)  ( (panel->isVisible) && (1) )    // TODO - add isEnabled
#define PANEL_ACCEPTS_KEY_EVENT(panel)    ( (panel->isVisible) )           // TODO - add isEnabled
#define PANEL_ACCEPTS_TOUCH_EVENT(panel)  ( (panel->isVisible) )           // TODO - add isEnabled


void guiPanel_Initialize(guiPanel_t *panel, guiGenericWidget_t *parent);
uint8_t guiPanel_ProcessKey(guiPanel_t *panel, uint8_t key);
uint8_t guiPanel_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event);




#endif
