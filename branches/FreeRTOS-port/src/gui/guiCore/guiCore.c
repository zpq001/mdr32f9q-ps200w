/**********************************************************
    Module guiCore

Widget's event processing function calls:

Direct:
    guiCore_ProcessMessageQueue
    guiCore_SetVisibleByTag
    guiCore_BroadcastEvent
    guiCore_RedrawAll
    guiCore_UpdateAll (using guiCore_BroadcastEvent)

Through message queue:
    guiCore_PostEventToFocused
    guiCore_TimerProcess
    guiCore_Init
    guiCore_ProcessTouchEvent
    guiCore_ProcessKeyEvent
    guiCore_ProcessEncoderEvent
    guiCore_ProcessTimers (using guiCore_TimerProcess)
    guiCore_RequestFocusChange
    guiCore_AcceptFocus
    guiCore_RequestFocusNextWidget (using guiCore_RequestFocusChange)
    guiCore_SetFocused (using guiCore_AcceptFocus)

**********************************************************/

#include <stdint.h>
#include "guiConfig.h"
#include "guiGraphWidgets.h"
#include "guiEvents.h"
#include "guiWidgets.h"
#include "guiCore.h"


// Predefined constant events - saves stack a bit
const guiEvent_t guiEvent_INIT = {GUI_EVENT_INIT, 0, 0, 0};
const guiEvent_t guiEvent_DRAW = {GUI_EVENT_DRAW, 0, 0, 0};
#ifdef CFG_USE_UPDATE
const guiEvent_t guiEvent_UPDATE = {GUI_EVENT_UPDATE, 0, 0, 0};
#endif
const guiEvent_t guiEvent_HIDE = {GUI_EVENT_HIDE, 0, 0, 0};
const guiEvent_t guiEvent_SHOW = {GUI_EVENT_SHOW, 0, 0, 0};
const guiEvent_t guiEvent_UNFOCUS = {GUI_EVENT_UNFOCUS, 0, 0, 0};
const guiEvent_t guiEvent_FOCUS = {GUI_EVENT_FOCUS, 0, 0, 0};

#ifdef GUI_CFG_USE_TIMERS
// Total count of timers should be defined in guiConfig.h
guiTimer_t guiTimers[GUI_TIMER_COUNT];
#endif

guiMsgQueue_t guiMsgQueue;
guiGenericWidget_t *rootWidget;         // Root widget must be present
guiGenericWidget_t *focusedWidget;      // Focused widget gets events from keys/encoder/touch




//===================================================================//
//===================================================================//
//                                                                   //
//                 GUI core message queue functions                  //
//                                                                   //
//===================================================================//


//-------------------------------------------------------//
//  Adds a message to GUI core message queue
//
//  Returns 0 if there is no space left
//  Returns non-zero if message has been put
//-------------------------------------------------------//
uint8_t guiCore_AddMessageToQueue(const guiGenericWidget_t *target, const guiEvent_t *event)
{
    if (guiMsgQueue.count < GUI_CORE_QUEUE_SIZE)
    {
        guiMsgQueue.queue[guiMsgQueue.tail].event = *event;
        guiMsgQueue.queue[guiMsgQueue.tail].target = (guiGenericWidget_t *)target;
        guiMsgQueue.count++;
        guiMsgQueue.tail++;
        if (guiMsgQueue.tail == GUI_CORE_QUEUE_SIZE)
            guiMsgQueue.tail = 0;
        return 1;
     }
    return 0;
}

//-------------------------------------------------------//
//  Reads a message from GUI core message queue
//
//  Returns 0 if queue is empty
//  Returns non-zero if message has been read
//-------------------------------------------------------//
uint8_t guiCore_GetMessageFromQueue(guiGenericWidget_t **target, guiEvent_t *event)
{
    if (guiMsgQueue.count > 0)
    {
        *target = guiMsgQueue.queue[guiMsgQueue.head].target;
        *event = guiMsgQueue.queue[guiMsgQueue.head].event;
        guiMsgQueue.count--;
        guiMsgQueue.head++;
        if (guiMsgQueue.head == GUI_CORE_QUEUE_SIZE)
            guiMsgQueue.head = 0;
        return 1;
    }
    return 0;
}


