/*******************************************************************
	Module i2c_eeprom.c
	
	Top-level functions for serial 24LC16B EEPROM device.

	24LC16B is 16K bit = 2048 bytes

	Bytes 0-15 store general device information
	Device profiles start at address 0x0010
	Each profile occupies EE_PROFILE_SIZE bytes.
	First profile is recent - it is saved on power off and used
	for restore on power on.
	
********************************************************************/

#include "MDR32Fx.h" 

#include "FreeRTOS.h"
#include "task.h"

#include "i2c_eeprom.h"
#include "eeprom.h"

#include "global_def.h"
#include "converter.h"


static device_profile_t device_profile_data;		
device_profile_t *device_profile = &device_profile_data;
char device_profile_name[EE_PROFILE_NAME_SIZE];

static global_settings_t global_settings_data;
global_settings_t * global_settings = &global_settings_data;

uint8_t profile_info[EE_PROFILES_COUNT];


static uint16_t crc16_update(uint16_t crc, uint8_t a)
{
	uint8_t i;
	crc ^= a;
	for (i = 0; i < 8; ++i)
	{
		if (crc & 1)
			crc = (crc >> 1) ^ 0xA001;
		else
			crc = (crc >> 1);
	}
	return crc;
}

static uint16_t get_crc16(uint8_t *data, uint16_t size, uint16_t seed)
{
	while(size--)
		seed = crc16_update(seed, *data++);
	return seed;
}
	

static void fill_global_settings_by_default(void)
{
	global_settings->adc_voltage_offset = 0;
	global_settings->adc_current_offset = 0;
	global_settings->number_of_power_cycles = 0;
}

static void fill_device_profile_by_default(void)
{
	device_profile->converter_profile.name[0] = '\0';
	
	// 5V channel voltage
	device_profile->converter_profile.ch5v_voltage.setting = 5000;
	device_profile->converter_profile.ch5v_voltage.limit_low = CONV_MIN_VOLTAGE_5V_CHANNEL;
	device_profile->converter_profile.ch5v_voltage.limit_high = CONV_MAX_VOLTAGE_5V_CHANNEL;
	device_profile->converter_profile.ch5v_voltage.enable_low_limit = 0;
	device_profile->converter_profile.ch5v_voltage.enable_high_limit = 0;
	// Current
	device_profile->converter_profile.ch5v_current.low_range.setting = 2000;
	device_profile->converter_profile.ch5v_current.low_range.limit_low = CONV_LOW_CURRENT_RANGE_MIN;
	device_profile->converter_profile.ch5v_current.low_range.limit_high = CONV_LOW_CURRENT_RANGE_MAX;
	device_profile->converter_profile.ch5v_current.low_range.enable_low_limit = 0;
	device_profile->converter_profile.ch5v_current.low_range.enable_high_limit = 0;
	device_profile->converter_profile.ch5v_current.high_range.setting = 4000;
	device_profile->converter_profile.ch5v_current.high_range.limit_low = CONV_HIGH_CURRENT_RANGE_MIN;
	device_profile->converter_profile.ch5v_current.high_range.limit_high = CONV_HIGH_CURRENT_RANGE_MAX;
	device_profile->converter_profile.ch5v_current.high_range.enable_low_limit = 0;
	device_profile->converter_profile.ch5v_current.high_range.enable_high_limit = 0;
	device_profile->converter_profile.ch5v_current.selected_range = CURRENT_RANGE_LOW;
	// 12V channel voltage
	device_profile->converter_profile.ch12v_voltage.setting = 12000;
	device_profile->converter_profile.ch12v_voltage.limit_low = CONV_MIN_VOLTAGE_12V_CHANNEL;
	device_profile->converter_profile.ch12v_voltage.limit_high = CONV_MAX_VOLTAGE_12V_CHANNEL;
	device_profile->converter_profile.ch12v_voltage.enable_low_limit = 0;
	device_profile->converter_profile.ch12v_voltage.enable_high_limit = 0;
	// Current
	device_profile->converter_profile.ch12v_current.low_range.setting = 1000;
	device_profile->converter_profile.ch12v_current.low_range.limit_low = CONV_LOW_CURRENT_RANGE_MIN;
	device_profile->converter_profile.ch12v_current.low_range.limit_high = CONV_LOW_CURRENT_RANGE_MAX;
	device_profile->converter_profile.ch12v_current.low_range.enable_low_limit = 0;
	device_profile->converter_profile.ch12v_current.low_range.enable_high_limit = 0;
	device_profile->converter_profile.ch12v_current.high_range.setting = 2000;
	device_profile->converter_profile.ch12v_current.high_range.limit_low = CONV_LOW_CURRENT_RANGE_MIN;
	device_profile->converter_profile.ch12v_current.high_range.limit_high = CONV_LOW_CURRENT_RANGE_MAX;
	device_profile->converter_profile.ch12v_current.high_range.enable_low_limit = 0;
	device_profile->converter_profile.ch12v_current.high_range.enable_high_limit = 0;
	device_profile->converter_profile.ch12v_current.selected_range = CURRENT_RANGE_LOW;
	
	// Other fields
	// ...
}



