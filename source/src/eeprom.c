/***************************************************************//**
	@brief i2c_eeprom
	
	Top-level functions for serial 24LC16B EEPROM device.

	24LC16B is 16K bit = 2048 bytes

	Data at EE_GSETTINGS_BASE stores general device information
	Device profiles start at address EE_PROFILES_BASE
	Each profile occupies EE_PROFILE_SIZE bytes.
	First profile is recent - it is saved on power off and used
	for restore on power on.
	
********************************************************************/

#include "MDR32Fx.h" 
#include "MDR32F9Qx_uart.h"	// definitions
#include <string.h>			// using memset

#include "FreeRTOS.h"
#include "task.h"

#include "i2c_eeprom.h"
#include "eeprom.h"

#include "global_def.h"
#include "converter.h"
#include "dispatcher.h"

/// Global settings 
static global_settings_t global_settings_data;
global_settings_t * global_settings = &global_settings_data;



/// Device profile
static device_profile_t device_profile_data;		
device_profile_t *device_profile = &device_profile_data;
/// Profile name
static char device_profile_name[EE_PROFILE_NAME_SIZE];
/// Array that holds profile states
uint8_t profile_info[EE_PROFILES_COUNT];


//-------------------------------------------------------//
/// @brief Function for single byte CRC update
/// @param[in] crc - initial CRC
/// @param[in] a - data byte
/// @return	updated CRC
//-------------------------------------------------------//
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

//-------------------------------------------------------//
/// @brief Function for data block CRC update
/// @param[in] data - pointer to data
/// @param[in] size - byte count
/// @param[in] seed - initial seed for CRC
/// @return updated CRC
//-------------------------------------------------------//
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
	global_settings->dac_voltage_offset = 0;
	global_settings->dac_current_low_offset = 0;
	global_settings->dac_current_high_offset = 0;
	global_settings->number_of_power_cycles = 0;
	global_settings->restoreRecentProfile = 1;
	global_settings->saveRecentProfile = 1;
	global_settings->dac_voltage_offset = 0;
	global_settings->dac_current_low_offset = 0;
	global_settings->dac_current_high_offset = 0;
	global_settings->uart1.baudRate = 115200;
	global_settings->uart1.enable = 1;
	global_settings->uart1.parity = UART_Parity_No;
	global_settings->uart2.baudRate = 115200;
	global_settings->uart2.enable = 1;
	global_settings->uart2.parity = UART_Parity_No;
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
	device_profile->converter_profile.overload.threshold = (10*1);	// x100us
	
	// Buttons
	device_profile->buttons_profile.ext_switch_enable = 0;
	device_profile->buttons_profile.ext_switch_inverse = 0;
	device_profile->buttons_profile.ext_switch_mode = EXTSW_DIRECT;
	
	// Other fields
	// ...
}





//=================================================================//
//	Load routines (from EEPROM)
//=================================================================//




/**
@brief	Reads global settings from EEPROM device 
			into global_settings_data structure
@return EE_OK - if operation has completed correctly
@return	EE_GSETTINGS_CRC_ERROR - if EEPROM stores incorrect data
@return	EE_GSETTINGS_HW_ERROR - if there was hardware error
@note	Caller should take care for data atomic access
*/
static uint8_t EE_LoadGlobalSettings(void)
{
	uint8_t err_code = EE_OK;
	uint8_t hw_result;
	uint16_t crc;
	uint16_t ee_crc;
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
	return err_code;
}


/**
@brief	Reads recent profile data from EEPROM device 
			into device_profile_data structure
@return EE_OK - if operation has completed correctly
@return	EE_PROFILE_CRC_ERROR - if EEPROM stores incorrect data
@return	EE_PROFILE_HW_ERROR - if there was hardware error
@note	Caller should take care for data atomic access.
			Profile name for recent profile is ignored and not read.
*/
static uint8_t EE_LoadRecentProfile(void)
{
	uint8_t err_code = EE_OK;
	uint8_t hw_result;
	uint16_t crc;
	uint16_t ee_crc;
	uint16_t ee_addr;
	
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
	return err_code;
}


