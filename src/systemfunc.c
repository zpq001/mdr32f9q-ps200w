/*******************************************************************
	Module systemfunc.c
	
		Functions for system control.
		

********************************************************************/


#include "MDR32Fx.h"
#include "MDR32F9Qx_rst_clk.h"
#include "MDR32F9Qx_ssp.h"
#include "MDR32F9Qx_port.h"
#include "MDR32F9Qx_timer.h"
#include "MDR32F9Qx_i2c.h"
#include "MDR32F9Qx_adc.h"

#include "systemfunc.h"
#include "defines.h"
#include "systick.h"
#include "lcd_1202.h"
#include "lcd_func.h"
#include "dwt_delay.h"

#include "control.h"
#include "led.h"

#include "fonts.h"



int16_t converter_temp_celsius = 0;




//==============================================================//
// Setup CPU core clock
// 16MHz clock from 4 MHz HSE
//==============================================================//
void Setup_CPU_Clock(void)
{
  /* Enable HSE */
  RST_CLK_HSEconfig(RST_CLK_HSE_ON);
  if (RST_CLK_HSEstatus() != SUCCESS)
  {
    /* Trap */
    while (1) {}
  }

  /* CPU_C1_SEL = HSE */
  RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, RST_CLK_CPU_PLLmul4);
  RST_CLK_CPU_PLLcmd(ENABLE);
  if (RST_CLK_CPU_PLLstatus() != SUCCESS)
  {
    /* Trap */
    while (1) {}
  }

  /* CPU_C3_SEL = CPU_C2_SEL */
  RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);
  /* CPU_C2_SEL = CPU_C1_SEL */
  RST_CLK_CPU_PLLuse(ENABLE);
  /* HCLK_SEL = CPU_C3_SEL */
  RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);
	
	// Update system clock variable
	SystemCoreClockUpdate();
	
	
	
		/* Enable clock on all ports */
  RST_CLK_PCLKcmd(ALL_PORTS_CLK, ENABLE);
	// Enable clock on peripheral blocks used in design
	RST_CLK_PCLKcmd(PERIPHERALS_CLK ,ENABLE);
	
}



