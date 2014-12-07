
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
#include "uart_proto.h"
#include "adc.h"
#include "converter.h"
#include "global_def.h"

//-------------------------------------------------------//
// OS stuff
xQueueHandle xQueueUART1TX;
xQueueHandle xQueueUART2TX;
static xSemaphoreHandle xSemaphoreUART1TX;
static xSemaphoreHandle xSemaphoreUART2TX;


//-------------------------------------------------------//
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


//-------------------------------------------------------//
// ISR callback
static uint8_t UART_TX_Done(uint8_t uart_num) {
	portBASE_TYPE xHPTaskWoken = pdFALSE;
	UART_TX_Task_Context_t *ctx = (uart_num == 1) ? &UART1_TX_Task_Context : &UART2_TX_Task_Context;
	xSemaphoreGiveFromISR( *ctx->xSemaphoreTx, &xHPTaskWoken );
	return (uint8_t)xHPTaskWoken;
}



static void UART_do_tx_DMA(UART_TX_Task_Context_t *ctx, char *string, uint16_t length) {
	uint32_t dma_address;
	uint16_t dma_count;
	while(length) {
		if ((uint32_t)string < 0x20000000UL) {
			// DMA cannot read from program memory, copy data to temporary buffer and send from it
			dma_count = (length > TX_BUFFER_SIZE) ? TX_BUFFER_SIZE : length;
			strncpy(ctx->tx_data_buffer, string, dma_count);
			length -= dma_count;
			string += dma_count;
			dma_address = (uint32_t)ctx->tx_data_buffer;
		} else {
			dma_address = (uint32_t)string;
			dma_count = length;
			length = 0;
		}
		UART_Send_Data(ctx->uart_num, (char *)dma_address, dma_count);
		// Wait for DMA
		xSemaphoreTake(*ctx->xSemaphoreTx, portMAX_DELAY);
	}
}




