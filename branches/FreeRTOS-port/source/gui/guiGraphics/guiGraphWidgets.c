/**********************************************************
  Module guiGraphWidgets contains functions for drawing widgets. (hi, Cap!)



**********************************************************/

#include <stdint.h>
#include "guiGraphHAL.h"
#include "guiGraphPrimitives.h"
#include "guiGraphWidgets.h"
#include "guiFonts.h"
#include "guiImages.h"

#include "guiWidgets.h"
#include "guiTextLabel.h"
//#include "guiCheckBox.h"




int16_t wx;
int16_t wy;


//-------------------------------------------------------//
// Sets point (0,0) of coordinate system
// Parameters x and y must be absolute values
// Widget's geometry is relative to (wx,wy)
//-------------------------------------------------------//
void guiGraph_SetBaseXY(int16_t x, int16_t y)
{
    wx = x;
    wy = y;
}

//-------------------------------------------------------//
// Sets point (0,0) of coordinate system
//-------------------------------------------------------//
void guiGraph_OffsetBaseXY(int16_t dx, int16_t dy)
{
    wx += dx;
    wy += dy;
}


/*
        There are two sources of widget redraw requests:

        First one is the widget itself - if some widget internal state is changed, it sets "redrawRequired" flag,
    thus indicating the core it's need to redraw. Widget also sets some internal redraw flags, which are analyzed
    only in appropriate draw function.

        Second is the GUI core. When GUI core discovers that widget should be redrawn by parent request, or for
    other reason, the core sets both "redrawRequired" and "redrawForced" flags.

    The "redrawForced" flag is set and cleared by core itself. The "redrawRequired" can be set by
    both GUI core and widget, but it should be cleared by widget after completing draw procedure and
    executing callback function.

*/


//-------------------------------------------------------//
// Draw a panel
//
//
//-------------------------------------------------------//
void guiGraph_DrawPanel(guiPanel_t *panel)
{

    //-----------------------------------------//
    // Draw background
    if (panel->redrawForced)
    {
        // Erase rectangle
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_FillRect(wx,wy,panel->width,panel->height,FILL_WITH_WHITE);
    }


    //-----------------------------------------//
    // Draw focus / frame
    if (((panel->redrawForced) || (panel->redrawFocus))  &&
        (panel->showFocus))
    {
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        if (panel->isFocused)
        {
            LCD_SetLineStyle(LINE_STYLE_DOTTED);
            LCD_DrawRect(wx,wy,panel->width,panel->height,1);
        }
        else
        {
            LCD_SetLineStyle(LINE_STYLE_SOLID);
            if (panel->frame)
                LCD_DrawRect(wx,wy,panel->width,panel->height,1);
            else
                LCD_DrawRect(wx,wy,panel->width,panel->height,0);
        }
    }
}



//-------------------------------------------------------//
// Draw a textLabel
//
//
//-------------------------------------------------------//
void guiGraph_DrawTextLabel(guiTextLabel_t *textLabel)
{
    rect_t rect;

    //-----------------------------------------//
    // Draw background and text
    if (textLabel->redrawForced)
    {
        // Erase rectangle
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_FillRect(wx,wy,textLabel->width-1,textLabel->height-1,FILL_WITH_WHITE);

        // Draw string
        if (textLabel->text)
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_OR);
            LCD_SetFont(textLabel->font);
            rect.x1 = wx + 0 + TEXT_LABEL_TEXT_MARGIN;
            rect.y1 = wy + 0;
            rect.x2 = wx + textLabel->width - 1 - TEXT_LABEL_TEXT_MARGIN;
            rect.y2 = wy + textLabel->height - 1;
            LCD_PrintStringAligned(textLabel->text, &rect, textLabel->textAlignment, IMAGE_MODE_NORMAL);
        }
    }

    //-----------------------------------------//
    // Draw focus
    if (((textLabel->redrawForced) || (textLabel->redrawFocus)) &&
        (textLabel->showFocus))
    {
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        if (textLabel->isFocused)
        {
            LCD_SetLineStyle(LINE_STYLE_DOTTED);
            LCD_DrawRect(wx,wy,textLabel->width,textLabel->height,1);
        }
        else
        {
            LCD_SetLineStyle(LINE_STYLE_SOLID);
            if (textLabel->hasFrame)
                LCD_DrawRect(wx,wy,textLabel->width,textLabel->height,1);
            else
                LCD_DrawRect(wx,wy,textLabel->width,textLabel->height,0);
        }
    }
}




