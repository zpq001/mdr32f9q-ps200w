
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
#include "systemfunc.h"
#include "eeprom.h"
#include "uart_hw.h"
#include "uart_tx.h"
#include "uart_rx.h"


// OS stuff
xQueueHandle xQueueUART1TX;
xQueueHandle xQueueUART2TX;
static xSemaphoreHandle xSemaphoreUART1TX;
static xSemaphoreHandle xSemaphoreUART2TX;


// Transmitter tasks context
typedef struct {
	const uint8_t uart_num;
	xQueueHandle *xQueue;
	xSemaphoreHandle *hwUART_mutex;			// used for sync with RX task
	xSemaphoreHandle *xSemaphoreTx;			// used as sync with TX DMA done ISR
	char tx_data_buffer[TX_BUFFER_SIZE];	// Transmitter buffer
} UART_TX_Task_Context_t;



static UART_TX_Task_Context_t UART1_TX_Task_Context = {
	1,				// uart number
	&xQueueUART1TX,
	&hwUART1_mutex,
	&xSemaphoreUART1TX,
	{0}
};

static UART_TX_Task_Context_t UART2_TX_Task_Context = {
	2,				// uart number
	&xQueueUART2TX,
	&hwUART2_mutex,
	&xSemaphoreUART2TX,
	{0}
};


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
// Callback from ISR

static uint8_t UART_TX_Done(uint8_t uart_num)
{
	portBASE_TYPE xHPTaskWoken = pdFALSE;
	UART_TX_Task_Context_t *ctx = (uart_num == 1) ? &UART1_TX_Task_Context : &UART2_TX_Task_Context;
	xSemaphoreGiveFromISR( *ctx->xSemaphoreTx, &xHPTaskWoken );
	return (uint8_t)xHPTaskWoken;
}



static void UART_do_tx_DMA(UART_TX_Task_Context_t *ctx, char *string, uint16_t length)
{
	uint32_t dma_address;
	uint16_t dma_count;
	
	while(length)
	{
		if ((uint32_t)string < 0x20000000UL)
		{
			// DMA cannot read from program memory, copy data to temporary buffer and send from it
			dma_count = (length > TX_BUFFER_SIZE) ? TX_BUFFER_SIZE : length;
			strncpy(ctx->tx_data_buffer, string, dma_count);
			length -= dma_count;
			string += dma_count;
			dma_address = (uint32_t)ctx->tx_data_buffer;
		}
		else
		{
			dma_address = (uint32_t)string;
			dma_count = length;
			length = 0;
		}
		
		UART_Send_Data(ctx->uart_num, (char *)dma_address, dma_count);

		// Wait for DMA
		xSemaphoreTake(*ctx->xSemaphoreTx, portMAX_DELAY);
	}
}





