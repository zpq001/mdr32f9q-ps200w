
#include "MDR32F9Qx_uart.h"
#include "MDR32F9Qx_dma.h"

#include "systemfunc.h"
#include "uart_hw.h"



extern DMA_CtrlDataTypeDef DMA_ControlTable[];




static UART_HW_Receive_Context_t uart_hw_rx_ctx1 = {
	{0},	// rx_buffer
	{0},	// Saved TCB
	DMA_Channel_UART1_RX,
	MDR_UART1
};

static UART_HW_Receive_Context_t uart_hw_rx_ctx2 = {
	{0},	// rx_buffer
	{0},	// Saved TCB
	DMA_Channel_UART2_RX,
	MDR_UART2
};


static UART_HW_Transmit_Context_t uart_hw_tx_ctx1 = {
	DMA_Channel_UART1_TX,
	MDR_UART1,
	{0}		// dma control struct
};

static UART_HW_Transmit_Context_t uart_hw_tx_ctx2 = {
	DMA_Channel_UART2_TX,
	MDR_UART2,
	{0}		// dma control struct
};


//-------------------------------------------------------//
// Ring buffer functions

static void UART_Init_RX_buffer(uart_dma_rx_buffer_t *rx_buffer)
{
	rx_buffer->size = RX_BUFFER_SIZE;
	rx_buffer->read_index = RX_BUFFER_SIZE;
	// Fill buffer with invalid data
	while(rx_buffer->read_index)
	{
		rx_buffer->data[--rx_buffer->read_index] = RX_EMPTY_DATA;
	}
	// read_index is 0 now!
}

static uint8_t UART_Get_from_RX_buffer(uart_dma_rx_buffer_t *rx_buffer, uint16_t *rx_word)
{
	*rx_word = rx_buffer->data[rx_buffer->read_index];
	if (*rx_word != RX_EMPTY_DATA)
	{
		// Valid data has been put into buffer by DMA
		rx_buffer->data[rx_buffer->read_index] = RX_EMPTY_DATA;	
		rx_buffer->read_index = (rx_buffer->read_index < (rx_buffer->size - 1)) ? rx_buffer->read_index + 1 : 0;
		return 1;
	}
	return 0;
}



//-------------------------------------------------------//
// Hardware functions


// DMA channel UARTx RX configuration 
static void UART_init_RX_DMA(UART_HW_Receive_Context_t *ucrx)
{
	DMA_ChannelInitTypeDef DMA_InitStr;
	DMA_CtrlDataInitTypeDef DMA_PriCtrlStr;
	uint32_t *tcb_ptr;
	
	// Setup Primary Control Data 
	DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)(&(ucrx->MDR_UARTx->DR));
	DMA_PriCtrlStr.DMA_DestBaseAddr = (uint32_t)ucrx->rx_buffer.data;		// dest (buffer of 16-bit shorts)
	DMA_PriCtrlStr.DMA_SourceIncSize = DMA_SourceIncNo;
	DMA_PriCtrlStr.DMA_DestIncSize = DMA_DestIncHalfword ;
	DMA_PriCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_PriCtrlStr.DMA_Mode = DMA_Mode_Basic;								// 
	DMA_PriCtrlStr.DMA_CycleSize = ucrx->rx_buffer.size;					// count of 16-bit shorts
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
	my_DMA_ChannelInit(ucrx->DMA_Channel, &DMA_InitStr);
	
	// Save created RX UART DMA control block for reinit in DMA ISR
	tcb_ptr = (uint32_t*)&DMA_ControlTable[ucrx->DMA_Channel];
	ucrx->saved_TCB[0] = *tcb_ptr++;
	ucrx->saved_TCB[1] = *tcb_ptr++;
	ucrx->saved_TCB[2] = *tcb_ptr;
}



// DMA channel UARTx TX configuration 
static void UART_init_TX_DMA(UART_HW_Transmit_Context_t *uctx)
{
	DMA_ChannelInitTypeDef DMA_InitStr;
	
	// Setup Primary Control Data 
	
	uctx->DMA_PriCtrlStr.DMA_SourceBaseAddr = 0;							// Will be set before channel start
	uctx->DMA_PriCtrlStr.DMA_DestBaseAddr = (uint32_t)(&(uctx->MDR_UARTx->DR));			
	uctx->DMA_PriCtrlStr.DMA_SourceIncSize = DMA_SourceIncByte;
	uctx->DMA_PriCtrlStr.DMA_DestIncSize = DMA_DestIncNo;
	uctx->DMA_PriCtrlStr.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	uctx->DMA_PriCtrlStr.DMA_Mode = DMA_Mode_Basic; 
	uctx->DMA_PriCtrlStr.DMA_CycleSize = 0;									// Will be set before channel start
	uctx->DMA_PriCtrlStr.DMA_NumContinuous = DMA_Transfers_1;
	uctx->DMA_PriCtrlStr.DMA_SourceProtCtrl = DMA_SourcePrivileged;			// ?
	uctx->DMA_PriCtrlStr.DMA_DestProtCtrl = DMA_DestPrivileged;				// ?
	
	// Setup Channel Structure
	DMA_InitStr.DMA_PriCtrlData = 0;										// Will be set before channel start
	DMA_InitStr.DMA_AltCtrlData = 0;										// Not used
	DMA_InitStr.DMA_ProtCtrl = 0;											// Not used
	DMA_InitStr.DMA_Priority = DMA_Priority_High;
	DMA_InitStr.DMA_UseBurst = DMA_BurstClear;								// Enable single words trasfer
	DMA_InitStr.DMA_SelectDataStructure = DMA_CTRL_DATA_PRIMARY;
	
	// Init DMA channel
	my_DMA_ChannelInit(uctx->DMA_Channel, &DMA_InitStr);
}






