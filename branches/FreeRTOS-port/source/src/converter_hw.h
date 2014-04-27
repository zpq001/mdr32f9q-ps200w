
#include "MDR32Fx.h" 


//-------------------------------------------------------//

// cmd_ADC_to_HWProcess bits
#define CMD_HW_OFF_BY_ADC		0x01
#define CMD_HW_ON_BY_ADC		0x02

// shutdown to HW process bits
#define CMD_HW_OFF_SHUTDOWN		0x01


// Low-level converter driver return values
#define CONVERTER_CMD_OK			0x10
#define CONVERTER_IS_NOT_READY		0x11
#define CONVERTER_IS_OVERLOADED		0x12
#define CONVERTER_ILLEGAL_ON_STATE	0x13



extern volatile uint8_t cmd_shutdown_to_HWProcess;

void SetVoltageDAC(uint16_t val);
void SetCurrentDAC(uint16_t val, uint8_t current_range);

uint8_t Converter_IsReady(void);

uint8_t Converter_SetFeedBackChannel(uint8_t channel);
uint8_t Converter_SetCurrentRange(uint8_t range);
uint8_t Converter_TurnOn(void);
uint8_t Converter_TurnOff(void);
uint8_t Converter_ClearOverloadFlag(void);

void Converter_HWProcess(void);

