/**********************************************************
    Module guiEditPanel




**********************************************************/


#include <stdio.h>      // due to printf
#include <stdint.h>

#include "guiFonts.h"
#include "guiGraphHAL.h"
#include "guiGraphPrimitives.h"
#include "guiGraphWidgets.h"
#include "guiImages.h"

#include "guiCore.h"
#include "guiEvents.h"
#include "guiWidgets.h"
#include "guiPanel.h"
#include "guiTextLabel.h"
#include "guiSpinBox.h"

#include "guiMessagePanel1.h"
//#include "guiMasterPanel.h"

//#include "guiTop.h"




static uint8_t guiMessagePanel1_onDraw(void *widget, guiEvent_t *event);
static uint8_t guiMessagePanel1_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
static uint8_t guiMessagePanel1_onVisibleChanged(void *widget, guiEvent_t *event);
static uint8_t guiMessagePanel1_onTimer(void *widget, guiEvent_t *event);


//--------- Edit panel 1  ----------//
#define MESSAGE_PANEL1_ELEMENTS_COUNT 10
guiPanel_t     guiMessagePanel1;
static void *guiMessagePanel1Elements[MESSAGE_PANEL1_ELEMENTS_COUNT];
guiWidgetHandler_t messagePanelHandlers[3];


//--------- Panel elements ---------//


messageView_t messageView;

void guiMessagePanel1_Initialize(guiGenericWidget_t *parent)
{
    // Initialize
    guiPanel_Initialize(&guiMessagePanel1, parent);
    guiMessagePanel1.widgets.count = MESSAGE_PANEL1_ELEMENTS_COUNT;
    guiMessagePanel1.widgets.elements = guiMessagePanel1Elements;
    guiMessagePanel1.x = 5;
    guiMessagePanel1.y = 5;
    guiMessagePanel1.width = 96-10;
    guiMessagePanel1.height = 68-15;
    guiMessagePanel1.showFocus = 0;
    guiMessagePanel1.focusFallsThrough = 0;
    guiMessagePanel1.keyTranslator = &guiMessagePanel1_KeyTranslator;
    guiMessagePanel1.handlers.count = 3;
    guiMessagePanel1.handlers.elements = messagePanelHandlers;
    guiMessagePanel1.handlers.elements[0].eventType = GUI_EVENT_DRAW;
    guiMessagePanel1.handlers.elements[0].handler = *guiMessagePanel1_onDraw;
    guiMessagePanel1.handlers.elements[1].eventType = GUI_ON_VISIBLE_CHANGED;
    guiMessagePanel1.handlers.elements[1].handler = *guiMessagePanel1_onVisibleChanged;
    guiMessagePanel1.handlers.elements[2].eventType = GUI_EVENT_TIMER;
    guiMessagePanel1.handlers.elements[2].handler = *guiMessagePanel1_onTimer;
    guiMessagePanel1.isVisible = 0;
    guiMessagePanel1.frame = 0;

    guiCore_TimerInit(GUI_MESSAGE_PANEL_TIMER, 20, TMR_RUN_ONCE, (guiGenericWidget_t *)&guiMessagePanel1, 0);
}




static uint8_t guiMessagePanel1_onDraw(void *widget, guiEvent_t *event)
{
    if (guiMessagePanel1.redrawForced)
    {
        // Draw frame
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_SetLineStyle(LINE_STYLE_SOLID);
        LCD_DrawRect(wx,wy,guiMessagePanel1.width,guiMessagePanel1.height,1);

        LCD_SetFont(&font_h10);

		// Draw icon
        // Draw caption text
        switch (messageView.type)
        {
            case MESSAGE_TYPE_INFO:
                LCD_PrintString("Information", wx + 20, wy + 2, IMAGE_MODE_NORMAL);
                break;
            case MESSAGE_TYPE_WARNING:
                LCD_PrintString("Warning", wx + 20, wy + 2, IMAGE_MODE_NORMAL);
                break;
            default:
                LCD_PrintString("Error", wx + 20, wy + 2, IMAGE_MODE_NORMAL);
                break;
        }
    }
    return 0;
}


static uint8_t guiMessagePanel1_onVisibleChanged(void *widget, guiEvent_t *event)
{
    if (guiMessagePanel1.isVisible)
    {
        guiCore_TimerStart(GUI_MESSAGE_PANEL_TIMER, TMR_DO_RESET);
    }
    return 0;
}


static uint8_t guiMessagePanel1_onTimer(void *widget, guiEvent_t *event)
{
    if (guiMessagePanel1.isVisible)
    {
        guiCore_SetVisible((guiGenericWidget_t *)&guiMessagePanel1, 0);
        if ((guiMessagePanel1.isFocused) && (messageView.lastFocused))
            guiCore_RequestFocusChange(messageView.lastFocused);
    }
    return 0;
}


static uint8_t guiMessagePanel1_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
	uint8_t exit_code = 0;
    guiPanelTranslatedKey_t *tkey = (guiPanelTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    
    if (event->spec == GUI_KEY_EVENT_UP_SHORT)
    {
        if (event->lparam == GUI_KEY_ESC)
            exit_code = 1;
    }
    if (exit_code)
    {
        guiMessagePanel1_onTimer(0, 0);
    }
    return GUI_EVENT_ACCEPTED;		// block keys
}






