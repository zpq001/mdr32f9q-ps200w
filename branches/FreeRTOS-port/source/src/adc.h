
#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"


// state_ADCProcess states
#define STATE_ADC_IDLE				0x00
#define STATE_ADC_DISPATCH			0x01
#define STATE_ADC_NORMAL_START_U	0x02
#define STATE_ADC_NORMAL_REPEAT_U	0x03
#define STATE_ADC_START_I			0x10
#define STATE_ADC_NORMAL_REPEAT_I	0x11

#define ADC_GET_ALL_NORMAL	1

extern xQueueHandle xQueueADC;


uint16_t ADC_GetVoltage(void);
uint16_t ADC_GetCurrent(void);
uint32_t ADC_GetPower(void);

void vTaskADC(void *pvParameters);
void Converter_HW_ADCProcess(void);


