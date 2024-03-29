/********************************************************************
	Module lcd_1202.c
	
		Low-level functions for Nokia 1202 GLCD

********************************************************************/

#include "MDR32F9Qx_ssp.h"
#include "MDR32F9Qx_port.h"

#include "lcd_1202_defs.h"		// LCD-specific command definitions
#include "lcd_1202.h"					
#include "defines.h"					// Contains port defines
#include "dwt_delay.h"				// Delays used for LCD



//--------- Macros --------------//
// Page address set 
#define LcdSetRow(row) 		LcdWrite(CMD_SET_PAGE | ((row) & 0x0F), CMD) 
// Column address set
#define LcdSetCol(col) 		LcdWrite(CMD_SET_COLUMN_UPPER_BITS | ((col)>>4), CMD); 			/* Sets the DDRAM column address - upper 3-bit */ \
													LcdWrite(CMD_SET_COLUMN_LOWER_BITS | ((col) & 0x0F), CMD); 	/* lower 4-bit */ 
																								
// Hardware-specific:
												
// Reset LCD line
#ifdef USE_HARDWARE_RESET
	#define lcd_set_reset_line			PORT_SetBits(MDR_PORTE, 1<<LCD_RST);
	#define lcd_clear_reset_line		PORT_ResetBits(MDR_PORTE, 1<<LCD_RST);
#endif

// Wait for SSP to send last word
#define LCD_TX_DONE (SSP_GetFlagStatus(MDR_SSP2,SSP_FLAG_BSY)==RESET)
//-------------------------------//



//==============================================================//
// Write 9-bit word to the LCD
// Hardware-specific function
//==============================================================//
void LcdWrite(uint8_t val, uint8_t mode)
{
	uint16_t temp = val;
	if (mode == DATA)		
		temp |= 0x0100;;	// set D/C bit (MSB of the packet)
	// Wait if TX FIFO is full
	while( SSP_GetFlagStatus(MDR_SSP2,SSP_FLAG_TNF)!= SET );
	SSP_SendData (MDR_SSP2,temp);
}

//==============================================================//
// Delay for LCD in us
// Hardware-specific function
//==============================================================//
void LcdDelayUs(uint16_t us)
{
	DWTDelayUs(us);
}

//==============================================================//
// Select LCD (LCD0, LCD1)
// Hardware-specific function
//==============================================================//
void LcdSelect(uint8_t lcd)
{
	(lcd==LCD0) ? PORT_SetBits(MDR_PORTE, 1<<LCD_SEL) : PORT_ResetBits(MDR_PORTE, 1<<LCD_SEL);
	LcdDelayUs(20);
}


//==============================================================//
// Initialize single LCD and flush memory
//==============================================================//
void LcdSingleInit(void)
{
	uint16_t i;
	// Software reset
	LcdWrite(CMD_RESET,CMD);	
	//---- Ignored by china LCDs ----//
	// Charge pump
	LcdWrite(CMD_CHARGE_PUMP_MUL,CMD);
	LcdWrite(0x01,CMD);					// A bit lower VLCD for 3.3V
	// Set VOP (contrast)
	LcdWrite(CMD_SET_VOP,CMD);
	LcdWrite(0x00,CMD);					// default
	// V0 - voltage range
	LcdWrite(CMD_SET_VO_RANGE | V0R_8V12,CMD);	// default
	// Electronic volume
	LcdWrite(CMD_SET_EV | 0x10,CMD);	// default
	// X-axis reverse
	#ifdef HORIZONTAL_REVERSE	
		LcdWrite(CMD_SELECT_SEGMENT_DIR | SEGMENT_REVERSE, CMD);
	#else 
		LcdWrite(CMD_SELECT_SEGMENT_DIR | SEGMENT_NORMAL, CMD);
	#endif
	//------------------------------//
	// Y-axis reverse
	#ifdef VERTICAL_REVERSE	
		LcdWrite(CMD_SELECT_COMMON_DIR | COMMON_REVERSE, CMD);
	#else 
		LcdWrite(CMD_SELECT_COMMON_DIR | COMMON_NORMAL, CMD);
	#endif
	// Display inversion
	#ifdef LCD_INVERSE
		LcdWrite(CMD_NORM_INVERSE | DISPLAY_INVERSE,CMD);	
	#else
		LcdWrite(CMD_NORM_INVERSE | DISPLAY_NORMAL,CMD);	
  #endif	
	// Power saver OFF
	LcdWrite(CMD_ALL_POINTS_ON_OFF | POINTS_NORMAL,CMD);	
	// Internal voltage enable
	LcdWrite(CMD_SET_POWER | ALL_ENABLE,CMD);
	// Start line 0
	LcdWrite(CMD_SET_START_LINE | 0x00, CMD);
	
	// Others deafults
	LcdWrite(CMD_SET_ICON_MODE | ICON_MODE_DISABLE,CMD);
	LcdWrite(CMD_LINES_NUMBER | 0x00,CMD);
	LcdWrite(CMD_SET_BIAS | 0x00,CMD);
	
	// Add memory init code here
	
	// clear LCD
	LcdSetRow(0);
  LcdSetCol(0);
	for(i=0; i<LCD_BUFFER_SIZE-1; i++)
  	LcdWrite(0,DATA);
  
	// Display ON
	LcdWrite(CMD_ON_OFF | DISPLAY_ON,CMD);
}


//==============================================================//
// Initializes both LCDs 
//==============================================================//
void LcdInit(void)
{
  // Hardware reset
	#ifdef USE_HARDWARE_RESET
		lcd_clear_reset_line;
		LcdDelayUs(10000);
		lcd_set_reset_line;
		LcdDelayUs(10000);
	#endif
  // Initialize LCD #0
	LcdSelect(LCD0);
	LcdSingleInit();
	// wait until all words are sent
	while (!LCD_TX_DONE);	
	// Initialize LCD #1
	LcdSelect(LCD1);
	LcdSingleInit();
}




//==============================================================//
// Copy buffer data into specified LCD memory by core
// 	Parameters:
//		uint8_t diplay				- number of LCD to update, LCD0 or LCD1
//		uint16_t* lcd_buffer	-	LCD buffer
//==============================================================//
void LcdUpdateByCore(uint8_t display, uint16_t* lcd_buffer)
{
	uint16_t i;
	
	LcdSelect(display);
	LcdSetRow(0);
	LcdSetCol(0);
	for (i=0; i<LCD_BUFFER_SIZE; i++)
		LcdWrite(lcd_buffer[i],DATA);
}



