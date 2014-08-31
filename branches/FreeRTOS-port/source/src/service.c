
#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"
#include "MDR32F9Qx_adc.h"

#include "FreeRTOS.h"
#include "task.h"

#include "defines.h"
#include "systemfunc.h"
#include "service.h"

#include "guiTop.h"

int16_t converter_temp_celsius = 0;

static uint32_t gui_msg;

int16_t Service_GetTemperature(void)
{
	return converter_temp_celsius;
}

void vTaskService(void *pvParameters) 
{
	uint32_t i;
	uint16_t cooler_speed;
	uint16_t adc_samples;
	
	// Initialize
	portTickType lastExecutionTime = xTaskGetTickCount();
	ADC2_SetChannel(ADC_CHANNEL_CONVERTER_TEMP); 
	
	while(1)
	{
		vTaskDelayUntil(&lastExecutionTime, ((portTickType)500 / portTICK_RATE_MS));
		
		// Get temparature
		adc_samples = 0;
		for (i=0; i<5; i++)
		{
			ADC2_Start();
			vTaskDelay(2);
			// Make sure conversion is done
			while( ADC_GetFlagStatus(ADC2_FLAG_END_OF_CONVERSION)==RESET );
			adc_samples += ADC2_GetResult();
		}
		converter_temp_celsius = (int16_t)( (float)adc_samples*0.2*(-0.226) + 230 );	//FIXME
		
		// Update cooler speed
		cooler_speed = (converter_temp_celsius < 25) ? 50 : converter_temp_celsius * 2;
		SetCoolerSpeed(cooler_speed);
		
		// Send notification to GUI
		gui_msg = GUI_TASK_UPDATE_TEMPERATURE_INDICATOR;
		xQueueSendToBack(xQueueGUI, &gui_msg, 0);
	}
	
}

