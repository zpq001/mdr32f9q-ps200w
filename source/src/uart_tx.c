
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
#include "dispatcher.h"
#include "buttons.h"
#include "systemfunc.h"
#include "eeprom.h"
#include "uart_hw.h"
#include "uart_tx.h"
#include "uart_rx.h"
#include "guiTop.h"		// button codes




	
extern uint32_t UART1_RX_saved_TCB[];
extern uint32_t UART2_RX_saved_TCB[];


// Transmitter buffers
static char 	uart1_tx_data_buff[TX_BUFFER_SIZE];
static char 	uart2_tx_data_buff[TX_BUFFER_SIZE];

xQueueHandle xQueueUART1TX;
xQueueHandle xQueueUART2TX;

static xSemaphoreHandle xSemaphoreUART1TX;
static xSemaphoreHandle xSemaphoreUART2TX;


static const uart_string_table_record_t uart_tx_table[] = {
	{ UART_RESPONSE_OK,					"OK\n"},
	{ UART_RESPONSE_UNKNOWN_CMD,		"Unknown command\n"},
	{ UART_RESPONSE_WRONG_ARGUMENT,		"Wrong argument(s)\n"},
	{ UART_RESPONSE_MISSING_ARGUMENT,	"Missing argument(s)\n"},
	{ UART_RESPONSE_CMD_ERROR,			"Command processing error\n"},
	{ 0,								"\n"}
};


static char *UART_get_predefined_string(uint8_t code)
{
	const uart_string_table_record_t *record;
	uint8_t i = 0;
	do
		record = &uart_tx_table[i++];
	while ((record->code != 0) && (record->code != code));
	return (char *)(record->text);
}



// void UART_sendStr(char *str);
// void UART_sendStrAlloc(char *str);


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
/*
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
*/




//-------------------------------------------------------//
// Callbacks from ISR

static uint8_t UART_TX_Done(uint8_t uart_num)
{
	portBASE_TYPE xHPTaskWoken = pdFALSE;
	if (uart_num == 1)
		xSemaphoreGiveFromISR( xSemaphoreUART1TX, &xHPTaskWoken );
	else
		xSemaphoreGiveFromISR( xSemaphoreUART2TX, &xHPTaskWoken );

	return (uint8_t)xHPTaskWoken;
}



static void UART_do_tx_DMA(uint8_t uart_num, char *string, uint16_t length)
{
	char *uart_tx_data_buffer = (uart_num == 1) ? uart1_tx_data_buff : uart2_tx_data_buff;
	xSemaphoreHandle *xSemaphoreUARTx = (uart_num == 1) ? &xSemaphoreUART1TX : &xSemaphoreUART2TX;
	uint32_t dma_address;
	uint16_t dma_count;
	
	while(length)
	{
		if ((uint32_t)string < 0x20000000UL)
		{
			// DMA cannot read from program memory, copy data to temporary buffer and send from it
			dma_count = (length > TX_BUFFER_SIZE) ? TX_BUFFER_SIZE : length;
			strncpy(uart_tx_data_buffer, string, dma_count);
			length -= dma_count;
			string += dma_count;
			dma_address = (uint32_t)uart_tx_data_buffer;
		}
		else
		{
			dma_address = (uint32_t)string;
			dma_count = length;
			length = 0;
		}
		
		UART_Send_Data(uart_num, (char *)dma_address, dma_count);

		// Wait for DMA
		xSemaphoreTake(*xSemaphoreUARTx, portMAX_DELAY);
	}
}





void vTaskUARTTransmitter(void *pvParameters) 
{
	// Uart number for task is passed by argument
	uint8_t my_uart_num = (uint32_t)pvParameters;
	xQueueHandle *xQueueUARTx = (my_uart_num == 1) ? &xQueueUART1TX : &xQueueUART2TX;
	xSemaphoreHandle *xSemaphoreUARTx = (my_uart_num == 1) ? &xSemaphoreUART1TX : &xSemaphoreUART2TX;
//	MDR_UART_TypeDef *MDR_UARTx = (my_uart_num == 1) ? MDR_UART1 : MDR_UART2;
	char *uart_tx_data_buffer = (my_uart_num == 1) ? uart1_tx_data_buff : uart2_tx_data_buff;
	uart_transmiter_msg_t msg;
	xSemaphoreHandle *hwUART_mutex = (my_uart_num == 1) ? &hwUART1_mutex : &hwUART2_mutex;
	
	char *string_to_send;
	uint16_t length;
	
	// UART and DMA initialization is performed by UART RX task, which is the main UART control task.
	// Here we should only setup TX done callbacks
	UART_Register_TX_Done_Callback(my_uart_num, UART_TX_Done);
	
	// Initialize OS items
	*xQueueUARTx = xQueueCreate( 10, sizeof( uart_transmiter_msg_t ) );
	if( *xQueueUARTx == 0 )
	{
		while(1);		// Queue was not created and must not be used.
	}
	// Semaphore is posted in DMA callback called from ISR to indicate DMA transfer finish
	vSemaphoreCreateBinary( *xSemaphoreUARTx );
	if( *xSemaphoreUARTx == 0 )
    {
        while(1);
    }
	xSemaphoreTake(*xSemaphoreUARTx, 0);	
	
	while(1)
	{
		xQueueReceive(*xQueueUARTx, &msg, portMAX_DELAY);
		string_to_send = 0;
		switch (msg.type)
		{
			case UART_SEND_STRING:
			case UART_SEND_ALLOCATED_STRING:
				string_to_send = msg.data;
				length = strlen(string_to_send);
				break;
			case UART_SEND_DATA:
			case UART_SEND_ALLOCATED_DATA:
				string_to_send = msg.data;
				length = msg.length;
				break;
			case UART_SEND_PROFILING:
				string_to_send = uart_tx_data_buffer;
				length = 0;
				sprintf(string_to_send,"Systick hook max ticks: %d\r",time_profile.max_ticks_in_Systick_hook);
				UART_do_tx_DMA(my_uart_num, string_to_send, strlen(string_to_send));
				sprintf(string_to_send,"Timer2 ISR max ticks: %d\r",time_profile.max_ticks_in_Timer2_ISR);
				UART_do_tx_DMA(my_uart_num, string_to_send, strlen(string_to_send));
				break;
			case UART_SEND_POWER_CYCLES_STAT:
				string_to_send = uart_tx_data_buffer;
				length = 0;
				sprintf(string_to_send,"Power on/off cycles: %d\r", global_settings->number_of_power_cycles);
				UART_do_tx_DMA(my_uart_num, string_to_send, strlen(string_to_send));
				break;
			default:
				// Get string from predefined response table
				string_to_send = UART_get_predefined_string(msg.type);
				length = strlen(string_to_send);
				break;
		}
		
		if ((string_to_send != 0) && (length != 0))
		{
			if ((*hwUART_mutex != 0) && (xSemaphoreTake(*hwUART_mutex, 0) == pdTRUE))
			{
				// Transfer data
				UART_do_tx_DMA(my_uart_num, string_to_send, strlen(string_to_send));
				xSemaphoreGive(*hwUART_mutex);
			}
		}

		// Free memory if required
		if ((msg.type == UART_SEND_ALLOCATED_STRING) || (msg.type == UART_SEND_ALLOCATED_DATA))
		{
			vPortFree(msg.data);		// heap_3 or heap_4 should be used
		}
	}
}













