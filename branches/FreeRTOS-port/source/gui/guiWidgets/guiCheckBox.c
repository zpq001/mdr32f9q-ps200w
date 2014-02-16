/**********************************************************
    Module checkBox




**********************************************************/

#include <stdint.h>         // using integer types
#include <string.h>         // using memset
#include "guiEvents.h"
#include "guiCore.h"
#include "guiWidgets.h"
#include "guiGraphWidgets.h"
#include "guiCheckBox.h"



//-------------------------------------------------------//
// Sets checked state
//
// This function does not perform any widget state checks
//      except isChecked state.
// Returns 1 if new state was applied. Otherwise returns 0.
//-------------------------------------------------------//
uint8_t guiCheckbox_SetChecked(guiCheckBox_t *checkBox, uint8_t newCheckedState, uint8_t callHandler)
{
    guiEvent_t event;
    if (checkBox == 0) return 0;

    if (newCheckedState)
    {
        // Check
        if (checkBox->isChecked) return 0;
        checkBox->isChecked = 1;
    }
    else
    {
        // Uncheck
        if (checkBox->isChecked == 0) return 0;
        checkBox->isChecked = 0;
    }
    // Checked state changed - call handler
    checkBox->redrawCheckedState = 1;
    checkBox->redrawRequired = 1;
    if (callHandler)
    {
        event.type = CHECKBOX_CHECKED_CHANGED;
        guiCore_CallEventHandler((guiGenericWidget_t *)checkBox, &event);
    }
    return 1;
}


//-------------------------------------------------------//
// Checkbox key handler
//
// Returns GUI_EVENT_ACCEPTED if key is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiCheckbox_ProcessKey(guiCheckBox_t *checkBox, uint8_t key)
{
    if (key == CHECKBOX_KEY_SELECT)
    {
        guiCheckbox_SetChecked(checkBox, !checkBox->isChecked, 1);
    }
    else
    {
        return GUI_EVENT_DECLINE;   // unknown key
    }
    return GUI_EVENT_ACCEPTED;
}





//-------------------------------------------------------//
// Default key event translator
//
//-------------------------------------------------------//
uint8_t checkBox_DefaultKeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiCheckboxTranslatedKey_t *tkey = (guiCheckboxTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_OK)
            tkey->key = CHECKBOX_KEY_SELECT;
    }
    return 0;
}


//-------------------------------------------------------//
// Checkbox event handler
//
// Returns GUI_EVENT_ACCEPTED if event is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiCheckBox_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    guiCheckBox_t *checkBox = (guiCheckBox_t *)widget;
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    guiCheckboxTranslatedKey_t tkey;
#ifdef USE_TOUCH_SUPPORT
    widgetTouchState_t touch;
#endif

    switch (event.type)
    {
        case GUI_EVENT_DRAW:
            guiGraph_DrawCheckBox(checkBox);
            // Call handler
            guiCore_CallEventHandler(widget, &event);
            // Reset flags - redrawForced will be reset by core
            checkBox->redrawFocus = 0;
            checkBox->redrawCheckedState = 0;
            checkBox->redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:
            if (CHECKBOX_ACCEPTS_FOCUS_EVENT(checkBox))
                guiCore_SetFocused((guiGenericWidget_t *)checkBox,1);
            else
                processResult = GUI_EVENT_DECLINE;      // Cannot accept focus
            break;
        case GUI_EVENT_UNFOCUS:
            guiCore_SetFocused((guiGenericWidget_t *)checkBox,0);
            checkBox->keepTouch = 0;
            break;
        case GUI_EVENT_SHOW:
            guiCore_SetVisible((guiGenericWidget_t *)checkBox, 1);
            break;
        case GUI_EVENT_HIDE:
            guiCore_SetVisible((guiGenericWidget_t *)checkBox, 0);
            break;
        case GUI_EVENT_KEY:
            processResult = GUI_EVENT_DECLINE;
            if (CHECKBOX_ACCEPTS_KEY_EVENT(checkBox))
            {
                if (checkBox->keyTranslator)
                {
                    processResult = checkBox->keyTranslator(widget, &event, &tkey);
                    if (tkey.key != 0)
                        processResult |= guiCheckbox_ProcessKey(checkBox, tkey.key);
                }
                // Call KEY event handler
                if (processResult == GUI_EVENT_DECLINE)
                    processResult = guiCore_CallEventHandler(widget, &event);
            }
            break;
#ifdef USE_TOUCH_SUPPORT
        case GUI_EVENT_TOUCH:
            if (CHECKBOX_ACCEPTS_TOUCH_EVENT(checkBox))
            {
                // Convert touch event
                guiCore_DecodeWidgetTouchEvent((guiGenericWidget_t *)checkBox, &event, &touch);
                // Check if widget holds the touch
                if (checkBox->keepTouch)
                {
                    if (touch.state == TOUCH_RELEASE)
                        checkBox->keepTouch = 0;
                }
                else if (touch.state != TOUCH_RELEASE)
                {
                    // New touch event to widget - it can either capture touch, or skip event to parent
                    if (touch.isInsideWidget)
                    {
                        // Capture
                        guiCore_SetFocused((guiGenericWidget_t *)checkBox,1);
                        guiCheckbox_SetChecked(checkBox, !checkBox->isChecked);
                        checkBox->keepTouch = 1;
                    }
                    else
                    {
                        // Skip
                        processResult = GUI_EVENT_DECLINE;
                        break;
                    }
                }
                // Call touch handler - return value is ignored
                event.type = GUI_ON_TOUCH_EVENT;
                event.spec = touch.state;
                event.lparam = (uint16_t)touch.x;
                event.hparam = (uint16_t)touch.y;
                guiCore_CallEventHandler(widget, &event);
            }
            else
            {
                processResult = GUI_EVENT_DECLINE;      // Cannot process touch event
            }
            break;
#endif
        default:
            // Widget cannot process incoming event. Try to find a handler.
            processResult = guiCore_CallEventHandler(widget, &event);
    }
    return processResult;
}


//-------------------------------------------------------//
// Set checkbox text and flags to redraw
//
//-------------------------------------------------------//
void guiCheckBox_SetText(guiCheckBox_t *checkBox, char *text)
{
    checkBox->text = text;
    checkBox->redrawRequired = 1;
    checkBox->redrawForced = 1;
}


//-------------------------------------------------------//
// Default initialization
//
//-------------------------------------------------------//
void guiCheckBox_Initialize(guiCheckBox_t *checkBox, guiGenericWidget_t *parent)
{
    memset(checkBox, 0, sizeof(*checkBox));
    checkBox->type = WT_CHECKBOX;
    checkBox->parent = parent;
    checkBox->acceptFocusByTab = 1;
    checkBox->acceptTouch = 1;
    checkBox->isVisible = 1;
    checkBox->showFocus = 1;
    checkBox->processEvent = guiCheckBox_ProcessEvent;
    checkBox->keyTranslator = checkBox_DefaultKeyTranslator;
    checkBox->textAlignment = ALIGN_LEFT;
}





