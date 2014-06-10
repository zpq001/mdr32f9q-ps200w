/**********************************************************
    Module SelectTextBox




**********************************************************/

#include <stdint.h>         // using integer types
#include <string.h>         // using memset
#include "guiEvents.h"
#include "guiCore.h"
#include "guiWidgets.h"
#include "guiSelectTextBox.h"
#include "guiGraphWidgets.h"




void guiSelectTextBox_SetIndex(guiSelectTextBox_t *selectTextBox, int16_t index, uint8_t callHandler)
{
    uint8_t newIndex;
    guiEvent_t event;
    if (selectTextBox != 0)
    {
        if (index < 0)
            newIndex = 0;
        else if (index >= selectTextBox->stringCount)
            newIndex = selectTextBox->stringCount - 1;
        else
            newIndex = index;

        if (newIndex != selectTextBox->selectedIndex)
        {
            selectTextBox->selectedIndex = newIndex;
            selectTextBox->redrawRequired = 1;
            selectTextBox->redrawForced = 1;

            // Call handler
            if (callHandler)
            {
                event.type = SELECTTEXTBOX_INDEX_CHANGED;
                guiCore_CallEventHandler((guiGenericWidget_t *)selectTextBox, &event);
            }
        }
    }
}


uint8_t guiSelectTextBox_SetActive(guiSelectTextBox_t *selectTextBox, uint8_t newActiveState, uint8_t restoreIndex)
{
    guiEvent_t event;
    if (selectTextBox == 0) return 0;

    if (newActiveState)
    {
        // Activate
        if (selectTextBox->isActive) return 0;
        selectTextBox->isActive = 1;
        selectTextBox->savedIndex = selectTextBox->selectedIndex;
    }
    else
    {
        // Deactivate
        if (selectTextBox->isActive == 0) return 0;
        selectTextBox->isActive = 0;
        if ((selectTextBox->restoreIndexOnEscape) && (restoreIndex))
        {
            guiSelectTextBox_SetIndex(selectTextBox, selectTextBox->savedIndex, 1);
            selectTextBox->newIndexAccepted = 0;
        }
        else
        {
            selectTextBox->newIndexAccepted = 1;
        }
    }
    // Active state changed - call handler
    selectTextBox->redrawRequired = 1;
    selectTextBox->redrawForced = 1;
    if (selectTextBox->handlers.count != 0)
    {
        event.type = SELECTTEXTBOX_ACTIVE_CHANGED;
        guiCore_CallEventHandler((guiGenericWidget_t *)selectTextBox, &event);
    }
    return 1;
}


//-------------------------------------------------------//
// textSpinBox key handler
//
// Returns GUI_EVENT_ACCEPTED if key is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiSelectTextBox_ProcessKey(guiSelectTextBox_t *selectTextBox, uint8_t key)
{
    if (selectTextBox->isActive)
    {
        if (key == SELECTTEXTBOX_KEY_SELECT)
        {
            guiSelectTextBox_SetActive(selectTextBox, 0, 0);
        }
        else if (key == SELECTTEXTBOX_KEY_EXIT)
        {
            guiSelectTextBox_SetActive(selectTextBox, 0, 1);
        }
        else if (key == SELECTTEXTBOX_KEY_NEXT)
        {
            guiSelectTextBox_SetIndex(selectTextBox, (int16_t)selectTextBox->selectedIndex + 1, 1);
        }
        else if (key == SELECTTEXTBOX_KEY_PREV)
        {
            guiSelectTextBox_SetIndex(selectTextBox, (int16_t)selectTextBox->selectedIndex - 1, 1);
        }
        else
        {
            return GUI_EVENT_DECLINE;
        }
    }
    else
    {
        if (key == SELECTTEXTBOX_KEY_SELECT)
        {
            guiSelectTextBox_SetActive(selectTextBox, 1, 0);
        }
        else
        {
            return GUI_EVENT_DECLINE;
        }
    }
    return GUI_EVENT_ACCEPTED;
}