void vTaskUARTTransmitter(void *pvParameters) {
	// Uart number for task is passed by argument
	UART_TX_Task_Context_t *ctx = ((uint32_t)pvParameters == 1) ? &UART1_TX_Task_Context : &UART2_TX_Task_Context;
	uart_transmiter_msg_t msg;
	char *answ;
	const char *tempStr;
	uint16_t length;
//	uint8_t temp8u[4];
	uint16_t temp16u[4];
	uint32_t temp32u[4];
//	uint8_t i;
	uint8_t freeMem;
	
	// Initialize OS items
	*ctx->xQueue = xQueueCreate( 10, sizeof( uart_transmiter_msg_t ) );
	if( *ctx->xQueue == 0 )
		while(1);

	// Semaphore is posted in DMA callback called from ISR to indicate DMA transfer finish
	vSemaphoreCreateBinary( *ctx->xSemaphoreTx );
	if( *ctx->xSemaphoreTx == 0 )
        while(1);
	xSemaphoreTake(*ctx->xSemaphoreTx, 0);	
	
	// UART and DMA initialization is performed by UART RX task, which is the main UART control task.
	// Here we should only setup TX callbacks
	UART_Register_TX_Done_Callback(ctx->uart_num, UART_TX_Done);
	
	while(1) {
		xQueueReceive(*ctx->xQueue, &msg, portMAX_DELAY);
		answ = 0;
		length = 0;
		freeMem = 0;
		switch (msg.type) {
			case UART_SEND_DATA:
				answ = msg.data.data;
				length = (msg.data.lengthSpecified) ? msg.data.length : strlen(msg.data.data);
				freeMem = msg.data.freeMem;
				break;
			case UART_SEND_ERROR:
				answ = ctx->tx_data_buffer;
				startString(answ, proto.message_types.error);
				switch (msg.error.code) {
					case ERROR_CANNOT_DETERMINE_GROUP:	tempStr = proto.errors.no_group;	break;
					case ERROR_MISSING_KEY:		tempStr = proto.errors.missing_key;			break;
					case ERROR_MISSING_VALUE:	tempStr = proto.errors.missing_value;		break;
					case ERROR_ILLEGAL_VALUE:	tempStr = proto.errors.illegal_value;		break;
					case ERROR_PARAM_READONLY:	tempStr = proto.errors.illegal_value;		break;
					default:					tempStr = proto.errors.param_readonly;		break;
				}
				appendToStringLast(answ, tempStr);
				length = strlen(answ);
				break;
			case UART_SEND_CONVERTER_DATA:
				answ = ctx->tx_data_buffer;
				startString(answ, (msg.spec == UART_MSG_INFO) ? proto.message_types.info : proto.message_types.ack);
				appendToString(answ, proto.groups.converter);
				switch (msg.converter.param) {
					case param_MEASURED_DATA:
						taskENTER_CRITICAL();
						temp16u[0] = ADC_GetVoltage();
						temp16u[1] = ADC_GetCurrent();
						temp32u[0] = ADC_GetPower();
						taskEXIT_CRITICAL();
						appendToString(answ, proto.parameters.measured);
						appendToStringU32(answ, proto.keys.vmea, temp16u[0]);
						appendToStringU32(answ, proto.keys.cmea, temp16u[1]);
						appendToStringU32(answ, proto.keys.pmea, temp32u[0]);
						break;
					case param_STATE:
						temp16u[0] = Converter_GetState();
						appendToString(answ, proto.parameters.state);
						appendToString(answ, proto.keys.state);
						appendToString(answ, (temp16u[0] == CONVERTER_ON) ? proto.values.on : proto.values.off);
						break;
					case param_CHANNEL:
						temp16u[0] = Converter_GetFeedbackChannel();
						appendToString(answ, proto.parameters.channel);
						appendToString(answ, (temp16u[0] == CHANNEL_5V) ? proto.flags.ch5v : proto.flags.ch12v);
						break;
					case param_CRANGE:
						temp16u[0] = Converter_GetCurrentRange(msg.converter.channel);
						appendToString(answ, proto.parameters.crange);
						appendToString(answ, (msg.converter.channel == CHANNEL_5V) ? proto.flags.ch5v : proto.flags.ch12v);
						appendToString(answ, (temp16u[0] == CURRENT_RANGE_LOW) ? proto.flags.crangeLow : proto.flags.crangeHigh);
						break;
					case param_VSET:				
						temp16u[0] = Converter_GetVoltageSetting(msg.converter.channel);
						appendToString(answ, proto.parameters.vset);
						appendToString(answ, (msg.converter.channel == CHANNEL_5V) ? proto.flags.ch5v : proto.flags.ch12v);
						appendToStringU32(answ, proto.keys.vset, temp16u[0]);
						break;
					case param_CSET:
						temp16u[0] = Converter_GetCurrentSetting(msg.converter.channel, msg.converter.current_range);
						appendToString(answ, proto.parameters.cset);
						appendToString(answ, (msg.converter.channel == CHANNEL_5V) ? proto.flags.ch5v : proto.flags.ch12v);
						appendToString(answ, (msg.converter.current_range == CURRENT_RANGE_LOW) ? proto.flags.crangeLow : proto.flags.crangeHigh);
						appendToStringU32(answ, proto.keys.cset, temp16u[0]);
						break;
				}
				terminateString(answ);
				length = strlen(answ);
				break;
			case UART_SEND_PROFILING:
				answ = ctx->tx_data_buffer;
				length = 0;
				sprintf(answ,"Systick hook max ticks: %d\r",time_profile.max_ticks_in_Systick_hook);
				UART_do_tx_DMA(ctx, answ, strlen(answ));
				sprintf(answ,"Timer2 ISR max ticks: %d\r",time_profile.max_ticks_in_Timer2_ISR);
				UART_do_tx_DMA(ctx, answ, strlen(answ));
				break;
			case UART_SEND_POWER_CYCLES_STAT:
				answ = ctx->tx_data_buffer;
				length = 0;
				sprintf(answ,"Power on/off cycles: %d\r", global_settings->number_of_power_cycles);
				UART_do_tx_DMA(ctx, answ, strlen(answ));
				break;
			default:
				break;
		}
		// Transfer data
		if ((answ != 0) && (length != 0)) {
			// Check if UART is enabled - the mutex is controlled by RX task
			if ((*ctx->hwUART_mutex != 0) && (xSemaphoreTake(*ctx->hwUART_mutex, 0) == pdTRUE)) {
				// Transfer data
				UART_do_tx_DMA(ctx, answ, length);
				xSemaphoreGive(*ctx->hwUART_mutex);
			}
		}
		// Free memory if required
		if (freeMem) {
			vPortFree(answ);		// heap_3 or heap_4 should be used
		}
	}
}













