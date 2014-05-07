
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


enum ButtonsTaskMsgTypes {	
	BUTTONS_TICK = 1, 
	BUTTONS_EXTSWITCH_SETTINGS,
	BUTTONS_LOAD_PROFILE,
	BUTTONS_SAVE_PROFILE,
	BUTTONS_START_OPERATION,
	BUTTONS_ENABLE_ON_OFF_CONTROL
};

typedef struct {
	uint8_t type;
	xSemaphoreHandle *pxSemaphore;
	union {
		struct {
			uint8_t enable;
			uint8_t inverse;
			uint8_t mode;
		} extSwitchSetting;
	};
} buttons_msg_t;

extern xQueueHandle xQueueButtons;
extern const buttons_msg_t buttons_tick_msg;

uint8_t BTN_IsExtSwitchEnabled(void);
uint8_t BTN_GetExtSwitchInversion(void);
uint8_t BTN_GetExtSwitchMode(void);

void vTaskButtons(void *pvParameters);


