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


messageView_t messageView;


static uint8_t guiMessagePanel1_onDraw(void *widget, guiEvent_t *event);
static uint8_t guiMessagePanel1_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);
//static uint8_t guiMessagePanel1_onVisibleChanged(void *widget, guiEvent_t *event);
static uint8_t guiMessagePanel1_onTimer(void *widget, guiEvent_t *event);


//-------- Message panel 1 ---------//
guiPanel_t     guiMessagePanel1;

//--------- Panel elements ---------//
// none



void guiMessagePanel1_Initialize(guiGenericWidget_t *parent)
{
    // Initialize
    guiPanel_Initialize(&guiMessagePanel1, parent);
    // Panel has no widgets
    guiMessagePanel1.x = 3;
    guiMessagePanel1.y = 3;
    guiMessagePanel1.width = 96-6;
    guiMessagePanel1.height = 68-12;
    guiMessagePanel1.showFocus = 0;
    guiMessagePanel1.focusFallsThrough = 0;
    guiMessagePanel1.isVisible = 0;
    guiMessagePanel1.frame = 0;
    guiMessagePanel1.keyTranslator = &guiMessagePanel1_KeyTranslator;
    guiCore_AllocateHandlers((guiGenericWidget_t *)&guiMessagePanel1, 2);
    guiCore_AddHandler((guiGenericWidget_t *)&guiMessagePanel1, GUI_EVENT_DRAW, guiMessagePanel1_onDraw);
    guiCore_AddHandler((guiGenericWidget_t *)&guiMessagePanel1, GUI_EVENT_TIMER, guiMessagePanel1_onTimer);

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
        LCD_DrawHorLine(wx+5,wy+23,guiMessagePanel1.width-10,1);

        LCD_SetFont(&font_h10_bold);
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);

		// Draw icon
        // Draw caption text
        switch (messageView.type)
        {
            case MESSAGE_TYPE_INFO:
                LCD_PrintString("Info", wx + 25, wy + 10, IMAGE_MODE_NORMAL);
                LCD_DrawImage(info_8x18, wx + 8,wy + 2, 8, 18, 1);
                break;
            case MESSAGE_TYPE_WARNING:
                LCD_PrintString("Warning", wx + 20, wy + 10, IMAGE_MODE_NORMAL);
                LCD_DrawImage(exclamation_6x18, wx + 8,wy + 2, 6, 18, 1);
                break;
            default:
                LCD_PrintString("Error", wx + 30, wy + 10, IMAGE_MODE_NORMAL);
                LCD_DrawImage(cross_15x15, wx + 8,wy + 5, 15, 15, 1);
                break;
        }

        LCD_SetFont(&font_h10);
        // Draw message
        switch (messageView.code)
        {
            case MESSAGE_PROFILE_LOADED:
                LCD_PrintString("Profile loaded", wx + 5, wy + 30, IMAGE_MODE_NORMAL);
                break;
            case MESSAGE_PROFILE_SAVED:
                LCD_PrintString("Profile saved", wx + 5, wy + 30, IMAGE_MODE_NORMAL);
                break;
            case MESSAGE_PROFILE_CRC_ERROR:
                LCD_PrintString("CRC mismatch", wx + 5, wy + 30, IMAGE_MODE_NORMAL);
                break;
            case MESSAGE_PROFILE_HW_ERROR:
                LCD_PrintString("EEPROM fail", wx + 5, wy + 30, IMAGE_MODE_NORMAL);
                break;
            case MESSAGE_PROFILE_LOADED_DEFAULT:
                LCD_PrintString("Using default", wx + 5, wy + 30, IMAGE_MODE_NORMAL);
                LCD_PrintString("profile", wx + 15, wy + 42, IMAGE_MODE_NORMAL);
                break;
            case MESSAGE_PROFILE_RESTORED_RECENT:
                LCD_PrintString("Loaded recent", wx + 5, wy + 30, IMAGE_MODE_NORMAL);
                LCD_PrintString("profile", wx + 15, wy + 42, IMAGE_MODE_NORMAL);
                break;
        }


    }
    return 0;
}


//static uint8_t guiMessagePanel1_onVisibleChanged(void *widget, guiEvent_t *event)
//{
//    if (guiMessagePanel1.isVisible)
//    {
//        guiCore_TimerStart(GUI_MESSAGE_PANEL_TIMER, TMR_DO_RESET);
//    }
//    return 0;
//}


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


void guiMessagePanel1_Show(uint8_t msgType, uint8_t msgCode, uint8_t takeFocus, uint8_t timeout)
{
    messageView.type = msgType;
    messageView.code = msgCode;
    // Reinit and start timer
    guiCore_TimerInit(GUI_MESSAGE_PANEL_TIMER, timeout, TMR_RUN_ONCE, (guiGenericWidget_t *)&guiMessagePanel1, 0);
    guiCore_TimerStart(GUI_MESSAGE_PANEL_TIMER, TMR_DO_RESET);
    if (guiMessagePanel1.isVisible == 0)
    {
        guiCore_SetVisible((guiGenericWidget_t *)&guiMessagePanel1, 1);
    }
    else
    {
        guiMessagePanel1.redrawRequired = 1;
        guiMessagePanel1.redrawForced = 1;
    }
    if (takeFocus)
    {
        messageView.lastFocused = focusedWidget;
        guiCore_RequestFocusChange((guiGenericWidget_t *)&guiMessagePanel1);
    }
}





