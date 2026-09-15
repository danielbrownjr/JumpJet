// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "cJSON.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    JJ_PSRAM_NOT_ATTEMPTED, JJ_PSRAM_UNAVAILABLE, JJ_PSRAM_INITIALIZED, JJ_PSRAM_UNKNOWN
} jj_psram_state_t;

typedef struct {
    const char *idf_version;
    const char *project;
    const char *version;
    int chip_model; // ESP-IDF esp_chip_model_t numeric value.
    unsigned chip_revision; // ESP-IDF: major * 100 + minor.
    unsigned chip_cores;
    int reset_reason; // ESP-IDF esp_reset_reason_t numeric value.
    bool flash_known;
    uint32_t flash_bytes;
    const char *configured_flash_image_mode;
    const char *configured_flash_image_frequency;
    jj_psram_state_t psram_state;
    uint32_t psram_bytes;
    uint64_t sampled_at_ms;
} jj_boot_observation_t;

typedef enum { JJ_OBSERVATION_UNKNOWN, JJ_OBSERVATION_FALSE, JJ_OBSERVATION_TRUE } jj_observation_t;
typedef struct {
    uint64_t sampled_at_ms;
    uint64_t sample_finished_at_ms;
    uint32_t free_internal_8bit_bytes;
    uint32_t minimum_free_internal_8bit_bytes;
    const char *wifi_state;
    jj_observation_t sta_associated;
    jj_observation_t sta_ipv4_available;
    jj_observation_t dhcp_client_started;
} jj_runtime_observation_t;

// Called once before the portal starts; no probing or memory initialization.
void jj_bringup_init(void);
cJSON *jj_bringup_snapshot_json(void);
// Pure serialization boundary, shared with host fixtures.
cJSON *jj_bringup_json(const jj_boot_observation_t *boot, const jj_runtime_observation_t *runtime);
