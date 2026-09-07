# Hardware baseline — Rev A Phase 1

Rev A.4.1 is the current authoritative schematic baseline. Its
[KiCad project source](../hardware/kicad/JumpJet_RevA/README.md) is checked into
the repository. The checked-in source set currently contains no `.kicad_pcb`;
creating the first Rev A PCB remains active Phase-1 implementation work.

The detailed living source of truth for measurements, component candidates,
design decisions, provisional values, blockers, and validation status is the
[Phase 1 hardware register](PHASE1_HARDWARE_REGISTER.md). Presence in that
register does not make a candidate BOM-final or approve fabrication.
Checking in the KiCad source likewise does not make the design fabrication-ready
or authorize heater actuation.

## Confirmed

- Intended supply/heater basis: 24 V, CZ4060, rated 200 W / 8.33 A.
- Standalone characterization sustained about 189 W / 7.89 A with no high-current
  cold inrush.
- The standalone heater/PTC region stabilized near 130 °C and its chassis near
  74–75 °C.
- Full results: [CZ4060 characterization](hardware/cz4060-characterization.md).
- The Sanyo Denki 9GA0424P3J001 is a prototype fan candidate, not BOM-final.
- The Phase-1 replacement-fan contract is continuous fused 24 V and ground with
  separate open-drain PWM and tach. The original Rev A.4.1 low-side switched
  two-wire block is retained as baseline evidence, not production intent.
- Published prototype facts and pending measurements are tracked in the
  [9GA0424P3J001 characterization record](hardware/9ga0424p3j001-characterization.md).
- Phase-1 schematic work now separates a protected-24-V-derived
  `+5V_SYS_GATE` rail from the unresolved `+5V_MCU` service/source-selected
  domain. The gate buffer cannot be powered from USB-only MCU power.
- The heater driver now uses a hardware-default-disabled active-low OE topology:
  an OE pull-up holds the driver disabled unless a separate open-drain enable
  transistor is explicitly asserted. Generic `HEATER_GATE_EN` and
  `HEATER_PWM_CMD` nets remain unassigned to controller pins.

## Not confirmed

Q1, F2, PCB copper, connectors, wiring, installed airflow, the exact ESP32-S3
module implementation, and the complete power path have not been validated
together. The LMR36520 and SN74LV4T125 are strong candidates, not BOM-final;
the regulator circuit, `+5V_MCU` source selection, reverse-current behavior,
and full power-transition bench matrix remain open. The upstream Jetpack pin
assignments are historical facts, not Jump Jet assignments.

No GPIO, ADC, divider, thermistor conversion, protection threshold, thermal trip,
cooldown criterion, or recovery threshold may be finalized without corresponding
source files and measured evidence. Fan interface values, production fan,
tach electrical behavior, pulses/revolution, RPM versus duty, minimum reliable
command, startup behavior, RPM/stall thresholds, and installed airflow/thermal
performance remain unresolved. A model-specific 25 kHz candidate test point is
not universal production policy.

The newer Prusa-derived void is the preferred enclosure-space reference; the
earlier smaller-void system-level incompatibility conclusion is superseded. PCB
over the cold-air intake remains preferred and mechanically plausible, but the
installed transform and aligned local constraint map are unresolved, so
fabrication-intent `Edge.Cuts` remain blocked.

## Firmware consequence

The foundation advertises no heater/fan capability and contains no heater/fan
GPIO, placeholder pin, PWM, ADC, thermistor conversion, or MOSFET actuation path.
The provisional numeric limit header was removed because unvalidated values are
not a hardware contract.

Heater actuation remains disabled while the safety-critical hardware design and
validation progress.

The build retains a provisional 4 MiB dual-OTA partition layout solely as an
ESP32-S3 development baseline. It is not a release hardware constraint.
