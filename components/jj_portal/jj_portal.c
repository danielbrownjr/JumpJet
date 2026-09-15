// SPDX-License-Identifier: GPL-3.0-or-later
#include "jj_portal.h"
#include "jj_config.h"
#include "jj_bringup.h"
#include "cJSON.h"
#include "dc_portal.h"
#include "dc_prusa.h"
#include "jj_identity.h"
#include "esp_app_desc.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "jj_portal";
static jj_interlock_t *s_interlock;
static char s_device_id[32];

static esp_err_t send_json(httpd_req_t *req, cJSON *json)
{
    char *body = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!body) return ESP_ERR_NO_MEM;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    esp_err_t err = httpd_resp_sendstr(req, body);
    free(body);
    return err;
}

static esp_err_t info_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "api_version", 2);
    cJSON_AddStringToObject(root, "device_id", s_device_id);
    cJSON_AddStringToObject(root, "firmware", esp_app_get_description()->version);
    cJSON_AddStringToObject(root, "project", esp_app_get_description()->project_name);
    cJSON *ui = cJSON_AddObjectToObject(root, "ui");
    cJSON_AddNumberToObject(ui, "schema", 1);
    cJSON_AddStringToObject(ui, "product", JJ_IDENTITY_PRODUCT_ID);
    cJSON_AddStringToObject(ui, "display_name", JJ_IDENTITY_DISPLAY_NAME);
    cJSON *capabilities = cJSON_AddArrayToObject(root, "capabilities");
    cJSON_AddItemToArray(capabilities, cJSON_CreateString("source_status"));
    cJSON_AddItemToArray(capabilities, cJSON_CreateString("polling"));
    return send_json(req, root);
}

static esp_err_t state_get(httpd_req_t *req)
{
    dc_prusa_status_t printer = {0};
    const esp_err_t printer_result = dc_prusa_get_status(&printer);
    const jj_outputs_t output = jj_interlock_snapshot(s_interlock);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "api_version", 2);
    cJSON_AddStringToObject(root, "mode", "off");
    cJSON_AddStringToObject(root, "controller", "disabled");
    cJSON *control = cJSON_AddObjectToObject(root, "control");
    cJSON_AddStringToObject(control, "source", "unavailable");
    cJSON_AddNullToObject(control, "temperature_c");
    cJSON_AddNumberToObject(root, "target_c", JJ_MANUAL_TARGET_DEFAULT_C);
    cJSON *heater = cJSON_AddObjectToObject(root, "heater");
    cJSON_AddBoolToObject(heater, "requested", false);
    cJSON_AddBoolToObject(heater, "allowed", false);
    cJSON_AddBoolToObject(heater, "delivered", false);
    cJSON_AddNumberToObject(heater, "delivered_percent", 0);
    cJSON_AddBoolToObject(heater, "available", false);
    cJSON *fan = cJSON_AddObjectToObject(root, "fan");
    cJSON_AddNumberToObject(fan, "requested_percent", output.fan_percent);
    cJSON_AddNumberToObject(fan, "delivered_percent", 0);
    cJSON_AddBoolToObject(fan, "available", false);
    cJSON_AddStringToObject(root, "dominant_constraint",
                           jj_block_reason_str(output.block_reason));
    cJSON *safety = cJSON_AddObjectToObject(root, "safety");
    cJSON_AddStringToObject(safety, "health", "degraded");
    cJSON_AddBoolToObject(safety, "commissioned", false);
    cJSON_AddBoolToObject(safety, "fault_latched", output.fault != JJ_FAULT_NONE);
    cJSON_AddStringToObject(safety, "fault", jj_fault_str(output.fault));
    cJSON_AddStringToObject(safety, "interlock", jj_block_reason_str(output.block_reason));
    cJSON *prusa = cJSON_AddObjectToObject(root, "printer");
    cJSON_AddStringToObject(prusa, "source", "prusalink");
    if (printer_result == ESP_OK && printer.status_age_ms != UINT32_MAX)
        cJSON_AddNumberToObject(prusa, "status_age_ms", printer.status_age_ms);
    else cJSON_AddNullToObject(prusa, "status_age_ms");
    cJSON_AddStringToObject(prusa, "connection", dc_prusa_state_str(printer.state));
    cJSON_AddStringToObject(prusa, "state", printer.printer_state);
    if (printer.online) {
        cJSON_AddNumberToObject(prusa, "bed_temperature_c", printer.bed_temp);
        cJSON_AddNumberToObject(prusa, "bed_target_c", printer.bed_target);
    } else {
        cJSON_AddNullToObject(prusa, "bed_temperature_c");
        cJSON_AddNullToObject(prusa, "bed_target_c");
    }
    cJSON *diagnostics = jj_bringup_snapshot_json();
    if (!diagnostics) { cJSON_Delete(root); return ESP_ERR_NO_MEM; }
    cJSON_AddItemToObject(root, "diagnostics", diagnostics);
    return send_json(req, root);
}

