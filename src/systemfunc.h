
// All ports
#define ALL_PORTS_CLK (RST_CLK_PCLK_PORTA | RST_CLK_PCLK_PORTB | \
                       RST_CLK_PCLK_PORTC | RST_CLK_PCLK_PORTD | \
                       RST_CLK_PCLK_PORTE | RST_CLK_PCLK_PORTF)
											
// Peripheral blocks, used in design											
#define PERIPHERALS_CLK 		(	RST_CLK_PCLK_SSP2 | RST_CLK_PCLK_TIMER1 | \
									RST_CLK_PCLK_TIMER2 | RST_CLK_PCLK_TIMER3 | \
									RST_CLK_PCLK_I2C | RST_CLK_PCLK_ADC )




//#define TSTATE_START_EXTERNAL	0x01		// ADC FSM states
//#define TSTATE_GET_EXTERNAL		0x02
//#define	TSTATE_DONE_EXTERNAL	0x03




void Setup_CPU_Clock(void);
void PortInit(void);
void SSPInit(void);
void ADCInit(void);

void LcdSetBacklight(uint16_t value);

void SetVoltagePWMPeriod(uint16_t new_period);
void SetCurrentPWMPeriod(uint16_t new_period);

void SetCoolerSpeed(uint16_t speed);

void TimersInit(void);
void DelayUs(uint16_t delay);

void SetBuzzerFreq(uint16_t freq);

void I2CInit(void);


void ProcessOverload(void);
void ProcessPowerOff(void);


//void ProcessTemperature(void);
//void ProcessCooler(void);
