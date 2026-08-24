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

`dc_prusa` v0.28.2 does not yet expose the monotonic time of its last successful
sample. The foundation therefore supplies an unknown/zero sample time, which makes
AUTO fail stale even if the connection currently reports online. The next
`dragon-core` change must add a last-success timestamp (or sample age) to
`dc_prusa_status_t`, update it only after a complete atomic parse, and test that
failed/partial polls never refresh it. Jump Jet must remain pinned cold until it
consumes that reviewed interface.
