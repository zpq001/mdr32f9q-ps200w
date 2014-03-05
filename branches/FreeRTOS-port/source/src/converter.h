
#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

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

#define CONV_MIN_OVERLOAD_THRESHOLD		1		// in units of Converter_HWProcess call period (0.2 ms)
#define CONV_MAX_OVERLOAD_THRESHOLD		(5*50)	








//-------------------------------------------------------//
// Voltage and current setting return codes
#define VALUE_OK					0x00
#define VALUE_BOUND_BY_SOFT_MAX		0x01
#define VALUE_BOUND_BY_SOFT_MIN		0x02
#define VALUE_BOUND_BY_ABS_MAX		0x04
#define VALUE_BOUND_BY_ABS_MIN		0x08
#define VALUE_ERROR					0xFF


//---------------------------------------------//
// Task queue messages


enum ConverterTaskMsgTypes {
	CONVERTER_TICK,							
	CONVERTER_TURN_ON,			
	CONVERTER_TURN_OFF,			
	CONVERTER_SWITCH_CHANNEL,
	CONVERTER_SET_VOLTAGE,		
	CONVERTER_SET_VOLTAGE_LIMIT,
	CONVERTER_SET_CURRENT,		
	CONVERTER_SET_CURRENT_LIMIT,
	CONVERTER_SET_CURRENT_RANGE,
	CONVERTER_SET_OVERLOAD_PARAMS,
	CONVERTER_OVERLOADED
};

/*
typedef struct {
    uint8_t type;
	uint8_t sender;
	uint8_t channel;
	uint8_t range;
	uint8_t limit_type;
	uint8_t enable;
	int32_t value;
} converter_message_t;
*/

typedef struct {
    uint8_t type;
	uint8_t sender;
	converter_arguments_t a;
} converter_message_t;

#define	VOLTAGE_SETTING_CHANGED			1
#define CURRENT_SETTING_CHANGED			2
#define VOLTAGE_LIMIT_CHANGED			3
#define CURRENT_LIMIT_CHANGED			4
#define CURRENT_RANGE_CHANGED			5
#define CHANNEL_CHANGED					6
#define OVERLOAD_SETTING_CHANGED		7
#define STATE_CHANGED					8


/*
enum converterTaskCmd {
	CONVERTER_TICK,				
	CONVERTER_UPDATE,			
	CONVERTER_TURN_ON,			
	CONVERTER_TURN_OFF,			
	
	CONVERTER_SWITCH_CHANNEL,
			
	CONVERTER_SET_VOLTAGE,		
	CONVERTER_SET_VOLTAGE_LIMIT,
	
	CONVERTER_SET_CURRENT,		
	CONVERTER_SET_CURRENT_RANGE,
	CONVERTER_SET_CURRENT_LIMIT,
	
	CONVERTER_SET_OVERLOAD_PARAMS,
	
	CONVERTER_OVERLOADED,
	
	CONVERTER_INITIALIZE
};
*/



/*
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
			uint8_t protection_enable;
            uint8_t warning_enable;
            int32_t threshold;
        } overload_setting;
		struct {
			uint8_t new_channel;
		} channel_setting;
    };
} converter_message_t;
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



#define CONVERTER_STATE_OFF			0x00
#define CONVERTER_STATE_ON			0x01
#define CONVERTER_STATE_OVERLOADED	0x02



extern converter_state_t converter_state;		// main converter control
extern xQueueHandle xQueueConverter;
extern xTaskHandle xTaskHandle_Converter;
extern const converter_message_t converter_tick_message;

void Converter_Init(uint8_t default_channel);
void vTaskConverter(void *pvParameters);