//-------------------------------------------------------//
// Draw a selectTextBox
//
//
//-------------------------------------------------------//
void guiGraph_DrawSelectTextBox(guiSelectTextBox_t *selectTextBox)
{
    rect_t rect;

    //-----------------------------------------//
    // Draw background and text
    if (selectTextBox->redrawForced)
    {
        // Erase rectangle
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_FillRect(wx,wy,selectTextBox->width,selectTextBox->height,FILL_WITH_WHITE);

        // Draw string
        LCD_SetPixelOutputMode(PIXEL_MODE_OR);
        LCD_SetFont(selectTextBox->font);
        rect.x1 = wx + 0;
        rect.y1 = wy + 0;
        rect.x2 = wx + selectTextBox->width - 1;
        rect.y2 = wy + selectTextBox->height - 1;
        if (selectTextBox->selectedIndex < selectTextBox->stringCount)
        {
            if (selectTextBox->stringList[selectTextBox->selectedIndex])
            {
                LCD_PrintStringAligned(selectTextBox->stringList[selectTextBox->selectedIndex], &rect, ALIGN_CENTER, IMAGE_MODE_NORMAL);
            }

            // Draw active state
            if (selectTextBox->isActive)
            {
                if (selectTextBox->selectedIndex > 0)
                    LCD_DrawImage(selector_tri, wx, wy, 6, 12, IMAGE_MODE_NORMAL);
                if (selectTextBox->selectedIndex < selectTextBox->stringCount - 1)
                    LCD_DrawImage(selector_tri_xrev, wx + selectTextBox->width - 6, wy, 6, 12, IMAGE_MODE_NORMAL);
            }
        }
        else
        {
            LCD_PrintStringAligned("?", &rect, ALIGN_CENTER, IMAGE_MODE_NORMAL);
        }

    }


    //-----------------------------------------//
    // Draw focus
    if (((selectTextBox->redrawForced) || (selectTextBox->redrawFocus)) &&
            (selectTextBox->showFocus) && (selectTextBox->isActive == 0))
    {
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        if (selectTextBox->isFocused)
        {
            LCD_SetLineStyle(LINE_STYLE_DOTTED);
            LCD_DrawRect(wx,wy,selectTextBox->width,selectTextBox->height,1);
        }
        else
        {
            LCD_SetLineStyle(LINE_STYLE_SOLID);
            if (selectTextBox->hasFrame)
                LCD_DrawRect(wx,wy,selectTextBox->width,selectTextBox->height,1);
            else
                LCD_DrawRect(wx,wy,selectTextBox->width,selectTextBox->height,0);
        }
    }
}



//-------------------------------------------------------//
// Draw checkbox
//
//
//-------------------------------------------------------//
void guiGraph_DrawCheckBox(guiCheckBox_t * checkBox)
{
    int8_t y_aligned;
    rect_t rect;
    uint8_t *img;

    y_aligned = checkBox->height - CHECKBOX_GRAPH_YSIZE;
    y_aligned /= 2;
    y_aligned = wy + y_aligned;

    //-----------------------------------------//
    // Draw background
    if (checkBox->redrawForced)
    {
        // Erase rectangle
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_FillRect(wx+1,wy+1,checkBox->width-2,checkBox->height-2,FILL_WITH_WHITE);

        // Draw string
        if (checkBox->text)
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_OR);
            LCD_SetFont(checkBox->font);
            rect.x1 = wx + 2 + CHECKBOX_TEXT_MARGIN + CHECKBOX_GRAPH_XSIZE;
            rect.y1 = wy + 1;
            rect.x2 = wx + checkBox->width - 2;
            rect.y2 = wy + checkBox->height - 2;
            LCD_PrintStringAligned(checkBox->text, &rect, checkBox->textAlignment, IMAGE_MODE_NORMAL);
        }

        // Draw rectangle frame
        //LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        //LCD_DrawRect(wx + 2,y_aligned,CHECKBOX_GRAPH_XSIZE,CHECKBOX_GRAPH_YSIZE,1);
    }


    //-----------------------------------------//
    // Draw focus / frame
    if ((checkBox->redrawForced) || (checkBox->redrawFocus))
    {
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        if (checkBox->isFocused)
        {
            LCD_SetLineStyle(LINE_STYLE_DOTTED);
            LCD_DrawRect(wx,wy,checkBox->width,checkBox->height,1);
        }
        else
        {
            LCD_SetLineStyle(LINE_STYLE_SOLID);
            if (checkBox->hasFrame)
                LCD_DrawRect(wx,wy,checkBox->width,checkBox->height,1);
            else
                LCD_DrawRect(wx,wy,checkBox->width,checkBox->height,0);
        }
    }


    //-----------------------------------------//
    // Draw focus / frame
    if ((checkBox->redrawForced) || (checkBox->redrawCheckedState))
    {
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        img = (checkBox->isChecked) ? CHECKBOX_IMG_CHECKED :
                                      CHECKBOX_IMG_EMPTY;
        LCD_DrawImage(img,wx+2,y_aligned,CHECKBOX_GRAPH_XSIZE, CHECKBOX_GRAPH_YSIZE,IMAGE_MODE_NORMAL);
    }

}

