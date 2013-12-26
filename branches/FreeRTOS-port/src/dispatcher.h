
typedef struct {
	uint32_t type;
	uint32_t data;
} dispatch_incoming_msg_t;



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
/*
#define DISPATCHER_TICK	0

// Converter control
#define DP_CONVERTER_TURN_ON		0x200
#define DP_CONVERTER_TURN_OFF		0x201
#define DP_CONVERTER_SET_VOLTAGE	0x210
#define DP_CONVERTER_SET_CURRENT	0x211
#define DP_CONVERTER_SET_CURRENT_LIMIT	0x212

// Control panel emulation
#define DP_EMU_BTN_DOWN			0x400
#define DP_EMU_ENC_DELTA		0x410
*/


extern xQueueHandle xQueueDispatcher;
extern const dispatch_incoming_msg_t dispatcher_tick_msg;

void vTaskDispatcher(void *pvParameters);



