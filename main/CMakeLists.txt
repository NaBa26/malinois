require_idf_targets()

idf_component_register(SRCS "main.c" "wifi-capture.c"
        INCLUDE_DIRS "include"
        REQUIRES esp_wifi nvs_flash esp_event ds3231)

