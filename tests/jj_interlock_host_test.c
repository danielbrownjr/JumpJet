#include "jj_interlock.h"
#include "jj_provisional_limits.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #condition); \
    exit(1); } } while (0)

static jj_inputs_t nominal(void)
{
    return (jj_inputs_t){
        .commissioned = true, .mode = JJ_MODE_AUTO,
        .chamber = {.status = JJ_SENSOR_OK, .temperature_c = 30.0f},
        .outlet = {.status = JJ_SENSOR_OK, .temperature_c = 35.0f},
        .case_sensor = {.status = JJ_SENSOR_OK, .temperature_c = 36.0f},
        .printer = {.online = true, .printing = true, .bed_target_c = 100.0f,
                    .sample_age_ms = 0},
        .fan_proven = true,
    };
}

static void test_uncommissioned_is_cold(void)
{
    jj_interlock_t state; jj_interlock_init(&state, NULL);
    jj_inputs_t input = nominal(); input.commissioned = false;
    jj_outputs_t output = jj_interlock_step(&state, &input);
    CHECK(!output.heater_requested);
    CHECK(output.block_reason == JJ_BLOCK_NOT_COMMISSIONED);
    CHECK(output.fault == JJ_FAULT_NONE);
}

static void test_auto_happy_path_and_clamp(void)
{
    jj_interlock_t state;
    jj_interlock_config_t config = jj_interlock_default_config();
    config.auto_chamber_target_c = 70.0f;
    jj_interlock_init(&state, &config);
    jj_inputs_t input = nominal();
    jj_outputs_t output = jj_interlock_step(&state, &input);
    CHECK(output.heater_requested);
    CHECK(output.effective_target_c == config.maximum_target_c);
    CHECK(output.fan_percent == 100);
}

static void test_auto_fails_cold_on_printer_conditions(void)
{
    jj_interlock_t state; jj_inputs_t input = nominal();
    jj_interlock_init(&state, NULL); input.printer.online = false;
    CHECK(jj_interlock_step(&state, &input).block_reason == JJ_BLOCK_PRINTER_OFFLINE);
    jj_interlock_init(&state, NULL); input = nominal(); input.printer.sample_age_ms = 12000;
    CHECK(jj_interlock_step(&state, &input).heater_requested);
    jj_interlock_init(&state, NULL); input = nominal(); input.printer.sample_age_ms = 12001;
    CHECK(jj_interlock_step(&state, &input).block_reason == JJ_BLOCK_PRINTER_STALE);
    jj_interlock_init(&state, NULL); input = nominal(); input.printer.sample_age_ms = UINT32_MAX;
    CHECK(jj_interlock_step(&state, &input).block_reason == JJ_BLOCK_PRINTER_STALE);
    jj_interlock_init(&state, NULL); input = nominal(); input.printer.printing = false;
    CHECK(jj_interlock_step(&state, &input).block_reason == JJ_BLOCK_PRINTER_STOPPED);
    jj_interlock_init(&state, NULL); input = nominal(); input.printer.bed_target_c = 79.9f;
    CHECK(jj_interlock_step(&state, &input).block_reason == JJ_BLOCK_BED_TARGET_LOW);
}

static void test_sensor_fault_latches_and_requires_safe_clear(void)
{
    jj_interlock_t state; jj_interlock_init(&state, NULL);
    jj_inputs_t input = nominal(); input.chamber.status = JJ_SENSOR_OPEN;
    jj_outputs_t output = jj_interlock_step(&state, &input);
    CHECK(output.fault == JJ_FAULT_SENSOR);
    CHECK(output.block_reason == JJ_BLOCK_FAULT_LATCHED);
    input = nominal();
    CHECK(jj_interlock_step(&state, &input).fault == JJ_FAULT_SENSOR);
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input.mode = JJ_MODE_OFF;
    CHECK(jj_interlock_clear_fault(&state, &input));
    CHECK(jj_interlock_step(&state, &input).fault == JJ_FAULT_NONE);
}

