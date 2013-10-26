/********************************************************************
	Module		Converter
	
	Purpose:
		- converter ON/OFF/CHARGE processing 
		- converter state indication by front panel LED
		- sound events
					
	Globals used:
		msg_Converter		<-	commands to this module
		state_Converter		<-	status of module
		adc, sound, etc 	<-	FIXME
		
	Operation: FIXME
		Module Converter accepts messages passed through global variable msg_Converter,
		processes them and sets or clears flag bits in global state_Converter.
		Converter module performs all control of converter board - enables/disables it,
		controls charging process and overload handling.
		Command bits in msg_Converter are cleared after they are processed and corresponding action is performed.
		Some status bits in state_Converter are sticky while others are not.
		See header file for message and status bits.
********************************************************************/


uint8_t state_HWProcess = 0;
uint8_t ctrl_HWProcess = 0;

uint8_t HW_request = 0;

uint8_t feedback_source;
uint8_t feedback_new;

//---------------------------------------------//
//	Set the converter output voltage
//	
//	Changes are checked and applied at once
//---------------------------------------------//
void Converter_SetVoltage(uint16_t val)
{

}

void Converter_SetCurrent(uint16_t val)
{

}

void Converter_SetFeedbackChannel(uint8_t val)
{
	feedback_new = val;
}

static void Converter_SetCurrentLimit(uint8_t val) ?
{

}

void Converter_Enable(void)
{
	HW_request |= HW_ON;
}

void Converter_Disable(void)
{
	HW_request |= HW_OFF;
}

void Converter_StartCharge(void)
{

}


void Converter_Init(void)
{
	

}

void Converter_Process(void)
{
	uint8_t HW_cmd = 0;
	
	next_conv_state = conv_state;
	switch(conv_state)
	{
		CONV_OFF:
			if (feedback_source != feedback_new)
			{
				_switch_feedback_channel_(feedback_new);
				feedback_source = feedback_new;
				break;
			}
			if (HW_request & HW_ON)
			{
				next_conv_state = CONV_ON;
				HW_cmd |= HW_ON;
			}
			break;
		CONV_ON:
			if ( (state_HWProcess & HW_OFF) || (HW_request & HW_OFF) )
			{
				next_conv_state = CONV_OFF;
				break;
			}
			if (feedback_source != feedback_new)
			{
				next_conv_state = CONV_OFF;
				break;
			}
	}
	
	conv_state = next_conv_state;
	
	HW_cmd |= HW_START_ADC_VOLTAGE | HW_START_ADC_CURRENT;
	ctrl_HWProcess = HW_cmd;		// Atomic
	
	HW_request = 0;

	// LED indication
	led_state = 0;
	if (state_HWProcess & HW_ON)
		led_state = LED_GREEN;
	else if (state_HWProcess & HW_OVERLOADED)
		led_state = LED_RED;
		
	UpdateLEDs();
}		



/*
	// Measuring voltage while charging
	Converter_HWProcess(HW_OFF);
	wait();
	select_ADC_channel(Voltage);
	wait();
	ADC_start();
	wait();
	Voltage = ADC_read();
	Converter_HWProcess(HW_ON);
	 
	// Measuring current or voltage normally
	select_ADC_channel(Current);
	wait();
	ADC_start();
	wait();
	Current = ADC_read();
*/


