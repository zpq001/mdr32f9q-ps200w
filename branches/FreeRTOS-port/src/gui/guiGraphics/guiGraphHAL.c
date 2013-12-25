/**********************************************************
  Module guiGraphHAL contains low-level LCD routines.



**********************************************************/

#include <stdint.h>
#include "guiGraphHAL.h"


// Display buffer
uint8_t lcdBuffer[LCD_BUFFER_SIZE];

uint8_t LCD_lineStyle;

static uint8_t pixel_and_mask;
static uint8_t pixel_or_mask;
static uint8_t pixel_xor_mask;



//-------------------------------------------------------//
// Sets pixel output mode. Pixels will be processed as follows:
//
// REWRITE: data = (data & ~bit_mask) | (pixelValue & bit_mask)
// AND:     data &= (pixelValue | ~bit_mask)
// OR:      data |= (pixelValue & bit_mask)
// XOR:     data ^= (pixelValue & bit_mask)
//-------------------------------------------------------//
void LCD_SetPixelOutputMode(uint8_t newMode)
{
    switch (newMode)
    {
        case PIXEL_MODE_AND:
            pixel_and_mask = 0x00;
            pixel_or_mask = 0x00;
            pixel_xor_mask = 0x00;
            break;
        case PIXEL_MODE_OR:
            pixel_and_mask = 0xFF;
            pixel_or_mask = 0xFF;
            pixel_xor_mask = 0x00;
            break;
        case PIXEL_MODE_XOR:
            pixel_and_mask = 0xFF;
            pixel_or_mask = 0x00;
            pixel_xor_mask = 0xFF;
            break;
        default:    // PIXEL_MODE_REWRITE:
            pixel_and_mask = 0x00;
            pixel_or_mask = 0xFF;
            pixel_xor_mask = 0x00;
            break;
    }
}


//-------------------------------------------------------//
// Sets line style
//
//-------------------------------------------------------//
void LCD_SetLineStyle(uint8_t newStyle)
{
    if (newStyle != LCD_lineStyle)
    {
        LCD_lineStyle = newStyle;
    }
}


//-------------------------------------------------------//
// Fills whole buffer with specified value
//	Parameters:
//		uint8_t value - pixel color to fill with (1 or 0)
// Buffer is filled direct with value specified
// No pixel output mode is used
//-------------------------------------------------------//
void LCD_FillWholeBuffer(uint8_t pixelValue)
{
    uint16_t i;
    if (pixelValue) pixelValue = 0xFF;

    for (i=0;i<LCD_BUFFER_SIZE;i++)
    {
        lcdBuffer[i] = pixelValue;
    }
}



//-------------------------------------------------------//
// Modify single pixel
//	Parameters:
//		uint8_t x_pos							- pixel x coordinate
//		uint8_t y_pos							- pixel y coordinate
//		uint8_t pixelValue						- new pixel value, 1 or 0
// Pixel output mode is set by calling LCD_setPixelOutputMode()
//-------------------------------------------------------//
void LCD_PutPixel (uint8_t x_pos, uint8_t y_pos, uint8_t pixelValue)
{
    int16_t buffer_index;
    uint8_t data, bit_mask;
    if (pixelValue) pixelValue = 0xFF;
    // Get byte index
    #ifdef SOFT_HORIZ_REVERSED
        buffer_index = ((uint16_t)(y_pos / 8))*LCD_XSIZE + LCD_XSIZE - 1 - x_pos;
    #else
        buffer_index = ((uint16_t)(y_pos / 8))*LCD_XSIZE + x_pos;
    #endif
    // Bit mask defines changed pixel
    bit_mask = 1 << (y_pos % 8);
    // Modify data byte
    data = (uint8_t)lcdBuffer[buffer_index];
    data &= pixelValue | ~bit_mask | pixel_and_mask;
    data |= pixelValue & bit_mask & pixel_or_mask;
    data ^= pixelValue & bit_mask & pixel_xor_mask;
    lcdBuffer[buffer_index] = data;
}



