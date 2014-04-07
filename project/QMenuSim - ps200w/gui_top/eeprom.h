
#include <stdint.h>

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

#define EE_GSETTNGS_SIZE				16
#define EE_PROFILE_SIZE					128		// total: data + name + crc + some free space
#define EE_PROFILE_NAME_SIZE			13		// 10 chars + \0
#define EE_CRC_SIZE						2		// using 16-bit simple CRC

#define EE_GSETTINGS_BASE				0
#define EE_RECENT_PROFILE_BASE			16
#define EE_PROFILES_BASE				(128 + 16)

#define EE_GSETTINGS_CRC_OFFSET			14

#define EE_PROFILE_NAME_OFFSET		(EE_PROFILE_SIZE - EE_CRC_SIZE - EE_PROFILE_NAME_SIZE - 1)
#define EE_PROFILE_CRC_OFFSET		(EE_PROFILE_SIZE - EE_CRC_SIZE - 1)

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

//#define EEPROM_HW_ERROR			1
//#define EEPROM_CRC_ERROR		2
//#define EEPROM_WRONG_ARGUMENT	3






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
	converter_profile_t converter_profile;
	// other fileds, charging, etc

} device_profile_t;





typedef struct {
	uint32_t number_of_power_cycles;
	int8_t adc_voltage_offset;
	int8_t adc_current_offset;
} global_settings_t;



//---------------------------------------------//


extern global_settings_t * global_settings;
extern device_profile_t *device_profile;
//extern char device_profile_name[];
extern uint8_t profile_info[];

uint8_t EE_GetProfileState(uint8_t i);
uint8_t EE_IsRecentProfileSavingEnabled(void);
uint8_t EE_IsRecentProfileRestoreEnabled(void);

