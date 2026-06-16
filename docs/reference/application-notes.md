# Application Notes

Vendor application notes are retained under `vendor/` as design references.
They do not provide INA228 register definitions and are not validation evidence
for this library.

| PDF | Relevance |
| --- | --- |
| `simplifying_high_voltage_sensing.pdf` | Compares shunt-based INA devices with Hall-effect alternatives; useful for system trade-offs and high-voltage safety framing. |
| `integrating_current_sensing.pdf` | Describes shunt selection and current-sensing signal-path trade-offs. |
| `integrated_current_sensing_adc.pdf` | Explains integrated current-sensing ADC architecture and current/power offload. |
| `improving_power_amplifier.pdf` | Shows periodic and real-time current/power monitoring use cases. |
| `digital_interfaces.pdf` | Summarizes I2C/SMBus/PMBus considerations and high-speed I2C context. |
| `current_sense_trace_design.pdf` | Covers PCB trace shunts and their large accuracy/temperature trade-offs. |
| `current-sense-amplifiers-shunt-resistor-layout.pdf` | Covers Kelvin routing and shunt layout basics. |
| `closed_loop_power_drive.pdf` | Demonstrates closed-loop constant-power control using current/power telemetry. |

Common guidance relevant to INA228 users:

- Choose the shunt value as an accuracy, dissipation, and dynamic-range trade-off.
- Use Kelvin connections and short, balanced sense routing.
- Treat PCB copper as a poor precision shunt unless calibrated per board and
  temperature range.
- The INA228 high common-mode rating does not make a development board or USB
  connected system safe.
- ALERT and measured values are monitoring aids, not certified protection.
