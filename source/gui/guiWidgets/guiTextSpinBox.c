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



uint8_t guiTextSpinBox_SetActive(guiTextSpinBox_t *textSpinBox, uint8_t newActiveState)
{
    guiEvent_t event;
    if (textSpinBox == 0) return 0;

    if (newActiveState)
    {
        // Activate
        if (textSpinBox->isActive) return 0;
        textSpinBox->isActive = 1;
    }
    else
    {
        // Deactivate
        if (textSpinBox->isActive == 0) return 0;
        textSpinBox->isActive = 0;
    }
    // Active state changed - call handler
    textSpinBox->redrawCharSelection = 1;
    textSpinBox->redrawRequired = 1;
    if (textSpinBox->handlers.count != 0)
    {
        event.type = TEXTSPINBOX_ACTIVE_CHANGED;
        guiCore_CallEventHandler((guiGenericWidget_t *)textSpinBox, &event);
    }
    return 1;
}



void guiTextSpinBox_SetActiveCharPos(guiTextSpinBox_t *textSpinBox, int8_t num)
{
    uint8_t newActiveChar;
    guiEvent_t event;
    if (textSpinBox != 0)
    {
        newActiveChar = (num >= textSpinBox->textLength) ? textSpinBox->textLength - 1 :
                                                             ((num < 0) ? 0 : num);
        if (newActiveChar != textSpinBox->activeChar)
        {
            textSpinBox->activeChar = newActiveChar;
            textSpinBox->redrawCharSelection = 1;
            textSpinBox->redrawRequired = 1;
            // Call handler
            event.type = TEXTSPINBOX_ACTIVE_CHAR_CHANGED;
            guiCore_CallEventHandler((guiGenericWidget_t *)textSpinBox, &event);
        }
    }
}


void guiTextSpinBox_SetText(guiTextSpinBox_t *textSpinBox, char *newText, uint8_t callHandler)
{
    uint8_t i, newTextIndex;
    char c;
    guiEvent_t event;
    if (textSpinBox != 0)
    {
        for (i=0, newTextIndex=0; i<textSpinBox->textLength; i++)
        {
            c = newText[newTextIndex];
            if (c != 0)
            {
                textSpinBox->text[i] = c;
                newTextIndex++;
            }
            else
            {
                textSpinBox->text[i] = ' ';
            }
        }

        textSpinBox->redrawText = 1;
        textSpinBox->redrawRequired = 1;

        // Call handler
        if (callHandler)
        {
            event.type = TEXTSPINBOX_TEXT_CHANGED;
            guiCore_CallEventHandler((guiGenericWidget_t *)textSpinBox, &event);
        }
    }
}

void guiTextSpinBox_IncrementActiveChar(guiTextSpinBox_t *textSpinBox, int8_t inc)
{
    guiEvent_t event;
    uint8_t c = textSpinBox->text[textSpinBox->activeChar];
    uint8_t newChar = LCD_GetNextFontChar(textSpinBox->font, c, inc);
    if (newChar != c)
    {
        textSpinBox->text[textSpinBox->activeChar] = newChar;
        textSpinBox->redrawText = 1;
        textSpinBox->redrawRequired = 1;
        event.type = TEXTSPINBOX_TEXT_CHANGED;
        guiCore_CallEventHandler((guiGenericWidget_t *)textSpinBox, &event);
    }
}

void guiTextSpinBox_SetActiveChar(guiTextSpinBox_t *textSpinBox, char newChar, uint8_t callHandler)
{
    guiEvent_t event;
    uint8_t c = textSpinBox->text[textSpinBox->activeChar];
    if (newChar != c)
    {
        textSpinBox->text[textSpinBox->activeChar] = newChar;
        textSpinBox->redrawText = 1;
        textSpinBox->redrawRequired = 1;
        // Call handler
        if (callHandler)
        {
            event.type = TEXTSPINBOX_TEXT_CHANGED;
            guiCore_CallEventHandler((guiGenericWidget_t *)textSpinBox, &event);
        }
    }
}




