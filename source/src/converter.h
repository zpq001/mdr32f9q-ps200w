
#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


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

#define CONV_MIN_OVERLOAD_TIMEOUT		0
#define CONV_MAX_OVERLOAD_TIMEOUT		(5*10)	// in units of Converter_HWProcess call period


//-------------------------------------------------------//
// Other

// Extended converter channel definition - used when
// currently operating channel should be updated
#define OPERATING_CHANNEL 2

// Extended converter current range definition - used when
// currently operating range should be updated
#define OPERATING_CURRENT_RANGE 2

// Software limit types
#define LIMIT_TYPE_LOW			0x00
#define LIMIT_TYPE_HIGH			0x01


//TODO - move it to some common for all tasks header
#define TID_UART		0x01
#define TID_GUI			0x02


//---------------------------------------------//
// Task queue messages




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
	
	CONVERTER_OVERLOADED,
	
	CONVERTER_INITIALIZE
};





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
} converter_message_t;





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
	uint8_t overload_protection_enable : 1;
	uint16_t overload_timeout;
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




