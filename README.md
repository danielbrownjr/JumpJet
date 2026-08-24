# Jump Jet

Jump Jet is Dan and Mauker's PCBWay-sponsored redesign of Philip Sørensen's
[Jetpack chamber heater](https://www.printables.com/model/1696936-jetpack-chamber-heater-for-the-core-one)
for the Prusa CORE One family. This repository was forked from
[`philip-soerensen/esp32-chamber-heater-core-one`](https://github.com/philip-soerensen/esp32-chamber-heater-core-one)
and preserves the original Arduino sketch as a historical reference.

The new firmware is a distinct ESP-IDF Dragon-family product. It consumes pinned,
board-neutral services from [`dragon-core`](https://github.com/justinh-rahb/dragon-core)
while keeping sensor conversion, GPIOs, heater/fan actuation, PID, product settings,
API behavior, and all heater safety policy local to Jump Jet.

## Current status

The `feature/dragon-core-foundation` milestone is deliberately **cold-safe**:

- ESP32-S3 ESP-IDF application skeleton
- all selected `dragon-core` components pinned to commit
  `10fc3ed78bbb5dd6287bfb0d022708ead1a44635` (`v0.28.2`)
- `dc_wifi`, `dc_portal`, `dc_ui`, `dc_evlog`, and read-only `dc_prusa` integration
- Jump Jet identity and truthful, read-only API v2 status
- pure-C interlock model with host tests for stale/offline/stopped printer state,
  low bed target, sensor faults, overtemperature, cooldown, and safe fault clearing
- OTA rejection before upload while heating, a second check before boot selection,
  and strict `jumpjet` image identity validation
- CI gates for the host interlock suite, static analysis, and an ESP-IDF 5.3
  build targeting the actual ESP32-S3 family
- **no heater GPIO or PWM implementation** until the PCB and safety chain are reviewed

## Host tests

```sh
./tests/run_interlock_host_test.sh
```

## ESP-IDF build

ESP-IDF 5.3 or newer with ESP32-S3 tools is required:

```sh
./tools/idf-build.sh . esp32s3
```

See [architecture](docs/ARCHITECTURE.md), [hardware baseline](docs/HARDWARE_BASELINE.md),
and the [migration map](docs/MIGRATION.md).

## License

GPL-3.0, inherited from the upstream Jetpack repository. `dragon-core` is consumed
as an MIT-licensed dependency.
