

// Defines listed here are not the PIN defines!!!
// PIN defines are in the header file "defines.h"
#define LED_GREEN 1
#define LED_RED	2

#define BUTTON_ESC 			0x01
#define BUTTON_LEFT			0x02
#define BUTTON_RIGHT 		0x04
#define BUTTON_OK				0x08
#define BUTTON_ENCODER 	0x10
#define MODE_SWITCH			0x20
#define BUTTON_ON 			0x40
#define BUTTON_OFF			0x80

extern uint8_t	button_state;			// global buttons state
extern uint8_t	raw_button_state;	// global raw buttons state

// Updates the global button_state variable
void UpdateButtons(void);

