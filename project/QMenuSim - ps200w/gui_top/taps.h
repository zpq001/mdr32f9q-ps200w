//

#include <stdint.h>


#define taskENTER_CRITICAL() ;
#define taskEXIT_CRITICAL() ;


typedef struct {
    uint16_t set_voltage;       // [mV]
    uint16_t set_current;       // [mA]
    uint32_t power_adc;			// [mW]
    uint8_t overload_prot_en;
    uint8_t overload_warn_en;
    uint16_t overload_threshold;
    uint8_t channel;
    uint8_t current_range;
    int16_t converter_temp_celsius;
    int8_t vdac_offset;
    int8_t cdac_offset;
} converter_state_t;


uint16_t Converter_GetVoltageSetting(uint8_t channel);
uint16_t Converter_GetVoltageAbsMax(uint8_t channel);
uint16_t Converter_GetVoltageAbsMin(uint8_t channel);
uint16_t Converter_GetVoltageLimitSetting(uint8_t channel, uint8_t limit_type);
uint8_t Converter_GetVoltageLimitState(uint8_t channel, uint8_t limit_type);
uint16_t Converter_GetCurrentSetting(uint8_t channel, uint8_t range);
uint16_t Converter_GetCurrentAbsMax(uint8_t channel, uint8_t range);
uint16_t Converter_GetCurrentAbsMin(uint8_t channel, uint8_t range);
uint16_t Converter_GetCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type);
uint8_t Converter_GetCurrentLimitState(uint8_t channel, uint8_t range, uint8_t limit_type);
uint8_t Converter_GetOverloadProtectionState(void);
uint8_t Converter_GetOverloadProtectionWarning(void);
uint16_t Converter_GetOverloadProtectionThreshold(void);
uint8_t Converter_GetCurrentRange(uint8_t channel);
uint8_t Converter_GetFeedbackChannel(void);
int8_t Converter_GetVoltageDacOffset(void);
int8_t Converter_GetCurrentDacOffset(uint8_t range);





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


// EEPROM function status, also EEPROM profile status
// can be ORed
#define EE_OK						0
#define EE_PROFILE_VALID			0		// Alias for EEPROM profile status
#define EE_GSETTINGS_HW_ERROR		(1<<0)
#define EE_GSETTINGS_CRC_ERROR		(1<<1)
#define EE_PROFILE_HW_ERROR			(1<<2)
#define EE_PROFILE_CRC_ERROR		(1<<3)
#define EE_WRONG_ARGUMENT			0x10


uint8_t EE_GetProfileState(uint8_t i);
uint8_t EE_IsRecentProfileSavingEnabled(void);
uint8_t EE_IsRecentProfileRestoreEnabled(void);





uint8_t BTN_IsExtSwitchEnabled(void);
uint8_t BTN_GetExtSwitchInversion(void);
uint8_t BTN_GetExtSwitchMode(void);


uint16_t ADC_GetVoltage(void);
uint16_t ADC_GetCurrent(void);
uint32_t ADC_GetPower(void);

int16_t Service_GetTemperature(void);



