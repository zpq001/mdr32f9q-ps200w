/*******************************************************************
	Module i2c_eeprom.c
	
	Top-level functions for serial 24LC16B EEPROM device.

	24LC16B is 16K bit = 2048 bytes

	Bytes 0-15 store general device information
	Device profiles start at address 0x0010
	Each profile occupies EEPROM_PROFILE_SIZE bytes.
	First profile is current - it is saved on power off and used
	for restore on power on.
	
********************************************************************/

#include "MDR32Fx.h" 

#include "FreeRTOS.h"
#include "task.h"

#include "eeprom.h"


device_profile_t device_profile;


uint8_t EEPROM_load_status_info(void)
{
}

uint8_t EEPROM_save_status_info(void)
{
}

void EEPROM_load_default_profile(void)
{
}

//-------------------------------------------------------//
//	Reads last saved profile into global device_profile
//	Returns 0 if success.
//-------------------------------------------------------//
uint8_t EEPROM_load_recent_profile(void)
{	
}

//-------------------------------------------------------//
//	Reads profile with specified number (starting with 0)
//	into global device_profile.
//	Returns 0 if success.
//-------------------------------------------------------//
uint8_t EEPROM_load_profile(uint8_t number)
{
}

//-------------------------------------------------------//
//	Saves the global device_profile
//	
//-------------------------------------------------------//
void EEPROM_save_recent_profile(void)
{
}


//-------------------------------------------------------//
//	Returns total count of profiles
//	
//-------------------------------------------------------//
uint8_t EEPROM_get_profile_count(void)
{
	return EEPROM_PROFILES_COUNT;
}

//-------------------------------------------------------//
//	Returns name of a profile
//	If profile is empty or there is CRC error 
//	(which is actualy the same), function return 0.
//-------------------------------------------------------//
char *EEPROM_get_profile_name(uint8_t number)
{
}