//-------------------------------------------------------//
// Draw radioButton
//
//
//-------------------------------------------------------//
void guiGraph_DrawRadioButton(guiRadioButton_t *button)
{
    int8_t y_aligned;
    int8_t x_aligned;
    rect_t rect1;

    rect1.x1 = wx;
    rect1.x2 = wx + button->width - 1;
    rect1.y1 = wy;
    rect1.y2 = wy + button->height - 1;


    y_aligned = wy + button->height / 2;
    x_aligned = wx + RADIOBUTTON_RADIUS + 2;


    if (button->redrawForced)
    {
        // Erase rectangle
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_FillRect(wx+1,wy+1,button->width-2,button->height-2,FILL_WITH_WHITE);


        // Draw string
        if (button->text)
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
            LCD_SetFont(button->font);
            rect1.x1 = wx + 2 + RADIOBUTTON_RADIUS*2 + RADIOBUTTON_TEXT_MARGIN;
            rect1.y1 = wy + 1;
            rect1.x2 = wx + button->width - 2;
            rect1.y2 = wy + button->height - 2;
            LCD_PrintStringAligned(button->text, &rect1, button->textAlignment, IMAGE_MODE_NORMAL);
        }
    }

    //-----------------------------------------//
    // Draw radiobutton circles
    if ((button->redrawForced) || (button->redrawCheckedState))
    {
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_DrawFilledCircle(x_aligned,y_aligned,RADIOBUTTON_RADIUS, 0);
        LCD_DrawCircle(x_aligned,y_aligned,RADIOBUTTON_RADIUS, 1);
        if (button->isChecked)
        {
            LCD_DrawFilledCircle(x_aligned,y_aligned,RADIOBUTTON_CHECK_RADIUS, 1);
        }
    }



    //-----------------------------------------//
    // Draw focus / frame
    if ((button->redrawForced) || (button->redrawFocus))
    {
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        if (button->isFocused)
        {
          LCD_SetLineStyle(LINE_STYLE_DOTTED);
          LCD_DrawRect(wx,wy,button->width,button->height,1);
        }
        else
        {
          LCD_SetLineStyle(LINE_STYLE_SOLID);
          LCD_DrawRect(wx,wy,button->width,button->height,0);
        }
    }

}



