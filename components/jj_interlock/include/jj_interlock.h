// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define JJ_MANUAL_TARGET_MIN_C     30.0f
#define JJ_MANUAL_TARGET_DEFAULT_C 45.0f
#define JJ_MANUAL_TARGET_MAX_C     50.0f

typedef enum { JJ_MODE_OFF = 0, JJ_MODE_MANUAL, JJ_MODE_AUTOMATIC } jj_mode_t;
typedef enum {
    JJ_AUTHORITY_NONE = 0,
    JJ_AUTHORITY_REMOTE,
    JJ_AUTHORITY_AUTOMATIC,
    JJ_AUTHORITY_REACQUIRING,
} jj_control_authority_t;
typedef enum {
    JJ_CONTROL_INHIBIT_NONE = 0,
    JJ_CONTROL_INHIBIT_NO_AUTHORITY,
    JJ_CONTROL_INHIBIT_STATE_REFRESH_REQUIRED,
    JJ_CONTROL_INHIBIT_NOT_ELIGIBLE,
} jj_control_inhibit_t;
typedef enum {
    JJ_THERMAL_IDLE = 0,
    JJ_THERMAL_HEATING,
    JJ_THERMAL_COOLDOWN,
} jj_thermal_state_t;
typedef enum {
    JJ_SENSOR_UNAVAILABLE = 0, JJ_SENSOR_OK, JJ_SENSOR_OPEN,
    JJ_SENSOR_SHORT, JJ_SENSOR_IMPLAUSIBLE,
} jj_sensor_status_t;
typedef enum {
    JJ_FAN_PROOF_UNAVAILABLE = 0, JJ_FAN_PROOF_PENDING,
    JJ_FAN_PROOF_PROVEN, JJ_FAN_PROOF_FAILED,
} jj_fan_proof_t;
typedef enum {
    JJ_FAULT_NONE = 0, JJ_FAULT_SENSOR, JJ_FAULT_OVERTEMPERATURE,
    JJ_FAULT_FAN, JJ_FAULT_NO_HEAT, JJ_FAULT_UNCONTROLLED_RISE,
    JJ_FAULT_CONFIG,
} jj_fault_t;
typedef enum {
    JJ_BLOCK_NONE = 0, JJ_BLOCK_OFF, JJ_BLOCK_NOT_COMMISSIONED,
    JJ_BLOCK_FAULT_LATCHED, JJ_BLOCK_PRINTER_UNAVAILABLE,
    JJ_BLOCK_PRINTER_NOT_PRINTING, JJ_BLOCK_AUTO_POLICY_UNAVAILABLE,
    JJ_BLOCK_MANUAL_TARGET_INVALID, JJ_BLOCK_FAN_PROOF_PENDING,
    JJ_BLOCK_INVALID_MODE, JJ_BLOCK_CONTROL_NO_AUTHORITY,
    JJ_BLOCK_STATE_REFRESH_REQUIRED, JJ_BLOCK_AUTO_AUTHORITY_UNAVAILABLE,
} jj_block_reason_t;

typedef struct { jj_sensor_status_t status; float temperature_c; } jj_sensor_sample_t;
typedef struct {
    bool online;
    bool printing; /* True only for dc_prusa's exact PRINTING state. */
    float bed_target_c;
} jj_printer_sample_t;
typedef struct {
    bool commissioned;
    jj_mode_t mode;
    float manual_target_c;
    jj_control_authority_t active_authority;
    jj_control_inhibit_t control_inhibit;
    bool manual_demand_authorized;
    bool automatic_target_available;
    float automatic_target_c;
    jj_sensor_sample_t chamber;
    jj_sensor_sample_t outlet;
    jj_sensor_sample_t case_sensor;
    bool overtemperature_detected;
    bool cooldown_required;
    bool fault_requires_thermal_management;
    jj_printer_sample_t printer;
    jj_fan_proof_t fan_proof;
} jj_inputs_t;
typedef struct {
    bool heater_requested;
    uint8_t fan_percent;
    float effective_target_c;
    bool thermal_management_required;
    jj_control_authority_t active_authority;
    jj_control_inhibit_t control_inhibit;
    jj_thermal_state_t thermal_state;
    jj_fault_t fault;
    jj_block_reason_t block_reason;
} jj_outputs_t;
typedef struct { jj_fault_t fault_latched; jj_outputs_t last_output; } jj_interlock_t;

jj_inputs_t jj_inputs_safe_defaults(void);
void jj_interlock_init(jj_interlock_t *state);
jj_outputs_t jj_interlock_step(jj_interlock_t *state, const jj_inputs_t *input);
bool jj_interlock_clear_fault(jj_interlock_t *state, const jj_inputs_t *input);
jj_outputs_t jj_interlock_snapshot(const jj_interlock_t *state);
const char *jj_fault_str(jj_fault_t fault);
const char *jj_block_reason_str(jj_block_reason_t reason);
const char *jj_control_authority_str(jj_control_authority_t authority);
const char *jj_control_inhibit_str(jj_control_inhibit_t inhibit);
const char *jj_thermal_state_str(jj_thermal_state_t state);
