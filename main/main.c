#include <stdio.h>
#include "wifi-capture.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void app_main(void)
{
    printf("                        ___                                      \n");
    printf("                       (   ) .-.                  .-.            \n");
    printf(" ___ .-. .-.     .---.  | | ( __) ___ .-.   .--. ( __)   .--.    \n");
    printf("(   )   '   \\   / .-, \\ | | (''\")(   )   \\ /    \\(''\") /  _  \\   \n");
    printf(" |  .-.  .-. ; (__) ; | | |  | |  |  .-. .|  .-. ;| | . .' `. ;  \n");
    printf(" | |  | |  | |   .'`  | | |  | |  | |  | || |  | || | | '   | |  \n");
    printf(" | |  | |  | |  / .'| | | |  | |  | |  | || |  | || | _\\_`.(___) \n");
    printf(" | |  | |  | | | /  | | | |  | |  | |  | || |  | || |(   ). '.   \n");
    printf(" | |  | |  | | ; |  ; | | |  | |  | |  | || '  | || | | |  `\\ |  \n");
    printf(" | |  | |  | | ' `-'  | | |  | |  | |  | |'  `-' /| | ; '._,' '  \n");
    printf("(___)(___)(___)`.__.'_.(___)(___)(___)(___)`.__.'(___) '.___.'   \n");
    printf("                                                                 \n");
    printf("                                                                 \n");

    vTaskDelay(pdMS_TO_TICKS(1000));
    init_wifi();
    xTaskCreate(packet_consumer_task, "packet-consumer", 4096, NULL, 5, NULL);
}
