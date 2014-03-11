
#include <stdint.h>

//---------------------------------------------//
// All definitions in bytes

#define EEPROM_GLOBAL_SETTINGS_OFFSET	0
#define EEPROM_GLOBAL_SETTINGS_SIZE		16

#define EEPROM_PROFILES_COUNT			10
#define EEPROM_PROFILE_NAME_LENGTH		13	// 12 chars + \0

#define EEPROM_PROFILE_SIZE				128
#define EEPROM_RECENT_PROFILE_OFFSET	16
#define EEPROM_PROFILE_OFFSET			(128 + 16)


// EEPROM function status - CHECKME
#define EEPROM_OK				0
#define EEPROM_HW_ERROR			1
#define EEPROM_CRC_ERROR		2
#define EEPROM_WRONG_ARGUMENT	3


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
	reg_profile_t ch5v_voltage;
	reg_profile_t ch12v_voltage;
	current_profile_t ch5v_current;
	current_profile_t ch12v_current;
	char name[EEPROM_PROFILE_NAME_LENGTH];
} converter_profile_t;



typedef struct {
	converter_profile_t converter_profile;
	// other fileds, uart, etc

} device_profile_t;

// sizeof(device_profile_t) must be <= (EEPROM_PROFILE_SIZE - 2) - there are two bytes for CRC



typedef struct {
	uint32_t number_of_power_cycles;
	int8_t adc_voltage_offset;
	int8_t adc_current_offset;
	uint8_t recent_profile_valid;
	uint16_t valid_profiles;
} global_settings_t;



// Wrapper + CRC
typedef struct {
	device_profile_t device_profile;
	uint16_t crc;
} device_profile_record_t;

// Wrapper + CRC
typedef struct {
	global_settings_t global_settings;
	uint16_t crc;
} global_settings_record_t;



