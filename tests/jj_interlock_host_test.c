#include "jj_interlock.h"
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

int main(void)
{
    test_uncommissioned_is_cold();
    test_auto_happy_path_and_clamp();
    test_auto_fails_cold_on_printer_conditions();
    test_sensor_fault_latches_and_requires_safe_clear();
    test_overtemperature_forces_heat_off_and_cooldown();
    puts("jj_interlock_host_test: PASS");
    return 0;
}
