
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "MDR32F9Qx_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "systick.h"
#include "stdint.h"
#include "uart.h"
#include "dispatcher.h"
#include "buttons.h"

#include "dwt_delay.h"

// Test
uint8_t test_mode = 0;

#define MDR_UARTn MDR_UART1

#if MDR_UARTn == MDR_UART2
const uint8_t DMA_Channel_UART_RX = DMA_Channel_UART2_RX;
const uint8_t DMA_Channel_UART_TX = DMA_Channel_UART2_TX;
#elif MDR_UARTn == MDR_UART1
const uint8_t DMA_Channel_UART_RX = DMA_Channel_UART1_RX;
const uint8_t DMA_Channel_UART_TX = DMA_Channel_UART1_TX;
#else
#error "No selected UART!"
#endif

// Transmitter buffers
static char 					uart2_tx_data_buff[TX_BUFFER_SIZE];
// Receiver buffers
static uint16_t 				uart2_rx_data_buff[RX_BUFFER_SIZE];
static uart_dma_rx_buffer_t 	uart2_rx_dma_buffer;

xQueueHandle xQueueUART2TX;




static void UART_sendStr(char *str);
static void UART_sendStrAlloc(char *str);


static void UART_Init_RX_buffer(uart_dma_rx_buffer_t *rx_buffer, uint16_t *data, uin32_t size)
{
	rx_buffer.data = data;
	rx_buffer.size = size;
	rx_buffer.read_index = 0;
	
	// Fill buffer with invalid data
	while(size)
	{
		data[--size] = EMPTY_DATA;
	}
}

uint8_t UART_Get_from_RX_buffer(uart_dma_rx_buffer_t *rx_buffer, uint16_t *rx_word)
{
	*rx_word = rx_buffer->data[rx_buffer->read_index];
	if (*rx_word != EMPTY_DATA)
	{
		// Valid data had been put into buffer by DMA
		rx_buffer->data[rx_buffer->read_index] = EMPTY_DATA;	
		rb->read_index = (rb->read_index < (rb->size - 1)) ? rb->read_index + 1 : 0;
		return 1;
	}
	return 0;
}



static void UART_init_RX_DMA(MDR_UART_TypeDef *MDR_UARTx, uint16_t *rx_buffer, uint32_t buffer_size)
{
	DMA_ChannelInitTypeDef DMA_InitStr;
	DMA_CtrlDataInitTypeDef DMA_PriCtrlStr, DMA_AltCtrlStr;
	DMA_StructInit(&DMA_InitStr);
	
	// DMA channel UARTx RX configuration 
	// Using Ping-pong mode
	
	// Set Primary Control Data 
	DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)(&(MDR_UARTx->DR));
	DMA_PriCtrlStr.DMA_DestBaseAddr = (uint32_t)rx_buffer;					// dest (buffer of 16-bit shorts)
	DMA_PriCtrlStr.DMA_SourceIncSize = DMA_SourceIncNo;
	DMA_PriCtrlStr.DMA_DestIncSize = DMA_DestIncHalfword ;
	DMA_PriCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_PriCtrlStr.DMA_Mode = DMA_Mode_PingPong;							// 
	DMA_PriCtrlStr.DMA_CycleSize = buffer_size;								// count of 16-bit shorts
	DMA_PriCtrlStr.DMA_NumContinuous = DMA_Transfers_8;
	DMA_PriCtrlStr.DMA_SourceProtCtrl = DMA_SourcePrivileged;				// ?
	DMA_PriCtrlStr.DMA_DestProtCtrl = DMA_DestPrivileged;					// ?
	
	// Set Alternate Control Data 
	DMA_AltCtrlStr.DMA_SourceBaseAddr = (uint32_t)(&(MDR_UARTx->DR));
	DMA_AltCtrlStr.DMA_DestBaseAddr = (uint32_t)rx_buffer;					
	DMA_AltCtrlStr.DMA_SourceIncSize = DMA_SourceIncNo;
	DMA_AltCtrlStr.DMA_DestIncSize = DMA_DestIncHalfword ;
	DMA_AltCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_AltCtrlStr.DMA_Mode = DMA_Mode_PingPong;							
	DMA_AltCtrlStr.DMA_CycleSize = buffer_size;								
	DMA_AltCtrlStr.DMA_NumContinuous = DMA_Transfers_8;
	DMA_AltCtrlStr.DMA_SourceProtCtrl = DMA_SourcePrivileged;				
	DMA_AltCtrlStr.DMA_DestProtCtrl = DMA_DestPrivileged;					
	
	// Set Channel Structure
	DMA_InitStr.DMA_PriCtrlData = &DMA_PriCtrlStr;
	DMA_InitStr.DMA_AltCtrlData = &DMA_AltCtrlStr;
	DMA_InitStr.DMA_Priority = DMA_Priority_High;
	DMA_InitStr.DMA_UseBurst = DMA_BurstClear;								// enable single words trasfer
	DMA_InitStr.DMA_SelectDataStructure = DMA_CTRL_DATA_PRIMARY;
	
	// Init DMA channel
	DMA_Init(DMA_Channel_UART_RX, &DMA_InitStr);
}



