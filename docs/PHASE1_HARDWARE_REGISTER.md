# Phase 1 hardware source-of-truth register

Last updated: 2026-09-03

This is the living engineering register for JumpJet Rev A Phase-1 hardware
implementation. It records what is known, how it is known, what remains
provisional, and which downstream decisions are blocked. Rev A.4.1 is the
current authoritative schematic baseline; the first Rev A PCB is being created
as part of Phase 1.

> **Presence in this register does not mean BOM-final, fabrication-approved, or
> release-approved. Only the explicit status field carries authority.**

## Status legend

| Status | Meaning |
|---|---|
| CONFIRMED FACT | Supported by an authoritative project artifact or direct evidence; not merely inferred. |
| MEASURED | Directly measured, with the measurement conditions and source identified. |
| DESIGN DECISION | Deliberately selected architecture or requirement; implementation may still need validation. |
| STRONG CANDIDATE | Preferred component candidate with substantial support, but not BOM-final. |
| PROTOTYPE CANDIDATE | Part selected for characterization or prototyping only. |
| ASSUMPTION / PROVISIONAL | Working value or approach that must not be treated as final. |
| TBD / BLOCKED | Required information or selection is absent and blocks one or more downstream activities. |

Confidence describes the quality of the current evidence, not release readiness.
The **Blocks** column identifies what cannot be frozen or completed until the
remaining validation closes.

## 1. System electrical basis

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| System voltage | CONFIRMED FACT | 24 V system architecture. | Rev A.4.1 schematic; [CZ4060 characterization](hardware/cz4060-characterization.md) | High | Validate production-supply tolerance and transients with the complete assembly. | BOM freeze + Fabrication |
| CZ4060 conservative design basis | CONFIRMED FACT | Size the heater path for 24 V, 200 W, and 8.33 A. | Heater rating and [CZ4060 characterization](hardware/cz4060-characterization.md), commit `903e5e0cdfc7a17102d51de1c01fa414be613b92` | High | Validate every production current-path element at this design load and installed temperature. | PCB layout + BOM freeze + Fabrication |
| Standalone sustained load | MEASURED | Approximately 189 W and 7.89 A at 24 V. | 60-minute standalone run in [CZ4060 characterization](hardware/cz4060-characterization.md) | High for tested specimen and setup | Repeat or bound unit-to-unit behavior; validate installed full-path voltage drop and temperature. | BOM freeze + Fabrication |
| Standalone startup | MEASURED | Approximately 196 W peak observed; no high-current cold inrush observed. Current rose over several minutes while the bench supply remained in CV mode. | Startup test in [CZ4060 characterization](hardware/cz4060-characterization.md) | High for tested specimen and setup | Confirm behavior with the production PSU, wiring, controls, and additional heater specimens. | BOM freeze + Fabrication |
| Standalone thermal behavior | MEASURED | Heater/PTC region approximately 130 °C steady; chassis approximately 74–75 °C steady. These are characterization results, not firmware or protection thresholds. | 60-minute standalone run in [CZ4060 characterization](hardware/cz4060-characterization.md) | High for tested specimen and setup | Installed-system mapping with production airflow, sensor placement, fault cases, and materials. | Firmware constants + BOM freeze + Fabrication |
| Cold DMM resistance | MEASURED | Approximately 20–21 Ω was observed cold, but it is not authoritative for heater-path sizing. Use the powered measurements and 8.33 A conservative design basis. | Cold-resistance note in [CZ4060 characterization](hardware/cz4060-characterization.md) | High | None for current-path design basis. | None |

Firmware thresholds must not be derived directly from the standalone heater
temperatures.