//==============================================================//
// Setup IO ports
// Ports clk is enabled before
//==============================================================//
void PortInit(void)
{
	PORT_InitTypeDef PORT_InitStructure;
  
	
	PORT_DeInit(MDR_PORTA);
	PORT_DeInit(MDR_PORTB);
	PORT_DeInit(MDR_PORTC);
	PORT_DeInit(MDR_PORTD);
	PORT_DeInit(MDR_PORTE);
	PORT_DeInit(MDR_PORTF);
	
	// default output value is 0
	// Set some outputs the default design values of 1
	//PORT_SetBits(MDR_PORTA, (1<<BUZ1) | (1<<BUZ2) );
	PORT_SetBits(MDR_PORTB, (1<<LGREEN) | (1<<LRED) );
	
	/*
	   Reset PORT initialization structure parameters values 
  PORT_InitStruct->PORT_Pin        = PORT_Pin_All;
  PORT_InitStruct->PORT_OE         = PORT_OE_IN;
  PORT_InitStruct->PORT_PULL_UP    = PORT_PULL_UP_OFF;
  PORT_InitStruct->PORT_PULL_DOWN  = PORT_PULL_DOWN_OFF;
  PORT_InitStruct->PORT_PD_SHM     = PORT_PD_SHM_OFF;
  PORT_InitStruct->PORT_PD         = PORT_PD_DRIVER;
  PORT_InitStruct->PORT_GFEN       = PORT_GFEN_OFF;
  PORT_InitStruct->PORT_FUNC       = PORT_FUNC_PORT;
  PORT_InitStruct->PORT_SPEED      = PORT_OUTPUT_OFF;
  PORT_InitStruct->PORT_MODE       = PORT_MODE_ANALOG;
	*/
	
	//======================= PORTA =================================//
	PORT_StructInit(&PORT_InitStructure);
	
	// Typical digital inputs:
	PORT_InitStructure.PORT_Pin   = (1<<OVERLD) | (1<<ENC_BTN);
	PORT_InitStructure.PORT_MODE  = PORT_MODE_DIGITAL;
  PORT_Init(MDR_PORTA, &PORT_InitStructure);
	
	// Digital input with pull-up
	PORT_InitStructure.PORT_Pin   = (1<<EEN);
	PORT_InitStructure.PORT_PULL_UP  = PORT_PULL_UP_ON;
  PORT_Init(MDR_PORTA, &PORT_InitStructure);
	
	// Typical digital outputs:
	PORT_InitStructure.PORT_Pin   = (1<<CLIM_SEL);
	PORT_InitStructure.PORT_OE    = PORT_OE_OUT;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_SLOW;
	PORT_Init(MDR_PORTA, &PORT_InitStructure);
	
	// Timer outputs to buzzer (TMR1.CH2, TMR1.CH2N)
  PORT_InitStructure.PORT_Pin   = (1<<BUZ1) | (1<<BUZ2);
	PORT_InitStructure.PORT_FUNC  = PORT_FUNC_ALTER;
	PORT_Init(MDR_PORTA, &PORT_InitStructure);
	
	// TODO: add USART1 pins
	
	
	//======================= PORTB =================================//
	PORT_StructInit(&PORT_InitStructure);
	
	// Typical digital inputs: buttons and encoder
	PORT_InitStructure.PORT_Pin   = (1<<SB_ESC) | (1<<SB_LEFT) | (1<<SB_RIGHT) | (1<<SB_OK) | (1<<SB_MODE) | (1<<ENCA) | (1<<ENCB) | (1<<PG) ;
	PORT_InitStructure.PORT_MODE  = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTB, &PORT_InitStructure);
	
	// Power good 
	// TODO: add interrupt
	PORT_InitStructure.PORT_Pin   = (1<<PG) ;
	PORT_Init(MDR_PORTB, &PORT_InitStructure);
	
	// Leds and buttons SB_ON, SB_OFF
	PORT_InitStructure.PORT_Pin   = (1<<LGREEN) | (1<<LRED);
	PORT_InitStructure.PORT_OE    = PORT_OE_OUT;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_SLOW;
	PORT_Init(MDR_PORTB, &PORT_InitStructure);

	
	//======================= PORTC =================================//
	PORT_StructInit(&PORT_InitStructure);
	
	// LCD Backlight (TMR3.CH1)
	PORT_InitStructure.PORT_Pin   = (1<<LCD_BL);
	PORT_InitStructure.PORT_MODE = PORT_MODE_DIGITAL;
	PORT_InitStructure.PORT_FUNC  = PORT_FUNC_ALTER;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_SLOW;
	PORT_Init(MDR_PORTC, &PORT_InitStructure);
	
	// I2C
	PORT_InitStructure.PORT_Pin = (1<<SCL) | (1<<SDA);
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_FAST;
  PORT_Init(MDR_PORTC, &PORT_InitStructure);
	
	
	//======================= PORTD =================================//
	PORT_StructInit(&PORT_InitStructure);
	
	// Analog functions
	PORT_InitStructure.PORT_Pin   = (1<<VREF_P) | (1<<VREF_N) | (1<<TEMP_IN) | (1<<UADC) | (1<<IADC) ;
	PORT_Init(MDR_PORTD, &PORT_InitStructure);
	
	// LCD CLK and CS
	PORT_InitStructure.PORT_Pin   = (1<<LCD_CLK) | (1<<LCD_CS) ;
	PORT_InitStructure.PORT_MODE  = PORT_MODE_DIGITAL;
	PORT_InitStructure.PORT_FUNC  = PORT_FUNC_ALTER;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_FAST;
	PORT_InitStructure.PORT_OE    = PORT_OE_OUT;
	PORT_Init(MDR_PORTD, &PORT_InitStructure);
	
	// MOSI 
  PORT_InitStructure.PORT_Pin   = (1<<LCD_MOSI);
	PORT_InitStructure.PORT_PULL_DOWN = PORT_PULL_DOWN_ON;
	PORT_Init(MDR_PORTD, &PORT_InitStructure);
	

	
	//======================= PORTE =================================//
	PORT_StructInit(&PORT_InitStructure);
			
  // LCD RST and SEL, Load disable output
  PORT_InitStructure.PORT_Pin   = (1<<LCD_RST) | (1<<LCD_SEL) | (1<<LDIS);
	PORT_InitStructure.PORT_MODE  = PORT_MODE_DIGITAL;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_FAST;
  PORT_InitStructure.PORT_OE    = PORT_OE_OUT;
  PORT_Init(MDR_PORTE, &PORT_InitStructure);

	// cooler PWM output (TMR3.CH3N)
	PORT_InitStructure.PORT_Pin   = (1<<CPWM);
  PORT_InitStructure.PORT_FUNC  = PORT_FUNC_OVERRID;
	PORT_Init(MDR_PORTE, &PORT_InitStructure);
	
	// voltage and current PWM outputs (TMR2.CH1N, TMR2.CH3N)
	PORT_InitStructure.PORT_Pin   = (1<<UPWM) | (1<<IPWM);
  PORT_InitStructure.PORT_FUNC  = PORT_FUNC_ALTER;
	PORT_Init(MDR_PORTE, &PORT_InitStructure);
	
	
	
	//======================= PORTF =================================//
	PORT_StructInit(&PORT_InitStructure);
	
	// Feedback channel select, converter enable
	PORT_InitStructure.PORT_Pin   = (1<<EN) | (1<<STAB_SEL) ;
	PORT_InitStructure.PORT_MODE  = PORT_MODE_DIGITAL;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_SLOW;
  PORT_InitStructure.PORT_OE    = PORT_OE_OUT;
  PORT_Init(MDR_PORTF, &PORT_InitStructure);
	
	// TODO: add USART2 functions	
	
	

}


