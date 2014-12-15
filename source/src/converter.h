
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
// Control message stuff

enum ConverterTaskMsgTypes {
	CONVERTER_TICK,
	CONVERTER_CONTROL,
	CONVERTER_PROFILE_CMD,
	CONVERTER_ADC_MEASUREMENT_READY,
	CONVERTER_OVERLOAD_EVENT
};

enum ConverterParameters {
	param_STATE,				
	param_CHANNEL,
	param_CRANGE,
	param_VSET,
	param_CSET,
	param_VLIMIT,
	param_CLIMIT,
	param_MEASURED_DATA,		
	param_OVERLOAD_PROTECTION,
	param_DAC_OFFSET
};

enum ConverterStateControl {
	cmd_TURN_ON,
	cmd_TURN_OFF,
	cmd_START_CHARGE,
	cmd_STOP_CHARGE,
	// Internal. Do not use outside of converter module.
	cmd_GET_READY_FOR_CHANNEL_SWITCH,
	cmd_GET_READY_FOR_PROFILE_LOAD,
	event_OVERLOAD,
	event_TICK
};

enum ConverterProfileControl {
	cmd_LOAD_PROFILE,
	cmd_SAVE_PROFILE
	//CONVERTER_LOAD_GLOBAL_SETTINGS,
	//CONVERTER_SAVE_GLOBAL_SETTINGS
};


typedef struct {
	uint8_t type;			// see ConverterTaskMsgTypes
	uint8_t param;
	uint8_t sender;
	xSemaphoreHandle *pxSemaphore;
	converter_arguments_t a;
} converter_message_t;


//-------------------------------------------------------//
// Voltage and current setting return codes

#define VALUE_BOUND_BY_SOFT_MAX		0x01
#define VALUE_BOUND_BY_SOFT_MIN		0x02
#define VALUE_BOUND_BY_ABS_MAX		0x04
#define VALUE_BOUND_BY_ABS_MIN		0x08
#define VALUE_UPDATED				0x10
#define VALUE_UPFORCED				0x20	// value change has been caused by another value change
#define VALUE_BOUND_MASK	(VALUE_BOUND_BY_SOFT_MAX | VALUE_BOUND_BY_SOFT_MIN | VALUE_BOUND_BY_ABS_MAX | VALUE_BOUND_BY_ABS_MIN)

enum ConverterErrorCodes { ERROR_NONE, ERROR_INTERNAL, ERROR_CMD };

enum ConverterStates {
	CONVERTER_STATE_OFF,
	CONVERTER_STATE_ON,
	CONVERTER_STATE_OVERLOADED,
	CONVERTER_STATE_CHARGING
};

enum ConverterStateEvents {
	CONV_TURNED_ON,
	CONV_TURNED_OFF,
	CONV_OVERLOADED,
	CONV_STARTED_CHARGE,
	CONV_FINISHED_CHARGE,
	CONV_ABORTED_CHARGE,
	CONV_NO_STATE_CHANGE
};

// Charge settings

// Charge status
enum ConverterChargeStatusValues {
	CHARGE_INACTIVE,
	CHARGE_STARTED,
	CHARGE_STOPPED,
	CHARGE_ABORTED,
	
	CHARGE_COMPLETED_BY_DV = 0x10,
	CHARGE_COMPLETED_BY_TIMER,
	CHARGE_COMPLETED_BY_VMAX,
	CHARGE_COMPLETED_BY_CMIN,
};

#define CHARGE_SUCCESS_MASK	(CHARGE_COMPLETED_BY_DV | CHARGE_COMPLETED_BY_TIMER | CHARGE_COMPLETED_BY_VMAX | CHARGE_COMPLETED_BY_CMIN)

//-------------------------------------------------------//
// Converter data structures

typedef struct {
	uint32_t setting;	
	uint32_t MINIMUM;					// const, minimum avaliable setting 
	uint32_t MAXIMUM;					// const, maximum avaliable setting 
	uint32_t limit_low;
	uint32_t limit_high;
	uint32_t LIMIT_MIN;					// const, minimum avaliable limit setting
	uint32_t LIMIT_MAX;					// const, maximum avaliable limit setting
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

// Charging stuff
typedef struct {
	uint32_t vset;
	uint32_t cset;
	int16_t dv;
	uint32_t vmax;
	uint32_t cmin;
	uint32_t time_limit;
	uint8_t use_dv : 1;
	uint8_t use_vmax : 1;
	uint8_t use_cmin : 1;
	uint8_t use_time_limit : 1;
} charge_settings_t;


typedef struct {
	int16_t dv_mea;
	uint32_t timer;
	uint8_t status;
} charge_status_t;


typedef struct {
	channel_state_t channel_5v;
	channel_state_t channel_12v;
	channel_state_t *channel;
	charge_settings_t charge_settings;
	charge_status_t charge_status;
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