## 2. ESP32-S3 Super Mini mechanical implementation

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| Module PCB body | MEASURED | 23.67 mm long × 17.91 mm wide. | Physical measurement of the actual module | High | Confirm against the 1:1 production-footprint plot before freeze. | Fabrication |
| Overall length including USB-C | MEASURED | 25.07 mm. | Physical measurement of the actual module | High | Confirm connector projection and carrier-board relationship on the 1:1 plot. | PCB layout + Fabrication |
| USB-C overhang | MEASURED | Approximately 1.40 mm beyond the module PCB edge. This is a datum, not a complete connector keepout. | Physical measurement of the actual module | High for overhang only | Measure shell width, height, position, mating plug, cable-head envelope, and enclosure access. | PCB layout + Fabrication |
| Pad count | CONFIRMED FACT | Nine castellations per side; 18 total. | Physical inspection of the actual module | High | Verify every pad number and label against the actual module silkscreen. | PCB layout + Fabrication |
| Nominal pin pitch | ASSUMPTION / PROVISIONAL | 2.54 mm; supported by an approximately 20.0 mm measured span across eight intervals, versus 20.32 mm nominal. | Physical measurement and derived nominal geometry | High | 1:1 physical placement and direct center-to-center verification. | PCB layout + Fabrication |
| Nominal row spacing | ASSUMPTION / PROVISIONAL | 15.24 mm; supported by an approximately 15.2 mm physical measurement. | Physical measurement and derived nominal geometry | High | 1:1 physical placement and direct row-center verification. | PCB layout + Fabrication |
| Side edge to row center | ASSUMPTION / PROVISIONAL | Approximately 1.335 mm, derived as `(17.91 - 15.24) / 2`. | Derived from measured body width and provisional nominal row spacing | Medium | Measure actual castellation centerline relative to both module edges. | PCB layout + Fabrication |
| End to first/last pin center | ASSUMPTION / PROVISIONAL | Approximately 1.675 mm if symmetric, derived as `(23.67 - 20.32) / 2`. | Derived from measured body length and provisional nominal pitch | Medium | Measure the actual longitudinal registration; do not assume symmetry. | PCB layout + Fabrication |
| Castellation receiving lands | TBD / BLOCKED | Pad width and depth are intentionally undefined. | No authoritative physical dimensions or trusted revision-matched drawing available | High confidence that evidence is missing | Measure castellation width, inward depth, plating/mask geometry, and body registration. | PCB layout + Fabrication |
| Production mounting | DESIGN DECISION | Direct solder of the castellated module to carrier-PCB SMD lands. The current drilled through-hole footprint is not the production architecture unless later physical verification explicitly reverses this decision. | Phase-1 mounting decision; inspection of Rev A.4.1 project footprint | High | Complete and physically verify the project-local castellated footprint. | PCB layout + Fabrication |
| Central underside PCB window | DESIGN DECISION | Provide a carrier-PCB window for underside component clearance, inspection/cleaning, avoiding a header/standoff dependency, and reducing trapped heat. It is not a safety airflow feature. | Phase-1 mechanical decision | High | Measure underside X/Y envelope, maximum height, solder/metal features, support regions, and required FR-4 rail widths. | PCB layout + Fabrication |
| Underside window geometry | TBD / BLOCKED | No length, width, corner, or support-rail dimensions are frozen. | Required physical underside measurements are unavailable | High confidence that evidence is missing | Complete underside measurements and structural/routing review. | PCB layout + Fabrication |
| Optional temporary locating holes | DESIGN DECISION | Two holes total, one at each end of one castellated row, for a removable 2.54 mm header assembly aid. They are not electrical pins or production attachment. | Phase-1 assembly decision | High | Measure the actual header posts; select NPTH/PTH; establish hole diameter and coordinates; check rails, USB, antenna, window, and removal clearance. | PCB layout + Fabrication |
| Locating-hole geometry | ASSUMPTION / PROVISIONAL | Candidate holes are Ø1.10 mm NPTH with no copper at X = −7.620 mm, Y = ±12.700 mm. Provisional footprint datum: origin at the module PCB geometric center; X runs across module width, Y runs along module length, and USB-C is at negative Y. The geometry is not final. | Phase-1 mounting/assembly-aid review using nominal 2.54 mm row extension and a nominal 0.635 mm square header-post concept | Medium | Measure the actual header; verify finished-hole and positional tolerances; physically fit-test insertion/removal; check interference with lands, support rails, window, antenna, and USB. | PCB layout + Fabrication |
| Antenna keepout | TBD / BLOCKED | Location, dimensions, and applicable copper layers are undefined. | Actual module antenna has not been physically identified and measured | High confidence that evidence is missing | Identify antenna on the actual revision; measure prohibited region; define copper/trace/component restrictions. | PCB layout + Fabrication |
| USB-C mechanical keepout | TBD / BLOCKED | Full shell, plug, cable-head, carrier-edge, and enclosure access envelope is undefined. | Only overall length and approximate overhang have been measured | High confidence that evidence is missing | Complete physical connector and service-access measurements. | PCB layout + Fabrication |
| Mandatory 1:1 verification | DESIGN DECISION | The actual module must be placed on a true-scale plot before footprint freeze. All 18 castellations, body, Pin 1, locating holes, USB projection, antenna direction, and underside window must pass without forced alignment. | Phase-1 acceptance requirement | High | Perform and document the physical fit check using the final proposed footprint. | Fabrication |