void vTaskUARTTransmitter(void *pvParameters) 
{
	// Uart number for task is passed by argument
	UART_TX_Task_Context_t *ctx = ((uint32_t)pvParameters == 1) ? &UART1_TX_Task_Context : &UART2_TX_Task_Context;
	uart_transmiter_msg_t msg;
	char *string_to_send;
	char *strArr[5];
	uint16_t length;
	uint8_t temp8u[4];
	uint16_t temp16u[4];
	uint32_t temp32u[4];
	
	
	// UART and DMA initialization is performed by UART RX task, which is the main UART control task.
	// Here we should only setup TX done callbacks
	UART_Register_TX_Done_Callback(ctx->uart_num, UART_TX_Done);
	
	// Initialize OS items
	*ctx->xQueue = xQueueCreate( 10, sizeof( uart_transmiter_msg_t ) );
	if( *ctx->xQueue == 0 )
	{
		while(1);		// Queue was not created and must not be used.
	}
	// Semaphore is posted in DMA callback called from ISR to indicate DMA transfer finish
	vSemaphoreCreateBinary( *ctx->xSemaphoreTx );
	if( *ctx->xSemaphoreTx == 0 )
    {
        while(1);
    }
	xSemaphoreTake(*ctx->xSemaphoreTx, 0);	
	
	while(1)
	{
		xQueueReceive(*ctx->xQueue, &msg, portMAX_DELAY);
		string_to_send = 0;
		switch (msg.type)
		{
			case UART_SEND_STRING:
			case UART_SEND_ALLOCATED_STRING:
				string_to_send = msg.char_array.data;
				length = strlen(string_to_send);
				break;
			case UART_SEND_DATA:
			case UART_SEND_ALLOCATED_DATA:
				string_to_send = msg.char_array.data;
				length = msg.char_array.length;
				break;
			case UART_SEND_CONVERTER_DATA:
				string_to_send = ctx->tx_data_buffer;
				if (msg.converter.mtype == SEND_MEASURED_VA)
				{
					taskENTER_CRITICAL();
					temp16u[0] = ADC_GetVoltage();
					temp16u[1] = ADC_GetCurrent();
					temp32u[0] = ADC_GetPower();
					taskEXIT_CRITICAL();
					sprintf(string_to_send, "-vm %d -cm %d -pm %d\r", temp16u[0], temp16u[1], temp32u[0]);
				}
				else if (msg.converter.mtype == SEND_VSET)
				{
					temp16u[0] = Converter_GetVoltageSetting(msg.converter.channel);
					strArr[0] = (msg.converter.channel == CHANNEL_5V) ? "-ch5v" : "-ch12v";
					sprintf(string_to_send, "-vset %s %d\r", strArr[0], temp16u[0]);
				}
				else if (msg.converter.mtype == SEND_VSET_LIM)
				{
					taskENTER_CRITICAL();
					temp16u[0] = Converter_GetVoltageSetting(msg.converter.channel);
					temp16u[1] = Converter_GetVoltageLimitSetting(msg.converter.channel, LIMIT_TYPE_LOW);
					temp16u[2] = Converter_GetVoltageLimitSetting(msg.converter.channel, LIMIT_TYPE_HIGH);
					temp8u[0] = Converter_GetVoltageLimitState(msg.converter.channel, LIMIT_TYPE_LOW);
					temp8u[1] = Converter_GetVoltageLimitState(msg.converter.channel, LIMIT_TYPE_HIGH);
					taskEXIT_CRITICAL();
					strArr[0] = (msg.converter.channel == CHANNEL_5V) ? "-ch5v" : "-ch12v";
					strArr[1] = (temp8u[0]) ? "-vminen" : "-vmindis";
					strArr[2] = (temp8u[1]) ? "-vmaxen" : "-vmaxdis";
					sprintf(string_to_send, "-vset %s %d -vmin %d %s -vmax %d %s\r", strArr[0], temp16u[0], temp16u[1], strArr[1], temp16u[2], strArr[1]);
				}
				if (msg.converter.mtype == CONV_TURNED_ON)
				{
					sprintf(string_to_send, "Converter turned ON\r");
				}
				else if (msg.converter.mtype == CONV_TURNED_OFF)
				{
					sprintf(string_to_send, "Converter turned OFF\r");
				}
				length = strlen(string_to_send);
				
			case UART_SEND_PROFILING:
				string_to_send = ctx->tx_data_buffer;
				length = 0;
				sprintf(string_to_send,"Systick hook max ticks: %d\r",time_profile.max_ticks_in_Systick_hook);
				UART_do_tx_DMA(ctx, string_to_send, strlen(string_to_send));
				sprintf(string_to_send,"Timer2 ISR max ticks: %d\r",time_profile.max_ticks_in_Timer2_ISR);
				UART_do_tx_DMA(ctx, string_to_send, strlen(string_to_send));
				break;
			case UART_SEND_POWER_CYCLES_STAT:
				string_to_send = ctx->tx_data_buffer;
				length = 0;
				sprintf(string_to_send,"Power on/off cycles: %d\r", global_settings->number_of_power_cycles);
				UART_do_tx_DMA(ctx, string_to_send, strlen(string_to_send));
				break;
			default:
				// Get string from predefined response table
				string_to_send = UART_get_predefined_string(msg.type);
				length = strlen(string_to_send);
				break;
		}
		
		if ((string_to_send != 0) && (length != 0))
		{
			// Check if UART is enabled - the mutex is controlled by RX task
			if ((*ctx->hwUART_mutex != 0) && (xSemaphoreTake(*ctx->hwUART_mutex, 0) == pdTRUE))
			{
				// Transfer data
				UART_do_tx_DMA(ctx, string_to_send, strlen(string_to_send));
				xSemaphoreGive(*ctx->hwUART_mutex);
			}
		}

		// Free memory if required
		if ((msg.type == UART_SEND_ALLOCATED_STRING) || (msg.type == UART_SEND_ALLOCATED_DATA))
		{
			vPortFree(msg.data);		// heap_3 or heap_4 should be used
		}
	}
}













