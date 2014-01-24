/********************************************************************
	Module		Converter
	
	Purpose:
		
					
	Globals used:

		
	Operation: 
	
 ___________________________________
|          CONVERTER TASK           |       <- top layer
|___________________________________|          provides interface to other system components
                  |
                  v
 ___________________________________
|   Converter HW control funtions   |       <- middle layer
|___________________________________|		   stores settings of voltage, current, overload
                  |
                  v
 ___________________________________
|   ISR - based low level control   |       <- low layer
|___________________________________|	


********************************************************************/



//---------------------------------------------//
// Converter_HWProcess

// state_HWProcess bits 
#define STATE_HW_ON				0x01
#define STATE_HW_OFF			0x02
#define STATE_HW_OVERLOADED		0x04
#define STATE_HW_OFF_BY_ADC		0x08
#define STATE_HW_TIMER_NOT_EXPIRED	0x10
#define STATE_HW_USER_TIMER_EXPIRED	0x20


// Analog regulation apply function arguments
#define APPLY_VOLTAGE	0x01
#define APPLY_CURRENT	0x02








volatile uint8_t state_HWProcess = STATE_HW_OFF;	
volatile uint8_t ctrl_HWProcess = 0;
// Globals used for communicating with converter control task called from ISR
volatile uint8_t cmd_ADC_to_HWProcess = 0;




converter_regulation_t reg_channel_5v;
converter_regulation_t reg_channel_12v;
converter_regulation_t *converter_regulation;




//---------------------------------------------------------------------------//
//	Applies operating channel voltage and current settings
//---------------------------------------------------------------------------//
static void ApplyAnalogRegulation(uint8_t code)
{
	uint16_t temp;
	
	if (code & APPLY_VOLTAGE)
	{
		temp = converter_regulation->voltage.setting;
		temp /= 5;
		SetVoltagePWMPeriod(temp);		// FIXME - we are setting not period but duty
	}
	if (code & APPLY_CURRENT)
	{
		temp = converter_regulation->current->setting;
		temp = (converter_regulation->current->RANGE == CURRENT_RANGE_HIGH) ? temp / 2 : temp;
		temp /= 5;
		SetCurrentPWMPeriod(temp);		// FIXME - we are setting not period but duty
	}
	
	/*
static void apply_regulation(void)
{
	uint16_t temp;
	
	// Apply voltage - same for both 5V and 12V channels
	temp = converter_regulation->voltage.setting;
	temp /= 5;
	SetVoltagePWMPeriod(temp);		// FIXME - we are setting not period but duty
	
	// Apply current different for 20A and 40A limits
	temp = converter_regulation->current->setting;
	temp = (converter_regulation->current->RANGE == CURRENT_RANGE_HIGH) ? temp / 2 : temp;
	temp /= 5;
	SetCurrentPWMPeriod(temp);		// FIXME - we are setting not period but duty
}
*/
}



//---------------------------------------------------------------------------//
//	Checks and sets new value in regultaion structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationValue(reg_setting_t *s, int32_t new_value, uint8_t *errCode)
{
	uint8_t result = 0;
	*errCode = VALUE_OK;
	// Check software limits
	if (new_value <= (int32_t)s->limit_low)
	{	
		new_value = (int32_t)s->limit_low;
		if (errCode) *errCode = VALUE_BOUND_BY_SOFT_MIN;
	}
	if (new_value >= (int32_t)s->limit_high) 
	{
		new_value = (int32_t)s->limit_high;
		if (errCode) *errCode = VALUE_BOUND_BY_SOFT_MAX;
	} 
	// Check absolute limits
	if (new_value <= (int32_t)s->MINIMUM)
	{	
		new_value = (int32_t)s->MINIMUM;
		if (errCode) *errCode = VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->MAXIMUM) 
	{
		new_value = (int32_t)s->MAXIMUM;
		if (errCode) *errCode = VALUE_BOUND_BY_ABS_MAX;
	} 
	if (s->setting != new_value)
	{
		s->setting = new_value;
		result |= VALUE_SET_NEW;
	}
	return result;
}

