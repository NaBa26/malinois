#include <nvs.h>
#include "include/wifi-capture.h"
#include "ds3231-module.h"

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "sdkconfig.h"
#include "portmacro.h"
#include "driver/i2c_types.h"
#include "nvs_flash.h"

static const char *TAG = "MALINOIS";
static RingbufHandle_t buf_handle;
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

static inline uint8_t bcd_to_dec(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret==ESP_ERR_NVS_NO_FREE_PAGES || ret==ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); ret=nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void wifi_rx_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    if (!pkt || buf_handle == NULL) return;

    const UBaseType_t res =  xRingbufferSend(buf_handle, pkt, sizeof(wifi_promiscuous_pkt_t), 0);
    if (res != pdTRUE) {
        ESP_LOGW(TAG, "Ring buffer full, dropping packet");
    }
}

void init_wifi() {
    init_nvs();
    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_start());

    buf_handle  = xRingbufferCreate(8192, RINGBUF_TYPE_NOSPLIT);
    assert(buf_handle);
    if (buf_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create ring buffer.\n");
    }

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(wifi_rx_packet_handler);
}


void packet_consumer_task(void *pvParameters) {
    uint8_t time_data[7] = {0};
    size_t item_size;

    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");

    for (;;) {
        const auto pkt = (wifi_promiscuous_pkt_t *) xRingbufferReceive(buf_handle, &item_size, portMAX_DELAY);
        if (pkt) {
            register_read(&dev_handle, 0x00, time_data, sizeof(time_data));


            uint8_t now[7];
            now[0] = time_data[0] & 0x7F;
            now[1] = time_data[1] & 0x7F;
            now[2] = time_data[2] & 0x3F;

            register_write(&dev_handle, 0x00, now, 7);

            static int counter = 0;
            if (++counter % 50 == 0) {
                ESP_LOGI(TAG,
                         "[%02d:%02d:%02d] RSSI=%d len=%d",
                         bcd_to_dec(now[2]),
                         bcd_to_dec(now[1]),
                         bcd_to_dec(now[0]),
                         pkt->rx_ctrl.rssi,
                         pkt->rx_ctrl.sig_len);
            }

            vRingbufferReturnItem(buf_handle, pkt);
        }
    }
}

void stop_capture() {
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
}

void deinit_wifi() {
    ESP_ERROR_CHECK(esp_wifi_deinit());
}
