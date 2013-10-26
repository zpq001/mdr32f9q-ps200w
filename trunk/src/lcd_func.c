/********************************************************************
	Module lcd_func.c
	
		Functions for GLCD
		All functions work with temporary buffer

********************************************************************/

#include <stdio.h>
#include "MDR32Fx.h"
#include "lcd_1202.h"
#include "font_defs.h"

#include "lcd_func.h"








// Lcd buffers
uint16_t lcd0_buffer[LCD_BUFFER_SIZE];
uint16_t lcd1_buffer[LCD_BUFFER_SIZE];



// ========================================================================== //
//                           Graphic functions                                //
// ===========================================================================//

//==============================================================//
// Modify single pixel
//	Parameters:
//		uint8_t x_pos							- pixel x coordinate
//		uint8_t y_pos							- pixel y coordinate
//		uint8_t mode							- pixel output mode, PIXEL_OFF, PIXEL_ON, PIXEL_XOR
//		uint16_t *lcd_buffer			- LCD buffer
//==============================================================//
void LcdPutPixel (uint8_t x_pos, uint8_t y_pos, uint8_t mode, uint16_t *lcd_buffer)     
{
int16_t buffer_index;
uint8_t offset, data;
	// Get byte index
	#ifdef SOFT_HORIZ_REV
		buffer_index = (y_pos / 8)*LCD_XSIZE + LCD_XSIZE - 1 - x_pos;
	#else
		buffer_index = (y_pos / 8)*LCD_XSIZE + x_pos;
	#endif	
	// Get bit offset
  offset  = y_pos % 8;                 
  data = lcd_buffer[buffer_index];                     
  if ( mode == PIXEL_OFF ) data &= ( ~( 0x01 << offset ) );		
      else if ( mode == PIXEL_ON ) data |= ( 0x01 << offset );
           else if ( mode  == PIXEL_XOR ) data ^= ( 0x01 << offset );
  lcd_buffer[buffer_index] = data;		          
}



void LcdPutHorLine(uint8_t x_pos, uint8_t y_pos, uint8_t length, uint8_t mode, uint16_t *lcd_buffer)
{
  uint8_t cnt_done=0;
  while (cnt_done<length)
    LcdPutPixel(x_pos+cnt_done++,y_pos,mode,lcd_buffer);  
}

void LcdPutVertLine(uint8_t x_pos, uint8_t y_pos, uint8_t length, uint8_t mode, uint16_t *lcd_buffer)
{
  uint8_t cnt_done=0;
  while (cnt_done<length)
    LcdPutPixel(x_pos,y_pos+cnt_done++,mode,lcd_buffer);  
}







//==============================================================//
// Fills LCD buffer with specified value
//	Parameters:
//		uint16_t* lcd_buffer			- pointer to a buffer
//		uint8_t value							- value to fill with
//==============================================================//
void LcdFillBuffer(uint16_t* lcd_buffer, uint8_t value)
{
	uint16_t i;
	for (i=0;i<LCD_BUFFER_SIZE;i++)
	{
		lcd_buffer[i] = value;
	}
}


/*
//==============================================================//
// Put a char using native font (Font_6x8)
// 0 <= y <= 9,	0 <= x <= (LCD_XSIZE - NativeFontWidth)
//==============================================================//
void LcdPutCharNative(uint8_t x, uint8_t y, uint8_t ch, uint8_t inversion, uint16_t* lcd_buffer)
{ 
int8_t i;
uint16_t PointerFont = (ch<<2) + (ch<<1);
uint16_t cnt;
	#ifdef SOFT_HORIZ_REV
		cnt=y*LCD_XSIZE+(LCD_XSIZE-x)- NativeFontWidth; 
		for(i = NativeFontWidth-1; i>=0; i--)
      lcd_buffer[cnt+i] =  Font_6x8_Data[PointerFont++]^inversion;   
	#else
		cnt=y*LCD_XSIZE+x;
		for(i = 0; i<=NativeFontWidth-1; i++)
			lcd_buffer[cnt+i] =  Font_6x8_Data[PointerFont++]^inversion; 
	#endif
}


void LcdPutStrNative(uint8_t x, uint8_t row, uint8_t *dataPtr, uint8_t inversion, uint16_t* lcd_buffer )
{
    // loop to the end of string
  while ((*dataPtr)&&(x<=LCD_XSIZE-NativeFontWidth) )
  {
    LcdPutCharNative(x, row, (*dataPtr),inversion, lcd_buffer);
    x+=NativeFontWidth;
    dataPtr++;
  }  
}
*/



