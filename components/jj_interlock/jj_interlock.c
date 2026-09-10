// SPDX-License-Identifier: GPL-3.0-or-later
#include "jj_interlock.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
static portMUX_TYPE s_state_lock = portMUX_INITIALIZER_UNLOCKED;
#define STATE_LOCK() portENTER_CRITICAL(&s_state_lock)
#define STATE_UNLOCK() portEXIT_CRITICAL(&s_state_lock)
#else
#define STATE_LOCK() ((void)0)
#define STATE_UNLOCK() ((void)0)
#endif

static bool sensor_ok(const jj_sensor_sample_t *sensor)
{
    return sensor->status == JJ_SENSOR_OK && isfinite(sensor->temperature_c);
}

static bool sensors_ok(const jj_inputs_t *input)
{
    return sensor_ok(&input->chamber) && sensor_ok(&input->outlet) &&
           sensor_ok(&input->case_sensor);
}

static bool manual_target_valid(float target_c)
{
    return isfinite(target_c) && target_c >= JJ_MANUAL_TARGET_MIN_C &&
           target_c <= JJ_MANUAL_TARGET_MAX_C;
}

jj_inputs_t jj_inputs_safe_defaults(void)
{
    return (jj_inputs_t){
        .commissioned = false,
        .mode = JJ_MODE_OFF,
        .manual_target_c = JJ_MANUAL_TARGET_DEFAULT_C,
        .active_authority = JJ_AUTHORITY_NONE,
        .control_inhibit = JJ_CONTROL_INHIBIT_NONE,
        .chamber = {.status = JJ_SENSOR_UNAVAILABLE},
        .outlet = {.status = JJ_SENSOR_UNAVAILABLE},
        .case_sensor = {.status = JJ_SENSOR_UNAVAILABLE},
        .fan_proof = JJ_FAN_PROOF_UNAVAILABLE,
    };
}

void jj_interlock_init(jj_interlock_t *state)
{
    if (!state) return;
    STATE_LOCK();
    memset(state, 0, sizeof(*state));
    state->last_output.block_reason = JJ_BLOCK_OFF;
    STATE_UNLOCK();
}

static jj_outputs_t blocked(jj_interlock_t *state, jj_block_reason_t reason,
                            const jj_inputs_t *input)
{
    jj_control_inhibit_t control_inhibit = input->control_inhibit;
    if (reason == JJ_BLOCK_CONTROL_NO_AUTHORITY)
        control_inhibit = JJ_CONTROL_INHIBIT_NO_AUTHORITY;
    else if (reason == JJ_BLOCK_STATE_REFRESH_REQUIRED)
        control_inhibit = JJ_CONTROL_INHIBIT_STATE_REFRESH_REQUIRED;
    else if (reason == JJ_BLOCK_PRINTER_UNAVAILABLE ||
             reason == JJ_BLOCK_PRINTER_NOT_PRINTING ||
             reason == JJ_BLOCK_AUTO_POLICY_UNAVAILABLE ||
             reason == JJ_BLOCK_AUTO_AUTHORITY_UNAVAILABLE ||
             reason == JJ_BLOCK_MANUAL_TARGET_INVALID ||
             reason == JJ_BLOCK_FAN_PROOF_PENDING)
        control_inhibit = JJ_CONTROL_INHIBIT_NOT_ELIGIBLE;
    const bool thermal_management = input->cooldown_required ||
        input->fault_requires_thermal_management || state->fault_latched == JJ_FAULT_FAN ||
        reason == JJ_BLOCK_FAN_PROOF_PENDING;
    jj_outputs_t output = {
        .heater_requested = false,
        .fan_percent = thermal_management ? 100 : 0,
        .effective_target_c = 0.0f,
        .thermal_management_required = thermal_management,
        .active_authority = input->active_authority,
        .control_inhibit = control_inhibit,
        .thermal_state = thermal_management ? JJ_THERMAL_COOLDOWN : JJ_THERMAL_IDLE,
        .fault = state->fault_latched,
        .block_reason = reason,
    };
    state->last_output = output;
    return output;
}

