#include <stdint.h>
#include "MDR32Fx.h"
#include "MDR32F9Qx_dma.h"

// Settings
#define RX_BUFFER_SIZE			200			// Filled by DMA in cyclical way
#define RX_EMPTY_DATA			0xFFFF		// RX buffer empty data marker


typedef uint8_t (*stream_parser_callback)(char);
typedef uint8_t (*uart_tx_done_callback)(uint8_t);

typedef struct {
	uint16_t read_index;
	uint16_t size;
	uint16_t data[RX_BUFFER_SIZE];
} uart_dma_rx_buffer_t;

typedef struct {
	// Receiver buffers
	uart_dma_rx_buffer_t rx_buffer;
	// Saved DMA configuration for fast reload in ISR
	uint32_t saved_TCB[3];
	uint8_t DMA_Channel;
	MDR_UART_TypeDef *MDR_UARTx;
	stream_parser_callback parser_cb;
} UART_HW_Receive_Context_t;

typedef struct {
	uint8_t DMA_Channel;
	MDR_UART_TypeDef *MDR_UARTx;
	DMA_CtrlDataInitTypeDef DMA_PriCtrlStr;
	uart_tx_done_callback tx_done_cb;
} UART_HW_Transmit_Context_t;





// Initialization
void UART_Initialize(uint8_t uart_num);
void UART_Register_RX_Stream_Callback(int uart_num, stream_parser_callback func);
void UART_Register_TX_Done_Callback(int uart_num, uart_tx_done_callback func);
// Control functions
void UART_Enable(uint8_t uart_num, uint32_t buadRate, uint8_t parity);
void UART_Disable(uint8_t uart_num);
// RX functions
void UART_Start_RX(uint8_t uart_num);
void UART_Stop_RX(uint8_t uart_num);
uint8_t UART_read_RX_stream(uint8_t uart_num, char **rx_string, uint16_t *num_of_chars_to_read);
void UART_clean_RX_stream(uint8_t uart_num);
// TX functions
void UART_Send_Data(uint8_t uart_num, char *data, uint16_t length);
// Handlers called from ISR
uint8_t UART_Rx_DMA_Done_Handler(uint8_t uart_num);
uint8_t UART_Tx_DMA_Done_Handler(uint8_t uart_num);





