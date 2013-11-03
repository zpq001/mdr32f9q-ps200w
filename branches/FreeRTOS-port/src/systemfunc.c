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








//-----------------------------------------------------------------//
// Setup clocks
// CPU core clock (HCLK) = 32 MHz clock from 4 MHz HSE
// ADC clock = 4 MHz clock from 4 MHz HSE
//-----------------------------------------------------------------//
void Setup_CPU_Clock(void)
{
	// Enable HSE
	RST_CLK_HSEconfig(RST_CLK_HSE_ON);
	if (RST_CLK_HSEstatus() != SUCCESS)
	{
		while (1) {}	// Trap
	}
	
	//-------------------------------//
	// Setup CPU PLL and CPU_C1_SEL
	// CPU_C1 = HSE,	PLL = x8
	RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, RST_CLK_CPU_PLLmul8);
	RST_CLK_CPU_PLLcmd(ENABLE);
	if (RST_CLK_CPU_PLLstatus() != SUCCESS)
	{
		while (1) {}	// Trap
	}
	// Setup CPU_C2 and CPU_C3
	// CPU_C3 = CPU_C2
	RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);
	// CPU_C2 = CPU PLL output
	RST_CLK_CPU_PLLuse(ENABLE);
	// Switch to CPU_C3
	// HCLK = CPU_C3
	RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);
	
	//-------------------------------//
	// Setup ADC clock
	// ADC_C2 = CPU_C1
	RST_CLK_ADCclkSelection(RST_CLK_ADCclkCPU_C1);
	// ADC_C3 = ADC_C2
	RST_CLK_ADCclkPrescaler(RST_CLK_ADCclkDIV1);
	// Enable ADC_CLK
	RST_CLK_ADCclkEnable(ENABLE);
	
	// Update system clock variable
	SystemCoreClockUpdate();
	
	// Enable clock on all ports (macro are defined in systemfunc.h)
	RST_CLK_PCLKcmd(ALL_PORTS_CLK, ENABLE);
	// Enable clock on peripheral blocks used in design
	RST_CLK_PCLKcmd(PERIPHERALS_CLK ,ENABLE);
}



//-----------------------------------------------------------------//
// Setup IO Ports
//-----------------------------------------------------------------//
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
	
	//================= PORTA =================//
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
	
	// debug
	PORT_StructInit(&PORT_InitStructure);
	PORT_InitStructure.PORT_Pin   = (1<<TXD1) | (1<<RXD1);
	PORT_InitStructure.PORT_OE    = PORT_OE_OUT;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_SLOW;
	PORT_InitStructure.PORT_MODE  = PORT_MODE_DIGITAL;
	PORT_Init(MDR_PORTA, &PORT_InitStructure);

	//================= PORTB =================//
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

	
	//================= PORTC =================//
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
	
	
	//================= PORTD =================//
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
	

	
	//================= PORTE =================//
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
	
	
	
	//================= PORTF =================//
	PORT_StructInit(&PORT_InitStructure);
	
	// Feedback channel select, converter enable
	PORT_InitStructure.PORT_Pin   = (1<<EN) | (1<<STAB_SEL) ;
	PORT_InitStructure.PORT_MODE  = PORT_MODE_DIGITAL;
	PORT_InitStructure.PORT_SPEED = PORT_SPEED_SLOW;
	PORT_InitStructure.PORT_OE    = PORT_OE_OUT;
	PORT_Init(MDR_PORTF, &PORT_InitStructure);
	
	// TODO: add USART2 functions	
	
}


//-----------------------------------------------------------------//
// Setup SSP module
// HCLK = 32 MHz
// The information rate is computed using the following formula:
//		F_SSPCLK / ( CPSDVR * (1 + SCR) )
// 3.2 MHz
//-----------------------------------------------------------------//
void SSPInit(void)
{
	SSP_InitTypeDef sSSP;
	SSP_StructInit (&sSSP);

	SSP_BRGInit(MDR_SSP2,SSP_HCLKdiv1);		// F_SSPCLK = HCLK / 1
	
	sSSP.SSP_SCR  = 0x04;		// 0 to 255
	sSSP.SSP_CPSDVSR = 2;		// even 2 to 254
	sSSP.SSP_Mode = SSP_ModeMaster;
	sSSP.SSP_WordLength = SSP_WordLength9b;
	sSSP.SSP_SPH = SSP_SPH_1Edge;
	sSSP.SSP_SPO = SSP_SPO_Low;
	sSSP.SSP_FRF = SSP_FRF_SPI_Motorola;
	sSSP.SSP_HardwareFlowControl = SSP_HardwareFlowControl_SSE;
	SSP_Init (MDR_SSP2,&sSSP);

	SSP_Cmd(MDR_SSP2, ENABLE);
}