//-------------------------------------------------------//
// Draw spinBox
//
//
//-------------------------------------------------------//
void guiGraph_DrawSpinBox(guiSpinBox_t * spinBox)
{
    uint8_t frameStyle;
    uint8_t framePixelValue;
    uint8_t charIndex;
    uint8_t charWidth;
    uint16_t charOffset;
    int8_t i;
    int16_t x,y, y_underline;
    uint8_t numDigits;
    char c;

    LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);

    //-----------------------------------------//
    // Draw background
    if ((spinBox->redrawForced) || (spinBox->redrawValue) || (spinBox->redrawDigitSelection))
    {
        // Erase rectangle
        LCD_FillRect(wx+1,wy+1,spinBox->width-2,spinBox->height-2,FILL_WITH_WHITE);
    }


    //-----------------------------------------//
    // Draw text value
    if ((spinBox->redrawForced) || (spinBox->redrawValue) || (spinBox->redrawDigitSelection))
    {
        x = wx + spinBox->width - 1 + spinBox->textRightOffset;
        y = wy + spinBox->textTopOffset;

        i = 0;
        charIndex = 0;
        // Add one extra digit for minus sign
        numDigits = (spinBox->value >= 0) ? spinBox->digitsToDisplay : spinBox->digitsToDisplay + 1;

        while(charIndex < numDigits)
        {
            c = (i == spinBox->dotPosition) ? '.' : spinBox->text[SPINBOX_STRING_LENGTH - 1 - charIndex++];
            if (LCD_GetFontItem(spinBox->font, c, &charWidth, &charOffset) == 0)
                continue;
            x -= charWidth;

            LCD_DrawImage(&spinBox->font->data[charOffset],x,y,charWidth,spinBox->font->height,IMAGE_MODE_NORMAL);
            if ((charIndex == spinBox->activeDigit+1) && (i != spinBox->dotPosition) && (spinBox->isActive))
            {
                LCD_SetLineStyle(LINE_STYLE_SOLID);
                for (y_underline = y + spinBox->font->height + SPINBOX_ACTIVE_UNDERLINE_MARGIN;
                     y_underline < y + spinBox->font->height + SPINBOX_ACTIVE_UNDERLINE_MARGIN + SPINBOX_ACTIVE_UNDERLINE_WIDTH;
                     y_underline++)
                    LCD_DrawHorLine(x,y_underline,charWidth,1);
            }
          /*  if ((charIndex == spinBox->activeDigit+1) && (i != spinBox->dotPosition) && (spinBox->isActive))
                LCD_DrawImage(&spinBox->font->data[charOffset],x,y,charWidth,spinBox->font->height,IMAGE_MODE_INVERSE);
            else
                LCD_DrawImage(&spinBox->font->data[charOffset],x,y,charWidth,spinBox->font->height,IMAGE_MODE_NORMAL);
                */
            i++;
        }
    }

    //-----------------------------------------//
    // Draw focus / frame
    if ((spinBox->redrawForced) || (spinBox->redrawFocus))
    {
        if ((spinBox->hasFrame) || (spinBox->showFocus))
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
            frameStyle = ((spinBox->showFocus) && (spinBox->isFocused)) ? LINE_STYLE_DOTTED : LINE_STYLE_SOLID;
            framePixelValue = ((spinBox->showFocus) && (spinBox->isFocused)) ? 1 : 0;
            framePixelValue |= (spinBox->hasFrame) ? 1 : 0;
            LCD_SetLineStyle(frameStyle);
            if (!((spinBox->redrawForced) && (framePixelValue == 0)))
                LCD_DrawRect(wx,wy,spinBox->width,spinBox->height,framePixelValue);
        }
    }
}


uint8_t guiGraph_GetStringListVisibleItemCount(guiStringList_t * list)
{
    uint8_t count;
    count = (list->height - STRINGLIST_V_FRAME_MARGIN * 1) / (list->font->height + STRINGLIST_INTERVAL);
    if (count > list->stringCount)
        count = list->stringCount;
    return count;
}



//-------------------------------------------------------//
// Draw textSpinBox
//
//
//-------------------------------------------------------//
void guiGraph_DrawTextSpinBox(guiTextSpinBox_t * textSpinBox)
{
    uint8_t frameStyle;
    uint8_t framePixelValue;
    uint8_t charWidth;
    uint16_t charOffset;
    int8_t i;
    int16_t x,y, y_underline;
    char c;

    LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);

    //-----------------------------------------//
    // Draw background
    if ((textSpinBox->redrawForced) || (textSpinBox->redrawText) || (textSpinBox->redrawCharSelection))
    {
        // Erase rectangle
        LCD_FillRect(wx+1,wy+1,textSpinBox->width-2,textSpinBox->height-2,FILL_WITH_WHITE);
    }


    //-----------------------------------------//
    // Draw text value
    if ((textSpinBox->redrawForced) || (textSpinBox->redrawText) || (textSpinBox->redrawCharSelection))
    {
        x = wx + textSpinBox->textLeftOffset;
        y = wy + textSpinBox->textTopOffset;
        i = 0;

        while(i < textSpinBox->textLength)
        {
            c = textSpinBox->text[i];
            if (LCD_GetFontItem(textSpinBox->font, c, &charWidth, &charOffset))
            {
                LCD_DrawImage(&textSpinBox->font->data[charOffset], x, y, charWidth, textSpinBox->font->height, IMAGE_MODE_NORMAL);
                // Draw underline mark
                if ((i == textSpinBox->activeChar) && (textSpinBox->isActive))
                {
                    LCD_SetLineStyle(LINE_STYLE_SOLID);
                    for (y_underline = y + textSpinBox->font->height + TEXTSPINBOX_UNDERLINE_MARGIN;
                         y_underline < y + textSpinBox->font->height + TEXTSPINBOX_UNDERLINE_MARGIN + TEXTSPINBOX_UNDERLINE_WIDTH;
                         y_underline++)
                        LCD_DrawHorLine(x,y_underline,charWidth,1);
                }
                x += charWidth + textSpinBox->font->spacing;
            }
            i++;
        }
    }

    //-----------------------------------------//
    // Draw focus / frame
    if ((textSpinBox->redrawForced) || (textSpinBox->redrawFocus))
    {
        if ((textSpinBox->hasFrame) || (textSpinBox->showFocus))
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
            frameStyle = ((textSpinBox->showFocus) && (textSpinBox->isFocused)) ? LINE_STYLE_DOTTED : LINE_STYLE_SOLID;
            framePixelValue = ((textSpinBox->showFocus) && (textSpinBox->isFocused)) ? 1 : 0;
            framePixelValue |= (textSpinBox->hasFrame) ? 1 : 0;
            LCD_SetLineStyle(frameStyle);
            if (!((textSpinBox->redrawForced) && (framePixelValue == 0)))
                LCD_DrawRect(wx,wy,textSpinBox->width,textSpinBox->height,framePixelValue);
        }
    }
}



