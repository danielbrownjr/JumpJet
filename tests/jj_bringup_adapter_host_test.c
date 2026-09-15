// SPDX-License-Identifier: GPL-3.0-or-later
// Simulated ESP-IDF return values: no physical measurements.
#include "idf_fixture.h"
#include "dc_wifi.h"
#include "jj_bringup.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool initialized, netif_present, up;
static esp_err_t association = ESP_FAIL, ip_result = ESP_FAIL, flash_result = ESP_FAIL;
static uint32_t ip_addr;
static int64_t clock_us;
static esp_netif_t netif;
static dc_wifi_state_t core_state;
const esp_app_desc_t *esp_app_get_description(void) { static const esp_app_desc_t app = {"fixture", "fixture"}; return &app; }
void esp_chip_info(esp_chip_info_t *chip) { *chip = (esp_chip_info_t){9, 2, 2}; }
const char *esp_get_idf_version(void) { return "fixture"; }
int esp_reset_reason(void) { return 1; }
int64_t esp_timer_get_time(void) { clock_us += 1000; return clock_us; }
esp_err_t esp_flash_get_physical_size(void *chip, uint32_t *size) { assert(!chip); *size = 8388608; return flash_result; }
bool esp_psram_is_initialized(void) { return initialized; }
size_t esp_psram_get_size(void) { assert(initialized); return 8192; }
size_t heap_caps_get_free_size(uint32_t caps) { assert(caps == (MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL)); return 100; }
size_t heap_caps_get_minimum_free_size(uint32_t caps) { assert(caps == (MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL)); return 50; }
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap) { (void)ap; return association; }
esp_netif_t *esp_netif_get_handle_from_ifkey(const char *key) { assert(!strcmp(key, "WIFI_STA_DEF")); return netif_present ? &netif : NULL; }
esp_err_t esp_netif_get_ip_info(esp_netif_t *n, esp_netif_ip_info_t *ip) { assert(n == &netif); ip->ip.addr = ip_addr; return ip_result; }
bool esp_netif_is_netif_up(esp_netif_t *n) { assert(n == &netif); return up; }
esp_err_t esp_netif_dhcpc_get_status(esp_netif_t *n, esp_netif_dhcp_status_t *status) { assert(n == &netif); *status = ESP_NETIF_DHCP_STARTED; return ESP_OK; }
dc_wifi_state_t dc_wifi_state(void) { return core_state; }
static cJSON *get(cJSON *obj, const char *key) { cJSON *value = cJSON_GetObjectItemCaseSensitive(obj, key); assert(value); return value; }
static void check_network(jj_observation_t expected_assoc, jj_observation_t expected_ip)
{
    cJSON *root = jj_bringup_snapshot_json();
    cJSON *runtime = get(root, "runtime"), *wifi = get(runtime, "wifi");
    cJSON *assoc = get(wifi, "sta_associated"), *ip = get(wifi, "sta_ipv4_available");
    assert(expected_assoc == JJ_OBSERVATION_UNKNOWN ? cJSON_IsNull(assoc) :
        expected_assoc == JJ_OBSERVATION_TRUE ? cJSON_IsTrue(assoc) : cJSON_IsFalse(assoc));
    assert(expected_ip == JJ_OBSERVATION_UNKNOWN ? cJSON_IsNull(ip) :
        expected_ip == JJ_OBSERVATION_TRUE ? cJSON_IsTrue(ip) : cJSON_IsFalse(ip));
    assert(get(runtime, "sample_finished_at_uptime_ms")->valuedouble >= get(runtime, "sampled_at_uptime_ms")->valuedouble);
    cJSON_Delete(root);
}
int main(void)
{
    jj_bringup_init();
    cJSON *root = jj_bringup_snapshot_json();
    cJSON *boot = get(root, "boot"), *psram = get(boot, "psram");
#if CONFIG_SPIRAM && CONFIG_SPIRAM_BOOT_INIT
    assert(!strcmp(get(psram, "initialization")->valuestring, "unavailable"));
#else
    assert(!strcmp(get(psram, "initialization")->valuestring, "not_attempted"));
#endif
    assert(cJSON_IsNull(get(psram, "detected_bytes")));
    assert(cJSON_IsNull(get(boot, "detected_flash_bytes")));
    cJSON_Delete(root);
    initialized = true; flash_result = ESP_OK;
    jj_bringup_init(); // Simulate a different boot, never called twice by firmware.
    root = jj_bringup_snapshot_json(); boot = get(root, "boot"); psram = get(boot, "psram");
#if CONFIG_SPIRAM
    assert(!strcmp(get(psram, "initialization")->valuestring, "initialized"));
    assert(get(psram, "detected_bytes")->valueint == 8192);
#else
    assert(!strcmp(get(psram, "initialization")->valuestring, "not_attempted"));
#endif
    assert(get(boot, "detected_flash_bytes")->valueint == 8388608);
    cJSON_Delete(root);
    check_network(JJ_OBSERVATION_UNKNOWN, JJ_OBSERVATION_UNKNOWN);
    netif_present = true; ip_result = ESP_OK; association = ESP_ERR_WIFI_NOT_CONNECT;
    check_network(JJ_OBSERVATION_FALSE, JJ_OBSERVATION_FALSE);
    association = ESP_OK; up = true;
    check_network(JJ_OBSERVATION_TRUE, JJ_OBSERVATION_FALSE); // associated, no lease
    ip_addr = 1; core_state = DC_WIFI_STATE_STA_CONNECTED;
    check_network(JJ_OBSERVATION_TRUE, JJ_OBSERVATION_TRUE);
    association = ESP_ERR_WIFI_NOT_CONNECT; // stale cached IP must not mean connected
    check_network(JJ_OBSERVATION_FALSE, JJ_OBSERVATION_FALSE);
    association = ESP_FAIL;
    check_network(JJ_OBSERVATION_UNKNOWN, JJ_OBSERVATION_UNKNOWN);
    association = ESP_OK; ip_result = ESP_FAIL;
    check_network(JJ_OBSERVATION_TRUE, JJ_OBSERVATION_UNKNOWN);
    puts("bring-up ESP-IDF adapter host fixtures: PASS");
}