//-------------------------------------------------------//
//  GUI core function for processing message queue
//
// Generally, if message target widget cannot process
// event, message is sent to it's parent and so on
// until it is accepted or root widget is sent the message
// If root cannot process this message, it is lost.
//-------------------------------------------------------//
void guiCore_ProcessMessageQueue(void)
{
    guiGenericWidget_t *target;
    guiEvent_t targetEvent;
    uint8_t processResult;

    while(guiCore_GetMessageFromQueue(&target,&targetEvent))
    {
        while(1)
        {
            if (target == 0)
                break;
            processResult = target->processEvent(target, targetEvent);
            if (processResult == GUI_EVENT_ACCEPTED)
                break;
            // Focused widget cannot process event - pass event to parent
            if (target->parent != 0)
            {
                target = target->parent;
            }
            else
            {
                // No widget can process this event - skip it.
                break;
            }
        }
    }
}



//-------------------------------------------------------//
//  Adds message for focused widget to message queue
//
//-------------------------------------------------------//
void guiCore_PostEventToFocused(guiEvent_t event)
{
    if (focusedWidget == 0)
        return;                 // Should not normally happen ?
    guiCore_AddMessageToQueue(focusedWidget, &event);
}





//===================================================================//
//===================================================================//
//                                                                   //
//                      GUI core timers functions                    //
//                                                                   //
//===================================================================//


#ifdef GUI_CFG_USE_TIMERS
//-------------------------------------------------------//
//  Initializes GUI core timer
//  Timer is identified by timerID which is simply index
//  of element in guiTimers[]
//
//-------------------------------------------------------//
void guiCore_TimerInit(uint8_t timerID, uint16_t period, uint8_t runOnce, guiGenericWidget_t *target, void (*handler)(uint8_t))
{
    if (timerID >= GUI_TIMER_COUNT)
        return;
    guiTimers[timerID].top = period;
    guiTimers[timerID].counter = 0;
    guiTimers[timerID].runOnce = (runOnce) ? 1 : 0;
    guiTimers[timerID].isEnabled = 0;
    guiTimers[timerID].targetWidget = target;
    guiTimers[timerID].handler = handler;
}

//-------------------------------------------------------//
//  Starts GUI core timer
//  Timer is identified by timerID which is simply index
//  of element in guiTimers[]
//
//  If doReset is non-zero, timer will be set to 0
//-------------------------------------------------------//
void guiCore_TimerStart(uint8_t timerID, uint8_t doReset)
{
    if (timerID >= GUI_TIMER_COUNT)
        return;
    if (doReset)
        guiTimers[timerID].counter = 0;
    guiTimers[timerID].isEnabled = 1;
}

//-------------------------------------------------------//
//  Stops GUI core timer
//  Timer is identified by timerID which is simply index
//  of element in guiTimers[]
//
//  If doReset is non-zero, timer will be set to 0
//-------------------------------------------------------//
void guiCore_TimerStop(uint8_t timerID, uint8_t doReset)
{
    if (timerID >= GUI_TIMER_COUNT)
        return;
    if (doReset)
        guiTimers[timerID].counter = 0;
    guiTimers[timerID].isEnabled = 0;
}

//-------------------------------------------------------//
//  Processes GUI core timer
//  Timer is identified by timerID which is simply index
//  of element in guiTimers[]
//
//  Timer's counter is incremented. If counter has reached
//  top, in is reset and timer event is sent to the target widget
//  Additionaly, handler is called (if specified)
//-------------------------------------------------------//
void guiCore_TimerProcess(uint8_t timerID)
{
    guiEvent_t event = {GUI_EVENT_TIMER, 0, 0, 0};
    if (timerID >= GUI_TIMER_COUNT)
        return;
    if (guiTimers[timerID].isEnabled)
    {
        guiTimers[timerID].counter++;
        if (guiTimers[timerID].counter >= guiTimers[timerID].top)
        {
            guiTimers[timerID].counter = 0;
            if (guiTimers[timerID].runOnce)
                guiTimers[timerID].isEnabled = 0;
            if (guiTimers[timerID].targetWidget != 0)
            {
                event.spec = timerID;
                guiCore_AddMessageToQueue(guiTimers[timerID].targetWidget, &event);
            }
            if (guiTimers[timerID].handler != 0)
                guiTimers[timerID].handler(timerID);
        }
    }
}
#endif





//===================================================================//
//===================================================================//
//                                                                   //
//                      Top GUI core functions                       //
//                                                                   //
//===================================================================//




