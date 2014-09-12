
#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "converter_task_def.h"

//-------------------------------------------------------//
// Global converter constraints

#define CONV_MAX_VOLTAGE_5V_CHANNEL		10000	// [mV]
#define CONV_MAX_VOLTAGE_12V_CHANNEL	20000	// [mV]
#define CONV_MIN_VOLTAGE_5V_CHANNEL		0		// [mV]
#define CONV_MIN_VOLTAGE_12V_CHANNEL	0		// [mV]

// Common for both channels
#define CONV_LOW_CURRENT_RANGE_MAX		20000	// [mA]
#define CONV_LOW_CURRENT_RANGE_MIN		0		// [mA]
#define CONV_HIGH_CURRENT_RANGE_MAX		40000	// [mA]
#define CONV_HIGH_CURRENT_RANGE_MIN		0		// [mA]

#define CONV_MIN_OVERLOAD_THRESHOLD		2		// in units of 100us (Converter_HWProcess call period is 0.2 ms)
#define CONV_MAX_OVERLOAD_THRESHOLD		(10*50)	





//---------------------------------------------//
// Task queue messages

#define CONVERTER_GROUP_MASK		0xF0
#define CONVERTER_CONTROL_GROUP		0x10
#define CONVERTER_SET_PARAMS_GROUP	0x20
#define CONVERTER_PROFILE_GROUP		0x30

enum ConverterTaskMsgTypes {
	// Control
	CONVERTER_TURN_ON = CONVERTER_CONTROL_GROUP,
	CONVERTER_TURN_OFF,
	CONVERTER_START_CHARGE,
	CONVERTER_OVERLOADED,
	CONVERTER_TICK,		
	
	// parameters setting
	CONVERTER_SET_VOLTAGE = CONVERTER_SET_PARAMS_GROUP,		
	CONVERTER_SET_VOLTAGE_LIMIT,
	CONVERTER_SET_CURRENT,		
	CONVERTER_SET_CURRENT_LIMIT,
	CONVERTER_SET_CURRENT_RANGE,
	CONVERTER_SWITCH_CHANNEL,
	CONVERTER_SET_OVERLOAD_PARAMS,
	CONVERTER_SET_DAC_PARAMS,
	
	// profile
	CONVERTER_LOAD_PROFILE = CONVERTER_PROFILE_GROUP,
	CONVERTER_SAVE_PROFILE
	//CONVERTER_LOAD_GLOBAL_SETTINGS,
	//CONVERTER_SAVE_GLOBAL_SETTINGS
	
};


typedef struct {
    uint8_t type;
	uint8_t sender;
	xSemaphoreHandle *pxSemaphore;
	converter_arguments_t a;
} converter_message_t;


//-------------------------------------------------------//
// Voltage and current setting return codes

#define VALUE_OK					0x00
#define VALUE_BOUND_BY_SOFT_MAX		0x01
#define VALUE_BOUND_BY_SOFT_MIN		0x02
#define VALUE_BOUND_BY_ABS_MAX		0x03
#define VALUE_BOUND_BY_ABS_MIN		0x04
#define CMD_OK						0x10
#define CMD_ERROR					0x11
#define EVENT_OK					0x12
#define EVENT_ERROR					0x13



//-------------------------------------------------------//
// spec values to dispatcher

enum ConverterEventSpec {
	VOLTAGE_SETTING_CHANGE = 1,
	CURRENT_SETTING_CHANGE,	
	VOLTAGE_LIMIT_CHANGE,
	CURRENT_LIMIT_CHANGE,	
	OVERLOAD_SETTING_CHANGE,
	CURRENT_RANGE_CHANGE,
	CHANNEL_CHANGE,
	STATE_CHANGE_TO_ON,
	STATE_CHANGE_TO_OFF,
	STATE_CHANGE_TO_OVERLOAD,
	PROFILE_LOADED,
	
	CONV_TURNED_ON,
	CONV_TURNED_OFF,
	CONV_OVERLOADED,
	CONV_STARTED_CHARGE,
	CONV_FINISHED_CHARGE,
	CONV_ABORTED_CHARGE,
	CONV_NO_STATE_CHANGE
};



//-------------------------------------------------------//
// Converter data structures

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
	// Voltage
	reg_setting_t voltage;
	// Current
	reg_setting_t current_low_range;
	reg_setting_t current_high_range;
	reg_setting_t* current;
} channel_state_t;


typedef struct {
	channel_state_t channel_5v;
	channel_state_t channel_12v;
	channel_state_t *channel;
	uint8_t state;
	uint8_t overload_protection_enable : 1;
	uint8_t overload_warning_enable : 1;
	uint16_t overload_threshold;
} converter_state_t;

/*
typedef struct {
	int16_t voltage_offset;
	int16_t current_low_offset;
	int16_t current_high_offset;
} dac_settings_t;
*/

//-------------------------------------------------------//
// Converter states

#define CONVERTER_STATE_OFF			0x00
#define CONVERTER_STATE_ON			0x01
#define CONVERTER_STATE_OVERLOADED	0x02




uint8_t Converter_GetState(void);
uint16_t Converter_GetVoltageSetting(uint8_t channel);
uint16_t Converter_GetVoltageAbsMax(uint8_t channel);
uint16_t Converter_GetVoltageAbsMin(uint8_t channel);
uint16_t Converter_GetVoltageLimitSetting(uint8_t channel, uint8_t limit_type);
uint8_t Converter_GetVoltageLimitState(uint8_t channel, uint8_t limit_type);
uint16_t Converter_GetCurrentSetting(uint8_t channel, uint8_t range);
uint16_t Converter_GetCurrentAbsMax(uint8_t channel, uint8_t range);
uint16_t Converter_GetCurrentAbsMin(uint8_t channel, uint8_t range);
uint16_t Converter_GetCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type);
uint8_t Converter_GetCurrentLimitState(uint8_t channel, uint8_t range, uint8_t limit_type);
uint8_t Converter_GetOverloadProtectionState(void);
uint8_t Converter_GetOverloadProtectionWarning(void);
uint16_t Converter_GetOverloadProtectionThreshold(void);
uint8_t Converter_GetCurrentRange(uint8_t channel);
uint8_t Converter_GetFeedbackChannel(void);
int8_t Converter_GetVoltageDacOffset(void);
int8_t Converter_GetCurrentDacOffset(uint8_t range);

//void Converter_SaveGlobalSettings(void);
void Converter_SaveProfile(void);

// TODO: check closed static data - converter_state, etc



extern converter_state_t converter_state;		// main converter control
//extern dac_settings_t dac_settings;
extern xQueueHandle xQueueConverter;
extern xTaskHandle xTaskHandle_Converter;
extern const converter_message_t converter_tick_message;

void Converter_Init(void);
void vTaskConverter(void *pvParameters);




