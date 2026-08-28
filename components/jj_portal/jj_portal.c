// SPDX-License-Identifier: GPL-3.0-or-later
#include "jj_portal.h"
#include "cJSON.h"
#include "dc_portal.h"
#include "dc_prusa.h"
#include "jj_identity.h"
#include "esp_app_desc.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_mac.h"
#include <errno.h>
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
    (void)dc_prusa_get_status(&printer);
    const jj_outputs_t output = jj_interlock_snapshot(s_interlock);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "api_version", 2);
    cJSON_AddStringToObject(root, "mode", "off");
    cJSON *heater = cJSON_AddObjectToObject(root, "heater");
    cJSON_AddBoolToObject(heater, "output", false);
    cJSON_AddNumberToObject(heater, "duty_percent", 0);
    cJSON_AddNumberToObject(heater, "target_c", 0);
    cJSON *fan = cJSON_AddObjectToObject(root, "fan");
    cJSON_AddNumberToObject(fan, "percent", output.fan_percent);
    cJSON *safety = cJSON_AddObjectToObject(root, "safety");
    cJSON_AddBoolToObject(safety, "commissioned", false);
    cJSON_AddBoolToObject(safety, "fault_latched", output.fault != JJ_FAULT_NONE);
    cJSON_AddStringToObject(safety, "fault", jj_fault_str(output.fault));
    cJSON_AddStringToObject(safety, "interlock", jj_block_reason_str(output.block_reason));
    cJSON *prusa = cJSON_AddObjectToObject(root, "printer");
    cJSON_AddStringToObject(prusa, "source", "prusalink");
    cJSON_AddStringToObject(prusa, "connection", dc_prusa_state_str(printer.state));
    cJSON_AddStringToObject(prusa, "state", printer.printer_state);
    if (printer.online) {
        cJSON_AddNumberToObject(prusa, "bed_temperature_c", printer.bed_temp);
        cJSON_AddNumberToObject(prusa, "bed_target_c", printer.bed_target);
    } else {
        cJSON_AddNullToObject(prusa, "bed_temperature_c");
        cJSON_AddNullToObject(prusa, "bed_target_c");
    }
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

static cJSON *field(const char *key, const char *label, const char *type,
                    const char *value, bool secret)
{
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "key", key);
    cJSON_AddStringToObject(item, "label", label);
    cJSON_AddStringToObject(item, "type", type);
    cJSON_AddStringToObject(item, "value", secret ? "" : value);
    if (secret) cJSON_AddBoolToObject(item, "secret", true);
    return item;
}

static cJSON *describe_product(void *ctx)
{
    (void)ctx;
    dc_prusa_config_t config = {0};
    (void)dc_prusa_get_config(&config);
    char port[8];
    snprintf(port, sizeof(port), "%u", (unsigned)config.port);
    cJSON *root = cJSON_CreateObject();
    cJSON *sections = cJSON_AddArrayToObject(root, "sections");
    cJSON *section = cJSON_CreateObject();
    cJSON_AddStringToObject(section, "title", "PrusaLink (read-only)");
    cJSON *fields = cJSON_AddArrayToObject(section, "fields");
    cJSON_AddItemToArray(fields, field("pr_host", "Printer host", "text", config.host, false));
    cJSON_AddItemToArray(fields, field("pr_port", "Port", "number", port, false));
    cJSON_AddItemToArray(fields, field("pr_key", "API key (blank keeps saved key)",
                                       "password", "", true));
    cJSON_AddItemToArray(sections, section);
    return root;
}

static bool valid_host(const char *host)
{
    if (!host || !host[0] || strlen(host) >= sizeof(((dc_prusa_config_t *)0)->host))
        return false;
    for (const unsigned char *p = (const unsigned char *)host; *p; ++p)
        if (*p <= 0x20 || *p == '/' || *p == '\\' || *p == ':' || *p == '#') return false;
    return true;
}

static esp_err_t apply_product(const cJSON *values, void *ctx,
                               char *message, size_t message_size)
{
    (void)ctx;
    dc_prusa_config_t config = {0};
    (void)dc_prusa_get_config(&config);
    const cJSON *host = cJSON_GetObjectItemCaseSensitive(values, "pr_host");
    const cJSON *port = cJSON_GetObjectItemCaseSensitive(values, "pr_port");
    const cJSON *key = cJSON_GetObjectItemCaseSensitive(values, "pr_key");
    if (!cJSON_IsString(host) || !valid_host(host->valuestring)) {
        snprintf(message, message_size, "Printer host is required and must not contain a URL path.");
        return ESP_ERR_INVALID_ARG;
    }
    long parsed_port = config.port ? config.port : DC_PRUSA_DEFAULT_PORT;
    if (cJSON_IsNumber(port)) {
        parsed_port = port->valueint;
        if (port->valuedouble != parsed_port) parsed_port = -1;
    } else if (cJSON_IsString(port)) {
        char *end = NULL;
        errno = 0;
        parsed_port = strtol(port->valuestring, &end, 10);
        if (errno || !end || *end) parsed_port = -1;
    } else {
        parsed_port = -1;
    }
    if (parsed_port < 1 || parsed_port > UINT16_MAX) {
        snprintf(message, message_size, "PrusaLink port must be 1-65535.");
        return ESP_ERR_INVALID_ARG;
    }
    if (key && !cJSON_IsString(key)) {
        snprintf(message, message_size, "PrusaLink API key must be text.");
        return ESP_ERR_INVALID_ARG;
    }
    if (key && strlen(key->valuestring) >= sizeof(config.api_key)) {
        snprintf(message, message_size, "PrusaLink API key is too long.");
        return ESP_ERR_INVALID_ARG;
    }
    snprintf(config.host, sizeof(config.host), "%s", host->valuestring);
    config.port = (uint16_t)parsed_port;
    if (key && key->valuestring[0])
        snprintf(config.api_key, sizeof(config.api_key), "%s", key->valuestring);
    esp_err_t err = dc_prusa_set_config(&config);
    if (err == ESP_OK)
        snprintf(message, message_size, "PrusaLink settings saved; restart to apply.");
    return err;
}

static bool authorize(httpd_req_t *req, void *ctx)
{
    (void)ctx;
    char value[65];
    return httpd_req_get_hdr_value_str(req, "X-Dragon-Auth", value, sizeof(value)) == ESP_OK &&
           value[0] != '\0';
}

static esp_err_t heater_off_guard(char *message, size_t message_size)
{
    const jj_outputs_t output = jj_interlock_snapshot(s_interlock);
    if (!output.heater_requested) return ESP_OK;
    snprintf(message, message_size, "Turn the heater off before this operation.");
    return ESP_ERR_INVALID_STATE;
}

static esp_err_t guard_operation(dc_portal_operation_t operation, void *ctx,
                                 char *message, size_t message_size)
{
    (void)operation;
    (void)ctx;
    return heater_off_guard(message, message_size);
}

static esp_err_t validate_image(const esp_app_desc_t *image, void *ctx,
                                char *message, size_t message_size)
{
    (void)ctx;
    // Independent post-upload recheck immediately before core selects the image.
    esp_err_t err = heater_off_guard(message, message_size);
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
        .describe_product = describe_product,
        .apply_product = apply_product,
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