//-------------------------------------------------------//
//  Top function for GUI core initializing
//  All components must be already initialized
//-------------------------------------------------------//
void guiCore_Init(guiGenericWidget_t *guiRootWidget)
{
    uint8_t i;
    // Init queue
    guiMsgQueue.count = 0;
    guiMsgQueue.head = 0;
    guiMsgQueue.tail = 0;

#ifdef GUI_CFG_USE_TIMERS
    // Disable all timers
    for (i=0; i<GUI_TIMER_COUNT; i++)
    {
        guiTimers[i].isEnabled = 0;
    }
#endif

    // Set root and focused widget and send initialize event
    // Root widget must set focus in itself or other widget
    // depending on design. If focus is not set, no keyboard
    // and encoder events will get processed.
    rootWidget = guiRootWidget;
    focusedWidget = 0;
    guiCore_AddMessageToQueue(rootWidget, &guiEvent_INIT);
    guiCore_ProcessMessageQueue();
}



//-------------------------------------------------------//
//  Top function for GUI redrawing
//
//-------------------------------------------------------//
void guiCore_RedrawAll(void)
{
    guiGenericWidget_t *widget;
    guiGenericWidget_t *nextWidget;
    uint8_t index;

    guiGenericWidget_t *w;
    uint8_t i;
    rect16_t inv_rect;

    // Start widget tree traverse from root widget
    widget = rootWidget;
    guiGraph_SetBaseXY(widget->x, widget->y);

    while(1)
    {
        // Process widget
        if (widget->redrawRequired)
        {
            // The redrawRequired flag is reset by widget after processing DRAW event
            widget->processEvent(widget, guiEvent_DRAW);
        }
        // Check if widget has children
        if (widget->isContainer)
        {
            if (((guiGenericContainer_t *)widget)->widgets.traverseIndex == 0)
            {
                // The first time visit
                // TODO - set graph clipping
            }

            // If container has unprocessed children
            if ( ((guiGenericContainer_t *)widget)->widgets.traverseIndex <
                 ((guiGenericContainer_t *)widget)->widgets.count )
            {
                // switch to next one if possible
                index = ((guiGenericContainer_t *)widget)->widgets.traverseIndex++;
                nextWidget = ((guiGenericContainer_t *)widget)->widgets.elements[index];
                // check if widget actually exists and is visible
                if ((nextWidget == 0) || (nextWidget->isVisible == 0))
                    continue;
                // Check if widget must be redrawn forcibly
                if (widget->redrawForced)
                {
                    nextWidget->redrawForced = 1;
                    nextWidget->redrawRequired = 1;
                }
                ///////////////////////////
#ifdef USE_Z_ORDER_REDRAW
                if ((widget->redrawForced == 0) &&(nextWidget->redrawRequired))
                {
                    // Widget will be redrawn - make overlapping widgets with higher Z index redraw too
                    inv_rect.x1 = nextWidget->x;
                    inv_rect.y1 = nextWidget->y;
                    inv_rect.x2 = nextWidget->x + nextWidget->width - 1;
                    inv_rect.y2 = nextWidget->y + nextWidget->height - 1;
                    for (i = index+1; i < ((guiGenericContainer_t *)widget)->widgets.count; i++)
                    {
                        w = ((guiGenericContainer_t *)widget)->widgets.elements[i];
                        if ((w != 0) && (w->isVisible))
                        {
                            if (guiCore_CheckWidgetOvelap(w, &inv_rect))
                            {
                                w->redrawForced = 1;
                                w->redrawRequired = 1;
                            }
                        }
                    }
                }
#endif
                ///////////////////////////
                if ((nextWidget->redrawRequired) || (nextWidget->isContainer))
                {
                    widget = nextWidget;
                    guiGraph_OffsetBaseXY(widget->x, widget->y);
                }
            }
            else
            {
                // All container child items are processed. Reset counter of processed items and move up.
                ((guiGenericContainer_t *)widget)->widgets.traverseIndex = 0;
                widget->redrawForced = 0;
                if (widget->parent == 0)    // root widget has no parent
                    break;
                else
                {
                    guiGraph_OffsetBaseXY(-widget->x, -widget->y);
                    widget = widget->parent;
                }
            }
        }
        else
        {
            // Widget has no children. Move up.
            widget->redrawForced = 0;
            guiGraph_OffsetBaseXY(-widget->x, -widget->y);
            widget = widget->parent;
        }
    }
}


