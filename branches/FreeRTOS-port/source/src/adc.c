#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"
#include "MDR32F9Qx_adc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "defines.h"
#include "converter.h"
#include "converter_hw.h"
#include "control.h"
#include "adc.h"

#include "dispatcher.h"


// ctrl_ADCProcess bits
// For ADC low-level routine
#define CMD_ADC_START_VOLTAGE		0x01
#define CMD_ADC_START_CURRENT		0x02
#define CMD_ADC_START_DISCON		0x04



uint16_t voltage_adc;		// [mV]
uint16_t current_adc;		// [mA]
uint32_t power_adc;			// [mW]

static uint16_t adc_voltage_counts;	// [ADC counts]
static uint16_t adc_current_counts;	// [ADC counts]

static volatile uint8_t ctrl_ADCProcess = 0;

static dispatch_msg_t dispatcher_msg;

xQueueHandle xQueueADC;
xSemaphoreHandle xSemaphoreADC;


uint16_t ADC_GetVoltage(void)
{
	return voltage_adc;
}

uint16_t ADC_GetCurrent(void)
{
	return current_adc;
}

uint32_t ADC_GetPower(void)
{
	return power_adc;
}



void vTaskADC(void *pvParameters) 
{
	uint32_t msg;
	
	// Initialize
	xQueueADC = xQueueCreate( 5, sizeof( uint32_t ) );		// Queue can contain 5 elements of type uint32_t
	if( xQueueADC == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	vSemaphoreCreateBinary( xSemaphoreADC );
	if( xSemaphoreADC == 0 )
    {
        while(1);
    }
	
	while(1)
	{
		xQueueReceive(xQueueADC, &msg, portMAX_DELAY);
		switch (msg)
		{
			case ADC_GET_ALL_NORMAL:
				// Send command to low-level task - must be atomic operations
				ctrl_ADCProcess = CMD_ADC_START_VOLTAGE | CMD_ADC_START_CURRENT;
				xSemaphoreTake(xSemaphoreADC, portMAX_DELAY);
			
				// Process adc samples
				voltage_adc = adc_voltage_counts>>2;
				voltage_adc *= 5;
				
				current_adc = adc_current_counts>>2;
				if (converter_state.channel->current->RANGE == CURRENT_RANGE_HIGH)
					current_adc *= 10;
				else
					current_adc *= 5;
				
				power_adc = voltage_adc * current_adc / 1000;
				
				// Send notification
				dispatcher_msg.type = DISPATCHER_NEW_ADC_DATA;
				dispatcher_msg.sender = sender_ADC;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
			
				break;
			
			
		}
		
	}
	
}





//---------------------------------------------//
//	Converter hardware ADC control task
//	
//	Low-level task for ADC start, normal or charging mode
//	
// TODO: ensure that ISR is non-interruptable
//---------------------------------------------//
void Converter_HW_ADCProcess(void)
{
	static uint8_t state_ADCProcess = STATE_ADC_IDLE;
	static uint8_t adc_cmd;
	static uint8_t adc_counter;
	
	
	// Process ADC FSM-based controller
	switch (state_ADCProcess)
	{
		case STATE_ADC_IDLE:
			adc_cmd = ctrl_ADCProcess;
			if (adc_cmd)
				state_ADCProcess = STATE_ADC_DISPATCH;
			break;
		case STATE_ADC_DISPATCH:
			if ((adc_cmd & (CMD_ADC_START_VOLTAGE | CMD_ADC_START_DISCON)) == CMD_ADC_START_VOLTAGE)
			{
				// Normal voltage measure 
				adc_cmd &= ~(CMD_ADC_START_VOLTAGE | CMD_ADC_START_DISCON);
				state_ADCProcess = STATE_ADC_NORMAL_START_U;
				adc_voltage_counts = 0;
			}
			else if (adc_cmd & CMD_ADC_START_CURRENT)
			{
				// Current measure
				adc_cmd &= ~CMD_ADC_START_CURRENT;
				state_ADCProcess = STATE_ADC_START_I;
				adc_current_counts = 0;
			}
			else
			{
				state_ADCProcess = STATE_ADC_IDLE;
				ctrl_ADCProcess = 0;						// Can be used as flag
				xSemaphoreGiveFromISR( xSemaphoreADC, 0 );	// We dont care for exact timing of ADC task
			}
			break;
		case STATE_ADC_NORMAL_START_U:
			// TODO: use DMA for this purpose
			ADC1_SetChannel(ADC_CHANNEL_VOLTAGE);
			adc_counter = 4;
			state_ADCProcess = STATE_ADC_NORMAL_REPEAT_U;
			break;
		case STATE_ADC_NORMAL_REPEAT_U:
			if (adc_counter < 4)
				adc_voltage_counts += ADC1_GetResult();
			if (adc_counter != 0)
			{
				ADC1_Start();
				adc_counter--;
			}
			else
			{
				state_ADCProcess = STATE_ADC_DISPATCH;
			}
			break;
		case STATE_ADC_START_I:
			// TODO: use DMA for this purpose
			ADC1_SetChannel(ADC_CHANNEL_CURRENT);
			adc_counter = 4;
			state_ADCProcess = STATE_ADC_NORMAL_REPEAT_I;
			break;
		case STATE_ADC_NORMAL_REPEAT_I:
			if (adc_counter < 4)
				adc_current_counts += ADC1_GetResult();
			ADC1_Start();
			if (adc_counter != 0)
			{
				ADC1_Start();
				adc_counter--;
			}
			else
			{
				state_ADCProcess = STATE_ADC_DISPATCH;
			}
			break;
		default:
			state_ADCProcess = STATE_ADC_IDLE;
			break;
	}
	
	
	
	/*
		//...
		
		// Disable converter for ADC
		cmd_ADC_to_HWProcess = STATE_HW_OFF_BY_ADC;			// Will disallow converter operation
		
		//...
		
		// Enable converter for ADC
		cmd_ADC_to_HWProcess = CMD_HW_ON_BY_ADC;			// Will allow converter operation
	*/
	

}







