

#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"


#define CONV_MAX_VOLTAGE_5V_CHANNEL		10000	// [mV]
#define CONV_MAX_VOLTAGE_12V_CHANNEL	20000	// [mV]
#define CONV_MIN_VOLTAGE_5V_CHANNEL		0		// [mV]
#define CONV_MIN_VOLTAGE_12V_CHANNEL	0		// [mV]

// Common for both channels
#define CONV_LOW_CURRENT_RANGE_MAX		20000	// [mA]
#define CONV_LOW_CURRENT_RANGE_MIN		0		// [mA]
#define CONV_HIGH_CURRENT_RANGE_MAX		40000	// [mA]
#define CONV_HIGH_CURRENT_RANGE_MIN		0		// [mA]

/*
// CheckSetVoltageRange error codes			// TODO: bring into *.c file
#define VCHECK_OK				0x01
#define VCHECK_ABS_MAX			0x02	// ABS limits have greater priority
#define VCHECK_ABS_MIN			0x03
#define VCHECK_SOFT_MAX			0x04
#define VCHECK_SOFT_MIN			0x05

// CheckSetCurrentRange error codes
#define CCHECK_OK				0x08
#define CCHECK_ABS_MAX			0x09	// ABS limits have greater priority
#define CCHECK_ABS_MIN			0x0A
#define CCHECK_SOFT_MAX			0x0B
#define CCHECK_SOFT_MIN			0x0C

//Converter_SetSoftLimit error codes
#define SLIM_OK					0x10
#define SLIM_MIN				0x11
#define SLIM_MAX				0x12

//Converter_SetSoftLimit mode codes
#define SET_LOW_VOLTAGE_LIMIT	0x01
#define SET_HIGH_VOLTAGE_LIMIT	0x02
#define SET_LOW_CURRENT_LIMIT	0x03
#define SET_HIGH_CURRENT_LIMIT	0x04
*/
#define LIMIT_TYPE_LOW			0x00
#define LIMIT_TYPE_HIGH			0x01


//TODO - move it to some common for all tasks header
#define TID_UART		0x01
#define TID_GUI			0x02

//---------------------------------------------//
// Converter_HWProcess

// state_HWProcess bits 
#define STATE_HW_ON				0x01
#define STATE_HW_OFF			0x02
#define STATE_HW_OVERLOADED		0x04
#define STATE_HW_OFF_BY_ADC		0x08
#define STATE_HW_TIMER_NOT_EXPIRED	0x10
#define STATE_HW_USER_TIMER_EXPIRED	0x20

// ctrl_HWProcess bits
#define CMD_HW_ON				0x01
#define CMD_HW_OFF				0x02
#define CMD_HW_RESET_OVERLOAD	0x04
#define CMD_HW_RESTART_USER_TIMER	0x08
#define CMD_HW_RESTART_LED_BLINK_TIMER 0x10

// cmd_ADC_to_HWProcess bits
#define CMD_HW_OFF_BY_ADC		0x01
#define CMD_HW_ON_BY_ADC		0x02

//---------------------------------------------//
// Converter_HW_ADCProcess

// state_ADCProcess states
#define STATE_ADC_IDLE				0x00
#define STATE_ADC_DISPATCH			0x01
#define STATE_ADC_NORMAL_START_U	0x02
#define STATE_ADC_NORMAL_REPEAT_U	0x03
#define STATE_ADC_START_I			0x10
#define STATE_ADC_NORMAL_REPEAT_I	0x11

// ctrl_ADCProcess bits
#define CMD_ADC_START_VOLTAGE		0x01
#define CMD_ADC_START_CURRENT		0x02
#define CMD_ADC_START_DISCON		0x04

//---------------------------------------------//
// Other

// Extended converter channel definition - used when
// currently operating channel should be updated
#define OPERATING_CHANNEL 2

// Extended converter current range definition - used when
// currently operating range should be updated
#define OPERATING_CURRENT_RANGE 2




#define HW_IRQ_PERIOD				(16*200)		// in units of 62.5ns, must be <= timer period
#define HW_ADC_CALL_PERIOD			5				// in units of HW_IRQ period
#define HW_UART2_RX_CALL_PERIOD		5				// in units of HW_IRQ period
#define HW_UART2_TX_CALL_PERIOD		5				// in units of HW_IRQ period



#define CONV_OFF				0x01
#define CONV_ON					0x02
#define CONV_OVERLOAD			0x10
#define CONV_STATE_MASK			0x0F

#define CMD_FB_5V			0x0001
#define CMD_FB_12V			0x0002
#define CMD_CLIM_20A		0x0004
#define CMD_CLIM_40A		0x0008
#define CMD_ON				0x0010
#define CMD_OFF				0x0020

