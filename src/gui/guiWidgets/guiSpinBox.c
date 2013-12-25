/**********************************************************
    Module spinBox




**********************************************************/

#include <stdint.h>         // using integer types
#include <string.h>         // using memset
#include "guiEvents.h"
#include "guiCore.h"
#include "guiWidgets.h"
#include "guiGraphWidgets.h"
#include "guiSpinBox.h"

#include "utils.h"



uint8_t guiSpinBox_SetActive(guiSpinBox_t *spinBox, uint8_t newActiveState, uint8_t restoreValue)
{
    guiEvent_t event;
    if (spinBox == 0) return 0;

    if (newActiveState)
    {
        // Activate
        if (spinBox->isActive) return 0;
        spinBox->isActive = 1;
        spinBox->savedValue = spinBox->value;
    }
    else
    {
        // Deactivate
        if (spinBox->isActive == 0) return 0;
        spinBox->isActive = 0;
        if ((spinBox->restoreValueOnEscape) && (restoreValue))
        {
            guiSpinBox_SetValue(spinBox, spinBox->savedValue);
            spinBox->newValueAccepted = 0;
        }
        else
        {
            spinBox->newValueAccepted = 1;
        }
    }
    // Active state changed - call handler
    spinBox->redrawDigitSelection = 1;
    spinBox->redrawRequired = 1;
    if (spinBox->handlers.count != 0)
    {
        event.type = SPINBOX_ACTIVE_CHANGED;
        guiCore_CallEventHandler((guiGenericWidget_t *)spinBox, &event);
    }
    return 1;
}



void guiSpinBox_SetActiveDigit(guiSpinBox_t *spinBox, int8_t num)
{
    uint8_t newActiveDigit;
    guiEvent_t event;
    if (spinBox != 0)
    {
        newActiveDigit = (num >= spinBox->digitsToDisplay) ? spinBox->digitsToDisplay - 1 :
                                                             ((num < 0) ? 0 : num);
        if (newActiveDigit != spinBox->activeDigit)
        {
            spinBox->activeDigit = newActiveDigit;
            spinBox->redrawDigitSelection = 1;
            spinBox->redrawRequired = 1;
            // Call handler
            event.type = SPINBOX_ACTIVE_DIGIT_CHANGED;
            guiCore_CallEventHandler((guiGenericWidget_t *)spinBox, &event);
        }
    }
}


void guiSpinBox_SetValue(guiSpinBox_t *spinBox, int32_t value)
{
    int32_t newValue;
    guiEvent_t event;
    if (spinBox != 0)
    {
        newValue = (value < spinBox->minValue) ? spinBox->minValue :
                   ((value > spinBox->maxValue) ? spinBox->maxValue : value);

        if (newValue != spinBox->value)
        {
            spinBox->value = newValue;
            spinBox->redrawValue = 1;
            spinBox->redrawRequired = 1;
            spinBox->digitsToDisplay = i32toa_align_right(spinBox->value, spinBox->text,
                                       SPINBOX_STRING_LENGTH | NO_TERMINATING_ZERO, spinBox->minDigitsToDisplay);
            // Call handler
            event.type = SPINBOX_VALUE_CHANGED;
            guiCore_CallEventHandler((guiGenericWidget_t *)spinBox, &event);
            // Check active digit position
            guiSpinBox_SetActiveDigit(spinBox, spinBox->activeDigit);
        }
    }
}

void guiSpinBox_IncrementValue(guiSpinBox_t *spinBox, int32_t delta)
{
    int32_t mul_c = 1;
    uint8_t i;
    if ((spinBox != 0) && (delta != 0))
    {
        for (i=0;i<spinBox->activeDigit;i++)
        {
            mul_c *= 10;
        }
        delta *= mul_c;
        guiSpinBox_SetValue(spinBox, spinBox->value + delta);
    }
}




//-------------------------------------------------------//
// SpinBox key handler
//
// Returns GUI_EVENT_ACCEPTED if key is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiSpinBox_ProcessKey(guiSpinBox_t *spinBox, uint8_t key)
{
    if (spinBox->isActive)
    {
        if (key == SPINBOX_KEY_SELECT)
        {
            guiSpinBox_SetActive(spinBox, 0, 0);
        }
        else if (key == SPINBOX_KEY_EXIT)
        {
            guiSpinBox_SetActive(spinBox, 0, 1);
        }
        else if (key == SPINBOX_KEY_LEFT)
        {
            // move active digit left
            guiSpinBox_SetActiveDigit(spinBox, spinBox->activeDigit + 1);
        }
        else if (key == SPINBOX_KEY_RIGHT)
        {
            // move active digit right
            guiSpinBox_SetActiveDigit(spinBox, spinBox->activeDigit - 1);
        }
        else if (key == SPINBOX_KEY_UP)
        {
            // increase value
            guiSpinBox_IncrementValue(spinBox, 1);
        }
        else if (key == SPINBOX_KEY_DOWN)
        {
            //decrease value
            guiSpinBox_IncrementValue(spinBox, -1);
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
            guiSpinBox_SetActive(spinBox, 1, 0);
        }
        else
        {
            return GUI_EVENT_DECLINE;
        }
    }

    return GUI_EVENT_ACCEPTED;
}



