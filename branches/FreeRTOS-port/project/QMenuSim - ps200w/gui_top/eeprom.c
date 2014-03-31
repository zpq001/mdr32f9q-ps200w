/*******************************************************************
	Module i2c_eeprom.c
	
	Top-level functions for serial 24LC16B EEPROM device.

	24LC16B is 16K bit = 2048 bytes

	Data at EE_GSETTINGS_BASE stores general device information
	Device profiles start at address EE_PROFILES_BASE
	Each profile occupies EE_PROFILE_SIZE bytes.
	First profile is recent - it is saved on power off and used
	for restore on power on.
	
********************************************************************/

#include <string.h>		// usigng memset

#include "eeprom.h"

#include "global_def.h"
#include "converter.h"


static device_profile_t device_profile_data;		
device_profile_t *device_profile = &device_profile_data;
char device_profile_name[EE_PROFILE_NAME_SIZE];

static global_settings_t global_settings_data;
global_settings_t * global_settings = &global_settings_data;

uint8_t profile_info[EE_PROFILES_COUNT];


uint8_t EE_GetProfileState(uint8_t i)
{
    return EE_PROFILE_VALID;
}





uint8_t EE_IsRecentProfileSavingEnabled(void)
{
    return 1;
}

uint8_t EE_IsRecentProfileRestoreEnabled(void)
{
    return 1;
}



/*

//-------------------------------------------------------//
//	Reads last saved profile into global device_profile
//	Returns 0 if success.
//-------------------------------------------------------//
uint8_t EE_load_recent_profile(void)
{	
}

//-------------------------------------------------------//
//	Reads profile with specified number (starting with 0)
//	into global device_profile.
//	Returns 0 if success.
//-------------------------------------------------------//
uint8_t EE_load_profile(uint8_t number)
{
}

//-------------------------------------------------------//
//	Saves the global device_profile
//	
//-------------------------------------------------------//
void EE_save_recent_profile(void)
{
}


//-------------------------------------------------------//
//	Returns total count of profiles
//	
//-------------------------------------------------------//
uint8_t EE_get_profile_count(void)
{
	return EE_PROFILES_COUNT;
}

//-------------------------------------------------------//
//	Returns name of a profile
//	If profile is empty or there is CRC error 
//	(which is actualy the same), function return 0.
//-------------------------------------------------------//
char *EE_get_profile_name(uint8_t number)
{
}
*/






