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

#include "i2c_eeprom.h"
#include "eeprom.h"

#include "global_def.h"
#include "converter.h"


static device_profile_record_t device_profile_record;		// device_profile + crc
device_profile_t *device_profile = &device_profile_record.device_profile;

static global_settings_record_t global_settings_record;
global_settings_t * global_settings = &global_settings_record.global_settings;


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

static uint16_t get_crc16(uint8_t *data, uint16_t size)
{
	uint16_t crc = 0xFFFF;
	while(size--)
		crc = crc16_update(crc, *data++);
	return crc;
}
	

static void fill_global_settings_by_default(void)
{
	global_settings->adc_voltage_offset = 0;
	global_settings->adc_current_offset = 0;
	global_settings->number_of_power_cycles = 0;
	global_settings->recent_profile_valid = 0;
	global_settings->valid_profiles = 0;
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




uint8_t EEPROM_LoadGlobalSettings(void)
{
	uint8_t hw_result;
	uint16_t crc;
	hw_result = EEPROM_ReadBlock(EEPROM_GLOBAL_SETTINGS_OFFSET, (uint8_t *)&global_settings_record, sizeof(global_settings_record_t));
	if (hw_result == 0)
	{
		// Hardware EEPROM access OK. Check CRC.
		crc = get_crc16((uint8_t *)&global_settings_record.global_settings, sizeof(global_settings_t));
		if (crc == global_settings_record.crc)
		{
			// EEPROM data is correct
			return EEPROM_OK;
		}
	}
	
	// EEPROM data is corrupted or EEPROM read error.
	// Fill structure with defaults			
	fill_global_settings_by_default();
	return (hw_result != 0) ? EEPROM_HW_ERROR : EEPROM_CRC_ERROR;
}


uint8_t EEPROM_LoadDeviceProfile(uint8_t num)
{
	
}





/*
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
*/