/**
@brief	Reads specified profile data from EEPROM device 
			into device_profile_data structure. 
@details	Profile name is read to device_profile_name char array.
@param[in] i - index of profile to load.
@return EE_OK - if operation has completed correctly
@return	EE_PROFILE_CRC_ERROR - if EEPROM stores incorrect data
@return	EE_PROFILE_HW_ERROR - if there was hardware error
@return EE_WRONG_ARGUMENT - if index is illegal
@note	Caller should take care for data atomic access.
		Profile state store in profile_info[i] is also updated by this function
*/
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



/**
@brief	Function reads all profiles stored in EEPROM to check which are valid (except recent profile).
@return none - results are saved in profile_info[] array
@note Function utilizes global_settings_data and device_profile_name as temporary buffers.
	Profile state profile_info[i] get folowing values:
	EE_PROFILE_VALID - if profile data is valid,
	EE_PROFILE_CRC_ERROR - if profile data CRC is broken,
	EE_PROFILE_HW_ERROR - if there was hardware error.
	
	TODO: check if this function can be replaced with simple for-looop where EE_LoadDeviceProfile() is called
*/
static void EE_ExamineProfiles(void)
{
	uint8_t hw_result;
	uint16_t crc;
	uint16_t ee_crc;
	uint16_t ee_addr;
	uint8_t i;
	
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
}




/**
@brief	Reads specified profile name from EEPROM device
@param[in] i - index of profile to load.
@param[in] str_to_fill - destination string. Must have length EE_PROFILE_NAME_SIZE or more.
@return EE_OK - if operation has completed correctly
@return	EE_PROFILE_CRC_ERROR - if EEPROM stores incorrect data
@return	EE_PROFILE_HW_ERROR - if there was hardware error
@return EE_WRONG_ARGUMENT - if index is illegal.
@note	Function does not check CRC. Instead, profile_info[i] is first analyzed. 
		Function will try to read name string only if i-th profile state is EE_PROFILE_VALID.
		Profile state stored in profile_info[i] is also updated by this function (in case of EEPROM hardware error)
*/
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



//=================================================================//
//	Saving routines (to EEPROM)
//=================================================================//



