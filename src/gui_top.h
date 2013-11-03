
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#define GUI_UPDATE_ALL	0
#define GUI_PROCESS_BUTTONS	1



extern xQueueHandle xQueueGUI;
void vTaskGUI(void *pvParameters);