//=================================================================//
//                         User interface                          //
//=================================================================//


//-------------------------------------------------------//
// Initialization

// System UART initialization must be done already - clock must be applied
void UART_Initialize(uint8_t uart_num)
{
	UART_HW_Receive_Context_t *ucrx = (uart_num == 1) ? &uart_hw_rx_ctx1 : &uart_hw_rx_ctx2;	
	UART_HW_Transmit_Context_t *uctx = (uart_num == 1) ? &uart_hw_tx_ctx1 : &uart_hw_tx_ctx2;	
	// Setup and init receiver ring buffer
	UART_Init_RX_buffer(&ucrx->rx_buffer);
	// Setup RX DMA channel
	UART_init_RX_DMA(ucrx);
	// Setup TX DMA channel
	UART_init_TX_DMA(uctx);
	// Enable RX DMA channel
	DMA_Cmd(ucrx->DMA_Channel, ENABLE);	
	// Call UART_Enable() and UART_start_RX() to start receive operation
	// Call UART_Send_Data() to start transmitting data
}


void UART_Register_RX_Stream_Callback(int uart_num, stream_parser_callback func)
{
	UART_HW_Receive_Context_t *ucrx = (uart_num == 1) ? &uart_hw_rx_ctx1 : &uart_hw_rx_ctx2;
	ucrx->parser_cb = func;
}

void UART_Register_TX_Done_Callback(int uart_num, uart_tx_done_callback func)
{
	UART_HW_Transmit_Context_t *uctx = (uart_num == 1) ? &uart_hw_tx_ctx1 : &uart_hw_tx_ctx2;	
	uctx->tx_done_cb = func;
}


//-------------------------------------------------------//
// Control functions

void UART_Enable(uint8_t uart_num, uint32_t baudRate, uint8_t parity)
{
	UART_HW_Receive_Context_t *ucrx = (uart_num == 1) ? &uart_hw_rx_ctx1 : &uart_hw_rx_ctx2;
	BaudRateStatus initStatus;
	UART_InitTypeDef UART_cfg;
	// Deinit UART
	UART_DeInit(ucrx->MDR_UARTx);
	// Fill init structure
	UART_cfg.UART_BaudRate = baudRate;
	UART_cfg.UART_WordLength = UART_WordLength8b;
	UART_cfg.UART_StopBits = UART_StopBits1;
	UART_cfg.UART_Parity = parity;
	UART_cfg.UART_FIFOMode = UART_FIFO_ON;
	UART_cfg.UART_HardwareFlowControl = UART_HardwareFlowControl_RXE | UART_HardwareFlowControl_TXE; 
	// Configure
	initStatus = UART_Init(ucrx->MDR_UARTx, &UART_cfg);
	UART_DMAConfig(ucrx->MDR_UARTx, UART_IT_FIFO_LVL_8words, UART_IT_FIFO_LVL_8words);
	UART_Cmd(ucrx->MDR_UARTx, ENABLE);
}

void UART_Disable(uint8_t uart_num)
{
	UART_HW_Receive_Context_t *ucrx = (uart_num == 1) ? &uart_hw_rx_ctx1 : &uart_hw_rx_ctx2;
	
	// Wait until transmitter is done	
	while (UART_GetFlagStatus(ucrx->MDR_UARTx, UART_FLAG_TXFE) == RESET) {};
	while (UART_GetFlagStatus(ucrx->MDR_UARTx, UART_FLAG_BUSY) == SET) {};
		
	// Disable UARTn
	UART_Cmd(ucrx->MDR_UARTx, DISABLE);
}

void UART_Start_RX(uint8_t uart_num)
{
	UART_HW_Receive_Context_t *ucrx = (uart_num == 1) ? &uart_hw_rx_ctx1 : &uart_hw_rx_ctx2;
	// Enable receiver
	//ucrx->MDR_UARTx->CR |= UART_HardwareFlowControl_RXE;
	// Enable UARTn DMA Rx request
	UART_DMACmd(ucrx->MDR_UARTx, UART_DMA_RXE, ENABLE);
}


