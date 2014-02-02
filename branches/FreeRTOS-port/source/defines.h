

/*
  LCD port map:

	LCD_RST 	 					PORTE 0
	LCD_SEL 	 					PORTE 2
	LCD backlight PWM 	 			PORTC 2
	
	LCD_MOSI 						PORTD 6 (SSP2 MOSI)
	LCD_CLK 	 					PORTD 5 (SSP2 CLK)
	LCD_CS 	 						PORTD 3 (SSP2 FSS)
*/


/*
		Timers used in design:
		
		TMR1 		CH2		-> BUZ
		
		TMR2		CH1N	-> UPWM
					CH3N	-> IPWM
		
		TMR3 		CH1 	-> LPWM
					CH3N	-> CPWM
*/

/*
		Buttons, LEDs, etc. pin map:
		
		ESC/EXIT		PORTB 	0
		SB-/dec			PORTB	1
		SB+/inc			PORTB	2
		OK/MENU			PORTB	4
		
		SB_ON/LGREEN	PORTB	7
		SB_OFF/LRED		PORTB	6
		
		BUZ1			PORTA	3
		BUZ2			PORTA	4
		
		ENCA			PORTB	8
		ENCB			PORTB	9
		ENCBTN			PORTA	5
		
		SWITCH			PORTB	5
*/

/*
		I2C eeprom
		
		SCL				PORTC	0
		SDA				PORTC	1

*/

//--------- pin defines for PORTA ----------//
#define OVERLD		0		// [in]		overload
#define CLIM_SEL	1		// [out] 	current amplifier gain select
#define EEN			2		// [in]		external ON/OFF switch
#define BUZ1		3		// [out] 	buzzer
#define BUZ2		4		// [out] 	buzzer
#define ENC_BTN		5		// [in] 	Encoder push button
#define RXD1		6		// [in]		USART1 RXD
#define TXD1		7		// [out]	USART1 TXD

//--------- pin defines for PORTB ----------//
#define SB_ESC		0		// [in]
#define SB_LEFT		1		// [in]
#define SB_RIGHT	2		// [in]
#define SB_OK		4		// [in]
#define SB_MODE		5		// [in] 		front-panel switch
#define LRED		6		// [in / out] 	also SB_OFF
#define LGREEN		7		// [in / out] 	also SB_ON
#define ENCA		8		// [in] 		incremental encoder pin A
#define ENCB		9		// [in] 		incremental encoder pin B
#define PG			10		// [in] 		not POWER GOOD

//--------- pin defines for PORTC ----------//
#define	SCL			0		// [out]		I2C clock
#define	SDA			1		// [in / out]	I2C data
#define LCD_BL 		2		// [out] 		LCD backlight PWM

//--------- pin defines for PORTD ----------//
#define VREF_P		0		// [in, analog] external reference +
#define VREF_N		1		// [in, analog] external reference -
#define TEMP_IN		2		// [in, analog]	temperature sense input
#define LCD_CS 		3		// [out] 		LCD chip select
#define UADC		4		// [in, analog]	voltage input
#define LCD_CLK 	5		// [out] 		LCD clock
#define LCD_MOSI 	6		// [out] 		LCD data
#define IADC			7	// [in, analog]	current input

//--------- pin defines for PORTE ----------//
#define LCD_RST 	0		// [out]	LCD reset
#define UPWM			1	// [out] 	voltage PWM
#define LCD_SEL 	2		// [out] 	LCD select
#define IPWM			3	// [out]	current PWM
#define LDIS			6	// [out]	12V channel load disable
#define CPWM			7	// [out]	system cooler PWM

//--------- pin defines for PORTF ----------//
#define RXD2			0	// [in]		USART2 RXD
#define	TXD2			1	// [out]	USRAT2 TXD
#define EN				2	// [out]	converter enable
#define STAB_SEL	3		// [out]	feedback channel select



//--------- ADC channels defines -----------//
#define ADC_CHANNEL_VOLTAGE 				ADC_CH_ADC4
#define ADC_CHANNEL_CURRENT 				ADC_CH_ADC7
#define ADC_CHANNEL_CONVERTER_TEMP 			ADC_CH_ADC2
#define ADC_CHANNEL_CPU_TEMP				ADC_CH_TEMP_SENSOR














