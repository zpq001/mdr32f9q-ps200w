
#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"
#include "MDR32F9Qx_ssp.h"
#include "MDR32F9Qx_port.h"
#include "MDR32F9Qx_timer.h"
#include "MDR32F9Qx_adc.h"

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
			DWTDelayUs(10);
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





/*****************************************************************************
*		Main function
*
******************************************************************************/
int main(void)

{
	uint16_t temp;
	int16_t enc_delta;
	char str1[20] =	"----------";
	char str2[20] =	"----------";

	
	uint8_t param;
	
	
	

	//=============================================//
	// system initialization
	//=============================================//
	Setup_CPU_Clock();
	SysTickStart((SystemCoreClock / 1000)); 			/* Setup SysTick Timer for 1 msec interrupts  */
	DWTCounterInit();
	SSPInit();
	I2CInit();
	TimersInit();
	PortInit();
	ADCInit();
	LcdInit();
	
	Converter_Init();
	
	SetCoolerSpeed(80);
	
	// beep
	SetBuzzerFreq(500);
	StartBeep(50);
	
	
	// Turn on backlight slowly
	for (temp = 0; temp <= 90; temp++)
	{
		LcdSetBacklight(temp);
		SysTickDelay(5);
	}
	
	WaitBeep();
	
	
	
	
	
	
	/*

//	LcdDisplayVoltageCurrent(1245,0,lcd0_buffer);
//  LcdDisplayVoltageCurrent(869,1,lcd1_buffer);

	LcdPutImage(digit_0,		0,	0,	11,	20,	lcd0_buffer);
	LcdPutImage(digit_1,		12,	0,	11,	20,	lcd0_buffer);
	LcdPutImage(digit_2,		24,	0,	11,	20,	lcd0_buffer);
	LcdPutImage(digit_3,		36,	0,	11,	20,	lcd0_buffer);
	LcdPutImage(digit_4,		48,	0,	11,	20,	lcd0_buffer);
	
	LcdPutImage(digit_5,		0,	0,	11,	20,	lcd1_buffer);
	LcdPutImage(digit_6,		12,	0,	11,	20,	lcd1_buffer);
	LcdPutImage(digit_7,		24,	0,	11,	20,	lcd1_buffer);
	LcdPutImage(digit_8,		36,	0,	11,	20,	lcd1_buffer);
	LcdPutImage(digit_9,		48,	0,	11,	20,	lcd1_buffer);
	
	*/
	
//	LcdPutStr(0,0,"12547",(tFont*)font_20x11,lcd0_buffer);
	
	//LcdPutStr(0,33,(uint8_t*)str2,(tSpecialFont*)font_16x10,lcd0_buffer);
	
//	LcdPutNormalStr(0,40,"SET:12V",(tNormalFont*)Font_8x12,lcd0_buffer);
//	LcdPutNormalStr(0,0,"137W",(tNormalFont*)Font_7x10_bold,lcd1_buffer);

//	LcdPutImage((uint8_t*)imgSelect0.data,30,30,imgSelect0.width,imgSelect0.height,lcd0_buffer);
	
//	LcdUpdateByCore(LCD0,lcd0_buffer);
//	LcdUpdateByCore(LCD1,lcd1_buffer);
	
//	for(;;);

	/*
	
	while(1) 	
	{
		SetCoolerSpeed(5);
		SetCoolerSpeed(10);
		SetCoolerSpeed(20);
		SetCoolerSpeed(30);
		SetCoolerSpeed(40);
		SetCoolerSpeed(50);
		SetCoolerSpeed(60);
		SetCoolerSpeed(70);
		SetCoolerSpeed(80);
		SetCoolerSpeed(90);
		SetCoolerSpeed(100);
	}
	
  */		
	
		
		
	/*	
	
	// Test EEPROM		
	if (EEPROM_test() == 0)
		LcdPutStrNative(0,0,"EEPROM OK!  ",0,lcd0_buffer);
	else
		LcdPutStrNative(0,0,"EEPROM error",0,lcd0_buffer);	
	*/
	
	/*
	while(1) 	
	{
		system_control.LoadDisable = LOAD_DISABLE;
		ApplySystemControl(&system_control);	
		
		system_control.LoadDisable = LOAD_ENABLE;
		ApplySystemControl(&system_control);	
	}
	*/
	
	


	
	
	param = 0;	// 0 - change voltage
				// 1 - current
				// 2 - current limit
							


	//=========================================================================//
	//	Main loop
	//=========================================================================//
	while(1)
	{
		
		
		UpdateButtons();					// Update global variable button_state
		enc_delta = GetEncoderDelta();		// Process incremental encoder
		
		Converter_ProcessADC();
		ProcessTemperature();
		
		//-------------------------------------------//
		// Process buttons
		
		if (button_state & BUTTON_OFF)
		{
			Converter_Disable();
		}
		else if (button_state & BUTTON_ON)
		{
			Converter_Enable();
		}

		//if (button_state) StartBeep(100);
		if (enc_delta) StartBeep(50);
		if (button_state & BUTTON_OK ) StartBeep(2000);
		
		

		//-------------------------------------------//
		// Switch regulation parameter
		
		if (button_state & BUTTON_ENCODER)
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
		if (button_state & MODE_SWITCH)
			Converter_SetFeedbackChannel(CHANNEL_5V);
		else
			Converter_SetFeedbackChannel(CHANNEL_12V);
		
		
		
		
		//---------------------------------------------------//
		// Graphical representation
		//---------------------------------------------------//
			
		
		// Clear LCD buffers
		LcdFillBuffer(lcd0_buffer,0x00);
		LcdFillBuffer(lcd1_buffer,0x00);
			
		
		//------------------------//
		// Voltage section
		//------------------------//
	  
	    // Measured 
		sprintf(str1,"%5.2fV",(float)(voltage_adc)/1000 );
		LcdPutSpecialStr(0,0,(uint8_t*)str1,(tSpecialFont*)&font_32x19,lcd0_buffer);
	  
		// Set
		sprintf(str2,"%5.2fV",(float)(regulation_setting_p->set_voltage)/1000 );
		LcdPutNormalStr(0,38,"SET:",(tNormalFont*)&font_8x12,lcd0_buffer);
		LcdPutSpecialStr(32,33,(uint8_t*)str2,(tSpecialFont*)&font_16x10,lcd0_buffer);
			
			
			
		//------------------------//
		// Current section
		//------------------------//
		
		
		// Measured 
		sprintf(str1,"%5.2fA",(float)(current_adc)/1000 );
		LcdPutSpecialStr(0,0,(uint8_t*)str1,(tSpecialFont*)&font_32x19,lcd1_buffer);
		
		// Set
		sprintf(str2,"%5.2fA",(float)(regulation_setting_p->set_current)/1000 );
		LcdPutNormalStr(0,38,"SET:",(tNormalFont*)&font_8x12,lcd1_buffer);
		LcdPutSpecialStr(32,33,(uint8_t*)str2,(tSpecialFont*)&font_16x10,lcd1_buffer);

		
			
		
		//------------------------//
		// Frames LCD0
		//------------------------//
	
		LcdPutHorLine(0,55,LCD_XSIZE,PIXEL_ON,lcd0_buffer);
		LcdPutVertLine(48,56,13,PIXEL_ON,lcd0_buffer);
		
		sprintf(str1,"%2.0f%cC",(float)converter_temp_celsius,0xb0); //0xb7 );
		LcdPutNormalStr(63,57,(uint8_t*)str1,(tNormalFont*)&font_8x12,lcd0_buffer);
		
		if (regulation_setting_p->CHANNEL == CHANNEL_5V)
			LcdPutNormalStr(0,57,"Ch. 5V",(tNormalFont*)&font_8x12,lcd0_buffer);
		else
			LcdPutNormalStr(0,57,"Ch.12V",(tNormalFont*)&font_8x12,lcd0_buffer);
		
		
		
		
		
		//------------------------//
		// Frames LCD1
		//------------------------//

		LcdPutHorLine(0,55,LCD_XSIZE,PIXEL_ON,lcd1_buffer);
		LcdPutVertLine(42,56,13,PIXEL_ON,lcd1_buffer);
		
		
		if (regulation_setting_p -> current_limit == CURRENT_LIM_HIGH)
			LcdPutNormalStr(2,57,"40A",(tNormalFont*)&font_8x12,lcd1_buffer);
		else
			LcdPutNormalStr(2,57,"20A",(tNormalFont*)&font_8x12,lcd1_buffer);
		
		sprintf(str1,"%5.1fW",(float)(power_adc)/1000 );
		LcdPutNormalStr(47,57,(uint8_t*)str1,(tNormalFont*)&font_8x12,lcd1_buffer);
		
		
		
		//------------------------//
		// Current or voltage regulation
		//	selection
		//------------------------//
		str1[0] = '<';
		str1[1] = '\0';
		switch(param)
			{
				case 0:
				  //LcdPutNormalStr(88,38,(uint8_t*)str1,(tNormalFont*)&font_8x12,lcd0_buffer);
				  LcdPutImage((uint8_t*)imgSelect0.data,89,38,imgSelect0.width,imgSelect0.height,lcd0_buffer);
				  break;
				case 1:
				  //LcdPutNormalStr(88,38,(uint8_t*)str1,(tNormalFont*)&font_8x12,lcd1_buffer);
				  LcdPutImage((uint8_t*)imgSelect0.data,89,38,imgSelect0.width,imgSelect0.height,lcd1_buffer);
				  break;
				case 2:
					//LcdPutNormalStr(33,57,(uint8_t*)str1,(tNormalFont*)&font_8x12,lcd1_buffer);
				  LcdPutImage((uint8_t*)imgSelect0.data,35,56,imgSelect0.width,imgSelect0.height,lcd1_buffer);
			}		
			
/*		if (blink_cnt & 0x02)
			LcdPutImage((uint8_t*)imgUnderline0.data,32+10,50,imgUnderline0.width,imgUnderline0.height,lcd0_buffer);
		blink_cnt++;
	*/	
		
		
		//---------------------------------------------------//
		// Process output tasks
		
		
		ProcessCooler();
			
		Converter_Process();
			
		
				
		LcdUpdateByCore(LCD0,lcd0_buffer);
		LcdUpdateByCore(LCD1,lcd1_buffer);
		
		SysTickDelay(100);
		

		
	}		// end of main loop
	//=========================================================================//

	

	
	
}

/*****************************************************************************/	

