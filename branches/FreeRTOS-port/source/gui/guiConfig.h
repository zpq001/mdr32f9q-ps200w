#ifndef __GUI_CONFIG_H_
#define __GUI_CONFIG_H_

#include <stdint.h>

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


#define emGUI_HEAP_SIZE             2000        // bytes
#define emGUI_BYTE_ALIGNMENT        4           // bytes
#define emGUI_POINTER_SIZE_TYPE     uint32_t



// Design-specific events
#define GUI_EVENT_START  0xF0
#define GUI_EVENT_EEPROM_MESSAGE    0xF1
#define GUI_SYSTEM_EVENT            0xF2

// System events
enum {
    //GUI_UPDATE_INITIAL = 0x01,
    GUI_UPDATE_VOLTAGE_SETTING = 0x01,
    GUI_UPDATE_CURRENT_SETTING,
    GUI_UPDATE_VOLTAGE_LIMIT,
    GUI_UPDATE_CURRENT_LIMIT,
    GUI_UPDATE_OVERLOAD_SETTINGS,
    GUI_UPDATE_PROFILE_SETTINGS,
    GUI_UPDATE_EXTSWITCH_SETTINGS,
    GUI_UPDATE_DAC_SETTINGS,
    GUI_UPDATE_CURRENT_RANGE,
    GUI_UPDATE_CHANNEL,
    GUI_UPDATE_ADC_INDICATORS,
    GUI_UPDATE_TEMPERATURE_INDICATOR,
    GUI_UPDATE_PROFILE_LIST,
    GUI_UPDATE_PROFILE_LIST_RECORD
};


typedef struct {
    //uint8_t type;
    uint8_t code;
    uint8_t local_request :1;
    uint8_t channel :1;
    uint8_t upd_low_limit :1;
    uint8_t upd_high_limit :1;
    uint8_t current_range :1;
    uint8_t index :5;
} system_event_t;


typedef struct {
    uint8_t type;
    uint8_t spec;
    system_event_t payload;
} my_custom_event_t;




#endif
