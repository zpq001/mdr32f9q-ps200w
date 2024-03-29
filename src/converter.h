

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

// ctrl_HWProcess bits
#define CMD_HW_ON				0x01
#define CMD_HW_OFF				0x02
#define CMD_HW_RESET_OVERLOAD	0x04

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







#define CONV_OFF				0x00
#define CONV_ON					0x01

#define CMD_FB_5V			0x0001
#define CMD_FB_12V			0x0002
#define CMD_CLIM_20A		0x0004
#define CMD_CLIM_40A		0x0008
#define CMD_ON				0x0010
#define CMD_OFF				0x0020

#define OVERLOAD_IGNORE_TIMEOUT	100
#define OVERLOAD_TIMEOUT		1


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



extern converter_regulation_t *regulation_setting_p;

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
void Converter_Init(void);
void Converter_Process(void);

void Converter_HWProcess(void);
void Converter_HW_ADCProcess(void);