//---------------------------------------------------------------------------//
//	Checks and sets new value limit in regultaion structure
//---------------------------------------------------------------------------//
static uint8_t SetRegulationLimit(reg_setting_t *s, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *errCode)
{
	uint8_t result = 0;
	*errCode = VALUE_OK;
	if (new_value <= (int32_t)s->LIMIT_MIN)
	{	
		new_value = (int32_t)s->LIMIT_MIN;
		if (errCode) *errCode = VALUE_BOUND_BY_ABS_MIN;
	}
	if (new_value >= (int32_t)s->LIMIT_MAX) 
	{
		new_value = (int32_t)s->LIMIT_MAX;
		if (errCode) *errCode = VALUE_BOUND_BY_ABS_MAX;
	} 
	if (limit_type == LIMIT_TYPE_LOW)
	{
		if ((s->limit_low != new_value) || (s->enable_low_limit != enable))
		{
			s->limit_low = new_value;
			s->enable_low_limit = enable;
			result |= VALUE_SET_NEW;
		}
	}
	else if (limit_type == LIMIT_TYPE_HIGH)
	{
		if ((s->limit_high != new_value) || (s->enable_high_limit != enable))
		{
			s->limit_high = new_value;
			s->enable_high_limit = enable;
			result |= VALUE_SET_NEW;
		}
	}
	return result;
}



//---------------------------------------------------------------------------//
// 	Set converter output voltage
//	input:
//		channel - converter channel to affect: CHANNEL_5V or CHANNEL_12V
//		new_value - new value to check and set
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		VOLTAGE_SETTING_APPLIED 	- if new setting has been set
//---------------------------------------------------------------------------//
static uint8_t Converter_SetVoltage(uint8_t channel, int32_t new_value, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : 0);
	uint8_t result = 0;

	// Check arguments
	if ( (r == 0) )
		return result;		// Illegal argument
					
	if (SetRegulationValue(&r->voltage, new_value, err_code) == NEW_VALUE_SET)
	{
		result = VOLTAGE_SETTING_APPLIED;
		// Update hardware
		if (r == converter_regulation)
			ApplyAnalogRegultaion(APPLY_VOLTAGE);	
	}
		
	return result;
}


//---------------------------------------------------------------------------//
// 	Set converter output current
//	input:
//		channel - converter channel to affect: CHANNEL_5V or CHANNEL_12V
//		current_range - 20A range or 40A range to modify
//		new_value - new value to check and set
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		CURRENT_SETTING_APPLIED 	- if new setting has been set
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrent(uint8_t channel, uint8_t current_range, int32_t new_value, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : 0);
	reg_setting_t *s = (current_range == CURRENT_RANGE_LOW) ? r->current_low_range : 
					   ((current_range == CURRENT_RANGE_HIGH) ? r->current_high_range : 0);
	uint8_t result = 0;
	
	// Check arguments
	if ( (r == 0) || (s == 0) )
		return result;		// Illegal argument
	
	if (SetRegulationValue(s, new_value, err_code) == NEW_VALUE_SET)
	{
		result = CURRENT_SETTING_APPLIED;
		// Update hardware
		if (s == converter_regulation->current)
			ApplyAnalogRegultaion(APPLY_CURRENT);
	}
	
	return 	result;			   
}



//---------------------------------------------------------------------------//
// 	Set converter voltage limit
//	input:
//		channel - converter channel to affect: CHANNEL_5V or CHANNEL_12V
//		limit_type - low or high limit to modify
//		new_value - new value to check and set
//		enable - set limit enabled or disabled
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		VOLTAGE_SETTING_APPLIED - if new voltage setting has been applied
//		VOLTAGE_LIMIT_APPLIED	- if new limit setting has been applied
//---------------------------------------------------------------------------//
static uint8_t Converter_SetVoltageLimit(uint8_t channel, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : 0);
	uint8_t result = 0;
	
	// Check arguments
	if ( (r == 0) || ((limit_type != LIMIT_TYPE_LOW) && (limit_type != LIMIT_TYPE_HIGH)) )
		return result;		// Illegal argument						
	
	if (SetRegulationLimit(&r->voltage, limit_type, new_value, enable, err_code) == NEW_VALUE_SET)
	{
		result = VOLTAGE_LIMIT_APPLIED;
		// Ensure that voltage setting lies inside new limits
		result |= Converter_SetVoltage(channel, r->voltage.setting);
	}
	
	return result;
}


