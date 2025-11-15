#pragma once

void init_wifi();

void packet_consumer_task(void *pvParameters);

void stop_capture();

void deinit_wifi();