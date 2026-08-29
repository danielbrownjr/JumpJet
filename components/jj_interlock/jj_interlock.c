// SPDX-License-Identifier: GPL-3.0-or-later
#include "jj_interlock.h"
#include "jj_provisional_limits.h"
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
    return sensor_ok(&input->chamber) &&
           sensor_ok(&input->outlet) &&
           sensor_ok(&input->case_sensor);
}

static bool cooling_required(const jj_inputs_t *input, float release_c)
{
    // A blocked controller may stop cooling only when every required sensor
    // authoritatively proves a cool condition. Any uncertainty biases fans on.
    if (!sensors_ok(input)) return true;
    return input->chamber.temperature_c > release_c ||
           input->outlet.temperature_c > release_c ||
           input->case_sensor.temperature_c > release_c;
}

static bool safely_cool(const jj_interlock_t *state, const jj_inputs_t *input)
{
    return sensors_ok(input) &&
           input->chamber.temperature_c <= state->config.cooldown_release_c &&
           input->outlet.temperature_c <= state->config.cooldown_release_c &&
           input->case_sensor.temperature_c <= state->config.cooldown_release_c;
}

static bool config_valid(const jj_interlock_config_t *config)
{
    if (!config ||
        !isfinite(config->maximum_target_c) ||
        !isfinite(config->chamber_hard_limit_c) ||
        !isfinite(config->outlet_hard_limit_c) ||
        !isfinite(config->case_hard_limit_c) ||
        !isfinite(config->cooldown_release_c) ||
        !isfinite(config->auto_bed_threshold_c) ||
        !isfinite(config->auto_chamber_target_c)) return false;

    // Product configuration may make these provisional ceilings stricter, but
    // must never raise the compiled safety limits or invert their ordering.
    return config->maximum_target_c > 0.0f &&
           config->maximum_target_c <= JJ_PROVISIONAL_MAXIMUM_TARGET_C &&
           config->chamber_hard_limit_c > config->maximum_target_c &&
           config->chamber_hard_limit_c <= JJ_PROVISIONAL_CHAMBER_HARD_LIMIT_C &&
           config->outlet_hard_limit_c > config->maximum_target_c &&
           config->outlet_hard_limit_c <= JJ_PROVISIONAL_OUTLET_HARD_LIMIT_C &&
           config->case_hard_limit_c > config->maximum_target_c &&
           config->case_hard_limit_c <= JJ_PROVISIONAL_CASE_HARD_LIMIT_C &&
           config->cooldown_release_c >= 0.0f &&
           config->cooldown_release_c <= JJ_PROVISIONAL_COOLDOWN_RELEASE_C &&
           config->cooldown_release_c < config->maximum_target_c &&
           config->auto_bed_threshold_c >= JJ_PROVISIONAL_AUTO_BED_THRESHOLD_C &&
           config->auto_chamber_target_c > 0.0f &&
           config->printer_stale_ms > 0 &&
           config->printer_stale_ms <= JJ_PROVISIONAL_PRINTER_STALE_MS;
}

jj_interlock_config_t jj_interlock_default_config(void)
{
    return (jj_interlock_config_t){
        .maximum_target_c = JJ_PROVISIONAL_MAXIMUM_TARGET_C,
        .chamber_hard_limit_c = JJ_PROVISIONAL_CHAMBER_HARD_LIMIT_C,
        .outlet_hard_limit_c = JJ_PROVISIONAL_OUTLET_HARD_LIMIT_C,
        .case_hard_limit_c = JJ_PROVISIONAL_CASE_HARD_LIMIT_C,
        .cooldown_release_c = JJ_PROVISIONAL_COOLDOWN_RELEASE_C,
        .auto_bed_threshold_c = JJ_PROVISIONAL_AUTO_BED_THRESHOLD_C,
        .auto_chamber_target_c = JJ_PROVISIONAL_AUTO_CHAMBER_TARGET_C,
        .printer_stale_ms = JJ_PROVISIONAL_PRINTER_STALE_MS,
    };
}

jj_inputs_t jj_inputs_safe_defaults(void)
{
    return (jj_inputs_t){
        .commissioned = false,
        .mode = JJ_MODE_OFF,
        .requested_target_c = 0.0f,
        .chamber = {.status = JJ_SENSOR_UNAVAILABLE},
        .outlet = {.status = JJ_SENSOR_UNAVAILABLE},
        .case_sensor = {.status = JJ_SENSOR_UNAVAILABLE},
        .printer = {.sample_age_ms = UINT32_MAX},
        .fan_proof = JJ_FAN_PROOF_UNAVAILABLE,
    };
}

void jj_interlock_init(jj_interlock_t *state, const jj_interlock_config_t *config)
{
    if (!state) return;
    STATE_LOCK();
    memset(state, 0, sizeof(*state));
    state->config = config ? *config : jj_interlock_default_config();
    state->last_output.block_reason = JJ_BLOCK_OFF;
    STATE_UNLOCK();
}

