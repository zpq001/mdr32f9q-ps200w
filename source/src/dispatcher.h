
#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "converter_task_def.h"



enum DispatcherTaskMsgTypes {	
	DISPATCHER_TICK = 1, 

	DISPATCHER_BUTTONS,
	DISPATCHER_ENCODER,
	DISPATCHER_CONVERTER,
	DISPATCHER_CONVERTER_EVENT,
	
	DISPATCHER_LOAD_PROFILE,
	DISPATCHER_SAVE_PROFILE,
	DISPATCHER_LOAD_PROFILE_RESPONSE,
	DISPATCHER_SAVE_PROFILE_RESPONSE,
	
	DISPATCHER_PROFILE_SETUP,
	
	DISPATCHER_TEST_FUNC1,
	DISPATCHER_SHUTDOWN,
	
	DISPATCHER_NEW_ADC_DATA
};


typedef struct {
	uint8_t type;
	uint8_t sender;
	union {
		struct {
			uint8_t msg_type;
			converter_arguments_t a;
		} converter_cmd;
/*(		struct {
			uint8_t msg_type;
			uint8_t msg_sender;
			uint8_t err_code;
			uint8_t spec;
			uint8_t channel : 2;
			uint8_t range : 2;
			uint8_t limit_type : 2;
		} converter_event; */
		
		struct {
			uint8_t msg_sender;
			uint8_t param;				// parameter that changed
			uint8_t spec;				// parameter modify specification
			uint8_t err_code;			// error (if any)
			
			uint8_t channel : 1;		// Related channel for the parameter
			uint8_t range : 1;			// Related current range for the parameter
			uint8_t limit_type : 1;
		} converter_event;
		
		struct {
			uint16_t event_type;
			uint16_t code;
		} key_event;
		struct {
			int16_t delta;
		} encoder_event;
		struct {
			uint8_t number;
		} profile_load;
		struct {
			uint8_t index;
			uint8_t profileState;
		} profile_load_response;
		struct {
			uint8_t number;
			char *new_name;
		} profile_save;
		struct {
			uint8_t index;
			uint8_t profileState;
		} profile_save_response;
		struct {
			uint8_t number;
			uint8_t sender;
			char **buffer;
		} profile_name_request;
		struct {
			uint8_t number;
			uint8_t sender;
			uint8_t profileState;
			char **buffer;
		} profile_name_response;
		struct {
			uint8_t saveRecentProfile;
			uint8_t restoreRecentProfile;
		} profile_setup;
	};
} dispatch_msg_t;



extern xQueueHandle xQueueDispatcher;
//extern const dispatch_msg_t dispatcher_tick_msg;
extern const dispatch_msg_t dispatcher_shutdown_msg;

void vTaskDispatcher(void *pvParameters);




