/**********************************************************
    Module button


**********************************************************/

#include <stdint.h>         // using integer types
#include <string.h>         // using memset
#include "guiEvents.h"
#include "guiCore.h"
#include "guiWidgets.h"
#include "guiRadioButton.h"
#include "guiGraphWidgets.h"






//-------------------------------------------------------//
// Sets checked state
//
// This function does not perform any widget state checks
//      except isChecked state.
// Returns 1 if new state was applied. Otherwise returns 0.
//-------------------------------------------------------//
uint8_t guiRadioButton_SetChecked(guiRadioButton_t *button, uint8_t newCheckedState, uint8_t callHandler)
{
    guiEvent_t event;
    if (button == 0) return 0;

    if (newCheckedState)
    {
        // Check
        if (button->isChecked) return 0;
        button->isChecked = 1;
    }
    else
    {
        // Uncheck
        if (button->isChecked == 0) return 0;
        button->isChecked = 0;
    }
    // Checked state changed - call handler
    button->redrawCheckedState = 1;
    button->redrawRequired = 1;
    if (callHandler)
    {
        event.type = RADIOBUTTON_CHECKED_CHANGED;
        guiCore_CallEventHandler((guiGenericWidget_t *)button, &event);
    }
    return 1;
}


//-------------------------------------------------------//
// Checks specified radiobutton and clears other
// radiobuttons which have same parent and radioIndex.
//
// This function does not perform any widget state checks
//      except isChecked state.
//-------------------------------------------------------//
void guiRadioButton_CheckExclusive(guiRadioButton_t *button, uint8_t callHandler)
{
    uint8_t i;
    guiGenericContainer_t *parent;
    guiGenericWidget_t *w;
    if ((button == 0) || (button->isChecked))  return;
    parent = (guiGenericContainer_t *)button->parent;
    for (i=0; i<parent->widgets.count; i++)
    {
        w = parent->widgets.elements[i];
        if ((w != 0) && (w->type == WT_RADIOBUTTON))
        {
            if ((((guiRadioButton_t *)w)->radioIndex == button->radioIndex) &&
                    (((guiRadioButton_t *)w)->isChecked))
                guiRadioButton_SetChecked((guiRadioButton_t *)w, 0, callHandler);
        }
    }
    guiRadioButton_SetChecked(button, 1, callHandler);
}


//-------------------------------------------------------//
// RadioButton key handler
//
// Returns GUI_EVENT_ACCEPTED if key is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiRadioButton_ProcessKey(guiRadioButton_t *button, uint8_t key)
{
    if (key == RADIOBUTTON_KEY_SELECT)
    {
        guiRadioButton_CheckExclusive(button, 1);
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
uint8_t guiRadioButton_DefaultKeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiRadioButtonTranslatedKey_t *tkey = (guiRadioButtonTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_OK)
            tkey->key = RADIOBUTTON_KEY_SELECT;
    }
    return 0;
}



uint8_t guiRadioButton_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    guiRadioButton_t *button = (guiRadioButton_t *)widget;
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    guiRadioButtonTranslatedKey_t tkey;
#ifdef emGUI_USE_TOUCH_SUPPORT
    widgetTouchState_t touch;
#endif

    switch (event.type)
    {
        case GUI_EVENT_DRAW:
            guiGraph_DrawRadioButton(button);
            // Call handler
            guiCore_CallEventHandler(widget, &event);
            // Reset flags - redrawForced will be reset by core
            button->redrawFocus = 0;
            button->redrawCheckedState = 0;
            button->redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:
            if (RADIOBUTTON_ACCEPTS_FOCUS_EVENT(button))
                guiCore_SetFocused((guiGenericWidget_t *)button,1);
            else
                processResult = GUI_EVENT_DECLINE;      // Cannot accept focus
            break;
        case GUI_EVENT_UNFOCUS:
            guiCore_SetFocused((guiGenericWidget_t *)button,0);
            button->keepTouch = 0;
            break;
        case GUI_EVENT_SHOW:
            guiCore_SetVisible((guiGenericWidget_t *)button, 1);
            break;
        case GUI_EVENT_HIDE:
            guiCore_SetVisible((guiGenericWidget_t *)button, 0);
            break;
        case GUI_EVENT_KEY:
            processResult = GUI_EVENT_DECLINE;
            if (RADIOBUTTON_ACCEPTS_KEY_EVENT(button))
            {
                if (button->keyTranslator)
                {
                    processResult = button->keyTranslator(widget, &event, &tkey);
                    if (tkey.key != 0)
                        processResult |= guiRadioButton_ProcessKey(button, tkey.key);
                }
                // Call KEY event handler
                if (processResult == GUI_EVENT_DECLINE)
                    processResult = guiCore_CallEventHandler(widget, &event);
            }
            break;
#ifdef emGUI_USE_TOUCH_SUPPORT
        case GUI_EVENT_TOUCH:
            if (RADIOBUTTON_ACCEPTS_TOUCH_EVENT(button))
            {
                // Convert touch event
                guiCore_DecodeWidgetTouchEvent((guiGenericWidget_t *)button, &event, &touch);
                // Check if widget holds the touch
                if (button->keepTouch)
                {
                    if (touch.state == TOUCH_RELEASE)
                    {
                        button->keepTouch = 0;
                    }
                }
                else if (touch.state != TOUCH_RELEASE)
                {
                    // New touch event to widget - it can either capture touch, or skip event to parent
                    if (touch.isInsideWidget)
                    {
                        // Capture
                        guiCore_SetFocused((guiGenericWidget_t *)button,1);
                        guiRadioButton_ProcessKey(button, RADIOBUTTON_KEY_SELECT);
                        button->keepTouch = 1;
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
// Default initialization
//
//-------------------------------------------------------//
void guiRadioButton_Initialize(guiRadioButton_t *button, guiGenericWidget_t *parent)
{
    memset(button, 0, sizeof(guiRadioButton_t));
    button->type = WT_RADIOBUTTON;
    button->parent = parent;
    button->acceptFocusByTab = 1;
    button->acceptTouch = 1;
    button->isVisible = 1;
    button->showFocus = 1;
    button->processEvent = guiRadioButton_ProcessEvent;
    button->keyTranslator = guiRadioButton_DefaultKeyTranslator;
    button->textAlignment = ALIGN_LEFT;
}



