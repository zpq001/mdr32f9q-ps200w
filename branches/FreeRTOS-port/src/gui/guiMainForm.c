/**********************************************************
    Module guiMainForm


    - instant setting of Voltage/Current
    - presetting Voltage/Current            <- separate panel
    - setting Voltage/Current limits        <- separate panel

    Keys and encoder operation WAY #1:
    Short press:
        Press encoder shortly to switch between Voltage/Current. Selecting by encoder push activates control.
        When Voltage/Current control is activated, press "<-", "->" buttons shortly to select digit you want to edit.
        When Voltage/Current control is NOT activated, use "<-", "->" buttons to switch between Voltage/Current/Current_Limit.
    Long press:
        Press encoder long to set software limits for regulation parameter
        Press "<-" long to preset voltage, "->" long to preset current


    Keys and encoder operation WAY #2:
    Short press:
        "<-" activates voltage setting, "->" activates current setting.
        Encoder push cycles edited digit.
    Long press:
        "<-" activates voltage preset menu, "->" activates current preset menu.

**********************************************************/

#include <stdio.h>      // due to printf
#include <stdint.h>

#include "guiFonts.h"
#include "guiGraphHAL.h"
#include "guiGraphPrimitives.h"
#include "guiGraphWidgets.h"

#include "guiCore.h"
#include "guiEvents.h"
#include "guiWidgets.h"
#include "guiPanel.h"
#include "guiTextLabel.h"

// Other forms and panels - in order to switch between them
#include "guiMainForm.h"
#include "guiMasterPanel.h"
#include "guiSetupPanel.h"

extern void guiLogEvent(char *string);


static uint8_t guiMainForm_ProcessEvents(guiGenericWidget_t *widget, guiEvent_t event);


//--------- Form elements ---------//
//static guiTextLabel_t textLabel_voltage;
//static char textLabel_voltage_data[10];

//static guiTextLabel_t textLabel_current;
//static char textLabel_current_data[10];


//----------- GUI Form  -----------//
#define MAIN_FORM_ELEMENTS_COUNT 2
guiPanel_t     guiMainForm;
static void *guiMainFormElements[MAIN_FORM_ELEMENTS_COUNT];


void guiMainForm_Initialize(void)
{
    // Initialize form
    guiPanel_Initialize(&guiMainForm, 0);
    guiMainForm.processEvent = guiMainForm_ProcessEvents;
    guiMainForm.widgets.count = MAIN_FORM_ELEMENTS_COUNT;
    guiMainForm.widgets.elements = guiMainFormElements;
    guiMainForm.widgets.elements[0] = &guiMasterPanel;
    guiMainForm.widgets.elements[1] = &guiSetupPanel;

    guiMainForm.x = 0;
    guiMainForm.y = 0;
    guiMainForm.width = 96 * 2;
    guiMainForm.height = 68;


    // Other panels are all initialized to be invisible
    guiMasterPanel_Initialize((guiGenericWidget_t *)&guiMainForm);
    guiSetupPanel_Initialize((guiGenericWidget_t *)&guiMainForm);
}


