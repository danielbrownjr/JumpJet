#include "jj_interlock.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #condition); \
    exit(1); } } while (0)

static jj_inputs_t nominal(jj_mode_t mode)
{
    jj_inputs_t input = {
        .commissioned = true,
        .mode = mode,
        .manual_target_c = JJ_MANUAL_TARGET_DEFAULT_C,
        .chamber = {.status = JJ_SENSOR_OK, .temperature_c = 30.0f},
        .outlet = {.status = JJ_SENSOR_OK, .temperature_c = 35.0f},
        .case_sensor = {.status = JJ_SENSOR_OK, .temperature_c = 36.0f},
        .printer = {.online = true, .printing = true, .bed_target_c = 100.0f},
        .fan_proof = JJ_FAN_PROOF_PROVEN,
    };
    if (mode == JJ_MODE_MANUAL) {
        input.active_authority = JJ_AUTHORITY_REMOTE;
        input.manual_demand_authorized = true;
    } else if (mode == JJ_MODE_AUTOMATIC) {
        input.active_authority = JJ_AUTHORITY_AUTOMATIC;
    }
    return input;
}

static jj_outputs_t step_once(jj_inputs_t input)
{
    jj_interlock_t state;
    jj_interlock_init(&state);
    return jj_interlock_step(&state, &input);
}

static void check_cold(jj_outputs_t output, jj_block_reason_t reason)
{
    CHECK(!output.heater_requested);
    CHECK(output.effective_target_c == 0.0f);
    CHECK(output.block_reason == reason);
}

static void test_boot_defaults_and_modes(void)
{
    jj_inputs_t safe = jj_inputs_safe_defaults();
    CHECK(!safe.commissioned);
    CHECK(safe.mode == JJ_MODE_OFF);
    CHECK(safe.manual_target_c == JJ_MANUAL_TARGET_DEFAULT_C);
    check_cold(step_once(safe), JJ_BLOCK_NOT_COMMISSIONED);

    jj_inputs_t input = nominal(JJ_MODE_OFF);
    check_cold(step_once(input), JJ_BLOCK_OFF);
    input.mode = (jj_mode_t)99;
    check_cold(step_once(input), JJ_BLOCK_INVALID_MODE);
}

static void test_manual_target_is_rejected_not_clamped(void)
{
    const float accepted[] = {JJ_MANUAL_TARGET_MIN_C, JJ_MANUAL_TARGET_DEFAULT_C,
                              JJ_MANUAL_TARGET_MAX_C};
    for (size_t i = 0; i < sizeof(accepted) / sizeof(accepted[0]); ++i) {
        jj_inputs_t input = nominal(JJ_MODE_MANUAL);
        input.manual_target_c = accepted[i];
        jj_outputs_t output = step_once(input);
        CHECK(output.heater_requested);
        CHECK(output.effective_target_c == accepted[i]);
    }
    const float rejected[] = {29.9f, 50.1f, NAN, INFINITY, -INFINITY};
    for (size_t i = 0; i < sizeof(rejected) / sizeof(rejected[0]); ++i) {
        jj_inputs_t input = nominal(JJ_MODE_MANUAL);
        input.manual_target_c = rejected[i];
        check_cold(step_once(input), JJ_BLOCK_MANUAL_TARGET_INVALID);
    }
}

static void test_automatic_is_whitelisted_and_policy_blocked(void)
{
    jj_inputs_t input = nominal(JJ_MODE_AUTOMATIC);
    input.printer.online = false; /* includes dc_prusa missing/stale result */
    check_cold(step_once(input), JJ_BLOCK_PRINTER_UNAVAILABLE);

    input = nominal(JJ_MODE_AUTOMATIC);
    input.printer.printing = false; /* every state other than exact PRINTING */
    check_cold(step_once(input), JJ_BLOCK_PRINTER_NOT_PRINTING);

    input = nominal(JJ_MODE_AUTOMATIC);
    input.printer.bed_target_c = 0.0f;
    check_cold(step_once(input), JJ_BLOCK_AUTO_POLICY_UNAVAILABLE);
    input.printer.bed_target_c = 200.0f;
    check_cold(step_once(input), JJ_BLOCK_AUTO_POLICY_UNAVAILABLE);
    input.printer.bed_target_c = NAN;
    check_cold(step_once(input), JJ_BLOCK_AUTO_POLICY_UNAVAILABLE);
}

