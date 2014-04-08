/**********************************************************
  Module guiGraphPrimitives contains low-level LCD routines.

  LCD functions description:
    - Pixel output mode specifies how data in the buffer is updated.
        Can be PIXEL_MODE_REWRITE - old pixel value is rewrited with new one.
               PIXEL_MODE_AND - old pixel value is ANDed with new value
               PIXEL_MODE_OR - old pixel value is ORed with new value
               PIXEL_MODE_XOR - old pixel value is XORed with new value
        Every function that modiifes LCD buffer data uses these modes.
    - pixelValue is an argument
    - LCD_currentFont specifies font that is used for text
      Can be set directly
    - LCD_lineStyle specifies how lines are drawed

**********************************************************/

#include <stdint.h>
#include "guiGraphPrimitives.h"
#include "guiGraphHAL.h"
#include "guiFonts.h"


const tFont* LCD_currentFont;


//-------------------------------------------------------//
// Sets current font for text printing
//
//-------------------------------------------------------//
void LCD_SetFont(const tFont *newFont)
{
    if (newFont != LCD_currentFont)
        LCD_currentFont = newFont;
}


//-------------------------------------------------------//
// Draws a rectangle using LCD_lineStyle
//	Parameters:
//		uint8_t x_pos	- pixel x coordinate
//		uint8_t y_pos	- pixel y coordinate
//      uint8_t width
//      uint8_t height
//      pixelValue  	- new pixel value, 1 or 0
// Pixel output mode is set by calling LCD_setPixelOutputMode()
//-------------------------------------------------------//
void LCD_DrawRect(uint8_t x_pos, uint8_t y_pos, uint8_t width, uint8_t height, uint8_t pixelValue)
{
    LCD_DrawHorLine(x_pos,y_pos,width,pixelValue);
    LCD_DrawHorLine(x_pos,y_pos + height - 1,width,pixelValue);
    LCD_DrawVertLine(x_pos,y_pos,height - 1,pixelValue);
    LCD_DrawVertLine(x_pos + width - 1,y_pos, height - 1,pixelValue);
}


//-------------------------------------------------------//
// Rerurns index in code table for code
// Returns -1 if not found
//-------------------------------------------------------//
int16_t LCD_GetFontIndexForChar(const tFont *font, uint8_t code)
{
    uint8_t itemCode;
    uint8_t start_index = 0;
    uint8_t end_index;
    uint8_t i;

    end_index = font->charCount;
    while (start_index < end_index)
    {
        i = start_index + (end_index - start_index) / 2;
        itemCode = font->codeTable[i];
        if (code < itemCode)
            end_index = i;
        else if (code > itemCode)
            start_index = i+1;
        else
        {
            // Found
            return i;
        }
    }
    return -1;
}


//-------------------------------------------------------//
// Rerurns width and offset of a font item
// Font array MUST be sorted by code.
// If no item with such code is found, 0 is returned.
// Using binary search, http://kvodo.ru/dvoichnyiy-poisk.html
//-------------------------------------------------------//
uint8_t LCD_GetFontItem(const tFont *font, uint8_t code, uint8_t *width, uint16_t *offset)
{
    uint8_t itemCode;
    uint8_t start_index = 0;
    uint8_t end_index;
    uint8_t i;

    if (font->codeTable == 0)
    {
        // Font char set is a contiguous array
        start_index = font->firstCharCode;
        end_index = start_index + (font->charCount - 1);
        if ((code < start_index) || (code > end_index))
            return 0;
        else
        {
            i = code-start_index;
            if (offset != 0)
            {
                if (font->offsetTable == 0)
                    *offset = (uint16_t)i * font->bytesPerChar;
                else
                    *offset = font->offsetTable[i];
            }
            if (font->widthTable == 0)
                *width = font->width;
            else
                *width = font->widthTable[i];
            return 1;
        }
    }
    else
    {
        end_index = font->charCount;
        // Font char set is defined by charTable
        while (start_index < end_index)
        {
            i = start_index + (end_index - start_index) / 2;
            itemCode = font->codeTable[i];
            if (code < itemCode)
                end_index = i;
            else if (code > itemCode)
                start_index = i+1;
            else
            {
                // Found
                if (offset != 0)
                    // Font must have valid offsetTable when codeTable is used
                    *offset = font->offsetTable[i];
                if (font->widthTable == 0)
                    *width = font->width;
                else
                    *width = font->widthTable[i];
                return 1;
            }
        }
    }
    return 0;
}

