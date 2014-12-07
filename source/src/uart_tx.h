#include <stdint.h>
#include "MDR32Fx.h"
#include "FreeRTOS.h"
#include "queue.h"

#include "converter_task_def.h"


//---------------------------------------------//
// Buffer settings
#define TX_BUFFER_SIZE			100


//---------------------------------------------//
// Task message definitions
enum uart_tx_msg_type_t {	
	UART_SEND_DATA,
	UART_SEND_ERROR,
	UART_SEND_CONVERTER_DATA,
	UART_SEND_PROFILING,
	UART_SEND_POWER_CYCLES_STAT
};

enum uart_tx_msg_spec_t {	
	UART_MSG_INFO = 0x0,
	UART_MSG_ACK = 0x1,
};


#pragma anon_unions
typedef struct {
	uint8_t type : 7;					// Type of the message
	uint8_t spec : 1;					// Ackownledge or information
	union {
		struct {
			char *data;						// Pointer to bytes
			uint16_t length;				// Bytes count. May be omitted if data is 0-terminated.
			uint8_t lengthSpecified : 1;	// Set if length is specified. If not set, strlen() will be used.
			uint8_t freeMem : 1;			// Set if memory for data has been allocated. Task will free it after sending data.
		} data;
		struct {
			uint8_t code;				
		} error;
		struct {
			uint8_t param;				// defines parameter to send
			uint8_t channel;			// Related channel for the parameter
			uint8_t current_range;		// Related current range for the parameter
		} converter;
	};
} uart_transmiter_msg_t;


//-------------------------------------------------------//
// Interface

extern xQueueHandle xQueueUART1TX;
extern xQueueHandle xQueueUART2TX;

void vTaskUARTTransmitter(void *pvParameters);



