
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

#include "uart_tx.h"
#include "uart_rx.h"

#include "guiTop.h"		// button codes

#include "uart_parser.h"



extern DMA_CtrlDataTypeDef DMA_ControlTable[];
	
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




static void UART_do_tx_DMA(MDR_UART_TypeDef *MDR_UARTx, DMA_CtrlDataInitTypeDef *DMA_PriCtrlStr_p, char *string, uint16_t length)
{
	uint32_t DMA_Channel_UARTn_TX = (MDR_UARTx == MDR_UART1) ? DMA_Channel_UART1_TX : DMA_Channel_UART2_TX;
	char *uart_tx_data_buffer = (MDR_UARTx == MDR_UART1) ? uart1_tx_data_buff : uart2_tx_data_buff;
	xSemaphoreHandle *xSemaphoreUARTx = (MDR_UARTx == MDR_UART1) ? &xSemaphoreUART1TX : &xSemaphoreUART2TX;
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
	
		DMA_PriCtrlStr_p->DMA_SourceBaseAddr = dma_address;
		DMA_PriCtrlStr_p->DMA_CycleSize = dma_count;
		
		// Start DMA
		DMA_CtrlInit (DMA_Channel_UARTn_TX, DMA_CTRL_DATA_PRIMARY, DMA_PriCtrlStr_p);
		DMA_Cmd(DMA_Channel_UARTn_TX, ENABLE);
		// Enable UART1 DMA Tx request
		UART_DMACmd(MDR_UARTx,UART_DMA_TXE, ENABLE);
	
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
	MDR_UART_TypeDef *MDR_UARTx = (my_uart_num == 1) ? MDR_UART1 : MDR_UART2;
	char *uart_tx_data_buffer = (my_uart_num == 1) ? uart1_tx_data_buff : uart2_tx_data_buff;
	DMA_CtrlDataInitTypeDef DMA_PriCtrlStr;
	uart_transmiter_msg_t msg;
	xSemaphoreHandle *hwUART_mutex = (my_uart_num == 1) ? &hwUART1_mutex : &hwUART2_mutex;
	
	char *string_to_send;
	uint16_t length;
	
	// Setup DMA channel
	UART_init_TX_DMA(MDR_UARTx, &DMA_PriCtrlStr);
	
	// Initialize OS items
	*xQueueUARTx = xQueueCreate( 10, sizeof( uart_transmiter_msg_t ) );
	if( *xQueueUARTx == 0 )
	{
		while(1);		// Queue was not created and must not be used.
	}
	// Semaphore is posted in DMA ISR to indicate DMA transfer finish
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
				UART_do_tx_DMA(MDR_UARTx, &DMA_PriCtrlStr, string_to_send, strlen(string_to_send));
				sprintf(string_to_send,"Timer2 ISR max ticks: %d\r",time_profile.max_ticks_in_Timer2_ISR);
				UART_do_tx_DMA(MDR_UARTx, &DMA_PriCtrlStr, string_to_send, strlen(string_to_send));
				break;
			case UART_SEND_POWER_CYCLES_STAT:
				string_to_send = uart_tx_data_buffer;
				length = 0;
				sprintf(string_to_send,"Power on/off cycles: %d\r", global_settings->number_of_power_cycles);
				UART_do_tx_DMA(MDR_UARTx, &DMA_PriCtrlStr, string_to_send, strlen(string_to_send));
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
				UART_do_tx_DMA(MDR_UARTx, &DMA_PriCtrlStr, string_to_send, length);
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
	
	if ((MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART2_TX)) == 0)	
	{
		// UART 2 DMA transfer complete
		if (MDR_UART2->DMACR & UART_DMACR_TXDMAE)
		{
			// Disable UART1 DMA Tx request
			MDR_UART2->DMACR &= ~UART_DMACR_TXDMAE;
			xSemaphoreGiveFromISR( xSemaphoreUART2TX, &xHigherPriorityTaskWokenByPost );
		}
	}	
	
	
	if ((MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART1_RX)) == 0)	
	{
		// Reload TCB
		tcb_ptr = (uint32_t*)&DMA_ControlTable[DMA_Channel_UART1_RX];
		*tcb_ptr++ = UART1_RX_saved_TCB[0];
		*tcb_ptr++ = UART1_RX_saved_TCB[1];
		*tcb_ptr = UART1_RX_saved_TCB[2];
		MDR_DMA->CHNL_ENABLE_SET = (1 << DMA_Channel_UART1_RX);
	}
	
	if ((MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART2_RX)) == 0)	
	{
		// Reload TCB
		tcb_ptr = (uint32_t*)&DMA_ControlTable[DMA_Channel_UART2_RX];
		*tcb_ptr++ = UART2_RX_saved_TCB[0];
		*tcb_ptr++ = UART2_RX_saved_TCB[1];
		*tcb_ptr = UART2_RX_saved_TCB[2];
		MDR_DMA->CHNL_ENABLE_SET = (1 << DMA_Channel_UART2_RX);
	}
		
	
	// Error handling
	if (DMA_GetFlagStatus(0, DMA_FLAG_DMA_ERR) == SET)
	{
		DMA_ClearError();	// normally this should not happen
	}
	
	// Force context switching if required
	//portEND_SWITCHING_ISR(xHigherPriorityTaskWokenByPost);
}





