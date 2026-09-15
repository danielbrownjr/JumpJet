# TinyS3D candidate characterization

Status: **PROTOTYPE CANDIDATE**, not a production MCU selection. High-level
hardware status belongs to the [Phase 1 register](../PHASE1_HARDWARE_REGISTER.md).
This record contains specimen evidence from 2026-09-15; it does not supersede
Super Mini measurements or approve a footprint, GPIO assignment, or power path.

## Specimen and observed hardware

Unexpected Maker TinyS3D, Adafruit #6401. Packaging: lot `C22379-001`, code
`P6401A`, packed/tested `2025-08-07`. The specimen was opened and USB-tested.

| Item | Observed result | Evidence |
|---|---|---|
| Mounting | Through-hole | Direct inspection |
| MCU | ESP32-S3 (QFN56), revision v0.2 | `esptool --port COM7 chip-id` |
| Flash | 8 MB; manufacturer `0x20` (XMC), device `0x4017`; quad SPI, 3.3 V eFuse setting | `esptool --port COM7 flash-id` |
| Crystal | 40 MHz reported | esptool detection; not an independent frequency measurement |
| STA MAC | `68:ee:8f:68:fd:58` | esptool and application boot log |
| USB | Native USB-Serial/JTAG; Espressif VID `303A`, ROM PID `1001` | esptool and Windows enumeration |
| Factory firmware | CircuitPython, `CIRCUITPY` volume, CDC console and HID interface | Windows enumeration before flashing |
| Current firmware | JumpJet Phase 0; CircuitPython fully replaced | Flashing and application boot capture |
| PSRAM | Presence/size unresolved | ROM query listed embedded flash only; schematic shows a separate PSRAM IC without a populated MPN |

COM7 was the test port; rediscover the port after USB re-enumeration.

## Firmware build and boot evidence

The actual `jumpjet.bin` built successfully for `esp32s3` with ESP-IDF v5.3.5,
reported version `0a2febc-dirty`, and booted after flashing. The dirty version
does not identify the exact build-tree changes or a reproducible binary hash.

| Boot observation | Result |
|---|---|
| Partition table | `nvs`, `otadata`, `app0`, `app1`, `coredump`, `storage`, matching `partitions.csv` |
| Identity | Project `jumpjet`, version `0a2febc-dirty`, ESP-IDF v5.3.5 |
| Flash warning | Detected 8192k; image header 4096k; boot used header size |
| Wi-Fi | Provisioning AP `JumpJet_FD59` at `192.168.4.1`; scan found 18 networks |
| Services | `dc_prusa`, `dc_portal`, `jj_portal` started |
| Actuation | Logged intentionally unimplemented heater actuator, consistent with Phase 0 |
| Completion | `main_task: Returned from app_main()`; no panic, crash or watchdog reset observed in captured boots |

The 4 MiB image header matches `CONFIG_ESPTOOLPY_FLASHSIZE_4MB=y` and the
provisional partition layout; the remaining physical flash was unused. Record
this warning rather than treating the candidate's 8 MB as a finalized build
configuration. AP startup and scanning do not establish real-network provisioning.

Working ROM-entry procedure on this specimen: disconnect USB, hold BOOT,
reconnect USB, wait approximately 2 seconds, then release BOOT. A BOOT/RESET
warm-reset attempt under CircuitPython returned to CircuitPython. An RTS reset
after flashing left the device in ROM download mode; normal application boot
required USB unplug/replug with no buttons held. These are distinct observations,
not a verified explanation of reset circuitry or behavior across specimens.

UART0 console uses IO43/TX and IO44/RX. Boot/application logs were also captured
on native USB with the existing secondary USB Serial/JTAG console setting;
open the console before the boot burst. No firmware console change was needed.

## 5 V / USB VBUS test: UNRESOLVED

| Direction / setup | Measurement | Limitation |
|---|---|---|
| USB powered; DMM from header `5V` to GND | 4.871 V | Source VBUS was not independently measured; cable/contact drop and onboard-path drop cannot be separated |
| USB-C disconnected; approximately 4.87 V applied to header `5V`; DMM from USB-C VBUS to GND | Started at 0.235 V and continued rising | Test stopped before settling; no defined VBUS load, elapsed-time trace, or final voltage recorded |

A high-impedance DMM can display voltage from leakage into an otherwise unloaded
node. Meter input impedance was not measured (approximately 10 MΩ was only a
typical-value assumption). The partial rising reading does not establish the
path's topology or available backfeed current. **Isolation, direct coupling,
and safe simultaneous powering are neither established nor ruled out.**

Retest with a defined load across USB-C VBUS/GND (the original notes suggested
approximately 1–5 kΩ), a current-limited header supply and USB host disconnected.
Record resistor value/rating, source voltage/current limit, header and VBUS
voltages versus time, settled voltage, and load current. Also measure actual
USB VBUS and header voltage in the forward direction, and unpowered continuity/
diode behavior in both directions. Do not revise the carrier power architecture
until the complete mixed-source behavior is verified.

