
typedef struct {
	uint16_t read_index;
	uint16_t size;
	uint16_t *data;
} uart_dma_rx_buffer_t;

typedef struct {
	uint16_t type;
	char *pdata;
} uart_transmiter_msg_t;


//---------------------------------------------//
// Buffer settings
#define RX_BUFFER_SIZE			200		// Filled by DMA in cyclical way
#define TX_BUFFER_SIZE			100

#define EMPTY_DATA				0xFFFF

// Parser settings
#define RX_MESSAGE_MAX_LENGTH	80		// Maximum message length
#define MAX_WORDS_IN_MESSAGE	20		// Maximum words separated by SPACING_SYMBOL in message


//---------------------------------------------//
// Protocol definitions
#define MESSAGE_NEW_LINE		'\n'	// similar to MESSAGE_END_SYMBOL
#define MESSAGE_END_SYMBOL		'\r'	// A message should end with this symbol
#define SPACING_SYMBOL			' '		// Words in the message should be separated by this symbol (1 or more)




//---------------------------------------------//
// Task message definitions
#define SEND_STRING				0x0001
#define SEND_ALLOCATED_STRING	0x0002
#define RESPONSE_OK				0x0100
#define RESPONSE_BAD_CMD		0x0101



void vTaskUARTReceiver(void *pvParameters);
void void vTaskUARTTransmitter(void *pvParameters);