static jj_outputs_t step_unlocked(jj_interlock_t *state, const jj_inputs_t *input)
{
    if (!input->commissioned)
        return blocked(state, JJ_BLOCK_NOT_COMMISSIONED, input);
    if (!sensors_ok(input)) state->fault_latched = JJ_FAULT_SENSOR;
    if (input->overtemperature_detected)
        state->fault_latched = JJ_FAULT_OVERTEMPERATURE;
    if (state->fault_latched != JJ_FAULT_NONE)
        return blocked(state, JJ_BLOCK_FAULT_LATCHED, input);
    if (input->mode == JJ_MODE_OFF)
        return blocked(state, JJ_BLOCK_OFF, input);
    if (input->mode != JJ_MODE_MANUAL && input->mode != JJ_MODE_AUTOMATIC)
        return blocked(state, JJ_BLOCK_INVALID_MODE, input);
    if (input->fan_proof == JJ_FAN_PROOF_FAILED ||
        (input->fan_proof != JJ_FAN_PROOF_UNAVAILABLE &&
         input->fan_proof != JJ_FAN_PROOF_PENDING &&
         input->fan_proof != JJ_FAN_PROOF_PROVEN)) {
        state->fault_latched = JJ_FAULT_FAN;
        return blocked(state, JJ_BLOCK_FAULT_LATCHED, input);
    }
    if (input->fan_proof != JJ_FAN_PROOF_PROVEN)
        return blocked(state, JJ_BLOCK_FAN_PROOF_PENDING, input);

    const float target = input->manual_target_c;
    if (input->mode == JJ_MODE_AUTOMATIC) {
        if (input->active_authority != JJ_AUTHORITY_AUTOMATIC)
            return blocked(state, JJ_BLOCK_AUTO_AUTHORITY_UNAVAILABLE, input);
        if (!input->printer.online)
            return blocked(state, JJ_BLOCK_PRINTER_UNAVAILABLE, input);
        if (!input->printer.printing)
            return blocked(state, JJ_BLOCK_PRINTER_NOT_PRINTING, input);
        /* dc_prusa owns its 15 s freshness decision. No second timer lives here. */
        if (!input->automatic_target_available ||
            !isfinite(input->automatic_target_c) ||
            input->automatic_target_c <= 0.0f)
            return blocked(state, JJ_BLOCK_AUTO_POLICY_UNAVAILABLE, input);
        const jj_outputs_t output = {
            .heater_requested = true,
            .fan_percent = 100,
            .effective_target_c = input->automatic_target_c,
            .thermal_management_required = true,
            .active_authority = JJ_AUTHORITY_AUTOMATIC,
            .control_inhibit = JJ_CONTROL_INHIBIT_NONE,
            .thermal_state = JJ_THERMAL_HEATING,
            .fault = JJ_FAULT_NONE,
            .block_reason = JJ_BLOCK_NONE,
        };
        state->last_output = output;
        return output;
    }
    if (input->active_authority == JJ_AUTHORITY_REACQUIRING ||
        input->control_inhibit == JJ_CONTROL_INHIBIT_STATE_REFRESH_REQUIRED)
        return blocked(state, JJ_BLOCK_STATE_REFRESH_REQUIRED, input);
    if (input->active_authority != JJ_AUTHORITY_REMOTE ||
        !input->manual_demand_authorized)
        return blocked(state, JJ_BLOCK_CONTROL_NO_AUTHORITY, input);
    if (!manual_target_valid(target))
        return blocked(state, JJ_BLOCK_MANUAL_TARGET_INVALID, input);
    jj_outputs_t output = {
        .heater_requested = true,
        .fan_percent = 100,
        .effective_target_c = target,
        .thermal_management_required = true,
        .active_authority = JJ_AUTHORITY_REMOTE,
        .control_inhibit = JJ_CONTROL_INHIBIT_NONE,
        .thermal_state = JJ_THERMAL_HEATING,
        .fault = JJ_FAULT_NONE,
        .block_reason = JJ_BLOCK_NONE,
    };
    state->last_output = output;
    return output;
}

jj_outputs_t jj_interlock_step(jj_interlock_t *state, const jj_inputs_t *input)
{
    if (!state || !input)
        return (jj_outputs_t){.fault = JJ_FAULT_SENSOR,
                              .block_reason = JJ_BLOCK_FAULT_LATCHED};
    STATE_LOCK();
    jj_outputs_t output = step_unlocked(state, input);
    STATE_UNLOCK();
    return output;
}

