# Upstream attribution and provenance

## Original project

- **Project:** Jetpack — chamber heater for the Prusa CORE One
- **Author:** [Philip Sørensen](https://github.com/philip-soerensen)
- **Source repository:**
  [`philip-soerensen/esp32-chamber-heater-core-one`](https://github.com/philip-soerensen/esp32-chamber-heater-core-one)
- **Mechanical project and build information:**
  [Printables model 1696936](https://www.printables.com/model/1696936-jetpack-chamber-heater-for-the-core-one)
- **License:** GNU General Public License v3.0
- **Jump Jet fork point:** commit
  [`d97f54a`](https://github.com/philip-soerensen/esp32-chamber-heater-core-one/commit/d97f54aa517d392b84590baf2a4200fe242e5568)

Philip's project established the original chamber-heater form factor, Arduino
implementation, PrusaLink-driven automation, PID control approach, and
three-thermistor safety concept. Jump Jet exists because that work was documented
and released as free software.

## Material retained in this repository

- the complete upstream Git history through the fork point
- the repository's GPL-3.0 `LICENSE`
- `esp32_chamber_heater_with_prusalink/esp32_chamber_heater_with_prusalink.ino`
  as the original implementation and historical reference

The retained Arduino sketch remains attributable to Philip and governed by the
repository's GPL-3.0 license. Its presence does not mean the sketch is the active
Jump Jet firmware.

## Jump Jet changes

Jump Jet replaces the active runtime with a product-specific ESP-IDF application
integrated with pinned `dragon-core` services. Jump Jet owns its PCB, GPIO and ADC
mapping, sensors, heater and fan drivers, control policy, safety state machine,
settings, APIs, UI identity, OTA restrictions, tests, and release process.

New Jump Jet work must not be presented as authored, reviewed, supported, or
endorsed by Philip unless he explicitly participates in that work. Conversely,
future documentation must not erase the original project's contribution merely
because the replacement firmware diverges substantially.

## License continuity

Jump Jet remains GPL-3.0. The original license and copyright history must remain
with redistributed source. `dragon-core` is an independently maintained,
MIT-licensed dependency consumed through pinned component revisions.