//-------------------------------------------------------//
// Rerurns next or previous char in the font
//
//-------------------------------------------------------//
uint8_t LCD_GetNextFontChar(const tFont *font, uint8_t code, int8_t scanDir)
{
    int16_t index;
    //scanDir = (scanDir < 0) ? -1 : ((scanDir > 0) ? 1 : 0);

    if (font->codeTable == 0)
    {
        // Char subset is contiguous, starting with _firstCharCode_ and containing _charCount_ chars.
        index = code + scanDir;
        if (index <= font->firstCharCode)
            code = font->firstCharCode;
        else if (index >= font->firstCharCode + font->charCount - 1)
            code = font->firstCharCode + font->charCount - 1;
        else
            code = (uint8_t)index;
    }
    else
    {
        // Char subset is defined by _codeTable_ table
        index = LCD_GetFontIndexForChar(font, code);
        index += scanDir;
        if (index < 0)
            index = 0;
        else if (index > font->charCount - 1)
            index = font->charCount - 1;
        code = font->codeTable[index];
    }
    return code;
}


//-------------------------------------------------------//
// Rerurns length of a string in pixels
//
//-------------------------------------------------------//
uint8_t LCD_GetStringWidth(const tFont *font, char *string)
{
    uint8_t length = 0;
    uint8_t index = 0;
    uint8_t charWidth;
    char c;

    while((c = string[index++]))
    {
        if (LCD_GetFontItem(LCD_currentFont, c, &charWidth, 0))
            length += charWidth + font->spacing;
    }

    length -= font->spacing;
    return length;
}


//-------------------------------------------------------//
// Prints a string with LCD_currentFont at current position
// mode:
//     IMAGE_MODE_NORMAL - normal images
//     IMAGE_MODE_INVERSE - inversed images
//-------------------------------------------------------//
void LCD_PrintString(char *str, uint8_t x, uint8_t y, uint8_t mode)
{
    uint8_t index = 0;
    uint8_t charWidth;
    uint16_t charOffset;
    char c;

    while((c = str[index++]))
    {

        if (LCD_GetFontItem(LCD_currentFont, c, &charWidth, &charOffset))
        {
            LCD_DrawImage(&LCD_currentFont->data[charOffset], x, y, charWidth, LCD_currentFont->height, mode);
            x += charWidth + LCD_currentFont->spacing;
        }
    }
}



//-------------------------------------------------------//
// Prints a string with LCD_currentFont inside rectangle using
//  alignment
// mode:
//     IMAGE_MODE_NORMAL - normal images
//     IMAGE_MODE_INVERSE - inversed images
//-------------------------------------------------------//
void LCD_PrintStringAligned(char *str, rect_t *rect, uint8_t alignment, uint8_t mode)
{
    uint8_t index = 0;
    uint8_t charWidth;
    uint16_t charOffset;
    char c;
    int16_t x_aligned, y_aligned;
    int16_t strWidthPx;

    // Find horizontal position
    if (alignment & ALIGN_LEFT)
    {
        x_aligned = rect->x1;       // pretty simple - take left rect border as starting point
    }
    else
    {
        // We need to compute length of whole string in pixels
        strWidthPx = LCD_GetStringWidth(LCD_currentFont,str);
        if (alignment & ALIGN_RIGHT)
            x_aligned = (int16_t)rect->x2 + 1 - strWidthPx;
        else
            x_aligned = rect->x1 + ((int16_t)(rect->x2 - rect->x1 + 1) - strWidthPx) / 2;
    }

    // Find vertical position
    if (alignment & ALIGN_TOP)
    {
        y_aligned = rect->y1;
    }
    else if (alignment & ALIGN_BOTTOM)
    {
        y_aligned = (int16_t)rect->y2 + 1 - LCD_currentFont->height;
    }
    else
    {
        y_aligned = rect->y1 + ((int16_t)(rect->y2 - rect->y1 + 1) - LCD_currentFont->height) / 2;
    }

    // Now print string
    while((c = str[index++]))
    {
        if (LCD_GetFontItem(LCD_currentFont, c, &charWidth, &charOffset))
        {
            LCD_DrawImage(&LCD_currentFont->data[charOffset], x_aligned, y_aligned, charWidth, LCD_currentFont->height, mode);
            x_aligned += charWidth + LCD_currentFont->spacing;
        }
    }
}


