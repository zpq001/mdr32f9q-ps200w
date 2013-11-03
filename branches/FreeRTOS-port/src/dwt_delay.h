

#define    DWT_CYCCNT    *(volatile uint32_t *)0xE0001004
#define    DWT_CONTROL   *(volatile uint32_t *)0xE0001000
#define    SCB_DEMCR     *(volatile uint32_t *)0xE000EDFC


// Enables DWT counter
void DWTCounterInit(void);


uint32_t DWTStartDelayUs(uint32_t us);
uint8_t DWTDelayInProgress(uint32_t time_mark);