static esp_err_t health_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", "cold_safe");
    cJSON_AddBoolToObject(root, "heater_available", false);
    return send_json(req, root);
}

static esp_err_t register_routes(httpd_handle_t server, void *ctx)
{
    (void)ctx;
    const httpd_uri_t routes[] = {
        {.uri = "/api/v2/info", .method = HTTP_GET, .handler = info_get},
        {.uri = "/api/v2/state", .method = HTTP_GET, .handler = state_get},
        {.uri = "/api/v2/health", .method = HTTP_GET, .handler = health_get},
    };
    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &routes[i]);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

static bool authorize(httpd_req_t *req, void *ctx)
{
    (void)ctx;
    char value[65];
    return httpd_req_get_hdr_value_str(req, "X-Dragon-Auth", value, sizeof(value)) == ESP_OK &&
           value[0] != '\0';
}

static esp_err_t thermal_state_guard(char *message, size_t message_size)
{
    const jj_outputs_t output = jj_interlock_snapshot(s_interlock);
    if (!output.heater_requested && !output.thermal_management_required)
        return ESP_OK;
    snprintf(message, message_size,
             "OTA is unavailable while heating or active thermal management is required.");
    return ESP_ERR_INVALID_STATE;
}

static esp_err_t guard_operation(dc_portal_operation_t operation, void *ctx,
                                 char *message, size_t message_size)
{
    (void)operation;
    (void)ctx;
    return thermal_state_guard(message, message_size);
}

static esp_err_t validate_image(const esp_app_desc_t *image, void *ctx,
                                char *message, size_t message_size)
{
    (void)ctx;
    // Independent post-upload recheck immediately before core selects the image.
    esp_err_t err = thermal_state_guard(message, message_size);
    if (err != ESP_OK) return err;
    if (strcmp(image->project_name, JJ_IDENTITY_PRODUCT_ID) == 0) return ESP_OK;
    snprintf(message, message_size, "Not a %s firmware image.", JJ_IDENTITY_DISPLAY_NAME);
    return ESP_ERR_INVALID_ARG;
}

static esp_err_t factory_reset(void *ctx)
{
    (void)ctx;
    return dc_prusa_clear_config();
}

esp_err_t jj_portal_start(jj_interlock_t *interlock)
{
    if (!interlock) return ESP_ERR_INVALID_ARG;
    s_interlock = interlock;
    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(s_device_id, sizeof(s_device_id), JJ_IDENTITY_DEVICE_ID_PREFIX "%02x%02x%02x",
             mac[3], mac[4], mac[5]);
    httpd_config_t http = HTTPD_DEFAULT_CONFIG();
    http.max_uri_handlers = 24;
    http.lru_purge_enable = true;
    const dc_portal_config_t config = {
        .product = JJ_IDENTITY_PRODUCT_ID,
        .display_name = JJ_IDENTITY_DISPLAY_NAME,
        .register_product_routes = register_routes,
        .describe_product = jj_config_describe,
        .apply_product = jj_config_apply,
        .authorize = authorize,
        .guard_operation = guard_operation,
        .validate_image = validate_image,
        .factory_reset = factory_reset,
        .httpd_config = &http,
    };
    esp_err_t err = dc_portal_start(&config);
    if (err == ESP_OK) ESP_LOGI(TAG, "Dragon-family portal started");
    return err;
}
