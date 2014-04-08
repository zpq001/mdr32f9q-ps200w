#include <stdint.h>



typedef struct {
	uint8_t enabled;
	uint8_t inv;
	uint8_t mode;
} extsw_mode_t;



enum ButtonsTaskMsgTypes {	
	BUTTONS_TICK = 1, 
	BUTTONS_EXTSWITCH_SETTINGS
};

typedef struct {
	uint8_t type;
	union {
		struct {
			uint8_t enabled;
			uint8_t inversed;
			uint8_t mode;
		} extSwitchSetting;
	};
} buttons_msg_t;





uint8_t BTN_IsExtSwitchEnabled(void);
uint8_t BTN_GetExtSwitchInversion(void);
uint8_t BTN_GetExtSwitchMode(void);

void vTaskButtons(void *pvParameters);