static jj_outputs_t blocked(jj_interlock_t *state, jj_block_reason_t reason,
                            const jj_inputs_t *input)
{
    jj_outputs_t output = {
        .heater_requested = false, .fan_percent = 0, .effective_target_c = 0.0f,
        .fault = state->fault_latched, .block_reason = reason,
    };
    const float cooldown_release_c = config_valid(&state->config) ?
        state->config.cooldown_release_c : JJ_PROVISIONAL_COOLDOWN_RELEASE_C;
    if (reason == JJ_BLOCK_FAN_PROOF_PENDING ||
        state->fault_latched == JJ_FAULT_FAN ||
        cooling_required(input, cooldown_release_c)) {
        output.fan_percent = 100;
    }
    state->last_output = output;
    return output;
}

static jj_outputs_t step_unlocked(jj_interlock_t *state, const jj_inputs_t *input)
{
    if (!config_valid(&state->config)) {
        state->fault_latched = JJ_FAULT_CONFIG;
        return blocked(state, JJ_BLOCK_FAULT_LATCHED, input);
    }
    if (!input->commissioned)
        return blocked(state, JJ_BLOCK_NOT_COMMISSIONED, input);
    if (!sensors_ok(input)) state->fault_latched = JJ_FAULT_SENSOR;
    // Each remaining authoritative sensor can independently raise a thermal
    // trip even when another sensor has failed.
    if ((sensor_ok(&input->chamber) &&
         input->chamber.temperature_c >= state->config.chamber_hard_limit_c) ||
        (sensor_ok(&input->outlet) &&
         input->outlet.temperature_c >= state->config.outlet_hard_limit_c) ||
        (sensor_ok(&input->case_sensor) &&
         input->case_sensor.temperature_c >= state->config.case_hard_limit_c))
        state->fault_latched = JJ_FAULT_OVERTEMPERATURE;
    if (state->fault_latched != JJ_FAULT_NONE)
        return blocked(state, JJ_BLOCK_FAULT_LATCHED, input);
    if (input->mode == JJ_MODE_OFF) return blocked(state, JJ_BLOCK_OFF, input);
    if (input->mode != JJ_MODE_MANUAL && input->mode != JJ_MODE_AUTO)
        return blocked(state, JJ_BLOCK_INVALID_MODE, input);

    float target = input->requested_target_c;
    if (input->mode == JJ_MODE_AUTO) {
        if (!input->printer.online)
            return blocked(state, JJ_BLOCK_PRINTER_OFFLINE, input);
        if (input->printer.sample_age_ms == UINT32_MAX ||
            input->printer.sample_age_ms > state->config.printer_stale_ms)
            return blocked(state, JJ_BLOCK_PRINTER_STALE, input);
        if (!input->printer.printing)
            return blocked(state, JJ_BLOCK_PRINTER_STOPPED, input);
        if (!isfinite(input->printer.bed_target_c) ||
            input->printer.bed_target_c < state->config.auto_bed_threshold_c)
            return blocked(state, JJ_BLOCK_BED_TARGET_LOW, input);
        target = state->config.auto_chamber_target_c;
    }
    if (!isfinite(target) || target <= 0.0f)
        return blocked(state, JJ_BLOCK_OFF, input);
    if (target > state->config.maximum_target_c) target = state->config.maximum_target_c;
    if (input->fan_proof == JJ_FAN_PROOF_FAILED ||
        (input->fan_proof != JJ_FAN_PROOF_UNAVAILABLE &&
         input->fan_proof != JJ_FAN_PROOF_PENDING &&
         input->fan_proof != JJ_FAN_PROOF_PROVEN)) {
        state->fault_latched = JJ_FAULT_FAN;
        return blocked(state, JJ_BLOCK_FAULT_LATCHED, input);
    }
    if (input->fan_proof != JJ_FAN_PROOF_PROVEN)
        return blocked(state, JJ_BLOCK_FAN_PROOF_PENDING, input);
    jj_outputs_t output = {
        .heater_requested = true, .fan_percent = 100, .effective_target_c = target,
        .fault = JJ_FAULT_NONE, .block_reason = JJ_BLOCK_NONE,
    };
    state->last_output = output;
    return output;
}

jj_outputs_t jj_interlock_step(jj_interlock_t *state, const jj_inputs_t *input)
{
    if (!state || !input)
        return (jj_outputs_t){
            .fan_percent = 100,
            .effective_target_c = 0.0f,
            .fault = JJ_FAULT_SENSOR,
            .block_reason = JJ_BLOCK_FAULT_LATCHED,
        };
    STATE_LOCK();
    jj_outputs_t output = step_unlocked(state, input);
    STATE_UNLOCK();
    return output;
}

static bool clear_fault_unlocked(jj_interlock_t *state, const jj_inputs_t *input)
{
    if (!config_valid(&state->config) ||
        input->mode != JJ_MODE_OFF || !safely_cool(state, input))
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
        return (jj_outputs_t){
            .fan_percent = 100,
            .fault = JJ_FAULT_SENSOR,
            .block_reason = JJ_BLOCK_FAULT_LATCHED,
        };
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
    case JJ_BLOCK_PRINTER_OFFLINE: return "printer_offline";
    case JJ_BLOCK_PRINTER_STALE: return "printer_stale";
    case JJ_BLOCK_PRINTER_STOPPED: return "printer_stopped";
    case JJ_BLOCK_BED_TARGET_LOW: return "bed_target_low";
    case JJ_BLOCK_FAN_PROOF_PENDING: return "fan_proof_pending";
    case JJ_BLOCK_INVALID_MODE: return "invalid_mode";
    default: return "unknown";
    }
}
