/***********************************************************************
	Definitions for Nokia 1202 LCD (ste2007 controller)
	
	There are two types of LCD commands:
	1. CMD is composed of a fixed command-specific code and
		a variable parameter value. For this case, use
		command code definition ORed with proper value.
		Such CMDs are commented as "command | params"
		For example, LCDWriteCMD(CMD_ON_OFF|DISPLAY_ON) or
		LCDWriteCMD(CMD_SET_PAGE|0x02).
		Result is transferred as a single 9-bit word.
	2. CMD is composed of two words: 
		- first is fixed command-specific code.
		- second is variable parameter value.
		First cmd code is transferred, then parameter value.
	
	The MSB of the 9-bit word is D/C bit.
	Set D/C to 0 when any commands or
		command parameter values are transferred.
	Set D/C to 1 only when display RAM data is transferred.
	
	LCD voltage is set (at t = 25C) as follows:
	VLCD = ( VOP[7:0] + EV[4:0]-16 + 32*V0R[2:0] )*B + VLCDmin, where
		B = 0.4V, VLCDmin = 3V.
	VOP is contrast, default is 0;
	EV  is Electronic Volume, default is 0x10;
	V0R is Voltage Range, default is 0x4;
	
	See ste2007 datasheet for futher information
	
	China LCDs do not have full functional!
	China OK:
		CMD_ON_OFF
		CMD_NORM_REVERSE
		CMD_ALL_POINTS_ON_OFF
		CMD_SET_PAGE
		CMD_SET_COLUMN_UPPER_BITS
		CMD_SET_COLUMN_LOWER_BITS
		CMD_SET_START_LINE
		CMD_SELECT_COMMON_DIR (vertical orientation)
		CMD_SET_POWER
		CMD_RESET
		CMD_NOP
***********************************************************************/



// D/C bit definitions
// D/C bit is set ONLY for display RAM data transmit.
// Command params are sent with D/C bit = 0
#define CMD 0
#define DATA 1

// Dispaly On/Off, Display all points ON/OFF
// command | params
#define CMD_ON_OFF 0xAE
#define DISPLAY_ON 0x1
#define DISPLAY_OFF 0x0

// Normal or reversed display
// command | params
#define CMD_NORM_INVERSE 0xA6
#define DISPLAY_NORMAL 0x0
#define DISPLAY_INVERSE 0x1

// All points on/off
// command | params
#define CMD_ALL_POINTS_ON_OFF 0xA4
#define POINTS_NORMAL 0x0
#define POINTS_ALL_ON 0x1

//Page address set
// command | params
// Valid values are 0x0 to 0xF
#define CMD_SET_PAGE 0xB0

// Column set high 3 bits
// 1. command  2. params
// Valid values are 0x0 to 0x7
#define CMD_SET_COLUMN_UPPER_BITS 0x10

// Column set low 4 bits
// 1. command  2. params
// Valid values are 0x0 to 0xF
#define CMD_SET_COLUMN_LOWER_BITS 0x00

// Display start line address set
// command | params
// Valid values are 0x00 to 0x3F
#define CMD_SET_START_LINE 0x40

// Select segment driver direction
// command | params
#define CMD_SELECT_SEGMENT_DIR 0xA0
#define SEGMENT_NORMAL 0x0
#define SEGMENT_REVERSE 0x1

// Select common driver direction
// command | params
#define CMD_SELECT_COMMON_DIR 0xC0
#define COMMON_NORMAL 0x0
#define COMMON_REVERSE 0x8

// Identification byte read
#define CMD_READ_ID 0xDB

// Power control set (booster, regulator, follower)
// command | params
#define CMD_SET_POWER 0x28
#define ALL_ENABLE 0x07
#define ALL_DISABLE 0x00

// Set voltage range (V0, V0R)
// command | params
// V0R range is 0x0 to 0x7, default is 0x4
#define CMD_SET_VO_RANGE 0x20
#define V0R_3V 		0x0
#define V0R_4V28 	0x1
#define V0R_5V56 	0x2
#define V0R_6V84 	0x3
#define V0R_8V12 	0x4
#define V0R_9V4  	0x5
#define V0R_10V68 	0x6
#define V0R_11V96 	0x7

// Set electronic volume (EV)
// command | params
// Volume value is 0x00 to 0x1F, default is 0x10
#define CMD_SET_EV 0x80

// Set VOP (VLCD)
// 1. command  2. params
// Contrast range is 0x00 to 0xFF (-127 to 127, default is 0)
#define CMD_SET_VOP 0xE1

// Reset
#define CMD_RESET 0xE2

// No operation
#define CMD_NOP 0xE3

// Temperature compensation
// 1. command  2. params
// Valid values are 0x0 to 0x7 (0 ppm to -1800 ppm)
#define CMD_TEMP_COMPENSATION 0x38

// Charge pump multiplication factor
// 1. command  2. params
// Valid factors are 0x0 to 0x2 (5x to 3x),  default is 0x0
#define CMD_CHARGE_PUMP_MUL 0x3D

// Change refresh rate
// 1. command  2. params
// Valid values are 0x0 to 0x3 (80 Hz to 65 Hz)
#define CMD_CHANGE_REFRESH_RATE 0xEF

// Select BIAS ratio
// command | params
// Valid ratios are 0x1 to 0x7 (9 lines to 81 lines)	default is 0x0
#define CMD_SET_BIAS 0x30

// N-line inversion
// 1. command  2. params
// Valid values are 0x00 to 0x1F + XOR_MODE
#define CMD_LINE_INVERT 0xAD
#define XOR_MODE_OFF 0x0
#define XOR_MODE_ON 0x20 

// Number of lines
// command | params
// Valid values are 0x0 to 0x7 (68 lines to 9 lines), 0x0 default
#define CMD_LINES_NUMBER 0xD0

// Image location
// 1. command  2. params
// Valid values are 0x0 to 0x7 (0 lines to 64 lines)
#define CMD_IMAGE_LOCATION 0xAC

// Icon mode
// command | params
#define CMD_SET_ICON_MODE 0xF8
#define ICON_MODE_DISABLE 0x0
#define ICON_MODE_ENABLE 0x1






























