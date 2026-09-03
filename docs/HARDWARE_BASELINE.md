# Hardware baseline — Rev A Phase 1

Rev A.4.1 is the current authoritative schematic baseline. It is intentionally
schematic-only: there is no missing authoritative PCB artifact to recover. The
first Rev A `.kicad_pcb` is being created as the next Phase-1 implementation
stage.

The detailed living source of truth for measurements, component candidates,
design decisions, provisional values, blockers, and validation status is the
[Phase 1 hardware register](PHASE1_HARDWARE_REGISTER.md). Presence in that
register does not make a candidate BOM-final or approve fabrication.

## Confirmed

- Intended supply/heater basis: 24 V, CZ4060, rated 200 W / 8.33 A.
- Standalone characterization sustained about 189 W / 7.89 A with no high-current
  cold inrush.
- The standalone heater/PTC region stabilized near 130 °C and its chassis near
  74–75 °C.
- Full results: [CZ4060 characterization](hardware/cz4060-characterization.md).
- The Sanyo Denki 9GA0424P3J001 is a prototype fan candidate, not BOM-final.

## Not confirmed

Q1, F2, PCB copper, connectors, wiring, installed airflow, the exact ESP32-S3
module implementation, and the complete power path have not been validated
together. The upstream Jetpack pin assignments are historical facts, not Jump
Jet assignments.

No GPIO, ADC, divider, thermistor conversion, fan/PWM method, protection threshold,
thermal trip, cooldown criterion, or recovery threshold may be finalized without
the corresponding source files and measured evidence.

## Firmware consequence

The foundation advertises no heater/fan capability and contains no heater/fan
GPIO, placeholder pin, PWM, ADC, thermistor conversion, or MOSFET actuation path.
The provisional numeric limit header was removed because unvalidated values are
not a hardware contract.

Heater actuation remains disabled while the safety-critical hardware design and
validation progress.

The build retains a provisional 4 MiB dual-OTA partition layout solely as an
ESP32-S3 development baseline. It is not a release hardware constraint.
