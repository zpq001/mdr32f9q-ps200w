/**********************************************************
    Module TextLabel




**********************************************************/

#include <stdint.h>         // using integer types
#include <string.h>         // using memset
#include "guiEvents.h"
#include "guiCore.h"
#include "guiWidgets.h"
#include "guiTextLabel.h"
#include "guiGraphWidgets.h"




uint8_t guiTextLabel_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    guiTextLabel_t *textLabel = (guiTextLabel_t *)widget;
    uint8_t processResult = GUI_EVENT_ACCEPTED;

    switch (event.type)
    {
        case GUI_EVENT_DRAW:
            guiGraph_DrawTextLabel(textLabel);
            // Call handler
            guiCore_CallEventHandler(widget, &event);
            // Reset flags
            textLabel->redrawFocus = 0;
            textLabel->redrawText = 0;
            textLabel->redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:
            if (TEXTLABEL_ACCEPTS_FOCUS_EVENT(textLabel))
                guiCore_SetFocused((guiGenericWidget_t *)textLabel, 1);
            else
                processResult = GUI_EVENT_DECLINE;      // Cannot accept focus
            break;
        case GUI_EVENT_UNFOCUS:
            guiCore_SetFocused((guiGenericWidget_t *)textLabel, 0);
            break;
        case GUI_EVENT_SHOW:
            guiCore_SetVisible((guiGenericWidget_t *)textLabel, 1);
            break;
        case GUI_EVENT_HIDE:
            guiCore_SetVisible((guiGenericWidget_t *)textLabel, 0);
            break;
        default:
            // Widget cannot process incoming event. Try to find a handler.
            processResult = guiCore_CallEventHandler(widget, &event);
    }
    return processResult;
}


//-------------------------------------------------------//
// Default initialization
//
//-------------------------------------------------------//
void guiTextLabel_Initialize(guiTextLabel_t *textLabel, guiGenericWidget_t *parent)
{
    memset(textLabel, 0, sizeof(*textLabel));
    textLabel->type = WT_TEXTLABEL;
    textLabel->parent = parent;
    textLabel->isVisible = 1;
    textLabel->showFocus = 0;
    textLabel->processEvent = guiTextLabel_ProcessEvent;
    textLabel->textAlignment = ALIGN_CENTER;
}


void guiTextLabel_SetText(guiTextLabel_t *textLabel, char *text)
{
    textLabel->text = text;
    textLabel->redrawText = 1;
    textLabel->redrawRequired = 1;
}

