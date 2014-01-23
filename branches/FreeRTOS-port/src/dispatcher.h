
typedef struct {
	uint32_t type;
	uint32_t data;
} dispatch_msg_t;



enum dispatcher_msg_type_t {	
	DISPATCHER_TICK, 
	DP_CONVERTER_TURN_ON, 
	DP_CONVERTER_TURN_OFF,
	DP_CONVERTER_SET_VOLTAGE,
	DP_CONVERTER_SET_CURRENT,
	DP_CONVERTER_SET_CURRENT_LIMIT,
	DP_EMU_BTN_DOWN,
	DP_EMU_ENC_DELTA,
	DISPATCHER_EMULATE_BUTTON
};



extern xQueueHandle xQueueDispatcher;
extern const dispatch_incoming_msg_t dispatcher_tick_msg;

void vTaskDispatcher(void *pvParameters);



