#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    printf("Hello, ESP32!\n");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 second
        printf("Tick\n");
    }
}