//---------------------------------------------------------------------------//
// 	Set converter current limit
//	input:
//		channel - converter channel to affect: CHANNEL_5V or CHANNEL_12V
//		current_range - 20A range or 40A range to modify
//		limit_type - low or high limit to modify
//		new_value - new value to check and set
//		enable - set limit enabled or disabled
//		*err_code - return code providing information about 
//			limiting new value by absolute and software limits
//	output:
//		CURRENT_SETTING_APPLIED - if new current setting has been applied
//		CURRENT_LIMIT_APPLIED	- if new limit setting has been applied
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrentLimit(uint8_t channel, uint8_t current_range, uint8_t limit_type, int32_t new_value, uint8_t enable, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : 0);
	reg_setting_t *s = (current_range == CURRENT_RANGE_LOW) ? r->current_low_range : 
					   ((current_range == CURRENT_RANGE_HIGH) ? r->current_high_range : 0);
	uint8_t result = 0;
	
	// Check arguments
	if ( (r == 0) || (s == 0) || ((limit_type != LIMIT_TYPE_LOW) && (limit_type != LIMIT_TYPE_HIGH)) )
		return result;		// Illegal argument
		
	if (SetRegulationLimit(s, limit_type, new_value, enable) == NEW_VALUE_SET)
	{
		result = CURRENT_LIMIT_APPLIED;
		// Ensure that current setting lies inside new limits
		result |= Converter_SetCurrent(channel, current_range, s->setting);
	}
	
	return result;
}



//---------------------------------------------------------------------------//
// 	Set converter overload protection parameters
//	input:
//		channel - converter channel to affect: CHANNEL_5V or CHANNEL_12V
//		protection_enable - set protection enabled or disabled
//		overload_timeout - specifies delay between overload discovering and turning OFF converter
//	output:
//		OVERLOAD_SETTINGS_APPLIED if new overload settings have been applied
//---------------------------------------------------------------------------//
static uint8_t Converter_SetProtectionControl(uint8_t channel, uint8_t protection_enable, int16_t overload_timeout, uint8_t *err_code = 0)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : 0);
								
	uint8_t result = 0;
	
	// Check arguments
	if ( (r == 0) )
		return result;		// Illegal argument
	
	// Bound new value
	if (overload_timeout >= MAX_OVERLOAD_TIMEOUT)
	{
		overload_timeout = MAX_OVERLOAD_TIMEOUT;
		if (err_code != 0) err_code = VALUE_BOUND_BY_ABS_MAX;
	}
	if (overload_timeout <= MIN_OVERLOAD_TIMEOUT)
	{
		overload_timeout = MIN_OVERLOAD_TIMEOUT;
		if (err_code != 0) err_code = VALUE_BOUND_BY_ABS_MIN;
	}				
						
	if ((r->overload_timeout != overload_timeout) || (r->overload_protection_enable != protection_enable))
	{
		r->overload_timeout = overload_timeout;
		r->overload_protection_enable = protection_enable;
		result = OVERLOAD_SETTINGS_APPLIED;
		// Overload protection mechanism uses data from converter_regulation, so no
		// 	special settings are required.
	}

	return result;
}




//---------------------------------------------------------------------------//
// 	Set converter current range
//	input:
//		channel - converter channel to affect: CHANNEL_5V or CHANNEL_12V
//		new_value - new current range 
//	output:
//		CURRENT_RANGE_APPLIED 	- if new setting has been set
//---------------------------------------------------------------------------//
static uint8_t Converter_SetCurrentRange(uint8_t channel, uint8_t new_value)
{
	converter_regulation_t *r = (channel == CHANNEL_5V) ? &channel_5v : 
								((channel == CHANNEL_12V) ? &channel_12v : 0);
	uint8_t result = 0;
	
	// Check converter state - converter should be disabled for enough time
	if ((state_HWProcess & STATE_HW_ON) || (state_HWProcess & STATE_HW_TIMER_NOT_EXPIRED) || (r == 0))
		return result;
	
	// Check argument - should be either CURRENT_RANGE_LOW or CURRENT_RANGE_HIGH
	if ( (new_value != CURRENT_RANGE_LOW) && (new_value != CURRENT_RANGE_HIGH) )
		return result;
	
	// Check if new value differs from current
	if (r->current->RANGE != new_value)
	{	
		r->current = (new_value == CURRENT_RANGE_LOW) ? &r->current_low_range : &r->current_high_range;
		result = CURRENT_RANGE_APPLIED;
		
		if (r == converter_regulation)
		{
			SetCurrentRange(converter_regulation->current->RANGE);
			ApplyAnalogRegulation( APPLY_CURRENT );
		}
	}
	
	return result;
}



