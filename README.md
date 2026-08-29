# Jump Jet

Jump Jet is a PCBWay-sponsored chamber-heater project for the Prusa CORE One
family, developed by [Dan](https://github.com/danielbrownjr) and
[Mauker](https://github.com/Mauker1). It redesigns the electronics and firmware
of Philip Sørensen's original Jetpack project while retaining its practical,
printer-integrated form factor.

> **Development status:** cold-safe foundation. The current firmware has no
> heater GPIO or PWM driver and cannot energize the heater. Do not treat it as
> installable heater-control firmware yet.

## Project lineage and credit

Jump Jet began as a fork of Philip Sørensen's
[`esp32-chamber-heater-core-one`](https://github.com/philip-soerensen/esp32-chamber-heater-core-one),
published with the
[Jetpack chamber-heater model and build information](https://www.printables.com/model/1696936-jetpack-chamber-heater-for-the-core-one).
Philip created the original mechanical concept, Arduino firmware, PrusaLink
automation, PID approach, and three-sensor safety concept on which this project
started. Thank you, Philip, for publishing the work under GPL-3.0.

This fork keeps the original Git history and GPL-3.0 license. The upstream
Arduino sketch is isolated under `legacy/jetpack-arduino/` as unmaintained
historical reference, not production firmware. Jump Jet is a substantially new
ESP-IDF/Dragon-family implementation and should
not be represented as Philip's work or as endorsed by him. See
[UPSTREAM.md](UPSTREAM.md) for exact provenance and retained upstream material.

The new firmware is a distinct ESP-IDF Dragon-family product. It consumes pinned,
board-neutral services from [`dragon-core`](https://github.com/justinh-rahb/dragon-core)
while keeping sensor conversion, GPIOs, heater/fan actuation, PID, product settings,
API behavior, and all heater safety policy local to Jump Jet.

## Current status

The current milestone is deliberately **cold-safe**:

- ESP32-S3 ESP-IDF application skeleton
- selected `dragon-core` components pinned to upstream commit
  `deeda5ef44fb8fbe78f908baab4b62f4486f2f0d`, which includes merged PrusaLink freshness work from PR #51
- `dc_wifi`, `dc_portal`, `dc_ui`, `dc_evlog`, and freshness-aware,
  read-only `dc_prusa` integration
- Jump Jet identity and truthful, read-only API v2 status
- pure-C interlock model with host tests for stale/offline/stopped printer state,
  low bed target, invalid policy/configuration, zero/unavailable inputs,
  conservative uncertain-sensor cooling, pending/failed fan proof,
  sensor-specific provisional overtemperature paths, and safe fault clearing
- OTA rejection before upload while heating, a second check before boot selection,
  and strict `jumpjet` image identity validation
- CI gates for the host interlock suite, static analysis, and an ESP-IDF 5.3
  build targeting the actual ESP32-S3 family
- **no heater GPIO or PWM implementation** until the PCB and safety chain are reviewed

## Safety boundary

Firmware is only one layer of the safety system. Jump Jet also requires correctly
rated wiring and connectors, input protection and fusing, an independent thermal
cutoff, a properly derated MOSFET, appropriate PCB copper and clearances, proven
airflow, temperature-suitable materials, and a physical design that cannot create
an unsafe hot spot. No software feature may substitute for those protections.

The default state is heater de-energized. Missing or stale printer data, invalid
sensors, stopped printing, a low bed target, a fault, or an uncommissioned hardware
configuration must all fail cold. Fans follow a separately defined cooldown/fault
policy.

## Architecture

Jump Jet is a distinct ESP-IDF product, not DragonBreath with different pins.
Pinned `dragon-core` components provide board-neutral Wi-Fi, provisioning, UI,
logging, OTA infrastructure, and read-only PrusaLink status. Jump Jet owns every
GPIO, sensor, actuator, setting, control loop, API mutation, capability declaration,
and hardware-specific safety decision.

## Host tests

```sh
./tests/run_interlock_host_test.sh
```

## ESP-IDF build

ESP-IDF 5.3 or newer with ESP32-S3 tools is required:

```sh
./tools/idf-build.sh . esp32s3
```

## Project planning

- [Roadmap](ROADMAP.md) — gated development phases and exit criteria
- [TODO](TODO.md) — concrete work queue and current blockers

## Documentation

- [Firmware architecture](docs/ARCHITECTURE.md)
- [Uncommissioned hardware baseline](docs/HARDWARE_BASELINE.md)
- [Safety verification evidence](docs/SAFETY_VERIFICATION.md)
- [Jetpack-to-Jump-Jet migration map](docs/MIGRATION.md)
- [Upstream attribution and provenance](UPSTREAM.md)

## Contributors and sponsorship

- [Dan](https://github.com/danielbrownjr) — project, firmware, and hardware
- [Mauker](https://github.com/Mauker1) — project and mechanical design
- Original Jetpack project: [Philip Sørensen](https://github.com/philip-soerensen)
- PCB fabrication sponsored by [PCBWay](https://www.pcbway.com/)

Jump Jet is an independent community project. It is not affiliated with or
endorsed by Prusa Research or the original Jetpack author.

## License

GPL-3.0, inherited from the upstream Jetpack repository. `dragon-core` is consumed
as an MIT-licensed dependency.
