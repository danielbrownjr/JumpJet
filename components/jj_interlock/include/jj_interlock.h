// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    JJ_MODE_OFF = 0,  /**< Heating is disabled; safe cooldown behavior may continue. */
    JJ_MODE_MANUAL,   /**< Use the locally requested chamber target. */
    JJ_MODE_AUTO,     /**< Derive heating permission and target from printer status and policy. */
} jj_mode_t;

typedef enum {
    JJ_SENSOR_UNAVAILABLE = 0, /**< No complete sensor sample is currently available. */
    JJ_SENSOR_OK,          /**< A fresh, finite temperature sample is valid for safety decisions. */
    JJ_SENSOR_OPEN,        /**< The sensor circuit is electrically consistent with an open circuit. */
    JJ_SENSOR_SHORT,       /**< The sensor circuit is electrically consistent with a short circuit. */
    JJ_SENSOR_IMPLAUSIBLE, /**< Conversion is non-finite or outside the configured physically credible range. */
} jj_sensor_status_t;

typedef enum {
    JJ_FAN_PROOF_UNAVAILABLE = 0, /**< No proof mechanism/sample is currently authoritative. */
    JJ_FAN_PROOF_PENDING,         /**< Fans are commanded; proof is still within its allowed startup interval. */
    JJ_FAN_PROOF_PROVEN,          /**< Required airflow has been established. */
    JJ_FAN_PROOF_FAILED,          /**< The product-level proof deadline or explicit failure condition was reached. */
} jj_fan_proof_t;

typedef enum {
    JJ_FAULT_NONE = 0,          /**< No safety fault is latched. */
    JJ_FAULT_SENSOR,            /**< One or more required temperature sensors are not JJ_SENSOR_OK. */
    JJ_FAULT_OVERTEMPERATURE,   /**< A monitored temperature reached its hard safety limit. */
    JJ_FAULT_FAN,               /**< Required airflow could not be proven while heating was requested. */
    JJ_FAULT_NO_HEAT,           /**< Temperature failed to rise as expected while heating was requested. */
    JJ_FAULT_UNCONTROLLED_RISE, /**< Temperature rose abnormally while heater output was not requested. */
    JJ_FAULT_CONFIG,            /**< Safety or policy configuration is invalid; heating is denied. */
} jj_fault_t;

typedef enum {
    JJ_BLOCK_NONE = 0,             /**< No interlock condition is blocking a heater request. */
    JJ_BLOCK_OFF,                  /**< Heating is blocked because the selected mode or target is off. */
    JJ_BLOCK_NOT_COMMISSIONED,     /**< Heating is blocked until hardware commissioning is complete. */
    JJ_BLOCK_FAULT_LATCHED,        /**< Heating is blocked by a latched safety fault. */
    JJ_BLOCK_PRINTER_OFFLINE,      /**< AUTO heating is blocked because the printer is disconnected. */
    JJ_BLOCK_PRINTER_STALE,        /**< AUTO heating is blocked because printer data is missing or too old. */
    JJ_BLOCK_PRINTER_STOPPED,      /**< AUTO heating is blocked because the printer is not printing. */
    JJ_BLOCK_BED_TARGET_LOW,       /**< AUTO heating is blocked because the bed target is below policy. */
    JJ_BLOCK_FAN_PROOF_PENDING,    /**< Heating awaits authoritative fan/airflow proof. */
    JJ_BLOCK_INVALID_MODE,         /**< Heating is blocked because the requested mode is not recognized. */
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
    float chamber_hard_limit_c;
    float outlet_hard_limit_c;
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
    jj_fan_proof_t fan_proof;
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
jj_inputs_t jj_inputs_safe_defaults(void);
void jj_interlock_init(jj_interlock_t *state, const jj_interlock_config_t *config);
jj_outputs_t jj_interlock_step(jj_interlock_t *state, const jj_inputs_t *input);
bool jj_interlock_clear_fault(jj_interlock_t *state, const jj_inputs_t *input);
jj_outputs_t jj_interlock_snapshot(const jj_interlock_t *state);
const char *jj_fault_str(jj_fault_t fault);
const char *jj_block_reason_str(jj_block_reason_t reason);
