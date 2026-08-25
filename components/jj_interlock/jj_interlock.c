// SPDX-License-Identifier: GPL-3.0-or-later
#include "jj_interlock.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static bool sensors_ok(const jj_inputs_t *input)
{
    return input->chamber.status == JJ_SENSOR_OK &&
           input->outlet.status == JJ_SENSOR_OK &&
           input->case_sensor.status == JJ_SENSOR_OK &&
           isfinite(input->chamber.temperature_c) &&
           isfinite(input->outlet.temperature_c) &&
           isfinite(input->case_sensor.temperature_c);
}

static bool safely_cool(const jj_interlock_t *state, const jj_inputs_t *input)
{
    return sensors_ok(input) &&
           input->outlet.temperature_c <= state->config.cooldown_release_c &&
           input->case_sensor.temperature_c <= state->config.cooldown_release_c;
}

jj_interlock_config_t jj_interlock_default_config(void)
{
    return (jj_interlock_config_t){
        .maximum_target_c = 60.0f,
        .case_hard_limit_c = 72.0f,
        .cooldown_release_c = 40.0f,
        .auto_bed_threshold_c = 80.0f,
        .auto_chamber_target_c = 50.0f,
        .printer_stale_ms = 12000,
    };
}

void jj_interlock_init(jj_interlock_t *state, const jj_interlock_config_t *config)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->config = config ? *config : jj_interlock_default_config();
    state->last_output.block_reason = JJ_BLOCK_OFF;
}

static jj_outputs_t blocked(jj_interlock_t *state, jj_block_reason_t reason,
                            const jj_inputs_t *input)
{
    jj_outputs_t output = {
        .heater_requested = false, .fan_percent = 0, .effective_target_c = 0.0f,
        .fault = state->fault_latched, .block_reason = reason,
    };
    if (sensors_ok(input) &&
        (input->outlet.temperature_c > state->config.cooldown_release_c ||
         input->case_sensor.temperature_c > state->config.cooldown_release_c)) {
        output.fan_percent = 100;
    }
    state->last_output = output;
    return output;
}

jj_outputs_t jj_interlock_step(jj_interlock_t *state, const jj_inputs_t *input)
{
    if (!state || !input)
        return (jj_outputs_t){.block_reason = JJ_BLOCK_FAULT_LATCHED};
    if (!input->commissioned)
        return blocked(state, JJ_BLOCK_NOT_COMMISSIONED, input);
    if (!sensors_ok(input)) state->fault_latched = JJ_FAULT_SENSOR;
    if (sensors_ok(input) &&
        input->case_sensor.temperature_c >= state->config.case_hard_limit_c)
        state->fault_latched = JJ_FAULT_OVERTEMPERATURE;
    if (state->fault_latched != JJ_FAULT_NONE)
        return blocked(state, JJ_BLOCK_FAULT_LATCHED, input);
    if (input->mode == JJ_MODE_OFF) return blocked(state, JJ_BLOCK_OFF, input);

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
    jj_outputs_t output = {
        .heater_requested = true, .fan_percent = 100, .effective_target_c = target,
        .fault = JJ_FAULT_NONE, .block_reason = JJ_BLOCK_NONE,
    };
    state->last_output = output;
    return output;
}

bool jj_interlock_clear_fault(jj_interlock_t *state, const jj_inputs_t *input)
{
    if (!state || !input || input->mode != JJ_MODE_OFF || !safely_cool(state, input))
        return false;
    state->fault_latched = JJ_FAULT_NONE;
    state->last_output = (jj_outputs_t){.block_reason = JJ_BLOCK_OFF};
    return true;
}

jj_outputs_t jj_interlock_snapshot(const jj_interlock_t *state)
{
    return state ? state->last_output :
        (jj_outputs_t){.block_reason = JJ_BLOCK_FAULT_LATCHED};
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
    default: return "unknown";
    }
}
