// Host-only API fixtures, never target headers or hardware evidence.
#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { int unused; } wifi_ap_record_t;
typedef struct { char project_name[32]; char version[32]; } esp_app_desc_t;
typedef struct { int model; uint16_t revision; uint8_t cores; } esp_chip_info_t;
typedef struct { int unused; } esp_netif_t;
typedef struct { struct { uint32_t addr; } ip; } esp_netif_ip_info_t;
typedef enum { ESP_NETIF_DHCP_INIT, ESP_NETIF_DHCP_STARTED, ESP_NETIF_DHCP_STOPPED } esp_netif_dhcp_status_t;
#define MALLOC_CAP_8BIT (1U << 2)
#define MALLOC_CAP_INTERNAL (1U << 11)
const esp_app_desc_t *esp_app_get_description(void);
void esp_chip_info(esp_chip_info_t *chip);
const char *esp_get_idf_version(void);
int esp_reset_reason(void);
int64_t esp_timer_get_time(void);
esp_err_t esp_flash_get_physical_size(void *chip, uint32_t *size);
bool esp_psram_is_initialized(void);
size_t esp_psram_get_size(void);
size_t heap_caps_get_free_size(uint32_t caps);
size_t heap_caps_get_minimum_free_size(uint32_t caps);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap);
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *key);
esp_err_t esp_netif_get_ip_info(esp_netif_t *netif, esp_netif_ip_info_t *ip);
bool esp_netif_is_netif_up(esp_netif_t *netif);
esp_err_t esp_netif_dhcpc_get_status(esp_netif_t *netif, esp_netif_dhcp_status_t *status);