static void test_automatic_authority_and_eligibility_are_independent(void)
{
    jj_inputs_t input = nominal(JJ_MODE_AUTOMATIC);
    input.automatic_target_available = true;
    input.automatic_target_c = 42.0f;
    jj_outputs_t output = step_once(input);
    CHECK(output.heater_requested);
    CHECK(output.active_authority == JJ_AUTHORITY_AUTOMATIC);
    CHECK(output.thermal_state == JJ_THERMAL_HEATING);

    input.active_authority = JJ_AUTHORITY_NONE;
    check_cold(step_once(input), JJ_BLOCK_AUTO_AUTHORITY_UNAVAILABLE);
    input.active_authority = JJ_AUTHORITY_AUTOMATIC;
    input.printer.online = false;
    output = step_once(input);
    check_cold(output, JJ_BLOCK_PRINTER_UNAVAILABLE);
    CHECK(output.control_inhibit == JJ_CONTROL_INHIBIT_NOT_ELIGIBLE);
    CHECK(output.fault == JJ_FAULT_NONE);
}

static void test_faults_latch_and_need_explicit_safe_clear(void)
{
    jj_interlock_t state;
    jj_interlock_init(&state);
    jj_inputs_t input = nominal(JJ_MODE_MANUAL);
    input.chamber.status = JJ_SENSOR_OPEN;
    jj_outputs_t output = jj_interlock_step(&state, &input);
    CHECK(output.fault == JJ_FAULT_SENSOR);
    check_cold(output, JJ_BLOCK_FAULT_LATCHED);

    input = nominal(JJ_MODE_MANUAL);
    CHECK(jj_interlock_step(&state, &input).fault == JJ_FAULT_SENSOR);
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.mode = JJ_MODE_OFF;
    input.cooldown_required = true;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.cooldown_required = false;
    CHECK(jj_interlock_clear_fault(&state, &input));

    jj_interlock_init(&state);
    input = nominal(JJ_MODE_MANUAL);
    input.overtemperature_detected = true;
    output = jj_interlock_step(&state, &input);
    CHECK(output.fault == JJ_FAULT_OVERTEMPERATURE);
    check_cold(output, JJ_BLOCK_FAULT_LATCHED);
}

static void test_off_keeps_thermal_management_request(void)
{
    jj_inputs_t input = nominal(JJ_MODE_OFF);
    input.cooldown_required = true;
    jj_outputs_t output = step_once(input);
    check_cold(output, JJ_BLOCK_OFF);
    CHECK(output.thermal_management_required);
    CHECK(output.fan_percent == 100);

    input.cooldown_required = false;
    input.fault_requires_thermal_management = true;
    output = step_once(input);
    CHECK(output.thermal_management_required);
    CHECK(output.fan_percent == 100);
}

static void test_fan_proof_and_null_inputs_fail_cold(void)
{
    jj_inputs_t input = nominal(JJ_MODE_MANUAL);
    input.fan_proof = JJ_FAN_PROOF_PENDING;
    check_cold(step_once(input), JJ_BLOCK_FAN_PROOF_PENDING);
    input.fan_proof = JJ_FAN_PROOF_FAILED;
    jj_outputs_t output = step_once(input);
    CHECK(output.fault == JJ_FAULT_FAN);
    check_cold(output, JJ_BLOCK_FAULT_LATCHED);

    jj_interlock_t state;
    jj_interlock_init(&state);
    check_cold(jj_interlock_step(NULL, &input), JJ_BLOCK_FAULT_LATCHED);
    check_cold(jj_interlock_step(&state, NULL), JJ_BLOCK_FAULT_LATCHED);
}

