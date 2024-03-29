/*******************************************************************
	Module i2c_eeprom.c
	
		Low-level functions for serial 24LC16B EEPROM device.

********************************************************************/

#include "MDR32Fx.h"
#include "MDR32F9Qx_i2c.h"


#include "defines.h"
#include "i2c_eeprom.h"


//==============================================================//
// Reads block from EEPROM devices
// max count for 24LC16B is 16 bytes
// Returns 0 if no errors happened
//==============================================================//
uint8_t EEPROMReadBlock(uint16_t address, uint8_t* data, uint8_t count)
{
	uint8_t error = 0;
	uint8_t dev_addr = (address & 0x0700) >> 7;
	dev_addr |= EEPROM_ADDRESS;
	
    /* Wait I2C bus is free */
    while (I2C_GetFlagStatus(I2C_FLAG_BUS_FREE) != SET) {}
	  /* Send device and bank adress */
    I2C_Send7bitAddress(dev_addr,I2C_Direction_Transmitter);
	  /* Wait end of transfer */
    while (I2C_GetFlagStatus(I2C_FLAG_nTRANS) != SET) {}
	  /* Read data if ACK was send */
    if (I2C_GetFlagStatus(I2C_FLAG_SLAVE_ACK) == SET)
    {
			// Transmit word address
			I2C_SendByte(address);
			/* Wait end of transfer */
			while (I2C_GetFlagStatus(I2C_FLAG_nTRANS) != SET) {}
				
			/* Read data if ACK was send */
			if (I2C_GetFlagStatus(I2C_FLAG_SLAVE_ACK) == SET)
			{
				/* Send device and bank adress again*/
				I2C_Send7bitAddress(dev_addr,I2C_Direction_Receiver);
				/* Wait end of transfer */
				while (I2C_GetFlagStatus(I2C_FLAG_nTRANS) != SET) {}
				/* Read data if ACK was send */
				if (I2C_GetFlagStatus(I2C_FLAG_SLAVE_ACK) == SET)
				{
					// Read data
					while(count--)
					{
						if (count)
							/* Recive byte and send ack */
							I2C_StartReceiveData(I2C_Send_to_Slave_ACK);
						else
							/* Recive byte and send nack */
							I2C_StartReceiveData(I2C_Send_to_Slave_NACK);

						/* Wait end of transfer */
						while (I2C_GetFlagStatus(I2C_FLAG_nTRANS) != SET) {}
				
						/* Get data from I2C RXD register */
						*data = I2C_GetReceivedData();
						data++;
					}
				}
				else
					error = 3;	// could not transfer device address for second time
			}
			else
				error = 2;	// could not transfer word address
    }
	else
		error = 1;	// could not even transfer device address
	
	/* Send stop */
	I2C_SendSTOP();

	return error;
}



//==============================================================//
// Writes block to EEPROM devices
// max count for 24LC16B is 16 bytes
// Returns 0 if no errors happened
//==============================================================//
uint8_t EEPROMWriteBlock(uint16_t address, uint8_t* data, uint8_t count)
{
	uint8_t error = 0;
	uint8_t dev_addr = (address & 0x0700) >> 7;
	dev_addr |= EEPROM_ADDRESS;
	
    /* Wait I2C bus is free */
    while (I2C_GetFlagStatus(I2C_FLAG_BUS_FREE) != SET) {}
	  /* Send device and bank adress */
    I2C_Send7bitAddress(dev_addr,I2C_Direction_Transmitter);
	  /* Wait end of transfer */
    while (I2C_GetFlagStatus(I2C_FLAG_nTRANS) != SET) {}
	  /* Transmit data if ACK was send */
    if (I2C_GetFlagStatus(I2C_FLAG_SLAVE_ACK) == SET)
    {
			// Transmit word address
			I2C_SendByte(address);
			/* Wait end of transfer */
			while (I2C_GetFlagStatus(I2C_FLAG_nTRANS) != SET) {}
			
			/* Transmit data if ACK was send */
			if (I2C_GetFlagStatus(I2C_FLAG_SLAVE_ACK) == SET)
			{
				// Send data
				while(count--)
				{
					I2C_SendByte(*data);
					data++;
					/* Wait end of transfer */
					while (I2C_GetFlagStatus(I2C_FLAG_nTRANS) != SET) {}
					// Check result
					if (I2C_GetFlagStatus(I2C_FLAG_SLAVE_NACK) == SET)
					{
						error = 3;	// error during data transfer
						break;
					}
				}
			}
			else
				error = 2;	// could not transfer word address
    }
	else
		error = 1;	// could not even transfer device address
	
	/* Send stop */
	I2C_SendSTOP();

	return error;
}


//==============================================================//
// Returns EEPROM status 
// Returns 0 during EEPROM write cycle
//==============================================================//
uint8_t EEPROMReady(void)
{
	/* Wait I2C bus is free */
  while (I2C_GetFlagStatus(I2C_FLAG_BUS_FREE) != SET) {}
	/* Send device and bank adress */
  I2C_Send7bitAddress(EEPROM_ADDRESS,I2C_Direction_Transmitter);
	/* Wait end of transfer */
  while (I2C_GetFlagStatus(I2C_FLAG_nTRANS) != SET) {}
	/* Send stop */
	I2C_SendSTOP();
	// If ACK is received, EEPROM is ready
	if (I2C_GetFlagStatus(I2C_FLAG_SLAVE_ACK) == SET)
		return EEPROM_READY;
	else
		return EEPROM_BUSY;
}



