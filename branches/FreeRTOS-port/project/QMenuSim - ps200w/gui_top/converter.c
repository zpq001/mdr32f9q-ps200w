
#include <stdio.h>
#include "converter.h"

//---------------------------------------------//
//

uint16_t Converter_GetVoltageSetting(uint8_t channel)
{
    return 9000;
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
    return 5000;
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
    return 1;
}

uint8_t Converter_GetOverloadProtectionWarning(void)
{
    return 0;
}

uint16_t Converter_GetOverloadProtectionThreshold(void)
{
    return 5;
}

uint8_t Converter_GetCurrentRange(uint8_t channel)
{
    return 0;
}

uint8_t Converter_GetFeedbackChannel(void)
{
    return 0;
}


int8_t Converter_GetVoltageDacOffset(void)
{
    return 0;
}

int8_t Converter_GetCurrentDacOffset(uint8_t range)
{
    return 0;
}

//---------------------------------------------//



