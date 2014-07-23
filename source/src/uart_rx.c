
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdint.h"



#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "global_def.h"
#include "key_def.h"		// button codes
#include "uart_hw.h"
#include "uart_parser.h"
#include "uart_rx.h"
#include "uart_tx.h"
#include "systemfunc.h"
#include "dispatcher.h"
#include "converter.h"
#include "guiTop.h"
#include "eeprom.h"


static void execute_command(uint8_t cmd_code, uint8_t uart_num, arg_t *args);



void UART_Get_comm_params(uint8_t uart_num, uart_param_request_t *r)
{
	uartx_settings_t *my_uart_settings = (uart_num == 1) ? &uart_settings.uart1 : &uart_settings.uart2;
	r->enable = my_uart_settings->enable;
	r->brate = my_uart_settings->baudRate;
	r->parity = my_uart_settings->parity;
}



// OS stuff
static xSemaphoreHandle xSyncSemaphore1;
static xSemaphoreHandle xSyncSemaphore2;
xSemaphoreHandle hwUART1_mutex;
xSemaphoreHandle hwUART2_mutex;
xQueueHandle xQueueUART1RX;
xQueueHandle xQueueUART2RX;

uart_receiver_msg_t uart_rx_tick_msg = {UART_RX_TICK};


typedef struct {
	uint16_t chars_to_read;
	uint16_t msg_length;
	uint8_t analyze_message;
	char received_msg[RX_MESSAGE_MAX_LENGTH];
	char *received_msg_ptr;
	char *argv[MAX_WORDS_IN_MESSAGE];		// Array of pointers to separate words
	uint16_t argc;							// Count of words in message
	uint8_t searchWord;
} UART_RX_Parser_Stuff_t;


typedef struct {
	const uint8_t uart_num;
	xQueueHandle *xQueue;
	xSemaphoreHandle *hwUART_mutex;
	xSemaphoreHandle *xSyncSem;
	uint8_t exeFunc;
    arg_t exeFuncArgs[MAX_FUNCTION_ARGUMENTS];
} UART_RX_Task_Context_t;


UART_RX_Task_Context_t UART1_Task_Context = {
	1,				// uart number
	&xQueueUART1RX,
	&hwUART1_mutex,
	&xSyncSemaphore1
};

UART_RX_Task_Context_t UART2_Task_Context = {
	2,				// uart number
	&xQueueUART2RX,
	&hwUART2_mutex,
	&xSyncSemaphore2
};



static uint8_t UartRxStreamParser(char c)
{
	if ((c == '\n') || (c == '\r'))
		return 1;
	else
		return 0;
}


