#include <stdint.h>
#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "converter_task_def.h"

enum { UMSG_INFO, UMSG_ACK }
enum { SEND_MEASURED_VA, SEND_VSET, SEND_ISET, SEND_VSET_LIM, SEND_ISET_LIM, SEND_STATE };


#pragma anon_unions

typedef struct {
	uint8_t type : 6;
	uint8_t spec : 2;
	union {
		struct {
			char *data;
			uint16_t length;
		} char_array;
		struct {
			uint8_t mtype;
			uint8_t spec;
			uint8_t channel;
			uint8_t type;
			uint8_t current_range;
			converter_answ_t answ;
		} converter;
	}
	
} uart_transmiter_msg_t;


typedef struct {
	const uint8_t code;
	const char *text;
} uart_string_table_record_t;



//---------------------------------------------//
// Buffer settings
#define TX_BUFFER_SIZE			100





//---------------------------------------------//
// Task message definitions

enum uart_tx_msg_type_t {	
	UART_RESPONSE_OK = 1, 
	UART_RESPONSE_UNKNOWN_CMD, 
	UART_RESPONSE_WRONG_ARGUMENT, 
	UART_RESPONSE_MISSING_ARGUMENT, 
	UART_RESPONSE_CMD_ERROR,
	UART_SEND_STRING,
	UART_SEND_ALLOCATED_STRING,
	UART_SEND_DATA,
	UART_SEND_ALLOCATED_DATA,
	UART_SEND_PROFILING,
	UART_SEND_POWER_CYCLES_STAT,
	
	UART_TX_START,
	UART_TX_STOP
};




extern xQueueHandle xQueueUART1TX;
extern xQueueHandle xQueueUART2TX;



void vTaskUARTTransmitter(void *pvParameters);