void UART_Stop_RX(uint8_t uart_num)
{	
	UART_HW_Receive_Context_t *ucrx = (uart_num == 1) ? &uart_hw_rx_ctx1 : &uart_hw_rx_ctx2;
	// Disable UARTn DMA Rx request
	UART_DMACmd(ucrx->MDR_UARTx, UART_DMA_RXE, DISABLE);
	// Clear RX FIFO by disabling UART receiver ?
	//ucrx->MDR_UARTx->CR &= ~UART_HardwareFlowControl_RXE;
}



//-------------------------------------------------------//
// Rx data functions

// Clean UART RX stream
void UART_clean_RX_stream(uint8_t uart_num)
{
	uint16_t dummy_word;
	UART_HW_Receive_Context_t *ucrx = (uart_num == 1) ? &uart_hw_rx_ctx1 : &uart_hw_rx_ctx2;
	while(UART_Get_from_RX_buffer(&ucrx->rx_buffer, &dummy_word));
}


// Read UART RX stream that is filled by DMA for num_of_chars_to_read symbols.
uint8_t UART_read_RX_stream(uint8_t uart_num, char **rx_string, uint16_t *num_of_chars_to_read)
{
	uint16_t rx_word;
	char rx_char;
	int8_t res_code;
	UART_HW_Receive_Context_t *ucrx = (uart_num == 1) ? &uart_hw_rx_ctx1 : &uart_hw_rx_ctx2;
	
	if ((rx_string == 0) || (num_of_chars_to_read == 0) || (*num_of_chars_to_read == 0))
		return 0;
	
	// Read chars from DMA stream
	while (UART_Get_from_RX_buffer(&ucrx->rx_buffer, &rx_word))
	{
		// Check for communication errors
		if ((rx_word & ( (1<<UART_Data_BE) | (1<<UART_Data_PE) | (1<<UART_Data_FE) )) != 0)
			continue;
		rx_char = (char)rx_word;
		*(*rx_string)++ = rx_char;
		(*num_of_chars_to_read)--;
		// Call parser callback if it has been registered
		if (ucrx->parser_cb)
		{
			res_code = ucrx->parser_cb(rx_char);
			if (res_code)
				return res_code;
		}
		if (*num_of_chars_to_read == 0)
			return 1;
	}
	return 0;
}



//-------------------------------------------------------//
// Tx data functions

// DMA cannot read from program memory
// Make sure data resides in SRAM
void UART_Send_Data(uint8_t uart_num, char *data, uint16_t length)
{
	UART_HW_Transmit_Context_t *uctx = (uart_num == 1) ? &uart_hw_tx_ctx1 : &uart_hw_tx_ctx2;	
	
	uctx->DMA_PriCtrlStr.DMA_SourceBaseAddr = (uint32_t)data;
	uctx->DMA_PriCtrlStr.DMA_CycleSize = length;

	// Start DMA
	DMA_CtrlInit (uctx->DMA_Channel, DMA_CTRL_DATA_PRIMARY, &uctx->DMA_PriCtrlStr);
	DMA_Cmd(uctx->DMA_Channel, ENABLE);
	
	// Enable UART1 DMA Tx request
	UART_DMACmd(uctx->MDR_UARTx, UART_DMA_TXE, ENABLE);
}





//-------------------------------------------------------//
// Handlers called from ISR
// Return value is flag of woken task with higher priority - FreeRTOS stuff

uint8_t UART_Rx_DMA_Done_Handler(uint8_t uart_num)
{
	UART_HW_Receive_Context_t *ucrx = (uart_num == 1) ? &uart_hw_rx_ctx1 : &uart_hw_rx_ctx2;
	// Reload TCB
	uint32_t *tcb_ptr = (uint32_t*)&DMA_ControlTable[ucrx->DMA_Channel];
	*tcb_ptr++ = ucrx->saved_TCB[0];
	*tcb_ptr++ = ucrx->saved_TCB[1];
	*tcb_ptr = ucrx->saved_TCB[2];
	// Restart channel
	MDR_DMA->CHNL_ENABLE_SET = (1 << ucrx->DMA_Channel);
	
	return 0;
}


uint8_t UART_Tx_DMA_Done_Handler(uint8_t uart_num)
{
	UART_HW_Transmit_Context_t *uctx = (uart_num == 1) ? &uart_hw_tx_ctx1 : &uart_hw_tx_ctx2;	
	// Disable UART DMA Tx request
	uctx->MDR_UARTx->DMACR &= ~UART_DMACR_TXDMAE;
	// Call handler
	if (uctx->tx_done_cb)
	{
		return uctx->tx_done_cb(uart_num);
	}
	return 0;
}