void LCD_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t mode)
{
    int16_t dy = 0;
    int16_t dx = 0;
    int16_t stepx = 0;
    int16_t stepy = 0;
    int16_t fraction = 0;

//       if (x1>LCD_X_RES || x2>LCD_X_RES || y1>LCD_Y_RES || y2>LCD_Y_RES) return;

    dy = y2 - y1;
    dx = x2 - x1;
    if (dy < 0)
    {
        dy = -dy;
        stepy = -1;
    }
    else stepy = 1;
    if (dx < 0)
    {
        dx = -dx;
        stepx = -1;
    }
    else stepx = 1;
    dy <<= 1;
    dx <<= 1;
    LCD_PutPixel(x1,y1,mode);
    if (dx > dy)
    {
        fraction = dy - (dx >> 1);
        while (x1 != x2)
        {
            if (fraction >= 0)
            {
                y1 += stepy;
                fraction -= dx;
            }
            x1 += stepx;
            fraction += dy;
            LCD_PutPixel(x1,y1,mode);
        }
    }
    else
    {
        fraction = dx - (dy >> 1);
        while (y1 != y2)
        {
            if (fraction >= 0)
            {
                x1 += stepx;
                fraction -= dy;
            }
            y1 += stepy;
            fraction += dx;
            LCD_PutPixel(x1,y1,mode);
        }
    }
}



void LCD_DrawCircle(int16_t x0, int16_t y0, int16_t radius, uint8_t mode)
{
    int16_t x = radius;
    int16_t y = 0;
    int16_t xChange = 1 - (radius << 1);
    int16_t yChange = 0;
    int16_t radiusError = 0;

    while (x >= y)
    {
        LCD_PutPixel(x0 - x, y0 + y, mode);
        LCD_PutPixel(x0 + x, y0 + y, mode);
        LCD_PutPixel(x0 - x, y0 - y, mode);
        LCD_PutPixel(x0 + x, y0 - y, mode);

        LCD_PutPixel(x0 - y, y0 + x, mode);
        LCD_PutPixel(x0 + y, y0 + x, mode);
        LCD_PutPixel(x0 - y, y0 - x, mode);
        LCD_PutPixel(x0 + y, y0 - x, mode);

        y++;
        radiusError += yChange;
        yChange += 2;
        if (((radiusError << 1) + xChange) > 0)
        {
            x--;
            radiusError += xChange;
            xChange += 2;
        }
    }
}




void LCD_DrawFilledCircle(int16_t x0, int16_t y0, int16_t radius, uint8_t mode)
{
    int16_t x = radius;
    int16_t y = 0;
    int16_t xChange = 1 - (radius << 1);
    int16_t yChange = 0;
    int16_t radiusError = 0;
    int16_t i;

    while (x >= y)
    {
        for (i = x0 - x; i <= x0 + x; i++)
        {
            LCD_PutPixel(i, y0 + y, mode);
            LCD_PutPixel(i, y0 - y, mode);
        }
        for (i = x0 - y; i <= x0 + y; i++)
        {
            LCD_PutPixel(i, y0 + x, mode);
            LCD_PutPixel(i, y0 - x, mode);
        }

        y++;
        radiusError += yChange;
        yChange += 2;
        if (((radiusError << 1) + xChange) > 0)
        {
            x--;
            radiusError += xChange;
            xChange += 2;
        }
    }
}



