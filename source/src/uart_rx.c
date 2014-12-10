
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
#include "uart_rx.h"
#include "uart_tx.h"
#include "uart_proto.h"
#include "systemfunc.h"
#include "dispatcher.h"
#include "converter.h"
#include "guiTop.h"
#include "eeprom.h"


static void execute_command(char **argv, uint8_t argc, uint8_t uart_num);


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



// Reads UART communication settings for specified UART number from
// global settings
// Used by GUI
void UART_Get_comm_params(uint8_t uart_num, uart_param_request_t *r) {
	uartx_settings_t *my_uart_settings = (uart_num == 1) ? &global_settings->uart1 : &global_settings->uart2;
	r->enable = my_uart_settings->enable;
	r->brate = my_uart_settings->baudRate;
	r->parity = my_uart_settings->parity;
}


//-------------------------------------------------------//
// Low-level callback
static uint8_t UartRxStreamParser(char c) {
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
	uartx_settings_t *my_uart_settings = (ctx->uart_num == 1) ? &global_settings->uart1 : &global_settings->uart2;
	uint8_t i;
	
	// Initialize OS items
	*ctx->xQueue = xQueueCreate( 5, sizeof( uart_receiver_msg_t ) );
	if(*ctx->xQueue == 0)
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
	
	while(1) {
		xQueueReceive(*ctx->xQueue, &msg, portMAX_DELAY);
		switch (msg.type) {
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
				if (xSemaphoreGetMutexHolder(*ctx->hwUART_mutex) != xTaskGetCurrentTaskHandle())
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
				taskENTER_CRITICAL();
				my_uart_settings->baudRate = msg.brate;
				my_uart_settings->parity = msg.parity;
				my_uart_settings->enable = msg.enable;
				taskEXIT_CRITICAL();
				if (my_uart_settings->enable) {
					// Enables UART and allows RX requests to DMA
					UART_Start_RX(ctx->uart_num);
					// Releasing mutex will allow UART TX task operation
					xSemaphoreGive(*ctx->hwUART_mutex);
				}
				break;
			case UART_RX_TICK:
				parser.analyze_message = UART_read_RX_stream(ctx->uart_num, &parser.received_msg_ptr, &parser.chars_to_read);
				if (parser.analyze_message) {
					parser.msg_length = RX_MESSAGE_MAX_LENGTH - parser.chars_to_read;
					// Split message string into argument words
					parser.argc = 0;
					parser.searchWord = 1;
					for (i=0; i<parser.msg_length; i++) {
						if ((parser.received_msg[i] == ' ') || (parser.received_msg[i] == '\t') || 
								(parser.received_msg[i] == '\n') || (parser.received_msg[i] == '\r')) {
							parser.received_msg[i] = 0;
							parser.searchWord = 1;
						} else if (parser.searchWord) {
							parser.argv[parser.argc++] = &parser.received_msg[i];
							parser.searchWord = 0;
						}
					}
					// If there are any words
					if (parser.argc > 0) {
						// Execute function
						execute_command(parser.argv, parser.argc, ctx->uart_num);
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

static void execute_command(char **argv, uint8_t argc, uint8_t uart_num)
{
	converter_message_t converter_msg;
	gui_msg_t gui_msg;
	dispatch_msg_t dispatcher_msg;
	uart_transmiter_msg_t transmitter_msg;
	xSemaphoreHandle *xSem = (uart_num == 1) ? &xSyncSemaphore1 : &xSyncSemaphore2;
	xQueueHandle *xQueueUARTTX = (uart_num == 1) ? &xQueueUART1TX : &xQueueUART2TX;
	
	uint8_t action_get = 0;
    uint8_t action_set = 0;
    uint32_t temp32u;
	int32_t temp32;
	uint8_t temp8u;
    char *temp_string;
    uint8_t arg_channel;
    uint8_t arg_crange;
	uint8_t errorCode = NO_ERROR;
	
	dispatcher_msg.sender = (uart_num == 1) ? sender_UART1 : sender_UART2;
	converter_msg.pxSemaphore = xSem;
	converter_msg.sender = (uart_num == 1) ? sender_UART1 : sender_UART2;
	converter_msg.type = CONVERTER_CONTROL;
	
	
	// Actions are common for all parameters and properties
    if (getIndexOfKey(argv, argc,proto.actions.get) >= 0) {
        action_get = 1;
    } else if (getIndexOfKey(argv, argc,proto.actions.set) >= 0) {
        action_set = 1;
    } else {
        action_get = 1;
    }
	
	//=========================================================================//
	// Group CONVERTER
    if (getIndexOfKey(argv, argc, proto.groups.converter) >= 0)
    {
		transmitter_msg.type = UART_SEND_CONVERTER_DATA;
		transmitter_msg.spec = UART_MSG_ACK;
		
		// Parse common arguments
		// Channel and current range arguments may be omitted - in this case
		// active channel and active current range for that channel will be used
        arg_channel = (getIndexOfKey(argv, argc, proto.flags.ch5v) >= 0) ? CHANNEL_5V :
                    ((getIndexOfKey(argv, argc, proto.flags.ch12v) >= 0) ? CHANNEL_12V : 
					Converter_GetFeedbackChannel());
        arg_crange = (getIndexOfKey(argv, argc, proto.flags.crangeLow) >= 0) ? CURRENT_RANGE_LOW :
                    ((getIndexOfKey(argv, argc, proto.flags.crangeHigh) >= 0) ? CURRENT_RANGE_HIGH : 
					Converter_GetCurrentRange(arg_channel));

        // Determine parameter
        if (getIndexOfKey(argv, argc, proto.parameters.state) >= 0) {
            // Parameter STATE
            if (action_set) {
				errorCode = getStringForKey(argv, argc, proto.keys.state, &temp_string);
                if (errorCode == NO_ERROR) {
                    if (strcmp(temp_string, proto.values.on) == 0) {
                        converter_msg.a.state_set.command = cmd_TURN_ON;
                    } else if (strcmp(temp_string, proto.values.off) == 0) {
                        converter_msg.a.state_set.command = cmd_TURN_OFF;
                    } else {
						errorCode = ERROR_ILLEGAL_VALUE;
                    }
				}
				if (errorCode == NO_ERROR) {
					converter_msg.param = param_STATE;
					xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
					xSemaphoreTake(*xSem, portMAX_DELAY);			// Wait
				}
            }
			// Acknowledge
			if (errorCode == NO_ERROR) {
				transmitter_msg.converter.param = param_STATE;
				xQueueSendToBack(*xQueueUARTTX, &transmitter_msg, portMAX_DELAY);
			}
        }
		else if (getIndexOfKey(argv, argc,proto.parameters.channel) >= 0) {
            // Parameter CHANNEL
            if (action_set) {
				errorCode = ERROR_PARAM_READONLY;
			} else {
				transmitter_msg.converter.param = param_CHANNEL;
				xQueueSendToBack(*xQueueUARTTX, &transmitter_msg, portMAX_DELAY);
			}
        }
		else if (getIndexOfKey(argv, argc,proto.parameters.crange) >= 0) {
            // Parameter CURRENT RANGE
			if (action_set) {
				converter_msg.a.crange_set.channel = arg_channel;
				converter_msg.a.crange_set.new_range = arg_crange;
				converter_msg.param = param_CRANGE;
				xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
				xSemaphoreTake(*xSem, portMAX_DELAY);
			}
			// Acknowledge
			transmitter_msg.converter.param = param_CRANGE;
			transmitter_msg.converter.channel = arg_channel;
			xQueueSendToBack(*xQueueUARTTX, &transmitter_msg, portMAX_DELAY);
        }
		else if (getIndexOfKey(argv, argc,proto.parameters.vset) >= 0) {
            // Parameter VSET
            if (action_set) {
				errorCode = getValueUI32ForKey(argv, argc, proto.keys.vset, &temp32u);
                if (errorCode == NO_ERROR) {
                    converter_msg.a.v_set.channel = arg_channel;
					converter_msg.a.v_set.value = temp32u;
					converter_msg.param = param_VSET;
					xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
					xSemaphoreTake(*xSem, portMAX_DELAY);
				}
            }
            // Acknowledge
			if (errorCode == NO_ERROR) {
				transmitter_msg.converter.param = param_VSET;
				transmitter_msg.converter.channel = arg_channel;				
				xQueueSendToBack(*xQueueUARTTX, &transmitter_msg, portMAX_DELAY);
			}
        }
		else if (getIndexOfKey(argv, argc,proto.parameters.cset) >= 0) {
            // Parameter CSET
            if (action_set) {
				errorCode = getValueUI32ForKey(argv, argc, proto.keys.cset, &temp32u);
                if (errorCode == NO_ERROR) {
                    converter_msg.a.c_set.channel = arg_channel;
					converter_msg.a.c_set.range = arg_crange;
					converter_msg.a.c_set.value = temp32u;
					converter_msg.param = param_CSET;
					xQueueSendToBack(xQueueConverter, &converter_msg, portMAX_DELAY);
					xSemaphoreTake(*xSem, portMAX_DELAY);
				}
            }
            // Acknowledge
			if (errorCode == NO_ERROR) {
				transmitter_msg.converter.param = param_CSET;
				transmitter_msg.converter.channel = arg_channel;
				transmitter_msg.converter.current_range = arg_crange;
				xQueueSendToBack(*xQueueUARTTX, &transmitter_msg, portMAX_DELAY);
			}
        }
    }	// end of group CONVERTER
	//=========================================================================//
	// Group BUTTONS
	else if (getIndexOfKey(argv, argc, proto.groups.buttons) >= 0)
	{	
		// Process button events
		temp8u = getStringForKey(argv, argc, proto.keys.btn_event, &temp_string);
		if (temp8u == NO_ERROR) {
			if (strcmp(temp_string, proto.button_events.down) == 0) {
				gui_msg.key_event.event = BTN_EVENT_DOWN;
			} else if (strcmp(temp_string, proto.button_events.up) == 0) {
				gui_msg.key_event.event = BTN_EVENT_UP;
			} else if (strcmp(temp_string, proto.button_events.up_short) == 0) {
				gui_msg.key_event.event = BTN_EVENT_UP_SHORT;
			} else if (strcmp(temp_string, proto.button_events.up_long) == 0) {
				gui_msg.key_event.event = BTN_EVENT_UP_LONG;
			} else if (strcmp(temp_string, proto.button_events.hold) == 0) {
				gui_msg.key_event.event = BTN_EVENT_HOLD;
			} else if (strcmp(temp_string, proto.button_events.repeat) == 0) {
				gui_msg.key_event.event = BTN_EVENT_REPEAT;
			} else {
				errorCode = ERROR_ILLEGAL_VALUE;
			}
			if (errorCode == NO_ERROR) {
				errorCode = getStringForKey(argv, argc, proto.keys.btn_code, &temp_string);
				if (errorCode == NO_ERROR) {
					if (strcmp(temp_string, proto.button_codes.esc) == 0) {
						gui_msg.key_event.code = BTN_ESC;
					} else if (strcmp(temp_string, proto.button_codes.ok) == 0) {
						gui_msg.key_event.code = BTN_OK;
					} else if (strcmp(temp_string, proto.button_codes.left) == 0) {
						gui_msg.key_event.code = BTN_LEFT;
					} else if (strcmp(temp_string, proto.button_codes.right) == 0) {
						gui_msg.key_event.code = BTN_RIGHT;
					} else if (strcmp(temp_string, proto.button_codes.on) == 0) {
						gui_msg.key_event.code = BTN_ON;
					} else if (strcmp(temp_string, proto.button_codes.off) == 0) {
						gui_msg.key_event.code = BTN_OFF;
					} else if (strcmp(temp_string, proto.button_codes.encoder) == 0) {
						gui_msg.key_event.code = BTN_ENCODER;
					} else {
						errorCode = ERROR_ILLEGAL_VALUE;
					}
				}
			}
			if (errorCode == NO_ERROR) {
				gui_msg.type = GUI_TASK_PROCESS_BUTTONS;
				xQueueSendToBack(xQueueGUI, &gui_msg, portMAX_DELAY);
			}
		}
		// Process encoder events
		temp8u = getValueI32ForKey(argv, argc, proto.keys.enc_delta, &temp32);
		if (temp8u == NO_ERROR) {
			gui_msg.type = GUI_TASK_PROCESS_ENCODER;
			gui_msg.encoder_event.delta = temp32;
		}
	}	// end of group BUTTONS
	//=========================================================================//
	// Group PROFILING
	else if (getIndexOfKey(argv, argc, proto.groups.profiling) >= 0)
	{
		transmitter_msg.type = UART_SEND_PROFILING;
		xQueueSendToBack(*xQueueUARTTX, &transmitter_msg, portMAX_DELAY);
	}
	//=========================================================================//
	// Group TEST
	else if (getIndexOfKey(argv, argc, proto.groups.test) >= 0)
	{
		dispatcher_msg.type = DISPATCHER_TEST_FUNC1;
		xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
	}
	//=========================================================================//
	// No group or unknown
	else
	{
		// Unknown group
		errorCode = ERROR_CANNOT_DETERMINE_GROUP;
	}
	
	
	
	if (errorCode != NO_ERROR) {
		// Send error code to the transmitter task 
		transmitter_msg.type = UART_SEND_ERROR;
		transmitter_msg.error.code = errorCode;
		xQueueSendToBack(*xQueueUARTTX, &transmitter_msg, portMAX_DELAY);
	}
	
}






