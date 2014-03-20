#ifndef __GUI_CONFIG_H_
#define __GUI_CONFIG_H_

// Custom key events
#include "key_def.h"

#define GUI_CORE_QUEUE_SIZE 20

#define GUI_CFG_USE_TIMERS
#define GUI_TIMER_COUNT 1
#define GUI_MESSAGE_PANEL_TIMER 0   // timer's name

#define CFG_USE_UPDATE

#define USE_Z_ORDER_REDRAW

//#define ALWAYS_PASS_TOUCH_TO_FOCUSED

//#define USE_TOUCH_SUPPORT


// Design-specific events
#define GUI_EVENT_START  0xF0
#define GUI_EVENT_EEPROM_MESSAGE    0xF1

#endif