void vTaskUARTReceiver(void *pvParameters) 
{
	char received_msg[RX_MESSAGE_MAX_LENGTH];
	uint16_t msg_length = 0;
	char *argv[MAX_WORDS_IN_MESSAGE];		// Array of pointers to separate words
	uint16_t argc = 0;						// Count of words in message
	char temp_char;
	uint16_t uart_rx_word;
	uint16_t search_for_word = 1;
	uint8_t cmd_ok;
	
	
	// Debug
	uint16_t i;
	uint32_t temp32u;
	
	char temp_str[50];
	
	portTickType lastExecutionTime = xTaskGetTickCount();
	dispatch_incoming_msg_t dispatcher_msg;
	
	UART_Init_RX_buffer(&uart2_rx_dma_buffer, &uart2_rx_data_buff, RX_BUFFER_SIZE);
	UART_init_RX_DMA(MDR_UARTn, &uart2_rx_dma_buffer, RX_BUFFER_SIZE);
	
	
	while(1)
	{
		vTaskDelayUntil(&lastExecutionTime, 5);		// 10ms period

		// Read full message from buffer
		while(UART_Get_from_RX_buffer(&uart2_rx_dma_buffer, &uart_rx_word))
		{
			if ((uart_rx_word & ( (1<<UART_Data_BE) | (1<<UART_Data_PE) | (1<<UART_Data_FE) )) != 0)
				continue;
				
			temp_char = (char)(uart_rx_word);
			
			if (temp_char == SPACING_SYMBOL)
			{
				received_msg[msg_length++] = '\0';
				search_for_word = 1;
			}
			else if ((temp_char == MESSAGE_END_SYMBOL) || (temp_char == MESSAGE_NEW_LINE))
			{
				received_msg[msg_length++] = '\0';
			}
			else
			{
				// Normal char
				if (search_for_word == 1)
				{
					argv[argc++] = &received_msg[msg_length];		// Found start position of a word
					search_for_word = 0;
				}
				received_msg[msg_length++] = temp_char;
			}
			
			
			if ((temp_char == MESSAGE_END_SYMBOL) || (temp_char == MESSAGE_NEW_LINE) || (msg_length == RX_MESSAGE_MAX_LENGTH))
			{
				// Received full message OR maximum allowed message length is reached
				// Parse message
				if (argc != 0)
				{
					cmd_ok = 1;
					dispatcher_msg.type = 0;
					
					//---------- Converter control -----------//
					if (strcmp(argv[0], "on") == 0)							// Turn converter ON	
					{
						dispatcher_msg.type = DP_CONVERTER_TURN_ON;
					}
					else if (strcmp(argv[0], "off") == 0)					// Turn converter OFF
					{
						dispatcher_msg.type = DP_CONVERTER_TURN_OFF;
					}
					else if (strcmp(argv[0], "set_voltage") == 0)			// Setting converter voltage
					{
						if (argc < 2)
						{
							UART_sendStrAlloc(argv[0]);
							UART_sendStr(" ERR: missing argument [mV]\r");
							cmd_ok = 0;
						}
						else
						{
							// Second argument is voltage value [mV]
							dispatcher_msg.type = DP_CONVERTER_SET_VOLTAGE;
							dispatcher_msg.data = strtoul(argv[1], 0, 0);
						}
					}
					else if (strcmp(argv[0], "set_current") == 0)			// Setting converter current
					{
						if (argc < 2)
						{
							UART_sendStrAlloc(argv[0]);
							UART_sendStr(" ERR: missing argument [mA]\r");
							cmd_ok = 0;
						}
						else
						{
							// Second argument is current value [mA]
							dispatcher_msg.type = DP_CONVERTER_SET_CURRENT;
							dispatcher_msg.data = strtoul(argv[1], 0, 0);
						}
					}
					else if (strcmp(argv[0], "set_current_limit") == 0)			// Setting converter current limit
					{
						if (argc < 2)
						{
							UART_sendStrAlloc(argv[0]);
							UART_sendStr(" ERR: missing argument (20/40)[A]\r");
							cmd_ok = 0;
						}
						else
						{
							// Second argument is current limit value [A]
							dispatcher_msg.type = DP_CONVERTER_SET_CURRENT_LIMIT;
							dispatcher_msg.data = strtoul(argv[1], 0, 0);
						}
					}
					//----- button and encoder emulation -----//
					else if (strcmp(argv[0], "btn_esc") == 0)				// ESC button
					{
						dispatcher_msg.type = DP_EMU_BTN_DOWN;
						dispatcher_msg.data = BTN_ESC;
					}
					else if (strcmp(argv[0], "btn_ok") == 0)				// OK button
					{
						dispatcher_msg.type = DP_EMU_BTN_DOWN;
						dispatcher_msg.data = BTN_OK;
					}
					else if (strcmp(argv[0], "btn_left") == 0)				// LEFT button
					{
						dispatcher_msg.type = DP_EMU_BTN_DOWN;
						dispatcher_msg.data = BTN_LEFT;
					}
					else if (strcmp(argv[0], "btn_right") == 0)				// RIGHT button
					{
						dispatcher_msg.type = DP_EMU_BTN_DOWN;
						dispatcher_msg.data = BTN_RIGHT;
					}
					else if (strcmp(argv[0], "btn_on") == 0)				// ON button
					{
						dispatcher_msg.type = DP_EMU_BTN_DOWN;
						dispatcher_msg.data = BTN_ON;
					}
					else if (strcmp(argv[0], "btn_off") == 0)				// OFF button
					{
						dispatcher_msg.type = DP_EMU_BTN_DOWN;
						dispatcher_msg.data = BTN_OFF;
					}
					else if (strcmp(argv[0], "push_encoder") == 0)			// Encoder push
					{
						dispatcher_msg.type = DP_EMU_BTN_DOWN;
						dispatcher_msg.data = BTN_ENCODER;
					}
					else if (strcmp(argv[0], "encoder_delta") == 0)			// Encoder delta
					{
						if (argc < 2)
						{
							UART_sendStrAlloc(argv[0]);
							UART_sendStr(" ERR: missing argument [ticks]\r");
							cmd_ok = 0;
						}
						else
						{
							// Second argument is encoder ticks (signed)
							dispatcher_msg.type = DP_EMU_ENC_DELTA;
							dispatcher_msg.data = (uint32_t)strtol(argv[1], 0, 0);
						}
					}
					//----------------- misc -----------------//
					else if (strcmp(argv[0], "get_time_profiling") == 0)			// Time profiling
					{
						sprintf(temp_str,"Systick hook max ticks: %d\r",time_profile.max_ticks_in_Systick_hook);
						UART_sendStrAlloc(temp_str);
						sprintf(temp_str,"Timer2 ISR max ticks: %d\r",time_profile.max_ticks_in_Timer2_ISR);
						UART_sendStrAlloc(temp_str);
					}
					//------------ unknown command -----------//
					else
					{
						UART_sendStrAlloc(argv[0]);
						UART_sendStr(" ERR: unknown cmd\r");
						cmd_ok = 0;
					}
					
					//----------------------------------------//
					
					// Send result to dispatcher
					if (dispatcher_msg.type)
						xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
					
					// Confirm
					if (cmd_ok)
					{
						UART_sendStrAlloc(argv[0]);
						UART_sendStr(" OK\r");
					}
			
				}			
				msg_length = 0;
				argc = 0;
				search_for_word = 1;
			}
		}  // \UART_Get_from_RX_buffer()
		
	}
}



