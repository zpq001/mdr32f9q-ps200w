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

#include "guiTop.h"

#include "guiMasterPanel.h"


static uint8_t guiEditPanel1_onDraw(void *widget, guiEvent_t *event);
static uint8_t guiEditPanel1_onVisibleChanged(void *widget, guiEvent_t *event);
static uint8_t guiEditPanel1_onKey(void *widget, guiEvent_t *event);



//--------- Edit panel 1  ----------//
#define EDIT_PANEL1_ELEMENTS_COUNT 20           // CHECKME - possibly too much
guiPanel_t     guiEditPanel1;
static void *guiEditPanel1Elements[EDIT_PANEL1_ELEMENTS_COUNT];
guiWidgetHandler_t editPanelHandlers[3];


//--------- Panel elements ---------//
// Title label
static guiTextLabel_t textLabel_title;        // Menu item list title


void guiEditPanel1_Initialize(guiGenericWidget_t *parent)
{
    // Initialize
    guiPanel_Initialize(&guiEditPanel1, parent);
    guiEditPanel1.widgets.count = EDIT_PANEL1_ELEMENTS_COUNT;
    guiEditPanel1.widgets.elements = guiEditPanel1Elements;
    guiEditPanel1.x = 5;
    guiEditPanel1.y = 5;
    guiEditPanel1.width = 96-10;
    guiEditPanel1.height = 68-13;
    guiEditPanel1.showFocus = 0;
    guiEditPanel1.focusFallsThrough = 0;
    guiEditPanel1.keyTranslator = 0;
    guiEditPanel1.handlers.count = 3;
    guiEditPanel1.handlers.elements = editPanelHandlers;
    guiEditPanel1.handlers.elements[0].eventType = GUI_EVENT_DRAW;
    guiEditPanel1.handlers.elements[0].handler = *guiEditPanel1_onDraw;
    guiEditPanel1.handlers.elements[1].eventType = GUI_ON_VISIBLE_CHANGED;
    guiEditPanel1.handlers.elements[1].handler = *guiEditPanel1_onVisibleChanged;
    guiEditPanel1.handlers.elements[2].eventType = GUI_EVENT_KEY;
    guiEditPanel1.handlers.elements[2].handler = *guiEditPanel1_onKey;

    // Initialize text label for menu list title
    guiTextLabel_Initialize(&textLabel_title, 0);
    textLabel_title.x = 5;
    textLabel_title.y = 5;
    textLabel_title.width = guiEditPanel1.width - 10;
    textLabel_title.height = 10;
    textLabel_title.textAlignment = ALIGN_TOP;
    textLabel_title.text = "Fast edit";
    textLabel_title.font = &font_h10_bold;




    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&textLabel_title, (guiGenericContainer_t *)&guiEditPanel1);
}




static uint8_t guiEditPanel1_onDraw(void *widget, guiEvent_t *event)
{
    if (guiEditPanel1.redrawForced)
    {
        // Draw frame
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_SetLineStyle(LINE_STYLE_SOLID);
        LCD_DrawHorLine(wx, wy, guiEditPanel1.width, 1);
        LCD_DrawHorLine(wx, wy + guiEditPanel1.height - 1, guiEditPanel1.width, 1);
        LCD_DrawVertLine(wx, wy, guiEditPanel1.height, 1);
        LCD_DrawVertLine(wx + guiEditPanel1.width - 1, wy, guiEditPanel1.height, 1);
    }
    return 0;
}


static uint8_t guiEditPanel1_onVisibleChanged(void *widget, guiEvent_t *event)
{
    return 0;
}


static uint8_t guiEditPanel1_onKey(void *widget, guiEvent_t *event)
{
    if ((event->spec == GUI_KEY_EVENT_UP_SHORT) || (event->spec == GUI_KEY_EVENT_HOLD))
    {
        if (event->lparam == GUI_KEY_ESC)
        {
            hideEditPanel1();
        }
        if (event->lparam == GUI_KEY_ENCODER)
        {
            hideEditPanel1();
        }
    }


    return GUI_EVENT_ACCEPTED;
}









