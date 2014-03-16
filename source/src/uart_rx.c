
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdint.h"

#include "MDR32F9Qx_uart.h"
#include "MDR32F9Qx_dma.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "global_def.h"
#include "key_def.h"		// button codes
#include "uart_parser.h"
#include "uart_rx.h"
#include "uart_tx.h"
#include "systemfunc.h"
#include "dispatcher.h"
#include "converter.h"



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
		// Enable UARTn DMA Rx request
		UART_DMACmd(MDR_UART1, UART_DMA_RXE, ENABLE);
	}
	else
	{
		// Setup and init receiver buffer
		UART_Init_RX_buffer(&uart2_rx_dma_buffer, uart2_rx_data_buff, RX_BUFFER_SIZE);
		// Setup DMA channel
		UART_init_RX_DMA(MDR_UART2, &uart2_rx_dma_buffer, UART2_RX_saved_TCB);
		// Enable DMA channel
		DMA_Cmd(DMA_Channel_UART2_RX, ENABLE);
		// Enable UARTn DMA Rx request
		UART_DMACmd(MDR_UART2, UART_DMA_RXE, ENABLE);
	}
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





void vTaskUARTReceiver(void *pvParameters) 
{
	// Uart number for task is passed by argument
	uint8_t my_uart_num = (uint32_t)pvParameters;
	char *received_msg = (my_uart_num == 1) ? uart1_received_msg : uart2_received_msg;
	char *received_msg_ptr = received_msg;
	uint16_t chars_to_read = RX_MESSAGE_MAX_LENGTH;
	uint16_t msg_length = 0;
	uint8_t analyze_message;
	// Parser stuff
	char *argv[MAX_WORDS_IN_MESSAGE];		// Array of pointers to separate words
	uint16_t argc = 0;						// Count of words in message
	uint8_t i, searchWord;
	uint8_t exeFunc;
    arg_t exeFuncArgs[MAX_FUNCTION_ARGUMENTS];
	
	portTickType lastExecutionTime = xTaskGetTickCount();
	
	// Start operation
	UART_init_and_start_RX_DMA(my_uart_num);
	
	while(1)
	{
		vTaskDelayUntil(&lastExecutionTime, 5);		// 10ms period

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
	dispatch_msg_t dispatcher_msg;
	uart_transmiter_msg_t transmitter_msg;
	xQueueHandle *xQueueUARTTX = (uart_num == 1) ? &xQueueUART1TX : &xQueueUART2TX;
	dispatcher_msg.type = 0;
	dispatcher_msg.sender = (uart_num == 1) ? sender_UART1 : sender_UART2;
	transmitter_msg.type = 0;

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
						dispatcher_msg.type = DISPATCHER_CONVERTER;
						dispatcher_msg.converter_cmd.msg_type = CONVERTER_TURN_OFF;
						break;
					case 1:
						// Turn converter ON
						dispatcher_msg.type = DISPATCHER_CONVERTER;
						dispatcher_msg.converter_cmd.msg_type = CONVERTER_TURN_ON;
						break;
					case 2:
						// Set voltage
						if (args[3].type != 0)
						{
							dispatcher_msg.type = DISPATCHER_CONVERTER;
							dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_VOLTAGE;
							dispatcher_msg.converter_cmd.a.v_set.channel = (args[1].type) ? args[1].flag : OPERATING_CHANNEL;	
							dispatcher_msg.converter_cmd.a.v_set.value = (int32_t)args[3].data32u;
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
							dispatcher_msg.type = DISPATCHER_CONVERTER;
							dispatcher_msg.converter_cmd.msg_type = CONVERTER_SET_CURRENT;
							dispatcher_msg.converter_cmd.a.c_set.channel = (args[1].type) ? args[1].flag : OPERATING_CHANNEL;	
							dispatcher_msg.converter_cmd.a.c_set.range = (args[2].type) ? args[2].flag : OPERATING_CURRENT_RANGE;
							dispatcher_msg.converter_cmd.a.c_set.value = (int32_t)args[3].data32u;
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
				dispatcher_msg.type = DISPATCHER_BUTTONS;
				dispatcher_msg.key_event.code = args[1].flag;
				dispatcher_msg.key_event.event_type = args[0].flag;
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
				dispatcher_msg.type = DISPATCHER_ENCODER;
				dispatcher_msg.encoder_event.delta = args[0].data32;
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
	
	
	
	// Send message to the dispatcher
	if (dispatcher_msg.type != 0)
	{
		xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);	
	}
		
	// Send message directly to UART
	if (transmitter_msg.type != 0)
	{
		xQueueSendToBack(*xQueueUARTTX, &transmitter_msg, 0);
	}
	
}








