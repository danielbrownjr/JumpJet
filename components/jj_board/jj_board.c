// SPDX-License-Identifier: GPL-3.0-or-later
#include "jj_board.h"

static const jj_board_role_t roles[] = {
    {"heater_enable", -1, JJ_GPIO_UNASSIGNED},
    {"fan_power", -1, JJ_GPIO_UNASSIGNED},
    {"fan_pwm", -1, JJ_GPIO_UNASSIGNED},
    {"fan_tach", -1, JJ_GPIO_UNASSIGNED},
    {"temperature_input", -1, JJ_GPIO_UNASSIGNED},
};

const jj_board_t *jj_board_profile(void)
{
    // A build profile, not runtime detection of the connected module.
    static const jj_board_t board = {
        "tinys3d_candidate", false, false, roles, sizeof(roles) / sizeof(roles[0])
    };
    return &board;
}

const char *jj_gpio_status_name(jj_gpio_status_t status)
{
    switch (status) {
    case JJ_GPIO_UNASSIGNED: return "unassigned";
    case JJ_GPIO_PROVISIONAL: return "provisional";
    case JJ_GPIO_BLOCKED: return "blocked";
    case JJ_GPIO_VERIFIED: return "verified";
    default: return "unknown";
    }
}