//-------------------------------------------------------//
// Draws horizontal line using LCD_lineStyle
//	Parameters:
//		uint8_t x_pos	- pixel x coordinate
//		uint8_t y_pos	- pixel y coordinate
//      pixelValue  	- new pixel value, 1 or 0
// Pixel output mode is set by calling LCD_setPixelOutputMode()
//-------------------------------------------------------//
void LCD_DrawHorLine(uint8_t x_pos, uint8_t y_pos, uint8_t length, uint8_t pixelValue)
{
    uint16_t buffer_index;
    int8_t buffer_increment;
    uint8_t data, bit_mask;
    uint8_t pixelInversed;
    uint8_t dashCompare;
    uint8_t dashPeriod;
    uint8_t dashCounter = 0;

    if (pixelValue) pixelValue = 0xFF;

    switch(LCD_lineStyle)
    {
        case LINE_STYLE_DASHED:
            dashPeriod = LCD_DASH_PERIOD;
            dashCompare = LCD_DASH_COMPARE;
            break;
        case LINE_STYLE_DOTTED:
            dashPeriod = LCD_DOT_PERIOD;
            dashCompare = LCD_DOT_COMPARE;
            break;
         default: //LINE_STYLE_SOLID:
            dashPeriod = 10;
            dashCompare = 10;   // arbitrary, but >= dashPeriod
            break;
    }

    // Get byte index
    #ifdef SOFT_HORIZ_REVERSED
        buffer_index = ((uint16_t)(y_pos / 8))*LCD_XSIZE + LCD_XSIZE - 1 - x_pos;
        buffer_increment = -1;
    #else
        buffer_index = ((uint16_t)(y_pos / 8))*LCD_XSIZE + x_pos;
        buffer_increment = 1;
    #endif
    // Bit mask defines changed pixel
    bit_mask = 1 << (y_pos % 8);
    while(length)
    {
        pixelInversed = (dashCounter < dashCompare) ? pixelValue : ~pixelValue;

        // Modify data byte
        data = (uint8_t)lcdBuffer[buffer_index];
        data &= pixelInversed | ~bit_mask | pixel_and_mask;
        data |= pixelInversed & bit_mask & pixel_or_mask;
        data ^= pixelInversed & bit_mask & pixel_xor_mask;
        lcdBuffer[buffer_index] = data;
        buffer_index += buffer_increment;
        length--;

        dashCounter++;
        if (dashCounter == dashPeriod)
            dashCounter = 0;
    }
}


//-------------------------------------------------------//
// Draws vertical line using LCD_lineStyle
//	Parameters:
//		uint8_t x_pos	- pixel x coordinate
//		uint8_t y_pos	- pixel y coordinate
//      pixelValue  	- new pixel value, 1 or 0
// Pixel output mode is set by calling LCD_setPixelOutputMode()
//-------------------------------------------------------//
void LCD_DrawVertLine(uint8_t x_pos, uint8_t y_pos, uint8_t length, uint8_t pixelValue)
{
    uint8_t dashCompare;
    uint8_t dashPeriod;
    uint8_t dashCounter = 0;
    uint8_t pixelInversed;

    if (pixelValue) pixelValue = 0xFF;

    switch(LCD_lineStyle)
    {
        case LINE_STYLE_DASHED:
            dashPeriod = LCD_DASH_PERIOD;
            dashCompare = LCD_DASH_COMPARE;
            break;
        case LINE_STYLE_DOTTED:
            dashPeriod = LCD_DOT_PERIOD;
            dashCompare = LCD_DOT_COMPARE;
            break;
         default: //LINE_STYLE_SOLID:
            dashPeriod = 10;
            dashCompare = 10;   // arbitrary, but >= dashPeriod
            break;
    }

    while(length)
    {
        pixelInversed = (dashCounter < dashCompare) ? pixelValue : ~pixelValue;

        LCD_PutPixel(x_pos, y_pos, pixelInversed);
        y_pos++;
        length--;

        dashCounter++;
        if (dashCounter == dashPeriod)
            dashCounter = 0;
    }
}






/*
void LCD_PutVertLine(uint8_t x_pos, uint8_t y_pos, uint8_t length, uint8_t mode)
{
    uint16_t buffer_index;
    uint8_t temp;
    uint8_t counter_inc;
    uint8_t bit_mask;
    uint8_t offset = y_pos % 8;
    uint8_t pixelInversed;
    uint8_t dashCompare;
    uint8_t dashPeriod;
    uint8_t dashCounter = 0;

    if (pixelValue) pixelValue = 0xFF;

    switch(LCD_lineStyle)
    {
        case LCD_LINE_DASHED:
            dashPeriod = LCD_DASH_PERIOD;
            dashCompare = LCD_DASH_COMPARE;
            break;
        case LCD_LINE_DOTTED:
            dashPeriod = LCD_DOT_PERIOD;
            dashCompare = LCD_DOT_COMPARE;
            break;
         default: //LCD_LINE_SOLID:
            dashPeriod = 10;
            dashCompare = 10;   // arbitrary, but >= dashPeriod
            break;
    }

    // Get byte index
    #ifdef SOFT_HORIZ_REVERSED
        buffer_index = ((uint16_t)(y_pos / 8))*LCD_XSIZE + LCD_XSIZE - 1 - x_pos;
    #else
        buffer_index = ((uint16_t)(y_pos / 8))*LCD_XSIZE + x_pos;
    #endif

    bit_mask = -(1<<offset);
    if (offset + length < 8 )
    {
        temp = (1<<(offset + length))-1;
        bit_mask &= temp;
        counter_inc = length;
    }
    else
    {
        counter_inc = 8-offset;
    }

    while(length)
    {
        pixelInversed = (dashCounter < dashCompare) ? pixelValue : ~pixelValue;

        //img_shift |= temp << offset;    //
        img_shift = 0xFF;



        temp = (uint8_t)lcdBuffer[buffer_index];     // get old value
        // Modify data byte
        // Bit mask defines changed pixel
        temp &= img_shift | ~bit_mask | pixel_and_mask;
        temp |= img_shift & bit_mask & pixel_or_mask;
        temp ^= img_shift & bit_mask & pixel_xor_mask;
        lcdBuffer[buffer_index] = temp;              // write new value back

        //img_shift = img_shift >> 8;						// shift 16-bit register
        buffer_index += LCD_XSIZE;

        vertical_bits_remain -= counter_inc;
        if (vertical_bits_remain >= 8)
            counter_inc = 8;
        else
            counter_inc = length;
        bit_mask = (1 << counter_inc)-1;

        length--;

        dashCounter++;
        if (dashCounter == dashPeriod)
            dashCounter = 0;
    }
}
*/




