
#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"
#include "MDR32F9Qx_ssp.h"
#include "MDR32F9Qx_port.h"
#include "MDR32F9Qx_timer.h"
#include "MDR32F9Qx_adc.h"

// Подключаем FreeRTOS
#include "FreeRTOS.h"
// из всех возможностей используем пока только переключение задач
#include "task.h"

//#include "gui_top.h"
#include "guiTop.h"

#include "stdio.h"

#include "defines.h"
#include "systemfunc.h"
#include "systick.h"
#include "lcd_1202.h"





#include "buttons.h"
#include "led.h"
#include "encoder.h"
#include "i2c_eeprom.h"
#include "eeprom.h"
#include "dwt_delay.h"
#include "control.h"

#include "converter.h"
#include "service.h"
#include "dispatcher.h"
#include "adc.h"
#include "uart_rx.h"
#include "uart_tx.h"

#include "buttons_top.h"

#include "sound_driver.h"



#if 0
//==============================================================//
// EEPROM simple test
//==============================================================//
uint8_t EEPROM_test(void)
{
	
	  //uint8_t eeprom_data_wr[] = {0x55, 0x66, 0x77, 0x88};
	  //uint16_t wait_cnt = 0;
		uint8_t eeprom_data_rd[4];
		uint8_t error = 0;
		
	/*
		error = EEPROMReady();
		error = EEPROMWriteBlock(0x0000,eeprom_data_wr,4);
		while (!EEPROMReady())
		{
			wait_cnt++;
			//DWTDelayUs(10);
		}
	  error = EEPROMWriteBlock(0x07FC,eeprom_data_wr,4);
  */
	
		error = EEPROMReadBlock(0x0000,eeprom_data_rd,4);
	  if (*((uint32_t*)eeprom_data_rd) != 0x88776655)
			error |= 0x01;
		
	  error = EEPROMReadBlock(0x07FC,eeprom_data_rd,4);
	  if (*((uint32_t*)eeprom_data_rd) != 0x44332211)
			error |= 0x02;
		
		return error;
}
#endif




/*
void vTaskLED1(void *pvParameters) {

		portTickType lastExecutionTime = xTaskGetTickCount();
        for (;;) {
                PORT_ResetBits(MDR_PORTB, 1<<LGREEN);
				vTaskDelayUntil(&lastExecutionTime, ((portTickType)500 / portTICK_RATE_MS));
			
                PORT_SetBits(MDR_PORTB, 1<<LGREEN);
                vTaskDelayUntil(&lastExecutionTime, ((portTickType)500 / portTICK_RATE_MS));
        }

}


void vTaskLED2(void *pvParameters) {

		portTickType lastExecutionTime = xTaskGetTickCount();
        for (;;) {
                PORT_ResetBits(MDR_PORTB, 1<<LRED);
                vTaskDelayUntil(&lastExecutionTime, ((portTickType)300 / portTICK_RATE_MS));
			
                PORT_SetBits(MDR_PORTB, 1<<LRED);
                vTaskDelayUntil(&lastExecutionTime, ((portTickType)300 / portTICK_RATE_MS));
        }

}
*/


/*****************************************************************************
*		Main function
*

	Start-up sequence:
	
		First, all hardware setup is made. It includes clocks, interrupt controller, 
		ADC, DMA and other peripheral blocks.
		After that, some low-level initialization functions are called, then
		interrupts are enabled. From this moment, low-level interrupt-driven 
		systems are working.
		After that, system settings are restored (from serial EEPROM or other source).
		Now tasks are created and sheduler is started. Some tasks start immediately, 
		while others wait for specific initialization messages. 
		Dispatcher task takes care ot that.

******************************************************************************/
int main(void)
{


	//=============================================//
	// System initialization
	//=============================================//
	Setup_CPU_Clock();
	HW_NVIC_init();
	DWT_Init();
	HW_SSPInit();
	HW_I2CInit();
	HW_TimersInit();
	HW_PortInit();
	HW_ADCInit();
	HW_UARTInit();
	HW_DMAInit();
	
	// Other initialization is done by tasks.
	InitButtons();
	LcdInit();

	// Some modules can use eeprom data structures directly.
	// Those structures must be initialized.
	EE_GetReadyForSystemInit();
	
	// Enable interrupt for fast low-level control dispatcher
	// TODO: bring ISR from systick.c to systemfunc.c
	NVIC_EnableIRQ(Timer2_IRQn);

	// Init converter
	Converter_Init();
	
	SetCoolerSpeed(80);
	LcdSetBacklight(85);
	
	// Reset time statistics
	time_profile.max_ticks_in_Systick_hook = 0;
	time_profile.max_ticks_in_Timer2_ISR = 0;
	
	
	xTaskCreate( vTaskGUI, 			( signed char * ) 		"GUI top", 		configMINIMAL_STACK_SIZE, 	NULL, 1, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskConverter, 	( signed char * ) 		"Converter", 	configMINIMAL_STACK_SIZE, 	NULL, 2, &xTaskHandle_Converter);
	xTaskCreate( vTaskService, 		( signed char * ) 		"Service", 		configMINIMAL_STACK_SIZE, 	NULL, 0, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskDispatcher, 	( signed char * ) 		"Dispatcher", 	configMINIMAL_STACK_SIZE, 	NULL, 3, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskButtons, 		( signed char * ) 		"buttons top", 	configMINIMAL_STACK_SIZE, 	NULL, 1, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskADC, 			( signed char * ) 		"ADC", 			configMINIMAL_STACK_SIZE, 	NULL, 2, ( xTaskHandle * ) NULL);
	
	// Transmitter task priority should be > receiver's due to unknown error which invokes hard fault handler with INVPC error
	// Error was caused by drawing outside of LCD buffer. Fixed.
	xTaskCreate( vTaskUARTReceiver, 	( signed char * ) 	"UART1 RX", 		256, 				(void *)1, 	1, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskUARTTransmitter, 	( signed char * ) 	"UART1 TX", 		256, 				(void *)1, 	1, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskUARTReceiver, 	( signed char * ) 	"UART2 RX", 		256, 				(void *)2, 	1, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskUARTTransmitter, 	( signed char * ) 	"UART2 TX", 		256, 				(void *)2, 	1, ( xTaskHandle * ) NULL);
	
	xTaskCreate( vTaskSound, 		( signed char * ) 		"Sound driver", configMINIMAL_STACK_SIZE, 	NULL, 1, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskEEPROM, 		( signed char * ) 		"EEPROM driver", configMINIMAL_STACK_SIZE, 	NULL, 0, ( xTaskHandle * ) NULL);
	
	// Start task synchronizer (using FreeRTOS software timers)
	Synchronizer_Initialize();
	Synchronizer_Start();
	
	// Start OS
	vTaskStartScheduler();
	
	while(1);
	
}

/*****************************************************************************/	

