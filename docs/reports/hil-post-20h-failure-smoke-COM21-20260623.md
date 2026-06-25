# INA228 HIL Validation Report

- Date/time: 2026-06-23T15:40:16.594373+02:00 to 2026-06-23T15:40:18.731194+02:00
- Elapsed: 2.1 s
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
- HIL command: `tools\run_i2c_hil.py --port COM21 --baud 115200 --suite smoke --timeout-s 8 --idle-s 0.3 --boot-settle-s 0.5 --boot-capture-s 1 --post-frame-drain-s 0.05 --empty-retries 0 --require-framed --fail-on-unknown --report docs\reports\hil-post-20h-failure-smoke-COM21-20260623.md --transcript docs\reports\hil-post-20h-failure-smoke-COM21-20260623.log --operator Codex --board ESP32S3_COM21 --environment esp32s3dev_Arduino --fixture INA228_0x41_low_voltage_no_fault_injection --safety benign_fixture_no_fault_stimulus_unattended_low_voltage --notes post_20h_reset_start_lost_frame_smoke_probe`
- Branch: hardening/ina228-industry-readiness
- Commit: 851ac4cd178bb01df220ab60a07a7f5abe982e39
- Dirty status:

```text
M examples/01_basic_bringup_cli/main.cpp
 M tools/run_i2c_hil.py
?? docs/reports/hil-20h-intensive-soak-COM21-20260623.md
?? docs/reports/hil-20h-intensive-soak-COM21-20260623.pid
?? docs/reports/hil-20h-shakedown-COM21-20260623.md
?? docs/reports/hil-20h-shakedown-COM21-20260623.pid
?? docs/reports/hil-20h-shakedown2-COM21-20260623.md
?? docs/reports/hil-20h-shakedown2-COM21-20260623.pid
?? docs/reports/hil-20h-shakedown3-COM21-20260623.md
?? docs/reports/hil-20h-shakedown3-COM21-20260623.pid
?? docs/reports/hil-20h-targeted-soak-COM21-20260623-attempt1.md
?? docs/reports/hil-20h-targeted-soak-COM21-20260623-attempt1.pid
?? docs/reports/hil-20h-targeted-soak-COM21-20260623.md
?? docs/reports/hil-20h-targeted-soak-COM21-20260623.pid
?? docs/reports/hil-post-20h-failure-smoke-COM21-20260623.md
?? docs/reports/hil-post-shakedown-failure-smoke-COM21-20260623.md
?? docs/reports/hil-post-soak-failure-smoke-COM21-20260623.md
?? docs/reports/hil-post-soak-failure2-smoke-COM21-20260623.md
?? docs/reports/hil-recover-contract-check-COM21-20260623.md
```

- Transcript: `docs/reports/hil-post-20h-failure-smoke-COM21-20260623.log`
- Notes: post_20h_reset_start_lost_frame_smoke_probe

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 7 | 0 | 0 | 5 |

## Timing Summary

- Commands executed: 12
- Commands recorded in detail: 12
- Soak commands executed: 0
- Soak rows recorded in detail: 0
- Recorded command latency min/mean/max: 0.062 / 0.080 / 0.187 s

- Maximum consecutive FAIL verdicts: 0

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | INA228 library version | === Version Info === / Example firmware build: Jun 23 2026 14:04:48 / INA228 library version: 2.0.0 / INA228 library full: 2.0.0 (851ac4c, 2026-06-23 14:04:4... | PASS | 0.063 | version |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / 00:                         -- -- -- -- -- -- -- -- / 10: ... | PASS | 0.187 | scan |
| 3 | smoke | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.063 | probe |
| 4 | smoke | `settings` | Active Settings, State:, Address: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.062 | settings |
| 5 | smoke | `drv` | Driver Health, State:, Online: | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 0 / Total failures: 0 / Success rate... | PASS | 0.063 | health |
| 6 | smoke | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | DIAG_ALRT |
| 7 | smoke | `raw` | Raw Registers, Vbus, Temp | === Raw Registers === / Vshunt: 40 (0x000028) / Vbus:   2210 (0x0008A2) / Temp:   3629 (0x0E2D) / Current:20 (0x000014) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | conversion raw read |
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