//-------------------------------------------------------//
//  Top function for touchscreen processing
//  x and y are absolute coordinates
//  touchState values are defined in guiCore.h
//-------------------------------------------------------//
void guiCore_ProcessTouchEvent(int16_t x, int16_t y, uint8_t touchState)
{
    guiEvent_t event;
    event.type = GUI_EVENT_TOUCH;
    event.spec = touchState;
    event.lparam = (uint16_t)x;
    event.hparam = (uint16_t)y;
#ifdef ALWAYS_PASS_TOUCH_TO_FOCUSED
    guiCore_AddMessageToQueue(focusedWidget, &event);
#else
    if ((focusedWidget != 0) && (focusedWidget->keepTouch))
        guiCore_AddMessageToQueue(focusedWidget, &event);
    else
        guiCore_AddMessageToQueue(rootWidget, &event);
#endif
    guiCore_ProcessMessageQueue();
}

//-------------------------------------------------------//
//  Top function for processing keys
//  default keys and specificators are defined in guiCore.h
//-------------------------------------------------------//
void guiCore_ProcessKeyEvent(uint16_t code, uint8_t spec)
{
    guiEvent_t event;
    event.type = GUI_EVENT_KEY;
    event.spec = spec;
    event.lparam = code;
    guiCore_AddMessageToQueue(focusedWidget, &event);
    guiCore_ProcessMessageQueue();
}

//-------------------------------------------------------//
//  Top function for processing encoder
//
//-------------------------------------------------------//
void guiCore_ProcessEncoderEvent(int16_t increment)
{
    guiEvent_t event;
    event.type = GUI_EVENT_ENCODER;
    event.spec = 0;
    event.lparam = (uint16_t)increment;
    guiCore_AddMessageToQueue(focusedWidget, &event);
    guiCore_ProcessMessageQueue();
}

//-------------------------------------------------------//
//  Top function for processing timers
//
//-------------------------------------------------------//
void guiCore_ProcessTimers(void)
{
    uint8_t i;
    for (i=0; i<GUI_TIMER_COUNT; i++)
    {
        guiCore_TimerProcess(i);
    }
    guiCore_ProcessMessageQueue();
}



//-------------------------------------------------------//
//  Sends event to all GUI elements for which validator
//  returns non-zero
//
//  guiCore_ProcessMessageQueue() should be called after
//  this function.
//  GUI message queue should be long enough - depending
//  on particular case
//-------------------------------------------------------//
void guiCore_BroadcastEvent(guiEvent_t event, uint8_t(*validator)(guiGenericWidget_t *widget))
{
    guiGenericWidget_t *widget;
    guiGenericWidget_t *nextWidget;
    uint8_t index;

    // Start widget tree traverse from root widget
    widget = rootWidget;

    while(1)
    {
        // Check if widget has children
        if (widget->isContainer)
        {
            if (((guiGenericContainer_t *)widget)->widgets.traverseIndex == 0)
            {
                // The first time visit
                if (validator(widget))
                {
                    widget->processEvent(widget, event);
                }
            }

            // If container has unprocessed children
            if ( ((guiGenericContainer_t *)widget)->widgets.traverseIndex <
                 ((guiGenericContainer_t *)widget)->widgets.count )
            {
                // switch to next one if possible
                index = ((guiGenericContainer_t *)widget)->widgets.traverseIndex++;
                nextWidget = ((guiGenericContainer_t *)widget)->widgets.elements[index];
                // check if widget actually exists
                if ((nextWidget == 0))
                    continue;
                widget = nextWidget;
            }
            else
            {
                // All container child items are processed. Reset counter of processed items and move up.
                ((guiGenericContainer_t *)widget)->widgets.traverseIndex = 0;
                if (widget->parent == 0)    // root widget has no parent
                    break;
                else
                    widget = widget->parent;
            }
        }
        else
        {
            // Widget has no children. Move up.
            if (validator(widget))
            {
                // The redrawRequired flag is reset by widget after processing DRAW event
                widget->processEvent(widget, event);
            }
            widget = widget->parent;
        }
    }
}

