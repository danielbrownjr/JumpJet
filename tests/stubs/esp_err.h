// Host-only ESP-IDF error fixture; never used by firmware.
#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG 0x102
#define ESP_ERR_INVALID_STATE 0x103
#define ESP_ERR_WIFI_NOT_CONNECT 0x300f