## Manufacturer reference evidence

These are recorded references, not specimen measurements or production decisions.
Sources inspected 2026-09-15:

- [Adafruit #6401](https://www.adafruit.com/product/6401), packaging label.
- [TinyS3 product-line page](https://esp32s3.com/tinys3.html): includes older/product-line specifications; do not assume every detail applies to this D-series specimen.
- [UnexpectedMaker ESP32-S3 repository](https://github.com/UnexpectedMaker/esp32s3):
  `series_d/pinout_cards/tinys3d_pinout.jpg`,
  `series_d/schematics/schematic-tinys3d-p1.pdf` (Rev D-P1, 2025-06-04,
  source `TinyS3D_P1.kicad_sch`, KiCad 9.0.2), and
  `series_d/3d models/TinyS3D.step` (Pcbnew export, 2025-05-26).

| Published property | Recorded claim / limitation |
|---|---|
| MCU / memory | ESP32-S3FN8, up to 240 MHz; 8 MB QSPI flash / 8 MB QSPI PSRAM on product-line page; only flash confirmed on specimen |
| Wireless | 2.4 GHz Wi-Fi 802.11b/g/n; Bluetooth LE 5 + Mesh; Bluetooth not tested |
| GPIO | 17 advertised; usable JumpJet assignment unresolved |
| LDO | Packaging advertises 700 mA, 3.3 V; no load/thermal validation |
| Battery | LiPo charging, rear JST-PH pads, I2C fuel gauge; no battery planned for Rev A |
| USB | Advertised backfeed protection; actual behavior unresolved above |
| Antenna | Onboard 3D antenna / u.FL, GPIO-selectable; onboard default advertised, not bench-verified |
| Dimensions | Page: 35 × 17.8 mm, maximum thickness 4.3 mm at USB end; label: 36.3 × 17.78 mm; caliper reconciliation pending |

STEP SHA-256: `2fd992c7fd6f1e4a2968fd0cf4ed012e1a562962de4554ae645d926159858131`.
Raw point data includes `36.3` and `17.78`; construction points do not establish
a solid bounding box. These values are not physical measurements.

### Pinout reference

Orientation below follows the manufacturer card, not a frozen carrier pin numbering.

| Location | Card labels / functions |
|---|---|
| Left row, top to bottom | IO35/SPI MO, IO37/SPI MI, IO36/SPI SCK, IO34, IO9/SCL/ADC1 CH8, IO8/SDA/ADC1 CH7, IO7/ADC1 CH6, IO6/ADC1 CH5, RESET, GND, IO43/TX, IO44/RX |
| Right row, top to bottom | BAT (1S LiPo, max 4.2 V), GND, 5V (max 5 V IN/OUT), 3V3 (max 3.3 V IN/OUT), IO1/ADC1 CH0, IO2, IO3, IO4, IO5, IO21, IO0 |
| Onboard functions | IO33: VBUS detect divider; IO9/SCL, IO8/SDA, IO10/INT: MAX17048G fuel gauge; IO18/data, IO17/power: RGB LED; IO38: antenna select (HIGH u.FL, LOW onboard) |

IO0/IO3 are marked as strapping pins. IO8/IO9 share the onboard fuel-gauge
I2C bus. Check the ESP32-S3 datasheet, onboard connections and actual header
exposure before assigning GPIO/ADC; do not subtract onboard-only pins from the
advertised header count without checking the card.

### Schematic observations requiring verification

- LP4055A charger VDD is on VBUS; BAT drives VBAT. R16 = 3.3 kΩ maps to
  300 mA in the schematic table (10 kΩ/110 mA, 5 kΩ/200 mA,
  2 kΩ/500 mA, 1.2 kΩ/830 mA). No battery charging was tested.
- NCP167BMX330TBG LDO input is on VBAT. T3/CJ3139K with R17/D8 appears
  to form the USB/battery power path; arbitration behavior is unverified.
- D5 is drawn between VBUS and `USB`, the header `5V` net per the card.
  A series diode is a lead for testing, not proof of system isolation.
- MAX17048G+T10 uses the shared IO8/IO9 bus.

As inspected, the repository provided older TinyS3 KiCad symbol/footprint files
and an EdgeS3D project under `series_d/KiCAD/`, but no TinyS3D-specific symbol or
footprint. Verify available assets before deriving a project-local footprint.
Through-hole carrier mounting (direct headers or sockets) remains undecided.

## Remaining characterization

- [ ] Retest loaded 5 V/VBUS behavior and mixed-source power states.
- [ ] Provision Wi-Fi onto a real network and verify connection/reconnection.
- [ ] Measure accessible board body, thickness, pin pitch/row spacing, USB
  overhang/shell and plug envelope; establish orientation, underside-contact
  isolation and antenna keepout for the carrier.
- [ ] Confirm PSRAM presence/size by runtime query or chip marking.
- [ ] Verify antenna-select default/IO38 behavior and usable GPIO/ADC mapping.
- [ ] Establish mounting method and verify the candidate footprint at 1:1 scale.