static void test_overtemperature_forces_heat_off_and_cooldown(void)
{
    jj_interlock_t state; jj_interlock_init(&state, NULL);
    jj_inputs_t input = nominal(); input.case_sensor.temperature_c = 72.0f;
    jj_outputs_t output = jj_interlock_step(&state, &input);
    CHECK(!output.heater_requested);
    CHECK(output.fault == JJ_FAULT_OVERTEMPERATURE);
    CHECK(output.fan_percent == 100);
}

static jj_outputs_t step_once(jj_interlock_config_t config, jj_inputs_t input)
{
    jj_interlock_t state;
    jj_interlock_init(&state, &config);
    return jj_interlock_step(&state, &input);
}

static void check_cold(jj_outputs_t output, jj_block_reason_t reason)
{
    CHECK(!output.heater_requested);
    CHECK(output.block_reason == reason);
}

static void expect_config_fault(jj_interlock_config_t config)
{
    jj_outputs_t output = step_once(config, nominal());
    check_cold(output, JJ_BLOCK_FAULT_LATCHED);
    CHECK(output.fault == JJ_FAULT_CONFIG);
}

static void test_named_provisional_defaults_and_invalid_config(void)
{
    jj_interlock_config_t config = jj_interlock_default_config();
    CHECK(config.maximum_target_c == JJ_PROVISIONAL_MAXIMUM_TARGET_C);
    CHECK(config.case_hard_limit_c == JJ_PROVISIONAL_CASE_HARD_LIMIT_C);
    CHECK(config.cooldown_release_c == JJ_PROVISIONAL_COOLDOWN_RELEASE_C);
    CHECK(config.auto_bed_threshold_c == JJ_PROVISIONAL_AUTO_BED_THRESHOLD_C);
    CHECK(config.auto_chamber_target_c == JJ_PROVISIONAL_AUTO_CHAMBER_TARGET_C);
    CHECK(config.printer_stale_ms == JJ_PROVISIONAL_PRINTER_STALE_MS);

    config.maximum_target_c = NAN; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.maximum_target_c = 0.0f; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.maximum_target_c = JJ_PROVISIONAL_MAXIMUM_TARGET_C + 0.1f;
    expect_config_fault(config);
    config = jj_interlock_default_config();
    config.case_hard_limit_c = INFINITY; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.case_hard_limit_c = config.maximum_target_c; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.case_hard_limit_c = JJ_PROVISIONAL_CASE_HARD_LIMIT_C + 0.1f;
    expect_config_fault(config);
    config = jj_interlock_default_config();
    config.cooldown_release_c = -0.1f; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.cooldown_release_c = NAN; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.cooldown_release_c = JJ_PROVISIONAL_COOLDOWN_RELEASE_C + 0.1f;
    expect_config_fault(config);
    config = jj_interlock_default_config();
    config.cooldown_release_c = config.maximum_target_c; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.auto_bed_threshold_c = NAN; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.auto_bed_threshold_c = JJ_PROVISIONAL_AUTO_BED_THRESHOLD_C - 0.1f;
    expect_config_fault(config);
    config = jj_interlock_default_config();
    config.auto_chamber_target_c = NAN; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.auto_chamber_target_c = -1.0f; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.printer_stale_ms = 0; expect_config_fault(config);
    config = jj_interlock_default_config();
    config.printer_stale_ms = JJ_PROVISIONAL_PRINTER_STALE_MS + 1U;
    expect_config_fault(config);
}

