# TinyS3D bring-up scaffolding

This feature branch configures the `tinys3d_candidate` profile. It does not detect
which module is attached or select a production board. All five product roles
(heater enable, fan power, fan PWM, fan tach, temperature input) remain unassigned:
`gpio = -1` internally and `null` in JSON. No GPIO driver or output path exists.
The existing interlock, control policy and OTA guards remain authoritative.

## Capture on the next bench session

Use the existing `GET /api/v2/state` and save the complete JSON response with a
host timestamp. `diagnostics.schema` is 1; there is no additional server or debug
mutation. Capture once after boot and repeatedly during provisioning/reconnection.
These instructions are future bench work; host fixtures are not device evidence.

| Surface | Meaning |
|---|---|
| `diagnostics.board` | Configured candidate profile, not detected module identity; production and heater availability are false |
| `diagnostics.boot` | Immutable startup observations, sampled before networking starts; timestamp is milliseconds since boot |
| `idf_version`, `project`, `version` | Runtime IDF version and application build identity from ESP-IDF; a dirty build is not a reproducible release |
| `chip_model_id`, revision major/minor, cores | ESP-IDF chip information; model ID follows that IDF version's `esp_chip_model_t` enum |
| `reset_reason_id` | ESP-IDF `esp_reset_reason_t`; observation only, no recovery-policy changes |
| `detected_flash_bytes` | `esp_flash_get_physical_size`, not image-header capacity; null on query failure |
| `configured_flash_image_mode`, `configured_flash_image_frequency` | ESPTOOLPY image-header strings, not measured bus mode/frequency; IDF may encode a conservative boot mode/frequency |
| `psram.initialization` | `not_attempted` with support disabled or boot init disabled; `unavailable` if boot init enabled but memory not initialized; `initialized` on positive API evidence; unknown reserved for unavailable evidence |
| `psram.physical_presence`, `detected_bytes` | Unknown/null unless initialization succeeded; failure or disabled support does not establish physical absence |
| `psram.self_test` | Always `not_attempted`; no allocation test or stress test is implemented |
| `diagnostics.runtime` | Fresh request-time queries, bracketed by uptime timestamps; sequential samples, not an atomic hardware snapshot |
| `free_internal_8bit_heap_bytes` | Current free heap with both INTERNAL and 8BIT capabilities, in bytes |
| `minimum_free_internal_8bit_heap_bytes` | ESP-IDF allocator's sum of per-region low-water marks for those capabilities; not necessarily a simultaneous global minimum |
| `wifi.core_state` | Pinned dc_wifi state; `sta_connected` is set on GOT_IP, not merely association |
| `wifi.sta_associated` | Current `esp_wifi_sta_get_ap_info` result: true on success, false on NOT_CONNECT, null on other failures |
| `wifi.sta_ipv4_available` | Nonzero IPv4 plus up STA netif plus observed association; cached IP after disconnection is not success; null when required evidence is unavailable |
| `wifi.dhcp_client_started` | DHCP client running, not proof of a lease; combine with IPv4 availability for the current shared DHCP path |
| `wifi.provisioning_completion` | Always `unknown`: pinned infrastructure has no durable completion marker |
| `printer.status_age_ms` | Existing dc_prusa sample age; null for unavailable/error, preserving its authoritative freshness semantics |

No credentials, SSID, IP address, full MAC, or new device identifier are included.
A client must compare uptime only within one boot. Save build identity and host
capture times to distinguish reboots. These samples do not promise an event
history or capture brief association/IP transitions between requests.

## Provisioning semantics and shared follow-up

Continue to use the shared provisioning UI and `/api/v1/provisioning/wifi`.
At dragon-core `4e041d864763d468a50e9649807827dd83dd54bc`, the HTTP handler sends
`ok/rebooting` before `dc_wifi_save_creds_and_reboot` persists settings. Therefore
that response, an AP starting, or a scan completing cannot prove provisioning
completion. The diagnostic reports unknown, never a fabricated success.

A separate upstream follow-up could provide a persistence outcome and timestamped
association/GOT_IP observations through a shared read-only snapshot. No upstream
code changes are included here. The next bench session must distinguish submission,
reboot, actual STA association and IP acquisition with the available evidence.

## PSRAM boundary

The default build retains the baseline configuration: PSRAM support is disabled,
flash image capacity remains 4 MiB, and no memory mode or pin is guessed. Firmware
never calls a PSRAM initialization API or changes application behavior based on
PSRAM. The conditional read-only adapter can describe a future independently
verified configuration. Such a configuration must tolerate absent memory before
being used; this task supplies no opt-in electrical profile and makes no boot
success claim for arbitrary PSRAM-enabled builds.

## GPIO review table

These are review considerations from the existing characterization record, not
verified product assignments. Status vocabulary is `unassigned`, `provisional`,
`blocked`, `verified`; adding the word verified to the enum verifies no GPIO.

| Candidate resource | Review status / remaining evidence |
|---|---|
| All actuator roles | Unassigned; no candidate GPIO selected; output-at-reset, gate bias, power-domain and boot hazards must be verified before use |
| IO0 / IO3 | Provisional reference: strapping pins; check boot levels and external loading |
| Native USB / JTAG resources | Blocked from assignment pending module pinout and debug/USB conflict review; no numerical assignment introduced |
| ADC-labelled header resources | Provisional silicon/reference capability only; ADC range, attenuation, noise, onboard sharing and actual module suitability unverified |
| IO8 / IO9 | Provisional shared onboard I2C reference; do not assume free ADC/GPIO suitability |
| IO43 / IO44 | Provisional console reference; inspect boot traffic and reset behavior before reuse |
| Heater / fan candidate pins | Unassigned; silicon output capability is not evidence of cold-safe product suitability |
| Mounting / header numbering | Unresolved; dimensions and mechanical/electrical mapping require specimen verification |

No onboard LED, antenna select, ADC, heater or fan is configured by this scaffold.

## Host validation

`tests/run_bringup_host_test.sh` uses the actual pinned dragon-core public Prusa
header and the cJSON submodule from the same ESP-IDF checkout used by firmware CI.
It rejects mismatched or modified dependency inputs. Adapter fixtures exercise
support-disabled, boot-init-disabled and boot-init-enabled branches, plus unknown,
associated-without-IP, connected and disconnected-with-cached-IP samples. Persistence and ESP-IDF
observations are explicitly simulated; no Wi-Fi hardware or physical HIL is used.

The product's existing secret contract is preserved: omitted or empty `pr_key`
retains, nonempty replaces, non-string rejects. There is no key-clear mutation in
this product callback; factory reset remains the existing full-config clear path.
Shared STA provisioning treats omitted/empty passwords as an open-network password;
AP configuration has its own default-password semantics. Those shared APIs are not
reimplemented or changed here. Descriptions redact saved product keys.
