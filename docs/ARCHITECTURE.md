# Jump Jet firmware architecture

Jump Jet is an ESP-IDF Dragon-family product, not a pin-swapped DragonBreath build.

## Ownership boundary

`dragon-core` provides board-neutral services: `dc_wifi`, `dc_portal`, `dc_ui`,
`dc_evlog`, and the read-only `dc_prusa` client. Every selected component is pinned
to the same reviewed commit in `main/idf_component.yml`.

Jump Jet owns all heater-specific behavior: GPIO and ADC assignments, thermistor
conversion, PWM/PID, fan control and proof, settings, API state, device identity,
interlocks, faults, cooldown, and recovery. Safety-critical policy must not migrate
into `dragon-core` merely to reduce application code.

## Boot order

1. Capture logs and initialize NVS.
2. Initialize the interlock with heat disabled.
3. Start the control/safety task.
4. Start Wi-Fi, read-only PrusaLink polling, and the management portal.
5. Mark a pending OTA image valid only after the cold-safe runtime is healthy.

The foundation image has no actuator driver and cannot energize the heater.

## OTA invariant

The portal checks heater state before accepting an upload. The product image
validator independently checks it again after image validation and immediately
before `dc_portal` selects the boot partition. Only images whose ESP-IDF project
identity is `jumpjet` are accepted.

## Automatic heat invariant

AUTO may request heat only while all sensors are valid, no fault is latched,
PrusaLink is online, its sample is fresh, printer state is actively printing, and
the bed target meets the configured policy threshold. Losing any prerequisite
requests zero heater output. Filament type is not inferred because PrusaLink does
not expose it.

`dc_prusa` now exposes `status_age_ms`, updated only after a complete atomic
PrusaLink status parse and set to `UINT32_MAX` when no complete sample is available.
Jump Jet passes that age directly into its product-owned interlock. The interlock
fails AUTO cold when the age exceeds 12 seconds, which is intentionally stricter
than dc_prusa's 15-second transport-level expiration backstop. The change is pinned
from Dan's fork while upstream `dragon-core` PR #51 is under review.