//-----------------------------------------------------------------//
// Setup I2C module
// HCLK = 32 MHz
// 125 kHz
//-----------------------------------------------------------------//
void I2CInit(void)
{
	//--------------- I2C INIT ---------------//
I2C_InitTypeDef I2C_InitStruct;
	
	// Enables I2C peripheral
	I2C_Cmd(ENABLE);
	
	// Initialize I2C_InitStruct
	I2C_InitStruct.I2C_ClkDiv = 256;		// 0x0000 to 0xFFFF
	I2C_InitStruct.I2C_Speed = I2C_SPEED_UP_TO_400KHz;

	// Configure I2C parameters
	I2C_Init(&I2C_InitStruct);
}


//-----------------------------------------------------------------//
//	Setup ADC
//	ADC_CLK = 4 MHz
//	ADC single conversion time is ADC_StartDelay + 
//		(29 to 36 ADC clock cycles depending on ADC_DelayGo)
//	ADC clock = 4MHz / 128 = 31.250kHz (T = 32uS)
//-----------------------------------------------------------------//
void ADCInit(void)
{
	ADC_InitTypeDef sADC;
	ADCx_InitTypeDef sADCx;
	
	// ADC Configuration
	// Reset all ADC settings
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

	// ADC1 Configuration 
	ADCx_StructInit (&sADCx);
	sADCx.ADC_ClockSource      = ADC_CLOCK_SOURCE_ADC;
	sADCx.ADC_SamplingMode     = ADC_SAMPLING_MODE_SINGLE_CONV;
	sADCx.ADC_ChannelSwitching = ADC_CH_SWITCHING_Disable;
	sADCx.ADC_ChannelNumber    = ADC_CH_TEMP_SENSOR;		
	sADCx.ADC_Channels         = 0;
	sADCx.ADC_LevelControl     = ADC_LEVEL_CONTROL_Disable;
	sADCx.ADC_LowLevel         = 0;
	sADCx.ADC_HighLevel        = 0;
	sADCx.ADC_VRefSource       = ADC_VREF_SOURCE_EXTERNAL;
	sADCx.ADC_IntVRefSource    = ADC_INT_VREF_SOURCE_EXACT;
	sADCx.ADC_Prescaler        = ADC_CLK_div_128;
	sADCx.ADC_DelayGo          = 0;		// CHECKME
	ADC1_Init (&sADCx);
	ADC2_Init (&sADCx);

	// Disable ADC interupts
	ADC1_ITConfig((ADCx_IT_END_OF_CONVERSION  | ADCx_IT_OUT_OF_RANGE), DISABLE);
	ADC2_ITConfig((ADCx_IT_END_OF_CONVERSION  | ADCx_IT_OUT_OF_RANGE), DISABLE);

	// ADC1 enable
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




//-----------------------------------------------------------------//
// Setup Timers
// HCLK = 32 MHz
// TIMER_CLK = HCLK / TIMER_HCLKdivx
// CLK = TIMER_CLK/(TIMER_Prescaler + 1) 
//-----------------------------------------------------------------//
void TimersInit(void)
{
	TIMER_CntInitTypeDef sTIM_CntInit;
	TIMER_ChnInitTypeDef sTIM_ChnInit;
	TIMER_ChnOutInitTypeDef sTIM_ChnOutInit;
	
	//======================= TIMER1 =======================//
	// Timer1		CH2		-> BUZ+
	//				CH2N	-> BUZ-
	// TIMER_CLK = HCLK
	// CLK = 1MHz
	// Default buzzer freq = 1 / 500us = 2kHz
	
	// Initialize timer 1 counter
	TIMER_CntStructInit(&sTIM_CntInit);
	sTIM_CntInit.TIMER_Prescaler                = 0x1F;		// 32MHz / (31 + 1) = 1MHz
	sTIM_CntInit.TIMER_Period                   = 499;		
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
	MDR_TIMER1->CCR2 = 249;	
  
	// Enable TIMER1 counter clock
	TIMER_BRGInit(MDR_TIMER1,TIMER_HCLKdiv1);

	// Enable TIMER1
	TIMER_Cmd(MDR_TIMER1,ENABLE);
	
	
	
	//======================= TIMER2 =======================//
	// Timer2		CH1N	-> UPWM
	//				CH3N	-> IPWM
	// 				CH2		-> HW control interrupt generation
	// TIMER_CLK = HCLK
	// CLK = 16MHz
	// PWM frequency = 3906.25 Hz (T = 256us)
	// PWM resolution = 12 bit
	
	// Initialize timer 2 counter
	TIMER_CntStructInit(&sTIM_CntInit);
	sTIM_CntInit.TIMER_Prescaler                = 0x1;		// CLK = 16MHz
	sTIM_CntInit.TIMER_Period                   = 0xFFF;	// 16MHz / 4096 = 3906.25 Hz 
	TIMER_CntInit (MDR_TIMER2,&sTIM_CntInit);

	// Initialize timer 2 channels 1,3
	TIMER_ChnStructInit(&sTIM_ChnInit);
	sTIM_ChnInit.TIMER_CH_Mode                = TIMER_CH_MODE_PWM;
	sTIM_ChnInit.TIMER_CH_REF_Format          = TIMER_CH_REF_Format6;
	sTIM_ChnInit.TIMER_CH_Number              = TIMER_CHANNEL1;			// voltage
	TIMER_ChnInit(MDR_TIMER2, &sTIM_ChnInit);
	sTIM_ChnInit.TIMER_CH_Number              = TIMER_CHANNEL3;			// curret
	TIMER_ChnInit(MDR_TIMER2, &sTIM_ChnInit);
	
	// Initialize timer 2 channel 2 - used for HW control interrupt generation
	TIMER_ChnStructInit(&sTIM_ChnInit);
	sTIM_ChnInit.TIMER_CH_Mode                = TIMER_CH_MODE_PWM;
	sTIM_ChnInit.TIMER_CH_REF_Format          = TIMER_CH_REF_Format1;	// REF output = 1 when CNT == CCR
	sTIM_ChnInit.TIMER_CH_Number              = TIMER_CHANNEL2;
	sTIM_ChnInit.TIMER_CH_CCR_UpdateMode      = TIMER_CH_CCR_Update_Immediately;
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
	// Set default CCR for interrupt generation
	MDR_TIMER2->CCR2 = 0;
	
	// Enable interrupts
	TIMER_ITConfig(MDR_TIMER2, TIMER_STATUS_CCR_REF_CH2, ENABLE);
	//TIMER_ITConfig(MDR_TIMER2, TIMER_STATUS_CNT_ZERO, ENABLE);
	
	// Enable TIMER2 counter clock
	TIMER_BRGInit(MDR_TIMER2,TIMER_HCLKdiv1);

	// Enable TIMER2
	TIMER_Cmd(MDR_TIMER2,ENABLE);
	
	
	
	//======================= TIMER3 =======================//
	// Timer3		CH1 	-> LPWM (LCD backlight PWM) 
	//				CH3N	-> CPWM (System cooler PWM)
	// TIMER_CLK = HCLK
	// CLK = 2MHz
	// PWM frequency = 20kHz
	// PWM resolution = 100
	
	// Initialize timer 3 counter
	TIMER_CntStructInit(&sTIM_CntInit);
	sTIM_CntInit.TIMER_Prescaler                = 0xF;		// 2MHz at 32MHz core clk
	sTIM_CntInit.TIMER_Period                   = 99;			// 20kHz at 2MHz
	TIMER_CntInit (MDR_TIMER3,&sTIM_CntInit);
		
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


	// Set default PWM duty cycle for LCD backlight PWM	
	MDR_TIMER3->CCR1 = 0;
	
	// Set default PWM duty cycle for system cooler PWM
	MDR_TIMER3->CCR3 = 0;

	// Enable TIMER3 counter clock
	TIMER_BRGInit(MDR_TIMER3,TIMER_HCLKdiv1);

	// Enable TIMER3
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
	uint32_t time_delay;
	 //if (system_status.LineInStatus == OFFLINE)
	 if (GetACLineStatus() == OFFLINE)
	 {	
		 __disable_irq();
		 
		 SetConverterState(CONVERTER_OFF);		// safe because we're stopping in this function
		 
		 SysTickStop();
		 StopBeep();
		 
		 time_delay = DWTStartDelayUs(5000);
		 
		 LcdSetBacklight(0);
		 SetCoolerSpeed(0);
		 SetVoltagePWMPeriod(0);
		 SetCurrentPWMPeriod(0);
		 
		 
		 // Put message
		 LcdFillBuffer(lcd0_buffer,0);
		 LcdFillBuffer(lcd1_buffer,0);
		 LcdPutNormalStr(0,10,"Power OFF",(tNormalFont*)&font_8x12,lcd0_buffer);
		 LcdPutNormalStr(0,10,"Power OFF",(tNormalFont*)&font_8x12,lcd1_buffer);
		 
		 
		 
		 while(DWTDelayInProgress(time_delay));
		 
		 SetFeedbackChannel(CHANNEL_12V);
		 SetCurrentLimit(CURRENT_LIM_HIGH); 
		 SetOutputLoad(LOAD_DISABLE); 
		  
		 LcdUpdateByCore(LCD0,lcd0_buffer);
		 LcdUpdateByCore(LCD1,lcd1_buffer);
		 
		 
		 
		 time_delay = DWTStartDelayUs(1000);
		 while(DWTDelayInProgress(time_delay));
		 
		 PORT_DeInit(MDR_PORTA);
		 PORT_DeInit(MDR_PORTB);
		 PORT_DeInit(MDR_PORTC);
		 PORT_DeInit(MDR_PORTD);
		 PORT_DeInit(MDR_PORTE);
		 PORT_DeInit(MDR_PORTF);
		 
		 while(1);
		 
	 }
 }
	
	 
	