//---------------------------------------------------------------------------//
// 	Set converter channel
//	input:
//		new_value - converter channel to select: CHANNEL_5V or CHANNEL_12V
//	output:
//		CHANNEL_SETTING_APPLIED 	- if new setting has been set
//---------------------------------------------------------------------------//
uint8_t Converter_SetChannel(uint8_t new_value)
{
	uint8_t result = 0;
	
	// Check converter state
	if ((state_HWProcess & STATE_HW_ON) || (state_HWProcess & STATE_HW_TIMER_NOT_EXPIRED))
	{
		// Converter is enabled, or safe interval is not expired
		return result;
	}
	
	// Check argument - should be either CHANNEL_5V or CHANNEL_12V
	if ( (new_value != CHANNEL_5V) && (new_value != CHANNEL_12V) )	
		return result;		// Illegal argument
	
	// Check if new value differs from current
	if (new_value != converter_regulation->CHANNEL) 
	{
		converter_regulation = (channel == CHANNEL_5V) ? &channel_5v : &channel_12v;
		result = CHANNEL_SETTING_APPLIED;
		
		// Apply controls
		__disable_irq();
		SetFeedbackChannel(converter_regulation->CHANNEL);		// PORTF can be accessed from ISR
		__enable_irq();
		SetCurrentRange(converter_regulation->current->RANGE);
		SetOutputLoad(converter_regulation->load_state);
		ApplyAnalogRegulation( APPLY_VOLTAGE | APPLY_CURRENT );
		// Overload protection mechanism uses data from converter_regulation, so no
		// 	special settings are required.
	}
	
	return result;
}















//=================================================================//
//=================================================================//
//          L O W  -  L E V E L   P R O C E S S I N G              //
//=================================================================//
//=================================================================//