## 3. ESP32 power architecture

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| Production power domains | DESIGN DECISION | Protected 24 V feeds the onboard buck. Its buck-only `+5V_SYS/GATE` domain powers gate-drive circuitry. A separately isolated or source-selected `+5V_MCU` domain feeds the ESP32-S3 Super Mini external 5 V/VBUS input. Raw 24 V must never reach the module, and USB service power must never energize gate-drive circuitry. | Phase-1 power-domain and service-safety decision | High | Verify module power pin/topology; select and qualify the regulator, isolation/source-selection method, sequencing, and power-domain implementation. | PCB layout + BOM freeze + Fabrication |
| USB-C role | DESIGN DECISION | Programming, debug, recovery, and service only; not the normal installed power source. USB-only MCU power must not energize the gate-driver domain or enable Q1. | Phase-1 service architecture decision | High | Validate every USB-only and mixed-supply power state and observe gate/OE during attach/remove. | PCB layout + Fabrication |
| External 5 V/VBUS pin | TBD / BLOCKED | Rev A.4.1 mapping must not substitute for physical verification of the exact module pin. | No actual-module label/circuit verification recorded | High confidence that evidence is missing | Verify the physical pin label, numbering, and electrical connection. | PCB layout + Fabrication |
| USB/external-5 V relationship | TBD / BLOCKED | Unknown whether USB-C VBUS and the external 5 V pin are directly connected, diode-ORed, switched, or otherwise isolated. | No trustworthy revision-matched schematic or circuit inspection | High confidence that evidence is missing | Inspect/trace the actual module and test onboard-only, USB-only, and simultaneous-source behavior. | PCB layout + BOM freeze + Fabrication |
| Backfeed protection | TBD / BLOCKED | Reverse-current blocking or explicit source selection is required unless inspection and testing prove it unnecessary. Final implementation may use Schottky OR-ing, an ideal-diode/load-switch arrangement, USB-VBUS isolation, or a service-power selector; no option is selected. | Depends on actual module topology and measured source behavior | High confidence that decision is blocked | Prove that USB cannot backfeed the buck/24 V system, onboard 5 V cannot unexpectedly source a USB host, and supplies cannot contend. | PCB layout + BOM freeze + Fabrication |
| Buck converter | TBD / BLOCKED | No MPN or final rating selected. Must support ESP32-S3 peaks/Wi-Fi bursts, boot transients, shared 74AHCT125 and fan-interface loads, other actual 5 V loads, and engineering margin. | Load inventory and installed thermal environment incomplete | High confidence that selection is blocked | Complete load inventory; evaluate voltage/transient rating, output current, efficiency, dissipation, reverse current, startup, and installed-temperature derating. | PCB layout + BOM freeze + Fabrication |
| Buck converter capacity target | ASSUMPTION / PROVISIONAL | Use at least 1.5 A continuous capability as an initial converter target. This is not a selected MPN or BOM-final rating. | Preliminary ESP32-S3 peak/burst allowance plus light interface-load margin | Medium | Complete the 5 V load inventory and converter loss/thermal derating at the maximum installed chamber/PCB temperature; increase the rating if required. | PCB layout + BOM freeze + Fabrication |
| Power-management firmware assumptions | TBD / BLOCKED | No firmware behavior may assume a source topology or power-good behavior that has not been established in hardware. | Power architecture unresolved | High | Close power-state truth table and any available power-good/reset behavior. | Firmware constants |

