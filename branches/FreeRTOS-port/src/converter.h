

#define CONV_MAX_VOLTAGE_5V_CHANNEL		10000	// [mV]
#define CONV_MAX_VOLTAGE_12V_CHANNEL	20000	// [mV]
#define CONV_MIN_VOLTAGE_5V_CHANNEL		0		// [mV]
#define CONV_MIN_VOLTAGE_12V_CHANNEL	0		// [mV]

// Common for both channels
#define CONV_HIGH_LIM_MAX_CURRENT		40000	// [mA]
#define CONV_LOW_LIM_MAX_CURRENT		20000	// [mA]
#define CONV_MIN_CURRENT				0		// [mA]


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
#define SET_LOW_VOLTAGE_SOFT_LIMIT	0x01
#define SET_HIGH_VOLTAGE_SOFT_LIMIT	0x02
#define SET_LOW_CURRENT_SOFT_LIMIT	0x03
#define SET_HIGH_CURRENT_SOFT_LIMIT	0x04


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

#define OVERLOAD_IGNORE_TIMEOUT	(5*100)		// 100 ms
#define OVERLOAD_TIMEOUT		5			// 1 ms
#define MINIMAL_OFF_TIME		(5*40)		// 40 ms
#define USER_TIMEOUT			(5*20)		// 20 ms	- used for delay between channel switch and ON command

// These defines set behavior of controller in case when 
// error status is generated simultaneously with processing OFF command
// Either must be set
#define CMD_HAS_PRIORITY 	0		// - no error will be shown
#define ERROR_HAS_PRIORITY	1		// - error will be shown

#if ((CMD_HAS_PRIORITY && ERROR_HAS_PRIORITY) || (!CMD_HAS_PRIORITY && !ERROR_HAS_PRIORITY))
#error "Either CMD_HAS_PRIORITY or ERROR_HAS_PRIORITY options must be set"
#endif 

//---------------------------------------------//
// Task queue messages

typedef struct {
	uint32_t type;
	uint32_t data_a;
	uint32_t data_b;
} conveter_message_t;

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


/*
typedef struct {
	uint8_t feedback_channel;
	uint8_t load_state;
} converter_state_t;
*/

typedef struct {
	
	uint8_t CHANNEL;			// const
	uint8_t load_state;
	
	uint16_t set_voltage;
	uint16_t MAX_VOLTAGE;		// const
	uint16_t MIN_VOLTAGE;		// const
	uint16_t soft_max_voltage;
	uint16_t soft_min_voltage;
	uint16_t SOFT_MAX_VOLTAGE_LIMIT;
	uint16_t SOFT_MIN_VOLTAGE_LIMIT;
	uint8_t soft_voltage_range_enable;
	
	uint8_t current_limit;
	uint16_t set_current;	
	uint16_t LOW_LIM_MAX_CURRENT;	// const
	uint16_t LOW_LIM_MIN_CURRENT;	// const
	uint16_t HIGH_LIM_MAX_CURRENT;	// const
	uint16_t HIGH_LIM_MIN_CURRENT;	// const
	uint16_t soft_max_current;
	uint16_t soft_min_current;
	uint16_t SOFT_MAX_CURRENT_LIMIT;
	uint16_t SOFT_MIN_CURRENT_LIMIT;
	uint8_t soft_current_range_enable;
	
} converter_regulation_t;


extern xQueueHandle xQueueConverter;

extern const conveter_message_t converter_tick_message;

extern converter_regulation_t *regulation_setting_p;

extern uint8_t cmd_ADC_to_HWProcess;

extern uint16_t voltage_adc;	// [mV]
extern uint16_t current_adc;	// [mA]
extern uint32_t power_adc;		// [mW]
	
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


