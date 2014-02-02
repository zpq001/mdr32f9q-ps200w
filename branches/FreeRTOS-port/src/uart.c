
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "MDR32F9Qx_uart.h"
#include "MDR32F9Qx_dma.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "systick.h"
#include "stdint.h"
#include "uart.h"
#include "dispatcher.h"
#include "buttons.h"
#include "systemfunc.h"

#include "guiTop.h"

#include "dwt_delay.h"

extern DMA_CtrlDataTypeDef DMA_ControlTable[];

	const char _resp_OK[] = "OK\r";
	const char _resp_UNKN_CMD[] = "Unknown cmd\r";


// Test
uint8_t test_mode = 0;

#define USE_UART1
//#define USE_UART2

#ifdef USE_UART1
//#define MDR_UARTn MDR_UART1
//const uint8_t DMA_Channel_UART_RX = DMA_Channel_UART1_RX;
//const uint8_t DMA_Channel_UART_TX = DMA_Channel_UART1_TX;
#endif

#ifdef USE_UART2
//#define MDR_UARTn MDR_UART2
//const uint8_t DMA_Channel_UART_RX = DMA_Channel_UART2_RX;
//const uint8_t DMA_Channel_UART_TX = DMA_Channel_UART2_TX;
#endif


// Transmitter buffers
static char 					uart1_tx_data_buff[TX_BUFFER_SIZE];
// Receiver buffers
static uint16_t 				uart1_rx_data_buff[RX_BUFFER_SIZE];
static uart_dma_rx_buffer_t 	uart1_rx_dma_buffer;

xQueueHandle xQueueUART1TX;
xSemaphoreHandle xSemaphoreUART1TX;

static uint32_t UART1_RX_saved_TCB[3];


//static void UART_sendStr(char *str);
static void UART_sendStrAlloc(char *str);


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

uint8_t UART_Get_from_RX_buffer(uart_dma_rx_buffer_t *rx_buffer, uint16_t *rx_word)
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



// DMA channel UARTx TX configuration 
static void UART_init_TX_DMA(MDR_UART_TypeDef *MDR_UARTx, DMA_CtrlDataInitTypeDef *DMA_PriCtrlStr_p)
{
	uint32_t DMA_Channel_UARTn_TX = (MDR_UARTx == MDR_UART1) ? DMA_Channel_UART1_TX : DMA_Channel_UART2_TX;
	DMA_ChannelInitTypeDef DMA_InitStr;
	
	// Setup Primary Control Data 
	DMA_PriCtrlStr_p->DMA_SourceBaseAddr = 0;									// Will be set before channel start
	DMA_PriCtrlStr_p->DMA_DestBaseAddr = (uint32_t)(&(MDR_UARTx->DR));			
	DMA_PriCtrlStr_p->DMA_SourceIncSize = DMA_SourceIncByte;
	DMA_PriCtrlStr_p->DMA_DestIncSize = DMA_DestIncNo;
	DMA_PriCtrlStr_p->DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_PriCtrlStr_p->DMA_Mode = DMA_Mode_Basic; 
	DMA_PriCtrlStr_p->DMA_CycleSize = 0;										// Will be set before channel start
	DMA_PriCtrlStr_p->DMA_NumContinuous = DMA_Transfers_1;
	DMA_PriCtrlStr_p->DMA_SourceProtCtrl = DMA_SourcePrivileged;				// ?
	DMA_PriCtrlStr_p->DMA_DestProtCtrl = DMA_DestPrivileged;					// ?
	
	// Setup Channel Structure
	DMA_InitStr.DMA_PriCtrlData = 0;										// Will be set before channel start
	DMA_InitStr.DMA_AltCtrlData = 0;										// Not used
	DMA_InitStr.DMA_ProtCtrl = 0;											// Not used
	DMA_InitStr.DMA_Priority = DMA_Priority_High;
	DMA_InitStr.DMA_UseBurst = DMA_BurstClear;								// Enable single words trasfer
	DMA_InitStr.DMA_SelectDataStructure = DMA_CTRL_DATA_PRIMARY;
	
	// Init DMA channel
	my_DMA_ChannelInit(DMA_Channel_UARTn_TX, &DMA_InitStr);
}


uint16_t parseKeyCode(char *arg)
{
	if (strcmp(arg, "btn_esc") == 0)
		return BTN_ESC;
	else if (strcmp(arg, "btn_ok") == 0)
		return BTN_OK;
	else if (strcmp(arg, "btn_left") == 0)
		return BTN_LEFT;
	else if (strcmp(arg, "btn_right") == 0)
		return BTN_RIGHT;
	else if (strcmp(arg, "btn_encoder") == 0)
		return BTN_ENCODER;	
	else
		return 0;
}

