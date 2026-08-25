// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum { JJ_MODE_OFF = 0, JJ_MODE_MANUAL, JJ_MODE_AUTO } jj_mode_t;
typedef enum {
    JJ_SENSOR_OK = 0, JJ_SENSOR_UNAVAILABLE, JJ_SENSOR_OPEN,
    JJ_SENSOR_SHORT, JJ_SENSOR_IMPLAUSIBLE,
} jj_sensor_status_t;
typedef enum {
    JJ_FAULT_NONE = 0, JJ_FAULT_SENSOR, JJ_FAULT_OVERTEMPERATURE,
    JJ_FAULT_FAN, JJ_FAULT_NO_HEAT, JJ_FAULT_UNCONTROLLED_RISE,
} jj_fault_t;
typedef enum {
    JJ_BLOCK_NONE = 0, JJ_BLOCK_OFF, JJ_BLOCK_NOT_COMMISSIONED,
    JJ_BLOCK_FAULT_LATCHED, JJ_BLOCK_PRINTER_OFFLINE, JJ_BLOCK_PRINTER_STALE,
    JJ_BLOCK_PRINTER_STOPPED, JJ_BLOCK_BED_TARGET_LOW,
} jj_block_reason_t;
typedef struct { jj_sensor_status_t status; float temperature_c; } jj_sensor_sample_t;
typedef struct {
    bool online;
    bool printing;
    float bed_target_c;
    uint32_t sample_age_ms;  // UINT32_MAX when no complete sample is available
} jj_printer_sample_t;
typedef struct {
    float maximum_target_c;
    float case_hard_limit_c;
    float cooldown_release_c;
    float auto_bed_threshold_c;
    float auto_chamber_target_c;
    uint32_t printer_stale_ms;
} jj_interlock_config_t;
typedef struct {
    bool commissioned;
    jj_mode_t mode;
    float requested_target_c;
    jj_sensor_sample_t chamber;
    jj_sensor_sample_t outlet;
    jj_sensor_sample_t case_sensor;
    jj_printer_sample_t printer;
    bool fan_proven;
} jj_inputs_t;
typedef struct {
    bool heater_requested;
    uint8_t fan_percent;
    float effective_target_c;
    jj_fault_t fault;
    jj_block_reason_t block_reason;
} jj_outputs_t;
typedef struct {
    jj_interlock_config_t config;
    jj_fault_t fault_latched;
    jj_outputs_t last_output;
} jj_interlock_t;

jj_interlock_config_t jj_interlock_default_config(void);
void jj_interlock_init(jj_interlock_t *state, const jj_interlock_config_t *config);
jj_outputs_t jj_interlock_step(jj_interlock_t *state, const jj_inputs_t *input);
bool jj_interlock_clear_fault(jj_interlock_t *state, const jj_inputs_t *input);
jj_outputs_t jj_interlock_snapshot(const jj_interlock_t *state);
const char *jj_fault_str(jj_fault_t fault);
const char *jj_block_reason_str(jj_block_reason_t reason);
