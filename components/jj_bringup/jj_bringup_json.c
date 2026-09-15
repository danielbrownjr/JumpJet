// SPDX-License-Identifier: GPL-3.0-or-later
#include "jj_bringup.h"
#include "jj_board.h"

static void observed_bool(cJSON *object, const char *key, jj_observation_t value)
{
    if (value == JJ_OBSERVATION_TRUE || value == JJ_OBSERVATION_FALSE)
        cJSON_AddBoolToObject(object, key, value == JJ_OBSERVATION_TRUE);
    else cJSON_AddNullToObject(object, key);
}

static const char *psram_status(jj_psram_state_t state)
{
    switch (state) {
    case JJ_PSRAM_NOT_ATTEMPTED: return "not_attempted";
    case JJ_PSRAM_UNAVAILABLE: return "unavailable";
    case JJ_PSRAM_INITIALIZED: return "initialized";
    default: return "unknown";
    }
}

cJSON *jj_bringup_json(const jj_boot_observation_t *boot, const jj_runtime_observation_t *runtime)
{
    if (!boot || !runtime) return NULL;
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;
    cJSON_AddNumberToObject(root, "schema", 1);
    const jj_board_t *profile = jj_board_profile();
    cJSON *board = cJSON_AddObjectToObject(root, "board");
    cJSON_AddStringToObject(board, "provenance", "configured_profile_not_detection");
    cJSON_AddStringToObject(board, "profile", profile->profile);
    cJSON_AddStringToObject(board, "status", "candidate");
    cJSON_AddBoolToObject(board, "production", profile->production);
    cJSON_AddBoolToObject(board, "heater_available", profile->heater_available);
    cJSON *roles = cJSON_AddArrayToObject(board, "gpio_roles");
    for (size_t i = 0; i < profile->role_count; ++i) {
        const jj_board_role_t *role = &profile->roles[i];
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "role", role->role);
        if (role->gpio < 0) cJSON_AddNullToObject(item, "gpio");
        else cJSON_AddNumberToObject(item, "gpio", role->gpio);
        cJSON_AddStringToObject(item, "status", jj_gpio_status_name(role->status));
        cJSON_AddItemToArray(roles, item);
    }
    cJSON *b = cJSON_AddObjectToObject(root, "boot");
    cJSON_AddStringToObject(b, "provenance", "esp_idf_boot_snapshot");
    cJSON_AddNumberToObject(b, "sampled_at_uptime_ms", (double)boot->sampled_at_ms);
    cJSON_AddStringToObject(b, "idf_version", boot->idf_version);
    cJSON_AddStringToObject(b, "project", boot->project);
    cJSON_AddStringToObject(b, "version", boot->version);
    cJSON_AddNumberToObject(b, "chip_model_id", boot->chip_model);
    cJSON_AddNumberToObject(b, "chip_revision_major", boot->chip_revision / 100);
    cJSON_AddNumberToObject(b, "chip_revision_minor", boot->chip_revision % 100);
    cJSON_AddNumberToObject(b, "chip_cores", boot->chip_cores);
    cJSON_AddNumberToObject(b, "reset_reason_id", boot->reset_reason);
    if (boot->flash_known) cJSON_AddNumberToObject(b, "detected_flash_bytes", boot->flash_bytes);
    else cJSON_AddNullToObject(b, "detected_flash_bytes");
    cJSON_AddStringToObject(b, "configured_flash_image_mode", boot->configured_flash_image_mode);
    cJSON_AddStringToObject(b, "configured_flash_image_frequency", boot->configured_flash_image_frequency);
    cJSON *psram = cJSON_AddObjectToObject(b, "psram");
    cJSON_AddStringToObject(psram, "initialization", psram_status(boot->psram_state));
    cJSON_AddStringToObject(psram, "physical_presence", "unknown");
    // Successful initialization is positive evidence. Disabled/failed is not absence.
    if (boot->psram_state == JJ_PSRAM_INITIALIZED) {
        cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(psram, "physical_presence"), "detected");
        cJSON_AddNumberToObject(psram, "detected_bytes", boot->psram_bytes);
    } else cJSON_AddNullToObject(psram, "detected_bytes");
    cJSON_AddStringToObject(psram, "self_test", "not_attempted");
    cJSON *r = cJSON_AddObjectToObject(root, "runtime");
    cJSON_AddStringToObject(r, "provenance", "esp_idf_and_dc_wifi_request_sample");
    cJSON_AddNumberToObject(r, "sampled_at_uptime_ms", (double)runtime->sampled_at_ms);
    cJSON_AddNumberToObject(r, "sample_finished_at_uptime_ms", (double)runtime->sample_finished_at_ms);
    cJSON_AddNumberToObject(r, "free_internal_8bit_heap_bytes", runtime->free_internal_8bit_bytes);
    cJSON_AddNumberToObject(r, "minimum_free_internal_8bit_heap_bytes", runtime->minimum_free_internal_8bit_bytes);
    cJSON *wifi = cJSON_AddObjectToObject(r, "wifi");
    cJSON_AddStringToObject(wifi, "core_state", runtime->wifi_state);
    cJSON_AddStringToObject(wifi, "provisioning_completion", "unknown");
    observed_bool(wifi, "sta_associated", runtime->sta_associated);
    observed_bool(wifi, "sta_ipv4_available", runtime->sta_ipv4_available);
    observed_bool(wifi, "dhcp_client_started", runtime->dhcp_client_started);
    return root;
}