#ifdef CFG_USE_UPDATE
//-------------------------------------------------------//
//  Top function for GUI elements update
//
//-------------------------------------------------------//
void guiCore_UpdateAll(void)
{
    guiCore_BroadcastEvent(guiEvent_UPDATE, guiCore_UpdateValidator);
    guiCore_ProcessMessageQueue();
}


//-------------------------------------------------------//
//  Returns true if widget requires update
//
//-------------------------------------------------------//
uint8_t guiCore_UpdateValidator(guiGenericWidget_t *widget)
{
    if ((widget == 0) || (widget->updateRequired == 0))
        return 0;
    else
        return 1;
}
#endif




//===================================================================//
//===================================================================//
//                                                                   //
//                    Drawing and touch support                      //
//                       geometry functions                          //
//                                                                   //
//===================================================================//


//-------------------------------------------------------//
// Makes specified rectangle invalid - it must be redrawn
// Rectangle coordinates are relative to widget's.
// When calling this function, widget should set it's
// redraw flags itself.
//-------------------------------------------------------//
void guiCore_InvalidateRect(guiGenericWidget_t *widget, int16_t x1, int16_t y1, uint16_t x2, uint16_t y2)
{
    /*
        Approach 1:
            Add this rectangle to the list of invalidated rectangles of thew form.
            Redrawing will be split into two stages:
                a.  Traverse whole widget tree and find for each container if it intercepts with any of the rectangles.
                    If there is some interseption, mark the container to fully redraw it and it's content if rectangle is
                    not an exact widget.
                    For each widget in the container also check interseption with the rectangles and mark those
                    who has interception.
                b.  Add marked widgets and containers to redraw list. Sort list by Z-index and redraw every widget.
        Approach 2:
            Check if the rectangle lies on the parent widget completely. If so, mark parent to be redrawn and exit.
            If rectangle spands over the parent's borders, check the same for parent's parent and so on, until
            root widget is reached - i.e. propagate up the tree.

        If using Z-order, possibly put all form widgets into single array?
    */


    while (1)
    {
        if (widget->parent == 0)    // root widget has no parent
            break;

        // Convert rectangle into parent's coordinates
        x1 += widget->x;
        x2 += widget->x;
        y1 += widget->y;
        y2 += widget->y;

        // Move up the tree
        widget = widget->parent;

        // Make parent widget redraw
        widget->redrawRequired = 1;
        widget->redrawForced = 1;

        // Check if rectangle lies inside parent
        if ( (x1 >= 0) &&
             (y1 >= 0) &&
             (x2 < widget->width) &&
             (y2 < widget->height) )
        {
            break;
        }
    }
}



//-------------------------------------------------------//
//  Verifies if widget and rectangle are overlapped
//
//  Returns true if interseption is not null.
//-------------------------------------------------------//
uint8_t guiCore_CheckWidgetOvelap(guiGenericWidget_t *widget, rect16_t *rect)
{
    if ((rect->x2 - rect->x1 <= 0) || (rect->y2 - rect->y1 <= 0))
        return 0;
    if ( (rect->x2 < widget->x) ||
         (rect->y2 < widget->y) ||
         (rect->x1 > widget->x + widget->width - 1) ||
         (rect->y1 > widget->y + widget->height - 1) )
        return 0;
    else
        return 1;
}



//-------------------------------------------------------//
//  Converts relative (x,y) for a specified widget to
//    absolute values (absolute means relative to screen's (0,0))
//
//-------------------------------------------------------//
void guiCore_ConvertToAbsoluteXY(guiGenericWidget_t *widget, int16_t *x, int16_t *y)
{
    while(widget != 0)
    {
        // Convert XY into parent's coordinates
        *x += widget->x;
        *y += widget->y;

        // Move up the tree
        widget = widget->parent;
    }
}

//-------------------------------------------------------//
//  Converts absolute (x,y) to relative for a specified widget
//     values (absolute means relative to screen's (0,0))
//
//-------------------------------------------------------//
void guiCore_ConvertToRelativeXY(guiGenericWidget_t *widget, int16_t *x, int16_t *y)
{
    while(widget != 0)
    {
        // Convert XY into parent's coordinates
        *x -= widget->x;
        *y -= widget->y;

        // Move up the tree
        widget = widget->parent;
    }
}