//-------------------------------------------------------//
// Saves global settings to EEPROM
//
//-------------------------------------------------------//
uint8_t EE_SaveGlobalSettings(void)
{
	uint8_t err_code = EE_OK;
	uint8_t hw_result;
	uint16_t crc;
	
	global_settings->number_of_power_cycles++;
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


//-------------------------------------------------------//
// Saves recent profile to EEPROM
//
//-------------------------------------------------------//
uint8_t EE_SaveRecentProfile(void)
{
	uint8_t err_code = EE_OK;
	uint8_t hw_result;
	uint16_t crc;
	
	if (global_settings->saveRecentProfile)
	{
		// Structure device_profile_data must be already prepared
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
	}
	else
	{
		// Profile save is NOT required
		err_code = EE_NOT_REQUIRED;
	}
	return err_code;
}




//-------------------------------------------------------//
// Saves profile to EEPROM
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
		// Device profile data structure must be filled before saving by
		// calling update_device_profile();
		
		// Fill temporary buffer with \0
		memset(device_profile_name, 0, sizeof(device_profile_name));
		// Copy name (first EE_PROFILE_NAME_SIZE - 1 characters or until \0 is found in source name)
		// Profile name always contains teminating \0
		strncpy(device_profile_name, name, EE_PROFILE_NAME_SIZE - 1);
		
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





//=================================================================//
//	Interface routines 
//=================================================================//



void EE_GetReadyForProfileSave(void)
{
	// Fill whole settings structure with FF's (padding)
	// These values will be written to EEPROM and used for CRC
	memset(device_profile, 0xFFFFFFFF, sizeof(device_profile_t));	
}

void EE_GetReadyForSystemInit(void)
{
	fill_global_settings_by_default();
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


uint8_t EE_IsRecentProfileSavingEnabled(void)
{
	return global_settings->saveRecentProfile;
}

uint8_t EE_IsRecentProfileRestoreEnabled(void)
{
	return global_settings->restoreRecentProfile;
}

static void EE_ApplyProfileSettings(uint8_t saveRecentProfile, uint8_t restoreRecentProfile)
{
	if ((restoreRecentProfile == 0) && (saveRecentProfile == 1))
	{
		// Saving profile without restoring is useless
		saveRecentProfile = 0;
	} 
	global_settings->saveRecentProfile = saveRecentProfile;
	global_settings->restoreRecentProfile = restoreRecentProfile;
}



/*
EEPROM settings:

	- save or not device global settings on POFF		<- always enabled
	- save or not device profile on POFF

	- load global settings from EEPROM upon start		<- always enabled
	- load recent profile or default settings upon start

*/






xQueueHandle xQueueEEPROM;

static eeprom_message_t msg;
static dispatch_msg_t dispatcher_msg;



void vTaskEEPROM(void *pvParameters) 
{
	uint8_t state;
	
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
				// Always restore global settings
				state = EE_LoadGlobalSettings();
				if (state & (EE_GSETTINGS_HW_ERROR | EE_GSETTINGS_CRC_ERROR))
				{
					// Loading default settings may be skipped if EE_GetReadyForSystemInit() had been called earlier
					fill_global_settings_by_default();
				}
				// The following function uses profile data structure, so call it before
				// restoring recent profile
				EE_ExamineProfiles();
				// Restore recent profile
				if (global_settings->restoreRecentProfile)
				{
					state |= EE_LoadRecentProfile();
					if (state & (EE_PROFILE_HW_ERROR | EE_PROFILE_CRC_ERROR))
					{
						fill_device_profile_by_default();
					}
				}
				else
				{
					// Profile restore is not required. Use defaults.
					fill_device_profile_by_default();
				}
				// Provide feedback
				*msg.initial_load.state = state;
				// Confirm operation
				if (msg.xSemaphorePtr != 0)
					xSemaphoreGive(*msg.xSemaphorePtr);	
				break;
				
			case EE_TASK_SHUTDOWN_SAVE:
				// Note - profile data structure and global settings mut NOT be modified during operation
				*msg.shutdown_save_result.global_settings_errcode = EE_SaveGlobalSettings();
				*msg.shutdown_save_result.recent_profile_errcode = EE_SaveRecentProfile();
				// Confirm operation
				if (msg.xSemaphorePtr != 0)
					xSemaphoreGive(*msg.xSemaphorePtr);	
				break;
				
			case EE_TASK_GET_PROFILE_NAME:
				*msg.profile_name_request.state = EE_GetProfileName(msg.profile_name_request.index, msg.profile_name_request.name);
				// Confirm operation
				if (msg.xSemaphorePtr != 0)
					xSemaphoreGive(*msg.xSemaphorePtr);	
				break;
			
			case EE_TASK_LOAD_PROFILE:
				// Load from EEPROM device
				state = EE_GetProfileState(msg.profile_load_request.index);
				if (state == EE_PROFILE_VALID)
				{
					state = EE_LoadDeviceProfile(msg.profile_load_request.index);
				}
				// Confirm operation
				if (msg.xSemaphorePtr != 0)
					xSemaphoreGive(*msg.xSemaphorePtr);		
				// Notify dispatcher
				dispatcher_msg.type = DISPATCHER_LOAD_PROFILE_RESPONSE;
				dispatcher_msg.profile_load_response.index = msg.profile_load_request.index;	// return passed index
				dispatcher_msg.profile_load_response.profileState = state;
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);		
				break;
			
			case EE_TASK_SAVE_PROFILE:
				// Save to EEPROM device
				state = EE_SaveDeviceProfile(msg.profile_save_request.index, msg.profile_save_request.newName);
				// Confirm operation
				if (msg.xSemaphorePtr != 0)
					xSemaphoreGive(*msg.xSemaphorePtr);	
				// Notify dispatcher
				dispatcher_msg.type = DISPATCHER_SAVE_PROFILE_RESPONSE;
				dispatcher_msg.profile_save_response.profileState = state;
				dispatcher_msg.profile_save_response.index = msg.profile_save_request.index;	// return passed index
				xQueueSendToBack(xQueueDispatcher, &dispatcher_msg, portMAX_DELAY);	
				break;
			
			case EE_TASK_PROFILE_SETTINGS:
				taskENTER_CRITICAL();
				EE_ApplyProfileSettings(msg.profile_settings.saveRecentProfile, msg.profile_settings.restoreRecentProfile);
				taskEXIT_CRITICAL();
				// Confirm operation
				if (msg.xSemaphorePtr != 0)
					xSemaphoreGive(*msg.xSemaphorePtr);
				break;
		}
	}
}




