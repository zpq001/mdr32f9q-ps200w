
#include "MDR32F9Qx_dma.h"

// All ports
#define ALL_PORTS_CLK (RST_CLK_PCLK_PORTA | RST_CLK_PCLK_PORTB | \
                       RST_CLK_PCLK_PORTC | RST_CLK_PCLK_PORTD | \
                       RST_CLK_PCLK_PORTE | RST_CLK_PCLK_PORTF)
											
// Peripheral blocks, used in design											
#define PERIPHERALS_CLK 		(	RST_CLK_PCLK_SSP2 | RST_CLK_PCLK_TIMER1 | \
									RST_CLK_PCLK_TIMER2 | RST_CLK_PCLK_TIMER3 | \
									RST_CLK_PCLK_I2C | RST_CLK_PCLK_ADC | RST_CLK_PCLK_UART1 | RST_CLK_PCLK_UART2 | RST_CLK_PCLK_DMA)




extern uint8_t analyze_shutdown;


void my_DMA_GlobalInit(void);
void my_DMA_ChannelInit(uint8_t DMA_Channel, DMA_ChannelInitTypeDef *DMA_ChannelInitStruct);

void Setup_CPU_Clock(void);
void HW_NVIC_init(void);
void HW_PortInit(void);
void HW_UARTInit(void);
void HW_UART_Set_Comm_Params(MDR_UART_TypeDef* UARTx, uint32_t buadRate, uint8_t parity);
void HW_SSPInit(void);
void HW_I2CInit(void);
void HW_ADCInit(void);
void HW_TimersInit(void);
void HW_DMAInit(void);

void LcdSetBacklight(uint16_t value);

void SetVoltagePWMPeriod(uint16_t new_period);
void SetCurrentPWMPeriod(uint16_t new_period);

void SetCoolerSpeed(uint16_t speed);


void DelayUs(uint16_t delay);

void SetBuzzerFreq(uint16_t freq);




void ProcessOverload(void);
void ProcessPowerOff(void);


//void ProcessTemperature(void);
//void ProcessCooler(void);
