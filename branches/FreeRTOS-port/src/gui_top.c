
#include "MDR32Fx.h"
#include "MDR32F9Qx_config.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#include "stdio.h"


#include "lcd_1202.h"
#include "lcd_func.h"
#include "fonts.h"
#include "images.h"

#include "gui_top.h"
#include "converter.h"	// voltage, current, etc
#include "service.h"	// temperature
#include "control.h"	// some defines

#include "buttons.h"
#include "encoder.h"


xQueueHandle xQueueGUI;

static void gui_update_all(void);
static void gui_process_buttons(void);


uint8_t param = 0;


void vTaskGUI(void *pvParameters) 
{
	
	
	uint32_t msg;
	
	// Initialize
	xQueueGUI = xQueueCreate( 5, sizeof( uint32_t ) );		// GUI queue can contain 5 elements of type uint32_t
	if( xQueueGUI == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	
	
	while(1)
	{
		xQueueReceive(xQueueGUI, &msg, portMAX_DELAY);
		switch (msg)
		{
			case GUI_UPDATE_ALL:
				gui_update_all();
			
				break;
			case GUI_PROCESS_BUTTONS:
				gui_process_buttons();
				
				break;
		}
		
	}
	
}


static void gui_process_buttons(void)
{
	conveter_message_t msg;
	msg.type = 0;
	
	//-------------------------------------------//
	// Switch regulation parameter
	
	if (buttons.action_down & BTN_ENCODER)
	{
		(param==2) ? param=0 : param++;
		
	}
		
		
	//-------------------------------------------//
	// Apply regulation
		
	if (encoder_delta)
	switch (param)
	{
		case 0:
			msg.type = CONVERTER_SET_VOLTAGE;
			msg.data_a = regulation_setting_p->set_voltage + encoder_delta*500;
			break;
		case 1:
			msg.type = CONVERTER_SET_CURRENT;
			msg.data_a = regulation_setting_p->set_current + encoder_delta*500;
			break;
		case 2:
			if (encoder_delta>0)
				msg.type = SET_CURRENT_LIMIT_40A;
			else
				msg.type = SET_CURRENT_LIMIT_20A;
			break;
	}
	if (msg.type)
		xQueueSendToBack(xQueueConverter, &msg, 0);
	
}


static void gui_update_all(void)
{
	char str1[20] =	"----------";
	char str2[20] =	"----------";
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
			
			
		LcdUpdateByCore(LCD0,lcd0_buffer);
		//vTaskDelay(1);
		LcdUpdateByCore(LCD1,lcd1_buffer);
}







