# JumpJet Rev A KiCad source

This directory contains the checked-in source from the authoritative Rev A.4.1
schematic package, `JumpJet_Controller_RevA4_1_J3_JST_XH.zip`.

Open `JumpJet_Controller_RevA4.kicad_pro`, then
`JumpJet_Controller_RevA4.kicad_sch`. The root schematic references four child
sheets in this directory. Project-local symbols and footprints resolve through
`sym-lib-table` and `fp-lib-table`.

## Authority and limitations

- Rev A.4.1 is the current authoritative **schematic** baseline.
- No `.kicad_pcb` exists in this source set. The first Rev A PCB remains active
  Phase 1 implementation work, not a missing artifact to recover.
- This source is not fabrication-ready and does not authorize heater or fan
  actuation.
- `BOM_PRELIMINARY.csv` is archive context only. It contains placeholders and
  does not override statuses in
  [`docs/PHASE1_HARDWARE_REGISTER.md`](../../../docs/PHASE1_HARDWARE_REGISTER.md).
- Native KiCad ERC has not been run in this environment because `kicad-cli` is
  unavailable. The archive provenance note records the same limitation.

## Footprint status

`JumpJet.pretty/ESP32-S3_SuperMini_2x9.kicad_mod` is the drilled legacy
footprint referenced by Rev A.4.1. It is retained so the current schematic
resolves, but it is obsolete for the selected production mounting method.

## Source-package exclusions

The archive's repair, warning-cleanup, presentation, polish, and connector
changelog text files were not imported because they are historical packaging
notes rather than project dependencies. Lock files, autosaves, backups,
fabrication outputs, and temporary exports are also intentionally excluded.
The archive README is retained under `references/` as provenance.
