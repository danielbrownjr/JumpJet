# Jetpack to Jump Jet migration

The original Arduino sketch is isolated in `legacy/jetpack-arduino/` as
unmaintained upstream historical reference. It is not production Jump Jet code.

| Upstream Jetpack behavior | Jump Jet direction |
|---|---|
| Credentials compiled into source | `dc_wifi` provisioning and persisted `dc_prusa` configuration |
| Arduino `loop()` | FreeRTOS safety task independent of networking |
| Direct `ledcWrite()` | Product-local actuator layer after hardware review |
| Three ad-hoc thermistor reads | Product-local calibrated ADC/NTC component with explicit open/short states |
| Combined PID and safety function | Auditable interlock, fault, actuator, and control layers |
| Unauthenticated GET mutations | Validated JSON mutations with a same-origin custom-header gate |
| Upload rejected only by image validity | Pre-upload heat guard, post-upload heat recheck, and project identity check |

The first milestone is intentionally cold-safe. A later hardware-commissioning
milestone will add ADC, fan, and heater drivers behind the tested interlock.
PrusaLink status age now flows from `dc_prusa` into the product interlock, but AUTO
remains disabled until the hardware is reviewed and commissioned.