static bool clear_fault_unlocked(jj_interlock_t *state, const jj_inputs_t *input)
{
    if (input->mode != JJ_MODE_OFF || !sensors_ok(input) ||
        input->overtemperature_detected || input->cooldown_required ||
        input->fault_requires_thermal_management)
        return false;
    state->fault_latched = JJ_FAULT_NONE;
    state->last_output = (jj_outputs_t){.block_reason = JJ_BLOCK_OFF};
    return true;
}

bool jj_interlock_clear_fault(jj_interlock_t *state, const jj_inputs_t *input)
{
    if (!state || !input) return false;
    STATE_LOCK();
    bool cleared = clear_fault_unlocked(state, input);
    STATE_UNLOCK();
    return cleared;
}

jj_outputs_t jj_interlock_snapshot(const jj_interlock_t *state)
{
    if (!state)
        return (jj_outputs_t){.fault = JJ_FAULT_SENSOR,
                              .block_reason = JJ_BLOCK_FAULT_LATCHED};
    STATE_LOCK();
    jj_outputs_t output = state->last_output;
    STATE_UNLOCK();
    return output;
}

const char *jj_fault_str(jj_fault_t fault)
{
    switch (fault) {
    case JJ_FAULT_NONE: return "none";
    case JJ_FAULT_SENSOR: return "sensor";
    case JJ_FAULT_OVERTEMPERATURE: return "overtemperature";
    case JJ_FAULT_FAN: return "fan";
    case JJ_FAULT_NO_HEAT: return "no_heat";
    case JJ_FAULT_UNCONTROLLED_RISE: return "uncontrolled_rise";
    case JJ_FAULT_CONFIG: return "config";
    default: return "unknown";
    }
}

const char *jj_block_reason_str(jj_block_reason_t reason)
{
    switch (reason) {
    case JJ_BLOCK_NONE: return "none";
    case JJ_BLOCK_OFF: return "off";
    case JJ_BLOCK_NOT_COMMISSIONED: return "not_commissioned";
    case JJ_BLOCK_FAULT_LATCHED: return "fault_latched";
    case JJ_BLOCK_PRINTER_UNAVAILABLE: return "printer_unavailable";
    case JJ_BLOCK_PRINTER_NOT_PRINTING: return "printer_not_printing";
    case JJ_BLOCK_AUTO_POLICY_UNAVAILABLE: return "automatic_policy_unavailable";
    case JJ_BLOCK_MANUAL_TARGET_INVALID: return "manual_target_invalid";
    case JJ_BLOCK_FAN_PROOF_PENDING: return "fan_proof_pending";
    case JJ_BLOCK_INVALID_MODE: return "invalid_mode";
    case JJ_BLOCK_CONTROL_NO_AUTHORITY: return "control_no_authority";
    case JJ_BLOCK_STATE_REFRESH_REQUIRED: return "state_refresh_required";
    case JJ_BLOCK_AUTO_AUTHORITY_UNAVAILABLE: return "automatic_authority_unavailable";
    default: return "unknown";
    }
}

const char *jj_control_authority_str(jj_control_authority_t authority)
{
    switch (authority) {
    case JJ_AUTHORITY_NONE: return "none";
    case JJ_AUTHORITY_REMOTE: return "remote";
    case JJ_AUTHORITY_AUTOMATIC: return "automatic";
    case JJ_AUTHORITY_REACQUIRING: return "reacquiring";
    default: return "unknown";
    }
}

const char *jj_control_inhibit_str(jj_control_inhibit_t inhibit)
{
    switch (inhibit) {
    case JJ_CONTROL_INHIBIT_NONE: return "none";
    case JJ_CONTROL_INHIBIT_NO_AUTHORITY: return "no_authority";
    case JJ_CONTROL_INHIBIT_STATE_REFRESH_REQUIRED: return "state_refresh_required";
    case JJ_CONTROL_INHIBIT_NOT_ELIGIBLE: return "not_eligible";
    default: return "unknown";
    }
}

const char *jj_thermal_state_str(jj_thermal_state_t state)
{
    switch (state) {
    case JJ_THERMAL_IDLE: return "idle";
    case JJ_THERMAL_HEATING: return "heating";
    case JJ_THERMAL_COOLDOWN: return "cooldown";
    default: return "unknown";
    }
}