//-------------------------------------------------------//
// Returns widget that has point (x;y) and can be touched -
//  either one of child widgets or widget itself.
// If widget is not visible, or not enabled, it is skipped.
// If no widget is found, 0 is returned
// X and Y parameters must be relative to container
//-------------------------------------------------------//
guiGenericWidget_t *guiCore_GetTouchedWidgetAtXY(guiGenericWidget_t *widget, int16_t x, int16_t y)
{
    guiGenericWidget_t *w;
    uint8_t i;

    // First check if point lies inside widget
    if ((x < 0) || (x >= widget->width))
        return 0;
    if ((y < 0) || (y >= widget->height))
        return 0;

    if (widget->isContainer)
    {
        // Point is inside container or one of it's widgets. Find out which one.
        i = ((guiGenericContainer_t *)widget)->widgets.count - 1;
        do
        {
            w = ((guiGenericContainer_t *)widget)->widgets.elements[i];
            if (w == 0)
                continue;
            if ((w->acceptTouch) && (w->isVisible))   // TODO - add isEnabled, etc
            {
                if ((x >= w->x) && (x < w->x + w->width) &&
                    (y >= w->y) && (y < w->y + w->height))
                {
                    return w;
                }
            }
        } while(i--);
    }
    // Not found - return widget itself
    return widget;
}







//===================================================================//
//===================================================================//
//                                                                   //
//                   Widget collections management                   //
//                                                                   //
//===================================================================//




//-------------------------------------------------------//
// Adds to queue focus message for newFocusedWidget
//
//-------------------------------------------------------//
void guiCore_RequestFocusChange(guiGenericWidget_t *newFocusedWidget)
{
    // Tell new widget to get focus
    if ((newFocusedWidget != focusedWidget) && (newFocusedWidget != 0))
    {
        guiCore_AddMessageToQueue(newFocusedWidget, &guiEvent_FOCUS);
    }
}

//-------------------------------------------------------//
// Sets GUI core focusedWidget pointer to widget
// This function should be called by a widget when it
// receives FOCUS message and agrees with it.
//-------------------------------------------------------//
void guiCore_AcceptFocus(guiGenericWidget_t *widget)
{
    uint8_t index;
    if ((widget != 0) && (widget != focusedWidget))     // CHECKME
    {
        // First tell currently focused widget to loose focus
        if (focusedWidget != 0)
        {
            guiCore_AddMessageToQueue(focusedWidget, &guiEvent_UNFOCUS);
        }
        focusedWidget = widget;
        if ((guiGenericContainer_t *)widget->parent != 0)
        {
            // Store index for container
            index = guiCore_GetWidgetIndex(focusedWidget);
            ((guiGenericContainer_t *)widget->parent)->widgets.focusedIndex = index;
        }
    }
}


//-------------------------------------------------------//
//  Pass focus to next widget in collection
//
//-------------------------------------------------------//
void guiCore_RequestFocusNextWidget(guiGenericContainer_t *container, int8_t tabDir)
{
    uint8_t currentTabIndex;
    uint8_t i;
    int16_t minTabIndex = 0x200;   // maximum x2
    int16_t tmp;
    uint8_t minWidgetIndex = container->widgets.count;
    guiGenericWidget_t *widget;

    currentTabIndex = 0;

    // Check if current widget belongs to specified container's collection
    if (focusedWidget)
    {
        if (focusedWidget->parent == (guiGenericWidget_t *)container)
            currentTabIndex = focusedWidget->tabIndex;
    }


    // Find widget with next tabIndex
    for (i = 0; i < container->widgets.count; i++)
    {
        widget = (guiGenericWidget_t *)container->widgets.elements[i];
        if (widget == 0)
            continue;
        if ((widget->acceptFocusByTab) && (widget->isVisible))
        {
            if (tabDir >= 0)
                tmp = (widget->tabIndex <= currentTabIndex) ? widget->tabIndex + 256 : widget->tabIndex;
            else
                tmp = (widget->tabIndex >= currentTabIndex) ? -(widget->tabIndex - 256) : -widget->tabIndex;

            if (tmp < minTabIndex)
            {
                minTabIndex = tmp;
                minWidgetIndex = i;
            }
        }
    }

    if (minWidgetIndex < container->widgets.count)
    {
        widget = container->widgets.elements[minWidgetIndex];
        //container->widgets.focusedIndex = minWidgetIndex;
        guiCore_RequestFocusChange(widget);
    }
}


