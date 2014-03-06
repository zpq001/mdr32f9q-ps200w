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

#include "guiEditPanel1.h"
#include "guiMasterPanel.h"


#include "guiTop.h"
#include "converter.h"



static uint8_t guiEditPanel1_onDraw(void *widget, guiEvent_t *event);
static uint8_t guiEditPanel1_onFocusChanged(void *widget, guiEvent_t *event);
static uint8_t guiEditPanel1_onKey(void *widget, guiEvent_t *event);

static uint8_t spinBox_KeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey);


//--------- Edit panel 1  ----------//
#define EDIT_PANEL1_ELEMENTS_COUNT 10
guiPanel_t     guiEditPanel1;
static void *guiEditPanel1Elements[EDIT_PANEL1_ELEMENTS_COUNT];
guiWidgetHandler_t editPanelHandlers[4];


//--------- Panel elements ---------//
static guiSpinBox_t spinBox_Edit;


editView_t editView;

void guiEditPanel1_Initialize(guiGenericWidget_t *parent)
{
    // Initialize
    guiPanel_Initialize(&guiEditPanel1, parent);
    guiEditPanel1.widgets.count = EDIT_PANEL1_ELEMENTS_COUNT;
    guiEditPanel1.widgets.elements = guiEditPanel1Elements;
    guiEditPanel1.x = 5;
    guiEditPanel1.y = 5;
    guiEditPanel1.width = 96-10;
    guiEditPanel1.height = 68-20;//68-13;
    guiEditPanel1.showFocus = 0;
    guiEditPanel1.focusFallsThrough = 0;
    guiEditPanel1.keyTranslator = 0;
    guiEditPanel1.handlers.count = 3;
    guiEditPanel1.handlers.elements = editPanelHandlers;
    guiEditPanel1.handlers.elements[0].eventType = GUI_EVENT_DRAW;
    guiEditPanel1.handlers.elements[0].handler = *guiEditPanel1_onDraw;
    guiEditPanel1.handlers.elements[1].eventType = GUI_EVENT_KEY;
    guiEditPanel1.handlers.elements[1].handler = *guiEditPanel1_onKey;
    guiEditPanel1.handlers.elements[2].eventType = GUI_ON_FOCUS_CHANGED;
    guiEditPanel1.handlers.elements[2].handler = *guiEditPanel1_onFocusChanged;

    guiSpinBox_Initialize(&spinBox_Edit, 0);
    spinBox_Edit.x = 10;
    spinBox_Edit.y = 20;
    spinBox_Edit.width = 40;
    spinBox_Edit.height = 18;
    spinBox_Edit.textRightOffset = -2;
    spinBox_Edit.textTopOffset = 2;
    spinBox_Edit.tabIndex = 2;
    spinBox_Edit.font = &font_h11;
    spinBox_Edit.dotPosition = 2;
    spinBox_Edit.activeDigit = 2;
    spinBox_Edit.minDigitsToDisplay = 3;
    spinBox_Edit.restoreValueOnEscape = 1;
    spinBox_Edit.showFocus = 0;
    spinBox_Edit.isVisible = 1;
    spinBox_Edit.value = 1;
    guiSpinBox_SetValue(&spinBox_Edit, 0, 0);
    spinBox_Edit.keyTranslator = spinBox_KeyTranslator;



    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&spinBox_Edit, (guiGenericContainer_t *)&guiEditPanel1);
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


        if (editView.mode == EDIT_MODE_VOLTAGE)
        {
            LCD_SetFont(&font_h11);
            LCD_PrintString("V", wx + 55, wy + 22, IMAGE_MODE_NORMAL);
            LCD_SetFont(&font_h10_bold);
            LCD_PrintString("Set voltage", wx + 10, wy + 3, IMAGE_MODE_NORMAL);
        }
        else
        {
            LCD_SetFont(&font_h11);
            LCD_PrintString("A", wx + 55, wy + 22, IMAGE_MODE_NORMAL);
            LCD_SetFont(&font_h10_bold);
            LCD_PrintString("Set current", wx + 10, wy + 3, IMAGE_MODE_NORMAL);
        }
    }
    return 0;
}


static uint8_t guiEditPanel1_onFocusChanged(void *widget, guiEvent_t *event)
{
    int32_t value;
    if (guiEditPanel1.isFocused)
    {
        if (editView.mode == EDIT_MODE_VOLTAGE)
        {
            spinBox_Edit.maxValue = Converter_GetVoltageAbsMax(editView.channel)/10;
            spinBox_Edit.minValue = Converter_GetVoltageAbsMin(editView.channel)/10;
            value = Converter_GetVoltageSetting(editView.channel);
            guiSpinBox_SetValue(&spinBox_Edit, value/10, 0);
        }
        else
        {
            spinBox_Edit.maxValue = Converter_GetCurrentAbsMax(editView.channel, editView.current_range)/10;
            spinBox_Edit.minValue = Converter_GetCurrentAbsMin(editView.channel, editView.current_range)/10;
            value = Converter_GetCurrentSetting(editView.channel, editView.current_range);
            guiSpinBox_SetValue(&spinBox_Edit, value/10, 0);
        }
        guiSpinBox_SetActiveDigit(&spinBox_Edit,editView.active_digit);

        guiCore_RequestFocusChange((guiGenericWidget_t *)&spinBox_Edit);
        event->type = SPINBOX_EVENT_ACTIVATE;
        guiCore_AddMessageToQueue((guiGenericWidget_t *)&spinBox_Edit,event);   // activate
    }
    return 0;
}


static uint8_t guiEditPanel1_onKey(void *widget, guiEvent_t *event)
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
        if (editView.mode == EDIT_MODE_VOLTAGE)
            applyGuiVoltageSetting(editView.channel, spinBox_Edit.value * 10);
        else
            applyGuiCurrentSetting(editView.channel, editView.current_range, spinBox_Edit.value * 10);
    }
    if (exit_code)
    {
        hideEditPanel1();
    }
    return GUI_EVENT_ACCEPTED;
}



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






