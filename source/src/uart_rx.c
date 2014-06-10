
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdint.h"

#include "MDR32F9Qx_uart.h"
#include "MDR32F9Qx_dma.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "global_def.h"
#include "key_def.h"		// button codes
#include "uart_parser.h"
#include "uart_rx.h"
#include "uart_tx.h"
#include "systemfunc.h"
#include "dispatcher.h"
#include "converter.h"
#include "guiTop.h"
#include "eeprom.h"



extern DMA_CtrlDataTypeDef DMA_ControlTable[];


// Receiver buffers
static uint16_t 				uart1_rx_data_buff[RX_BUFFER_SIZE];
static uint16_t 				uart2_rx_data_buff[RX_BUFFER_SIZE];
static uart_dma_rx_buffer_t 	uart1_rx_dma_buffer;
static uart_dma_rx_buffer_t 	uart2_rx_dma_buffer;

// Saved DMA configuration for fast reload in ISR
uint32_t UART1_RX_saved_TCB[3];
uint32_t UART2_RX_saved_TCB[3];

// RX task buffers which store messages that are read from DMA stream
// Defining these buffers as global saves task stack
static char uart1_received_msg[RX_MESSAGE_MAX_LENGTH];
static char uart2_received_msg[RX_MESSAGE_MAX_LENGTH];


static void execute_command(uint8_t cmd_code, uint8_t uart_num, arg_t *args);




static void UART_Init_RX_buffer(uart_dma_rx_buffer_t *rx_buffer, uint16_t *data, uint32_t size)
{
	rx_buffer->data = data;
	rx_buffer->size = size;
	rx_buffer->read_index = 0;
	
	// Fill buffer with invalid data
	while(size)
	{
		data[--size] = EMPTY_DATA;
	}
}

static uint8_t UART_Get_from_RX_buffer(uart_dma_rx_buffer_t *rx_buffer, uint16_t *rx_word)
{
	*rx_word = rx_buffer->data[rx_buffer->read_index];
	if (*rx_word != EMPTY_DATA)
	{
		// Valid data had been put into buffer by DMA
		rx_buffer->data[rx_buffer->read_index] = EMPTY_DATA;	
		rx_buffer->read_index = (rx_buffer->read_index < (rx_buffer->size - 1)) ? rx_buffer->read_index + 1 : 0;
		return 1;
	}
	return 0;
}


// DMA channel UARTx RX configuration 
static void UART_init_RX_DMA(MDR_UART_TypeDef *MDR_UARTx, uart_dma_rx_buffer_t *rx_buffer, uint32_t *saved_tcb)
{
	DMA_ChannelInitTypeDef DMA_InitStr;
	DMA_CtrlDataInitTypeDef DMA_PriCtrlStr;
	uint32_t *tcb_ptr;
	uint32_t DMA_Channel_UARTn_RX = (MDR_UARTx == MDR_UART1) ? DMA_Channel_UART1_RX : DMA_Channel_UART2_RX;
	
	// Setup Primary Control Data 
	DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)(&(MDR_UARTx->DR));
	DMA_PriCtrlStr.DMA_DestBaseAddr = (uint32_t)rx_buffer->data;			// dest (buffer of 16-bit shorts)
	DMA_PriCtrlStr.DMA_SourceIncSize = DMA_SourceIncNo;
	DMA_PriCtrlStr.DMA_DestIncSize = DMA_DestIncHalfword ;
	DMA_PriCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_PriCtrlStr.DMA_Mode = DMA_Mode_Basic;								// 
	DMA_PriCtrlStr.DMA_CycleSize = rx_buffer->size;							// count of 16-bit shorts
	DMA_PriCtrlStr.DMA_NumContinuous = DMA_Transfers_1;
	DMA_PriCtrlStr.DMA_SourceProtCtrl = DMA_SourcePrivileged;				// ?
	DMA_PriCtrlStr.DMA_DestProtCtrl = DMA_DestPrivileged;					// ?
	
	// Setup Channel Structure
	DMA_InitStr.DMA_PriCtrlData = &DMA_PriCtrlStr;
	DMA_InitStr.DMA_AltCtrlData = 0;										// Not used
	DMA_InitStr.DMA_ProtCtrl = 0;											// Not used
	DMA_InitStr.DMA_Priority = DMA_Priority_High ;
	DMA_InitStr.DMA_UseBurst = DMA_BurstClear;								// Enable single words trasfer
	DMA_InitStr.DMA_SelectDataStructure = DMA_CTRL_DATA_PRIMARY;
	
	// Init DMA channel
	my_DMA_ChannelInit(DMA_Channel_UARTn_RX, &DMA_InitStr);
	
	// Save created RX UART DMA control block for reinit in DMA ISR
	tcb_ptr = (uint32_t*)&DMA_ControlTable[DMA_Channel_UARTn_RX];
	saved_tcb[0] = *tcb_ptr++;
	saved_tcb[1] = *tcb_ptr++;
	saved_tcb[2] = *tcb_ptr;
}


