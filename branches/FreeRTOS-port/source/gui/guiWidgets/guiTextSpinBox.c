/**********************************************************
    Module textSpinBox




**********************************************************/

#include <stdint.h>         // using integer types
#include <string.h>         // using memset
#include "guiEvents.h"
#include "guiCore.h"
#include "guiWidgets.h"
#include "guiGraphWidgets.h"
#include "guiTextSpinBox.h"

#include "utils.h"



uint8_t guiTextSpinBox_SetActive(guiTextSpinBox_t *textSpinBox, uint8_t newActiveState, uint8_t restoreValue)
{
    /*guiEvent_t event;
    if (textSpinBox == 0) return 0;

    if (newActiveState)
    {
        // Activate
        if (textSpinBox->isActive) return 0;
        textSpinBox->isActive = 1;
        textSpinBox->savedValue = textSpinBox->value;
    }
    else
    {
        // Deactivate
        if (textSpinBox->isActive == 0) return 0;
        textSpinBox->isActive = 0;
        if ((textSpinBox->restoreValueOnEscape) && (restoreValue))
        {
            guiTextSpinBox_SetValue(textSpinBox, textSpinBox->savedValue, 1);
            textSpinBox->newValueAccepted = 0;
        }
        else
        {
            textSpinBox->newValueAccepted = 1;
        }
    }
    // Active state changed - call handler
    textSpinBox->redrawDigitSelection = 1;
    textSpinBox->redrawRequired = 1;
    if (textSpinBox->handlers.count != 0)
    {
        event.type = SPINBOX_ACTIVE_CHANGED;
        guiCore_CallEventHandler((guiGenericWidget_t *)textSpinBox, &event);
    }*/
    return 1;
}



void guiTextSpinBox_SetActiveDigit(guiTextSpinBox_t *textSpinBox, int8_t num)
{
    /*uint8_t newActiveDigit;
    guiEvent_t event;
    if (textSpinBox != 0)
    {
        newActiveDigit = (num >= textSpinBox->digitsToDisplay) ? textSpinBox->digitsToDisplay - 1 :
                                                             ((num < 0) ? 0 : num);
        if (newActiveDigit != textSpinBox->activeDigit)
        {
            textSpinBox->activeDigit = newActiveDigit;
            textSpinBox->redrawDigitSelection = 1;
            textSpinBox->redrawRequired = 1;
            // Call handler
            event.type = SPINBOX_ACTIVE_DIGIT_CHANGED;
            guiCore_CallEventHandler((guiGenericWidget_t *)textSpinBox, &event);
        }
    }*/
}


void guiTextSpinBox_SetValue(guiTextSpinBox_t *textSpinBox, int32_t value, uint8_t callHandler)
{
   /* int32_t newValue;
    guiEvent_t event;
    if (textSpinBox != 0)
    {
        newValue = (value < textSpinBox->minValue) ? textSpinBox->minValue :
                   ((value > textSpinBox->maxValue) ? textSpinBox->maxValue : value);

        if (newValue != textSpinBox->value)
        {
            textSpinBox->value = newValue;
            textSpinBox->redrawValue = 1;
            textSpinBox->redrawRequired = 1;
            textSpinBox->digitsToDisplay = i32toa_align_right(textSpinBox->value, textSpinBox->text,
                                       SPINBOX_STRING_LENGTH | NO_TERMINATING_ZERO, textSpinBox->minDigitsToDisplay);
            // Call handler
            if (callHandler)
            {
                event.type = SPINBOX_VALUE_CHANGED;
                guiCore_CallEventHandler((guiGenericWidget_t *)textSpinBox, &event);
            }
            // Check active digit position
            guiTextSpinBox_SetActiveDigit(textSpinBox, textSpinBox->activeDigit);
        }
    }*/
}

void guiTextSpinBox_IncrementValue(guiTextSpinBox_t *textSpinBox, int32_t delta)
{
    /*int32_t mul_c = 1;
    uint8_t i;
    if ((textSpinBox != 0) && (delta != 0))
    {
        for (i=0;i<textSpinBox->activeDigit;i++)
        {
            mul_c *= 10;
        }
        delta *= mul_c;
        guiTextSpinBox_SetValue(textSpinBox, textSpinBox->value + delta, 1);
    }*/
}




//-------------------------------------------------------//
// textSpinBox key handler
//
// Returns GUI_EVENT_ACCEPTED if key is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiTextSpinBox_ProcessKey(guiTextSpinBox_t *textSpinBox, uint8_t key)
{
  /*  if (textSpinBox->isActive)
    {
        if (key == SPINBOX_KEY_SELECT)
        {
            guiTextSpinBox_SetActive(textSpinBox, 0, 0);
        }
        else if (key == SPINBOX_KEY_EXIT)
        {
            guiTextSpinBox_SetActive(textSpinBox, 0, 1);
        }
        else if (key == SPINBOX_KEY_LEFT)
        {
            // move active digit left
            guiTextSpinBox_SetActiveDigit(textSpinBox, textSpinBox->activeDigit + 1);
        }
        else if (key == SPINBOX_KEY_RIGHT)
        {
            // move active digit right
            guiTextSpinBox_SetActiveDigit(textSpinBox, textSpinBox->activeDigit - 1);
        }
        else
        {
            return GUI_EVENT_DECLINE;
        }
    }
    else
    {
        if (key == SPINBOX_KEY_SELECT)
        {
            guiTextSpinBox_SetActive(textSpinBox, 1, 0);
        }
        else
        {
            return GUI_EVENT_DECLINE;
        }
    }
*/
    return GUI_EVENT_ACCEPTED;
}


