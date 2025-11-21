#include "wifi-capture.h"
#include "ds3231-module.h"

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

static const char *TAG = "MALINOIS";

/* Ring buffer: stores captured packets between ISR callback → consumer task */
static RingbufHandle_t buf_handle;

/* I2C handles for DS3231 */
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

/* ---------------------------------------------------------
   Minimal 802.11 MAC header.
   Note: This is NOT the full real IEEE header; just the
   fields needed for frame type/subtype + MAC addresses.
--------------------------------------------------------- */
typedef struct {
    uint16_t frame_ctrl;
    uint16_t duration_id;
    uint8_t addr1[6];  // Destination
    uint8_t addr2[6];  // Source
    uint8_t addr3[6];
    uint16_t seq_ctrl;
    uint8_t addr4[6];
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[];
} wifi_ieee80211_packet_t;


/* Convert BCD → decimal for RTC reads */
static inline uint8_t bcd_to_dec(uint8_t v) {
    return ((v >> 4) * 10) + (v & 0x0F);
}

/* ---------------------------------------------------------
   Init NVS (mandatory for Wi-Fi, ESP-IDF requirement)
--------------------------------------------------------- */
static void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

/* ---------------------------------------------------------
   Wi-Fi promiscuous callback (ISR context!)
   Keep it SHORT and NON-BLOCKING.
--------------------------------------------------------- */
static void wifi_rx_packet_handler(void *buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    if (!pkt || !buf_handle) return;

    // NOTE: Using struct size is technically wrong,
    // but acceptable for simple sniffers.
    if (!xRingbufferSend(buf_handle, pkt, sizeof(wifi_promiscuous_pkt_t), 0)) {
        ESP_LOGW(TAG, "Ring buffer full → packet dropped");
    }
}

/* ---------------------------------------------------------
   Initialize Wi-Fi in promiscuous mode
--------------------------------------------------------- */
void init_wifi() {
    init_nvs();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // FIX: Set channel manually (otherwise logs always say "1")
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

    // Create ring buffer (8 KB)
    buf_handle = xRingbufferCreate(8192, RINGBUF_TYPE_NOSPLIT);
    assert(buf_handle);

    // Enable sniffer mode
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(wifi_rx_packet_handler);
}

/* ---------------------------------------------------------
   Packet consumer: runs forever, logs filtered info.
--------------------------------------------------------- */
void packet_consumer_task(void *pvParameters) {
    uint8_t time_data[7] = {0};
    size_t item_size;

    // Init RTC
    i2c_master_init(&bus_handle, &dev_handle);

    Time current_time = {
        .hours   = CONFIG_DS3231_SET_HOUR,
        .minutes = CONFIG_DS3231_SET_MINUTE,
        .seconds = CONFIG_DS3231_SET_SECOND,
    };
    set_init_time(&dev_handle, &current_time);

    ESP_LOGI(TAG, "RTC time initialized: %02d:%02d:%02d", current_time.hours, current_time.minutes, current_time.seconds);

    static int counter = 0;

    for (;;) {
        // Block until packet arrives
        wifi_promiscuous_pkt_t *pkt =
            (wifi_promiscuous_pkt_t *)xRingbufferReceive(buf_handle, &item_size, portMAX_DELAY);

        if (pkt) {
            // Read current time from RTC
            register_read(&dev_handle, 0x00, time_data, sizeof(time_data));

            // Cast payload to custom header
            const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)pkt->payload;
            const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

            uint8_t type    = (hdr->frame_ctrl >> 2) & 0x3;
            uint8_t subtype = (hdr->frame_ctrl >> 4) & 0xF;

            if (++counter % 50 == 0) {
                ESP_LOGI(TAG,
                    "\nTime: %02d:%02d:%02d\n"
                    "RSSI: %d dBm | Len: %d bytes | Rate: %u\n"
                    "Channel: %u | Mode: %s | MCS: %u | BW: %s\n"
                    "SRC: %02X:%02X:%02X:%02X:%02X:%02X\n"
                    "DST: %02X:%02X:%02X:%02X:%02X:%02X\n"
                    "Type: %u | Subtype: %u",

                    bcd_to_dec(time_data[2]),
                    bcd_to_dec(time_data[1]),
                    bcd_to_dec(time_data[0]),

                    pkt->rx_ctrl.rssi,
                    pkt->rx_ctrl.sig_len,
                    pkt->rx_ctrl.rate,
                    pkt->rx_ctrl.channel,
                    pkt->rx_ctrl.sig_mode ? "HT" : "Legacy",
                    pkt->rx_ctrl.mcs,
                    pkt->rx_ctrl.cwb ? "40MHz" : "20MHz",

                    hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
                    hdr->addr2[3], hdr->addr2[4], hdr->addr2[5],

                    hdr->addr1[0], hdr->addr1[1], hdr->addr1[2],
                    hdr->addr1[3], hdr->addr1[4], hdr->addr1[5],

                    type, subtype
                );
            }

            vRingbufferReturnItem(buf_handle, pkt);
        }
    }
}
