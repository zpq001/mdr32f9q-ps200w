#ifndef __CONVERTER_TASK_DEF_
#define __CONVERTER_TASK_DEF_

#include <stdint.h>


typedef union 
{
		struct 
		{
			uint8_t command;
		} state_set;
		struct 
		{
			uint8_t channel;
			int32_t value;
		} v_set;
        struct 
		{
			uint8_t channel;
            uint8_t type;
            uint8_t enable;
            int32_t value;
        } vlim_set;
		struct 
		{
			uint8_t channel;
			uint8_t range;
			int32_t value;
		} c_set;
		struct 
		{
			uint8_t channel;		// channel to affect
			uint8_t range;			// 20A (low) or 40A (high)
			uint8_t type;			// min or max limit
			uint8_t enable;			// enable/disable limit check
			int32_t value;			// new value
		} clim_set;
		struct 
		{
			uint8_t channel;
			uint8_t new_range;
		} crange_set;
		struct 
		{
			uint8_t protection_enable;
            uint8_t warning_enable;
            int32_t threshold;
        } overload_set;
		struct 
		{
			uint8_t new_channel;
		} ch_set;
		struct 
		{
			uint8_t voltage_offset;
            uint8_t current_low_offset;
            uint8_t current_high_offset;
        } dac_set;
} converter_arguments_t;






#endif
