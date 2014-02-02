/**********************************************************
    Module guiPanel




**********************************************************/

#include <stdint.h>         // using integer types
#include <string.h>         // using memset
#include "guiConfig.h"
#include "guiEvents.h"
#include "guiCore.h"
#include "guiWidgets.h"
#include "guiGraphWidgets.h"
#include "guiPanel.h"


//-------------------------------------------------------//
// Panel key handler
//
// Returns GUI_EVENT_ACCEPTED if key is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiPanel_ProcessKey(guiPanel_t *panel, uint8_t key)
{
    guiGenericWidget_t *w;
    if (panel->isFocused)
    {
        if (key == PANEL_KEY_SELECT)
            guiCore_RequestFocusNextWidget((guiGenericContainer_t *)panel,1);
        else
            return GUI_EVENT_DECLINE;
    }
    else
    {
        // Key event came from child elements
        w = panel->widgets.elements[ panel->widgets.focusedIndex ];
        if (key == PANEL_KEY_PREV)
        {
            // Check if event should be passed to parent
            if ((panel->focusFallsThrough) && (guiCore_CheckWidgetTabIndex(w) == TABINDEX_IS_MIN))
                return GUI_EVENT_DECLINE;
            guiCore_RequestFocusNextWidget((guiGenericContainer_t *)panel,-1);
        }
        else if (key == PANEL_KEY_NEXT)
        {
            // Check if event should be passed to parent
            if ((panel->focusFallsThrough) && (guiCore_CheckWidgetTabIndex(w) == TABINDEX_IS_MAX))
                return GUI_EVENT_DECLINE;
            guiCore_RequestFocusNextWidget((guiGenericContainer_t *)panel,1);
        }
        else if (key == PANEL_KEY_ESC)
        {
            if (panel->focusFallsThrough)
                return GUI_EVENT_DECLINE;
            //guiPanel_SetFocused(panel, 1);
            guiCore_SetFocused((guiGenericWidget_t *)panel,1);
        }
        else
        {
            return GUI_EVENT_DECLINE;   // unknown key
        }
    }
    return GUI_EVENT_ACCEPTED;
}




//-------------------------------------------------------//
// Panel event handler
//
// Returns GUI_EVENT_ACCEPTED if event is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiPanel_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    guiPanel_t *panel = (guiPanel_t *)widget;
    uint8_t key;
#ifdef USE_TOUCH_SUPPORT
    containerTouchState_t touch;
#endif

    // Process GUI messages - focus, draw, etc
    switch(event.type)
    {
        case GUI_EVENT_DRAW:
            guiGraph_DrawPanel(panel);
            // Call handler
            guiCore_CallEventHandler((guiGenericWidget_t *)panel, &event);
            // Reset flags - redrawForced will be reset by core
            panel->redrawFocus = 0;
            panel->redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:
            if (PANEL_ACCEPTS_FOCUS_EVENT(panel))
            {
                guiCore_SetFocused((guiGenericWidget_t *)panel,1);
                if (panel->focusFallsThrough)
                {
                    guiCore_RequestFocusNextWidget((guiGenericContainer_t *)panel,1);
                    //guiCore_RequestFocusChange(panel->widgets.elements[ panel->widgets.focusedIndex ]);
                }
            }
            else
            {
                processResult = GUI_EVENT_DECLINE;      // Cannot accept focus
            }
            break;
        case GUI_EVENT_UNFOCUS:
            guiCore_SetFocused((guiGenericWidget_t *)panel, 0);
            panel->keepTouch = 0;
            break;
        case GUI_EVENT_SHOW:
            guiCore_SetVisible((guiGenericWidget_t *)panel, 1);
            break;
        case GUI_EVENT_HIDE:
            guiCore_SetVisible((guiGenericWidget_t *)panel, 0);
            break;
        case GUI_EVENT_ENCODER:
            processResult = GUI_EVENT_DECLINE;
            if (PANEL_ACCEPTS_ENCODER_EVENT(panel))
            {
                key = (int16_t)event.lparam < 0 ? PANEL_KEY_PREV :
                      ((int16_t)event.lparam > 0 ? PANEL_KEY_NEXT : 0);
                if (key != 0)
                    processResult = guiPanel_ProcessKey(panel,key);
                // Call ENCODER event handler
                processResult |= guiCore_CallEventHandler(widget, &event);
            }
            break;
        case GUI_EVENT_KEY:
            processResult = GUI_EVENT_DECLINE;
            if (PANEL_ACCEPTS_KEY_EVENT(panel))
            {
                if (event.spec == GUI_KEY_EVENT_DOWN)
                {
                    if (event.lparam == GUI_KEY_OK)
                        key = PANEL_KEY_SELECT;
                    else if (event.lparam == GUI_KEY_ESC)
                        key = PANEL_KEY_ESC;
                    else if ((event.lparam == GUI_KEY_LEFT) || (event.lparam == GUI_KEY_UP))
                        key = PANEL_KEY_PREV;
                    else if ((event.lparam == GUI_KEY_RIGHT) || (event.lparam == GUI_KEY_DOWN))
                        key = PANEL_KEY_NEXT;
                    else
                        key = 0;
                    if (key != 0)
                        processResult = guiPanel_ProcessKey(panel,key);
                }
                // Call KEY event handler
                processResult |= guiCore_CallEventHandler(widget, &event);
            }
            break;
#ifdef USE_TOUCH_SUPPORT
        case GUI_EVENT_TOUCH:
            if (PANEL_ACCEPTS_TOUCH_EVENT(panel))
            {
                // Convert touch event
                guiCore_DecodeContainerTouchEvent((guiGenericWidget_t *)panel, &event, &touch);
                // Check if widget holds the touch
                if (panel->keepTouch)
                {
                    if (touch.state == TOUCH_RELEASE)
                        panel->keepTouch = 0;
                }
                else if (touch.state != TOUCH_RELEASE)
                {
                    // Determine if touch point lies inside one of child widgets
                    if (touch.widgetAtXY == 0)
                    {
                        // Touch point does not lie inside panel - skip that.
                        processResult = GUI_EVENT_DECLINE;
                        break;
                    }
                    else if (touch.widgetAtXY != (guiGenericWidget_t *)panel)
                    {
                        // Point belogs to one of child widgets - pass touch event to it.
                        guiCore_AddMessageToQueue(touch.widgetAtXY, &event);
                    }
                    else
                    {
                        // Point belogs to panel itself
                        guiCore_SetFocused((guiGenericWidget_t *)panel,1);
                        panel->keepTouch = 1;
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
void guiPanel_Initialize(guiPanel_t *panel, guiGenericWidget_t *parent)
{
    memset(panel, 0, sizeof(*panel));
    panel->type = WT_PANEL;
    panel->parent = parent;
    panel->isContainer = 1;
    panel->acceptTouch = 1;
    panel->acceptFocusByTab = 1;
    panel->focusFallsThrough = 1;
    panel->processEvent = guiPanel_ProcessEvent;
    panel->frame = FRAME_NONE;
}



