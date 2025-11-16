#include <nvs.h>
#include "include/wifi-capture.h"
#include "ds3231-module.h"

#include "esp_wifi_types.h"
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

typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration_id;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t seq_ctrl;
    uint8_t addr4[6];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[];
} wifi_ieee80211_packet_t;


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



    Time t = { .seconds = 25, .minutes = 25, .hours = 16 };
    set_init_time(&dev_handle, &t);

    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Current time set as [%02d:%02d:%02d]", t.hours,t.minutes,t.seconds);

    for (;;) {
        const auto pkt = (wifi_promiscuous_pkt_t *) xRingbufferReceive(buf_handle, &item_size, portMAX_DELAY);
        if (pkt) {
            register_read(&dev_handle, 0x00, time_data, sizeof(time_data));


            const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)pkt->payload;
            const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

            const uint8_t frame_type    = (hdr->frame_ctrl & 0x0C) >> 2;
            const uint8_t frame_subtype = (hdr->frame_ctrl & 0xF0) >> 4;

            static int counter = 0;
            if (++counter % 50 == 0) {

                ESP_LOGI(TAG,
                         "\n"
                         "⟶ Time: %02d:%02d:%02d\n"
                         "   RSSI: %d dBm | Len: %d bytes | Rate: %u Mbps\n"
                         "   Channel: %u | HT: %s | MCS: %u | BW: %s\n"
                         "   SRC: %02X:%02X:%02X:%02X:%02X:%02X\n"
                         "   DST: %02X:%02X:%02X:%02X:%02X:%02X\n"
                         "   Type: %u | Subtype: %u\n",

                         // Timestamp
                         bcd_to_dec(time_data[2] & 0x3F),
                         bcd_to_dec(time_data[1] & 0x7F),
                         bcd_to_dec(time_data[0] & 0x7F),

                         // Basic radio info
                         pkt->rx_ctrl.rssi,
                         pkt->rx_ctrl.sig_len,
                         pkt->rx_ctrl.rate,

                         // Channel & PHY
                         pkt->rx_ctrl.channel,
                         (pkt->rx_ctrl.sig_mode ? "HT" : "Legacy"),
                         pkt->rx_ctrl.mcs,
                         (pkt->rx_ctrl.cwb ? "40MHz" : "20MHz"),

                         // Source MAC
                         hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
                         hdr->addr2[3], hdr->addr2[4], hdr->addr2[5],

                         // Destination MAC
                         hdr->addr1[0], hdr->addr1[1], hdr->addr1[2],
                         hdr->addr1[3], hdr->addr1[4], hdr->addr1[5],

                         // Frame classification
                         frame_type,
                         frame_subtype
                );
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
