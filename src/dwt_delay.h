

#define    DWT_CYCCNT    *(volatile uint32_t *)0xE0001004
#define    DWT_CONTROL   *(volatile uint32_t *)0xE0001000
#define    SCB_DEMCR     *(volatile uint32_t *)0xE000EDFC


// Enables DWT counter
void DWTCounterInit(void);

// Non - blocking us delay
void DWTStartDelayUs(uint32_t us);
// Non - blocking delay status
uint8_t DWTDelayInProgress(void);
// Blocking us delay
void DWTDelayUs(uint32_t us);




