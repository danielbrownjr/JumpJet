// SPDX-License-Identifier: GPL-3.0-or-later
#include "jj_bringup.h"
#include "dc_wifi.h"
#include "esp_app_desc.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "sdkconfig.h"
#if CONFIG_SPIRAM
#include "esp_psram.h"
#endif

static jj_boot_observation_t s_boot;

void jj_bringup_init(void)
{
    const esp_app_desc_t *app = esp_app_get_description();
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    s_boot = (jj_boot_observation_t) {
        .idf_version = esp_get_idf_version(), .project = app->project_name,
        .version = app->version, .chip_model = chip.model,
        .chip_revision = chip.revision, .chip_cores = chip.cores,
        .reset_reason = esp_reset_reason(),
        .configured_flash_image_mode = CONFIG_ESPTOOLPY_FLASHMODE,
        .configured_flash_image_frequency = CONFIG_ESPTOOLPY_FLASHFREQ,
        .psram_state = JJ_PSRAM_NOT_ATTEMPTED,
        .sampled_at_ms = (uint64_t)(esp_timer_get_time() / 1000),
    };
    s_boot.flash_known = esp_flash_get_physical_size(NULL, &s_boot.flash_bytes) == ESP_OK;
#if CONFIG_SPIRAM
    if (esp_psram_is_initialized()) {
        s_boot.psram_state = JJ_PSRAM_INITIALIZED;
        s_boot.psram_bytes = (uint32_t)esp_psram_get_size();
    } else {
#if CONFIG_SPIRAM_BOOT_INIT
        s_boot.psram_state = JJ_PSRAM_UNAVAILABLE;
#else
        s_boot.psram_state = JJ_PSRAM_NOT_ATTEMPTED;
#endif
    }
#endif
}

static const char *wifi_state_name(dc_wifi_state_t state)
{
    switch (state) {
    case DC_WIFI_STATE_INIT: return "init";
    case DC_WIFI_STATE_STA_CONNECTING: return "sta_connecting";
    case DC_WIFI_STATE_STA_CONNECTED: return "sta_connected";
    case DC_WIFI_STATE_AP_PORTAL: return "ap_portal";
    default: return "unknown";
    }
}

cJSON *jj_bringup_snapshot_json(void)
{
    jj_runtime_observation_t runtime = {
        .sampled_at_ms = (uint64_t)(esp_timer_get_time() / 1000),
        .free_internal_8bit_bytes = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        .minimum_free_internal_8bit_bytes = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
        .wifi_state = wifi_state_name(dc_wifi_state()),
    };
    wifi_ap_record_t ap;
    esp_err_t association = esp_wifi_sta_get_ap_info(&ap);
    if (association == ESP_OK) runtime.sta_associated = JJ_OBSERVATION_TRUE;
    else if (association == ESP_ERR_WIFI_NOT_CONNECT) runtime.sta_associated = JJ_OBSERVATION_FALSE;
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta) {
        esp_netif_ip_info_t ip = {0};
        if (esp_netif_get_ip_info(sta, &ip) == ESP_OK) {
            // IP may be cached after disconnection; do not present that as live.
            runtime.sta_ipv4_available = esp_netif_is_netif_up(sta) && ip.ip.addr != 0 &&
                runtime.sta_associated == JJ_OBSERVATION_TRUE ? JJ_OBSERVATION_TRUE :
                runtime.sta_associated == JJ_OBSERVATION_UNKNOWN ? JJ_OBSERVATION_UNKNOWN : JJ_OBSERVATION_FALSE;
        }
        esp_netif_dhcp_status_t dhcp;
        if (esp_netif_dhcpc_get_status(sta, &dhcp) == ESP_OK)
            runtime.dhcp_client_started = dhcp == ESP_NETIF_DHCP_STARTED ? JJ_OBSERVATION_TRUE : JJ_OBSERVATION_FALSE;
    }
    runtime.sample_finished_at_ms = (uint64_t)(esp_timer_get_time() / 1000);
    return jj_bringup_json(&s_boot, &runtime);
}
