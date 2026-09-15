// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "cJSON.h"
#include "esp_err.h"
#include <stddef.h>
cJSON *jj_config_describe(void *ctx);
esp_err_t jj_config_apply(const cJSON *values, void *ctx, char *message, size_t message_size);