// Initializes DMA for UART receive
static void UART_init_and_start_RX_DMA(uint8_t uart_num)
{
	if (uart_num == 1)
	{
		// Setup and init receiver buffer
		UART_Init_RX_buffer(&uart1_rx_dma_buffer, uart1_rx_data_buff, RX_BUFFER_SIZE);
		// Setup DMA channel
		UART_init_RX_DMA(MDR_UART1, &uart1_rx_dma_buffer, UART1_RX_saved_TCB);
		// Enable DMA channel
		DMA_Cmd(DMA_Channel_UART1_RX, ENABLE);	
	}
	else
	{
		// Setup and init receiver buffer
		UART_Init_RX_buffer(&uart2_rx_dma_buffer, uart2_rx_data_buff, RX_BUFFER_SIZE);
		// Setup DMA channel
		UART_init_RX_DMA(MDR_UART2, &uart2_rx_dma_buffer, UART2_RX_saved_TCB);
		// Enable DMA channel
		DMA_Cmd(DMA_Channel_UART2_RX, ENABLE);
	}
}


static void UART_start_RX(MDR_UART_TypeDef *MDR_UARTx)
{
	// Enable UARTn
	UART_Cmd(MDR_UARTx, ENABLE);
	// Enable UARTn DMA Rx request
	UART_DMACmd(MDR_UARTx, UART_DMA_RXE, ENABLE);
}


static void UART_stop_RX(MDR_UART_TypeDef *MDR_UARTx)
{	
	// Disable UARTn DMA Rx request
	UART_DMACmd(MDR_UARTx, UART_DMA_RXE, DISABLE);
	
	// Wait until current symbol is completely received - how?		
	while (UART_GetFlagStatus(MDR_UARTx, UART_FLAG_TXFE) == RESET) {};
	while (UART_GetFlagStatus(MDR_UARTx, UART_FLAG_BUSY) == SET) {};
		
	// Disable UARTn
	UART_Cmd(MDR_UARTx, DISABLE);
}




// Reads UART RX stream that is filled by DMA for num_of_chars_to_read symbols.
// If message terminating symbols are found, 1 is returned even if not all chars have been read.
static uint8_t UART_read_rx_stream(uint8_t uart_num, char **rx_string, uint16_t *num_of_chars_to_read)
{
	uint16_t rx_word;
	char rx_char;
	uart_dma_rx_buffer_t *dma_buf_ptr = (uart_num == 1) ? &uart1_rx_dma_buffer : &uart2_rx_dma_buffer;
	
	if ((rx_string == 0) || (num_of_chars_to_read == 0) || (*num_of_chars_to_read == 0))
		return 0;
	
	// Read chars from DMA stream
	while (UART_Get_from_RX_buffer(dma_buf_ptr, &rx_word))
	{
		// Check for communication errors
		if ((rx_word & ( (1<<UART_Data_BE) | (1<<UART_Data_PE) | (1<<UART_Data_FE) )) != 0)
			continue;
		rx_char = (char)rx_word;
		*(*rx_string)++ = rx_char;
		(*num_of_chars_to_read)--;
		if ((*num_of_chars_to_read == 0) || (rx_char == '\n') || (rx_char == '\r'))
			return 1;
	}
	return 0;
}

// Cleans UART RX stream
static void UART_clean_rx_stream(uint8_t uart_num)
{
	uint16_t dummy_word;
	uart_dma_rx_buffer_t *dma_buf_ptr = (uart_num == 1) ? &uart1_rx_dma_buffer : &uart2_rx_dma_buffer;
	while(UART_Get_from_RX_buffer(dma_buf_ptr, &dummy_word));
}



void UART_Get_comm_params(uint8_t uart_num, uart_param_request_t *r)
{
	uartx_settings_t *my_uart_settings = (uart_num == 1) ? &uart_settings.uart1 : &uart_settings.uart2;
	r->enable = my_uart_settings->enable;
	r->brate = my_uart_settings->baudRate;
	r->parity = my_uart_settings->parity;
}




static xSemaphoreHandle xSemaphoreConverter1, xSemaphoreConverter2;

xSemaphoreHandle hwUART1_mutex;
xSemaphoreHandle hwUART2_mutex;


xQueueHandle xQueueUART1RX, xQueueUART2RX;

uart_receiver_msg_t uart_rx_tick_msg = {UART_RX_TICK};