## 4. Heater path and independent interruption

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| Rev A.4.1 heater path | CONFIRMED FACT | `+24V_PROT -> F2 -> TF1 -> TS1 -> J2 -> external heater -> Q1 -> GND` | Current Rev A.4.1 schematic | High | Recheck connectivity after every schematic revision and against the first PCB. | PCB layout + Fabrication |
| Independent series protection | CONFIRMED FACT | TF1 and TS1 are physically in series with heater current; firmware is not the sole means of heater interruption. | Current Rev A.4.1 schematic | High for topology | Select and validate final parts, mounting, temperatures, current capacity, and installed fault behavior. | PCB layout + BOM freeze + Firmware constants + Fabrication |
| Complete heater-current path | TBD / BLOCKED | The assembled path has not been validated for voltage drop, heating, or fault interruption. | [CZ4060 characterization](hardware/cz4060-characterization.md) explicitly excludes Q1, F2, PCB copper, connectors, wiring, and installed behavior | High confidence that validation is incomplete | Validate final parts, PCB geometry, full-path voltage drop, component/copper temperatures, and installed fault cases. | BOM freeze + Fabrication |

## 5. Q1 heater MOSFET and gate drive

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| Q1 candidate | STRONG CANDIDATE | Texas Instruments CSD18540Q5B; 60 V logic-level N-MOSFET; maximum `RDS(on)` approximately 3.3 mΩ at 4.5 V. Not BOM-final. | Manufacturer data and Phase-1 component review | High for published characteristics | Reconfirm ordering code, lifecycle, package, pin/multipad mapping, and production availability. | PCB layout + BOM freeze + Fabrication |
| Estimated Q1 conduction loss | ASSUMPTION / PROVISIONAL | About 0.205 W at 7.89 A and 0.229 W at 8.33 A using 3.3 mΩ: `P = I²R`. These are max-`RDS(on)`-basis conduction estimates only, not total hot operating loss. | Candidate data and measured/design currents | Medium | Include hot `RDS(on)`, switching loss, duty/frequency, gate waveform, copper spreading, and measured junction/board temperature. | PCB layout + BOM freeze + Firmware constants + Fabrication |
| Rev A.4.1 gate buffer | CONFIRMED FACT | Rev A.4.1 uses an SN74AHCT125 for gate buffering, and the used output-enable inputs are tied low. This records the present schematic, not approval of its power-transition behavior. | Current Rev A.4.1 schematic | High | Reinspect after the required schematic correction and verify the implemented channel mapping. | PCB layout + Fabrication |
| Gate network concept | ASSUMPTION / PROVISIONAL | Provisional 47 Ω gate resistor and a direct Q1 gate pull-down. The direct pull-down must be retained independently of MCU and buffer state. | Current Rev A.4.1 schematic concept and Phase-1 default-OFF requirement | Medium | Select the pull-down value and gate resistance from leakage, turn-off, waveform, PWM, and switching-loss validation. | PCB layout + BOM freeze + Firmware constants + Fabrication |
| Required gate-drive correction | DESIGN DECISION | Heater gate drive must be hardware-default-disabled through power-up, power-down, reset, and brownout. Power the gate driver only from the 24 V-derived `+5V_SYS/GATE` domain so USB-only MCU power cannot drive Q1. Use a driver with documented partial-power-down behavior or equivalent isolation. Add test points for gate-driver power, OE, and Q1 gate. No replacement driver is BOM-final. | Phase-1 power/backfeed and default-OFF review | High | Revise the schematic; select/qualify the driver or isolation; verify OE biasing, partial-power sequencing, and Q1 gate voltage through all 24 V/USB/reset transitions. | PCB layout + BOM freeze + Firmware constants + Fabrication |
| Q1 footprint and thermal implementation | TBD / BLOCKED | Exact verified KiCad footprint, thermal-pad implementation, and copper spreading are not frozen. | Footprint/PCB validation not completed | High confidence that implementation is incomplete | Verify package drawing and pad mapping; complete thermal/current layout and measurement plan. | PCB layout + Fabrication |

