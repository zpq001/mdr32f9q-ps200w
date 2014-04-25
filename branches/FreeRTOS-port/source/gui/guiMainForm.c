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
#include "guiImages.h"

#include "guiCore.h"
#include "guiEvents.h"
#include "guiWidgets.h"
#include "guiPanel.h"
#include "guiTextLabel.h"

// Other forms and panels
#include "guiMainForm.h"
#include "guiMasterPanel.h"
#include "guiSetupPanel.h"
#include "guiEditPanel1.h"
#include "guiEditPanel2.h"
#include "guiMessagePanel1.h"


extern void guiLogEvent(char *string);


static uint8_t guiMainForm_ProcessEvents(guiGenericWidget_t *widget, guiEvent_t event);


//----------- GUI Form  -----------//
guiPanel_t     guiMainForm;
static uint8_t greetingFlags = 0;


void guiMainForm_Initialize(void)
{
    // Initialize form
    guiPanel_Initialize(&guiMainForm, 0);   // no parent => root
    guiCore_AllocateWidgetCollection((guiGenericContainer_t *)&guiMainForm, 3);
    guiMainForm.x = 0;
    guiMainForm.y = 0;
    guiMainForm.width = 96 * 2;
    guiMainForm.height = 68;
    guiMainForm.processEvent = guiMainForm_ProcessEvents;

    // Other panels are all initialized to be invisible
    guiMasterPanel_Initialize((guiGenericWidget_t *)&guiMainForm);
    guiSetupPanel_Initialize((guiGenericWidget_t *)&guiMainForm);
    guiEditPanel1_Initialize((guiGenericWidget_t *)&guiMasterPanel);    // parent is not important here
    guiMessagePanel1_Initialize((guiGenericWidget_t *)&guiSetupPanel);    //
    guiEditPanel2_Initialize((guiGenericWidget_t *)&guiSetupPanel);

    // Add other widgets
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&guiMasterPanel, (guiGenericContainer_t *)&guiMainForm);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&guiSetupPanel, (guiGenericContainer_t *)&guiMainForm);
    guiCore_AddWidgetToCollection((guiGenericWidget_t *)&guiMessagePanel1, (guiGenericContainer_t *)&guiMainForm);
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
            //guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiSetupPanel, &event);
            break;
         case GUI_EVENT_START:
            // The following actions should be made by greeting timer expire
            guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMasterPanel, &guiEvent_SHOW);
            guiCore_RequestFocusChange((guiGenericWidget_t *)&guiMasterPanel);
            break;
          case GUI_EVENT_EEPROM_MESSAGE:
            // event.spec = 1 => EEPROM is OK
            greetingFlags |= (event.spec) ? 0x01 : 0x02;
            guiMainForm.redrawRequired = 1;
            break;
          case GUI_EVENT_DRAW:
            // Greeting screen
            LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
            if (guiMainForm.redrawForced)
            {
                // Erase rectangle
                LCD_FillRect(wx,wy,guiMasterPanel.width,guiMasterPanel.height,FILL_WITH_WHITE);
                LCD_SetFont(&font_h11);
                LCD_PrintString("PS200", 28, 2, 1);
                LCD_PrintString("Power supply", 8, 15, 1);
                LCD_PrintString("MDR32F9Qx", 96+5, 2, 1);
                LCD_SetFont(&font_h10);
                LCD_PrintString("powered by", 96+10, 15, 1);
                LCD_DrawImage(freeRTOS_logo_96x36, 96,27,96,36,1);
            }
            LCD_SetFont(&font_6x8_mono);
            if ((greetingFlags & 0x03) == 0x01)
                LCD_PrintString("EEPROM OK", 5, 55, 1);
            else if ((greetingFlags & 0x03) == 0x02)
                LCD_PrintString("EEPROM FAIL", 5, 55, 1);
            // Reset flags
            greetingFlags = 0;
            guiMainForm.redrawFocus = 0;
            guiMainForm.redrawRequired = 0;
            break;
          case GUI_EVENT_KEY:
            if ((event.spec == GUI_KEY_EVENT_HOLD) && (event.lparam == GUI_KEY_ESC))
            {
                if (guiMasterPanel.isVisible)
                {
                    //guiCore_SetVisible((guiGenericWidget_t *)&guiMasterPanel, 0);
                    //guiCore_SetVisible((guiGenericWidget_t *)&guiSetupPanel, 1);
                    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMasterPanel, &guiEvent_HIDE);
                    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiSetupPanel, &guiEvent_SHOW);
                    guiCore_RequestFocusChange((guiGenericWidget_t *)&guiSetupPanel);
                }
                else
                {
                    //guiCore_SetVisible((guiGenericWidget_t *)&guiSetupPanel, 0);
                    //guiCore_SetVisible((guiGenericWidget_t *)&guiMasterPanel, 1);
                    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiSetupPanel, &guiEvent_HIDE);
                    guiCore_AddMessageToQueue((guiGenericWidget_t *)&guiMasterPanel, &guiEvent_SHOW);
                    guiCore_RequestFocusChange((guiGenericWidget_t *)&guiMasterPanel);
                }
                break;
            }
            break;
    }
    return GUI_EVENT_ACCEPTED;
}



