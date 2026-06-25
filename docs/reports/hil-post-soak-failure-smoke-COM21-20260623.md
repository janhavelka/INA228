# INA228 HIL Validation Report

- Date/time: 2026-06-23T13:30:46.731790+02:00 to 2026-06-23T13:30:49.198240+02:00
- Elapsed: 2.5 s
- Port: COM21
- Baud: 115200
- Suite: smoke
- Soak requested: 0.0 s
- Operator: Codex
- Board/environment: ESP32S3_COM21 / esp32s3dev_Arduino
- Fixture: INA228_0x41_low_voltage_no_fault_injection
- Safety assumptions: benign_fixture_no_fault_stimulus_unattended_low_voltage
- OS: Windows-11-10.0.26200-SP0
- Python: 3.12.10
- HIL command: `tools\run_i2c_hil.py --port COM21 --baud 115200 --suite smoke --timeout-s 12 --idle-s 0.3 --boot-settle-s 0.5 --boot-capture-s 1 --command-pause-s 0.05 --drain-before-command-s 0.05 --require-framed --fail-on-unknown --report docs\reports\hil-post-soak-failure-smoke-COM21-20260623.md --transcript docs\reports\hil-post-soak-failure-smoke-COM21-20260623.log --operator Codex --board ESP32S3_COM21 --environment esp32s3dev_Arduino --fixture INA228_0x41_low_voltage_no_fault_injection --safety benign_fixture_no_fault_stimulus_unattended_low_voltage --notes post_20h_soak_attempt_failure_responsiveness_check`
- Branch: hardening/ina228-industry-readiness
- Commit: 851ac4cd178bb01df220ab60a07a7f5abe982e39
- Dirty status:

```text
?? docs/reports/hil-20h-targeted-soak-COM21-20260623.md
?? docs/reports/hil-20h-targeted-soak-COM21-20260623.pid
?? docs/reports/hil-post-soak-failure-smoke-COM21-20260623.md
```

- Transcript: `docs/reports/hil-post-soak-failure-smoke-COM21-20260623.log`
- Notes: post_20h_soak_attempt_failure_responsiveness_check

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 7 | 0 | 0 | 5 |

## Timing Summary

- Commands executed: 12
- Commands recorded in detail: 12
- Soak commands executed: 0
- Soak rows recorded in detail: 0
- Recorded command latency min/mean/max: 0.062 / 0.080 / 0.188 s

- Maximum consecutive FAIL verdicts: 0

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | INA228 library version | === Version Info === / Example firmware build: Jun 23 2026 11:46:12 / INA228 library version: 2.0.0 / INA228 library full: 2.0.0 (29228f1, 2026-06-23 11:46:1... | PASS | 0.062 | version |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | [runner] drained stale serial input before command: / > / [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / ... | PASS | 0.188 | scan |
| 3 | smoke | `probe` | Status: OK | [runner] drained stale serial input before command: / > / [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can cl... | PASS | 0.062 | probe |
| 4 | smoke | `settings` | Active Settings, State:, Address: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.062 | settings |
| 5 | smoke | `drv` | Driver Health, State:, Online: | [runner] drained stale serial input before command: / > / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failure... | PASS | 0.063 | health |
| 6 | smoke | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | DIAG_ALRT |
| 7 | smoke | `raw` | Raw Registers, Vbus, Temp | [runner] drained stale serial input before command: / > / === Raw Registers === / Vshunt: 18 (0x000012) / Vbus:   2207 (0x00089F) / Temp:   3668 (0x0E54) / C... | PASS | 0.063 | conversion raw read |
| 8 | not-run | `<fixture: disconnected target>` | safe absent-device fixture | requires safe disconnect or switched fixture | NOT RUN | 0.000 | requires safe disconnect or switched fixture |
| 9 | not-run | `<fixture: bus fault injection>` | safe fault-injection fixture | requires safe NACK/timeout/bus-error injection | NOT RUN | 0.000 | requires safe NACK/timeout/bus-error injection |
| 10 | not-run | `<fixture: alert pin capture>` | alert pin instrumentation | requires alert-pin wiring and safe threshold stimulus | NOT RUN | 0.000 | requires alert-pin wiring and safe threshold stimulus |
| 11 | not-run | `<fixture: MCU reset or power cycle>` | controlled reset/power fixture | requires explicit reset/power-cycle control | NOT RUN | 0.000 | requires explicit reset/power-cycle control |
| 12 | not-run | `<8-hour soak>` | --soak-hours 8 | soak not requested for this run | NOT RUN | 0.000 | soak not requested for this run |

## Limitations

- Hardware safety and fixture details must be filled in by the operator.
- This runner records serial CLI evidence only; external instruments must be logged separately.
- Staged `maxInstructions` coverage is limited to the example CLI commands. The `transfer` suite records example callback counts, not logic-analyzer bus bytes. Example `tick()` calls between serial commands can add readiness reads; exact assertions are kept to deterministic paths and other paths record snapshots.
- Soak test was not requested in this run.
