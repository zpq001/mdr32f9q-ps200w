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

#include "guiEditPanel2.h"
#include "guiSetupPanel.h"


#include "guiTop.h"
#include "converter.h"



static uint8_t guiEditPanel2_onDraw(void *widget, guiEvent_t *event);
static uint8_t guiEditPanel2_onFocusChanged(void *widget, guiEvent_t *event);
static uint8_t guiEditPanel2_onKey(void *widget, guiEvent_t *event);

//static uint8_t spinBox_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);


//--------- Edit panel 1  ----------//
guiPanel_t     guiEditPanel2;



//--------- Panel elements ---------//


void guiEditPanel2_Initialize(guiGenericWidget_t *parent)
{
    // Initialize
    guiPanel_Initialize(&guiEditPanel2, parent);
    guiCore_AllocateWidgetCollection((guiGenericContainer_t *)&guiEditPanel2, 5);
    guiCore_AllocateHandlers((guiGenericWidget_t *)&guiEditPanel2, 3);
    guiEditPanel2.x = 3;
    guiEditPanel2.y = 3;
    guiEditPanel2.width = 96-6;
    guiEditPanel2.height = 68-12;
    guiEditPanel2.showFocus = 0;
    guiEditPanel2.focusFallsThrough = 0;
    guiEditPanel2.keyTranslator = 0;
    guiCore_AddHandler((guiGenericWidget_t *)&guiEditPanel2, GUI_EVENT_DRAW, guiEditPanel2_onDraw);
    guiCore_AddHandler((guiGenericWidget_t *)&guiEditPanel2, GUI_EVENT_KEY, guiEditPanel2_onKey);
    guiCore_AddHandler((guiGenericWidget_t *)&guiEditPanel2, GUI_ON_FOCUS_CHANGED, guiEditPanel2_onFocusChanged);


}




static uint8_t guiEditPanel2_onDraw(void *widget, guiEvent_t *event)
{
    if (guiEditPanel2.redrawForced)
    {
        // Draw frame
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_SetLineStyle(LINE_STYLE_SOLID);
		LCD_DrawRect(wx,wy,guiEditPanel2.width,guiEditPanel2.height,1);

        LCD_SetFont(&font_h10);
        LCD_PrintString("Edit name", wx + 5, wy + 2, IMAGE_MODE_NORMAL);
    }
    return 0;
}


static uint8_t guiEditPanel2_onFocusChanged(void *widget, guiEvent_t *event)
{
    //int32_t value;
    if (guiEditPanel2.isFocused)
    {
       
    }
    return 0;
}


static uint8_t guiEditPanel2_onKey(void *widget, guiEvent_t *event)
{
    uint8_t exit_code = 0;

    if (event->spec == GUI_KEY_EVENT_UP_SHORT)
    {
        if (event->lparam == GUI_KEY_ESC)
            exit_code = 1;
        else if ((event->lparam == GUI_KEY_OK) || (event->lparam == GUI_KEY_ENCODER))
            exit_code = 2;
    }
    else if (event->spec == GUI_KEY_EVENT_HOLD)
    {
        if ((event->lparam == GUI_KEY_ESC)  || (event->lparam == GUI_KEY_ENCODER))
            exit_code = 1;
        if (event->lparam == GUI_KEY_OK)
            exit_code = 2;
    }

    if (exit_code == 2)
    {
        //if (editView.mode == EDIT_MODE_VOLTAGE)
        //    applyGuiVoltageSetting(editView.channel, spinBox_Edit.value * 10);
        //else
        //    applyGuiCurrentSetting(editView.channel, editView.current_range, spinBox_Edit.value * 10);
    }
    if (exit_code)
    {
        //hideEditPanel1();
    }
    return GUI_EVENT_ACCEPTED;
}


/*
static uint8_t spinBox_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiSpinboxTranslatedKey_t *tkey = (guiSpinboxTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    tkey->increment = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_LEFT)
            tkey->key = SPINBOX_KEY_LEFT;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = SPINBOX_KEY_RIGHT;
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        if (((guiSpinBox_t *)widget)->isActive == 0)
        {
            event->type = SPINBOX_EVENT_ACTIVATE;
            guiCore_AddMessageToQueue(widget,event);   // activate
            return GUI_EVENT_ACCEPTED;
        }
        tkey->increment = (int16_t)event->lparam;
    }
    return 0;
}
*/





