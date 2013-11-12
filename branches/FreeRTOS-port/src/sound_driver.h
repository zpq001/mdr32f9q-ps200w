
//---------------------------------------------//
// Sound driver message codes
#define SYNC 								0xFFFF
#define SND_CONV_CMD_OK						0x0001
#define SND_CONV_CMD_ILLEGAL				0x0002
#define SND_CONV_SETTING_OK					SND_CONV_CMD_OK
#define SND_CONV_SETTING_ILLEGAL			SND_CONV_CMD_ILLEGAL
#define SND_CONV_OVERLOADED					0x0003
#define SND_CONV_INSTANT_OVERLOAD			0x0004

//---------------------------------------------//
// Sound driver priorities
#define SND_CONVERTER_PRIORITY_NORMAL		50
#define SND_CONVERTER_PRIORITY_HIGH			51
#define SND_CONVERTER_PRIORITY_HIGHEST		52



//---------------------------------------------//
// Sound driver FSM states - private
#define IDLE							0x00
#define START_NEW_SAMPLE				0x01
#define GET_NEXT_SAMPLE_RECORD			0x02
#define APPLY_CURRENT_SAMPLE_RECORD		0x03
#define APPLY_NEXT_TONE_RECORD			0x04
#define WAIT_FOR_CURRENT_TONE			0x05



extern xQueueHandle xQueueSound;
extern const uint32_t sound_driver_sync_msg;
extern const uint32_t sound_instant_overload_msg;

void vTaskSound(void *pvParameters);


