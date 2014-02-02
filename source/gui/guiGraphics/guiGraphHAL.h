#ifndef __GUI_GRAPH_HAL_
#define __GUI_GRAPH_HAL_

#include <stdint.h>


// Size definitions in points for Nokia 1202 LCD
#define LCD_XSIZE (2*96)
#define LCD_YSIZE 68

// Buffer size in bytes
#define LCD_BUFFER_SIZE (2*96*9)

/*
// Size definitions in points
#define LCD_XSIZE (2*128)
#define LCD_YSIZE 128

// Buffer size in bytes
#define LCD_BUFFER_SIZE (LCD_XSIZE * 16)
*/

// LCD functions settings
//#define SOFT_HORIZ_REVERSED

// counter increments from 0 to LCD_xxx_PERIOD-1
// if counter < LCD_xxx_COMPARE, pixel is put unchaged
// if counter >= LCD_xxx_COMPARE, pixel is put inversed
#define LCD_DOT_PERIOD      4
#define LCD_DOT_COMPARE     2
#define LCD_DASH_PERIOD     7
#define LCD_DASH_COMPARE    5




// Pixel output modes
#define PIXEL_MODE_REWRITE  0x00
#define PIXEL_MODE_AND      0x01
#define PIXEL_MODE_OR       0x02
#define PIXEL_MODE_XOR      0x03

// Image output modes
#define IMAGE_MODE_NORMAL    0x01
#define IMAGE_MODE_INVERSE   0x00
// Aliases for fill rect
#define FILL_WITH_BLACK      0x01
#define FILL_WITH_WHITE      0x00

#define LCD_FillRect(x_pos, y_pos, width, height, mode) \
    LCD_DrawImage(0, x_pos, y_pos, width, height, mode)


// Line drawing mode (not for all functions)
#define LINE_STYLE_SOLID      0x10
#define LINE_STYLE_DASHED     0x20
#define LINE_STYLE_DOTTED     0x30





extern uint8_t lcdBuffer[LCD_BUFFER_SIZE];
extern uint8_t LCD_lineStyle;

void LCD_SetPixelOutputMode(uint8_t newMode);
void LCD_SetLineStyle(uint8_t newStyle);

void LCD_FillWholeBuffer(uint8_t pixelValue);
void LCD_PutPixel (uint8_t x_pos, uint8_t y_pos, uint8_t pixelValue);
void LCD_DrawHorLine(uint8_t x_pos, uint8_t y_pos, uint8_t length, uint8_t pixelValue);
void LCD_DrawVertLine(uint8_t x_pos, uint8_t y_pos, uint8_t length, uint8_t pixelValue);

void LCD_DrawImage(const uint8_t* img, uint8_t x_pos, uint8_t y_pos, uint8_t width, uint8_t height, uint8_t mode);


#endif