//-------------------------------------------------------//
// Returns index of a widget in parent's collection
//
//-------------------------------------------------------//
uint8_t guiCore_GetWidgetIndex(guiGenericWidget_t *widget)
{
    uint8_t i;
    if ((widget == 0) || (widget->parent == 0))
        return 0;
    for (i = 0; i < ((guiGenericContainer_t *)widget->parent)->widgets.count; i++)
    {
        if (((guiGenericContainer_t *)widget->parent)->widgets.elements[i] == widget)
            return i;
    }
    return 0;   // error - widget is not present in parent's collection
}


//-------------------------------------------------------//
// Checks widget's tabindex in parent's collection.
// If current widget is the last in the collection that can be focused,
//      TABINDEX_IS_MAX is returned.
// If current widget is the first in the collection that can be focused,
//      TABINDEX_IS_MIN is returned.
// Else TABINDEX_IS_NORM is returned.
//-------------------------------------------------------//
uint8_t guiCore_CheckWidgetTabIndex(guiGenericWidget_t *widget)
{
    // TODO - add canBeFocused() function
    uint8_t i;
    uint8_t currTabIndex;
    uint8_t maxTabIndex;
    uint8_t minTabIndex;
    guiGenericWidget_t *w;
    if (widget == 0) return 0;

    currTabIndex = widget->tabIndex;
    maxTabIndex = currTabIndex;
    minTabIndex = currTabIndex;

    for (i = 0; i < ((guiGenericContainer_t *)widget->parent)->widgets.count; i++)
    {
        w = ((guiGenericContainer_t *)widget->parent)->widgets.elements[i];
        if (w == 0) continue;
        if ((w->acceptFocusByTab == 0) || (w->isVisible == 0)) continue;    // TODO - add isEnabled
        if (w->tabIndex > widget->tabIndex)
            maxTabIndex = w->tabIndex;
        else if (w->tabIndex < widget->tabIndex)
            minTabIndex = w->tabIndex;
    }

    if (currTabIndex == maxTabIndex)
        return TABINDEX_IS_MAX;
    if (currTabIndex == minTabIndex)
        return TABINDEX_IS_MIN;

    return TABINDEX_IS_NORM;
}





//===================================================================//
//===================================================================//
//                                                                   //
//                   General widget API fucntions                    //
//                                                                   //
//===================================================================//



//-------------------------------------------------------//
//  Shows or hides a widget.
//  This function should be called by widget as response for
//      received SHOW or HIDE message
//
// This function does not perform any widget state checks
//      except visible state.
// Returns 1 if new state was applied. Otherwise returns 0.
//-------------------------------------------------------//
uint8_t guiCore_SetVisible(guiGenericWidget_t *widget, uint8_t newVisibleState)
{
    guiEvent_t event;
    if (widget == 0) return 0;
    if (newVisibleState)
    {
        // Show widget
        if (widget->isVisible) return 0;
        widget->isVisible = 1;
        widget->redrawForced = 1;
    }
    else
    {
        // Hide widget
        if (widget->isVisible == 0) return 0;
        widget->isVisible = 0;
        guiCore_InvalidateRect(widget, widget->x, widget->y,
              widget->x + widget->width - 1, widget->y + widget->height - 1);
    }
    // Visible state changed - call handler
    if (widget->handlers.count != 0)
    {
        event.type = GUI_ON_VISIBLE_CHANGED;
        guiCore_CallEventHandler(widget, &event);
    }
    return 1;
}



//-------------------------------------------------------//
//  Sets or clears focus on widget.
//  This function should be called by widget as response for
//      received FOCUS or UNFOCUS message
//
// This function does not perform any widget state checks
//      except focused state.
// Returns 1 if new state was applied. Otherwise returns 0.
//-------------------------------------------------------//
uint8_t guiCore_SetFocused(guiGenericWidget_t *widget, uint8_t newFocusedState)
{
    guiEvent_t event;
    if (widget == 0) return 0;

    if (newFocusedState)
    {
        // Set focus on widget
        if (widget->isFocused) return 0;
        widget->isFocused = 1;
        guiCore_AcceptFocus(widget);
    }
    else
    {
        // Focus was removed
        if (widget->isFocused == 0) return 0;
        widget->isFocused = 0;
    }
    // Focused state changed - call handler
    //if (widget->showFocus)        // CHECKME
    //{
        widget->redrawFocus = 1;
        widget->redrawRequired = 1;
    //}
    // Focus state changed - call handler
    if (widget->handlers.count != 0)
    {
        event.type = GUI_ON_FOCUS_CHANGED;
        guiCore_CallEventHandler(widget, &event);
    }
    return 1;
}