void SSPInit(void)
{
	SSP_InitTypeDef sSSP;
	SSP_StructInit (&sSSP);

	SSP_BRGInit(MDR_SSP2,SSP_HCLKdiv1);
	
	sSSP.SSP_SCR  = 0x04;
  sSSP.SSP_CPSDVSR = 2;
  sSSP.SSP_Mode = SSP_ModeMaster;
  sSSP.SSP_WordLength = SSP_WordLength9b;
  sSSP.SSP_SPH = SSP_SPH_1Edge;
  sSSP.SSP_SPO = SSP_SPO_Low;
  sSSP.SSP_FRF = SSP_FRF_SPI_Motorola;
  sSSP.SSP_HardwareFlowControl = SSP_HardwareFlowControl_SSE;
  SSP_Init (MDR_SSP2,&sSSP);

	SSP_Cmd(MDR_SSP2, ENABLE);

}


void I2CInit(void)
{
	//--------------- I2C INIT ---------------//
I2C_InitTypeDef I2C_InitStruct;
	
  /* Enables I2C peripheral */
  I2C_Cmd(ENABLE);
	
	/* Initialize I2C_InitStruct */
  I2C_InitStruct.I2C_ClkDiv = 128;	// 125 kHz clock @ 16MHz
  I2C_InitStruct.I2C_Speed = I2C_SPEED_UP_TO_400KHz;

  /* Configure I2C parameters */
  I2C_Init(&I2C_InitStruct);
	
}


