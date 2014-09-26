#ifndef __CONVERTER_TASK_DEF_
#define __CONVERTER_TASK_DEF_

#include <stdint.h>





typedef union 
{
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





//-------------------------------------------------------//

//#define CONFIRMATION_IF

#ifdef CONFIRMATION_IF

typedef union {
	struct {
		uint8_t channel;
		int32_t value;
		struct {
			int32_t vset;
			uint8_t at_max: 1;
			uint8_t at_min: 1;
			uint8_t changed: 1;
			uint8_t err_code;
		} result;
	} voltage_setting;
	struct {
		uint8_t channel;
		uint8_t current_range;
		int32_t value;
		struct {
			int32_t cset;
			uint8_t at_max: 1;
			uint8_t at_min: 1;
			uint8_t err_code;
		} result;
	} current_setting;
} converter_cmd_args_t;


typedef struct {
	uint8_t command_type;
	converter_cmd_args_t ca;
} converter_command_t;








#endif