//-------------------------------------------------------//
// spinBox event handler
//
// Returns GUI_EVENT_ACCEPTED if event is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiSpinBox_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    guiSpinBox_t *spinBox = (guiSpinBox_t *)widget;
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    uint8_t key;

    switch (event.type)
    {
        case GUI_EVENT_DRAW:
            guiGraph_DrawSpinBox(spinBox);
            // Call handler
            guiCore_CallEventHandler(widget, &event);
            // Reset flags - redrawForced will be reset by core
            spinBox->redrawFocus = 0;
            spinBox->redrawDigitSelection = 0;
            spinBox->redrawValue = 0;
            spinBox->redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:
            if (SPINBOX_ACCEPTS_FOCUS_EVENT(spinBox))
                guiCore_SetFocused((guiGenericWidget_t *)spinBox,1);
            else
                processResult = GUI_EVENT_DECLINE;      // Cannot accept focus
            break;
        case GUI_EVENT_UNFOCUS:
            guiCore_SetFocused((guiGenericWidget_t *)spinBox,0);
            guiSpinBox_SetActive(spinBox, 0, 0);        // Do not restore
            //spinBox->keepTouch = 0;
            break;
        case SPINBOX_EVENT_ACTIVATE:
            if (spinBox->isFocused)
            {
                guiSpinBox_SetActive(spinBox, 1, 0);
            }
            // Accept event anyway
            break;
        case GUI_EVENT_SHOW:
            guiCore_SetVisible((guiGenericWidget_t *)spinBox, 1);
            break;
        case GUI_EVENT_HIDE:
            guiCore_SetVisible((guiGenericWidget_t *)spinBox, 0);
            break;
        case GUI_EVENT_ENCODER:
            processResult = GUI_EVENT_DECLINE;
            if (SPINBOX_ACCEPTS_ENCODER_EVENT(spinBox))
            {
                if (spinBox->isActive)
                {
                    guiSpinBox_IncrementValue(spinBox, (int16_t)event.lparam);
                    processResult = GUI_EVENT_ACCEPTED;
                }
                processResult |= guiCore_CallEventHandler(widget, &event);
            }
            break;
        case GUI_EVENT_KEY:
            processResult = GUI_EVENT_DECLINE;
            if (SPINBOX_ACCEPTS_KEY_EVENT(spinBox))
            {
                if (event.spec == GUI_KEY_EVENT_DOWN)
                {
                    if (event.lparam == GUI_KEY_OK)
                        key = SPINBOX_KEY_SELECT;
                    else if (event.lparam == GUI_KEY_ESC)
                        key = SPINBOX_KEY_EXIT;
                    else if (event.lparam == GUI_KEY_LEFT)
                        key = SPINBOX_KEY_LEFT;
                    else if (event.lparam == GUI_KEY_RIGHT)
                        key = SPINBOX_KEY_RIGHT;
                    else if (event.lparam == GUI_KEY_UP)
                        key = SPINBOX_KEY_UP;
                    else if (event.lparam == GUI_KEY_DOWN)
                        key = SPINBOX_KEY_DOWN;
                    else
                        key = 0;
                    if (key != 0)
                        processResult = guiSpinBox_ProcessKey(spinBox,key);
                }
                // Call KEY event handler
                processResult |= guiCore_CallEventHandler(widget, &event);
            }
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
void guiSpinBox_Initialize(guiSpinBox_t *spinBox, guiGenericWidget_t *parent)
{
    memset(spinBox, 0, sizeof(*spinBox));
    spinBox->type = WT_SPINBOX;
    spinBox->parent = parent;
    spinBox->acceptFocusByTab = 1;
    spinBox->isVisible = 1;
    spinBox->showFocus = 1;
    spinBox->processEvent = guiSpinBox_ProcessEvent;
    spinBox->minDigitsToDisplay = 1;
    spinBox->maxValue = INT32_MAX;
    spinBox->minValue = INT32_MIN;
}


