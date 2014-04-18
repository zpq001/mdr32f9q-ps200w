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
		//----- profiles ------//
		struct {
			uint8_t event;
			uint8_t err_code;
			uint8_t index;
		} profile_event;
		struct {
			uint8_t profileState;
		} profile_name_response;
	};
} gui_msg_t;


enum guiTaskCmd {
	GUI_TASK_REDRAW,
	GUI_TASK_PROCESS_BUTTONS,
	GUI_TASK_PROCESS_ENCODER,
	GUI_TASK_RESTORE_ALL,
	GUI_TASK_EEPROM_STATE,
	
	GUI_TASK_UPDATE_CONVERTER_STATE,
	
	
	GUI_TASK_PROFILE_EVENT,
	GUI_TASK_UPDATE_PROFILE_SETUP,
		
	GUI_TASK_UPDATE_VOLTAGE_CURRENT,
	GUI_TASK_UPDATE_VOLTAGE_SETTING,
	GUI_TASK_UPDATE_CURRENT_SETTING,	
	GUI_TASK_UPDATE_CURRENT_LIMIT,
	GUI_TASK_UPDATE_FEEDBACK_CHANNEL,
	GUI_TASK_UPDATE_TEMPERATURE_INDICATOR,
	GUI_TASK_UPDATE_SOFT_LIMIT_SETTINGS,
};


enum profileEvents {
	PROFILE_LOAD,
	PROFILE_SAVE,
	PROFILE_LOAD_RECENT
};




void GUI_Init(void);

extern xQueueHandle xQueueGUI;
extern const gui_msg_t gui_msg_redraw;
void vTaskGUI(void *pvParameters);




void applyGuiCurrentRange(uint8_t channel, uint8_t new_range);


// Indicators
void guiTop_UpdateGuiVoltageIndicator(void);
void guiTop_UpdateCurrentIndicator(void);
void guiTop_UpdatePowerIndicator(void);
void guiTop_UpdateTemperatureIndicator(void);
// Voltage setting
void guiTop_ApplyGuiVoltageSetting(uint8_t channel, int16_t new_set_voltage);
void guiTop_UpdateVoltageSetting(uint8_t channel);
// Current setting
void guiTop_ApplyCurrentSetting(uint8_t channel, uint8_t currentRange, int16_t new_set_current);
void guiTop_UpdateCurrentSetting(uint8_t channel, uint8_t currentRange);
// Limits
void guiTop_ApplyGuiVoltageLimit(uint8_t channel, uint8_t limit_type, uint8_t enable, int16_t value);
void guiTop_ApplyGuiCurrentLimit(uint8_t channel, uint8_t currentRange, uint8_t limit_type, uint8_t enable, int16_t value);
void guiTop_UpdateVoltageLimit(uint8_t channel, uint8_t limit_type);
void guiTop_UpdateCurrentLimit(uint8_t channel, uint8_t range, uint8_t limit_type);
// Current range
void guiTop_ApplyCurrentRange(uint8_t channel, uint8_t new_current_range);
void guiTop_UpdateCurrentRange(uint8_t channel);
// Overload
void guiTop_ApplyGuiOverloadSettings(uint8_t protection_enable, uint8_t warning_enable, int32_t threshold);
void guiTop_UpdateOverloadSettings(void);
// Profiles
void guiTop_ApplyGuiProfileSettings(uint8_t saveRecentProfile, uint8_t restoreRecentProfile);
void guiTop_UpdateProfileSettings(void);
void guiTop_LoadProfile(uint8_t index);
void guiTop_SaveProfile(uint8_t index, char *profileName);
// Extrnal switch
void guiTop_ApplyExtSwitchSettings(uint8_t enable, uint8_t inverse, uint8_t mode);
void guiTop_UpdateExtSwitchSettings(void);
// DAC offset
void guiTop_ApplyDacSettings(int8_t v_offset, int8_t c_low_offset, int8_t c_high_offset);
void guiTop_UpdateDacSettings(void);





