
#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "converter_task_def.h"



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
	DISPATCHER_CONVERTER_EVENT
};


typedef struct {
	uint8_t type;
	uint8_t sender;
	union {
		struct {
			uint8_t msg_type;
			converter_arguments_t a;
		} converter_cmd;
		struct {
			uint8_t msg_type;
			uint8_t msg_sender;
			uint8_t err_code;
			uint8_t spec;
			uint8_t channel : 2;
			uint8_t range : 2;
			uint8_t limit_type : 2;
		} converter_event;
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




