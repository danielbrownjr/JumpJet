# Jump Jet TODO

This is the working queue. The [roadmap](ROADMAP.md) owns sequencing and phase
gates; this file owns concrete tasks. Move substantial work into GitHub issues as
implementation starts, and keep only the summary checkbox here.

## Needed from the current hardware design

- [ ] Add or link the current KiCad schematic and PCB source
- [ ] Add the current BOM with manufacturer part numbers and acceptable substitutions
- [ ] Confirm the exact ESP32-S3 module/dev board and installed flash size
- [ ] Confirm the complete GPIO/ADC assignment, including boot strapping and USB pins
- [ ] Record heater resistance or rated power/current at 24 V
- [ ] Record both fan part numbers, current, connector, PWM method, and tach availability
- [ ] Record every thermistor part number, beta/Steinhart-Hart data, tolerance, and location
- [ ] Confirm the MOSFET part, gate network, thermal environment, and default-off circuitry
- [ ] Confirm fuse type/rating, independent thermal-cutoff part/rating, and physical placement
- [ ] Define what the physical switch interrupts or commands
- [ ] Add the latest enclosure/duct model and intended airflow direction

## Firmware — immediate

- [x] Add last-success sample age to `dc_prusa_status_t` in Dan's `dragon-core` fork
- [x] Test that only a complete, atomically parsed PrusaLink sample refreshes freshness
- [x] Submit the freshness API upstream in `dragon-core` PR #51 and pin Jump Jet to its green commit
- [x] After PR #51 merges, repin Jump Jet to the resulting upstream commit or release tag
- [ ] Replace provisional AUTO thresholds with named, validated settings; keep heat disabled
- [ ] Define printer states that count as actively printing for CORE One Buddy firmware
- [ ] Add configuration validation tests for PrusaLink host, port, and secret retention
- [x] Add host tests for exact stale boundaries, unavailable sample age, invalid targets, and fault-clear temperature boundaries
- [ ] Add API contract tests for truthful capabilities and cold-safe state

## Electrical and safety analysis

- [ ] Draw the complete 24 V power and return path from supply through protection to loads
- [ ] Calculate normal, startup, and credible fault current for heater plus both fans
- [ ] Calculate connector/contact loading and wire ampacity with temperature derating
- [ ] Calculate MOSFET conduction and switching loss at worst-case voltage, current, duty, and ambient
- [ ] Check gate voltage/current, pull-down behavior, MCU reset behavior, and MOSFET SOA
- [ ] Calculate trace/via current capacity and expected copper temperature rise
- [ ] Calculate thermistor-divider voltage and ADC counts over open, short, and operating range
- [ ] Define independent thermal-cutoff trip temperature from measured enclosure hot spots
- [ ] Establish measured margins for: normal operating ceiling < firmware hard trip < independent hardware cutoff < lowest applicable material/component limit
- [ ] Produce a hazard/FMEA table with detection, response, latch behavior, and independent mitigation

## Firmware — after hardware review

- [ ] Implement `jj_board` with safe-at-reset GPIO initialization
- [ ] Implement ADC sampling, calibration, filtering, and explicit sensor status
- [ ] Implement two fan channels and available fan/airflow proof
- [ ] Implement physical-switch handling and debouncing
- [ ] Implement bounded settings persistence with corrupt-NVS recovery
- [ ] Implement the heater actuator behind one interlock-controlled interface
- [ ] Implement PWM/PID with output clamp, anti-windup, and bumpless stop/restart behavior
- [ ] Implement and persist latched safety faults
- [ ] Implement failure-to-heat and uncontrolled-rise detection from measured thermal behavior
- [ ] Add control-task watchdog and reset-reason reporting
- [ ] Define brownout and reboot cooldown behavior

## UI, API, and OTA

- [ ] Finalize Jump Jet API v2 state and mutation schemas
- [ ] Add chamber, outlet, and case temperature telemetry with sensor status
- [ ] Add target, heater duty, both fan states, mode, and commissioning state
- [ ] Add PrusaLink connection, printer state, bed temperature/target, and sample age
- [ ] Define optional Klipper/Moonraker integration for read-only printer state and bed-target telemetry, with the same stale-data fail-cold semantics as PrusaLink
- [ ] Add visible interlocks, latched fault reason, and safe recovery guidance
- [ ] Add settings for PrusaLink and the bed-target policy with strict bounds
- [ ] Add revision/lease semantics for remote heat commands if manual remote heat is retained
- [ ] Test immediate pre-upload OTA rejection while heating
- [ ] Test the second heater-state guard before selecting the uploaded image for boot
- [ ] Test wrong-product, corrupt, interrupted, and rollback OTA paths
- [ ] Advertise `power_on` or `auto` only after those paths pass HIL commissioning

## Bench and HIL validation

- [ ] Build a dummy-load/current-limited bring-up fixture before connecting the heater
- [ ] Add HIL scenarios for every sensor rail/open/short fault
- [ ] Add HIL scenarios for fan failure, heater no-response, abnormal rise, and stuck-on indication
- [ ] Add HIL scenarios for stale/offline PrusaLink and printer stop/error transitions
- [ ] Add watchdog, brownout, corrupt-NVS, factory-reset, and interrupted-OTA scenarios
- [ ] Characterize heat-up, overshoot, steady-state duty, hot spots, airflow, and cooldown
- [ ] Record connector, wire, PCB, MOSFET, heater-case, outlet, and chamber temperatures
- [ ] Run extended soak testing only after deliberate fault injection passes

## Documentation and release

- [ ] Maintain schematic, wiring diagram, BOM, pin map, and PCB-revision compatibility table
- [ ] Write first-flash, OTA, rollback, factory-reset, and recovery procedures
- [ ] Write assembly, inspection, commissioning, and troubleshooting procedures
- [ ] Define production-test fixture steps and acceptance limits
- [ ] Package versioned binaries, checksums, source provenance, and release notes
- [ ] Publish known limitations and required external safety hardware