//==============================================================//
// Print an image at specified area
// Parameters:
//		uint8_t* img					- pointer to image
//		uint8_t x_pos					- horizontal image position, pixels
//		uint8_t y_pos					- vertical image position, pixels
//		uint8_t width					-	image width
//		uint8_t height				- image height
//		uint16_t* lcd_buffer	- buffer to work with
// If defined SOFT_HORIZ_REV, function will put image reversed
//==============================================================//
void LcdPutImage(uint8_t* img, uint8_t x_pos, uint8_t y_pos, uint8_t width, uint8_t height, uint16_t* lcd_buffer)
{
	uint16_t vertical_bits_remain, counter_inc;
	uint16_t picture_index, buffer_index;
	uint16_t start_buffer_index;
	uint16_t i;
	uint16_t img_shift;
	uint8_t mask, temp;
	uint8_t offset = y_pos % 8;

	#ifdef SOFT_HORIZ_REV
		start_buffer_index = (y_pos / 8)*LCD_XSIZE + LCD_XSIZE - 1 - x_pos;
	#else
		start_buffer_index = (y_pos / 8)*LCD_XSIZE + x_pos;
	#endif
	
	//-------------//
	
	// X-loop
	for (i=0;i<width;i++)
	{
			img_shift = 0;
			picture_index = i;
			#ifdef SOFT_HORIZ_REV
				buffer_index =  start_buffer_index - i;
			#else
				buffer_index =  start_buffer_index + i;
			#endif
			vertical_bits_remain = height;
			mask = -(1<<offset); 
			
			if (offset + height < 8 )
			{
				temp = (1<<(offset + height))-1;
				mask &= temp;
				counter_inc = height;
			}
			else
			{
				counter_inc = 8-offset;
			}
		

			// Y-loop
			while(vertical_bits_remain)
			{			  
				temp = img[picture_index];						// get image byte			
				img_shift |= temp << offset;					// add shifted value to the 16-bit accumulator
		
				temp = lcd_buffer[buffer_index];			// get old value
				temp &= ~mask	;												// clear only used bits
				temp |= img_shift & mask;							// add only used bits
				lcd_buffer[buffer_index] = temp;			// write new value back
		
				img_shift = img_shift >> 8;						// shift 16-bit register
				buffer_index += LCD_XSIZE;
				picture_index += width;	
			
				vertical_bits_remain -= counter_inc;
				if (vertical_bits_remain >= 8)
					counter_inc = 8;
				else
					counter_inc = vertical_bits_remain;	
				mask = (1 << counter_inc)-1;
							
			}
	}
}




	/*
	uint16_t i,j;
	for (j=0;j<9;j++)
		for (i=0;i<LCD_XSIZE-1; i++)
		  lcd_buffer[j*LCD_XSIZE+i] = img[j*LCD_XSIZE+LCD_XSIZE-i-1]; 
	*/
	
	/*
	uint16_t buffer_index;
	uint16_t picture_index = 0;
	uint16_t i,j;
	
	for (i=0;i<height/8;i++)
	{
		buffer_index = (y_pos+i)*(LCD_XSIZE) + (LCD_XSIZE-1-x_pos);
		for (j=0;j<width;j++)
		{
			lcd_buffer[buffer_index--] = img[picture_index++];
		}
	}
	*/
	
	
	

tSpecialFontItem GetFontItem(tSpecialFont* myFont, char c)
{
	uint16_t i;
	for (i=0;i<myFont->count;i++)
		if(myFont->Chars[i].code == c)
			return myFont->Chars[i];
	// If not found
	return myFont->Chars[0];
}



void LcdPutSpecialStr(uint8_t x_pos, uint8_t y_pos, uint8_t* str, tSpecialFont* myFont, uint16_t* lcd_buffer)
{
	uint16_t i = 0;
	tSpecialFontItem item_ptr;
	while(str[i] != '\0')
	{
		item_ptr = GetFontItem(myFont,str[i++]);
		LcdPutImage((uint8_t*)item_ptr.data,x_pos,y_pos,item_ptr.width,myFont->height,lcd_buffer);
		x_pos += item_ptr.width;
	}
}


void LcdPutNormalStr(uint8_t x_pos, uint8_t y_pos, uint8_t* str, tNormalFont* myFont, uint16_t* lcd_buffer)
{
	uint16_t i = 0;
	uint16_t charPointer;
	
	while(str[i] != '\0')
	{
		charPointer = str[i++] * myFont->bytesPerChar;
		LcdPutImage((uint8_t*)&myFont->data[charPointer],x_pos,y_pos,myFont->width,myFont->height,lcd_buffer);
		x_pos += myFont->width;
	}

	
}





