// SPDX-License-Identifier: GPL-3.0-or-later
// Jump Jet — Dragon-family firmware for the Prusa CORE One chamber heater.
// Foundation milestone: no actuator driver exists, so this image cannot energize heat.

#include "dc_evlog.h"
#include "dc_prusa.h"
#include "dc_wifi.h"
#include "jj_interlock.h"
#include "jj_portal.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "jumpjet";
static jj_interlock_t s_interlock;

static bool printer_is_printing(const char *state)
{
    return state && strcmp(state, "PRINTING") == 0;
}

static void control_task(void *arg)
{
    (void)arg;
    jj_block_reason_t previous = JJ_BLOCK_NONE;
    for (;;) {
        dc_prusa_status_t printer = {0};
        (void)dc_prusa_get_status(&printer);
        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
        const jj_inputs_t input = {
            .now_ms = now_ms,
            .commissioned = false,
            .mode = JJ_MODE_OFF,
            .requested_target_c = 0.0f,
            .chamber = {.status = JJ_SENSOR_UNAVAILABLE},
            .outlet = {.status = JJ_SENSOR_UNAVAILABLE},
            .case_sensor = {.status = JJ_SENSOR_UNAVAILABLE},
            .printer = {
                .online = printer.online,
                .printing = printer_is_printing(printer.printer_state),
                .bed_target_c = printer.bed_target,
                // dc_prusa v0.28.2 does not expose last-success time. Zero keeps
                // AUTO stale/fail-cold until that board-neutral API is added.
                .sample_ms = 0,
            },
        };
        const jj_outputs_t output = jj_interlock_step(&s_interlock, &input);
        if (output.block_reason != previous) {
            dc_evlog_add("interlock: %s", jj_block_reason_str(output.block_reason));
            previous = output.block_reason;
        }
        // Intentionally no GPIO/PWM write in the foundation image.
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void app_main(void)
{
    dc_evlog_console_init();
    dc_evlog_init();
    ESP_LOGI(TAG, "Jump Jet cold-safe foundation starting");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    const jj_interlock_config_t config = jj_interlock_default_config();
    jj_interlock_init(&s_interlock, &config);
    BaseType_t created = xTaskCreate(control_task, "jj_control", 4096, NULL, 8, NULL);
    ESP_ERROR_CHECK(created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
    const dc_wifi_identity_t identity = {
        .hostname = "jumpjet",
        .instance_name = "Jump Jet",
        .ap_ssid_prefix = "JumpJet_",
        .ap_password = DC_WIFI_DEFAULT_AP_PASSWORD,
    };
    ESP_ERROR_CHECK(dc_wifi_set_identity(&identity));
    ESP_ERROR_CHECK(dc_wifi_start());
    ESP_ERROR_CHECK(dc_prusa_start());
    ESP_ERROR_CHECK(jj_portal_start(&s_interlock));
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (running && esp_ota_get_state_partition(running, &state) == ESP_OK &&
        state == ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());
    }
    dc_evlog_add("boot complete: actuator unavailable; heater forced off");
    ESP_LOGW(TAG, "heater actuator is intentionally not implemented");
}
