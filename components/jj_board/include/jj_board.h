// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    JJ_GPIO_UNASSIGNED, JJ_GPIO_PROVISIONAL, JJ_GPIO_BLOCKED, JJ_GPIO_VERIFIED
} jj_gpio_status_t;

typedef struct {
    const char *role;
    int gpio; // -1 means unavailable, never a usable assignment.
    jj_gpio_status_t status;
} jj_board_role_t;

typedef struct {
    const char *profile;
    bool production;
    bool heater_available;
    const jj_board_role_t *roles;
    size_t role_count;
} jj_board_t;

const jj_board_t *jj_board_profile(void);
const char *jj_gpio_status_name(jj_gpio_status_t status);