static void test_null_mode_and_target_fail_cold_boundaries(void)
{
    jj_interlock_t state;
    jj_interlock_init(&state, NULL);
    jj_inputs_t input = nominal();
    check_cold(jj_interlock_step(NULL, &input), JJ_BLOCK_FAULT_LATCHED);
    check_cold(jj_interlock_step(&state, NULL), JJ_BLOCK_FAULT_LATCHED);

    input.mode = JJ_MODE_OFF;
    check_cold(step_once(jj_interlock_default_config(), input), JJ_BLOCK_OFF);
    input.mode = (jj_mode_t)99;
    check_cold(step_once(jj_interlock_default_config(), input), JJ_BLOCK_INVALID_MODE);

    input = nominal(); input.mode = JJ_MODE_MANUAL; input.requested_target_c = -0.1f;
    check_cold(step_once(jj_interlock_default_config(), input), JJ_BLOCK_OFF);
    input.requested_target_c = 0.0f;
    check_cold(step_once(jj_interlock_default_config(), input), JJ_BLOCK_OFF);
    input.requested_target_c = NAN;
    check_cold(step_once(jj_interlock_default_config(), input), JJ_BLOCK_OFF);
    input.requested_target_c = INFINITY;
    check_cold(step_once(jj_interlock_default_config(), input), JJ_BLOCK_OFF);
    input.requested_target_c = JJ_PROVISIONAL_MAXIMUM_TARGET_C - 0.1f;
    CHECK(step_once(jj_interlock_default_config(), input).effective_target_c ==
          input.requested_target_c);
    input.requested_target_c = JJ_PROVISIONAL_MAXIMUM_TARGET_C;
    CHECK(step_once(jj_interlock_default_config(), input).effective_target_c ==
          JJ_PROVISIONAL_MAXIMUM_TARGET_C);
    input.requested_target_c = JJ_PROVISIONAL_MAXIMUM_TARGET_C + 0.1f;
    CHECK(step_once(jj_interlock_default_config(), input).effective_target_c ==
          JJ_PROVISIONAL_MAXIMUM_TARGET_C);
}

static void test_auto_printer_boundaries_and_unknown_age(void)
{
    const jj_interlock_config_t config = jj_interlock_default_config();
    jj_inputs_t input = nominal();
    input.printer.sample_age_ms = config.printer_stale_ms - 1U;
    CHECK(step_once(config, input).heater_requested);
    input.printer.sample_age_ms = config.printer_stale_ms;
    CHECK(step_once(config, input).heater_requested);
    input.printer.sample_age_ms = config.printer_stale_ms + 1U;
    check_cold(step_once(config, input), JJ_BLOCK_PRINTER_STALE);
    // dc_prusa supplies an already-derived age. Sentinel-scale/wrapped values
    // are compared directly, without subtraction, and therefore fail stale.
    input.printer.sample_age_ms = UINT32_MAX - 1U;
    check_cold(step_once(config, input), JJ_BLOCK_PRINTER_STALE);
    input.printer.sample_age_ms = UINT32_MAX;
    check_cold(step_once(config, input), JJ_BLOCK_PRINTER_STALE);

    input = nominal(); input.printer.bed_target_c = config.auto_bed_threshold_c - 0.1f;
    check_cold(step_once(config, input), JJ_BLOCK_BED_TARGET_LOW);
    input.printer.bed_target_c = config.auto_bed_threshold_c;
    CHECK(step_once(config, input).heater_requested);
    input.printer.bed_target_c = config.auto_bed_threshold_c + 0.1f;
    CHECK(step_once(config, input).heater_requested);
    input.printer.bed_target_c = NAN;
    check_cold(step_once(config, input), JJ_BLOCK_BED_TARGET_LOW);
    input.printer.bed_target_c = INFINITY;
    check_cold(step_once(config, input), JJ_BLOCK_BED_TARGET_LOW);
}

static jj_sensor_sample_t *sensor_at(jj_inputs_t *input, size_t sensor)
{
    if (sensor == 0) return &input->chamber;
    if (sensor == 1) return &input->outlet;
    return &input->case_sensor;
}

