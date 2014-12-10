
#include "MDR32Fx.h"                    
#include "MDR32F9Qx_timer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "sound_defs.h"
#include "sound_samples.h"
#include "sound_driver.h"


static const sample_t *event_table[] = 
{
	&sample_beep_600Hz_x2,			// 0 - unknown command
	&sample_beep_1000Hz_50ms,		// 1
	&sample_beep_800Hz_50ms,		// 2
	&sample_beep_800Hz_x3,			// 3
	&sample_beep_800Hz_20ms			// 4
};



xQueueHandle xQueueSound;
const uint32_t sound_driver_sync_msg = SYNC;
const uint32_t sound_instant_overload_msg = SND_CONV_INSTANT_OVERLOAD | SND_CONVERTER_PRIORITY_HIGH;

static const sample_t *decode_event(uint16_t event_code)
{
	const sample_t *psample;
	if (event_code >= sizeof(event_table) / sizeof(sample_t *))
		event_code = 0;
	psample = event_table[event_code];
	return psample;
}

static void set_beeper_period(uint16_t period)
{
	MDR_TIMER1->ARR = period - 1;	
	MDR_TIMER1->CCR2 = period / 2;		// 50% duty
}

static void set_beeper_output(uint8_t enable)
{
	uint32_t temp = MDR_TIMER1->CH2_CNTRL1;
	temp &= ~( (3 << TIMER_CH_CNTRL1_SELO_Pos) | (3 << TIMER_CH_CNTRL1_NSELO_Pos));
	if (enable)
		temp |= (TIMER_CH_OutSrc_REF << TIMER_CH_CNTRL1_SELO_Pos) | (TIMER_CH_OutSrc_REF << TIMER_CH_CNTRL1_NSELO_Pos);
	else
		temp |= (TIMER_CH_OutSrc_Only_1 << TIMER_CH_CNTRL1_SELO_Pos) | (TIMER_CH_OutSrc_Only_1 << TIMER_CH_CNTRL1_NSELO_Pos);
	MDR_TIMER1->CH2_CNTRL1 = temp;
}

//-------------------------------------------------------//
// Messages to sound driver should be composed like this
// msg = cmd_TURN_ON   |   12;
//       ^                       ^
//       event code              priority
// If there is some sample playing and incoming events's priority is higher than current,
// current sample is interrupted and new sample is started. 
// If event's priority is equal or less than current's, event is missed.
// All sample processing (starting, stopping, changing to another) are tied to SYNC message
//-------------------------------------------------------//
void vTaskSound(void *pvParameters) 
{
	uint32_t income_msg;
	uint16_t income_event_code;
	uint16_t income_event_priority;
	
	uint16_t pending_event_code = 0;
	uint16_t pending_event_priority = 0;
	
	uint16_t current_event_priority = 0;
	
	uint16_t state;
	uint8_t fsm_exit;
	
	const sample_t *psample;
	const tone_t *ptone;
	const tone_t *current_tone_record;
	uint16_t tone_repeats;
	uint16_t current_tone_period;
	uint16_t current_tone_duration;
	
	// Initialize
	xQueueSound = xQueueCreate( 10, sizeof( uint32_t ) );		// Queue can contain 10 elements of type uint32_t
	if( xQueueSound == 0 )
	{
		// Queue was not created and must not be used.
		while(1);
	}
	
	while(1)
	{
		xQueueReceive(xQueueSound, &income_msg, portMAX_DELAY);
		
		if (income_msg != SYNC)
		{
			// Received some message
			income_event_priority = (uint16_t)income_msg;
			income_event_code = (uint16_t)(income_msg >> 16);
			
			if (income_event_priority > pending_event_priority)
			{
				pending_event_priority = income_event_priority;
				pending_event_code = income_event_code;
			}
		}
		else
		{
			if (pending_event_priority > current_event_priority)
			{
				// Override FSM state
				state = START_NEW_SAMPLE;	
			}
			//---------------------------//
			// Sound driver FSM
			fsm_exit = 0;
			while (fsm_exit == 0)
			{
				switch(state)
				{
					case START_NEW_SAMPLE:
						psample = decode_event(pending_event_code);
						state = GET_NEXT_SAMPLE_RECORD;
						current_event_priority = pending_event_priority;
						pending_event_priority = 0;
						break;
					case GET_NEXT_SAMPLE_RECORD:
						ptone = psample->tones;
						tone_repeats = psample->num_repeats;
						if ((ptone == 0) && (tone_repeats == 0))
						{
							// Found zero sample - finished
							state = IDLE;
						}
						else
						{
							psample++;
							state = APPLY_CURRENT_SAMPLE_RECORD;
						}
						break;
					case APPLY_CURRENT_SAMPLE_RECORD:
						if (tone_repeats == 0)
						{
							state = GET_NEXT_SAMPLE_RECORD;
						}
						else
						{
							tone_repeats--;
							current_tone_record = ptone;
							state = APPLY_NEXT_TONE_RECORD;
						}
						break;
					case APPLY_NEXT_TONE_RECORD:
						current_tone_period = current_tone_record->tone_period;
						current_tone_duration = current_tone_record->duration;
						if ((current_tone_duration == 0) && (current_tone_period == 0))
						{
							state = APPLY_CURRENT_SAMPLE_RECORD;
						}
						else
						{
							current_tone_record++;
							if (current_tone_period != 0)
							{
								set_beeper_period(current_tone_period);
								set_beeper_output(1);
							}
							else
							{
								set_beeper_output(0);
							}
							state = WAIT_FOR_CURRENT_TONE;
						}
						break;
					case WAIT_FOR_CURRENT_TONE:
						if (current_tone_duration == 0)
						{
							// Finished current tone
							state = APPLY_NEXT_TONE_RECORD;
						}
						else
						{
							current_tone_duration--;
							fsm_exit = 1;
						}
						break;
					default:
						current_event_priority = 0;
						set_beeper_output(0);
						fsm_exit = 1;
						break;
				}
			}
			//---------------------------//
		}
	}
}