static void test_remote_fault_acknowledgement_policy(void)
{
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_SENSOR) ==
          JJ_REMOTE_ACK_WHEN_HEALTHY);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_OVERTEMPERATURE) ==
          JJ_REMOTE_ACK_AFTER_REVALIDATION);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_FAN) ==
          JJ_REMOTE_ACK_AFTER_REVALIDATION);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_NO_HEAT) ==
          JJ_REMOTE_ACK_AFTER_REVALIDATION);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_WATCHDOG_RESET) ==
          JJ_REMOTE_ACK_AFTER_REVALIDATION);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_UNEXPECTED_RESET) ==
          JJ_REMOTE_ACK_AFTER_REVALIDATION);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_BROWNOUT_RESET) ==
          JJ_REMOTE_ACK_AFTER_REVALIDATION);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_UNCONTROLLED_RISE) ==
          JJ_REMOTE_ACK_NEVER);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_CONFIG) == JJ_REMOTE_ACK_NEVER);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_STUCK_ON) == JJ_REMOTE_ACK_NEVER);
    CHECK(jj_fault_remote_ack_policy(JJ_FAULT_COMMANDED_OFF_PROOF) ==
          JJ_REMOTE_ACK_NEVER);
    CHECK(jj_fault_remote_ack_policy((jj_fault_t)999) == JJ_REMOTE_ACK_NEVER);

    jj_interlock_t state;
    jj_interlock_init(&state);
    jj_inputs_t input = nominal(JJ_MODE_OFF);

    state.fault_latched = JJ_FAULT_SENSOR;
    CHECK(jj_interlock_clear_fault(&state, &input));

    state.fault_latched = JJ_FAULT_SENSOR;
    input.chamber.status = JJ_SENSOR_IMPLAUSIBLE;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.chamber.status = JJ_SENSOR_OPEN;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.chamber.status = JJ_SENSOR_SHORT;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.chamber.status = JJ_SENSOR_OK;
    input.chamber.temperature_c = NAN;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.chamber.temperature_c = 30.0f;

    state.fault_latched = JJ_FAULT_FAN;
    input.fan_proof = JJ_FAN_PROOF_PENDING;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.fan_proof = JJ_FAN_PROOF_PROVEN;
    CHECK(jj_interlock_clear_fault(&state, &input));

    state.fault_latched = JJ_FAULT_OVERTEMPERATURE;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.overtemperature_reset_proven = true;
    input.cooldown_required = true;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.cooldown_required = false;
    CHECK(jj_interlock_clear_fault(&state, &input));

    state.fault_latched = JJ_FAULT_NO_HEAT;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.no_heat_revalidation_proven = true;
    CHECK(jj_interlock_clear_fault(&state, &input));

    state.fault_latched = JJ_FAULT_UNCONTROLLED_RISE;
    input.reset_revalidation_proven = true;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    state.fault_latched = JJ_FAULT_CONFIG;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    state.fault_latched = (jj_fault_t)999;
    CHECK(!jj_interlock_clear_fault(&state, &input));

    state.fault_latched = JJ_FAULT_WATCHDOG_RESET;
    CHECK(jj_interlock_clear_fault(&state, &input));
}

int main(void)
{
    test_boot_defaults_and_modes();
    test_manual_target_is_rejected_not_clamped();
    test_automatic_is_whitelisted_and_policy_blocked();
    test_automatic_authority_and_eligibility_are_independent();
    test_faults_latch_and_need_explicit_safe_clear();
    test_off_keeps_thermal_management_request();
    test_fan_proof_and_null_inputs_fail_cold();
    test_remote_fault_acknowledgement_policy();
    puts("jj_interlock_host_test: PASS");
    return 0;
}