//-------------------------------------------------------//
// Default key event translator
//
//-------------------------------------------------------//
uint8_t guiSelectTextBox_DefaultKeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiSelectTextBoxTranslatedKey_t *tkey = (guiSelectTextBoxTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_OK)
            tkey->key = SELECTTEXTBOX_KEY_SELECT;
        else if (event->lparam == GUI_KEY_ESC)
            tkey->key = SELECTTEXTBOX_KEY_EXIT;
        else if (event->lparam == GUI_KEY_LEFT)
            tkey->key = SELECTTEXTBOX_KEY_PREV;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = SELECTTEXTBOX_KEY_NEXT;
        else if (event->lparam == GUI_KEY_UP)
            tkey->key = SELECTTEXTBOX_KEY_PREV;
        else if (event->lparam == GUI_KEY_DOWN)
            tkey->key = SELECTTEXTBOX_KEY_NEXT;
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        if ((int8_t)event->lparam < 0)
            tkey->key = SELECTTEXTBOX_KEY_PREV;
        else if ((int8_t)event->lparam > 0)
            tkey->key = SELECTTEXTBOX_KEY_NEXT;
    }
    return 0;
}




uint8_t guiSelectTextBox_ProcessEvent(guiGenericWidget_t *widget, guiEvent_t event)
{
    guiSelectTextBox_t *selectTextBox = (guiSelectTextBox_t *)widget;
    uint8_t processResult = GUI_EVENT_ACCEPTED;
    guiSelectTextBoxTranslatedKey_t tkey;

    switch (event.type)
    {
        case GUI_EVENT_DRAW:
            guiGraph_DrawSelectTextBox(selectTextBox);
            // Call handler
            guiCore_CallEventHandler(widget, &event);
            // Reset flags
            selectTextBox->redrawFocus = 0;
            selectTextBox->redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:
            if (SELECTTEXTBOX_ACCEPTS_FOCUS_EVENT(selectTextBox))
                guiCore_SetFocused((guiGenericWidget_t *)selectTextBox, 1);
            else
                processResult = GUI_EVENT_DECLINE;      // Cannot accept focus
            break;
        case GUI_EVENT_UNFOCUS:
            guiCore_SetFocused((guiGenericWidget_t *)selectTextBox, 0);
            guiSelectTextBox_SetActive(selectTextBox, 0, 1);    // Restore index
            break;
        case SELECTTEXTBOX_EVENT_ACTIVATE:
            if (selectTextBox->isFocused)
            {
                guiSelectTextBox_SetActive(selectTextBox, 1, 0);
            }
            // Accept event anyway
            break;
        case GUI_EVENT_SHOW:
            guiCore_SetVisible((guiGenericWidget_t *)selectTextBox, 1);
            break;
        case GUI_EVENT_HIDE:
            guiCore_SetVisible((guiGenericWidget_t *)selectTextBox, 0);
            break;
        case GUI_EVENT_KEY:
            processResult = GUI_EVENT_DECLINE;
            if (SELECTTEXTBOX_ACCEPTS_KEY_EVENT(selectTextBox))
            {
                if (selectTextBox->keyTranslator)
                {
                    processResult = selectTextBox->keyTranslator(widget, &event, &tkey);
                    if (tkey.key != 0)
                    {
                        processResult |= guiSelectTextBox_ProcessKey(selectTextBox, tkey.key);
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
    }
    return processResult;
}


//-------------------------------------------------------//
// Default initialization
//
//-------------------------------------------------------//
void guiSelectTextBox_Initialize(guiSelectTextBox_t *selectTextBox, guiGenericWidget_t *parent)
{
    memset(selectTextBox, 0, sizeof(*selectTextBox));
    selectTextBox->type = WT_SELECTTEXTBOX;
    selectTextBox->parent = parent;
    selectTextBox->acceptFocusByTab = 1;
    selectTextBox->isVisible = 1;
    selectTextBox->showFocus = 1;
    selectTextBox->processEvent = guiSelectTextBox_ProcessEvent;
    selectTextBox->keyTranslator = guiSelectTextBox_DefaultKeyTranslator;
}


