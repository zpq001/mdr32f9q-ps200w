

#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"

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





//TODO - move it to some common for all tasks header
#define TID_UART		0x01
#define TID_GUI			0x02





#define CONV_OFF				0x01
#define CONV_ON					0x02
#define CONV_OVERLOAD			0x10
#define CONV_STATE_MASK			0x0F






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









extern xQueueHandle xQueueConverter;

extern const conveter_message_t converter_tick_message;





extern uint16_t voltage_adc;	// [mV]
extern uint16_t current_adc;	// [mA]
extern uint32_t power_adc;		// [mW]

extern uint8_t taskConverter_Enable;


void Converter_Init(uint8_t default_channel);
void vTaskConverter(void *pvParameters);





