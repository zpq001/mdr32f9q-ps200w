
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*
    EEPROM profile record structure (every record has size EE_PROFILE_SIZE in bytes):
	
    ------------------------    <- i-th profile record offset = A
    |                      |    <- A + 0
    |   profile data       |
    |                      |    <- A + sizeof(device_profile_t) - 1
    ------------------------
    |   some free space    |    <- A + sizeof(device_profile_t)
    |                      |
    ------------------------
    |     name string      |    <- A + EE_PROFILE_NAME_OFFSET
    |                      |
    ------------------------
    |        CRC           |    <- A + EE_CRC_OFFSET
    |                      |
    ------------------------

*/



//---------------------------------------------//
// EEPROM stores global settings + 1 recent profile + few user profiles
// All definitions in bytes

#define EE_PROFILES_COUNT				10		

#define EE_GSETTNGS_SIZE				32
#define EE_PROFILE_SIZE					128		// total: data + name + crc + some free space
#define EE_PROFILE_NAME_SIZE			13		// 12 chars + \0
#define EE_CRC_SIZE						2		// using 16-bit simple CRC

#define EE_GSETTINGS_BASE				0
#define EE_RECENT_PROFILE_BASE			32
#define EE_PROFILES_BASE				(EE_RECENT_PROFILE_BASE + EE_PROFILE_SIZE)

#define EE_GSETTINGS_CRC_OFFSET			(EE_GSETTNGS_SIZE - EE_CRC_SIZE)

#define EE_PROFILE_NAME_OFFSET		(EE_PROFILE_SIZE - EE_CRC_SIZE - EE_PROFILE_NAME_SIZE)
#define EE_PROFILE_CRC_OFFSET		(EE_PROFILE_SIZE - EE_CRC_SIZE)

/* 
For example, accessing name of 5-th profile would be like this:
	i = 5;
	char name[EE_PROFILE_NAME_SIZE];
	ee_address = EE_PROFILES_BASE + (i * EE_PROFILE_SIZE) + EE_PROFILE_NAME_OFFSET;
	read_eeprom(ee_address, name, EE_PROFILE_NAME_SIZE);
	
Note:
    sizeof(device_profile_t) must be <= (EE_PROFILE_SIZE - EE_CRC_SIZE - EE_PROFILE_NAME_SIZE)
*/

// EEPROM function status, also EEPROM profile status
// can be ORed
#define EE_OK						0
#define EE_PROFILE_VALID			0		// Alias for EEPROM profile status
#define EE_GSETTINGS_HW_ERROR		(1<<0)
#define EE_GSETTINGS_CRC_ERROR		(1<<1)	
#define EE_PROFILE_HW_ERROR			(1<<2)
#define EE_PROFILE_CRC_ERROR		(1<<3)
#define EE_WRONG_ARGUMENT			0x10
#define EE_NOT_REQUIRED				0x11




typedef struct {
	uint16_t setting;
	uint16_t limit_low;
	uint16_t limit_high;
	uint8_t enable_low_limit;
	uint8_t enable_high_limit;
} reg_profile_t;

typedef struct {
	reg_profile_t low_range;
	reg_profile_t high_range;
	uint8_t selected_range;
} current_profile_t;

typedef struct {
	uint8_t protection_enable;
	uint8_t warning_enable;
	uint16_t threshold;
} overload_profile_t;



typedef struct {
	reg_profile_t ch5v_voltage;
	reg_profile_t ch12v_voltage;
	current_profile_t ch5v_current;
	current_profile_t ch12v_current;
	overload_profile_t overload;
} converter_profile_t;

typedef struct {
	uint8_t ext_switch_enable;
	uint8_t ext_switch_inverse;
	uint8_t ext_switch_mode;
} buttons_profile_t;


typedef struct {
	converter_profile_t converter_profile;
	buttons_profile_t buttons_profile;
	// other fileds, charging, etc

} device_profile_t;

typedef struct {
	uint8_t enable;
	uint8_t parity;
	uint32_t baudRate;
} uartx_settings_t;



typedef struct {
	uint32_t number_of_power_cycles;
	int8_t dac_voltage_offset;
	int8_t dac_current_low_offset;
	int8_t dac_current_high_offset;
	uint8_t saveRecentProfile;
	uint8_t restoreRecentProfile;
	uartx_settings_t uart1;
	uartx_settings_t uart2;
} global_settings_t;



//---------------------------------------------//

enum {
	EE_TASK_INITIAL_LOAD,
	EE_TASK_SHUTDOWN_SAVE,
	EE_TASK_GET_PROFILE_NAME,
	EE_TASK_LOAD_PROFILE,
	EE_TASK_SAVE_PROFILE,
	EE_TASK_PROFILE_SETTINGS
};



typedef struct {
	uint8_t type;
	uint8_t sender;
	xSemaphoreHandle *xSemaphorePtr;
	union {
		struct {
			uint8_t *state;
		} initial_load;
		struct {
			uint8_t index;
			uint8_t *state;
			char *name;
		} profile_name_request;
		struct {
			uint8_t index;
        } profile_load_request;
		struct {
			uint8_t index;
			char *newName;
        } profile_save_request;
		struct {
			uint8_t saveRecentProfile;
			uint8_t restoreRecentProfile;
		} profile_settings;
		struct {
			uint8_t *global_settings_errcode;
			uint8_t *recent_profile_errcode;
		} shutdown_save_result;
	};
} eeprom_message_t;



extern xQueueHandle xQueueEEPROM;

extern global_settings_t *global_settings;
extern device_profile_t *device_profile;

// These two are special functions for calling from routine which
// services power-down sequence
uint8_t EE_SaveGlobalSettings(void);
uint8_t EE_SaveRecentProfile(void);

void EE_GetReadyForProfileSave(void);
void EE_GetReadyForSystemInit(void);

// General functions
uint8_t EE_GetProfileState(uint8_t i);
uint8_t EE_IsRecentProfileSavingEnabled(void);
uint8_t EE_IsRecentProfileRestoreEnabled(void);

void vTaskEEPROM(void *pvParameters);