//---------------------------------------------//
//	Converter hardware Enable/Disable control task
//	
//	Low-level task for accessing Enbale/Disable functions.
//	There may be an output overload which has to be correctly handled - converter should be disabled
//	The routine takes care for overload and enable/disable control coming from top-level controllers
//	
// TODO: ensure that ISR is non-interruptable
//---------------------------------------------//
void Converter_HWProcess(void) 
{
	static uint16_t overload_ignore_counter;
	static uint16_t overload_counter;
	static uint16_t safe_counter = 0;
	static uint16_t user_counter = 0;
	static uint16_t led_blink_counter = 0;
	static uint16_t overload_warning_counter = 0;
	uint8_t overload_check_enable;
	uint8_t raw_overload_flag;
	uint8_t led_state;

	//-------------------------------//
	// Get converter status and process overload timers
	
	// Due to hardware specialty overload input is active when converter is powered off
	// Overload timeout counter reaches 0 when converter has been enabled for OVERLOAD_IGNORE_TIMEOUT ticks
	overload_check_enable = 0;
	if ((state_HWProcess & (STATE_HW_OFF | STATE_HW_OFF_BY_ADC)) || (converter_regulation->overload_protection_enable == 0))
		overload_ignore_counter = OVERLOAD_IGNORE_TIMEOUT;
	else if (overload_ignore_counter != 0)
		overload_ignore_counter--;
	else
		overload_check_enable = 1;
		
	
	if (overload_check_enable)
		raw_overload_flag = GetOverloadStatus();
	else
		raw_overload_flag = NORMAL;
	
	if (raw_overload_flag == NORMAL)
		//overload_counter = OVERLOAD_TIMEOUT;
		overload_counter = converter_regulation->overload_timeout;
	else if (overload_counter != 0)
		overload_counter--;
	

	//-------------------------------//
	// Check overload 	
	if (overload_counter == 0)
	{
		// Converter is overloaded
		state_HWProcess &= ~STATE_HW_ON;
		state_HWProcess |= STATE_HW_OFF | STATE_HW_OVERLOADED;	// Set status for itself and top-level software
		safe_counter = MINIMAL_OFF_TIME;						// Start timer to provide minimal OFF timeout
		xQueueSendToFrontFromISR(xQueueConverter, &converter_update_message, 0);	// No need for exact timing
	}
	
	//-------------------------------//
	// Process commands from top-level converter controller
	if (ctrl_HWProcess & CMD_HW_RESET_OVERLOAD)
	{
		state_HWProcess &= ~STATE_HW_OVERLOADED;
	}
	if ( (ctrl_HWProcess & CMD_HW_OFF) && (!(state_HWProcess & STATE_HW_OFF)) )
	{
		state_HWProcess &= ~STATE_HW_ON;
		state_HWProcess |= STATE_HW_OFF;		
		safe_counter = MINIMAL_OFF_TIME;						// Start timer to provide minimal OFF timeout
	}
	else if ( (ctrl_HWProcess & CMD_HW_ON) && (!(state_HWProcess & STATE_HW_ON)) )
	{
		state_HWProcess &= ~STATE_HW_OFF;
		state_HWProcess |= STATE_HW_ON;								
	}
	if (ctrl_HWProcess & CMD_HW_RESTART_USER_TIMER)
	{
		user_counter = USER_TIMEOUT;
	}
	if (ctrl_HWProcess & CMD_HW_RESTART_LED_BLINK_TIMER)
	{
		user_counter = LED_BLINK_TIMEOUT;
	}
	
	//-------------------------------//
	// Process commands from top-level ADC controller
	if (cmd_ADC_to_HWProcess & CMD_HW_OFF_BY_ADC)
	{
		state_HWProcess |= STATE_HW_OFF_BY_ADC;
	}
	else if (cmd_ADC_to_HWProcess & CMD_HW_ON_BY_ADC)
	{
		state_HWProcess &= ~STATE_HW_OFF_BY_ADC;
	}
	
	// Reset commands
	ctrl_HWProcess = 0;
	cmd_ADC_to_HWProcess = 0;

	//-------------------------------//
	// Apply converter state
	// TODO - check IRQ disable while accessing converter control port (MDR_PORTF) for write
	if (state_HWProcess & (STATE_HW_OFF | STATE_HW_OFF_BY_ADC))
		SetConverterState(CONVERTER_OFF);
	else if (state_HWProcess & STATE_HW_ON)
		SetConverterState(CONVERTER_ON);
		
	//-------------------------------//
	// LED indication
	// Uses raw converter state and top-level conveter status
	// TODO - check IRQ disable while accessing LED port (MDR_PORTB) for write
	led_state = 0;
	if ((state_HWProcess & STATE_HW_ON) && (led_blink_counter == 0))
		led_state |= LED_GREEN;
	if ((raw_overload_flag == OVERLOAD) || (conv_state & CONV_OVERLOAD))
		led_state |= LED_RED;
	UpdateLEDs(led_state);
	
	//-------------------------------//
	// Overload sound warning
	if ((raw_overload_flag == OVERLOAD) && (overload_warning_counter == 0))
	{
		xQueueSendToFrontFromISR(xQueueSound, &sound_instant_overload_msg, 0);	// No need for exact timing
		overload_warning_counter = OVERLOAD_WARNING_TIMEOUT;
	}
	
	//-------------------------------//
	// Process timers
	
	// Process safe timer - used by top-level controller to provide a safe minimal OFF timeout
	if (safe_counter != 0)
	{
		safe_counter--;
		state_HWProcess |= STATE_HW_TIMER_NOT_EXPIRED;
	}
	else
	{
		state_HWProcess &= ~STATE_HW_TIMER_NOT_EXPIRED;
	}
	
	// Process user timer - used by top-level controller to provide a safe interval after switching channels
	if (user_counter != 0)
	{
		user_counter--;
		state_HWProcess &= ~STATE_HW_USER_TIMER_EXPIRED;
	}
	else
	{
		state_HWProcess |= STATE_HW_USER_TIMER_EXPIRED;
	}
	
	// Process LED blink timer
	if (led_blink_counter != 0)
	{
		led_blink_counter--;
	}
	
	// Process overload sound warning timer
	if (overload_warning_counter != 0)
	{
		overload_warning_counter--;
	}
}









