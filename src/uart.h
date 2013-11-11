
//---------------------------------------------//
// Buffer settings
#define RX_RING_BUFFER_SIZE		200
#define TX_RING_BUFFER_SIZE		200

// Parser settings
#define RX_MESSAGE_MAX_LENGTH	80		// Maximum message length
#define MAX_WORDS_IN_MESSAGE	20		// Maximum words separated by SPACING_SYMBOL in message


//---------------------------------------------//
// Protocol definitions
#define MESSAGE_END_SYMBOL		'\r'	// A message should end with this symbol
#define SPACING_SYMBOL			' '		// Words in the message should be separated by this symbol (1 or more)









void UARTInit(void);
void vTaskUARTReceiver(void *pvParameters);
void processUartRX(void);

void processUartTX(void);


