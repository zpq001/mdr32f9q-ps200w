

// state_ADCProcess states
#define STATE_ADC_IDLE				0x00
#define STATE_ADC_DISPATCH			0x01
#define STATE_ADC_NORMAL_START_U	0x02
#define STATE_ADC_NORMAL_REPEAT_U	0x03
#define STATE_ADC_START_I			0x10
#define STATE_ADC_NORMAL_REPEAT_I	0x11

#define ADC_GET_ALL_NORMAL	1

extern xQueueHandle xQueueADC;

void vTaskADC(void *pvParameters);