//-------------------------------------------------------//
// Default key event translator
//
//-------------------------------------------------------//
uint8_t guiTextSpinBox_DefaultKeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
 /*   guiSpinboxTranslatedKey_t *tkey = (guiSpinboxTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    tkey->increment = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_OK)
            tkey->key = SPINBOX_KEY_SELECT;
        else if (event->lparam == GUI_KEY_ESC)
            tkey->key = SPINBOX_KEY_EXIT;
        else if (event->lparam == GUI_KEY_LEFT)
            tkey->key = SPINBOX_KEY_LEFT;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = SPINBOX_KEY_RIGHT;
        else if (event->lparam == GUI_KEY_UP)
            tkey->increment = 1;
        else if (event->lparam == GUI_KEY_DOWN)
            tkey->increment = -1;
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        tkey->increment = (int16_t)event->lparam;
    }*/
    return 0;
}




//-------------------------------------------------------//
// textSpinBox event handler
//
// Returns GUI_EVENT_ACCEPTED if event is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiTextSpinBox_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    guiTextSpinBox_t *textSpinBox = (guiTextSpinBox_t *)widget;
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    guiTextSpinBoxTranslatedKey_t tkey;
/*
    switch (event.type)
    {
        case GUI_EVENT_DRAW:
            guiGraph_DrawSpinBox(textSpinBox);
            // Call handler
            guiCore_CallEventHandler(widget, &event);
            // Reset flags - redrawForced will be reset by core
            textSpinBox->redrawFocus = 0;
            textSpinBox->redrawDigitSelection = 0;
            textSpinBox->redrawValue = 0;
            textSpinBox->redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:
            if (SPINBOX_ACCEPTS_FOCUS_EVENT(textSpinBox))
                guiCore_SetFocused((guiGenericWidget_t *)textSpinBox,1);
            else
                processResult = GUI_EVENT_DECLINE;      // Cannot accept focus
            break;
        case GUI_EVENT_UNFOCUS:
            guiCore_SetFocused((guiGenericWidget_t *)textSpinBox,0);
            guiTextSpinBox_SetActive(textSpinBox, 0, 0);        // Do not restore
            //textSpinBox->keepTouch = 0;
            break;
        case SPINBOX_EVENT_ACTIVATE:
            if (textSpinBox->isFocused)
            {
                guiTextSpinBox_SetActive(textSpinBox, 1, 0);
            }
            // Accept event anyway
            break;
        case GUI_EVENT_SHOW:
            guiCore_SetVisible((guiGenericWidget_t *)textSpinBox, 1);
            break;
        case GUI_EVENT_HIDE:
            guiCore_SetVisible((guiGenericWidget_t *)textSpinBox, 0);
            break;
        case GUI_EVENT_KEY:
            processResult = GUI_EVENT_DECLINE;
            if (SPINBOX_ACCEPTS_KEY_EVENT(textSpinBox))
            {
                if (textSpinBox->keyTranslator)
                {
                    processResult = textSpinBox->keyTranslator(widget, &event, &tkey);
                    if (tkey.key != 0)
                    {
                        processResult |= guiTextSpinBox_ProcessKey(textSpinBox, tkey.key);
                    }
                    else if ((tkey.increment != 0) && (textSpinBox->isActive))
                    {
                        guiTextSpinBox_IncrementValue(textSpinBox, tkey.increment);
                        processResult |= GUI_EVENT_ACCEPTED;
                    }
                }
                // Call KEY event handler
                if (processResult == GUI_EVENT_DECLINE)
                    processResult = guiCore_CallEventHandler(widget, &event);
            }
            break;

        default:
            // Widget cannot process incoming event. Try to find a handler.
            processResult = guiCore_CallEventHandler(widget, &event);
    }*/
    return processResult;
}





//-------------------------------------------------------//
// Default initialization
//
//-------------------------------------------------------//
void guiTextSpinBox_Initialize(guiTextSpinBox_t *textSpinBox, guiGenericWidget_t *parent)
{
  /*  memset(textSpinBox, 0, sizeof(*textSpinBox));
    textSpinBox->type = WT_SPINBOX;
    textSpinBox->parent = parent;
    textSpinBox->acceptFocusByTab = 1;
    textSpinBox->isVisible = 1;
    textSpinBox->showFocus = 1;
    textSpinBox->processEvent = guiTextSpinBox_ProcessEvent;
    textSpinBox->keyTranslator = guiTextSpinBox_DefaultKeyTranslator;
    textSpinBox->minDigitsToDisplay = 1;
    textSpinBox->maxValue = INT32_MAX;
    textSpinBox->minValue = INT32_MIN; */
}


