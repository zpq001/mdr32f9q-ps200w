#ifndef __GUI_GRAPH_PRIMITIVES_H_
#define __GUI_GRAPH_PRIMITIVES_H_

#include <stdint.h>
#include "guiGraphHAL.h"
#include "guiFonts.h"

// Align modes - required alignment mode is selected by
// combining these flags
#define ALIGN_CENTER    0x00
#define ALIGN_LEFT      0x01
#define ALIGN_RIGHT     0x02
#define ALIGN_TOP       0x04
#define ALIGN_BOTTOM    0x08
// Aliases
#define ALIGN_TOP_LEFT      (ALIGN_TOP | ALIGN_LEFT)
#define ALIGN_BOTTOM_LEFT   (ALIGN_BOTTOM | ALIGN_LEFT)
#define ALIGN_TOP_RIGHT     (ALIGN_TOP | ALIGN_RIGHT)
#define ALIGN_BOTTOM_RIGHT   (ALIGN_BOTTOM | ALIGN_RIGHT)


typedef struct {
    uint8_t x1;
    uint8_t y1;
    uint8_t x2;
    uint8_t y2;
} rect_t;


extern const tFont* LCD_currentFont;

void LCD_SetFont(const tFont *newFont);

void LCD_DrawRect(uint8_t x_pos, uint8_t y_pos, uint8_t width, uint8_t height, uint8_t pixelValue);
uint8_t LCD_GetFontItem(const tFont *font, uint8_t code, uint8_t *width, uint16_t *offset);
uint8_t LCD_GetNextFontChar(const tFont *font, uint8_t code, int8_t scanDir);
void LCD_PrintString(char *str, uint8_t x, uint8_t y, uint8_t mode);
void LCD_PrintStringAligned(char *str, rect_t *rect, uint8_t alignment, uint8_t mode);
void LCD_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t mode);
void LCD_DrawCircle(int16_t x0, int16_t y0, int16_t radius, uint8_t mode);
void LCD_DrawFilledCircle(int16_t x0, int16_t y0, int16_t radius, uint8_t mode);

#endif