//-------------------------------------------------------//
// Print an image at specified area
// This function can be also used for filling rectangles
// Parameters:
//		uint8_t* img					- pointer to image
//          if img = 0, area is filled with color, depending on mode parameter
//		uint8_t x_pos					- horizontal image position, pixels
//		uint8_t y_pos					- vertical image position, pixels
//		uint8_t width					- image width
//		uint8_t height                  - image height
//      mode:
//          IMAGE_MODE_NORMAL - normal image,   if img = 0, image is treated as all 1's
//          IMAGE_MODE_INVERSE - inversed image, if img = 0, image is treated as all 0's
//
// If defined SOFT_HORIZ_REVERSED, function will put image reversed
// Pixel output mode is set by calling LCD_setPixelOutputMode()
//-------------------------------------------------------//
void LCD_DrawImage(const uint8_t* img, uint8_t x_pos, uint8_t y_pos, uint8_t width, uint8_t height, uint8_t mode)
{
    uint16_t vertical_bits_remain, counter_inc; // counter_inc - uint8_t ? FIXME
    uint16_t picture_index, buffer_index;
    uint16_t start_buffer_index;
    uint16_t i;
    uint16_t img_shift;
    uint8_t bit_mask, temp;
    uint8_t offset = y_pos % 8;
    /*
        img != 0, mode = 1 - print text as black
            => mode = 0x00
        img != 0, mode = 0 - print text as white
            => mode = 0xFF
        img = 0, mode = 1 - fill with black
            => mode = 0x00
        img = 0, mode = 0 - fill with white
            => mode = 0xFF
    */
    mode = (mode) ? 0x00 : 0xFF;

    #ifdef SOFT_HORIZ_REVERSED
        start_buffer_index = ((uint16_t)(y_pos / 8))*LCD_XSIZE + LCD_XSIZE - 1 - x_pos;
    #else
        start_buffer_index = ((uint16_t)(y_pos / 8))*LCD_XSIZE + x_pos;
    #endif

    //-------------//

    // X-loop
    for (i=0;i<width;i++)
    {
            img_shift = 0;
            picture_index = i;
            #ifdef SOFT_HORIZ_REVERSED
                buffer_index =  start_buffer_index - i;
            #else
                buffer_index =  start_buffer_index + i;
            #endif
            vertical_bits_remain = height;
            bit_mask = -(1<<offset);

            if (offset + height < 8 )
            {
                temp = (1<<(offset + height))-1;
                bit_mask &= temp;
                counter_inc = height;
            }
            else
            {
                counter_inc = 8-offset;
            }

            // Y-loop
            while(vertical_bits_remain)
            {
                if (img != 0)
                    temp = img[picture_index];                  // get image byte
                else
                    temp = 0xFF;
                temp ^= mode;
                img_shift |= temp << offset;					// add shifted value to the 16-bit accumulator

                temp = (uint8_t)lcdBuffer[buffer_index];     // get old value
                // Modify data byte
                // Bit mask defines changed pixel
                temp &= img_shift | ~bit_mask | pixel_and_mask;
                temp |= img_shift & bit_mask & pixel_or_mask;
                temp ^= img_shift & bit_mask & pixel_xor_mask;
                lcdBuffer[buffer_index] = temp;              // write new value back

                img_shift = img_shift >> 8;						// shift 16-bit register
                buffer_index += LCD_XSIZE;
                picture_index += width;

                vertical_bits_remain -= counter_inc;
                if (vertical_bits_remain >= 8)
                    counter_inc = 8;
                else
                    counter_inc = vertical_bits_remain;
                bit_mask = (1 << counter_inc)-1;
            }
    }
}









