# INA228 HIL Validation Report

- Date/time: 2026-06-23T14:24:24.844850+02:00 to 2026-06-23T14:24:29.377078+02:00
- Elapsed: 4.5 s
- Port: COM21
- Baud: 115200
- Suite: functional
- Soak requested: 0.0 s
- Operator: Codex
- Board/environment: ESP32S3_COM21 / esp32s3dev_Arduino
- Fixture: INA228_0x41_low_voltage_no_fault_injection
- Safety assumptions: benign_fixture_no_fault_stimulus_unattended_low_voltage
- OS: Windows-11-10.0.26200-SP0
- Python: 3.12.10
- HIL command: `tools\run_i2c_hil.py --port COM21 --baud 115200 --suite functional --timeout-s 8 --idle-s 0.3 --boot-settle-s 0.5 --boot-capture-s 1 --post-frame-drain-s 0.05 --empty-retries 2 --require-framed --fail-on-unknown --report docs\reports\hil-recover-contract-check-COM21-20260623.md --transcript docs\reports\hil-recover-contract-check-COM21-20260623.log --operator Codex --board ESP32S3_COM21 --environment esp32s3dev_Arduino --fixture INA228_0x41_low_voltage_no_fault_injection --safety benign_fixture_no_fault_stimulus_unattended_low_voltage --notes recover_expectation_contract_check`
- Branch: hardening/ina228-industry-readiness
- Commit: 851ac4cd178bb01df220ab60a07a7f5abe982e39
- Dirty status:

```text
M examples/01_basic_bringup_cli/main.cpp
 M tools/run_i2c_hil.py
?? docs/reports/hil-20h-shakedown-COM21-20260623.md
?? docs/reports/hil-20h-shakedown-COM21-20260623.pid
?? docs/reports/hil-20h-shakedown2-COM21-20260623.md
?? docs/reports/hil-20h-shakedown2-COM21-20260623.pid
?? docs/reports/hil-20h-targeted-soak-COM21-20260623-attempt1.md
?? docs/reports/hil-20h-targeted-soak-COM21-20260623-attempt1.pid
?? docs/reports/hil-20h-targeted-soak-COM21-20260623.md
?? docs/reports/hil-20h-targeted-soak-COM21-20260623.pid
?? docs/reports/hil-post-shakedown-failure-smoke-COM21-20260623.md
?? docs/reports/hil-post-soak-failure-smoke-COM21-20260623.md
?? docs/reports/hil-post-soak-failure2-smoke-COM21-20260623.md
?? docs/reports/hil-recover-contract-check-COM21-20260623.md
```

- Transcript: `docs/reports/hil-recover-contract-check-COM21-20260623.log`
- Notes: recover_expectation_contract_check

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 44 | 0 | 0 | 5 |

## Timing Summary

- Commands executed: 49
- Commands recorded in detail: 49
- Soak commands executed: 0
- Soak rows recorded in detail: 0
- Recorded command latency min/mean/max: 0.062 / 0.067 / 0.188 s

