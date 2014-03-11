
#include <stdint.h>

// EEPROM address in system
#define EEPROM_ADDRESS 0xA0

#define EEPROM_OK 0
#define EEPROM_READY 1
#define EEPROM_BUSY 0


// The 24LC16B Microchip device has organization of
//	8 banks by 256 bytes with total of 2 kbytes
//	0x0000 to 0x07FF
// Page buffer is 16 bytes

// Reads block from EEPROM devices
uint8_t EEPROM_ReadBlock(uint16_t address, uint8_t* data, uint16_t count);
// Writes block to EEPROM devices
uint8_t EEPROM_WriteBlock(uint16_t address, uint8_t* data, uint16_t count);


