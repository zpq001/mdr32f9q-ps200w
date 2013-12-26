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


#define GUI_TASK_REDRAW				0		// draw
#define GUI_TASK_PROCESS_BUTTONS	1		// buttons
#define GUI_TASK_PROCESS_ENCODER	2		// encoder

#define GUI_TASK_UPDATE_VOLTAGE_CURRENT			0x10	// Refreshes voltage and current indicators
#define GUI_TASK_UPDATE_VOLTAGE_SETTING			0x14	// Refreshes voltage setting
#define GUI_TASK_UPDATE_CURRENT_SETTING			0x15	// Refreshes current setting

#define GUI_TASK_UPDATE_CURRENT_LIMIT			0x13	// Refreshes current limit
#define GUI_TASK_UPDATE_FEEDBACK_CHANNEL		0x11	// Refreshes feedback channel
#define GUI_TASK_UPDATE_TEMPERATURE_INDICATOR	0x12	// Refreshes temperature



// SelectedChannel
#define	GUI_CHANNEL_5V			0x1
#define	GUI_CHANNEL_12V			0x0
// CurrentLimit
#define	GUI_CURRENT_LIM_HIGH	0x1
#define	GUI_CURRENT_LIM_LOW	 	0x0



extern xQueueHandle xQueueGUI;
void vTaskGUI(void *pvParameters);

void applyGuiVoltageSetting(uint16_t new_set_voltage);
void applyGuiCurrentSetting(uint16_t new_set_current);
void applyGuiCurrentLimit(uint8_t new_current_limit);