void ADCInit(void)
{
	ADC_InitTypeDef sADC;
	ADCx_InitTypeDef sADCx;
//	uint16_t temp_adc;
	
	/* 
					ADC_StructInit
	ADC_InitStruct->ADC_SynchronousMode      = ADC_SyncMode_Independent;
  ADC_InitStruct->ADC_StartDelay           = 0;
  ADC_InitStruct->ADC_TempSensor           = ADC_TEMP_SENSOR_Disable;
  ADC_InitStruct->ADC_TempSensorAmplifier  = ADC_TEMP_SENSOR_AMPLIFIER_Disable;
  ADC_InitStruct->ADC_TempSensorConversion = ADC_TEMP_SENSOR_CONVERSION_Disable;
  ADC_InitStruct->ADC_IntVRefConversion    = ADC_VREF_CONVERSION_Disable;
  ADC_InitStruct->ADC_IntVRefTrimming      = 0;
	*/
	
	/*
					ADCx_StructInit
	ADCx_InitStruct->ADC_ClockSource      = ADC_CLOCK_SOURCE_CPU;
  ADCx_InitStruct->ADC_SamplingMode     = ADC_SAMPLING_MODE_SINGLE_CONV;
  ADCx_InitStruct->ADC_ChannelSwitching = ADC_CH_SWITCHING_Disable;
  ADCx_InitStruct->ADC_ChannelNumber    = ADC_CH_ADC0;
  ADCx_InitStruct->ADC_Channels         = 0;
  ADCx_InitStruct->ADC_LevelControl     = ADC_LEVEL_CONTROL_Disable;
  ADCx_InitStruct->ADC_LowLevel         = 0;
  ADCx_InitStruct->ADC_HighLevel        = 0;
  ADCx_InitStruct->ADC_VRefSource       = ADC_VREF_SOURCE_INTERNAL;
  ADCx_InitStruct->ADC_IntVRefSource    = ADC_INT_VREF_SOURCE_INEXACT;
  ADCx_InitStruct->ADC_Prescaler        = ADC_CLK_div_None;
  ADCx_InitStruct->ADC_DelayGo          = 0;
	*/
	
	 /* ADC Configuration */
  /* Reset all ADC settings */
  ADC_DeInit();
  ADC_StructInit(&sADC);

  sADC.ADC_SynchronousMode      = ADC_SyncMode_Independent;
  sADC.ADC_StartDelay           = 10;
  sADC.ADC_TempSensor           = ADC_TEMP_SENSOR_Enable;
  sADC.ADC_TempSensorAmplifier  = ADC_TEMP_SENSOR_AMPLIFIER_Enable;
  sADC.ADC_TempSensorConversion = ADC_TEMP_SENSOR_CONVERSION_Enable;
  sADC.ADC_IntVRefConversion    = ADC_VREF_CONVERSION_Enable;
  sADC.ADC_IntVRefTrimming      = 1;
  ADC_Init (&sADC);

  /* ADC1 Configuration */
  ADCx_StructInit (&sADCx);
  sADCx.ADC_ClockSource      = ADC_CLOCK_SOURCE_CPU;
  sADCx.ADC_SamplingMode     = ADC_SAMPLING_MODE_SINGLE_CONV;
  sADCx.ADC_ChannelSwitching = ADC_CH_SWITCHING_Disable;
  sADCx.ADC_ChannelNumber    = ADC_CH_TEMP_SENSOR;		
  sADCx.ADC_Channels         = 0;
  sADCx.ADC_LevelControl     = ADC_LEVEL_CONTROL_Disable;
  sADCx.ADC_LowLevel         = 0;
  sADCx.ADC_HighLevel        = 0;
  sADCx.ADC_VRefSource       = ADC_VREF_SOURCE_EXTERNAL;
  sADCx.ADC_IntVRefSource    = ADC_INT_VREF_SOURCE_EXACT;
  sADCx.ADC_Prescaler        = ADC_CLK_div_512;
  sADCx.ADC_DelayGo          = 0;
  ADC1_Init (&sADCx);
  ADC2_Init (&sADCx);

  /* Enable ADC1 EOCIF and AWOIFEN interupts */
  //ADC1_ITConfig((ADCx_IT_END_OF_CONVERSION  | ADCx_IT_OUT_OF_RANGE), DISABLE);

  /* ADC1 enable */
  ADC1_Cmd (ENABLE);
  ADC2_Cmd (ENABLE);
	
	
	
	//-------------------//
/*	ADC1_SetChannel(ADC_CH_TEMP_SENSOR);
	ADC1_Start();
	while( ADC_GetFlagStatus(ADC1_FLAG_END_OF_CONVERSION)==RESET );
  temp_adc = ADC1_GetResult();
	
	ADC1_Start();
	while( ADC_GetFlagStatus(ADC1_FLAG_END_OF_CONVERSION)==RESET );
  temp_adc = ADC1_GetResult();
	
	temp_adc = temp_adc;
	*/
	
}





