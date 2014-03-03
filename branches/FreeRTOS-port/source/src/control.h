
/********************************************************************
	Definitions for system control
********************************************************************/

#include "global_def.h"



/*
typedef struct 
{
	uint8_t	ConverterState 		:1;		// open collector
	uint8_t	SelectedChannel		:1;
	uint8_t CurrentLimit			:1;
	uint8_t	LoadDisable				:1;		// open collector
}	SYSTEM_CONTROL_Typedef;// __attribute__((bitband));
*/

/********************************************************************
	Definitions for system status
********************************************************************/

// ConverterOverload
#define	NORMAL 			0x0
#define	OVERLOAD		0x1
// ExternalSwitchState
#define	SWITCH_OFF		0x0		// TODO: remove to buttons
#define	SWITCH_ON		0x1
// LineInStatus
#define	OFFLINE			0x0
#define	ONLINE			0x1



/*
typedef struct 
{
	uint8_t		ConverterOverload		:1;
	uint8_t		ExternalSwitchState :1;
	uint8_t		LineInStatus				:1;
}	SYSTEM_STATUS_Typedef;// __attribute__((bitband));;
*/





uint8_t GetOverloadStatus(void);
uint8_t GetACLineStatus(void);

uint8_t GetConverterState(void);

void SetConverterState(uint8_t newState);
void SetFeedbackChannel(uint8_t newChannel);
void SetCurrentRange(uint8_t newLimit);
void SetOutputLoad(uint8_t newLoad);

