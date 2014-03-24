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

#include "MDR32Fx.h" 
#include <string.h>		// usigng memset

#include "FreeRTOS.h"
#include "task.h"

#include "i2c_eeprom.h"
#include "eeprom.h"

#include "global_def.h"
#include "converter.h"
#include "dispatcher.h"


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
	// Fill whole settings structure with FF's (padding)
	// These values will be written to EEPROM and used for CRC
	memset(global_settings, 0xFFFFFFFF, sizeof(global_settings_t));
	global_settings->adc_voltage_offset = 0;
	global_settings->adc_current_offset = 0;
	global_settings->number_of_power_cycles = 0;
}

static void update_global_settings(void)
{
	// Gather configuration from all the modules
	global_settings->number_of_power_cycles++;
}

static void fill_device_profile_by_default(void)
{	
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
	
	// Overload
	device_profile->converter_profile.overload.protection_enable = 1;
	device_profile->converter_profile.overload.warning_enable = 0;
	device_profile->converter_profile.overload.threshold = (5*1);	// x 200us
	
	// Other fields
	// ...
}


static void update_device_profile(void)
{
	// Fill whole settings structure with FF's (padding)
	// These values will be written to EEPROM and used for CRC
	memset(device_profile, 0xFFFFFFFF, sizeof(device_profile_t));
	
	// 5V channel voltage
	device_profile->converter_profile.ch5v_voltage.setting = Converter_GetVoltageSetting(CHANNEL_5V);
	device_profile->converter_profile.ch5v_voltage.limit_low = Converter_GetVoltageLimitSetting(CHANNEL_5V, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_voltage.limit_high = Converter_GetVoltageLimitSetting(CHANNEL_5V, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_voltage.enable_low_limit = Converter_GetVoltageLimitState(CHANNEL_5V, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_voltage.enable_high_limit = Converter_GetVoltageLimitState(CHANNEL_5V, LIMIT_TYPE_HIGH);
	// Current
	device_profile->converter_profile.ch5v_current.low_range.setting = Converter_GetCurrentSetting(CHANNEL_5V, CURRENT_RANGE_LOW);
	device_profile->converter_profile.ch5v_current.low_range.limit_low = Converter_GetCurrentLimitSetting(CHANNEL_5V, CURRENT_RANGE_LOW, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_current.low_range.limit_high = Converter_GetCurrentLimitSetting(CHANNEL_5V, CURRENT_RANGE_LOW, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_current.low_range.enable_low_limit = Converter_GetCurrentLimitState(CHANNEL_5V, CURRENT_RANGE_LOW, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_current.low_range.enable_high_limit = Converter_GetCurrentLimitState(CHANNEL_5V, CURRENT_RANGE_LOW, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_current.high_range.setting = Converter_GetCurrentSetting(CHANNEL_5V, CURRENT_RANGE_HIGH);
	device_profile->converter_profile.ch5v_current.high_range.limit_low = Converter_GetCurrentLimitSetting(CHANNEL_5V, CURRENT_RANGE_HIGH, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_current.high_range.limit_high = Converter_GetCurrentLimitSetting(CHANNEL_5V, CURRENT_RANGE_HIGH, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_current.high_range.enable_low_limit = Converter_GetCurrentLimitState(CHANNEL_5V, CURRENT_RANGE_HIGH, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch5v_current.high_range.enable_high_limit = Converter_GetCurrentLimitState(CHANNEL_5V, CURRENT_RANGE_HIGH, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch5v_current.selected_range = Converter_GetCurrentRange(CHANNEL_5V);
	
	// 12V channel voltage
	device_profile->converter_profile.ch12v_voltage.setting = Converter_GetVoltageSetting(CHANNEL_12V);
	device_profile->converter_profile.ch12v_voltage.limit_low = Converter_GetVoltageLimitSetting(CHANNEL_12V, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_voltage.limit_high = Converter_GetVoltageLimitSetting(CHANNEL_12V, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_voltage.enable_low_limit = Converter_GetVoltageLimitState(CHANNEL_12V, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_voltage.enable_high_limit = Converter_GetVoltageLimitState(CHANNEL_12V, LIMIT_TYPE_HIGH);
	// Current
	device_profile->converter_profile.ch12v_current.low_range.setting = Converter_GetCurrentSetting(CHANNEL_12V, CURRENT_RANGE_LOW);
	device_profile->converter_profile.ch12v_current.low_range.limit_low = Converter_GetCurrentLimitSetting(CHANNEL_12V, CURRENT_RANGE_LOW, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_current.low_range.limit_high = Converter_GetCurrentLimitSetting(CHANNEL_12V, CURRENT_RANGE_LOW, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_current.low_range.enable_low_limit = Converter_GetCurrentLimitState(CHANNEL_12V, CURRENT_RANGE_LOW, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_current.low_range.enable_high_limit = Converter_GetCurrentLimitState(CHANNEL_12V, CURRENT_RANGE_LOW, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_current.high_range.setting = Converter_GetCurrentSetting(CHANNEL_12V, CURRENT_RANGE_HIGH);
	device_profile->converter_profile.ch12v_current.high_range.limit_low = Converter_GetCurrentLimitSetting(CHANNEL_12V, CURRENT_RANGE_HIGH, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_current.high_range.limit_high = Converter_GetCurrentLimitSetting(CHANNEL_12V, CURRENT_RANGE_HIGH, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_current.high_range.enable_low_limit = Converter_GetCurrentLimitState(CHANNEL_12V, CURRENT_RANGE_HIGH, LIMIT_TYPE_LOW);
	device_profile->converter_profile.ch12v_current.high_range.enable_high_limit = Converter_GetCurrentLimitState(CHANNEL_12V, CURRENT_RANGE_HIGH, LIMIT_TYPE_HIGH);
	device_profile->converter_profile.ch12v_current.selected_range = Converter_GetCurrentRange(CHANNEL_12V);
	
	// Overload
	device_profile->converter_profile.overload.protection_enable = Converter_GetOverloadProtectionState();
	device_profile->converter_profile.overload.warning_enable = Converter_GetOverloadProtectionWarning();
	device_profile->converter_profile.overload.threshold = Converter_GetOverloadProtectionThreshold();
	
	// Other fields
	// ...
}




//	Return: bits are set indicating errors:
//		EE_GSETTINGS_HW_ERROR if there was an hardware error for global settings
//  	EE_GSETTINGS_CRC_ERROR if there was CRC error for global settings
//		EE_PROFILE_HW_ERROR if there was an hardware error for recent profile
//		EE_PROFILE_CRC_ERROR if there was CRC error for recent profile
//		If specific bit is cleared, it means that that kind of error has not occured.
static uint8_t EE_InitialLoad(void)
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
	hw_result = EEPROM_ReadBlock(ee_addr, (uint8_t *)&global_settings_data, sizeof(global_settings_t));
	ee_addr = EE_GSETTINGS_BASE + EE_GSETTINGS_CRC_OFFSET;
	hw_result |= EEPROM_ReadBlock(ee_addr, (uint8_t *)&ee_crc, EE_CRC_SIZE);
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
		hw_result = EEPROM_ReadBlock(ee_addr, (uint8_t *)&device_profile_data, sizeof(device_profile_t));
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_NAME_OFFSET;
		hw_result |= EEPROM_ReadBlock(ee_addr, (uint8_t *)&device_profile_name, EE_PROFILE_NAME_SIZE);
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_CRC_OFFSET;
		hw_result |= EEPROM_ReadBlock(ee_addr, (uint8_t *)&ee_crc, EE_CRC_SIZE);
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
				profile_info[i] = EE_PROFILE_CRC_ERROR;
			}
		}
		else
		{
			// EEPROM hardware error. Cannot access device.
			profile_info[i] = EE_PROFILE_HW_ERROR;
		}
	}
	
	
	//-------------------------------//
	//	Read recent profile
	ee_addr = EE_RECENT_PROFILE_BASE;
	hw_result = EEPROM_ReadBlock(ee_addr, (uint8_t *)&device_profile_data, sizeof(device_profile_t));
	ee_addr = EE_RECENT_PROFILE_BASE + EE_PROFILE_CRC_OFFSET;
	hw_result |= EEPROM_ReadBlock(ee_addr, (uint8_t *)&ee_crc, EE_CRC_SIZE);
	if (hw_result == 0)
	{
		// Hardware EEPROM access OK. Check CRC.
		crc = get_crc16((uint8_t *)&device_profile_data, sizeof(device_profile_t), 0xFFFF);
		if (crc != ee_crc)
		{
			// Profile CRC is broken.
			err_code |= EE_PROFILE_CRC_ERROR;
		}
	}
	else
	{
		// EEPROM hardware error. Cannot access device.
		err_code |= EE_PROFILE_HW_ERROR;
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




uint8_t EE_SaveGlobalSettings(void)
{
	uint8_t err_code = EE_OK;
	uint8_t hw_result;
	uint16_t crc;
	
	update_global_settings();
	crc = get_crc16((uint8_t *)&global_settings_data, sizeof(global_settings_t), 0xFFFF);
	
	//-------------------------------//
	//	Write global settings
	hw_result = EEPROM_WriteBlock(EE_GSETTINGS_BASE, (uint8_t *)&global_settings_data, sizeof(global_settings_t));
	hw_result |= EEPROM_WriteBlock(EE_GSETTINGS_BASE + EE_GSETTINGS_CRC_OFFSET, (uint8_t *)&crc, EE_CRC_SIZE);
	if (hw_result != 0)
	{
		// EEPROM hardware error. Cannot access device.
		err_code = EE_GSETTINGS_HW_ERROR;
	}
	return err_code;
}



uint8_t EE_SaveRecentProfile(void)
{
	uint8_t err_code = EE_OK;
	uint8_t hw_result;
	uint16_t crc;
	
	update_device_profile();
	crc = get_crc16((uint8_t *)&device_profile_data, sizeof(device_profile_t), 0xFFFF);

	//-------------------------------//
	//	Write recent profile
	hw_result = EEPROM_WriteBlock(EE_RECENT_PROFILE_BASE, (uint8_t *)&device_profile_data, sizeof(device_profile_t));
	hw_result |= EEPROM_WriteBlock(EE_RECENT_PROFILE_BASE + EE_PROFILE_CRC_OFFSET, (uint8_t *)&crc, EE_CRC_SIZE);
	if (hw_result != 0)
	{
		// EEPROM hardware error. Cannot access device.
		err_code = EE_PROFILE_HW_ERROR;
	}
	return err_code;
}






//-------------------------------------------------------//
// Loads data from specified profile into device_profile_data structure
//
//
//-------------------------------------------------------//
static uint8_t EE_LoadDeviceProfile(uint8_t i)
{	
	uint8_t err_code;
	uint8_t hw_result;
	uint16_t crc;
	uint16_t ee_crc;
	uint16_t ee_addr;
	
	if (i < EE_PROFILES_COUNT) 
	{
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE);
		hw_result = EEPROM_ReadBlock(ee_addr, (uint8_t *)&device_profile_data, sizeof(device_profile_t));
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_NAME_OFFSET;
		hw_result |= EEPROM_ReadBlock(ee_addr, (uint8_t *)&device_profile_name, EE_PROFILE_NAME_SIZE);
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_CRC_OFFSET;
		hw_result |= EEPROM_ReadBlock(ee_addr, (uint8_t *)&ee_crc, EE_CRC_SIZE);
		if (hw_result == 0)
		{
			// Hardware EEPROM access OK. Check CRC.
			crc = get_crc16((uint8_t *)&device_profile_data, sizeof(device_profile_t), 0xFFFF);
			crc = get_crc16((uint8_t *)&device_profile_name, EE_PROFILE_NAME_SIZE, crc);
			if (crc == ee_crc)
			{
				// Profile CRC is OK.
				profile_info[i] = EE_PROFILE_VALID;
				err_code = EE_OK;
			}	
			else
			{
				// Profile CRC is broken.
				profile_info[i] = EE_PROFILE_CRC_ERROR;
				err_code = EE_PROFILE_CRC_ERROR;
			}
		}
		else
		{
			// EEPROM hardware error. Cannot access device.
			profile_info[i] = EE_PROFILE_HW_ERROR;
			err_code = EE_PROFILE_HW_ERROR;
		}
	}
	else
	{
		// Wrong profile number
		err_code = EE_WRONG_ARGUMENT;
	}	
	return err_code;
}


//-------------------------------------------------------//
//	Returns name of a profile
//	profile_info[i] must be filled correctly before calling.
//
//-------------------------------------------------------//
static uint8_t EE_GetProfileName(uint8_t i, char *str_to_fill)
{
	uint8_t err_code;
	uint8_t hw_result;
	uint16_t ee_addr;
	
	if (i < EE_PROFILES_COUNT) 
	{
		if (profile_info[i] == EE_PROFILE_VALID)
		{
			ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_NAME_OFFSET;
			hw_result = EEPROM_ReadBlock(ee_addr, (uint8_t *)str_to_fill, EE_PROFILE_NAME_SIZE);
			if (hw_result == 0)
			{
				// Hardware EEPROM access OK.
				err_code = EE_OK;
			}
			else
			{
				// EEPROM hardware error. Cannot access device.
				profile_info[i] = EE_PROFILE_HW_ERROR;
				err_code = EE_PROFILE_HW_ERROR;
			}
		}
		else if (profile_info[i] == EE_PROFILE_CRC_ERROR)
		{
			err_code = EE_PROFILE_CRC_ERROR;
		}
		else
		{
			err_code = EE_PROFILE_HW_ERROR;
		}
	}
	else
	{
		// Wrong profile number
		err_code = EE_WRONG_ARGUMENT;
	}	
	return err_code;
}



//-------------------------------------------------------//
// Saves profile data to EEPROM
//
//-------------------------------------------------------//
static uint8_t EE_SaveDeviceProfile(uint8_t i, char *name)
{	
	uint8_t err_code;
	uint8_t hw_result;
	uint16_t crc;
	uint16_t ee_addr;
	
	if (i < EE_PROFILES_COUNT) 
	{
		//update_device_profile(); - moved to task
		// Copy name
		strncpy(device_profile_name, name, EE_PROFILE_NAME_SIZE);
		
		crc = get_crc16((uint8_t *)&device_profile_data, sizeof(device_profile_t), 0xFFFF);
		crc = get_crc16((uint8_t *)&device_profile_name, EE_PROFILE_NAME_SIZE, crc);

		//-------------------------------//
		//	Write profile
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE);
		hw_result = EEPROM_WriteBlock(ee_addr, (uint8_t *)&device_profile_data, sizeof(device_profile_t));
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_NAME_OFFSET;
		hw_result |= EEPROM_WriteBlock(ee_addr, (uint8_t *)&device_profile_name, EE_PROFILE_NAME_SIZE);
		ee_addr = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_CRC_OFFSET;
		hw_result |= EEPROM_WriteBlock(ee_addr, (uint8_t *)&crc, EE_CRC_SIZE);
		
		if (hw_result == 0)
		{
			err_code = EE_OK;
			profile_info[i] = EE_PROFILE_VALID;
		}
		else
		{
			// EEPROM hardware error. Cannot access device.
			err_code = EE_PROFILE_HW_ERROR;
			profile_info[i] = EE_PROFILE_HW_ERROR;
		}
	}
	else
	{
		// Wrong profile number
		err_code = EE_WRONG_ARGUMENT;
	}
	return err_code;
}



uint8_t EE_GetProfileState(uint8_t i)
{
	uint8_t err_code;
	if (i < EE_PROFILES_COUNT) 
	{
		err_code = profile_info[i];
	}
	else
	{
		// Wrong profile number
		err_code = EE_WRONG_ARGUMENT;
	}
	return err_code;
}



/*
EEPROM settings:

	- save or not device global settings on POFF
	- save or not device profile on POFF

	- load global settings from EEPROM upon start
	- load recent profile or default settings upon start

*/






xQueueHandle xQueueEEPROM;

static eeprom_message_t msg;
static dispatch_msg_t dispatcher_msg;


void vTaskEEPROM(void *pvParameters) 
{
	// Initialize
	xQueueEEPROM = xQueueCreate( 10, sizeof( eeprom_message_t ) );		// Queue can contain 10 elements
	if( xQueueEEPROM == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	
	while(1)
	{
		xQueueReceive(xQueueEEPROM, &msg, portMAX_DELAY);
		switch (msg.type)
		{
			case EE_TASK_INITIAL_LOAD:
				*msg.initial_load.state = EE_InitialLoad();
				if (msg.xSemaphorePtr != 0)
				{
					// Confirm
					xSemaphoreGive(*msg.xSemaphorePtr);		
				}
				break;
				
			case EE_TASK_GET_PROFILE_NAME:
				*msg.profile_name_request.state = EE_GetProfileName(msg.profile_name_request.index, msg.profile_name_request.name);
				if (msg.xSemaphorePtr != 0)
				{
					// Confirm
					xSemaphoreGive(*msg.xSemaphorePtr);		
				}
				break;
			
			case EE_TASK_LOAD_PROFILE:
				dispatcher_msg.type = DISPATCHER_LOAD_PROFILE_RESPONSE;
				dispatcher_msg.profile_load_response.profileState = EE_GetProfileState(msg.profile_load_request.index);
				if (dispatcher_msg.profile_load_response.profileState == EE_PROFILE_VALID)
				{
					dispatcher_msg.profile_load_response.profileState = EE_LoadDeviceProfile(msg.profile_load_request.index);
				}
				// Confirm
				dispatcher_msg.profile_load_response.index = msg.profile_load_request.index;	// return passed index
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);		
				break;
			
			case EE_TASK_SAVE_PROFILE:
				// Gather system state
				update_device_profile();
				if (msg.xSemaphorePtr != 0)
				{
					// Confirm
					xSemaphoreGive(*msg.xSemaphorePtr);		
				}
				// Save to EEPROM device
				dispatcher_msg.type = DISPATCHER_SAVE_PROFILE_RESPONSE;
				dispatcher_msg.profile_save_response.profileState = EE_SaveDeviceProfile(msg.profile_save_request.index, msg.profile_save_request.newName);
				dispatcher_msg.profile_save_response.index = msg.profile_save_request.index;	// return passed index
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
				break;
		}
	}
}




