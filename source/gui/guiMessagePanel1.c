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


//--------- Edit panel 1  ----------//
#define MESSAGE_PANEL1_ELEMENTS_COUNT 10
guiPanel_t     guiMessagePanel1;
static void *guiMessagePanel1Elements[MESSAGE_PANEL1_ELEMENTS_COUNT];
guiWidgetHandler_t messagePanelHandlers[1];


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
    guiMessagePanel1.height = 68-20;//68-13;
    guiMessagePanel1.showFocus = 0;
    guiMessagePanel1.focusFallsThrough = 0;
    guiMessagePanel1.keyTranslator = &guiMessagePanel1_KeyTranslator;
    guiMessagePanel1.handlers.count = 1;
    guiMessagePanel1.handlers.elements = messagePanelHandlers;
    guiMessagePanel1.handlers.elements[0].eventType = GUI_EVENT_DRAW;
    guiMessagePanel1.handlers.elements[0].handler = *guiMessagePanel1_onDraw;
    guiMessagePanel1.isVisible = 0;
}




static uint8_t guiMessagePanel1_onDraw(void *widget, guiEvent_t *event)
{
    if (guiMessagePanel1.redrawForced)
    {
        // Draw frame
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_SetLineStyle(LINE_STYLE_SOLID);
        LCD_DrawHorLine(wx, wy, guiMessagePanel1.width, 1);
        LCD_DrawHorLine(wx, wy + guiMessagePanel1.height - 1, guiMessagePanel1.width, 1);
        LCD_DrawVertLine(wx, wy, guiMessagePanel1.height, 1);
        LCD_DrawVertLine(wx + guiMessagePanel1.width - 1, wy, guiMessagePanel1.height, 1);

		// Draw icon
		// Draw text
       
    }
    return 0;
}




static uint8_t guiMessagePanel1_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiPanelTranslatedKey_t *tkey = (guiPanelTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    uint8_t exit_code = 0;

    if (event->spec == GUI_KEY_EVENT_UP_SHORT)
    {
        if (event->lparam == GUI_KEY_ESC)
            exit_code = 1;
    }
    if (exit_code)
    {
        guiCore_SetVisible((guiGenericWidget_t *)&guiMessagePanel1, 0);
        guiCore_RequestFocusChange(messageView.lastFocused);
    }
    return GUI_EVENT_ACCEPTED;		// block keys
}






