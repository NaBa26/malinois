#include <stdio.h>
#include "wifi-capture.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void)
{
    printf("\n");
    printf("             _ _          _    \n");
    printf("  _ __  __ _| (_)_ _  ___(_)___\n");
    printf(" | '  \\/ _` | | | ' \\/ _ \\ (_-<\n");
    printf(" |_|_|_\\__,_|_|_|_||_\\___/_/__/\n");
    printf("                                \n");
    printf("\n");

    vTaskDelay(pdMS_TO_TICKS(1000));
    init_wifi();
    xTaskCreate(packet_consumer_task, "packet-consumer", 4096, NULL, 5, NULL);
}
