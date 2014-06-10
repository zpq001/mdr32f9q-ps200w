#include <stdint.h>
#include "MDR32Fx.h"

#include "FreeRTOS.h"
#include "queue.h"



typedef struct {
	uint8_t type;
	char *data;
	uint16_t length;
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