void TimersInit(void)
{
	TIMER_CntInitTypeDef sTIM_CntInit;
	TIMER_ChnInitTypeDef sTIM_ChnInit;
	TIMER_ChnOutInitTypeDef sTIM_ChnOutInit;
	
	//======================= TIMER1 =================================//
	// Timer1		CH2		-> BUZ+
	//				CH2N	-> BUZ-

	// Initialize timer 1 counter
	TIMER_CntStructInit(&sTIM_CntInit);
  sTIM_CntInit.TIMER_Prescaler                = 0xF;		// 1MHz at 16MHz core clk
  sTIM_CntInit.TIMER_Period                   = 500;		// Default buzzer freq
  TIMER_CntInit (MDR_TIMER1,&sTIM_CntInit);
	
	// Initialize timer 1 channel 2
  TIMER_ChnStructInit(&sTIM_ChnInit);
  sTIM_ChnInit.TIMER_CH_Mode                = TIMER_CH_MODE_PWM;
  sTIM_ChnInit.TIMER_CH_REF_Format          = TIMER_CH_REF_Format6;
  sTIM_ChnInit.TIMER_CH_Number              = TIMER_CHANNEL2;
  TIMER_ChnInit(MDR_TIMER1, &sTIM_ChnInit);
	
  // Initialize timer 1 channel 2 output
  TIMER_ChnOutStructInit(&sTIM_ChnOutInit);
  sTIM_ChnOutInit.TIMER_CH_DirOut_Polarity          = TIMER_CHOPolarity_NonInverted;
  sTIM_ChnOutInit.TIMER_CH_DirOut_Source            = TIMER_CH_OutSrc_Only_1;
  sTIM_ChnOutInit.TIMER_CH_DirOut_Mode              = TIMER_CH_OutMode_Output;
  sTIM_ChnOutInit.TIMER_CH_NegOut_Polarity          = TIMER_CHOPolarity_NonInverted;
  sTIM_ChnOutInit.TIMER_CH_NegOut_Source            = TIMER_CH_OutSrc_Only_1;
  sTIM_ChnOutInit.TIMER_CH_NegOut_Mode              = TIMER_CH_OutMode_Output;
  sTIM_ChnOutInit.TIMER_CH_Number                   = TIMER_CHANNEL2;
  TIMER_ChnOutInit(MDR_TIMER1, &sTIM_ChnOutInit);

  // Set default buzzer duty
	MDR_TIMER1->CCR2 = 250;	
  
  /* Enable TIMER1 counter clock */
  TIMER_BRGInit(MDR_TIMER1,TIMER_HCLKdiv1);

  /* Enable TIMER1 */
  TIMER_Cmd(MDR_TIMER1,ENABLE);
	
	
	
	//======================= TIMER2 =================================//
	// Timer2		CH1N	-> UPWM
	//				CH3N	-> IPWM
	
	// Initialize timer 2 counter
	TIMER_CntStructInit(&sTIM_CntInit);
  sTIM_CntInit.TIMER_Prescaler                = 0x0;		// 3906.25 Hz at 16MHz core clk
  sTIM_CntInit.TIMER_Period                   = 0xFFF;		// 12-bit 
  TIMER_CntInit (MDR_TIMER2,&sTIM_CntInit);

	// Initialize timer 2 channels 1,3
  TIMER_ChnStructInit(&sTIM_ChnInit);
  sTIM_ChnInit.TIMER_CH_Mode                = TIMER_CH_MODE_PWM;
  sTIM_ChnInit.TIMER_CH_REF_Format          = TIMER_CH_REF_Format6;
  sTIM_ChnInit.TIMER_CH_Number              = TIMER_CHANNEL1;	// voltage
  TIMER_ChnInit(MDR_TIMER2, &sTIM_ChnInit);
	sTIM_ChnInit.TIMER_CH_Number              = TIMER_CHANNEL3;	// curret
  TIMER_ChnInit(MDR_TIMER2, &sTIM_ChnInit);
	
	// Initialize timer 2 channels 1,3 output
  TIMER_ChnOutStructInit(&sTIM_ChnOutInit);
  sTIM_ChnOutInit.TIMER_CH_NegOut_Polarity          = TIMER_CHOPolarity_Inverted;
  sTIM_ChnOutInit.TIMER_CH_NegOut_Source            = TIMER_CH_OutSrc_REF;
  sTIM_ChnOutInit.TIMER_CH_NegOut_Mode              = TIMER_CH_OutMode_Output;
  sTIM_ChnOutInit.TIMER_CH_Number                   = TIMER_CHANNEL1;
  TIMER_ChnOutInit(MDR_TIMER2, &sTIM_ChnOutInit);
	sTIM_ChnOutInit.TIMER_CH_Number                   = TIMER_CHANNEL3;
  TIMER_ChnOutInit(MDR_TIMER2, &sTIM_ChnOutInit);
	
	// Set default voltage PWM duty cycle
	MDR_TIMER2->CCR1 = 0;	
	// Set default current PWM duty cycle
	MDR_TIMER2->CCR3 = 0;	
	
	/* Enable TIMER1 counter clock */
  TIMER_BRGInit(MDR_TIMER2,TIMER_HCLKdiv1);

  /* Enable TIMER1 */
  TIMER_Cmd(MDR_TIMER2,ENABLE);
	
	
	
	//======================= TIMER3 =================================//
	// Timer3		CH1 	-> LPWM (LCD backlight PWM) 
	//				CH3N	-> CPWM (System cooler PWM)
	
	// Initialize timer 3 counter
	TIMER_CntStructInit(&sTIM_CntInit);
  sTIM_CntInit.TIMER_Prescaler                = 0x7;		// 2MHz at 16MHz core clk
  sTIM_CntInit.TIMER_Period                   = 100;		// 20kHz at 1MHz
  TIMER_CntInit (MDR_TIMER3,&sTIM_CntInit);
	
	//-----------------------------//
	//-----------------------------//
	
  // Initialize timer 3 channel 1
  TIMER_ChnStructInit(&sTIM_ChnInit);
	sTIM_ChnInit.TIMER_CH_Number              = TIMER_CHANNEL1;
  sTIM_ChnInit.TIMER_CH_Mode                = TIMER_CH_MODE_PWM;
  sTIM_ChnInit.TIMER_CH_REF_Format          = TIMER_CH_REF_Format6;
  TIMER_ChnInit(MDR_TIMER3, &sTIM_ChnInit);

  // Initialize timer 3 channel 1 output
  TIMER_ChnOutStructInit(&sTIM_ChnOutInit);
	sTIM_ChnOutInit.TIMER_CH_Number                   = TIMER_CHANNEL1;
  sTIM_ChnOutInit.TIMER_CH_DirOut_Source            = TIMER_CH_OutSrc_REF;
	sTIM_ChnOutInit.TIMER_CH_DirOut_Polarity          = TIMER_CHOPolarity_NonInverted;
  sTIM_ChnOutInit.TIMER_CH_DirOut_Mode              = TIMER_CH_OutMode_Output;
  TIMER_ChnOutInit(MDR_TIMER3, &sTIM_ChnOutInit);

  //-----------------------------//
	
  // Initialize timer 3 channel 3
	sTIM_ChnInit.TIMER_CH_Number              = TIMER_CHANNEL3;
  TIMER_ChnInit(MDR_TIMER3, &sTIM_ChnInit);
	
  // Initialize timer 3 channel 3 output
	sTIM_ChnOutInit.TIMER_CH_Number           	= TIMER_CHANNEL3;
  sTIM_ChnOutInit.TIMER_CH_DirOut_Source      = TIMER_CH_OutSrc_Only_0;
  sTIM_ChnOutInit.TIMER_CH_DirOut_Mode        = TIMER_CH_OutMode_Input;
	sTIM_ChnOutInit.TIMER_CH_NegOut_Source     	= TIMER_CH_OutSrc_REF;
	sTIM_ChnOutInit.TIMER_CH_NegOut_Polarity   	= TIMER_CHOPolarity_Inverted;
	sTIM_ChnOutInit.TIMER_CH_NegOut_Mode       	= TIMER_CH_OutMode_Output;
  TIMER_ChnOutInit(MDR_TIMER3, &sTIM_ChnOutInit);
	
	//-----------------------------//

  // Set default PWM duty cycle for LCD backlight PWM	
	MDR_TIMER3->CCR1 = 0;
	
	// Set default PWM duty cycle for system cooler PWM
	MDR_TIMER3->CCR3 = 0;

  /* Enable TIMER3 counter clock */
  TIMER_BRGInit(MDR_TIMER3,TIMER_HCLKdiv1);

  /* Enable TIMER3 */
  TIMER_Cmd(MDR_TIMER3,ENABLE);
	

}







