# Jump Jet product and safety contract

This is the authoritative behavioral contract for Jump Jet. It distinguishes
required product behavior from what the current cold-safe foundation implements.
Hardware-dependent values remain unknown until evidence supports them.

## Operating modes and retained configuration

Jump Jet has exactly three operating modes: **OFF**, **MANUAL**, and
**AUTOMATIC**. Boot and reset always select OFF; mode is never restored across a
reboot. OFF removes the heating request but does not erase retained settings.
Cooldown or fault-related fan activity may continue while mode is OFF.

The retained MANUAL target persists across mode changes and reboot. Its allowed
range is 30–50 °C inclusive and its default is 45 °C. Inputs outside that range
are rejected, never clamped. Atomic mode/target mutations now model these
semantics; NVS persistence remains a later implementation gate.

## Control authority and control inhibits

Operating mode, active control authority, recoverable control inhibit, latched
hardware/safety fault, and thermal handling are separate state dimensions.
Authority is one of NONE, REMOTE, AUTOMATIC, or REACQUIRING.

MANUAL heater demand is bound to a 60-second remote lease. The timeout is an
initial product/network policy, not a hardware safety threshold. Lease expiry
or explicit takeover immediately removes MANUAL demand, advances the generation,
and creates a recoverable `no_authority` inhibit. It does not create or clear a
hardware fault. Autonomous cooldown and fault fan policy continues. A reconnect
retains visible configuration but never restores old demand: the client must
acquire authority, complete an authoritative refresh, and submit a new eligible
request.

No client silently wins a remote lease. A different owner receives
`control_authority_required` unless it explicitly confirms takeover. Takeover
revokes the old lease, zeros prior remote demand, advances generation, and also
requires refresh plus a new request.

AUTOMATIC authority is product-local after an atomic transition commits. Its
demand never depends on a browser heartbeat or inherits REMOTE authority. It
continues after browser lease expiry only while fresh exact `PRINTING` status,
valid sensors, fan proof, interlocks, policy eligibility, and all safety vetoes
remain satisfied. The bed-target policy remains undefined, so production
AUTOMATIC heating stays unavailable.

Mutations carry lease ID, control generation, expected state revision, complete
requested state, and an optional idempotency UUID. Lease, generation, revision,
current inputs, faults, and transition eligibility are validated atomically;
the server commits the complete request or nothing. Stable failures are
`control_authority_required`, `control_authority_lost`,
`control_generation_stale`, `state_revision_conflict`,
`control_request_ineligible`, `control_request_conflict`, and
`hardware_fault_latched`. Reacquisition never acknowledges a real fault.

## AUTOMATIC and PrusaLink

`dc_prusa` is read-only and owns status freshness. Jump Jet consumes its existing
15-second freshness result directly and must not add a second freshness timer.
Missing or stale status is therefore presented to Jump Jet as unavailable.

Eligibility is an allowlist: only exact `PRINTING` is eligible. `PAUSED`, `BUSY`,
`ATTENTION`, `IDLE`, `READY`, `FINISHED`, `STOPPED`, `ERROR`, `UNKNOWN`, empty,
and every future unrecognized state fail cold.

The production bed-target-to-chamber-target policy is TBD. It must be derived and
reviewed rather than inferred from legacy behavior. Until that policy exists,
AUTOMATIC production heating is unavailable even when `PRINTING` is fresh.

## Safety and recovery

- Boot/reset commands heater OFF before any other product behavior.
- Missing/stale printer data, invalid sensors, faults, and unsupported policy
  inhibit heating.
- Watchdog, panic, and brownout resets create a latched inhibit. After reboot,
  health must be revalidated and a user must explicitly acknowledge it.
- Suspected stuck-on heating requires a power cycle, explicit acknowledgement,
  and evidence of sane commanded-OFF behavior before heat may be enabled again.
- Cooldown and fault fan policy is product-owned and may operate in OFF.
- Browser and API authorization are management controls, never the safety boundary.

Reset-cause persistence, stuck-on proof, physical sensors, and physical fan proof
are not implemented in this heater-incapable foundation. They are mandatory gates
before an actuator is introduced.

## OTA guard

OTA must be rejected while heating, during active cooldown, or while a fault
requires active thermal management. The authoritative product guard is checked
before upload and again immediately before selecting the uploaded image for boot.
An uploaded image must also carry the Jump Jet product identity.

## Diagnostic semantics

Diagnostics use this order so a client cannot confuse demand with delivery:

1. mode and controller
2. controlling temperature and source
3. target
4. requested output
5. allowed output
6. delivered output
7. dominant constraint
8. health, degraded state, and fault

Capabilities are truthful. Heating or fan capability must not be advertised while
physical actuation is unavailable.

## Heater-incapable foundation invariant

Production code contains no heater GPIO, placeholder heater pin, fan GPIO, fake
board constants, PWM hardware, ADC, thermistor conversion, or MOSFET actuation.
Logical demand and thermal-management requests may be modeled for host tests, but
delivered physical output remains zero and unavailable.

## Confirmed hardware facts

The standalone CZ4060 characterization at 24 V found:

- conservative design basis: 200 W / 8.33 A
- measured sustained load: about 189 W / 7.89 A
- no high-current cold inrush
- heater/PTC region around 130 °C steady
- chassis around 74–75 °C steady

The evidence and setup are in
[`hardware/cz4060-characterization.md`](hardware/cz4060-characterization.md).
Those temperatures are observations, not firmware thresholds.

## Hardware-dependent TBDs

The full Q1/F2/PCB copper/connectors/wiring path is not validated, and the
authoritative `.kicad_pcb` is missing. Final GPIO, ADC, thermistor conversion,
protection thresholds, cooldown criteria, and recovery thresholds must not be
invented. Sanyo Denki 9GA0424P3J001 is a prototype fan candidate only and is not
BOM-final.