void vTaskUARTReceiver(void *pvParameters)
{
	// Uart number for task is passed by argument
	uart_receiver_msg_t msg;
	UART_RX_Task_Context_t *ctx = ((uint32_t)pvParameters == 1) ? &UART1_Task_Context : &UART2_Task_Context;
	UART_RX_Parser_Stuff_t parser;
	uartx_settings_t *my_uart_settings = (ctx->uart_num == 1) ? &uart_settings.uart1 : &uart_settings.uart2;
	uint8_t i;
	
	// Initialize OS items
	*ctx->xQueue = xQueueCreate( 5, sizeof( uart_receiver_msg_t ) );
	if( *ctx->xQueue == 0 )
		while(1);
	
	vSemaphoreCreateBinary( *ctx->xSyncSem );
	if (*ctx->xSyncSem == 0)
		while(1);
		
	// Mutex that will be used for UART TX task synchronization
	// must be initially taken
	taskENTER_CRITICAL();
	*ctx->hwUART_mutex = xSemaphoreCreateMutex();
	if(*ctx->hwUART_mutex == 0)
		while(1); 
	xSemaphoreTake(*ctx->hwUART_mutex, 0);
	taskEXIT_CRITICAL();
	
	//-------------------------------//
	
	// Initialize UART hardware
	UART_Initialize(ctx->uart_num);
	UART_Register_RX_Stream_Callback(ctx->uart_num, UartRxStreamParser);
	
	// Initialize parsef stuff
	parser.received_msg_ptr = parser.received_msg;
	parser.chars_to_read = RX_MESSAGE_MAX_LENGTH;
	
	while(1)
	{
		xQueueReceive(*ctx->xQueue, &msg, portMAX_DELAY);
		switch (msg.type)
		{
			case UART_INITIAL_START:
				// Read global settings, setup UART hardware and start listening if required
				UART_Enable(ctx->uart_num, my_uart_settings->baudRate, my_uart_settings->parity);
				if (my_uart_settings->enable)
				{
					// Allows RX requests to DMA
					UART_Start_RX(ctx->uart_num);
					// Releasing mutex will allow UART TX task operation
					xSemaphoreGive(*ctx->hwUART_mutex);
				}
				break;
			case UART_APPLY_NEW_SETTINGS:
				// We have to stop receiver before waiting for TX mutex to avoid DMA RX buffer overflow
				// Disable receiver. After that there may be some data in the buffer.
				UART_Stop_RX(ctx->uart_num);
				// Get mutex for TX UART - prevent TX UART task from using hardware
				xSemaphoreTake(*ctx->hwUART_mutex, portMAX_DELAY);
				// Wait until UART transmitter is done and turn UART off
				UART_Disable(ctx->uart_num);
				// Reset receive buffer items
				UART_clean_RX_stream(ctx->uart_num);
				parser.received_msg_ptr = parser.received_msg;
				parser.chars_to_read = RX_MESSAGE_MAX_LENGTH;
				// Apply settings
				UART_Enable(ctx->uart_num, msg.brate, msg.parity);
				// TODO: analyze return code
				my_uart_settings->baudRate = msg.brate;
				my_uart_settings->parity = msg.parity;
				my_uart_settings->enable = msg.enable;
				if (my_uart_settings->enable)
				{
					// Enables UART and allows RX requests to DMA
					UART_Start_RX(ctx->uart_num);
					// Releasing mutex will allow UART TX task operation
					xSemaphoreGive(*ctx->hwUART_mutex);
				}
				break;
			case UART_RX_TICK:
				parser.analyze_message = UART_read_RX_stream(ctx->uart_num, &parser.received_msg_ptr, &parser.chars_to_read);
				if (parser.analyze_message)
				{
					parser.msg_length = RX_MESSAGE_MAX_LENGTH - parser.chars_to_read;
					
					// Split message string into argument words
					parser.argc = 0;
					parser.searchWord = 1;
					for (i=0; i<parser.msg_length; i++)
					{
						if ( (parser.received_msg[i] == ' ') || (parser.received_msg[i] == '\t') || 
							 (parser.received_msg[i] == '\n') || (parser.received_msg[i] == '\r'))
						{
							parser.received_msg[i] = 0;
							parser.searchWord = 1;
						}
						else if (parser.searchWord)
						{
							parser.argv[parser.argc++] = &parser.received_msg[i];
							parser.searchWord = 0;
						}
					}
					// If there are any words
					if (parser.argc > 0)
					{
						// Clear argument being passed to exe function -
						// actualy fill args[i].type with ATYPE_NONE (0)
						memset(ctx->exeFuncArgs, 0, sizeof(ctx->exeFuncArgs));
						
						// Parse string into function and arguments
						ctx->exeFunc = parse_argv(parser.argv, parser.argc, ctx->exeFuncArgs);
					
						// Execute function
						execute_command(ctx->exeFunc, ctx->uart_num, ctx->exeFuncArgs);
					}
					// Done - reset parser stuff
					parser.chars_to_read = RX_MESSAGE_MAX_LENGTH;
					parser.received_msg_ptr = parser.received_msg;
				}	
				break;
		}	
	}
}




//=================================================================//
//  Parser-called functions
//=================================================================//





