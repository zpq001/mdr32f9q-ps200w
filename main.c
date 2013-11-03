
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

#include "gui_top.h"

#include "stdio.h"

#include "defines.h"
#include "systemfunc.h"
#include "systick.h"
#include "lcd_1202.h"
#include "lcd_func.h"





#include "buttons.h"
#include "led.h"
#include "encoder.h"
#include "i2c_eeprom.h"
#include "dwt_delay.h"
#include "control.h"

#include "converter.h"
#include "service.h"
#include "dispatcher.h"

#include "fonts.h"
#include "images.h"



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
******************************************************************************/
int main(void)

{
//	uint16_t temp;
	int16_t enc_delta;


	
	uint8_t param;
	
	
	

	//=============================================//
	// system initialization
	//=============================================//
	Setup_CPU_Clock();
	//SysTickStart((SystemCoreClock / 1000)); 			/* Setup SysTick Timer for 1 msec interrupts  */
	DWTCounterInit();
	SSPInit();
	I2CInit();
	TimersInit();
	PortInit();
	ADCInit();
	LcdInit();
	
	InitButtons();
	ProcessButtons();
	// Default feedback channel select
	if (buttons.raw_state & SW_CHANNEL)
		Converter_Init(CHANNEL_5V);
	else
		Converter_Init(CHANNEL_12V);
	
	
	// Enable interrupt
	NVIC_EnableIRQ(Timer2_IRQn);
	
	
	SetCoolerSpeed(80);
	LcdSetBacklight(85);
	

	
	xTaskCreate( vTaskGUI, ( signed char * ) "GUI TOP", configMINIMAL_STACK_SIZE, NULL, 1, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskConverter, ( signed char * ) "Converter", configMINIMAL_STACK_SIZE, NULL, 2, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskService, ( signed char * ) "Service", configMINIMAL_STACK_SIZE, NULL, 0, ( xTaskHandle * ) NULL);
	xTaskCreate( vTaskDispatcher, ( signed char * ) "Dispatcher", configMINIMAL_STACK_SIZE, NULL, 3, ( xTaskHandle * ) NULL);
	
	
	vTaskStartScheduler();
	


	while(1);
	
	
	param = 0;	// 0 - change voltage
				// 1 - current
				// 2 - current limit
							


	//=========================================================================//
	//	Main loop
	//=========================================================================//
	while(1)
	{
		
		
		//UpdateButtons();					// Update global variable button_state
		ProcessButtons();
		enc_delta = GetEncoderDelta();		// Process incremental encoder
		
	//	Converter_ProcessADC();
//		ProcessTemperature();
		
		//-------------------------------------------//
		// Process buttons
		
		if (buttons.action_down & BTN_OFF)
		{
			Converter_Disable();
		}
		else if (buttons.action_down & BTN_ON)
		{
			Converter_Enable();
		}

		//if (button_state) StartBeep(100);
		if (enc_delta) StartBeep(50);
		
		

		//-------------------------------------------//
		// Switch regulation parameter
		
		if (buttons.action_down & BTN_ENCODER)
		{
			(param==2) ? param=0 : param++;
			
		}
			
		//-------------------------------------------//
		// Apply regulation
		
		if (enc_delta)
			switch (param)
			{
				case 0:
					Converter_SetVoltage(regulation_setting_p->set_voltage + enc_delta*500);
					break;
				case 1:
					Converter_SetCurrent(regulation_setting_p->set_current + enc_delta*500);
					break;
				case 2:
					if (enc_delta>0)
						Converter_SetCurrentLimit(CURRENT_LIM_HIGH);
					else
						Converter_SetCurrentLimit(CURRENT_LIM_LOW);
					break;
			}
		
			
			
		// Feedback channel select
		if (buttons.raw_state & SW_CHANNEL)
			Converter_SetFeedbackChannel(CHANNEL_5V);
		else
			Converter_SetFeedbackChannel(CHANNEL_12V);
		
		
		
		
		
/*		if (blink_cnt & 0x02)
			LcdPutImage((uint8_t*)imgUnderline0.data,32+10,50,imgUnderline0.width,imgUnderline0.height,lcd0_buffer);
		blink_cnt++;
	*/	
		
		
		//---------------------------------------------------//
		// Process output tasks
		
		
	//	ProcessCooler();
			
		Converter_Process();
			
		
				
		
		
		SysTickDelay(100);
		

		
	}		// end of main loop
	//=========================================================================//

	

	
	
}

/*****************************************************************************/	

