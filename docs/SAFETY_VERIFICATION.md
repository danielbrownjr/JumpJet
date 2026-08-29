# Safety verification evidence

This table distinguishes implemented evidence from work that still requires
hardware characterization. A source-code assertion is not a substitute for a
physical measurement or fault-injection record.

| Requirement | Verification method | Evidence artifact | Artifact location |
|---|---|---|---|
| Phase 0 production firmware contains no output-actuation primitive outside an explicit allowlist. | Static allowlist scan on every CI run; ESP32-S3 link build. | Actuation guard and CI job. | `tests/check_actuation_allowlist.sh`, `tools/production-actuation-allowlist.txt`, `.github/workflows/firmware-build.yml` |
| Represented interlock prerequisites and boundary values fail cold. | Warnings-as-errors host state-machine tests. | Interlock host test executable/source. | `tests/jj_interlock_host_test.c`, `tests/run_interlock_host_test.sh` |
| Canonical product identity is non-empty, fits the 31-byte ESP-IDF project-name payload, and drives OTA/project identity. | Positive and negative CMake validation plus static drift checks and ESP32-S3 build. | Identity contract check and generated build descriptor. | `tests/check_identity_contract.sh`, `components/jj_identity/`, `build/jumpjet.bin` when built |
| OTA is rejected before upload while heat is requested and checked again before boot selection. **Implemented; hardware fault-injection pending.** | Product-hook static contract and inspection of pinned `dc_portal`; future live upload tests. | Product OTA contract check; no hardware trace yet. | `tests/check_ota_contract.sh`, `components/jj_portal/jj_portal.c`, pinned `dc_portal` at `deeda5e` |
| Cold-safe commissioning state remains observable, rather than existing only as a log event. | API source-contract check; future device response capture. | API contract assertions; no hardware capture yet. | `tests/check_ota_contract.sh`, `components/jj_portal/jj_portal.c` |
| Normal operating ceiling < firmware hard trip < independent hardware cutoff < lowest applicable material/component limit, with justified margins. **Pending Phase 1.** | Thermal characterization and component/material review. | No evidence artifact exists yet. | `TODO.md`, `docs/HARDWARE_BASELINE.md` |
| Independent hardware cutoff cannot be defeated by firmware. **Pending Phase 1.** | Schematic/BOM review and physical fault injection. | No evidence artifact exists yet. | `ROADMAP.md` Phase 1 and `TODO.md` |
| Sensor rail/open/short, airflow failure, no-heat, uncontrolled-rise, stuck-on, watchdog, brownout, and interrupted-OTA responses work on release-intent hardware. **Pending later phases.** | Bench/HIL fault injection. | No evidence artifact exists yet. | `ROADMAP.md` Phases 2-5 and `TODO.md` |
