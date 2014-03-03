
#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"

/*
typedef struct {
	uint8_t type;
	
	union {
		uint32_t data;
		struct {
			uint8_t type;
			uint8_t data;
		} converter_cmd;
		struct {
			uint8_t type;
			uint8_t sender;
			uint8_t data;
		} converter_resp;
		struct {
			uint16_t event_type;
			uint16_t code;
		} key_event;
		struct {
			int16_t delta;
		} encoder_event;
	};
	
	
} dispatch_msg_t;
*/

/*
enum dispatcher_msg_type_t {	
	DISPATCHER_TICK, 
	DP_CONVERTER_TURN_ON, 
	DP_CONVERTER_TURN_OFF,
	DP_CONVERTER_SET_VOLTAGE,
	DP_CONVERTER_SET_CURRENT,
	DP_CONVERTER_SET_CURRENT_LIMIT,
	DP_EMU_BTN_DOWN,
	DP_EMU_ENC_DELTA,
	DISPATCHER_EMULATE_BUTTON,
	DISPATCHER_BUTTONS,
	DISPATCHER_ENCODER,
	DISPATCHER_CONVERTER
};

*/

enum DispatcherTaskMsgTypes {	
	DISPATCHER_TICK = 1, 
	DP_CONVERTER_TURN_ON, 
	DP_CONVERTER_TURN_OFF,
	DP_CONVERTER_SET_VOLTAGE,
	DP_CONVERTER_SET_CURRENT,
	DP_CONVERTER_SET_CURRENT_LIMIT,
	
	DISPATCHER_BUTTONS,
	DISPATCHER_ENCODER,
	DISPATCHER_CONVERTER,
	DISPATCHER_CONVERTER_RESPONSE
};


typedef struct {
	uint8_t type;
	uint8_t sender;
	union {
		struct {
			uint8_t type;
			uint8_t channel;
			uint8_t range;
			uint8_t limit_type;
			uint8_t enable;
			int32_t value;			
		} converter_cmd;
		struct {
			uint8_t msg_type;
			uint8_t msg_sender;
			uint8_t err_code;
			uint8_t spec;
			uint8_t channel : 2;
			uint8_t range : 2;
			uint8_t limit_type : 2;
		} converter_resp;
		struct {
			uint16_t event_type;
			uint16_t code;
		} key_event;
		struct {
			int16_t delta;
		} encoder_event;
	};
} dispatch_msg_t;

extern xQueueHandle xQueueDispatcher;
extern const dispatch_msg_t dispatcher_tick_msg;

void vTaskDispatcher(void *pvParameters);