//-------------------------------------------------------//
//  Shows or hides widgets in a collection
//
//  Affected widgets are sent message directly, bypassing queue.
//  guiCore_ProcessMessageQueue() should be called after this function
//  to process posted messages from callbacks or handlers
//
//  Widgets are affected using mode paramenter:
//      ITEMS_IN_RANGE_ARE_VISIBLE
//      ITEMS_IN_RANGE_ARE_INVISIBLE
//      ITEMS_OUT_OF_RANGE_ARE_VISIBLE
//      ITEMS_OUT_OF_RANGE_ARE_INVISIBLE
//  mode can be any combination of these constants
//-------------------------------------------------------//
void guiCore_SetVisibleByTag(guiWidgetCollection_t *collection, uint8_t minTag, uint8_t maxTag, uint8_t mode)
{
    uint8_t i;
    uint8_t tagInRange;
    guiGenericWidget_t *widget;
    for(i=0; i<collection->count; i++)
    {
        widget = (guiGenericWidget_t *)collection->elements[i];
        if (widget == 0)
            continue;
        tagInRange = ((widget->tag >= minTag) && (widget->tag <= maxTag)) ? mode & 0x3 : mode & 0xC;
        if ((tagInRange == ITEMS_IN_RANGE_ARE_VISIBLE) || (tagInRange == ITEMS_OUT_OF_RANGE_ARE_VISIBLE))
        {
            if (widget->isVisible == 0)
                widget->processEvent(widget, guiEvent_SHOW);
                //guiCore_AddMessageToQueue(widget, &guiEvent_SHOW);
        }
        else if ((tagInRange == ITEMS_IN_RANGE_ARE_INVISIBLE) || (tagInRange == ITEMS_OUT_OF_RANGE_ARE_INVISIBLE))
        {
            if (widget->isVisible)
                widget->processEvent(widget, guiEvent_HIDE);
                //guiCore_AddMessageToQueue(widget, &guiEvent_HIDE);
        }
    }
}


//-------------------------------------------------------//
//  Call widget's handler for an event
//
//  Function searches through the widget's handler table
//  and call handlers for matching event type
//-------------------------------------------------------//
uint8_t guiCore_CallEventHandler(guiGenericWidget_t *widget, guiEvent_t *event)
{
    uint8_t i;
    uint8_t handlerResult = GUI_EVENT_DECLINE;
    for(i=0; i<widget->handlers.count; i++)
    {
        if (widget->handlers.elements[i].eventType == event->type)
        {
            handlerResult = widget->handlers.elements[i].handler(widget, event);
        }
    }
    return handlerResult;
}



void guiCore_DecodeWidgetTouchEvent(guiGenericWidget_t *widget, guiEvent_t *touchEvent, widgetTouchState_t *decodedTouchState)
{
    // Convert coordinates to widget's relative
    decodedTouchState->x = (int16_t)touchEvent->lparam;
    decodedTouchState->y = (int16_t)touchEvent->hparam;
    guiCore_ConvertToRelativeXY(widget,&decodedTouchState->x, &decodedTouchState->y);
    decodedTouchState->state = touchEvent->spec;
    // Determine if touch point lies inside the widget
    decodedTouchState->isInsideWidget = (guiCore_GetTouchedWidgetAtXY(widget,decodedTouchState->x, decodedTouchState->y)) ? 1 : 0;
}


void guiCore_DecodeContainerTouchEvent(guiGenericWidget_t *widget, guiEvent_t *touchEvent, containerTouchState_t *decodedTouchState)
{
    // Convert coordinates to widget's relative
    decodedTouchState->x = (int16_t)touchEvent->lparam;
    decodedTouchState->y = (int16_t)touchEvent->hparam;
    guiCore_ConvertToRelativeXY(widget,&decodedTouchState->x, &decodedTouchState->y);
    decodedTouchState->state = touchEvent->spec;
    // Determine if touch point lies inside the widget
    decodedTouchState->widgetAtXY = guiCore_GetTouchedWidgetAtXY(widget,decodedTouchState->x, decodedTouchState->y);
}