//	Return: bits are set indicating errors:
//		EE_GSETTINGS_HW_ERROR if there was an hardware error for global settings
//  	EE_GSETTINGS_CRC_ERROR if there was CRC error for global settings
//		EE_PROFILE_HW_ERROR if there was an hardware error for recent profile
//		EE_PROFILE_CRC_ERROR if there was CRC error for recent profile
//		If specific bit is cleared, it means that that kind of error has not occured.
uint8_t EE_InitialLoad(void)
{
	uint8_t err_code = EE_OK;
	uint8_t hw_result;
	uint16_t crc;
	uint16_t ee_crc;
	uint8_t i;
	uint16_t ee_addr;
	
	//-------------------------------//
	//	Read global settings
	ee_addr = EE_GSETTINGS_BASE;
	hw_result = EE_ReadBlock(ee_addr, (uint8_t *)&global_settings_data, sizeof(global_settings_t));
	ee_addr = EE_GSETTINGS_BASE + EE_GSETTINGS_CRC_OFFSET;
	hw_result |= EE_ReadBlock(ee_addr, (uint8_t *)&ee_crc, EE_CRC_SIZE);
	if (hw_result == 0)
	{
		// Hardware EEPROM access OK. Check CRC.
		crc = get_crc16((uint8_t *)&global_settings_data, sizeof(global_settings_t), 0xFFFF);
		if (crc != ee_crc)
		{
			// Global settings CRC is broken.
			err_code |= EE_GSETTINGS_CRC_ERROR;
		}	
	}
	else
	{
		// EEPROM hardware error. Cannot access device.
		err_code |= EE_GSETTINGS_HW_ERROR;
	}
	
	//-------------------------------//
	//	Read profiles and fill info structure
	for (i=0; i<EE_PROFILES_COUNT; i++)
	{
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE);
		hw_result = EE_ReadBlock(ee_addr, (uint8_t *)&device_profile_data, sizeof(device_profile_t));
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_NAME_OFFSET;
		hw_result |= EE_ReadBlock(ee_addr, (uint8_t *)&device_profile_name, EE_PROFILE_NAME_SIZE);
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_CRC_OFFSET;
		hw_result |= EE_ReadBlock(ee_addr, (uint8_t *)&ee_crc, EE_CRC_SIZE);
		if (hw_result == 0)
		{
			// Hardware EEPROM access OK. Check CRC.
			crc = get_crc16((uint8_t *)&device_profile_data, sizeof(device_profile_t), 0xFFFF);
			crc = get_crc16((uint8_t *)&device_profile_name, EE_PROFILE_NAME_SIZE, crc);
			if (crc == ee_crc)
			{
				// Profile CRC is OK.
				profile_info[i] = EE_PROFILE_VALID;
			}	
			else
			{
				// Profile CRC is broken.
				profile_info[i] = EE_PROFILE_CRC_ERR;
			}
		}
		else
		{
			// EEPROM hardware error. Cannot access device.
			profile_info[i] = EE_PROFILE_HW_ERR;
		}
	}
	
	
	//-------------------------------//
	//	Read recent profile
	ee_addr = EE_RECENT_PROFILE_BASE;
	hw_result = EE_ReadBlock(ee_addr, (uint8_t *)&device_profile_data, sizeof(device_profile_t));
	ee_addr = EE_RECENT_PROFILE_BASE + EE_PROFILE_NAME_OFFSET;
	hw_result |= EE_ReadBlock(ee_addr, (uint8_t *)&device_profile_name, EE_PROFILE_NAME_SIZE);
	ee_addr = EE_RECENT_PROFILE_BASE + EE_PROFILE_CRC_OFFSET;
	hw_result |= EE_ReadBlock(ee_addr, (uint8_t *)&ee_crc, EE_CRC_SIZE);
	if (hw_result == 0)
	{
		// Hardware EEPROM access OK. Check CRC.
		crc = get_crc16((uint8_t *)&device_profile_data, sizeof(device_profile_t), 0xFFFF);
		crc = get_crc16((uint8_t *)&device_profile_name, EE_PROFILE_NAME_SIZE, crc);
		if (crc != ee_crc)
		{
			// Profile CRC is broken.
			err_code |= EE_PROFILE_CRC_ERR;
		}
	}
	else
	{
		// EEPROM hardware error. Cannot access device.
		err_code |= EE_PROFILE_HW_ERR;
	}
	
	// Check status and restore defaults
	if (err_code & (EE_GSETTINGS_HW_ERROR | EE_GSETTINGS_CRC_ERROR))
	{
		fill_global_settings_by_default();
	}
	if (err_code & (EE_PROFILE_HW_ERROR | EE_PROFILE_CRC_ERROR))
	{
		fill_device_profile_by_default();
	}

	return err_code;
}





uint8_t EE_LoadDeviceProfile(uint8_t num)
{
	
}





/*
void EE_load_default_profile(void)
{
}

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