void vTaskUARTReceiver(void *pvParameters)
{
	// Uart number for task is passed by argument
	uint8_t my_uart_num = (uint32_t)pvParameters;
	xQueueHandle *xQueueUARTx = (my_uart_num == 1) ? &xQueueUART1RX : &xQueueUART2RX;
	MDR_UART_TypeDef *MDR_UARTx = (my_uart_num == 1) ? MDR_UART1 : MDR_UART2;
	uartx_settings_t *my_uart_settings = (my_uart_num == 1) ? &uart_settings.uart1 : &uart_settings.uart2;
	uart_receiver_msg_t msg;
	xSemaphoreHandle *hwUART_mutex = (my_uart_num == 1) ? &hwUART1_mutex : &hwUART2_mutex;
	char *received_msg = (my_uart_num == 1) ? uart1_received_msg : uart2_received_msg;
	char *received_msg_ptr = received_msg;
	uint16_t chars_to_read = RX_MESSAGE_MAX_LENGTH;
	uint16_t msg_length = 0;
	uint8_t analyze_message;
	uint8_t rx_enable = 0;
	// Parser stuff
	char *argv[MAX_WORDS_IN_MESSAGE];		// Array of pointers to separate words
	uint16_t argc = 0;						// Count of words in message
	uint8_t i, searchWord;
	uint8_t exeFunc;
    arg_t exeFuncArgs[MAX_FUNCTION_ARGUMENTS];
	
	// Initialize OS items
	*xQueueUARTx = xQueueCreate( 5, sizeof( uart_receiver_msg_t ) );
	if( *xQueueUARTx == 0 )
		while(1);
	
	//------------//
	vSemaphoreCreateBinary( xSemaphoreConverter1 );
	if( xSemaphoreConverter1 == 0 )
    {
        while(1);
    }
	xSemaphoreTake(xSemaphoreConverter1, 0);
	
	vSemaphoreCreateBinary( xSemaphoreConverter2 );
	if( xSemaphoreConverter2 == 0 )
    {
        while(1);
    }
	xSemaphoreTake(xSemaphoreConverter2, 0);
	
	// Create mutex that will be used for UART TX task synchronization
	// It must be initially taken
	taskENTER_CRITICAL();
	*hwUART_mutex = xSemaphoreCreateMutex();
	if( *hwUART_mutex == 0 )
		while(1); 
	xSemaphoreTake(*hwUART_mutex, 0);
	taskEXIT_CRITICAL();
	
	//------------//
	
	// Start DMA operation
	UART_init_and_start_RX_DMA(my_uart_num);
	
	
	while(1)
	{
		xQueueReceive(*xQueueUARTx, &msg, portMAX_DELAY);
		switch (msg.type)
		{
			case UART_INITIAL_START:
				// Read global settings, setup UART hardware and start listening if required
				HW_UART_Set_Comm_Params(MDR_UARTx, my_uart_settings->baudRate, my_uart_settings->parity);
				if (my_uart_settings->enable)
				{
					UART_start_RX(MDR_UARTx);
					rx_enable = 1;
					xSemaphoreGive(*hwUART_mutex);
				}
				break;
			case UART_APPLY_NEW_SETTINGS:
				// First have to stop TX
				xSemaphoreTake(*hwUART_mutex, portMAX_DELAY);
				UART_stop_RX(MDR_UARTx);
				// CHECKME - stop RX before waiting for TX mutex to avoid DMA RX buffer overflow
				// Reset buffer items
				UART_clean_rx_stream(my_uart_num);
				chars_to_read = RX_MESSAGE_MAX_LENGTH;
				received_msg_ptr = received_msg;
				// Apply settings
				HW_UART_Set_Comm_Params(MDR_UARTx, msg.brate, msg.parity);
				// TODO: analyze return code
				my_uart_settings->baudRate = msg.brate;
				my_uart_settings->parity = msg.parity;
				my_uart_settings->enable = msg.enable;
				if (my_uart_settings->enable)
				{
					UART_start_RX(MDR_UARTx);
					rx_enable = 1;
					xSemaphoreGive(*hwUART_mutex);
				}
				else
				{
					rx_enable = 0;
				}
				break;
			case UART_RX_TICK:
				if (rx_enable == 0)
					break;
				analyze_message = UART_read_rx_stream(my_uart_num, &received_msg_ptr, &chars_to_read);
				if (analyze_message)
				{
					msg_length = RX_MESSAGE_MAX_LENGTH - chars_to_read;
					
					// Split message string into argument words
					argc = 0;
					searchWord = 1;
					for (i=0; i<msg_length; i++)
					{
						if ( (received_msg[i] == ' ') || (received_msg[i] == '\t') || 
							 (received_msg[i] == '\n') || (received_msg[i] == '\r'))
						{
							received_msg[i] = 0;
							searchWord = 1;
						}
						else if (searchWord)
						{
							argv[argc++] = &received_msg[i];
							searchWord = 0;
						}
					}
					
					if (argc > 0)
					{
						// Clear parsedArguments to function -
						// actualy fill args[i].type with ATYPE_NONE
						memset(exeFuncArgs, 0, sizeof(exeFuncArgs));
						
						// Parse string into function and arguments
						exeFunc = parse_argv(argv, argc, exeFuncArgs);
					
						// Execute function
						execute_command(exeFunc, my_uart_num, exeFuncArgs);
					}
					
					chars_to_read = RX_MESSAGE_MAX_LENGTH;
					received_msg_ptr = received_msg;
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
	xSemaphoreHandle *xSem = (uart_num == 1) ? &xSemaphoreConverter1 : &xSemaphoreConverter2;
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