static uint8_t guiMainForm_ProcessEvents(struct guiGenericWidget_t *widget, guiEvent_t event)
{
    // Process GUI messages - focus, draw, etc
    switch(event.type)
    {
         case GUI_EVENT_INIT:
            guiMainForm.isVisible = 1;
            guiMainForm.redrawRequired = 1;
            guiMainForm.redrawForced = 1;
            guiCore_SetFocused((guiGenericWidget_t *)&guiMainForm, 1);
            // The following actions should be made by greeting timer expire
            guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMasterPanel, &guiEvent_SHOW);
            guiCore_RequestFocusChange((guiGenericWidget_t *)&guiMasterPanel);
            break;
          case GUI_EVENT_DRAW:
            // Clear screen and put greeting image
            // TODO
            //guiGraph_DrawPanel(&guiMainForm);
            // Draw static elemens
            //if (guiMainForm.redrawForced)
            //    LCD_DrawHorLine(0,110,255,1);
            // Reset flags
            guiMainForm.redrawFocus = 0;
            guiMainForm.redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:

            //guiCore_SetFocused((guiGenericWidget_t *)&guiMainForm, 1);
            //guiCore_RequestFocusChange(guiMainForm.widgets.elements[guiMainForm.widgets.focusedIndex]);
            //guiCore_RequestFocusNextWidget((guiGenericContainer_t *)&guiMainForm,1);
            break;
        case GUI_EVENT_UNFOCUS:
            //guiCore_SetFocused((guiGenericWidget_t *)&guiMainForm, 0);
            break;

        case GUI_EVENT_KEY:
            if ((event.spec == GUI_KEY_EVENT_DOWN) && (event.lparam == GUI_KEY_ESC))
            {
                if (guiMasterPanel.isVisible)
                {
                    guiCore_SetVisible((guiGenericWidget_t *)&guiMasterPanel, 0);
                    guiCore_SetVisible((guiGenericWidget_t *)&guiSetupPanel, 1);
                    guiCore_RequestFocusChange((guiGenericWidget_t *)&guiSetupPanel);
                }
                else
                {
                    guiCore_SetVisible((guiGenericWidget_t *)&guiSetupPanel, 0);
                    guiCore_SetVisible((guiGenericWidget_t *)&guiMasterPanel, 1);
                    guiCore_RequestFocusChange((guiGenericWidget_t *)&guiMasterPanel);
                }
                break;
            }

        /*
            if (event.spec == GUI_KEY_EVENT_DOWN)
            {
                if (event.lparam == GUI_KEY_LEFT)
                    guiCore_RequestFocusNextWidget((guiGenericContainer_t *)&guiMainForm,-1);
                else if (event.lparam == GUI_KEY_RIGHT)
                    guiCore_RequestFocusNextWidget((guiGenericContainer_t *)&guiMainForm,1);
                else if (event.lparam == GUI_KEY_ESC)
                {
                    //guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiSubForm1, &guiEvent_HIDE);
                    //guiCore_SetVisibleByTag(&guiMainForm.widgets, 10,20,ITEMS_IN_RANGE_ARE_VISIBLE);
                    //guiCore_RequestFocusChange((guiGenericWidget_t *)&guiMainForm);
                }
            } */
            break;
    }
    return GUI_EVENT_ACCEPTED;
}



/*
static uint8_t textLabel_onFocusChanged(void *sender, guiEvent_t *event)
{
    guiGenericWidget_t *textLabel = (guiGenericWidget_t *)sender;
    if (textLabel->isFocused)
    {
        if (textLabel == (guiGenericWidget_t *)&textLabel1)
            textLabel_current.text = "Focused label 1";
        else if (textLabel == (guiGenericWidget_t *)&textLabel2)
            textLabel_current.text = "Focused label 2";
        else if (textLabel == (guiGenericWidget_t *)&textLabel3)
            textLabel_current.text = "Focused label 3";
        else if (textLabel == (guiGenericWidget_t *)&textLabel4)
            textLabel_current.text = "Focused label 4";
        else
            textLabel_current.text = "Focused ???";

        textLabel_current.redrawText = 1;
        textLabel_current.redrawRequired = 1;
    }

    return GUI_EVENT_ACCEPTED;
}



static uint8_t textLabel_onButtonEvent(void *sender, guiEvent_t *event)
{
    guiGenericWidget_t *textLabel = (guiGenericWidget_t *)sender;

    if (event->lparam == GUI_KEY_OK)
    {
        if (textLabel == (guiGenericWidget_t *)&textLabel1)
        {

        }
        else if (textLabel == (guiGenericWidget_t *)&textLabel2)
        {
            guiCore_SetVisibleByTag(&guiMainForm.widgets, 10,20,ITEMS_IN_RANGE_ARE_INVISIBLE);
            guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiSubForm1, &guiEvent_SHOW);
            guiCore_RequestFocusChange((guiGenericWidget_t *)&guiSubForm1);
        }
        else if (textLabel == (guiGenericWidget_t *)&textLabel3)
        {

        }
        else //if (textLabel == (guiGenericWidget_t *)&textLabel4)
        {

        }
        return GUI_EVENT_ACCEPTED;
    }
    return GUI_EVENT_DECLINE;
}
*/