//-------------------------------------------------------//
// Draw stringList
//
//
//-------------------------------------------------------//
void guiGraph_DrawStringList(guiStringList_t * list)
{
    uint8_t frameStyle;
    uint8_t framePixelValue;
    uint8_t elementHeight;
    rect_t rect;
    uint8_t index;
    uint8_t itemsToDisplay;

    LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);

    //-----------------------------------------//
    // Draw background
    if ((list->redrawForced) || (0))
    {
        // Erase rectangle
        LCD_FillRect(wx+1,wy+1,list->width-2,list->height-2,FILL_WITH_WHITE);
    }

    //-----------------------------------------//
    // Draw strings
    if (list->redrawForced)
    {
        elementHeight = list->font->height + STRINGLIST_INTERVAL;
        LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
        LCD_SetFont(list->font);

        index = list->firstIndexToDisplay;
        itemsToDisplay = guiGraph_GetStringListVisibleItemCount(list);

        rect.x1 = wx + STRINGLIST_H_FRAME_MARGIN;
        rect.y1 = wy + STRINGLIST_V_FRAME_MARGIN;
        rect.x2 = wx + list->width - STRINGLIST_H_FRAME_MARGIN - 1;
        rect.y2 = wy + elementHeight;

        while (itemsToDisplay--)
        {
            rect.x1 += STRINGLIST_H_TEXT_MARGIN;
            if (list->strings[index])
                LCD_PrintStringAligned(list->strings[index], &rect, list->textAlignment, IMAGE_MODE_NORMAL);
            rect.x1 -= STRINGLIST_H_TEXT_MARGIN;

            if (index == list->selectedIndex)
            {
                // Draw selection
                LCD_SetPixelOutputMode(PIXEL_MODE_XOR);
                LCD_FillRect(rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1,FILL_WITH_BLACK);
                LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
                if (list->isActive && list->showStringFocus)
                {
                    LCD_SetLineStyle(LINE_STYLE_DOTTED);
                    LCD_DrawRect(rect.x1, rect.y1, rect.x2 - rect.x1 + 1, rect.y2 - rect.y1 + 1, 1);
                }
            }
            index = (index >= list->stringCount - 1) ? 0 : index + 1;
            rect.y1 += elementHeight;
            rect.y2 += elementHeight;
        }
    }



    //-----------------------------------------//
    // Draw focus / frame
    if ((list->redrawForced) || (list->redrawFocus))
    {
        if ((list->hasFrame) || (list->showFocus))
        {
            LCD_SetPixelOutputMode(PIXEL_MODE_REWRITE);
            frameStyle = ((list->showFocus) && (list->isFocused) && (list->isActive == 0)) ? LINE_STYLE_DOTTED : LINE_STYLE_SOLID;
            framePixelValue = ((list->showFocus) && (list->isFocused) && (list->isActive == 0)) ? 1 : 0;
            framePixelValue |= (list->hasFrame) ? 1 : 0;
            LCD_SetLineStyle(frameStyle);
            if (!((list->redrawForced) && (framePixelValue == 0)))
                LCD_DrawRect(wx,wy,list->width,list->height,framePixelValue);
        }
    }
}

