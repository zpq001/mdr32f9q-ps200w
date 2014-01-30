
//-------------------------------------------------------//
// ctrl_ADCProcess bits
// For ADC task
#define CMD_ADC_START_VOLTAGE		0x01
#define CMD_ADC_START_CURRENT		0x02
#define CMD_ADC_START_DISCON		0x04


//-------------------------------------------------------//
// Voltage and current setting return codes
#define VALUE_OK					0x00
#define VALUE_BOUND_BY_SOFT_MAX		0x01
#define VALUE_BOUND_BY_SOFT_MIN		0x02
#define VALUE_BOUND_BY_ABS_MAX		0x04
#define VALUE_BOUND_BY_ABS_MIN		0x08


//-------------------------------------------------------//
// Converter control functions return values
#define ERROR_ARGS					0xFF
// these two can be set simultaneously
#define VOLTAGE_SETTING_APPLIED		0x01	
#define VOLTAGE_LIMIT_APPLIED		0x02
// these two can be set simultaneously
#define CURRENT_SETTING_APPLIED		0x01	
#define CURRENT_LIMIT_APPLIED		0x02

#define OVERLOAD_SETTINGS_APPLIED	0x03
#define CURRENT_RANGE_APPLIED		0x04
#define CHANNEL_SETTING_APPLIED		0x05

#define CONVERTER_CMD_OK			0x10
#define CONVERTER_IS_NOT_READY		0x11
#define CONVERTER_IS_OVERLOADED		0x12



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
// Other

// Extended converter channel definition - used when
// currently operating channel should be updated
#define OPERATING_CHANNEL 2

// Extended converter current range definition - used when
// currently operating range should be updated
#define OPERATING_CURRENT_RANGE 2

// Current limit types
#define LIMIT_TYPE_LOW			0x00
#define LIMIT_TYPE_HIGH			0x01


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


// CHECKME: remove this later
extern converter_regulation_t channel_5v;
extern converter_regulation_t channel_12v;
extern converter_regulation_t *regulation_setting_p;

// For using from ADC task
extern volatile uint8_t cmd_ADC_to_HWProcess;

// For calling from timer ISR
void Converter_HWProcess(void);

uint8_t GetActualChannel(uint8_t channel);
uint8_t GetActualCurrentRange(uint8_t range);

uint8_t Converter_IsReady(void);

uint8_t Converter_SetVoltage(uint8_t channel, int32_t new_value, uint8_t *err_code);
uint8_t Converter_SetVoltageLimit(uint8_t channel, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code);
uint8_t Converter_SetCurrent(uint8_t channel, uint8_t current_range, int32_t new_value, uint8_t *err_code);
uint8_t Converter_SetCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code);
uint8_t Converter_SetProtectionControl(uint8_t channel, uint8_t protection_enable, int16_t overload_timeout, uint8_t *err_code);
uint8_t Converter_SetCurrentRange(uint8_t channel, uint8_t new_value);
uint8_t Converter_SetChannel(uint8_t new_value, uint8_t apply_anyway);
uint8_t Converter_TurnOn(void);
uint8_t Converter_TurnOff(void);
uint8_t Converter_ClearOverloadFlag(void);

uint16_t Converter_GetVoltageSetting(uint8_t channel);
uint16_t Converter_GetVoltageLimitSetting(uint8_t channel, uint8_t limit_type);
uint16_t Converter_GetCurrentSetting(uint8_t channel, uint8_t current_range);
uint16_t Converter_GetCurrentLimitSetting(uint8_t channel, uint8_t current_range, uint8_t limit_type);
uint8_t Converter_IsVoltageLimitEnabled(uint8_t channel, uint8_t limit_type);
uint8_t Converter_IsCurrentLimitEnabled(uint8_t channel, uint8_t current_range, uint8_t limit_type);
uint8_t Converter_GetCurrentRange(uint8_t channel);
uint8_t Converter_GetChannel(void);
uint16_t Converter_GetOverloadTimeout(uint8_t channel);
uint8_t Converter_IsOverloadProtectionEnabled(uint8_t channel);
//uint8_t Converter_GetState(void);










