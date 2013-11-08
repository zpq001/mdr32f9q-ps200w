
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "stdint.h"
#include "ring_buffer.h"
#include "uart.h"


static ring_buffer_t uart1_tx_rbuf;
static ring_buffer_t uart1_rx_rbuf;
static char uart1_tx_data_buff[TX_RING_BUFFER_SIZE];
static char uart1_rx_data_buff[RX_RING_BUFFER_SIZE];


static char test_str1[] = "  abcd ef   ghijklmno p q r\r eee \rggg\r12345\ron\roff\r set_voltage 7500\r ";
static char test_str2[] = "\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r";
static char test_str3[] = "a1a2a3a4a5a6a7a8a9a0b1b2b3b4b5b6b7b8b9b0c1c2c3c4c5c6c7c8c9c0d1d2d3d4d5d6d7d8d9d0\r";

// Private functions
static void sendString2(char *pstr1, char *pstr2);


void UARTInit(void)
{
	initRingBuffer(&uart1_rx_rbuf, uart1_rx_data_buff, RX_RING_BUFFER_SIZE);
	initRingBuffer(&uart1_tx_rbuf, uart1_tx_data_buff, TX_RING_BUFFER_SIZE);
}



void vTaskUARTReceiver(void *pvParameters) 
{
	char received_msg[RX_MESSAGE_MAX_LENGTH];
	uint16_t msg_length = 0;
	char *argv[MAX_WORDS_IN_MESSAGE];		// Array of pointers to separate words
	uint16_t argc = 0;								// Count of words in message
	char temp_char;
	uint16_t search_for_word = 1;
	
	// Debug
	uint16_t i;
	uint32_t temp32u;
	
///	portTickType lastExecutionTime = xTaskGetTickCount();
	
	
	
	
	while(1)
	{
///		vTaskDelayUntil(&lastExecutionTime, 5);		// 10ms period

		// Read full message from buffer
		while(getFromRingBuffer(&uart1_rx_rbuf, &temp_char))
		{
			
			if (temp_char == SPACING_SYMBOL)
			{
				received_msg[msg_length++] = '\0';
				search_for_word = 1;
			}
			else if (temp_char == MESSAGE_END_SYMBOL)
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
			
			
			if ((temp_char == MESSAGE_END_SYMBOL) || (msg_length == RX_MESSAGE_MAX_LENGTH))
			{
				// Received full message OR maximum allowed message length is reached
				//-------------------------//
				// Parse message
				if (argc != 0)
				{
					//----- Debug -----//
					for (i=0;i<argc;i++)
					{
						//printf("%s\r",argv[i]);	
						sendString2(argv[i], "\r");
					}
					//-----------------//
					
					if (strcmp(argv[0], "on") == 0)
					{
						// Turn converter ON	
						// send message to converter task - TODO
						sendString2(argv[0], " OK\r");
					}
					else if (strcmp(argv[0], "off") == 0)
					{
						// Turn converter OFF	
						// send message to converter task - TODO
						sendString2(argv[0], " OK\r");
					}
					else if (strcmp(argv[0], "set_voltage") == 0)
					{
						// Setting converter voltage
						if (argc < 2)
						{
							sendString2(argv[0], " ERR: missing argument [mV]\r");
						}
						else
						{
							// Second argument is voltage value [mV]
							temp32u = strtoul(argv[1], 0, 0);
							// send message to converter task - TODO
							sendString2(argv[0], " OK\r");
						}
					}
			
				}			
				//-------------------------//
				msg_length = 0;
				argc = 0;
				search_for_word = 1;
			}
		}
	}


}



void processUartRX(void)
{
	uint16_t temp;
/*	while ( (__UART_RX_FIFO_IS_NOT_EMPTY__) && (!ringBufferIsFull(uart1_rx_rbuf)) )
	{
		temp = __UART_READ_FIFO_ITEM__;
		if (__IS_VALID__(temp)
		{
			putIntoRingBuffer(&uart1_rx_rbuf, (char)temp);
		}
	} */
	
	// Test
	static char *pstr = test_str1;
	//while ( (*pstr != '\0') && (!ringBufferIsFull(uart1_rx_rbuf)) )
	if ( (*pstr != '\0') && (!ringBufferIsFull(uart1_rx_rbuf)) )
	{
		temp = *pstr++;
		putIntoRingBuffer(&uart1_rx_rbuf, (char)temp);
		
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
		while(!putIntoRingBuffer(&uart1_tx_rbuf, temp_char));			
	}	
	while(temp_char = *pstr2++)
	{
		while(!putIntoRingBuffer(&uart1_tx_rbuf, temp_char));			
	}	
}


void processUartTX(void)
{
	char temp;
/*	while ( (__UART_TX_FIFO_IS_NOT_FULL__) && (!ringBufferIsEmpty(uart1_tx_rbuf)) )
	{
		temp = getFromRingBuffer(&uart1_tx_rbuf, &temp))
		__UART_WRITE_FIFO_ITEM__(temp);
	}
*/

	// Test
	while (!ringBufferIsEmpty(uart1_tx_rbuf))
	{
		getFromRingBuffer(&uart1_tx_rbuf, &temp);
		printf("%c",temp);
	}
}