## 6. F2 fuse and holder

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| F2 fuse candidate | STRONG CANDIDATE | Littelfuse 0287015.U, 15 A ATOF. Not BOM-final. The 15 A candidate provides healthier margin around the approximately 7.9–8.33 A load than a 10 A fuse after elevated-temperature derating. | Heater characterization, conservative design basis, and Phase-1 component review | Medium-high | Validate production-PSU short-circuit/current-limit behavior, time-current coordination, ambient derating, interrupt behavior, and service conditions. | PCB layout + BOM freeze + Fabrication |
| F2 holder candidate | STRONG CANDIDATE | Littelfuse 178.6165.0001. Not BOM-final. | Phase-1 component review | Medium-high | Verify exact footprint, current/temperature performance, pad and copper heating, mechanical retention, and removal/service clearance. | PCB layout + BOM freeze + Fabrication |

## 7. J2 and heater wiring

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| J2 candidate | STRONG CANDIDATE | Phoenix Contact 1715022 / MKDS 1,5/2; nominal IEC current approximately 17.5 A. The 8.33 A heater basis is approximately 48% of that nominal value. Not BOM-final. | Manufacturer rating and Phase-1 component review | Medium-high | Verify exact footprint, conductor/ferrule compatibility, torque, terminal and pad temperature, creepage/clearance, and service access. | PCB layout + BOM freeze + Fabrication |
| Heater wire | DESIGN DECISION | Use 16 AWG high-temperature-insulated wire, with approximately 200 °C-class insulation near the heater. | Current requirement and heater thermal environment | High for direction | Finalize insulation construction, color, length, routing, bend radius, abrasion protection, and installed-temperature margin. | BOM freeze + Fabrication |
| Wire family | STRONG CANDIDATE | TE Connectivity/Raychem Spec 55 family; examples 55/0212-16-2 red and 55/0212-16-0 black. Not BOM-final. | Phase-1 wiring review | Medium | Verify exact construction, approvals, availability, termination method, voltage drop, and harness temperature. | BOM freeze |
| Heater termination details | TBD / BLOCKED | Ferrule type, terminal torque, strain relief, wire length, voltage drop, and terminal/harness temperatures are not frozen. | Harness and installed validation incomplete | High confidence that details are incomplete | Build and thermally validate the production-intent harness and termination. | BOM freeze + Fabrication |
| JST-XH heater use | DESIGN DECISION | JST-XH is not acceptable for the approximately 8 A heater-current path. | Current-path requirement | High | None; maintain this exclusion through schematic and PCB review. | None |

