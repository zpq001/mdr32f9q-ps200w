
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


#include "fonts.h"
#include "images.h"


extern uint8_t system_overloaded;


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
	uint16_t i;
	int16_t voltage_pwm_period;
	int16_t current_pwm_period;
	uint16_t cooler_speed;
	uint16_t voltage_adc;
	uint16_t current_adc;
	uint16_t converter_temp;
	float converter_temp_norm = 0;
	uint8_t param;
	uint8_t temperature_cnt = 0;
	uint32_t temperature_acc = 0;
	
	uint8_t is_charging = 0;
	uint16_t power_on_counter = 0;
	
	uint16_t voltage_adc_stored = 0;
	uint16_t charge_ticks = 0;
	int16_t voltage_delta = 0;
	
	uint8_t delta_lim_counter = 0;
	uint8_t blink_cnt = 0;

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
	
	SetCoolerSpeed(50);
	
	
	
	
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
	
	
  //	__disable_irq();
  //	__enable_irq();

  voltage_pwm_period = 1000;
	current_pwm_period = 200;
	cooler_speed = 50;
	
	param = 0;	// 0 - change voltage
							// 1 - current
							// 2 - current limit
							


	//=========================================================================//
	//	Main loop
	//=========================================================================//
	while(1)
	{
		// Clear LCD buffers
		LcdFillBuffer(lcd0_buffer,0x00);
		LcdFillBuffer(lcd1_buffer,0x00);
		

		
		//-------------------------------------------//
		// Get new system status
		
		UpdateButtons();												// Update global variable button_state
//		UpdateSystemStatus(&system_status);			// Update system status
		enc_delta = GetEncoderDelta();					// Process incremental encoder
		
		
		//-------------------------------------------//
		// Process buttons
		
		if (button_state & BUTTON_OFF)
		{
			system_control.ConverterState = CONVERTER_OFF;
			system_overloaded = 0;
			led_state &= ~(LED_GREEN | LED_RED);
			is_charging = 0;
			system_control.LoadDisable = LOAD_ENABLE;

		}
		else if ( (button_state & BUTTON_ON) && (system_overloaded == 0) )
		{
			if (is_charging == 0)
			{
				system_control.ConverterState = CONVERTER_ON;
				led_state |= LED_GREEN;
				system_control.LoadDisable = LOAD_ENABLE;
			}
		}

		//if (button_state) StartBeep(100);
		if (enc_delta) StartBeep(50);
		if (button_state & BUTTON_OK ) StartBeep(2000);
		
		// Detect long press "ON" button
		i = raw_button_state & BUTTON_ON;
		if (i == BUTTON_ON) 
			power_on_counter = (power_on_counter < 10) ?  power_on_counter + 1 : power_on_counter;
	  else
			power_on_counter = 0;
				
		// Start charge when "ON" button is pressed for long enough
		if ( (is_charging == 0) && (power_on_counter == 10) )
		{
			// Set the flag
			is_charging = 1;
			// Disable resistive load on output
			system_control.LoadDisable = LOAD_DISABLE;
			// Set high voltage
			voltage_pwm_period = 3200;	// 16V
			// Last voltage
			voltage_adc_stored = 0;
			// Delta
			voltage_delta = 0;
			// Delta counter
			delta_lim_counter = 0;

			charge_ticks = 0;
			StartBeep(300);
		}
		
		
		//-------------------------------------------//
		// Measure voltage, current and temperature

		if (is_charging == 0)
		{
			ADC1_SetChannel(ADC_CHANNEL_VOLTAGE); 
			DWTDelayUs(10);
			ADC1_Start();
			while( ADC_GetFlagStatus(ADC1_FLAG_END_OF_CONVERSION)==RESET );
			voltage_adc		= ADC1_GetResult();
		}
		else
		{
			system_control.ConverterState = CONVERTER_OFF;
			led_state |= LED_RED;
			ApplySystemControl(&system_control);				
			UpdateLEDs();	
			
			SysTickDelay(50);
			ADC1_SetChannel(ADC_CHANNEL_VOLTAGE);
			voltage_adc = 0;
			for (i=0;i<4;i++)
			{
				DWTDelayUs(100);
				ADC1_Start();
				while( ADC_GetFlagStatus(ADC1_FLAG_END_OF_CONVERSION)==RESET );
				voltage_adc		+= ADC1_GetResult();
			}
			voltage_adc >>= 2;
			
			
			system_control.ConverterState = CONVERTER_ON;
			led_state &= ~LED_RED;
			ApplySystemControl(&system_control);	
			SysTickDelay(50);
			UpdateLEDs();	
		}			
		
		
		ADC1_SetChannel(ADC_CHANNEL_CURRENT); 
		DWTDelayUs(100);
		ADC1_Start();
		while( ADC_GetFlagStatus(ADC1_FLAG_END_OF_CONVERSION)==RESET );
		current_adc		= ADC1_GetResult();
		
		
		ADC1_SetChannel(ADC_CHANNEL_CONVERTER_TEMP); 
		DWTDelayUs(10);
		ADC1_Start();
		while( ADC_GetFlagStatus(ADC1_FLAG_END_OF_CONVERSION)==RESET );
		converter_temp = ADC1_GetResult();
		
		temperature_acc += converter_temp;
		
		if(	temperature_cnt >= 9)
		{
			temperature_cnt = 0;
			
			
		  converter_temp_norm =  (float)temperature_acc*0.1*(-0.226) + 230 ;
			
			temperature_acc = 0;
		}
		else
			temperature_cnt++;
		
	
			//-------------------------------------------//
		// Switch regulation parameter
		if (button_state & BUTTON_ENCODER)
		{
			(param==2) ? param=0 : param++;
			
		}
			
			
			
		
		if (enc_delta)
			switch (param)
			{
				case 0:
					  // Do not regulate voltage while charging
					  if (is_charging == 1) break;	
						voltage_pwm_period += enc_delta*100;
						if (voltage_pwm_period < 0)
							voltage_pwm_period = 0;
						else if (voltage_pwm_period > 4000)
							voltage_pwm_period = 4000;
						break;
				case 1:
						current_pwm_period += enc_delta*100;
						if (current_pwm_period < 0) 
							current_pwm_period = 0;
						else if (current_pwm_period > 4000)
							current_pwm_period = 4000;
						break;
				case 2:
						if (enc_delta>0)
							system_control.CurrentLimit = CURRENT_LIM_MAX;
						else
							system_control.CurrentLimit = CURRENT_LIM_MIN;
						break;
			}
		
			
			
			// Feedback channel select
		if (button_state & MODE_SWITCH)
		{
				// If changed, turn off converter
				if (system_control.SelectedChannel == CHANNEL_12V)
				{
						system_control.ConverterState = CONVERTER_OFF;
						led_state &= ~LED_GREEN;
						UpdateLEDs();
					
						// Stop charging
						system_control.LoadDisable = LOAD_ENABLE;
					  is_charging = 0;
					
						ApplySystemControl(&system_control);	
					  voltage_pwm_period = 1000;
				    current_pwm_period = 500;
					  SysTickDelay(100);
				}
				system_control.SelectedChannel = CHANNEL_5V;
		}
		else
		{
			// If changed, turn off converter
				if (system_control.SelectedChannel == CHANNEL_5V)
				{
						system_control.ConverterState = CONVERTER_OFF;
						led_state &= ~LED_GREEN;
						UpdateLEDs();
					
					  // Stop charging
						system_control.LoadDisable = LOAD_ENABLE;
					  is_charging = 0;
					
						ApplySystemControl(&system_control);	
					  voltage_pwm_period = 1000;
				    current_pwm_period = 200;
					  SysTickDelay(100);
				}
				system_control.SelectedChannel = CHANNEL_12V;
		}
		
		
		
		
		//------ Process charging --------//
		if (is_charging)
		{
			if (charge_ticks > 20)	
			{
					charge_ticks = 0;
					voltage_delta = (int32_t)voltage_adc - (int32_t)voltage_adc_stored;
					// LSB == 5mV
					if ( voltage_delta < -5 )	// if dV < -25mV, stop charge
					{
						delta_lim_counter++;
						if (delta_lim_counter == 1)
						{
							Beep(50);
							SysTickDelay(50);
						  Beep(50);
						}
						// Charge process finish
						if (delta_lim_counter == 2)
						{
							// Stop charging
							delta_lim_counter = 0;
							is_charging = 0;
							system_control.ConverterState = CONVERTER_OFF;
							led_state &= ~LED_GREEN;
							// Beep
							StartBeep(100);
							SysTickDelay(200);
							StartBeep(100);
							SysTickDelay(200);
							StartBeep(100);
							SysTickDelay(200);
							StartBeep(100);
						}
					}
					// Store maximum
					if (voltage_adc_stored < voltage_adc)
						voltage_adc_stored = voltage_adc;
			}
			else
				charge_ticks++;
	}
		
		//--------------------------------//
		
		
		
		
		
		
		// Cooler speed regulation
		// Proportional
		cooler_speed = (converter_temp_norm < 25) ? 50 : converter_temp_norm * 2;
		

		
		
		
		
		//---------------------------------------------------//
		// Graphical representation
		//---------------------------------------------------//
			
			
			
		//------------------------//
		// Voltage section
		//------------------------//
	  
		// Measured 
		sprintf(str1,"%5.2fV",(float)(voltage_adc>>1)/100 );
		LcdPutSpecialStr(0,0,(uint8_t*)str1,(tSpecialFont*)&font_32x19,lcd0_buffer);
		
		// Set
		if (is_charging == 0)
		{
			sprintf(str2,"%5.2fV",(float)(voltage_pwm_period>>1)/100 );
			LcdPutNormalStr(0,38,"SET:",(tNormalFont*)&font_8x12,lcd0_buffer);
			LcdPutSpecialStr(32,33,(uint8_t*)str2,(tSpecialFont*)&font_16x10,lcd0_buffer);
			
		}			
		else
		{
			sprintf(str1,"dU=%4dmV", voltage_delta*5);
			LcdPutNormalStr(0,40,(uint8_t*)str1,(tNormalFont*)&font_8x12,lcd0_buffer);
		}
		
			
			
			
		//------------------------//
	  // Current section
		//------------------------//
		
		if (system_control.CurrentLimit == CURRENT_LIM_MAX)
		{
			// Measured 
			sprintf(str1,"%5.2fA",(float)(current_adc)/100 );
			// Set
			sprintf(str2,"%5.2fA",(float)(current_pwm_period)/100 );
		}
		else
		{
			// Measured 
			sprintf(str1,"%5.2fA",(float)(current_adc>>1)/100 );
			// Set
			sprintf(str2,"%5.2fA",(float)(current_pwm_period>>1)/100 );
		}
		
		LcdPutSpecialStr(0,0,(uint8_t*)str1,(tSpecialFont*)&font_32x19,lcd1_buffer);
		LcdPutNormalStr(0,38,"SET:",(tNormalFont*)&font_8x12,lcd1_buffer);
		LcdPutSpecialStr(32,33,(uint8_t*)str2,(tSpecialFont*)&font_16x10,lcd1_buffer);

		
			
		
		//------------------------//
		// Frames LCD0
		//------------------------//
	
	  LcdPutHorLine(0,55,LCD_XSIZE,PIXEL_ON,lcd0_buffer);
		LcdPutVertLine(48,56,13,PIXEL_ON,lcd0_buffer);
		
		sprintf(str1,"%2.0f%cC",converter_temp_norm,0xb0); //0xb7 );
		LcdPutNormalStr(63,57,(uint8_t*)str1,(tNormalFont*)&font_8x12,lcd0_buffer);
		
		if (system_control.SelectedChannel == CHANNEL_5V)
			LcdPutNormalStr(0,57,"Ch. 5V",(tNormalFont*)&font_8x12,lcd0_buffer);
		else
			LcdPutNormalStr(0,57,"Ch.12V",(tNormalFont*)&font_8x12,lcd0_buffer);
		
		
		
		
		
		//------------------------//
		// Frames LCD1
		//------------------------//

		LcdPutHorLine(0,55,LCD_XSIZE,PIXEL_ON,lcd1_buffer);
		LcdPutVertLine(42,56,13,PIXEL_ON,lcd1_buffer);
		
		if (system_control.CurrentLimit == CURRENT_LIM_MAX)
			LcdPutNormalStr(2,57,"40A",(tNormalFont*)&font_8x12,lcd1_buffer);
		else
			LcdPutNormalStr(2,57,"20A",(tNormalFont*)&font_8x12,lcd1_buffer);
		
		
		if (system_control.CurrentLimit == CURRENT_LIM_MAX)
		  sprintf(str1,"%5.1fW",(float)(voltage_adc>>1)*( (float)(current_adc)/100 )/100 );
		else
			sprintf(str1,"%5.1fW",(float)(voltage_adc>>1)*( (float)(current_adc>>1)/100 )/100 );
		
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
		// Apply settings
		
		ApplySystemControl(&system_control);				
		UpdateLEDs();																
		
		SetVoltagePWMPeriod(voltage_pwm_period);	
		SetCurrentPWMPeriod(current_pwm_period);
		SetCoolerSpeed(cooler_speed);
				
		LcdUpdateByCore(LCD0,lcd0_buffer);
		LcdUpdateByCore(LCD1,lcd1_buffer);
		
		SysTickDelay(200);
		

		
	}		// end of main loop
	//=========================================================================//

	

	
	
}

/*****************************************************************************/	

