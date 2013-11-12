
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "MDR32F9Qx_uart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "systick.h"
#include "stdint.h"
#include "ring_buffer.h"
#include "uart.h"
#include "dispatcher.h"
#include "buttons.h"

uint8_t test_mode = 1;

static ring_buffer_t uart2_tx_rbuf;
static ring_buffer_t uart2_rx_rbuf;
static char uart2_tx_data_buff[TX_RING_BUFFER_SIZE];
static char uart2_rx_data_buff[RX_RING_BUFFER_SIZE];


// Private functions
static void sendString2(char *pstr1, char *pstr2);


void UARTInit(void)
{
	initRingBuffer(&uart2_rx_rbuf, uart2_rx_data_buff, RX_RING_BUFFER_SIZE);
	initRingBuffer(&uart2_tx_rbuf, uart2_tx_data_buff, TX_RING_BUFFER_SIZE);
}



void vTaskUARTReceiver(void *pvParameters) 
{
	char received_msg[RX_MESSAGE_MAX_LENGTH];
	uint16_t msg_length = 0;
	char *argv[MAX_WORDS_IN_MESSAGE];		// Array of pointers to separate words
	uint16_t argc = 0;						// Count of words in message
	char temp_char;
	uint16_t search_for_word = 1;
	uint8_t cmd_ok;
	
	
	// Debug
	uint16_t i;
	uint32_t temp32u;
	
	char temp_str[50];
	
	portTickType lastExecutionTime = xTaskGetTickCount();
	
	dispatch_incoming_msg_t dispatcher_msg;
	
	
	while(1)
	{
		vTaskDelayUntil(&lastExecutionTime, 5);		// 10ms period

		// Read full message from buffer
		while(getFromRingBuffer(&uart2_rx_rbuf, &temp_char))
		{
			if (test_mode)
			{
				while(!putIntoRingBuffer(&uart2_tx_rbuf, temp_char));			
				continue;
			}
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
					//----- Debug -----//
			/*		for (i=0;i<argc;i++)
					{
						//printf("%s\r",argv[i]);	
						sendString2(argv[i], "\r");
					} */
					//-----------------//
					
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
							sendString2(argv[0], " ERR: missing argument [mV]\r");
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
							sendString2(argv[0], " ERR: missing argument [mA]\r");
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
							sendString2(argv[0], " ERR: missing argument (20/40)[A]\r");
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
							sendString2(argv[0], " ERR: missing argument [ticks]\r");
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
						sprintf(temp_str,"%d\r",time_profile.max_ticks_in_Systick_hook);
						sendString2("Systick hook max ticks: ", temp_str);
						sprintf(temp_str,"%d\r",time_profile.max_ticks_in_Timer2_ISR);
						sendString2("Timer2 ISR max ticks: ", temp_str);
					}
					//------------ unknown command -----------//
					else
					{
						sendString2(argv[0], " ERR: unknown cmd\r");
						cmd_ok = 0;
					}
					
					//----------------------------------------//
					
					// Send result to dispatcher
					if (dispatcher_msg.type)
						xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, 0);
					
					// Confirm
					if (cmd_ok)
						sendString2(argv[0], " OK\r");
			
				}			
				msg_length = 0;
				argc = 0;
				search_for_word = 1;
			}
		}  // \getFromRingBuffer()
		
	}
}



void processUartRX(void)
{
	uint16_t temp;
	while ( (UART_GetFlagStatus(MDR_UART2,UART_FLAG_RXFE) == RESET) && (!ringBufferIsFull(uart2_rx_rbuf)) )
	{
		temp = UART_ReceiveData(MDR_UART2);
		if ((temp & ( (1<<UART_Data_BE) | (1<<UART_Data_PE) | (1<<UART_Data_FE) )) == 0)
		{
			putIntoRingBuffer(&uart2_rx_rbuf, (char)temp);
		}
	} 
}


//=================================================================//
//=================================================================//
// Transmitter


// Simple function to send two 0-terminated strings
void sendString2(char *pstr1, char *pstr2)
{
	char temp_char;
	while(temp_char = *pstr1++)
	{
		while(!putIntoRingBuffer(&uart2_tx_rbuf, temp_char));			
	}	
	while(temp_char = *pstr2++)
	{
		while(!putIntoRingBuffer(&uart2_tx_rbuf, temp_char));			
	}	
}


void processUartTX(void)
{
	char temp;
	while ( (UART_GetFlagStatus(MDR_UART2,UART_FLAG_TXFF) == RESET) && (!ringBufferIsEmpty(uart2_tx_rbuf)) )
	{
		getFromRingBuffer(&uart2_tx_rbuf, &temp);
		UART_SendData(MDR_UART2,(uint16_t)temp);
	}
}