//=================================================================//
//=================================================================//
// Transmitter

static void UART_sendStr(char *str)
{
	uart_transmiter_msg_t msg;
	msg.type = SEND_STRING;
	msg.pdata = str;
	xQueueSendToBack(xQueueUART2TX, &msg, 0);
}

static void UART_sendStrAlloc(char *str)
{
	uart_transmiter_msg_t msg;
	uint32_t str_length = strlen(str);
	//taskENTER_CRITICAL
	msg.pdata = pvPortMalloc(str_length);		// heap_3 or heap_4 should be used
	//taskEXIT_CRITICAL
	if (msg.pdata == 0)
		return;
	strcpy(msg.pdata, str);
	msg.type = SEND_ALLOCATED_STRING;
	xQueueSendToBack(xQueueUART2TX, &msg, 0);
}



void vTaskUARTTransmitter(void *pvParameters) 
{
	uart_transmiter_msg_t income_msg;
	
	const char _resp_OK[] = "OK\r";
	const char _resp_BAD_CMD[] = "bad cmd\r";
	
	
	
	DMA_ChannelInitTypeDef DMA_InitStr;
	DMA_CtrlDataInitTypeDef DMA_PriCtrlStr;
	DMA_StructInit(&DMA_InitStr);
	
	// DMA channel UARTx RX configuration 
	// Using normal mode
	
	// Set Primary Control Data 
	//DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)tx_buffer;
	DMA_PriCtrlStr.DMA_DestBaseAddr = (uint32_t)(&(MDR_UARTn->DR));			
	DMA_PriCtrlStr.DMA_SourceIncSize = DMA_SourceIncByte;
	DMA_PriCtrlStr.DMA_DestIncSize = DMA_DestIncNo;
	DMA_PriCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_PriCtrlStr.DMA_Mode = DMA_Mode_Stop; 
	//DMA_PriCtrlStr.DMA_CycleSize = buffer_size;							
	DMA_PriCtrlStr.DMA_NumContinuous = DMA_Transfers_8;
	DMA_PriCtrlStr.DMA_SourceProtCtrl = DMA_SourcePrivileged;				// ?
	DMA_PriCtrlStr.DMA_DestProtCtrl = DMA_DestPrivileged;					// ?
	
	// Set Channel Structure
	DMA_InitStr.DMA_PriCtrlData = &DMA_PriCtrlStr;
	DMA_InitStr.DMA_Priority = DMA_Priority_High;
	DMA_InitStr.DMA_UseBurst = DMA_BurstClear;								// enable single words trasfer
	DMA_InitStr.DMA_SelectDataStructure = DMA_CTRL_DATA_PRIMARY;
	
	// Init DMA channel
	DMA_Init(DMA_Channel_UART_TX, &DMA_InitStr);
	DMA_PriCtrlStr.DMA_Mode = DMA_Mode_Basic;
	
	// Initialize
	xQueueUART2TX = xQueueCreate( 10, sizeof( uart_tx_incoming_msg_t ) );
	if( xQueueUART2TX == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	
	
	while(1)
	{
		xQueueReceive(xQueueDispatcher, &income_msg, portMAX_DELAY);
		if ( (income_msg.type == SEND_STRING) || (income_msg.type == SEND_ALLOCATED_STRING) )
		{
			// Task has to transmit a char string and free memory after trasnfer is complete
			DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)(&(income_msg.pdata));
			DMA_PriCtrlStr.DMA_CycleSize = strlen(income_msg.pdata) - 1;
		}
		else if (income_msg.type == RESPONSE_OK)
		{
			DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)(&(_resp_OK));
			DMA_PriCtrlStr.DMA_CycleSize = strlen(_resp_OK) - 1;
		}
		else if (income_msg.type == RESPONSE_BAD_CMD)
		{
			DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)(&(_resp_BAD_CMD));
			DMA_PriCtrlStr.DMA_CycleSize = strlen(_resp_BAD_CMD) - 1;
		}
		
		// Start DMA
		DMA_CtrlInit (DMA_Channel_UART_TX, DMA_CTRL_DATA_PRIMARY, &DMA_PriCtrlStr);
		DMA_Cmd(DMA_Channel_UART_TX, ENABLE);
			
		// Wait for DMA
		// TODO: use semaphore
		while((DMA_GetFlagStatus(DMA_Channel_UART_TX, DMA_FLAG_CHNL_ENA)));
	
		// Free memory
		if (income_msg.type == SEND_ALLOCATED_STRING)
		{
			//taskENTER_CRITICAL
			vPortFree(income_msg.string);		// heap_3 or heap_4 should be used
			//taskEXIT_CRITICAL
		}
	}
}










