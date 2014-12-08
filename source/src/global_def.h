#ifndef __GLOBAL_DEF_H_
#define __GLOBAL_DEF_H_



// ConverterState
#define CONVERTER_OFF    	0x0
#define CONVERTER_ON     	0x1
// SelectedChannel
#define	CHANNEL_5V			0x1
#define	CHANNEL_12V			0x0
// CurrentRange
#define	CURRENT_RANGE_HIGH	0x1
#define	CURRENT_RANGE_LOW 	0x0
// LoadDisable
#define	LOAD_DISABLE		0x1
#define	LOAD_ENABLE			0x0





//-------------------------------------------------------//
// Other

// Extended converter channel definition - used when
// currently operating channel should be updated
#define OPERATING_CHANNEL 2

// Extended converter current range definition - used when
// currently operating range should be updated
#define OPERATING_CURRENT_RANGE 2

// Software limit types
#define LIMIT_TYPE_LOW			0x00
#define LIMIT_TYPE_HIGH			0x01


// External switch modes
enum ExtSwModes {EXTSW_DIRECT, EXTSW_TOGGLE, EXTSW_TOGGLE_OFF};

// UART parity modes - those values come from StdPeriph UART library
enum MyUartParity {PARITY_NO = 0x0, PARITY_ODD = 0x2, PARITY_EVEN = 0x6};


enum senders {
	sender_UART1 = 1,
	sender_UART2,

	sender_GUI,
	sender_CONVERTER,
	sender_ADC,
	sender_BUTTONS
};




#endif

