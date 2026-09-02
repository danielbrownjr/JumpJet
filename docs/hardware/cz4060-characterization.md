# CZ4060 standalone heater characterization

## Scope

This document records standalone electrical and thermal characterization of the CZ4060 heater. It is not validation of the complete Jump Jet heater path or the heater's behavior when installed in a printer chamber.

The measured temperatures in this document are characterization results only. They are not firmware trip thresholds, PCB or component allowable temperatures, or final product limits. This test did not validate Q1, F2, PCB copper, connectors, wiring, production thermistor placement, or installed chamber behavior.

## Test setup

- Heater: CZ4060
- Supply: 24.0 V
- Bench PSU: 0–30 V / 0–10 A
- PSU mode during characterization: constant voltage (CV)
- Current limit: 9.0 A
- Integrated fan powered normally
- Airflow unobstructed
- Ambient: approximately 22.2 °C

## Startup transient

| Time | Current | Power |
|---|---:|---:|
| ~10 s | 2.348 A | 56.3 W |
| ~30 s | 3.308 A | 79.3 W |
| ~60 s | 4.452 A | 106.8 W |
| ~90 s | 5.575 A | 133.8 W |
| ~120 s | 6.528 A | 156.8 W |
| ~150 s | 7.518 A | 180.4 W |
| ~180 s | 8.178 A | 196.2 W |

### Observations

- No high-current cold inrush was observed.
- The heater ramped upward into its rated-power region over several minutes.
- Peak observed power during the ramp was approximately 196 W.
- The PSU remained in CV mode.

## 60-minute steady-state run

| Time | Current | Power | Heater/PTC region | Chassis |
|---|---:|---:|---:|---:|
| 10 min | 7.896 A | 189.5 W | 130 °C | 55 °C |
| 20 min | 7.894 A | 189.4 W | 130 °C | 66.9 °C |
| 30 min | 7.883 A | 189.1 W | 130 °C | 66 °C |
| 45 min | 7.881 A | 189.0 W | 130 °C | 75 °C |
| 60 min | 7.893 A | 189.4 W | 130 °C | 74.1 °C |

### Conclusions from the standalone run

- Sustained load was approximately 189 W / 7.89 A at 24 V.
- The measured heater/PTC region stabilized around 130 °C.
- The measured chassis temperature stabilized around approximately 74–75 °C.
- No long-term electrical or heater-temperature creep was observed over the 60-minute run.
- The 24 V / 200 W nameplate rating is conservative but realistic for design sizing.

## Fan-assisted cooldown

Starting conditions:

- Heater/PTC region: approximately 130 °C
- Chassis: approximately 74.1 °C
- Ambient: approximately 22.2 °C

| Cooldown time | Heater/PTC region | Chassis |
|---|---:|---:|
| 1 min | 58 °C | 38 °C |
| 2 min | 49 °C | 32 °C |
| 5 min | 40 °C | 27 °C |
| ~10 min | ~33 °C | ~25 °C |

### Conclusions from the standalone cooldown

- Most stored heat was removed in the first few minutes with the integrated fan running.
- The chassis was close to ambient within roughly 5–10 minutes.
- These results support evaluating a temperature-based cooldown policy rather than relying on a long fixed timer.
- Final cooldown thresholds remain TBD until production sensor placement and system-level validation are complete.

## Cold resistance note

An isolated cold DMM measurement was approximately 20–21 Ω. This value is not representative of powered operating impedance and must not be used to infer heater wattage or size the power path. Powered characterization is authoritative for current and power behavior.

For design purposes, the hardware must still be conservatively sized around the rated case:

- 24 V
- 200 W
- approximately 8.33 A

## Engineering implications

The measured startup and steady-state behavior provide real design inputs for future work on:

- F2 fuse coordination
- Q1 MOSFET conduction-loss and thermal analysis
- PCB copper current loading
- connector loading
- wiring voltage drop
- terminal and contact heating
- thermal-cutoff placement
- firmware failure-to-heat and cooldown modeling

These inputs do not themselves validate any of those elements. In particular, the measured 130 °C heater-region temperature and 74–75 °C chassis temperature are not production firmware thresholds or component allowable temperatures.

## Remaining validation

The following work is still required before the heater path is considered production-validated:

- Characterize the actual Jump Jet power path through Q1, F2, PCB copper, connectors, and wiring.
- Measure voltage drop through the full power path at approximately 8 A.
- Measure MOSFET temperature.
- Measure fuse temperature.
- Measure connector temperature.
- Measure PCB and copper temperature.
- Perform installed chamber and system thermal characterization.
- Validate production thermistor placement and response.
- Derive the final failure-to-heat threshold.
- Derive the abnormal-rise threshold.
- Derive the cooldown-complete threshold.
- Derive recovery criteria.