// This is a huge switch-based procedure for executing commands
// received from UARTs.
// Possibly there is a better way to implement it.
static void execute_command(uint8_t cmd_code, uint8_t uart_num, arg_t *args)
{
	converter_message_t converter_msg;
	xSemaphoreHandle *xSem = (uart_num == 1) ? &xSyncSemaphore1 : &xSyncSemaphore2;
	gui_msg_t gui_msg;
	dispatch_msg_t dispatcher_msg;
	uart_transmiter_msg_t transmitter_msg;
	xQueueHandle *xQueueUARTTX = (uart_num == 1) ? &xQueueUART1TX : &xQueueUART2TX;
	dispatcher_msg.type = 0;
	dispatcher_msg.sender = (uart_num == 1) ? sender_UART1 : sender_UART2;
	transmitter_msg.type = 0;
	gui_msg.type = 0;
	
	converter_msg.pxSemaphore = xSem;
	converter_msg.sender = (uart_num == 1) ? sender_UART1 : sender_UART2;
	
	switch (cmd_code)
	{
		//=========================================================================//
		// Converter command
		case UART_CMD_CONVERTER:
			if (args[0].type != 0)
			{
				switch(args[0].flag)
				{
					case 0:
						// Turn converter OFF
						converter_msg.type = CONVERTER_TURN_OFF;
						xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
						xSemaphoreTake(*xSem, portMAX_DELAY);			// Wait
						transmitter_msg.type = UART_RESPONSE_OK;		// Ack
						break;
					case 1:
						// Turn converter ON
						converter_msg.type = CONVERTER_TURN_ON;
						xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
						xSemaphoreTake(*xSem, portMAX_DELAY);			// Wait
						transmitter_msg.type = UART_RESPONSE_OK;		// Ack
						break;
					case 2:
						// Set voltage
						if (args[3].type != 0)
						{
							converter_msg.type = CONVERTER_SET_VOLTAGE;
							converter_msg.a.v_set.channel = (args[1].type) ? args[1].flag : OPERATING_CHANNEL;	
							converter_msg.a.v_set.value = (int32_t)args[3].data32u;
							xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
							xSemaphoreTake(*xSem, portMAX_DELAY);			// Wait
							transmitter_msg.type = UART_RESPONSE_OK;		// Ack
						}
						else
						{
							// value is missing
							transmitter_msg.type = UART_RESPONSE_MISSING_ARGUMENT;
						}
						break;
					case 3:
						// Set current
						if (args[3].type != 0)
						{
							converter_msg.type = CONVERTER_SET_CURRENT;
							converter_msg.a.c_set.channel = (args[1].type) ? args[1].flag : OPERATING_CHANNEL;	
							converter_msg.a.c_set.range = (args[2].type) ? args[2].flag : OPERATING_CURRENT_RANGE;
							converter_msg.a.c_set.value = (int32_t)args[3].data32u;
							xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
							xSemaphoreTake(*xSem, portMAX_DELAY);			// Wait
							transmitter_msg.type = UART_RESPONSE_OK;		// Ack
						}
						else
						{
							// value is missing
							transmitter_msg.type = UART_RESPONSE_MISSING_ARGUMENT;
						}
						break;
						
					default:
						transmitter_msg.type = UART_RESPONSE_WRONG_ARGUMENT;
				}		
			}
			else
			{
				transmitter_msg.type = UART_RESPONSE_MISSING_ARGUMENT;
			}
			break;
			
		//=========================================================================//
		// Key command
		case UART_CMD_KEY:
			if ((args[0].type != 0) && (args[1].type != 0))
			{
				gui_msg.type = GUI_TASK_PROCESS_BUTTONS;
				gui_msg.key_event.event = args[0].flag;
				gui_msg.key_event.code = args[1].flag;
				transmitter_msg.type = UART_RESPONSE_OK;
			}
			else
			{
				// Unknown key event or code
				transmitter_msg.type = UART_RESPONSE_WRONG_ARGUMENT;
			}
			break;
			
		//=========================================================================//
		// Encoder command
		case UART_CMD_ENCODER:
			if (args[0].type != 0)
			{
				gui_msg.type = GUI_TASK_PROCESS_ENCODER;
				gui_msg.encoder_event.delta = args[0].data32;
				transmitter_msg.type = UART_RESPONSE_OK;
			}
			else
			{
				// Missing argument
				transmitter_msg.type = UART_RESPONSE_WRONG_ARGUMENT;
			}
			break;
			
		//=========================================================================//
		// Time profiling command
		case UART_CMD_PROFILING:
			transmitter_msg.type = UART_SEND_PROFILING;
			break;
		
		//=========================================================================//
		// Test function 1 command	
		case UART_CMD_TEST1:
			dispatcher_msg.type = DISPATCHER_TEST_FUNC1;
			break;
		
		//=========================================================================//
		// Unknown command
		default:
			transmitter_msg.type = UART_RESPONSE_UNKNOWN_CMD;
	}
	
	
	// Send message to the GUI
	if (gui_msg.type != 0)
	{
		xQueueSendToBack(xQueueGUI, &gui_msg, portMAX_DELAY);
	}
	
	// Send message to the dispatcher
	if (dispatcher_msg.type != 0)
	{
		xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
	}
		
	// Send message directly to UART
	if (transmitter_msg.type != 0)
	{
		xQueueSendToBack(*xQueueUARTTX, &transmitter_msg, portMAX_DELAY);
	}
	
}








