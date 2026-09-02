# Jump Jet firmware architecture

Jump Jet is an ESP-IDF Dragon-family product, not a pin-swapped DragonBreath build.

## Ownership boundary

Pinned `dragon-core` v0.32.0 services provide board-neutral Wi-Fi, portal, UI,
event logging, and read-only PrusaLink status. `dc_prusa` applies its authoritative
15-second freshness behavior; Jump Jet does not run another freshness timer.

Jump Jet owns mode and target settings, AUTOMATIC policy, diagnostics, OTA product
guards, cooldown, recovery, all hardware definitions, sensing, actuation, and
safety decisions. See [PRODUCT_SAFETY_CONTRACT.md](PRODUCT_SAFETY_CONTRACT.md).

## Boot order

1. Initialize logging and NVS.
2. Initialize the interlock in OFF with physical actuation unavailable.
3. Start the control/safety task.
4. Start Wi-Fi, read-only PrusaLink polling, and the management portal.
5. Wait for a synchronized cold-safe interlock cycle.
6. Verify zero requested heat and zero effective target.
7. Only then mark a pending OTA image valid.

Mode is not persisted. Reset always returns to OFF. The 45 °C retained manual
target is configuration, not permission to heat.

## Control and diagnostics

Only the exact PrusaLink state `PRINTING` passes the AUTOMATIC state allowlist.
Even then, AUTOMATIC remains cold because the bed-target-to-chamber-target policy
is deliberately undefined. Manual targets outside 30–50 °C are rejected.

The abstract interlock can represent logical heat demand, fan proof, cooldown,
fault thermal management, and authoritative overtemperature detection without
inventing GPIOs or temperature thresholds. The production image has no physical
output path, so API delivery remains zero/unavailable.

## OTA invariant

The portal rejects OTA when heat is requested or active thermal management is
required. The same authoritative guard is rerun immediately before boot selection,
then project identity is checked. Browser/API controls cannot override interlocks.
