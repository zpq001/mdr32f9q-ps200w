
#include "MDR32Fx.h"
#include "MDR32F9Qx_dma.h"


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


static volatile uint32_t dma_channel_active_set = 0;

//---------------------------------------------//
//	This function is similar to SPL function DMA_Cmd()
// but with additional channels flag processing.
// It is assumed that interrupts are enabled.
//---------------------------------------------//
void DMA_Cmd_and_Update_flags(uint8_t DMA_Channel, FunctionalState NewState)
{
	// Access to dma_channel_active_set must be atomic
  __disable_irq();
  /* Channel Enable/Disable */
  if ( NewState == ENABLE)
  {
    MDR_DMA->CHNL_ENABLE_SET = (1 << DMA_Channel);
	dma_channel_active_set |= (1 << DMA_Channel);
  }
  else
  {
    MDR_DMA->CHNL_ENABLE_CLR = (1 << DMA_Channel);
	dma_channel_active_set &= ~(1 << DMA_Channel);
  }
  __enable_irq();
}



// Possibly will be enough to analyze and reset UART request enable ?
// instead of extra variable
void DMA_IRQHandler(void)
{
	uint32_t channels_completed = ~MDR_DMA->CHNL_ENABLE_SET & dma_channel_active_set;
	if (channels_completed & (1<<DMA_Channel_UART1_TX))		// UART 1 DMA transfer complete
	{
		// postSemaphoreFromISR(__complete__);
	}
	if (channels_completed & (1<<DMA_Channel_UART2_TX))		// UART 2 DMA transfer complete
	{
		// postSemaphoreFromISR(__complete__);
	}
	// Error handling
	if (DMA_GetFlagStatus(0, DMA_FLAG_DMA_ERR) == SET)
	{
		DMA_ClearError();	// normally this should not happen
	}
	dma_channel_active_set &= ~channels_completed;
}