#define OVERLOAD_IGNORE_TIMEOUT		(5*100)		// 100 ms	- delay after converter turn ON and first overload check
//#define OVERLOAD_TIMEOUT			5			// 1 ms
#define MINIMAL_OFF_TIME			(5*40)		// 40 ms
#define USER_TIMEOUT				(5*20)		// 20 ms	- used for delay between channel switch and ON command
#define LED_BLINK_TIMEOUT			(5*100)		// 100ms	- used for charging indication
#define OVERLOAD_WARNING_TIMEOUT	(5*200)		// 200ms	- used for sound overload warning

#define MIN_OVERLOAD_TIMEOUT		0
#define MAX_OVERLOAD_TIMEOUT		(5*10)

// These defines set behavior of controller in case when 
// error status is generated simultaneously with processing OFF command
// Either must be set
#define CMD_HAS_PRIORITY 	1		// - no error will be shown
#define ERROR_HAS_PRIORITY	0		// - error will be shown

#if ((CMD_HAS_PRIORITY && ERROR_HAS_PRIORITY) || (!CMD_HAS_PRIORITY && !ERROR_HAS_PRIORITY))
#error "Either CMD_HAS_PRIORITY or ERROR_HAS_PRIORITY options must be set"
#endif 

//---------------------------------------------//
// Task queue messages


#pragma anon_unions

typedef struct {
    uint16_t type;
	uint16_t sender;
    union {
        struct {
            uint32_t a;
            uint32_t b;
        } data;
		//----- voltage -----//
		struct {
			uint8_t channel;
			int32_t value;
		} voltage_setting;
        struct {
			uint8_t channel;
            uint8_t type;
            uint8_t enable;
            int32_t value;
        } voltage_limit_setting;
		//----- current -----//
		struct {
			uint8_t channel;
			uint8_t range;
			int32_t value;
		} current_setting;
		struct {
			uint8_t channel;		// channel to affect
			uint8_t range;			// 20A (low) or 40A (high)
			uint8_t type;			// min or max limit
			uint8_t enable;			// enable/disable limit check
			int32_t value;			// new value
		} current_limit_setting;
		struct {
			uint8_t channel;
			uint8_t new_range;
		} current_range_setting;
		struct {
			uint8_t new_channel;
		} channel_setting;
    };
} conveter_message_t;




enum converterTaskCmd {
	CONVERTER_TICK,				
	CONVERTER_UPDATE,			
	CONVERTER_TURN_ON,			
	CONVERTER_TURN_OFF,			
	
	CONVERTER_SWITCH_TO_5VCH,	
	CONVERTER_SWITCH_TO_12VCH,
	
	CONVERTER_SWITCH_CHANNEL,
			
	CONVERTER_SET_VOLTAGE,		
	CONVERTER_SET_VOLTAGE_LIMIT,
	
	CONVERTER_SET_CURRENT,		
	CONVERTER_SET_CURRENT_RANGE,
	CONVERTER_SET_CURRENT_LIMIT,
	
	CONVERTER_INITIALIZE
};


/*
	There are two channels in the power supply. Every channel has it's own voltage and current settings.
	Every channel has 2 current ranges - low (20A for now) and high (40A). Every current range
	has related limitations and settings, so if current range is changed, current setting is changed too.
*/



typedef struct {
	uint16_t setting;	
	uint16_t MINIMUM;					// const, minimum avaliable setting 
	uint16_t MAXIMUM;					// const, maximum avaliable setting 
	uint16_t limit_low;
	uint16_t limit_high;
	uint16_t LIMIT_MIN;					// const, minimum current setting
	uint16_t LIMIT_MAX;					// const, maximum current setting
	uint8_t enable_low_limit : 1;		
	uint8_t enable_high_limit : 1;
	uint8_t RANGE : 1;					// used as const, only for current
} reg_setting_t;



typedef struct {
	uint8_t CHANNEL : 1;						// used as const
	uint8_t load_state : 1;	
	//uint8_t current_range : 1;
	uint8_t overload_protection_enable : 1;
	uint16_t overload_timeout;
	// Voltage
	reg_setting_t voltage;
	// Current
	reg_setting_t current_low_range;
	reg_setting_t current_high_range;
	reg_setting_t* current;
	
} converter_regulation_t;


extern xQueueHandle xQueueConverter;

extern const conveter_message_t converter_tick_message;

extern converter_regulation_t channel_5v;
extern converter_regulation_t channel_12v;
extern converter_regulation_t *regulation_setting_p;

extern volatile uint8_t cmd_ADC_to_HWProcess;

extern uint16_t voltage_adc;	// [mV]
extern uint16_t current_adc;	// [mA]
extern uint32_t power_adc;		// [mW]

extern uint8_t taskConverter_Enable;


void Converter_Init(uint8_t default_channel);
void vTaskConverter(void *pvParameters);

void Converter_HWProcess(void);
void Converter_HW_ADCProcess(void);





