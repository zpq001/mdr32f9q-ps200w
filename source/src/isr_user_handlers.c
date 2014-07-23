
#include <stdint.h>
#include "MDR32Fx.h"
#include "MDR32F9Qx_dma.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "uart_hw.h"

void DMA_IRQHandler(void)
{
	portBASE_TYPE xHPTaskWoken = pdFALSE;
	
	// UART 1 DMA transfer complete
	if ((MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART1_TX)) == 0)
	{
		// One may use some kind of software flag instead of hardware bits
		if (MDR_UART1->DMACR & UART_DMACR_TXDMAE)
		{
			xHPTaskWoken |= UART_Tx_DMA_Done_Handler(1);
		}
	}
	
	// UART 2 DMA transfer complete
	if ((MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART2_TX)) == 0)	
	{
		if (MDR_UART2->DMACR & UART_DMACR_TXDMAE)
		{
			xHPTaskWoken |= UART_Tx_DMA_Done_Handler(2);
		}
	}	
	
	// UART 1 DMA receive complete
	if ((MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART1_RX)) == 0)	
	{
		xHPTaskWoken |= UART_Rx_DMA_Done_Handler(1);
	}
	
	// UART 2 DMA receive complete
	if ((MDR_DMA->CHNL_ENABLE_SET & (1<<DMA_Channel_UART2_RX)) == 0)	
	{
		xHPTaskWoken |= UART_Rx_DMA_Done_Handler(2);
	}
		
	
	// Error handling
	if (DMA_GetFlagStatus(0, DMA_FLAG_DMA_ERR) == SET)
	{
		DMA_ClearError();	// normally this should not happen
	}
	
	// Force context switching if required
	//portEND_SWITCHING_ISR(xHigherPriorityTaskWokenByPost);
}











