#include <stdint.h>


enum ExtSwModes {EXTSW_DIRECT, EXTSW_TOGGLE, EXTSW_TOGGLE_OFF};

typedef struct {
	uint8_t enabled;
	uint8_t inv;
	uint8_t mode;
} extsw_mode_t;



void vTaskButtons(void *pvParameters);