//---------------------------------------------//
//	Converter hardware Enable/Disable control task
//	
//	Low-level task for accessing Enbale/Disable functions.
//	There may be an output overload which has to be correctly handled - converter should be disabled
//	This task takes care for overload and top-level enable/disable control
//	This task also starts Voltage/Current ADC operation
//---------------------------------------------//
void Converter_HWProcess(void) 
{
	uint16_t temp;
	static uint16_t overload_ignore_counter;
	static uint16_t overload_counter;
	
	
	// Due to hardware specialty overload input is active when converter is powered off
	if ( (state_HWProcess & (HW_OFF | HW_OFF_BY_ADC)) || (0/*_overload_functions_disabled_*/) )
		overload_ignore_counter = OVERLOAD_IGNORE_TIMEOUT;
	else if (overload_ignore_counter != 0)
		overload_ignore_counter--;
	
	temp = MDR_PORTA->RXTX;
	// Overload timeout counter reaches 0 when converter has been enabled for OVERLOAD_IGNORE_TIMEOUT ticks
	if ( (overload_ignore_counter != 0) || (temp & (1<<OVERLD) == 0) )
		overload_counter = OVERLOAD_TIMEOUT;
	else if (overload_counter != 0)
		overload_counter--;
	
	//-------------------------------//

	
	// Check overload 
	if (overload_counter == 0)
	{
		// Converter is overloaded
		state_HWProcess &= ~HW_ON;
		state_HWProcess |= HW_OFF | HW_OVERLOADED;	// Set status for itself and top-level software
	}
	
	// Process ON/OFF commands
	if (ctrl_HWProcess & HW_RESET_OVERLOAD)
	{
		state_HWProcess &= ~HW_OVERLOADED;
	}
	if ( (ctrl_HWProcess & HW_OFF) && (!(state_HWProcess & HW_OVERLOADED)) )
	{
		state_HWProcess &= ~(HW_ON);
		state_HWProcess |= HW_OFF;				
	}
	else if ( (ctrl_HWProcess & HW_ON) && (!(state_HWProcess & HW_OVERLOADED)) )
	{
		state_HWProcess &= ~(HW_OFF);
		state_HWProcess |= HW_ON;								
	}
	
	
	
	uint8_t adc_cmd;
	uint8_t adc_state;
	uint16_t voltage_samples;
	uint16_t current_samples;
	
	// Process ADC FSM-based controller
	next_adc_state = adc_state;
	switch (adc_state)
	{
		case ADC_IDLE:
			adc_cmd = state_HWProcess & (HW_START_ADC_VOLTAGE | HW_START_ADC_CURRENT | HW_START_ADC_DISCON);
			if (adc_cmd)
				next_adc_state = ADC_DISPATCH;
			break;
		case ADC_DISPATCH:
			if (adc_cmd & (HW_START_ADC_VOLTAGE | HW_START_ADC_DISCON) == HW_START_ADC_VOLTAGE)
			{
				// Normal voltage measure 
				adc_cmd &= ~(HW_START_ADC_VOLTAGE | HW_START_ADC_DISCON);
				next_adc_state = ADC_NORMAL_START_U;
				voltage_samples = 0;
			}
			else if (adc_cmd & HW_START_ADC_CURRENT)
			{
				// Current measure
				adc_cmd &= ~HW_START_ADC_CURRENT;
				next_adc_state = ADC_START_I;
				current_samples = 0;
			}
			else
			{
				// Update global Voltage and Current
				voltage_adc = voltage_samples>>2;
				current_adc = current_samples>>2;
				next_adc_state = ADC_IDLE;
			}
			break;
			
		case ADC_NORMAL_START_U:
			// TODO: use DMA for this purpose
			ADC1_SetChannel(ADC_CHANNEL_VOLTAGE);
			adc_counter = 4;
			next_adc_state = ADC_NORMAL_REPEAT_U;
			break;
		case ADC_NORMAL_REPEAT_U:
			if (adc_counter < 4)
				voltage_samples += ADC1_GetResult();
			ADC1_Start();
			if (--adc_counter == 0)
				next_adc_state = ADC_DISPATCH;
			break;
			
			
		case ADC_START_I:
			// TODO: use DMA for this purpose
			ADC1_SetChannel(ADC_CHANNEL_CURRENT);
			adc_counter = 4;
			next_adc_state = ADC_NORMAL_REPEAT_I
			break;
		case ADC_NORMAL_REPEAT_I:
			if (adc_counter < 4)
				current_samples += ADC1_GetResult();
			ADC1_Start();
			if (--adc_counter == 0)
				next_adc_state = ADC_DISPATCH;
			break;
		
		default:
			next_adc_state = ADC_IDLE;
			break;
	}
	
	adc_state = next_adc_state;
	
	/*
	if (ctrl_HWProcess & HW_START_ADC)
	{
		//...
		
		// Disable converter for ADC
		state_HWProcess |= HW_OFF_BY_ADC;
		
		//...
		
		// Enable converter for ADC
		state_HWProcess &= ~HW_OFF_BY_ADC;
		
	}
	*/
	
	
	// Reset command
	ctrl_HWProcess = 0;
	
	
	
	// Apply converter state
	if (state_HWProcess & (HW_OFF | HW_OFF_BY_ADC))
		PORT_ResetBits(MDR_PORTF, 1<<EN);
	else if (state_HWProcess & HW_ON)
		PORT_SetBits(MDR_PORTF, 1<<EN);
	
}






