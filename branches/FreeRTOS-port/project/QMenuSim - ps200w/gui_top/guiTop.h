
#include <stdint.h>
#include "guiGraphHAL.h"

#ifdef __cplusplus
extern "C" {
#endif

// Temporary display buffer, used for splitting GUI buffer into two separate LCD's
//#define DISPLAY_XSIZE 96
#define DISPLAY_XSIZE (LCD_XSIZE / 2)
//#define DISPLAY_YSIZE 68
#define DISPLAY_YSIZE LCD_YSIZE
//#define DISPLAY_BUFFER_SIZE 96*9
#define DISPLAY_BUFFER_SIZE (LCD_BUFFER_SIZE / 2)




//-----------------------------------//
// Callbacks
#define LOG_FROM_TOP      10
#define LOG_FROM_BOTTOM   11

typedef void (*cbLogPtr)(int type, char *string);
typedef void (*cbLcdUpdatePtr)(int displayNum, void *buffer);

void registerLogCallback(cbLogPtr fptr);
void registerLcdUpdateCallback(cbLcdUpdatePtr fptr);
//-----------------------------------//





// Functions for top-level
void guiInitialize(void);
void guiUpdateTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
void guiDrawAll(void);

void guiButtonEvent(uint16_t buttonCode, uint8_t eventType);
//void guiButtonPressed(uint16_t buttonCode);
//void guiButtonReleased(uint16_t buttonCode);
void guiEncoderRotated(int32_t delta);


// Functions for calling from GUI
void guiLogEvent(char *string);




//=================================================================//
//=================================================================//
//                      Hardware emulation interface               //
//=================================================================//

typedef struct {
    uint32_t type;
    union {
        struct {
            uint32_t a;
            uint32_t b;
        } data;
        struct {
            uint8_t limit;
            uint8_t enable;
            uint32_t value;
        } voltage_limit_setting;
    };
    //	uint32_t data_a;
    //	uint32_t data_b;
} conveter_message_t;



// SelectedChannel
#define	GUI_CHANNEL_5V			0x1
#define	GUI_CHANNEL_12V			0x0
// CurrentLimit
#define	GUI_CURRENT_RANGE_HIGH	0x1
#define	GUI_CURRENT_RANGE_LOW	 	0x0



void guiUpdateVoltageIndicator(void);
void guiUpdateVoltageSetting(void);
void applyGuiVoltageSetting(int16_t new_set_voltage);
void applyGuiVoltageLimit(uint8_t type, uint8_t enable, int16_t value);

void guiUpdateCurrentIndicator(void);
void guiUpdateCurrentSetting(void);
void applyGuiCurrentSetting(int16_t new_set_current);

void guiUpdateChannelSetting(void);
void applyGuiChannelSetting(uint8_t new_channel);

void guiUpdateCurrentRange(void);
void applyGuiCurrentRange(uint8_t new_current_range);

void guiUpdatePowerIndicator(void);
void guiUpdateTemperatureIndicator(void);



#ifdef __cplusplus
}
#endif