static void test_all_sensor_invalid_and_fan_fault_inputs(void)
{
    const jj_sensor_status_t invalid_statuses[] = {
        JJ_SENSOR_UNAVAILABLE, JJ_SENSOR_OPEN, JJ_SENSOR_SHORT,
        JJ_SENSOR_IMPLAUSIBLE,
    };
    for (size_t sensor = 0; sensor < 3; sensor++) {
        for (size_t status = 0;
             status < sizeof(invalid_statuses) / sizeof(invalid_statuses[0]); status++) {
            jj_inputs_t input = nominal();
            sensor_at(&input, sensor)->status = invalid_statuses[status];
            jj_outputs_t output = step_once(jj_interlock_default_config(), input);
            check_cold(output, JJ_BLOCK_FAULT_LATCHED);
            CHECK(output.fault == JJ_FAULT_SENSOR);
        }
        jj_inputs_t input = nominal();
        sensor_at(&input, sensor)->temperature_c = NAN;
        CHECK(step_once(jj_interlock_default_config(), input).fault == JJ_FAULT_SENSOR);
        input = nominal();
        sensor_at(&input, sensor)->temperature_c = INFINITY;
        CHECK(step_once(jj_interlock_default_config(), input).fault == JJ_FAULT_SENSOR);
    }

    jj_inputs_t input = nominal(); input.fan_proven = false;
    jj_outputs_t output = step_once(jj_interlock_default_config(), input);
    check_cold(output, JJ_BLOCK_FAULT_LATCHED);
    CHECK(output.fault == JJ_FAULT_FAN);
}

static void test_temperature_and_fault_clear_boundaries(void)
{
    const jj_interlock_config_t config = jj_interlock_default_config();
    jj_inputs_t input = nominal();
    input.case_sensor.temperature_c = config.case_hard_limit_c - 0.1f;
    CHECK(step_once(config, input).heater_requested);
    input.case_sensor.temperature_c = config.case_hard_limit_c;
    CHECK(step_once(config, input).fault == JJ_FAULT_OVERTEMPERATURE);
    input.case_sensor.temperature_c = config.case_hard_limit_c + 0.1f;
    CHECK(step_once(config, input).fault == JJ_FAULT_OVERTEMPERATURE);

    jj_interlock_t state;
    jj_interlock_init(&state, &config);
    input = nominal(); input.chamber.status = JJ_SENSOR_OPEN;
    (void)jj_interlock_step(&state, &input);
    input = nominal();
    CHECK(jj_interlock_step(&state, &input).fault == JJ_FAULT_SENSOR);
    CHECK(!jj_interlock_clear_fault(NULL, &input));
    CHECK(!jj_interlock_clear_fault(&state, NULL));
    CHECK(!jj_interlock_clear_fault(&state, &input));
    jj_interlock_config_t invalid_config = config;
    invalid_config.maximum_target_c = JJ_PROVISIONAL_MAXIMUM_TARGET_C + 0.1f;
    jj_interlock_t invalid_state;
    jj_interlock_init(&invalid_state, &invalid_config);
    jj_inputs_t off_input = nominal(); off_input.mode = JJ_MODE_OFF;
    CHECK(!jj_interlock_clear_fault(&invalid_state, &off_input));
    input.mode = JJ_MODE_OFF; input.outlet.status = JJ_SENSOR_OPEN;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    input = nominal(); input.mode = JJ_MODE_OFF;
    input.outlet.temperature_c = config.cooldown_release_c + 0.1f;
    CHECK(!jj_interlock_clear_fault(&state, &input));
    CHECK(jj_interlock_step(&state, &input).fan_percent == 100);
    input.outlet.temperature_c = config.cooldown_release_c;
    CHECK(jj_interlock_clear_fault(&state, &input));
    CHECK(jj_interlock_step(&state, &input).fan_percent == 0);

    jj_interlock_init(&state, &config);
    input = nominal(); input.chamber.status = JJ_SENSOR_SHORT;
    (void)jj_interlock_step(&state, &input);
    input = nominal(); input.mode = JJ_MODE_OFF;
    input.case_sensor.temperature_c = config.cooldown_release_c - 0.1f;
    CHECK(jj_interlock_clear_fault(&state, &input));
}

int main(void)
{
    test_uncommissioned_is_cold();
    test_auto_happy_path_and_clamp();
    test_auto_fails_cold_on_printer_conditions();
    test_sensor_fault_latches_and_requires_safe_clear();
    test_overtemperature_forces_heat_off_and_cooldown();
    test_named_provisional_defaults_and_invalid_config();
    test_null_mode_and_target_fail_cold_boundaries();
    test_auto_printer_boundaries_and_unknown_age();
    test_all_sensor_invalid_and_fan_fault_inputs();
    test_temperature_and_fault_clear_boundaries();
    puts("jj_interlock_host_test: PASS");
    return 0;
}
