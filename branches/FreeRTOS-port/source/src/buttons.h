/**********************************************************
	Module for processing buttons and switches
	using bit FIFOs
	
	Type: Header file
	Written by: AvegaWanderer 10.2013
**********************************************************/


// User definitions
//#define BTN_ESC			0x01
//#define BTN_LEFT		0x02
//#define BTN_RIGHT		0x04
//#define BTN_OK			0x08
//#define BTN_ENCODER		0x10
//#define SW_CHANNEL		0x20
//#define BTN_ON			0x40
//#define BTN_OFF			0x80
//#define SW_EXTERNAL		0x0100
#include "key_def.h"

#define BTN_JTAG_MASK	(BTN_ESC | BTN_LEFT | BTN_RIGHT | BTN_OK)
#define NOT_USE_JTAG_BUTTONS


//=============================================//
// Button processing setup

// Total count
#define TOTAL_BUTTONS	9

// Select minimal data type which can hold all your buttons (one bit per button or switch)
#define btn_type_t		uint16_t		
// Select minimal data type for bit FIFO to support desired delays (see settings check below)
#define bfifo_type_t	uint16_t		

// Choose which actions are supported by button processor - comment unused to optimize code size
//#define USE_ACTION_REP				// Emulation of repeated pressing
#define USE_ACTION_UP				// Triggers when button is released
#define USE_ACTION_UP_SHORT			// Triggers when button is released and had been pressed for short time
//#define USE_ACTION_UP_LONG			// Triggers when button is released and had been pressed for long time
#define USE_ACTION_HOLD				// Triggers when button is being pressed for long time
//#define USE_ACTION_TOGGLE			// Toggles every time button is pressed


// Set time options - all delays are in units of ProcessButtons() call period.
#define LONG_PRESS_DELAY	10		// After this delay button actions will be considered as long
#define REPEAT_DELAY		6		// After this delay emulation of repeated pressing becomes active

// Set inversion of raw_button_state
#define RAW_BUTTON_INVERSE_MASK		0x00
//=============================================// 

// Bit FIFO definitions
#define BFIFO_LONG_MASK		( ((1 << (LONG_PRESS_DELAY + 1)) - 1) & ~0x3 )
#define BFIFO_REPEAT_MASK	( ((1 << REPEAT_DELAY) - 1) & ~0x3 )
#define BFIFO_HOLD			( ((1 << LONG_PRESS_DELAY) - 1) & ~0x3 )
#define BFIFO_BUTTON_MASK	0x3
#define BFIFO_BUTTON_DOWN	0x1
#define BFIFO_BUTTON_UP		0x2
#define BFIFO_BUTTON_HOLD	0x3

// Check settings
/*
#if ((LONG_PRESS_DELAY + 1) > (sizeof(bfifo_type_t) * 8))
	#error "LONG_PRESS_DELAY is too great for specified bfifo_type_t size. LONG_PRESS_DELAY should not exceed (number of bits in bfifo_type_t - 1)"
#endif
#if (REPEAT_DELAY > (sizeof(bfifo_type_t) * 8))
	#error "REPEAT_DELAY is too great for specified bfifo_type_t size. REPEAT_DELAY should not exceed number of bits in bfifo_type_t"
#endif
#if (TOTAL_BUTTONS > (sizeof(btn_type_t) * 8))
	#error "Buttons count specified exceeds number of bits in btn_type_t. TOTAL_BUTTONS should be less or equal to number of bits in btn_type_t"
#endif
*/
#if (LONG_PRESS_DELAY < 3)
	#error "LONG_PRESS_DELAY should be 3 or greater"
#endif
#if (REPEAT_DELAY < 3)
	#warning "REPEAT_DELAY should be 3 or greater"
#endif

// Extra defines used for cleaning unnecessary code - do not modify
#ifdef USE_ACTION_REP
	#define ANALYSE_HOLD
#endif
#ifdef USE_ACTION_UP
	#define ANALYSE_ACTION_UP
#endif
#ifdef USE_ACTION_UP_SHORT
	#define ANALYSE_ACTION_UP
	#define ANALYZE_TIMED_ACTION_UP
#endif
#ifdef USE_ACTION_UP_LONG
	#define ANALYSE_ACTION_UP
	#define ANALYZE_TIMED_ACTION_UP
#endif
#ifdef USE_ACTION_HOLD
	#define ANALYSE_HOLD
#endif



// Button state structure
typedef struct {
	btn_type_t raw_state;		// Non-processed button state (only RAW_BUTTON_INVERSE_MASK is applied)
	btn_type_t action_down;		// Bit is set once when button is pressed
	#ifdef USE_ACTION_REP
	btn_type_t action_rep;		// Bit is set once when button is pressed. Bit is set continuously after REPEAT_DELAY until button is released
	#endif
	#ifdef USE_ACTION_UP
	btn_type_t action_up;		// Bit is set once when button is released
	#endif
	#ifdef USE_ACTION_UP_SHORT
	btn_type_t action_up_short;	// Bit is set once when button is released before LONG_PRESS_DELAY
	#endif
	#ifdef USE_ACTION_UP_LONG
	btn_type_t action_up_long;	// Bit is set once when button is released after LONG_PRESS_DELAY
	#endif
	#ifdef USE_ACTION_HOLD
	btn_type_t action_hold;		// Bit is set once when button is pressed for LONG_PRESS_DELAY
	#endif
	#ifdef USE_ACTION_TOGGLE
	btn_type_t action_toggle;	// Bit is set every time button is pressed
	#endif
} buttons_t;


//=============================================//
// This raw_button_state represents sampled state of buttons being processed - input data for ProcessButtons()
// It is assumed that if some button is pressed, corresponding bit is set - this is true for all buttons.
// If your raw_button_state is inversed, set RAW_BUTTON_INVERSE_MASK to all ones
//extern btn_type_t raw_button_state;		

// Processed output
extern buttons_t buttons;


//=============================================//
void InitButtons(void);
void ProcessButtons(void);



