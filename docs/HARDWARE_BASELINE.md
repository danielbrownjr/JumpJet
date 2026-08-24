# Hardware baseline — not yet commissioned

The upstream Jetpack sketch used an ESP32-S3 Super Mini, three analog thermistors,
GPIO 12 for the fan, and GPIO 13 for the heater module. Those facts describe the
upstream prototype; they are **not** accepted Jump Jet PCB assignments.

Planned Jump Jet hardware currently includes a 24 V heater, two fans, MOSFET
control, thermistor sensing, a fuse, an independent thermal cutoff, a physical
switch, and JST-XH signal connections. Component ratings, divider values,
thermistor curves, GPIOs, current paths, connector loading, copper geometry,
airflow, and thermal thresholds remain unverified until the current schematic,
BOM, and mechanical airflow path are reviewed.

Consequently, the foundation firmware advertises no heating capability and
contains no heater GPIO/PWM driver.

The build currently uses a provisional 4 MiB dual-OTA partition layout because
that is the conservative ESP32-S3 Super Mini baseline. The exact module/flash
population on the Jump Jet PCB must be confirmed before this becomes a release
hardware constraint.