//-------------------------------------------------------//
// textSpinBox key handler
//
// Returns GUI_EVENT_ACCEPTED if key is processed,
//         GUI_EVENT_DECLINE otherwise
//-------------------------------------------------------//
uint8_t guiTextSpinBox_ProcessKey(guiTextSpinBox_t *textSpinBox, uint8_t key)
{
    if (textSpinBox->isActive)
    {
        if (key == TEXTSPINBOX_KEY_SELECT)
        {
            guiTextSpinBox_SetActive(textSpinBox, 0);
        }
        else if (key == TEXTSPINBOX_KEY_EXIT)
        {
            guiTextSpinBox_SetActive(textSpinBox, 0);
        }
        else if (key == TEXTSPINBOX_KEY_LEFT)
        {
            // move active digit left
            guiTextSpinBox_SetActiveCharPos(textSpinBox, textSpinBox->activeChar - 1);
        }
        else if (key == TEXTSPINBOX_KEY_RIGHT)
        {
            // move active digit right
            guiTextSpinBox_SetActiveCharPos(textSpinBox, textSpinBox->activeChar + 1);
        }
        else if (key == TEXTSPINBOX_KEY_UP)
        {
            guiTextSpinBox_IncrementActiveChar(textSpinBox, 1);
        }
        else if (key == TEXTSPINBOX_KEY_DOWN)
        {
            guiTextSpinBox_IncrementActiveChar(textSpinBox, -1);
        }
        else
        {
            return GUI_EVENT_DECLINE;
        }
    }
    else
    {
        if (key == TEXTSPINBOX_KEY_SELECT)
        {
            guiTextSpinBox_SetActive(textSpinBox, 1);
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
uint8_t guiTextSpinBox_DefaultKeyTranslator(guiGenericWidget_t *widget, guiEvent_t *event, void *translatedKey)
{
    guiTextSpinBoxTranslatedKey_t *tkey = (guiTextSpinBoxTranslatedKey_t *)translatedKey;
    tkey->key = 0;
    tkey->increment = 0;
    if (event->spec == GUI_KEY_EVENT_DOWN)
    {
        if (event->lparam == GUI_KEY_OK)
            tkey->key = TEXTSPINBOX_KEY_SELECT;
        else if (event->lparam == GUI_KEY_ESC)
            tkey->key = TEXTSPINBOX_KEY_EXIT;
        else if (event->lparam == GUI_KEY_LEFT)
            tkey->key = TEXTSPINBOX_KEY_LEFT;
        else if (event->lparam == GUI_KEY_RIGHT)
            tkey->key = TEXTSPINBOX_KEY_RIGHT;
        else if (event->lparam == GUI_KEY_UP)
            tkey->key = TEXTSPINBOX_KEY_UP;
        else if (event->lparam == GUI_KEY_DOWN)
            tkey->key = TEXTSPINBOX_KEY_DOWN;
    }
    else if (event->spec == GUI_ENCODER_EVENT)
    {
        tkey->increment = (int8_t)event->lparam;
    }
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

    switch (event.type)
    {
        case GUI_EVENT_DRAW:
            guiGraph_DrawTextSpinBox(textSpinBox);
            // Call handler
            guiCore_CallEventHandler(widget, &event);
            // Reset flags - redrawForced will be reset by core
            textSpinBox->redrawFocus = 0;
            textSpinBox->redrawText= 0;
            textSpinBox->redrawCharSelection = 0;
            textSpinBox->redrawRequired = 0;
            break;
        case GUI_EVENT_FOCUS:
            if (TEXTSPINBOX_ACCEPTS_FOCUS_EVENT(textSpinBox))
                guiCore_SetFocused((guiGenericWidget_t *)textSpinBox,1);
            else
                processResult = GUI_EVENT_DECLINE;      // Cannot accept focus
            break;
        case GUI_EVENT_UNFOCUS:
            guiCore_SetFocused((guiGenericWidget_t *)textSpinBox,0);
            guiTextSpinBox_SetActive(textSpinBox, 0);        // Do not restore
            break;
        case TEXTSPINBOX_EVENT_ACTIVATE:
            if (textSpinBox->isFocused)
            {
                guiTextSpinBox_SetActive(textSpinBox, 1);
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
            if (TEXTSPINBOX_ACCEPTS_KEY_EVENT(textSpinBox))
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
                        guiTextSpinBox_IncrementActiveChar(textSpinBox, tkey.increment);
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
    }
    return processResult;
}





//-------------------------------------------------------//
// Default initialization
//
//-------------------------------------------------------//
void guiTextSpinBox_Initialize(guiTextSpinBox_t *textSpinBox, guiGenericWidget_t *parent)
{
    memset(textSpinBox, 0, sizeof(*textSpinBox));
    textSpinBox->type = WT_TEXTSPINBOX;
    textSpinBox->parent = parent;
    textSpinBox->acceptFocusByTab = 1;
    textSpinBox->isVisible = 1;
    textSpinBox->showFocus = 1;
    textSpinBox->processEvent = guiTextSpinBox_ProcessEvent;
    textSpinBox->keyTranslator = guiTextSpinBox_DefaultKeyTranslator;
}


