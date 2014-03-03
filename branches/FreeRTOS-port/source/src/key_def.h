


// Key codes
#define BTN_ESC			0x01
#define BTN_LEFT		0x02
#define BTN_RIGHT		0x04
#define BTN_OK			0x08
#define BTN_ENCODER		0x10
#define SW_CHANNEL		0x20
#define BTN_ON			0x40
#define BTN_OFF			0x80
#define SW_EXTERNAL		0x0100


// Key events
#define BTN_EVENT_DOWN		1
#define BTN_EVENT_UP		2
#define BTN_EVENT_UP_SHORT  3
#define BTN_EVENT_UP_LONG   4
#define BTN_EVENT_HOLD      5
#define BTN_EVENT_REPEAT    6



//-------------------------------------------------------//
// Key events for GUI

// Switch to redefine GUI default key codes and events
#define USE_CUSTOM_KEYS		

// Key event specifications
#define GUI_KEY_EVENT_DOWN      BTN_EVENT_DOWN
#define GUI_KEY_EVENT_UP        BTN_EVENT_UP
#define GUI_KEY_EVENT_UP_SHORT  BTN_EVENT_UP_SHORT
#define GUI_KEY_EVENT_UP_LONG   BTN_EVENT_UP_LONG
#define GUI_KEY_EVENT_HOLD      BTN_EVENT_HOLD
#define GUI_KEY_EVENT_REPEAT    BTN_EVENT_REPEAT
#define GUI_ENCODER_EVENT		7

// Button codes
#define GUI_KEY_ESC     	BTN_ESC
#define GUI_KEY_OK      	BTN_OK
#define GUI_KEY_LEFT    	BTN_LEFT
#define GUI_KEY_RIGHT   	BTN_RIGHT
#define GUI_KEY_UP      	0
#define GUI_KEY_DOWN    	0
#define GUI_KEY_ENCODER 	BTN_ENCODER

//-------------------------------------------------------//