## 8. Fan architecture and prototype

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| Original integrated fan | CONFIRMED FACT | YD4020HBL, 24 V, 0.13 A, two-wire, no native PWM, and no tach. Its two-wire control architecture has been superseded by the four-wire PWM/tach direction. | Physical/product identification from CZ4060 assembly review | Medium-high | Confirm nameplate and electrical behavior if retained for comparative testing only. | None |
| Replacement fan prototype | PROTOTYPE CANDIDATE | Sanyo Denki 9GA0424P3J001; ordered for characterization, ETA 2026-09-15. Not BOM-final. | Prototype purchase record | High for order state; unvalidated for application | Complete electrical, airflow, pressure, acoustic, thermal, stall, and installed-system characterization. | BOM freeze + Firmware constants |
| Four-wire fan architecture | DESIGN DECISION | Continuous fused 24 V fan power, separate PWM control, and separate tach/RPM feedback. Do not low-side PWM the fan's 24 V supply when using native four-wire control. | Phase-1 fan architecture decision | High | Verify candidate interface requirements and qualify final fan/control parts. | PCB layout + BOM freeze + Fabrication |
| Fan PWM interface | ASSUMPTION / PROVISIONAL | Provide a fan-compatible open-drain interface; exact circuit and component values remain pending datasheet and bench verification. | Common four-wire architecture and Phase-1 direction; candidate-specific evidence incomplete | Medium | Verify pinout, PWM voltage/current/frequency requirements, pull-up behavior, open/high-Z behavior, and 0% behavior. | PCB layout + BOM freeze + Firmware constants + Fabrication |
| Fan tach interface | TBD / BLOCKED | Tach output type, pull-up rail/value, pulses per revolution, filtering, protection, and stall semantics are not established. | Prototype not yet characterized | High confidence that evidence is missing | Characterize output electrically and across startup, minimum speed, normal operation, and locked rotor. | PCB layout + BOM freeze + Firmware constants + Fabrication |
| Fan performance envelope | TBD / BLOCKED | Startup/steady current, minimum reliable PWM/speed, RPM versus duty, CZ4060 airflow/static pressure, locked-rotor behavior, stall-detection latency, hub temperature, installed chamber margin, cooldown versus RPM, and minimum safe airflow/RPM remain unknown. | Prototype characterization pending | High confidence that evidence is missing | Complete bench and installed-system characterization. Do not freeze RPM thresholds beforehand. | BOM freeze + Firmware constants |

The first PCB may include configurable interface provisions, but fabrication is
acceptable only if the unresolved interface is deliberately bounded and does not
require unsupported electrical assumptions.

## 9. Independent thermal protection

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| TF1 and TS1 schematic values | TBD / BLOCKED | Rev A.4.1 placeholders are TF1 75 °C / 15 A and TS1 normally closed 75 °C / 16 A. The 75 °C values are not finalized production selections. | Current Rev A.4.1 schematic and standalone thermal results | High confidence that placeholders are not validated | Select temperatures only after installed-system thermal and fault mapping. | BOM freeze + Firmware constants + Fabrication |
| Thermal-device families | ASSUMPTION / PROVISIONAL | SCHOTT SEFUSE SF and Protherm 03EC are families to investigate, not selected parts and not BOM-final. | Phase-1 component-family review | Low-medium | Establish exact construction, package, current/voltage rating, trip tolerance, mounting method, aging, availability, and approvals. | PCB layout + BOM freeze + Fabrication |
| Protection ordering | DESIGN DECISION | `normal operating ceiling < firmware hard trip < independent hardware cutoff < lowest applicable material/component limit` with justified margins. | JumpJet safety architecture | High | Establish every numeric boundary from installed measurements and component/material limits. | BOM freeze + Firmware constants + Fabrication |
| Installed thermal evidence | TBD / BLOCKED | Required measurements include TF1 body, TS1 body, heater hotspot, chamber, enclosure/material, wiring, and connector temperatures under normal, failed-fan, restricted-airflow, and heater-stuck-on cases. | Standalone characterization explicitly does not cover installed behavior | High confidence that evidence is missing | Complete production-intent installed thermal mapping and fault testing. | BOM freeze + Firmware constants + Fabrication |

The approximately 130 °C heater/PTC and 74–75 °C chassis measurements do not
identify a valid production cutoff temperature or mounting location.

## 10. PCB implementation

