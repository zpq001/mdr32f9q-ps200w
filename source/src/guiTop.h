#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "guiCore.h"

// Key events
#define BTN_EVENT_DOWN		GUI_KEY_EVENT_DOWN
#define BTN_EVENT_UP		GUI_KEY_EVENT_UP
#define BTN_EVENT_UP_SHORT  GUI_KEY_EVENT_UP_SHORT
#define BTN_EVENT_UP_LONG   GUI_KEY_EVENT_UP_LONG
#define BTN_EVENT_HOLD      GUI_KEY_EVENT_HOLD
#define BTN_EVENT_REPEAT    GUI_KEY_EVENT_REPEAT

#pragma anon_unions


typedef struct {
	uint32_t type;
	union {
        struct {
            uint32_t a;
        } data;
		//----- key driver ----//
		struct {
			uint16_t event;
			uint16_t code;
		} key_event;
		struct {
            int16_t delta;
        } encoder_event;
		//----- converter -----//
		struct {
			uint8_t spec;
			uint8_t channel;
			uint8_t type;
			uint8_t current_range;
		} converter_event;
	};
} gui_msg_t;


enum guiTaskCmd {
	GUI_TASK_REDRAW,
	GUI_TASK_PROCESS_BUTTONS,
	GUI_TASK_PROCESS_ENCODER,
	GUI_TASK_RESTORE_ALL,
	GUI_TASK_EEPROM_STATE,
	
	GUI_TASK_UPDATE_CONVERTER_STATE,
	
	GUI_TASK_UPDATE_VOLTAGE_CURRENT,
	GUI_TASK_UPDATE_VOLTAGE_SETTING,
	GUI_TASK_UPDATE_CURRENT_SETTING,	
	GUI_TASK_UPDATE_CURRENT_LIMIT,
	GUI_TASK_UPDATE_FEEDBACK_CHANNEL,
	GUI_TASK_UPDATE_TEMPERATURE_INDICATOR,
	GUI_TASK_UPDATE_SOFT_LIMIT_SETTINGS,
};



#define	VOLTAGE_SETTING_CHANGED			(1<<0)
#define CURRENT_SETTING_CHANGED			(1<<1)
#define VOLTAGE_LIMIT_CHANGED			(1<<2)
#define CURRENT_LIMIT_CHANGED			(1<<3)
#define CURRENT_RANGE_CHANGED			(1<<4)
#define CHANNEL_CHANGED					(1<<5)
#define OVERLOAD_SETTING_CHANGED		(1<<6)



// SelectedChannel
#define	GUI_CHANNEL_5V				0x1		// Actualy a copy of definitions in common.h
#define	GUI_CHANNEL_12V				0x0		// CHECKME - possibly gather all definitions that are common to hardware and GUI into separate file?
// CurrentLimit
#define	GUI_CURRENT_RANGE_HIGH		0x1
#define	GUI_CURRENT_RANGE_LOW	 	0x0
// Software limit types
#define GUI_LIMIT_TYPE_LOW			0x00
#define GUI_LIMIT_TYPE_HIGH			0x01
	


void GUI_Init(void);

extern xQueueHandle xQueueGUI;
void vTaskGUI(void *pvParameters);

uint16_t getVoltageSetting(uint8_t channel);
uint16_t getVoltageAbsMax(uint8_t channel);
uint16_t getVoltageAbsMin(uint8_t channel);
uint16_t getVoltageLimitSetting(uint8_t channel, uint8_t limit_type);
uint8_t getVoltageLimitState(uint8_t channel, uint8_t limit_type);
uint16_t getCurrentSetting(uint8_t channel, uint8_t range);
uint16_t getCurrentAbsMax(uint8_t channel, uint8_t range);
uint16_t getCurrentAbsMin(uint8_t channel, uint8_t range);
uint16_t getCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type);
uint8_t getCurrentLimitState(uint8_t channel, uint8_t range, uint8_t limit_type);
uint8_t getOverloadProtectionState(void);
uint8_t getOverloadProtectionWarning(void);
uint16_t getOverloadProtectionThreshold(void);
uint8_t getCurrentRange(uint8_t channel);



void applyGuiVoltageSetting(uint8_t channel, int32_t new_set_voltage);
void applyGuiVoltageLimit(uint8_t channel, uint8_t type, uint8_t enable, int32_t value);
void applyGuiCurrentSetting(uint8_t channel, uint8_t range, int32_t new_set_current);
void applyGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t type, uint8_t enable, int32_t value);
void applyGuiCurrentRange(uint8_t channel, uint8_t new_range);
void applyGuiOverloadSetting(uint8_t protection_enable, uint8_t warning_enable, int32_t threshold);

void updateGuiVoltageLimit(uint8_t channel, uint8_t type);
void updateGuiCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t type);
void updateGuiOverloadSetting(void);