void SetVoltagePWMPeriod(uint16_t new_period)
{
	MDR_TIMER2->CCR1 = new_period;	
}

void SetCurrentPWMPeriod(uint16_t new_period)
{
	MDR_TIMER2->CCR3 = new_period;	
}


void SetBuzzerFreq(uint16_t freq)
{
	MDR_TIMER1->ARR = freq;	
	MDR_TIMER1->CCR2 = freq/2;		// 50% duty
}



void SetCoolerSpeed(uint16_t speed)
{
	MDR_TIMER3->CCR3 = speed;
}





//==============================================================//
// Set LCD backlight intense
// 0 to 100
//==============================================================//
void LcdSetBacklight(uint16_t value)
{
	if (value>100) value = 100;
		MDR_TIMER3->CCR1 = value;
}



void ProcessPowerOff(void)
{
	 //if (system_status.LineInStatus == OFFLINE)
	 if (GetACLineStatus() == OFFLINE)
	 {	
		 SetConverterState(CONVERTER_OFF);		// safe because we're stopping in this function
		 
		 SysTickStop();
		 StopBeep();
		 
		 DWTStartDelayUs(5000);
		 
		 LcdSetBacklight(0);
		 SetCoolerSpeed(0);
		 SetVoltagePWMPeriod(0);
		 SetCurrentPWMPeriod(0);
		 
		 
		 // Put message
		 LcdFillBuffer(lcd0_buffer,0);
		 LcdFillBuffer(lcd1_buffer,0);
		 LcdPutNormalStr(0,10,"Power OFF",(tNormalFont*)&font_8x12,lcd0_buffer);
		 LcdPutNormalStr(0,10,"Power OFF",(tNormalFont*)&font_8x12,lcd1_buffer);
		 
		 
		 
		 while(DWTDelayInProgress());
		 
		 SetFeedbackChannel(CHANNEL_12V);
		 SetCurrentLimit(CURRENT_LIM_HIGH); 
		 SetOutputLoad(LOAD_DISABLE); 
		  
		 LcdUpdateByCore(LCD0,lcd0_buffer);
		 LcdUpdateByCore(LCD1,lcd1_buffer);
		 
		 
		 DWTDelayUs(1000);
		 PORT_DeInit(MDR_PORTA);
		 PORT_DeInit(MDR_PORTB);
		 PORT_DeInit(MDR_PORTC);
		 PORT_DeInit(MDR_PORTD);
		 PORT_DeInit(MDR_PORTE);
		 PORT_DeInit(MDR_PORTF);
		 
		 while(1);
		 
	 }
 }
	
	 
	


	 