- Maximum consecutive FAIL verdicts: 0

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | INA228 library version | === Version Info === / Example firmware build: Jun 23 2026 14:04:48 / INA228 library version: 2.0.0 / INA228 library full: 2.0.0 (851ac4c, 2026-06-23 14:04:4... | PASS | 0.062 | version |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / 00:                         -- -- -- -- -- -- -- -- / 10: ... | PASS | 0.188 | scan |
| 3 | smoke | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.062 | probe |
| 4 | smoke | `settings` | Active Settings, State:, Address: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.063 | settings |
| 5 | smoke | `drv` | Driver Health, State:, Online: | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 0 / Total failures: 0 / Success rate... | PASS | 0.062 | health |
| 6 | smoke | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | DIAG_ALRT |
| 7 | smoke | `raw` | Raw Registers, Vbus, Temp | === Raw Registers === / Vshunt: 36 (0x000024) / Vbus:   2214 (0x0008A6) / Temp:   3626 (0x0E2A) / Current:39 (0x000027) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | conversion raw read |
| 8 | functional | `help` | INA228 CLI Help, read, raw | === INA228 CLI Help === / [W] Safety: this example does not make 85 V systems safe. Use qualified design practices, isolation where needed, fusing, creepage/... | PASS | 0.063 | help |
| 9 | functional | `scanina` | INA228 Address Probe, Healthy INA228 devices | === INA228 Address Probe (0x40-0x4F) === / Note: INA228 probes read DIAG_ALRT for MEMSTAT and can clear CNVRF/latched diagnostic evidence. / 0x40: -- / 0x41:... | PASS | 0.062 | INA scan |
| 10 | functional | `mfgid` | Manufacturer ID | [I] Manufacturer ID: 0x5449 / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | manufacturer ID |
| 11 | functional | `devid` | Device ID | [I] Device ID: 0x2281 / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | device ID |
| 12 | functional | `timing` | Conversion ready, Estimated conversion time | Conversion ready: YES / Estimated conversion time: 3156 us (4 ms) / CURRENT_LSB: 0.000019073 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | timing |
| 13 | functional | `vbus` | Vbus | [I] Vbus: 0.4313 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | bus voltage |
| 14 | functional | `vshunt` | Vshunt | [I] Vshunt: 0.0000053 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | shunt voltage |
| 15 | functional | `temp` | Temp | [I] Temp: 28.34 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | die temperature |
| 16 | functional | `current` | Current | [I] Current: 0.000858 A / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | current |
| 17 | functional | `power` | Power | [I] Power: 0.000244 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | power |
| 18 | functional | `energy` | Energy | [I] Energy: 0.000000000 J / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | energy |
| 19 | functional | `charge` | Charge | [I] Charge: 0.134220123 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | charge |
| 20 | functional | `read` | Vbus, Power, Accum | [I] Reading all measurements: / Vbus:    0.4316 V / Vshunt:  0.0000113 V / Temp:    28.34 C / Current: 0.000744 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | aggregate read |
| 21 | functional | `diag` | DIAG_ALRT Flags, MEMSTAT | === DIAG_ALRT Flags === / Note: this read is destructive/status-clearing for CNVRF and latched diagnostic evidence. / MEMSTAT:   yes / CNVRF:     yes / ALATC... | PASS | 0.062 | parsed diagnostics |
| 22 | functional | `limits` | Alert Limits | === Alert Limits === / SOVL:      0x7FFF  163.835 mV / SUVL:      0x8000  -163.840 mV / BOVL:      0x7FFF  102.3969 V / BUVL:      0x0000  0.0000 V / TEMP_LI... | PASS | 0.063 | alert limits |
| 23 | functional | `alatch` | Alert latch | [I] Alert latch: no / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | alert latch query |
| 24 | functional | `cnvralert` | Conversion-ready alert | [I] Conversion-ready alert: no / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | conversion alert query |
| 25 | functional | `alslow` | Slow alert | [I] Slow alert: no / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | slow alert query |
| 26 | functional | `apol` | Alert polarity | [I] Alert polarity: active-low / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | alert polarity query |
| 27 | functional | `mode` | Mode | Mode: CONT_ALL (15) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | mode query |
| 28 | functional | `convtime` | Conversion times | Conversion times: VBUS=1052us  VSHUNT=1052us  TEMP=1052us / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | conversion time query |
| 29 | functional | `averaging` | Averaging | Averaging: 1 samples / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | averaging query |
| 30 | functional | `adcrange` | ADC range | ADC range: +/-163.84mV / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | ADC range query |
| 31 | functional | `cal` | CURRENT_LSB | Calibration: Rshunt=0.015000 ohm  MaxCurrent=10.000000 A  CURRENT_LSB=0.000019073 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | calibration query |
| 32 | functional | `tempco` | Shunt temp coeff | [I] Shunt temp coeff: 0 ppm/degC / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | temperature coefficient query |
| 33 | functional | `tempcomp` | Temperature compensation | [I] Temperature compensation: no / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | temperature compensation query |
| 34 | functional | `delay` | Conversion delay | [I] Conversion delay: 0 x 2 ms (0 ms) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | conversion delay query |
| 35 | functional | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | readiness |
| 36 | functional | `reg16 0x3E` | 0x | Reg 0x3E = 0x5449 (21577) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | manufacturer register raw16 |
| 37 | functional | `reg16 0x3F` | 0x | Reg 0x3F = 0x2281 (8833) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | device register raw16 |
| 38 | functional | `reg24 0x05` | 0x | Reg 0x05 = 0x008A30 (35376) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | VBUS raw24 |
| 39 | functional | `reg40 0x09` | 0x | Reg 0x09 = 0x0000000000 / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | ENERGY raw40 |
| 40 | functional | `rstacc` | reset | [I] resetAccumulators(): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | accumulator reset |
| 41 | functional | `recover` | Attempting recovery, frame_status=OK | [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Co... | PASS | 0.062 | manual recover |
| 42 | functional | `selftest` | INA228 selftest | === INA228 selftest (diagnostic commands; reads DIAG_ALRT) === / Note: DIAG_ALRT reads can clear CNVRF and latched evidence. / [PASS] probe responds / [PASS]... | PASS | 0.063 | self-test |
| 43 | functional | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 86 ms / Rate: 581.40 samples/s / Vbus V:    min=0.4289 avg=0.4313 ma... | PASS | 0.140 | short stress |
| 44 | functional | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 24 ms / Rate: 2083.33 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.078 | short mixed stress |
| 45 | not-run | `<fixture: disconnected target>` | safe absent-device fixture | requires safe disconnect or switched fixture | NOT RUN | 0.000 | requires safe disconnect or switched fixture |
| 46 | not-run | `<fixture: bus fault injection>` | safe fault-injection fixture | requires safe NACK/timeout/bus-error injection | NOT RUN | 0.000 | requires safe NACK/timeout/bus-error injection |
| 47 | not-run | `<fixture: alert pin capture>` | alert pin instrumentation | requires alert-pin wiring and safe threshold stimulus | NOT RUN | 0.000 | requires alert-pin wiring and safe threshold stimulus |
| 48 | not-run | `<fixture: MCU reset or power cycle>` | controlled reset/power fixture | requires explicit reset/power-cycle control | NOT RUN | 0.000 | requires explicit reset/power-cycle control |
| 49 | not-run | `<8-hour soak>` | --soak-hours 8 | soak not requested for this run | NOT RUN | 0.000 | soak not requested for this run |

## Limitations

- Hardware safety and fixture details must be filled in by the operator.
- This runner records serial CLI evidence only; external instruments must be logged separately.
- Staged `maxInstructions` coverage is limited to the example CLI commands. The `transfer` suite records example callback counts, not logic-analyzer bus bytes. Example `tick()` calls between serial commands can add readiness reads; exact assertions are kept to deterministic paths and other paths record snapshots.
- Soak test was not requested in this run.
