
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


#define GUI_UPDATE_ALL	0



extern xQueueHandle xQueueGUI;
void vTaskGUI(void *pvParameters);



