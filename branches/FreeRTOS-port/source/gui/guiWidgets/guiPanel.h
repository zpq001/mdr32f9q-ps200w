#ifndef __GUI_PANEL_H_
#define __GUI_PANEL_H_

#include "guiWidgets.h"


// Widget type ID - must be unique!
#define WT_PANEL        0x01


typedef struct guiPanel_t {
    //----- Inherited from generic container -----//
    GENERIC_CONTAINER_PATTERN
    //------- Widget - specific fields --------//
    uint8_t focusFallsThrough : 1;
    uint8_t frame : 3;
    // uint8_t focusIsKeptOnChilds : 1;      // doesn't let unfocus child widgets when focusFallsThrough is set. CHECKME
} guiPanel_t;



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
