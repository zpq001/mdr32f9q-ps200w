
// Size definitions in points
#define LCD_PHY_XSIZE 96
#define LCD_PHY_YSIZE 68

// Size definitions in bytes
#define LCD_PHY_BUFFER_SIZE 864 //96*9


#define LCD0 1
#define LCD1 2


//------ LCD init settings -------//

// X-axis hardware reverse
#define HORIZONTAL_NORMAL
//#define HORIZONTAL_REVERSE

// Y-axis hardware reverse
//#define VERTICAL_NORMAL
#define VERTICAL_REVERSE

// Hardware reset
#define USE_HARDWARE_RESET

// Display inversion
//#define LCD_INVERSE
//-------------------------------//






//------------------------------------//
// Public functions

// Initializes both LCDs and flushes memory
void LcdInit(void);
// Copy buffer data into specified LCD memory by core
void LcdUpdateByCore(uint8_t display, uint16_t* lcd_buffer);
void LcdUpdateBothByCore(uint8_t* lcd_buffer);

//------------------------------------//
// Internal functions

// Initialize selected display
void LcdSingleInit(void);
// Write 9-bit word over SPI
void LcdWrite(uint8_t val, uint8_t mode);
// Delay for LCD in us
void LcdDelayUs(uint16_t us);
// Select LCD (LCD0, LCD1)
void LcdSelect(uint8_t lcd);







