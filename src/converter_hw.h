
#include "MDR32Fx.h" 




//-------------------------------------------------------//
// Overload 
#define OVERLOAD_IGNORE_TIMEOUT		(5*100)		// 100 ms	- delay after converter turn ON and first overload check
//#define OVERLOAD_TIMEOUT			5			// 1 ms
#define MINIMAL_OFF_TIME			(5*40)		// 40 ms
#define USER_TIMEOUT				(5*20)		// 20 ms	- used for delay between channel switch and ON command
#define LED_BLINK_TIMEOUT			(5*100)		// 100ms	- used for charging indication
#define OVERLOAD_WARNING_TIMEOUT	(5*200)		// 200ms	- used for sound overload warning

#define MIN_OVERLOAD_TIMEOUT		0
#define MAX_OVERLOAD_TIMEOUT		(5*10)



//-------------------------------------------------------//
// ctrl_ADCProcess bits
// For ADC task
#define CMD_ADC_START_VOLTAGE		0x01
#define CMD_ADC_START_CURRENT		0x02
#define CMD_ADC_START_DISCON		0x04




#define CONVERTER_CMD_OK			0x10
#define CONVERTER_IS_NOT_READY		0x11
#define CONVERTER_IS_OVERLOADED		0x12





void SetVoltageDAC(uint16_t val);
void SetCurrentDAC(uint16_t val, uint8_t current_range);

void Converter_SetFeedBackChannel(uint8_t channel);
void Converter_SetCurrentRange(uint8_t range);

uint8_t Converter_IsReady(void);
uint8_t Converter_IsOn(void);

uint8_t Converter_TurnOn(void);
uint8_t Converter_TurnOff(void);

