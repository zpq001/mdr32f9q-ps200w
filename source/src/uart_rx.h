
typedef struct {
	uint16_t read_index;
	uint16_t size;
	uint16_t *data;
} uart_dma_rx_buffer_t;


//---------------------------------------------//
// Buffer settings
#define RX_BUFFER_SIZE			200		// Filled by DMA in cyclical way
#define EMPTY_DATA				0xFFFF

// Parser settings
#define RX_MESSAGE_MAX_LENGTH	80		// Maximum message length
#define MAX_WORDS_IN_MESSAGE	20		// Maximum words separated by SPACING_SYMBOL in message



typedef struct {
	char *keyword;
	void *nextTable;
	void (*func_ptr)(int32_t n);
	int32_t funcArg;
} parser_table_record_t;


void vTaskUARTReceiver(void *pvParameters);




