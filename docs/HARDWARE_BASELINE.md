# Hardware baseline — not commissioned

## Confirmed

- Intended supply/heater basis: 24 V, CZ4060, rated 200 W / 8.33 A.
- Standalone characterization sustained about 189 W / 7.89 A with no high-current
  cold inrush.
- The standalone heater/PTC region stabilized near 130 °C and its chassis near
  74–75 °C.
- Full results: [CZ4060 characterization](hardware/cz4060-characterization.md).
- The Sanyo Denki 9GA0424P3J001 is a prototype fan candidate, not BOM-final.

## Not confirmed

The authoritative `.kicad_pcb` remains missing. Q1, F2, PCB copper, connectors,
wiring, installed airflow, the exact ESP32-S3 module, and the complete power path
have not been validated together. The upstream Jetpack pin assignments are
historical facts, not Jump Jet assignments.

No GPIO, ADC, divider, thermistor conversion, fan/PWM method, protection threshold,
thermal trip, cooldown criterion, or recovery threshold may be finalized without
the corresponding source files and measured evidence.

## Firmware consequence

The foundation advertises no heater/fan capability and contains no heater/fan
GPIO, placeholder pin, PWM, ADC, thermistor conversion, or MOSFET actuation path.
The provisional numeric limit header was removed because unvalidated values are
not a hardware contract.

The build retains a provisional 4 MiB dual-OTA partition layout solely as an
ESP32-S3 development baseline. It is not a release hardware constraint.