uint16_t parseKeyType(char *arg)
{
	if (strcmp(arg, "down") == 0)
		return BTN_EVENT_DOWN;
	else if (strcmp(arg, "up") == 0)
		return BTN_EVENT_UP;
	else if (strcmp(arg, "up_short") == 0)
		return BTN_EVENT_UP_SHORT;
	else if (strcmp(arg, "up_long") == 0)
		return BTN_EVENT_UP_LONG;
	else if (strcmp(arg, "hold") == 0)
		return BTN_EVENT_HOLD;
	else if (strcmp(arg, "repeat") == 0)
		return BTN_EVENT_REPEAT;
	else
		return 0;
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
	
	uint16_t keyCmdType;
	uint16_t keyCmdCode;
	
	// Debug
//	uint16_t i;
//	uint32_t temp32u;
	
	char temp_str[50];
	
	portTickType lastExecutionTime = xTaskGetTickCount();
	dispatch_msg_t dispatcher_msg;
	uart_transmiter_msg_t transmitter_msg;
	
	// Setup and init receiver buffer
	UART_Init_RX_buffer(&uart1_rx_dma_buffer, uart1_rx_data_buff, RX_BUFFER_SIZE);
	
	// Setup DMA channel
	UART_init_RX_DMA(MDR_UART1, &uart1_rx_dma_buffer, UART1_RX_saved_TCB);
	// Enable DMA channel
	DMA_Cmd(DMA_Channel_UART1_RX, ENABLE);
	// Enable UARTn DMA Rx request
	UART_DMACmd(MDR_UART1,UART_DMA_RXE, ENABLE);
	
	
	//HW_NVIC_check();		// FIXME - debug
	
	
	while(1)
	{
		/////////////////////////
	/*	while(1)
		{
			vTaskDelayUntil(&lastExecutionTime, 5);
			
			transmitter_msg.type = UNKNOWN_CMD;
			xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0); 
			xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0); 
			xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0); 
			xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0); 
			xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0); 
		}
		*/
		/////////////////////////
		vTaskDelayUntil(&lastExecutionTime, 5);		// 10ms period

		// Read full message from buffer
		while(UART_Get_from_RX_buffer(&uart1_rx_dma_buffer, &uart_rx_word))
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
					dispatcher_msg.type = 0;
					transmitter_msg.type = RESPONSE_OK;
					
					
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
							transmitter_msg.type = SEND_STRING;
							transmitter_msg.pdata = "ERR: missing argument [mV]\r";
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
							transmitter_msg.type = SEND_STRING;
							transmitter_msg.pdata = "ERR: missing argument [mA]\r";
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
							transmitter_msg.type = SEND_STRING;
							transmitter_msg.pdata = "ERR: missing argument (20/40)[A]\r";
						}
						else
						{
							// Second argument is current limit value [A]
							dispatcher_msg.type = DP_CONVERTER_SET_CURRENT_LIMIT;
							dispatcher_msg.data = strtoul(argv[1], 0, 0);
						}
					}
					//----- button and encoder emulation -----//	
					else if (strcmp(argv[0], "key") == 0)				// KEY command
					{
						// There should be 2 parameters - key code specifier and key event specifier
						if (argc < 3)
						{
							transmitter_msg.type = SEND_STRING;
							transmitter_msg.pdata = "ERR: missing key command arguments\r";
						}
						else
						{
							keyCmdType = parseKeyType(argv[1]);
							if (keyCmdType == 0)
							{
								transmitter_msg.type = SEND_STRING;
								transmitter_msg.pdata = "ERR: unknown key type\r";
							}
							else
							{
								keyCmdCode = parseKeyCode(argv[2]);
								if (keyCmdCode == 0)
								{
									transmitter_msg.type = SEND_STRING;
									transmitter_msg.pdata = "ERR: unknown key code\r";
								}
								else
								{
									// Send parsed key command to dispatcher
									// msg.data[31:16] = key code, 	msg.data[15:0] = key event type
									dispatcher_msg.type = DISPATCHER_EMULATE_BUTTON;
									dispatcher_msg.data = ((uint32_t)keyCmdCode << 16) | keyCmdType;
								}
							}
						}
					}
					else if (strcmp(argv[0], "encoder_delta") == 0)			// Encoder delta
					{
						if (argc < 2)
						{
							transmitter_msg.type = SEND_STRING;
							transmitter_msg.pdata = "ERR: missing argument [ticks]\r";
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
						// FIXME
						sprintf(temp_str,"Systick hook max ticks: %d\r",time_profile.max_ticks_in_Systick_hook);
						UART_sendStrAlloc(temp_str);
						sprintf(temp_str,"Timer2 ISR max ticks: %d\r",time_profile.max_ticks_in_Timer2_ISR);
						UART_sendStrAlloc(temp_str);
					}
					//------------ unknown command -----------//
					else
					{
						transmitter_msg.type = UNKNOWN_CMD;
					}
					
					//----------------------------------------//
					
					// Send result to dispatcher
					if (dispatcher_msg.type)
						xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
					
					// Send response over UART
					xQueueSendToBack(xQueueUART1TX, &transmitter_msg, 0); 
			
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

// 1. Send a string from program memory (make a copy inside transmitter task)
// 2. Send a string from data memory (make a copy using malloc())
// 3. Send a string from data memory (constant)

/*
static void UART_sendStr(char *str)
{
	uart_transmiter_msg_t msg;
	msg.type = SEND_STRING;
	msg.pdata = str;
	xQueueSendToBack(xQueueUART1TX, &msg, 0); 
} */

static void UART_sendStrAlloc(char *str)
{
	uart_transmiter_msg_t msg;
	uint32_t str_length = strlen(str);
	msg.pdata = pvPortMalloc(str_length);		// heap_3 or heap_4 should be used
	if (msg.pdata == 0)
		return;
	strcpy(msg.pdata, str);
	msg.type = SEND_ALLOCATED_STRING;
	if (xQueueSendToBack(xQueueUART1TX, &msg, 0) == errQUEUE_FULL)
		vPortFree(msg.pdata);
}




void vTaskUARTTransmitter(void *pvParameters) 
{
	uart_transmiter_msg_t income_msg;
	//uint32_t src_address;
	char *string_to_send;
	uint32_t n;
	
	DMA_CtrlDataInitTypeDef DMA_PriCtrlStr;
	
	// Setup DMA channel
	UART_init_TX_DMA(MDR_UART1, &DMA_PriCtrlStr);
	
	// Initialize OS items
	xQueueUART1TX = xQueueCreate( 10, sizeof( uart_transmiter_msg_t ) );
	if( xQueueUART1TX == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	vSemaphoreCreateBinary( xSemaphoreUART1TX );
	if( xSemaphoreUART1TX == 0 )
    {
        while(1);
    }
	
	xSemaphoreTake(xSemaphoreUART1TX, 0);	
	
	while(1)
	{
		xQueueReceive(xQueueUART1TX, &income_msg, portMAX_DELAY);
		if ( (income_msg.type == SEND_STRING) || (income_msg.type == SEND_ALLOCATED_STRING) )
		{
			string_to_send = income_msg.pdata;
		}
		else if (income_msg.type == RESPONSE_OK)
		{
			string_to_send = (char *)_resp_OK;
		}
		else //if (income_msg.type == UNKNOWN_CMD)
		{
			string_to_send = (char *)_resp_UNKN_CMD;
		}
		
		// Get number of chars to transmit
		n = strlen(string_to_send);
		// DMA cannot read from program memory, copy data to temporary buffer
		if ((uint32_t)string_to_send < 0x20000000UL)
		{
			if (n > TX_BUFFER_SIZE)
				n = TX_BUFFER_SIZE;
			strncpy(uart1_tx_data_buff, string_to_send, n);
			DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)&uart1_tx_data_buff;
		}
		else
		{
			DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)string_to_send;
		}
		// Setup DMA control block
		DMA_PriCtrlStr.DMA_CycleSize = n;
		// Start DMA
		DMA_CtrlInit (DMA_Channel_UART1_TX, DMA_CTRL_DATA_PRIMARY, &DMA_PriCtrlStr);
		DMA_Cmd(DMA_Channel_UART1_TX, ENABLE);
		// Enable UART1 DMA Tx request
		UART_DMACmd(MDR_UART1,UART_DMA_TXE, ENABLE);
			
		// Wait for DMA
		xSemaphoreTake(xSemaphoreUART1TX, portMAX_DELAY);
	
		// Free memory if required
		if (income_msg.type == SEND_ALLOCATED_STRING)
		{
			vPortFree(income_msg.pdata);		// heap_3 or heap_4 should be used
		}
	}
}






