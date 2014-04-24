
#include <stdio.h>
#include "converter.h"

//---------------------------------------------//
// Emulation of converter hardware

converter_state_t converter_state;







uint16_t Converter_GetVoltageSetting(uint8_t channel)
{
    return converter_state.set_voltage;
}

uint16_t Converter_GetVoltageAbsMax(uint8_t channel)
{
    return 20000;
}

uint16_t Converter_GetVoltageAbsMin(uint8_t channel)
{
    return 0;
}

uint16_t Converter_GetVoltageLimitSetting(uint8_t channel, uint8_t limit_type)
{
    return 0;
}

uint8_t Converter_GetVoltageLimitState(uint8_t channel, uint8_t limit_type)
{
    return 0;
}

uint16_t Converter_GetCurrentSetting(uint8_t channel, uint8_t range)
{
    return converter_state.set_current;
}

uint16_t Converter_GetCurrentAbsMax(uint8_t channel, uint8_t range)
{
    return 40000;
}

uint16_t Converter_GetCurrentAbsMin(uint8_t channel, uint8_t range)
{
    return 0;
}

uint16_t Converter_GetCurrentLimitSetting(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    return 0;
}

uint8_t Converter_GetCurrentLimitState(uint8_t channel, uint8_t range, uint8_t limit_type)
{
    return 0;
}

uint8_t Converter_GetOverloadProtectionState(void)
{
    return converter_state.overload_prot_en;
}

uint8_t Converter_GetOverloadProtectionWarning(void)
{
    return converter_state.overload_warn_en;
}

uint16_t Converter_GetOverloadProtectionThreshold(void)
{
    return converter_state.overload_threshold;
}

uint8_t Converter_GetCurrentRange(uint8_t channel)
{
    return converter_state.current_range;
}

uint8_t Converter_GetFeedbackChannel(void)
{
    return converter_state.channel;
}


int8_t Converter_GetVoltageDacOffset(void)
{
    return converter_state.vdac_offset;
}

int8_t Converter_GetCurrentDacOffset(uint8_t range)
{
    return converter_state.cdac_offset;
}

//---------------------------------------------//



