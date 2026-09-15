// SPDX-License-Identifier: GPL-3.0-or-later
// Synthetic host fixtures: these are not device observations.
#include "jj_bringup.h"
#include "jj_board.h"
#include "jj_config.h"
#include "dc_prusa.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dc_prusa_config_t saved = {.host = "printer.local", .port = 80, .api_key = "fixture-secret"};
static unsigned writes;
static esp_err_t persistence_result;
esp_err_t dc_prusa_get_config(dc_prusa_config_t *out) { *out = saved; return ESP_OK; }
esp_err_t dc_prusa_set_config(const dc_prusa_config_t *value)
{
    ++writes;
    if (persistence_result == ESP_OK) saved = *value;
    return persistence_result;
}
static cJSON *get(cJSON *root, const char *key)
{
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    assert(item);
    return item;
}
static void config_case(const char *text, esp_err_t expected)
{
    cJSON *values = cJSON_Parse(text);
    assert(values);
    dc_prusa_config_t before = saved;
    unsigned previous_writes = writes;
    char message[160] = {0};
    assert(jj_config_apply(values, NULL, message, sizeof(message)) == expected);
    assert(!strstr(message, "fixture-secret"));
    if (expected == ESP_ERR_INVALID_ARG) {
        assert(writes == previous_writes);
        assert(memcmp(&saved, &before, sizeof(saved)) == 0);
    }
    cJSON_Delete(values);
}
static void test_config(void)
{
    config_case("{\"pr_host\":\"printer.local\",\"pr_port\":1}", ESP_OK);
    assert(saved.port == 1 && !strcmp(saved.api_key, "fixture-secret"));
    config_case("{\"pr_host\":\"192.168.1.2\",\"pr_port\":\"65535\",\"pr_key\":\"\"}", ESP_OK);
    assert(saved.port == 65535 && !strcmp(saved.api_key, "fixture-secret"));
    const char *bad[] = {
        "{}", "{\"pr_host\":null,\"pr_port\":80}",
        "{\"pr_host\":\"\",\"pr_port\":80}",
        "{\"pr_host\":\"http://printer\",\"pr_port\":80}",
        "{\"pr_host\":\"printer/path\",\"pr_port\":80}",
        "{\"pr_host\":\"printer#fragment\",\"pr_port\":80}",
        "{\"pr_host\":\"printer\\\\path\",\"pr_port\":80}",
        "{\"pr_host\":\"printer local\",\"pr_port\":80}",
        "{\"pr_host\":\"printer\\n\",\"pr_port\":80}",
        "{\"pr_host\":\"printer\"}",
        "{\"pr_host\":\"printer\",\"pr_port\":null}",
        "{\"pr_host\":\"printer\",\"pr_port\":true}",
        "{\"pr_host\":\"printer\",\"pr_port\":0}",
        "{\"pr_host\":\"printer\",\"pr_port\":-1}",
        "{\"pr_host\":\"printer\",\"pr_port\":65536}",
        "{\"pr_host\":\"printer\",\"pr_port\":1.5}",
        "{\"pr_host\":\"printer\",\"pr_port\":1e100}",
        "{\"pr_host\":\"printer\",\"pr_port\":\"\"}",
        "{\"pr_host\":\"printer\",\"pr_port\":\"80x\"}",
        "{\"pr_host\":\"printer\",\"pr_port\":\"99999999999999999999999\"}",
        "{\"pr_host\":\"printer\",\"pr_port\":80,\"pr_key\":null}",
        "{\"pr_host\":\"printer\",\"pr_port\":80,\"pr_key\":false}",
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) config_case(bad[i], ESP_ERR_INVALID_ARG);
    char long_value[66]; memset(long_value, 'a', sizeof(long_value)); long_value[65] = 0;
    char input[256];
    snprintf(input, sizeof(input), "{\"pr_host\":\"%s\",\"pr_port\":80}", long_value);
    config_case(input, ESP_ERR_INVALID_ARG);
    snprintf(input, sizeof(input), "{\"pr_host\":\"printer\",\"pr_port\":80,\"pr_key\":\"%s\"}", long_value);
    config_case(input, ESP_ERR_INVALID_ARG);
    long_value[63] = 0;
    snprintf(input, sizeof(input), "{\"pr_host\":\"%s\",\"pr_port\":80}", long_value);
    config_case(input, ESP_OK);
    config_case("{\"pr_host\":\"printer\",\"pr_port\":80,\"pr_key\":\"replacement-fixture\"}", ESP_OK);
    assert(!strcmp(saved.api_key, "replacement-fixture"));
    cJSON *description = jj_config_describe(NULL);
    char *serialized = cJSON_PrintUnformatted(description);
    assert(serialized && !strstr(serialized, "replacement-fixture"));
    cJSON *section = cJSON_GetArrayItem(get(description, "sections"), 0);
    cJSON *key = cJSON_GetArrayItem(get(section, "fields"), 2);
    assert(cJSON_IsTrue(get(key, "secret")));
    assert(!strcmp(get(key, "value")->valuestring, ""));
    free(serialized); cJSON_Delete(description);
    persistence_result = ESP_FAIL;
    config_case("{\"pr_host\":\"printer\",\"pr_port\":80,\"pr_key\":\"unsaved\"}", ESP_FAIL);
    assert(!strcmp(saved.api_key, "replacement-fixture"));
    // This product API has no explicit key-clear mutation. Empty retains;
    // factory reset delegates to dc_prusa_clear_config in the existing portal.
}
static void test_diagnostics(void)
{
    const jj_board_t *board = jj_board_profile();
    assert(!strcmp(board->profile, "tinys3d_candidate"));
    assert(!board->production && !board->heater_available);
    assert(board->role_count == 5);
    for (size_t i = 0; i < board->role_count; ++i)
        assert(board->roles[i].gpio == -1 && board->roles[i].status == JJ_GPIO_UNASSIGNED);
    assert(!strcmp(jj_gpio_status_name(JJ_GPIO_PROVISIONAL), "provisional"));
    assert(!strcmp(jj_gpio_status_name(JJ_GPIO_BLOCKED), "blocked"));
    assert(!strcmp(jj_gpio_status_name(JJ_GPIO_VERIFIED), "verified"));
    assert(!strcmp(jj_gpio_status_name((jj_gpio_status_t)99), "unknown"));
    jj_boot_observation_t boot = {.idf_version="fixture", .project="fixture", .version="fixture",
        .configured_flash_image_mode="dio", .configured_flash_image_frequency="80m", .chip_revision=2};
    jj_runtime_observation_t runtime = {.wifi_state="init", .sampled_at_ms=100, .sample_finished_at_ms=101};
    for (int state = JJ_PSRAM_NOT_ATTEMPTED; state <= JJ_PSRAM_UNKNOWN; ++state) {
        boot.psram_state = (jj_psram_state_t)state;
        boot.psram_bytes = 8192; // deliberately synthetic; not module capacity evidence
        cJSON *root = jj_bringup_json(&boot, &runtime);
        cJSON *b = get(root, "boot"), *psram = get(b, "psram");
        assert(cJSON_IsNull(get(b, "detected_flash_bytes")));
        assert(get(b, "chip_revision_minor")->valueint == 2);
        const char *expected[] = {"not_attempted", "unavailable", "initialized", "unknown"};
        assert(!strcmp(get(psram, "initialization")->valuestring, expected[state]));
        if (state == JJ_PSRAM_INITIALIZED) assert(get(psram, "detected_bytes")->valueint == 8192);
        else {
            assert(cJSON_IsNull(get(psram, "detected_bytes")));
            assert(!strcmp(get(psram, "physical_presence")->valuestring, "unknown"));
        }
        cJSON *wifi = get(get(root, "runtime"), "wifi");
        assert(cJSON_IsNull(get(wifi, "sta_associated")));
        assert(cJSON_IsNull(get(wifi, "sta_ipv4_available")));
        assert(!strcmp(get(wifi, "provisioning_completion")->valuestring, "unknown"));
        assert(cJSON_IsFalse(get(get(root, "board"), "heater_available")));
        char *text = cJSON_PrintUnformatted(root);
        cJSON *roundtrip = cJSON_Parse(text); assert(roundtrip);
        free(text); cJSON_Delete(roundtrip); cJSON_Delete(root);
    }
    boot.flash_known = true; boot.flash_bytes = 8388608;
    runtime.sta_associated = JJ_OBSERVATION_TRUE;
    runtime.sta_ipv4_available = JJ_OBSERVATION_FALSE;
    runtime.dhcp_client_started = JJ_OBSERVATION_TRUE;
    cJSON *root = jj_bringup_json(&boot, &runtime);
    assert(get(get(root, "boot"), "detected_flash_bytes")->valueint == 8388608);
    cJSON *wifi = get(get(root, "runtime"), "wifi");
    assert(cJSON_IsTrue(get(wifi, "sta_associated")));
    assert(cJSON_IsFalse(get(wifi, "sta_ipv4_available")));
    assert(cJSON_IsTrue(get(wifi, "dhcp_client_started")));
    cJSON_Delete(root);
    assert(jj_bringup_json(NULL, &runtime) == NULL);
}
int main(void)
{
    test_config(); test_diagnostics();
    puts("bring-up serialization, board and configuration host fixtures: PASS");
}