| Item | Status | Current value / decision | Evidence / source | Confidence | Remaining validation | Blocks |
|---|---|---|---|---|---|---|
| Rev A PCB state | CONFIRMED FACT | PCB design is the current Rev A implementation stage. There is no missing authoritative PCB artifact to recover; a `.kicad_pcb` has not yet been created and must now be designed. | Rev A.4.1 package scope and Phase-1 project state | High | Create and review the first Rev A PCB from the authoritative schematic. | PCB layout + Fabrication |
| Starting stackup | ASSUMPTION / PROVISIONAL | Two layers, 1.6 mm FR-4, 2 oz copper. | Phase-1 routing/current-density starting point | Medium | Review high-current routing, grounding, RF, mechanical, thermal, cost, and PCBWay manufacturing requirements before freeze. | PCB layout + BOM freeze + Fabrication |
| Heater-path copper geometry | ASSUMPTION / PROVISIONAL | Keep heater-path neckdowns at least 4 mm as an initial minimum; prefer broad 8–10 mm or wider pours where practical; minimize vias and unnecessary current-loop area. Place the high-current/safety path before convenience circuitry. | 8.33 A conservative design basis and Phase-1 layout direction | Medium | Calculate and inspect every bottleneck, layer transition, pad entry, thermal relief, and temperature rise for the final stackup. | PCB layout + Fabrication |
| Ground and return strategy | DESIGN DECISION | Prefer a continuous ground plane with a controlled heater/fan/buck high-current return corridor. Thermistor/ADC returns must not share narrow or common-impedance copper with heater current. Do not create arbitrary split grounds that damage ESP32 RF return integrity. | Phase-1 signal-integrity and current-return strategy | High for topology; implementation provisional | Review actual return geometry, buck loops, ADC noise, RF keepout, and current-path coupling after placement/routing. | PCB layout + Fabrication |
| High-current placement priority | DESIGN DECISION | Place and review `+24V_PROT -> F2 -> TF1 -> TS1 -> J2 -> heater -> Q1 -> GND` before convenience circuitry. Keep Q1, J2, F2, and the heater-current path compact without weakening conductor geometry for appearance. | Rev A.4.1 topology and Phase-1 implementation priority | High | Complete placement, current-loop, service-clearance, and thermal review. | PCB layout + Fabrication |
| PCB validation | TBD / BLOCKED | Current-density bottlenecks, Q1 thermal spreading, fuse-holder and J2 pad heating, high-current return geometry, ADC noise, antenna keepout, DRC, fabricated-board temperature rise, and full-path voltage drop are unvalidated. | No Rev A PCB exists yet | High confidence that validation is pending | Create the board, review design evidence, fabricate only after blockers close, then thermally/electrically test production-intent hardware. | PCB layout + BOM freeze + Fabrication |

## Current fabrication blockers

Fabrication remains blocked until all of the following are closed or deliberately
bounded by reviewed production-intent provisions:

- ESP32 castellation dimensions and exact body/pad registration
- physical pin numbering and Pin 1 verification
- underside component envelope and maximum height
- PCB window and support-rail geometry
- antenna location and keepout
- USB connector, plug, cable-head, and service-access envelope
- physical fit of the optional locating header
- external 5 V / USB-C VBUS topology
- USB/onboard mixed-source and backfeed testing
- hardware-default-OFF gate-driver schematic correction
- complete regulator load inventory and installed-temperature thermal analysis
- Q1 multipad/footprint and F2 holder-footprint verification
- TF1/TS1 part, mounting, and thermal-coupling implementation
- J1, F1, and MOD1 part and footprint selections
- PCB envelope, mounting-hole coordinates, connector-facing edges, and USB datum
- final 1:1 production-footprint fit check with the actual module

## Register maintenance rules

- Preserve the exact status classes defined in this document.
- Change a status only when the evidence/source and remaining-validation fields
  are updated at the same time.
- A `STRONG CANDIDATE` or `PROTOTYPE CANDIDATE` does not become BOM-final by
  appearing here.
- Record contradictions and failed validation directly; do not silently replace
  inconvenient evidence.
- Link measurement reports, schematic revisions, datasheets, test records, and
  relevant commits wherever practical.
- Keep firmware GPIOs, ADC constants, PWM/RPM thresholds, thermal thresholds,
  and recovery limits provisional until the corresponding hardware evidence is
  complete.