void DMA_IRQHandler(void)
{
	uint32_t *tcb_ptr;
	portBASE_TYPE xHigherPriorityTaskWokenByPost = pdFALSE;
	
	if ((MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART1_TX)) == 0)	
	{
		// UART 1 DMA transfer complete
		if (MDR_UART1->DMACR & UART_DMACR_TXDMAE)
		{
			// Disable UART1 DMA Tx request
			MDR_UART1->DMACR &= ~UART_DMACR_TXDMAE;
			xSemaphoreGiveFromISR( xSemaphoreUART1TX, &xHigherPriorityTaskWokenByPost );
		}
	}
	/*
	if (MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART2_TX) == 0)	
	{
		// UART 2 DMA transfer complete
		// Disable UART2 DMA Tx request
		UART_DMACmd(MDR_UART2,UART_DMA_TXE, DISABLE);
		// postSemaphoreFromISR(__complete__);
	}	
	*/
	
	if ((MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART1_RX)) == 0)	
	{
		// Reload TCB
		tcb_ptr = (uint32_t*)&DMA_ControlTable[DMA_Channel_UART1_RX];
		*tcb_ptr++ = UART1_RX_saved_TCB[0];
		*tcb_ptr++ = UART1_RX_saved_TCB[1];
		*tcb_ptr = UART1_RX_saved_TCB[2];
		MDR_DMA->CHNL_ENABLE_SET = (1 << DMA_Channel_UART1_RX);
	}
		
	
	// Error handling
	if (DMA_GetFlagStatus(0, DMA_FLAG_DMA_ERR) == SET)
	{
		DMA_ClearError();	// normally this should not happen
	}
	
	// Force context switching if required
	//portEND_SWITCHING_ISR(xHigherPriorityTaskWokenByPost);
}





