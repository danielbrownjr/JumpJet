// SPDX-License-Identifier: GPL-3.0-or-later
#include "jj_config.h"
#include "dc_prusa.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

cJSON *jj_config_describe(void *ctx)
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

esp_err_t jj_config_apply(const cJSON *values, void *ctx,
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