void ProcessTemperature(void)
{
	static uint8_t fsm_state = TSTATE_START_EXTERNAL;
	static uint16_t adc_samples;
	static uint16_t sample_counter = 0;
	
	switch (fsm_state)
	{
		case TSTATE_START_EXTERNAL:
			ADC2_SetChannel(ADC_CHANNEL_CONVERTER_TEMP); 
			DWTDelayUs(10);
			ADC2_Start();
			fsm_state = TSTATE_GET_EXTERNAL;
			adc_samples = 0;
			sample_counter = 9;
			break;
		case TSTATE_GET_EXTERNAL:
			adc_samples += ADC2_GetResult();
			if (sample_counter != 0)
			{
				ADC2_Start();
				sample_counter--;
			}
			else
			{
				fsm_state = TSTATE_DONE_EXTERNAL;
			}
			break;
		case TSTATE_DONE_EXTERNAL:
			converter_temp_celsius = (int16_t)( (float)adc_samples*0.1*(-0.226) + 230 );	//FIXME
			fsm_state = TSTATE_START_EXTERNAL;
			break;
		
		default:
			fsm_state = TSTATE_START_EXTERNAL;
	}
	
	
		/*
		ADC2_SetChannel(ADC_CHANNEL_CONVERTER_TEMP); 
		DWTDelayUs(10);
		ADC2_Start();
		while( ADC_GetFlagStatus(ADC2_FLAG_END_OF_CONVERSION)==RESET );
		converter_temp = ADC2_GetResult();
		
		temperature_acc += converter_temp;
		
		if(	temperature_cnt >= 9)
		{
			temperature_cnt = 0;
			converter_temp_norm =  (float)temperature_acc*0.1*(-0.226) + 230 ;
			temperature_acc = 0;
		}
		else
		{
			temperature_cnt++;
		}
		*/
}	
	 

void ProcessCooler(void)
{
	uint16_t cooler_speed;
	cooler_speed = (converter_temp_celsius < 25) ? 50 : converter_temp_celsius * 2;
	SetCoolerSpeed(cooler_speed);
}
	


	





