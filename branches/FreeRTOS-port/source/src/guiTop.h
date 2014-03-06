#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "global_def.h"


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





void GUI_Init(void);

extern xQueueHandle xQueueGUI;
void vTaskGUI(void *pvParameters);


void applyGuiVoltageSetting(uint8_t channel, int32_t new_set_voltage);
void applyGuiVoltageLimit(uint8_t channel, uint8_t type, uint8_t enable, int32_t value);
void applyGuiCurrentSetting(uint8_t channel, uint8_t range, int32_t new_set_current);
void applyGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t type, uint8_t enable, int32_t value);
void applyGuiCurrentRange(uint8_t channel, uint8_t new_range);
void applyGuiOverloadSetting(uint8_t protection_enable, uint8_t warning_enable, int32_t threshold);




