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



typedef struct {
	uint32_t type;
	uint32_t data;
} gui_incoming_msg_t;


enum guiTaskCmd {
	GUI_TASK_REDRAW,
	GUI_TASK_PROCESS_BUTTONS,
	GUI_TASK_PROCESS_ENCODER,
	GUI_TASK_RESTORE_ALL,
	GUI_TASK_EEPROM_STATE,
	
	GUI_TASK_UPDATE_VOLTAGE_CURRENT,
	GUI_TASK_UPDATE_VOLTAGE_SETTING,
	GUI_TASK_UPDATE_CURRENT_SETTING,	
	GUI_TASK_UPDATE_CURRENT_LIMIT,
	GUI_TASK_UPDATE_FEEDBACK_CHANNEL,
	GUI_TASK_UPDATE_TEMPERATURE_INDICATOR,
	GUI_TASK_UPDATE_SOFT_LIMIT_SETTINGS
};

/*
#define GUI_TASK_REDRAW				0		// draw
#define GUI_TASK_PROCESS_BUTTONS	1		// buttons
#define GUI_TASK_PROCESS_ENCODER	2		// encoder
#define GUI_TASK_RESTORE_ALL		3		// read data from settings and converter structures and update widgets
#define GUI_TASK_EEPROM_STATE		4

#define GUI_TASK_UPDATE_VOLTAGE_CURRENT			0x10	// Refreshes voltage and current indicators
#define GUI_TASK_UPDATE_VOLTAGE_SETTING			0x14	// Refreshes voltage setting
#define GUI_TASK_UPDATE_CURRENT_SETTING			0x15	// Refreshes current setting

#define GUI_TASK_UPDATE_CURRENT_LIMIT			0x13	// Refreshes current limit
#define GUI_TASK_UPDATE_FEEDBACK_CHANNEL		0x11	// Refreshes feedback channel
#define GUI_TASK_UPDATE_TEMPERATURE_INDICATOR	0x12	// Refreshes temperature

#define GUI_TASK_UPDATE_SOFT_LIMIT_SETTINGS		0x16	// Refreshes all voltage and current software limit settings
*/

// SelectedChannel
#define	GUI_CHANNEL_5V				0x1		// Actualy a copy of definitions in common.h
#define	GUI_CHANNEL_12V				0x0		// CHECKME - possibly gather all definitions that are common to hardware and GUI into separate file?
// CurrentLimit
#define	GUI_CURRENT_RANGE_HIGH		0x1
#define	GUI_CURRENT_RANGE_LOW	 	0x0



void GUI_Init(void);

extern xQueueHandle xQueueGUI;
void vTaskGUI(void *pvParameters);

void applyGuiVoltageSetting(uint16_t new_set_voltage);
void applyGuiVoltageLimit(uint8_t type, uint8_t enable, uint16_t value);
void applyGuiCurrentSetting(uint16_t new_set_current);

void applyGuiCurrentRange(uint8_t new_range);



