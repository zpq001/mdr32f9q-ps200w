

#define CONV_MAX_VOLTAGE_5V_CHANNEL		10000	// [mV]
#define CONV_MAX_VOLTAGE_12V_CHANNEL	20000	// [mV]
#define CONV_MIN_VOLTAGE_5V_CHANNEL		0		// [mV]
#define CONV_MIN_VOLTAGE_12V_CHANNEL	0		// [mV]

// Common for both channels
#define CONV_LOW_CURRENT_RANGE_MAX		20000	// [mA]
#define CONV_LOW_CURRENT_RANGE_MIN		0		// [mA]
#define CONV_HIGH_CURRENT_RANGE_MAX		40000	// [mA]
#define CONV_HIGH_CURRENT_RANGE_MIN		0		// [mA]


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

#define OVERLOAD_IGNORE_TIMEOUT		(5*100)		// 100 ms
#define OVERLOAD_TIMEOUT			5			// 1 ms
#define MINIMAL_OFF_TIME			(5*40)		// 40 ms
#define USER_TIMEOUT				(5*20)		// 20 ms	- used for delay between channel switch and ON command
#define LED_BLINK_TIMEOUT			(5*100)		// 100ms	- used for charging indication
#define OVERLOAD_WARNING_TIMEOUT	(5*200)		// 200ms	- used for sound overload warning

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


typedef struct {
    uint32_t type;
    union {
        struct {
            uint32_t a;
            uint32_t b;
        } data;
        struct {
            uint16_t mode;
            uint16_t enable;
            uint32_t value;
        } voltage_limit_setting;
    };
    //	uint32_t data_a;
    //	uint32_t data_b;
} conveter_message_t;




enum converterTaskCmd {
	CONVERTER_TICK,				
	CONVERTER_UPDATE,			
	CONVERTER_TURN_ON,			
	CONVERTER_TURN_OFF,			
	
	CONVERTER_SWITCH_TO_5VCH,	
	CONVERTER_SWITCH_TO_12VCH,
	CONVERTER_SET_CURRENT_RANGE,		
	CONVERTER_SET_VOLTAGE,		
	CONVERTER_SET_VOLTAGE_LIMIT,
	
	CONVERTER_SET_CURRENT,		
	CONVERTER_INITIALIZE
};

/*
#define CONVERTER_TICK				0xFF
#define CONVERTER_UPDATE			0xFE
#define CONVERTER_TURN_ON			0x01
#define CONVERTER_TURN_OFF			0x02
#define CONVERTER_SWITCH_TO_5VCH	0x03
#define CONVERTER_SWITCH_TO_12VCH	0x04
#define SET_CURRENT_LIMIT_20A		0x05
#define SET_CURRENT_LIMIT_40A		0x06
#define CONVERTER_SET_VOLTAGE		0x07
#define CONVERTER_SET_CURRENT		0x08
#define CONVERTER_INITIALIZE		0x09
#define CONVETER_SET_VOLTAGE_LIMIT	0x0A
*/


/*
	There are two channels in the power supply. Every channel has it's own voltage and current settings.
	Every channel has 2 current ranges - low (20A for now) and high (40A). Every current range
	has related limitations and settings, so if current range is changed, current setting is changed too.
*/

typedef struct {
	uint16_t setting;	
	uint16_t MINIMUM;					// const, minimum avaliable current setting 
	uint16_t MAXIMUM;					// const, maximum avaliable current setting 
	uint16_t limit_low;
	uint16_t limit_high;
	uint16_t LIMIT_MIN;					// const, minimum current limit setting
	uint16_t LIMIT_MAX;					// const, maximum current limit setting
	uint8_t enable_low_limit : 1;		
	uint8_t enable_high_limit : 1;
	uint8_t RANGE : 1;					// const, used for current range distinction
} current_setting_t;

typedef struct {
	uint16_t setting;	
	uint16_t MINIMUM;					// const, minimum avaliable current setting 
	uint16_t MAXIMUM;					// const, maximum avaliable current setting 
	uint16_t limit_low;
	uint16_t limit_high;
	uint16_t LIMIT_MIN;					// const, minimum current limit setting
	uint16_t LIMIT_MAX;					// const, maximum current limit setting
	uint8_t enable_low_limit : 1;		
	uint8_t enable_high_limit : 1;
} voltage_setting_t;

typedef struct {
	uint8_t CHANNEL : 1;						// const
	uint8_t load_state : 1;	
	uint8_t overload_protection_enable : 1;
	uint8_t overload_timeout;
	// Voltage
	voltage_setting_t voltage;
	// Current
	current_setting_t current_low_range;
	current_setting_t current_high_range;
	current_setting_t *current;
} converter_regulation_t;


extern xQueueHandle xQueueConverter;

extern const conveter_message_t converter_tick_message;

extern converter_regulation_t *regulation_setting_p;

extern volatile uint8_t cmd_ADC_to_HWProcess;

extern uint16_t voltage_adc;	// [mV]
extern uint16_t current_adc;	// [mA]
extern uint32_t power_adc;		// [mW]

extern uint8_t taskConverter_Enable;

void Converter_ProcessADC(void);
uint8_t Converter_SetVoltage(int32_t new_voltage);
uint8_t Converter_SetCurrent(int32_t new_current);
uint8_t Converter_SetSoftLimit(int32_t new_limit, converter_regulation_t *reg_p, uint8_t mode);
void Converter_SetFeedbackChannel(uint8_t new_channel);
void Converter_SetCurrentLimit(uint8_t new_limit);
void Converter_Enable(void);
void Converter_Disable(void);
void Converter_Init(uint8_t default_channel);
void Converter_Process(void);


void vTaskConverter(void *pvParameters);

void Converter_HWProcess(void);
void Converter_HW_ADCProcess(void);





