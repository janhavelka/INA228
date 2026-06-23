# INA228 HIL Validation Report

- Date/time: 2026-06-22T23:32:27.885801+02:00 to 2026-06-23T07:34:20.897815+02:00
- Elapsed: 28913.0 s
- Port: COM21
- Baud: 115200
- Suite: exhaustive
- Soak requested: 28800.0 s
- Operator: Codex
- Board/environment: ESP32S3_COM21 / esp32s3dev_Arduino
- Fixture: INA228_0x41_low_voltage_no_fault_injection
- Safety assumptions: benign_fixture_no_fault_stimulus
- OS: Windows-11-10.0.26200-SP0
- Python: 3.12.10
- HIL command: `tools\run_i2c_hil.py --port COM21 --baud 115200 --suite exhaustive --timeout-s 10 --idle-s 0.3 --boot-settle-s 1 --boot-capture-s 4 --prompt-token > --empty-retries 1 --marker-retries 1 --command-pause-s 0.1 --benchmark-count 100 --soak-hours 8 --soak-store-every 5000 --soak-progress-every 5000 --include-not-run --report docs\reports\hil-validation-COM21-20260622.md --transcript docs\reports\hil-validation-COM21-20260622.log --operator Codex --board ESP32S3_COM21 --environment esp32s3dev_Arduino --fixture INA228_0x41_low_voltage_no_fault_injection --safety benign_fixture_no_fault_stimulus --notes 8h_framed_100ms_paced_continue_through_UNKNOWN_rows_all_FAIL_UNKNOWN_retained --fail-on-unknown`
- Branch: hardening/ina228-industry-readiness
- Commit: 5840497166200a3b1eb9e8f68f447b6d7182367a
- Dirty status:

```text
M examples/01_basic_bringup_cli/main.cpp
 M examples/esp_idf/basic/main/main.cpp
 M tools/run_i2c_hil.py
?? docs/prompts/04-exhaustive-hil-validation-and-audit.md
?? docs/reports/
```

- Transcript: `docs/reports/hil-validation-COM21-20260622.log`
- Notes: 8h_framed_100ms_paced_continue_through_UNKNOWN_rows_all_FAIL_UNKNOWN_retained

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 256931 | 0 | 48 | 4 |

## Timing Summary

- Commands executed: 256983
- Commands recorded in detail: 864
- Soak commands executed: 256218
- Soak rows recorded in detail: 99
- Recorded command latency min/mean/max: 0.000 / 0.958 / 20.094 s

- Maximum consecutive FAIL verdicts: 0

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | INA228 library version | === Version Info === / Example firmware build: Jun 22 2026 21:16:34 / INA228 library version: 2.0.0 / INA228 library full: 2.0.0 (5840497, 2026-06-22 21:16:3... | PASS | 0.000 | version |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / 00:                         -- -- -- -- -- -- -- -- / 10: ... | PASS | 0.140 | scan |
| 3 | smoke | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.016 | probe |
| 4 | smoke | `settings` | Active Settings, State:, Address: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.016 | settings |
| 5 | smoke | `drv` | Driver Health, State:, Online: | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 0 / Total failures: 0 / Success ra... | PASS | 0.000 | health |
| 6 | smoke | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | DIAG_ALRT |
| 7 | smoke | `raw` | Raw Registers, Vbus, Temp | === Raw Registers === / Vshunt: 23 (0x000017) / Vbus:   2201 (0x000899) / Temp:   3784 (0x0EC8) / Current:25 (0x000019) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.000 | conversion raw read |
| 8 | functional | `help` | INA228 CLI Help, read, raw | === INA228 CLI Help === / [W] Safety: this example does not make 85 V systems safe. Use qualified design practices, isolation where needed, fusing, creepage/... | PASS | 0.016 | help |
| 9 | functional | `scanina` | INA228 Address Probe, Healthy INA228 devices | > === INA228 Address Probe (0x40-0x4F) === / Note: INA228 probes read DIAG_ALRT for MEMSTAT and can clear CNVRF/latched diagnostic evidence. / 0x40: -- / 0x4... | PASS | 0.000 | INA scan |
| 10 | functional | `mfgid` | Manufacturer ID | > [I] Manufacturer ID: 0x5449 / > [I] | PASS | 0.000 | manufacturer ID |
| 11 | functional | `devid` | Device ID | [I] Device ID: 0x2281 / > [I] | PASS | 0.000 | device ID |
| 12 | functional | `timing` | Conversion ready, Estimated conversion time | > Conversion ready: YES / Estimated conversion time: 3156 us (4 ms) / CURRENT_LSB: 0.000019073 A / > [I] | PASS | 0.000 | timing |
| 13 | functional | `vbus` | Vbus | > [I] Vbus: 0.4279 V / > [I] | PASS | 0.000 | bus voltage |
| 14 | functional | `vshunt` | Vshunt | > [I] Vshunt: 0.0000081 V / > [I] | PASS | 0.000 | shunt voltage |
| 15 | functional | `temp` | Temp | [I] Temp: 29.57 C / > [I] | PASS | 0.000 | die temperature |
| 16 | functional | `current` | Current | [I] Current: 0.000420 A / > [I] | PASS | 0.000 | current |
| 17 | functional | `power` | Power | > [I] Power: 0.000183 W / > [I] | PASS | 0.000 | power |
| 18 | functional | `energy` | Energy | > [I] Energy: 0.000000000 J / > [I] | PASS | 0.000 | energy |
| 19 | functional | `charge` | Charge | > [I] Charge: 2.807865143 C / > [I] | PASS | 0.000 | charge |
| 20 | functional | `read` | Vbus, Power, Accum | > [I] Reading all measurements: / Vbus:    0.4279 V / Vshunt:  0.0000113 V / Temp:    29.56 C / Current: 0.000744 A / Power:   0.000305 W / Energy:  0.000000... | PASS | 0.015 | aggregate read |
| 21 | functional | `diag` | DIAG_ALRT Flags, MEMSTAT | === DIAG_ALRT Flags === / Note: this read is destructive/status-clearing for CNVRF and latched diagnostic evidence. / MEMSTAT:   yes / CNVRF:     yes / ALATC... | PASS | 0.000 | parsed diagnostics |
| 22 | functional | `limits` | Alert Limits | === Alert Limits === / SOVL:      0x7FFF  163.835 mV / SUVL:      0x8000  -163.840 mV / BOVL:      0x7FFF  102.3969 V / BUVL:      0x0000  0.0000 V / TEMP_LI... | PASS | 0.000 | alert limits |
| 23 | functional | `alatch` | Alert latch | > [I] Alert latch: no / > [I] | PASS | 0.016 | alert latch query |
| 24 | functional | `cnvralert` | Conversion-ready alert | [I] Conversion-ready alert: no / > [I] | PASS | 0.000 | conversion alert query |
| 25 | functional | `alslow` | Slow alert | [I] Slow alert: no / > [I] | PASS | 0.000 | slow alert query |
| 26 | functional | `apol` | Alert polarity | [I] Alert polarity: active-low / > [I] | PASS | 0.000 | alert polarity query |
| 27 | functional | `mode` | Mode | > Mode: CONT_ALL (15) / > [I] | PASS | 0.000 | mode query |
| 28 | functional | `convtime` | Conversion times | > Conversion times: VBUS=1052us  VSHUNT=1052us  TEMP=1052us / > [I] | PASS | 0.000 | conversion time query |
| 29 | functional | `averaging` | Averaging | > Averaging: 1 samples / > [I] | PASS | 0.000 | averaging query |
| 30 | functional | `adcrange` | ADC range | > ADC range: +/-163.84mV / > [I] | PASS | 0.000 | ADC range query |
| 31 | functional | `cal` | CURRENT_LSB | > Calibration: Rshunt=0.015000 ohm  MaxCurrent=10.000000 A  CURRENT_LSB=0.000019073 A / > [I] | PASS | 0.015 | calibration query |
| 32 | functional | `tempco` | Shunt temp coeff | > [I] Shunt temp coeff: 0 ppm/degC / > [I] | PASS | 0.000 | temperature coefficient query |
| 33 | functional | `tempcomp` | Temperature compensation | [I] Temperature compensation: no / > [I] | PASS | 0.000 | temperature compensation query |
| 34 | functional | `delay` | Conversion delay | > [I] Conversion delay: 0 x 2 ms (0 ms) / > [I] | PASS | 0.000 | conversion delay query |
| 35 | functional | `ready` | Conversion ready | > [I] Conversion ready: yes / > [I] | PASS | 0.015 | readiness |
| 36 | functional | `reg16 0x3E` | 0x | Reg 0x3E = 0x5449 (21577) / > [I] | PASS | 0.000 | manufacturer register raw16 |
| 37 | functional | `reg16 0x3F` | 0x | Reg 0x3F = 0x2281 (8833) / > [I] | PASS | 0.000 | device register raw16 |
| 38 | functional | `reg24 0x05` | 0x | >   Reg 0x05 = 0x008900 (35072) / > [I] | PASS | 0.000 | VBUS raw24 |
| 39 | functional | `reg40 0x09` | 0x | >   Reg 0x09 = 0x0000000000 / > [I] | PASS | 0.000 | ENERGY raw40 |
| 40 | functional | `rstacc` | reset | > [I] resetAccumulators(): OK / > [I] | PASS | 0.000 | accumulator reset |
| 41 | functional | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.000 | manual recover |
| 42 | functional | `selftest` | INA228 selftest | === INA228 selftest (diagnostic commands; reads DIAG_ALRT) === / Note: DIAG_ALRT reads can clear CNVRF and latched evidence. / [PASS] probe responds / [PASS]... | PASS | 0.000 | self-test |
| 43 | functional | `stress 50` | Stress Summary, Errors: | > === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 85 ms / Rate: 588.24 samples/s / Vbus V:    min=0.4262 avg=0.4287 ... | PASS | 0.078 | short stress |
| 44 | functional | `stress_mix 50` | stress_mix summary, fail= | > === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 24 ms / Rate: 2083.33 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curr... | PASS | 0.031 | short mixed stress |
| 45 | exhaustive | `integer` | Integer Sample, Bus:, Current:, Power: | === Integer Sample === / Bus:     427 mV / Shunt:   7 uV / Temp:    29563 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | fixed-unit integer sample |
| 46 | exhaustive | `diagsnap` | DIAG_ALRT Snapshot, cache-only | > === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   2328353 ms / MEMSTAT: ... | PASS | 0.000 | cache-only diagnostic snapshot |
| 47 | exhaustive | `ready_step 0` | INVALID_PARAM | [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | readiness zero-budget rejection |
| 48 | exhaustive | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | readiness single-instruction poll |
| 49 | exhaustive | `sample_step 0` | INVALID_PARAM | [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | power sample zero-budget rejection |
| 50 | exhaustive | `sample_step 1` | readPowerSampleRawStep, IN_PROGRESS | [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | power sample budget one |
| 51 | exhaustive | `sample_step 2` | readPowerSampleRawStep | [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | power sample budget two |
| 52 | exhaustive | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   9 uV / Temp:    29563 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | power sample full budget |
| 53 | exhaustive | `apply_start` | startApplyCalibration, IN_PROGRESS | [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | calibration job start |
| 54 | exhaustive | `apply_step 0` | INVALID_PARAM | > [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | calibration job zero-budget rejection |
| 55 | exhaustive | `apply_step 1` | pollApplyCalibration, IN_PROGRESS | > [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.015 | calibration job budget one |
| 56 | exhaustive | `apply_step 6` | pollApplyCalibration, OK | [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.000 | calibration job full budget |
| 57 | exhaustive | `reset_start` | startResetJob, IN_PROGRESS | > [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.000 | reset job start |
| 58 | exhaustive | `reset_step 0` | INVALID_PARAM | > [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | reset job zero-budget rejection |
| 59 | exhaustive | `reset_step 1` | pollResetJob, IN_PROGRESS | [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | reset job budget one |
| 60 | exhaustive | `reset_step 16` | pollResetJob, OK | [I] pollResetJob(16): OK / > [I] | PASS | 0.000 | reset job completion budget |
| 61 | exhaustive | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | restore continuous-all mode after reset job |
| 62 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4330 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 63 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4330 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 64 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4311 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 65 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4326 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 66 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4336 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 67 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4322 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 68 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4320 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 69 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4309 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 70 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4313 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 71 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4301 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 72 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4307 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 73 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4322 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 74 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4303 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 75 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4307 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 76 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4307 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 77 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4303 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 78 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4309 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 79 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4293 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 80 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4301 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 81 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4311 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 82 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4291 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 83 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4313 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 84 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4293 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 85 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4291 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 86 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4297 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 87 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4295 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 88 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4291 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 89 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4307 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 90 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4287 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 91 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4297 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 92 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4291 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 93 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4309 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 94 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4301 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 95 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4307 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 96 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4297 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 97 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4301 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 98 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4279 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 99 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4279 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 100 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4289 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 101 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4299 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 102 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4307 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 103 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4285 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 104 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4289 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 105 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4279 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 106 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4295 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 107 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4277 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 108 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4277 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 109 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4297 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 110 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4297 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 111 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4279 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 112 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4283 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 113 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4301 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 114 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4283 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 115 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4287 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 116 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4295 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 117 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4301 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 118 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4285 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 119 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4307 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 120 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4301 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 121 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4285 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 122 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4297 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 123 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4283 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 124 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4291 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 125 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4279 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 126 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4297 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 127 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4295 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 128 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4260 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 129 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4291 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 130 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4281 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 131 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4295 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 132 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4283 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 133 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4285 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 134 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4279 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 135 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4291 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 136 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4305 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 137 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4287 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 138 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4271 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 139 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4271 V / > [I] | PASS | 0.015 | benchmark bus voltage |
| 140 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4275 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 141 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4283 V / > [I] | PASS | 0.015 | benchmark bus voltage |
| 142 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4289 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 143 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4271 V / > [I] | PASS | 0.015 | benchmark bus voltage |
| 144 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4287 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 145 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4285 V / > [I] | PASS | 0.016 | benchmark bus voltage |
| 146 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4291 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 147 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4287 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 148 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4279 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 149 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4301 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 150 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4299 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 151 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4275 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 152 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4309 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 153 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4281 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 154 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4287 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 155 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4299 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 156 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4264 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 157 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4313 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 158 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4281 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 159 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4291 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 160 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4281 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 161 | benchmark | `vbus` | Vbus | > [I] Vbus: 0.4295 V / > [I] | PASS | 0.000 | benchmark bus voltage |
| 162 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000056 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 163 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000062 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 164 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000091 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 165 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000094 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 166 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000078 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 167 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000100 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 168 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000053 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 169 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000069 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 170 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000088 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 171 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000075 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 172 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000056 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 173 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000081 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 174 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000059 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 175 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000084 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 176 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000075 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 177 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000066 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 178 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000072 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 179 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000100 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 180 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000053 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 181 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000041 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 182 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000072 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 183 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000109 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 184 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000041 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 185 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000075 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 186 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000059 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 187 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000075 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 188 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000050 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 189 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000119 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 190 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000062 V / > [I] | PASS | 0.016 | benchmark shunt voltage |
| 191 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000091 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 192 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000072 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 193 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000047 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 194 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000062 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 195 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000091 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 196 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000075 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 197 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000044 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 198 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000091 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 199 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000062 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 200 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000081 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 201 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000056 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 202 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000072 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 203 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000091 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 204 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000062 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 205 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000097 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 206 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000072 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 207 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000094 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 208 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000072 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 209 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000113 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 210 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000100 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 211 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000078 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 212 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000044 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 213 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000081 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 214 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000081 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 215 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000069 V / > [I] | PASS | 0.016 | benchmark shunt voltage |
| 216 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000072 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 217 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000088 V / > [I] | PASS | 0.016 | benchmark shunt voltage |
| 218 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000059 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 219 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000081 V / > [I] | PASS | 0.016 | benchmark shunt voltage |
| 220 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000056 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 221 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000084 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 222 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000075 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 223 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000084 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 224 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000088 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 225 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000072 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 226 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000094 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 227 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000062 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 228 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000103 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 229 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000084 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 230 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000084 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 231 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000109 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 232 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000053 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 233 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000084 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 234 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000091 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 235 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000081 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 236 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000088 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 237 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000066 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 238 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000084 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 239 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000069 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 240 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000069 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 241 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000056 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 242 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000100 V / > [I] | PASS | 0.016 | benchmark shunt voltage |
| 243 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000075 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 244 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000078 V / > [I] | PASS | 0.016 | benchmark shunt voltage |
| 245 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000059 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 246 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000097 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 247 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000041 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 248 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000072 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 249 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000091 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 250 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000041 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 251 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000075 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 252 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000103 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 253 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000078 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 254 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000094 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 255 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000066 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 256 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000119 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 257 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000078 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 258 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000041 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 259 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000075 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 260 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000059 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 261 | benchmark | `vshunt` | Vshunt | > [I] Vshunt: 0.0000066 V / > [I] | PASS | 0.000 | benchmark shunt voltage |
| 262 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 263 | benchmark | `temp` | Temp | > [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 264 | benchmark | `temp` | Temp | [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 265 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 266 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 267 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 268 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 269 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 270 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 271 | benchmark | `temp` | Temp | [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 272 | benchmark | `temp` | Temp | [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 273 | benchmark | `temp` | Temp | > [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 274 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 275 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.016 | benchmark temperature |
| 276 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 277 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.016 | benchmark temperature |
| 278 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 279 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.015 | benchmark temperature |
| 280 | benchmark | `temp` | Temp | > [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 281 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 282 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 283 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 284 | benchmark | `temp` | Temp | > [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 285 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 286 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 287 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 288 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 289 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 290 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 291 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 292 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 293 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 294 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 295 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 296 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 297 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 298 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 299 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 300 | benchmark | `temp` | Temp | > [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 301 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 302 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 303 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 304 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.016 | benchmark temperature |
| 305 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 306 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.015 | benchmark temperature |
| 307 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 308 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 309 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 310 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 311 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 312 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 313 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 314 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 315 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 316 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 317 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 318 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 319 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 320 | benchmark | `temp` | Temp | [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 321 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 322 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 323 | benchmark | `temp` | Temp | > [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 324 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 325 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 326 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 327 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 328 | benchmark | `temp` | Temp | > [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 329 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 330 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 331 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 332 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 333 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 334 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 335 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 336 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 337 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.015 | benchmark temperature |
| 338 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 339 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.016 | benchmark temperature |
| 340 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 341 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.016 | benchmark temperature |
| 342 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 343 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 344 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 345 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.016 | benchmark temperature |
| 346 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 347 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 348 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 349 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 350 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 351 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 352 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 353 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 354 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 355 | benchmark | `temp` | Temp | [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 356 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 357 | benchmark | `temp` | Temp | > [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 358 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 359 | benchmark | `temp` | Temp | > [I] Temp: 29.58 C / > [I] | PASS | 0.000 | benchmark temperature |
| 360 | benchmark | `temp` | Temp | > [I] Temp: 29.57 C / > [I] | PASS | 0.000 | benchmark temperature |
| 361 | benchmark | `temp` | Temp | [I] Temp: 29.59 C / > [I] | PASS | 0.000 | benchmark temperature |
| 362 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 13 (0x00000D) / Vbus:   2185 (0x000889) / Temp:   3786 (0x0ECA) / Current:14 (0x00000E) / Power:  1 (0x000001) / Energy: 0 / ... | PASS | 0.015 | benchmark raw sample |
| 363 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 23 (0x000017) / Vbus:   2189 (0x00088D) / Temp:   3786 (0x0ECA) / Current:25 (0x000019) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 364 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 18 (0x000012) / Vbus:   2196 (0x000894) / Temp:   3786 (0x0ECA) / Current:19 (0x000013) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 365 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 22 (0x000016) / Vbus:   2191 (0x00088F) / Temp:   3786 (0x0ECA) / Current:24 (0x000018) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 366 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 24 (0x000018) / Vbus:   2196 (0x000894) / Temp:   3787 (0x0ECB) / Current:17 (0x000011) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 367 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 25 (0x000019) / Vbus:   2202 (0x00089A) / Temp:   3786 (0x0ECA) / Current:27 (0x00001B) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.016 | benchmark raw sample |
| 368 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 18 (0x000012) / Vbus:   2191 (0x00088F) / Temp:   3785 (0x0EC9) / Current:19 (0x000013) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 369 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 19 (0x000013) / Vbus:   2191 (0x00088F) / Temp:   3787 (0x0ECB) / Current:17 (0x000011) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 370 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 24 (0x000018) / Vbus:   2194 (0x000892) / Temp:   3788 (0x0ECC) / Current:26 (0x00001A) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 371 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 28 (0x00001C) / Vbus:   2187 (0x00088B) / Temp:   3785 (0x0EC9) / Current:30 (0x00001E) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 372 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 20 (0x000014) / Vbus:   2190 (0x00088E) / Temp:   3790 (0x0ECE) / Current:21 (0x000015) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.016 | benchmark raw sample |
| 373 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 20 (0x000014) / Vbus:   2189 (0x00088D) / Temp:   3787 (0x0ECB) / Current:19 (0x000013) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 374 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 29 (0x00001D) / Vbus:   2186 (0x00088A) / Temp:   3787 (0x0ECB) / Current:31 (0x00001F) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 375 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 19 (0x000013) / Vbus:   2201 (0x000899) / Temp:   3787 (0x0ECB) / Current:20 (0x000014) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.016 | benchmark raw sample |
| 376 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 29 (0x00001D) / Vbus:   2189 (0x00088D) / Temp:   3788 (0x0ECC) / Current:31 (0x00001F) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 377 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 28 (0x00001C) / Vbus:   2196 (0x000894) / Temp:   3786 (0x0ECA) / Current:30 (0x00001E) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 378 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 22 (0x000016) / Vbus:   2197 (0x000895) / Temp:   3788 (0x0ECC) / Current:24 (0x000018) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 379 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 15 (0x00000F) / Vbus:   2199 (0x000897) / Temp:   3787 (0x0ECB) / Current:16 (0x000010) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 380 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 21 (0x000015) / Vbus:   2190 (0x00088E) / Temp:   3787 (0x0ECB) / Current:22 (0x000016) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.016 | benchmark raw sample |
| 381 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 32 (0x000020) / Vbus:   2197 (0x000895) / Temp:   3787 (0x0ECB) / Current:34 (0x000022) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 382 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 20 (0x000014) / Vbus:   2191 (0x00088F) / Temp:   3787 (0x0ECB) / Current:21 (0x000015) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 383 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 33 (0x000021) / Vbus:   2203 (0x00089B) / Temp:   3787 (0x0ECB) / Current:36 (0x000024) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 384 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 15 (0x00000F) / Vbus:   2195 (0x000893) / Temp:   3787 (0x0ECB) / Current:16 (0x000010) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 385 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 30 (0x00001E) / Vbus:   2198 (0x000896) / Temp:   3789 (0x0ECD) / Current:32 (0x000020) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 386 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 23 (0x000017) / Vbus:   2194 (0x000892) / Temp:   3786 (0x0ECA) / Current:25 (0x000019) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 387 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 28 (0x00001C) / Vbus:   2189 (0x00088D) / Temp:   3787 (0x0ECB) / Current:30 (0x00001E) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 388 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 11 (0x00000B) / Vbus:   2186 (0x00088A) / Temp:   3787 (0x0ECB) / Current:12 (0x00000C) / Power:  1 (0x000001) / Energy: 0 ... | PASS | 0.016 | benchmark raw sample |
| 389 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 28 (0x00001C) / Vbus:   2198 (0x000896) / Temp:   3788 (0x0ECC) / Current:30 (0x00001E) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 390 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 21 (0x000015) / Vbus:   2203 (0x00089B) / Temp:   3787 (0x0ECB) / Current:22 (0x000016) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 391 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 18 (0x000012) / Vbus:   2195 (0x000893) / Temp:   3788 (0x0ECC) / Current:19 (0x000013) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 392 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 22 (0x000016) / Vbus:   2197 (0x000895) / Temp:   3787 (0x0ECB) / Current:24 (0x000018) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 393 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 26 (0x00001A) / Vbus:   2191 (0x00088F) / Temp:   3786 (0x0ECA) / Current:28 (0x00001C) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.015 | benchmark raw sample |
| 394 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 28 (0x00001C) / Vbus:   2195 (0x000893) / Temp:   3787 (0x0ECB) / Current:30 (0x00001E) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 395 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 15 (0x00000F) / Vbus:   2200 (0x000898) / Temp:   3787 (0x0ECB) / Current:16 (0x000010) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 396 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 34 (0x000022) / Vbus:   2198 (0x000896) / Temp:   3787 (0x0ECB) / Current:37 (0x000025) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 397 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 22 (0x000016) / Vbus:   2199 (0x000897) / Temp:   3787 (0x0ECB) / Current:13 (0x00000D) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 398 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 36 (0x000024) / Vbus:   2188 (0x00088C) / Temp:   3787 (0x0ECB) / Current:39 (0x000027) / Power:  5 (0x000005) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 399 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 19 (0x000013) / Vbus:   2190 (0x00088E) / Temp:   3788 (0x0ECC) / Current:20 (0x000014) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 400 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 27 (0x00001B) / Vbus:   2193 (0x000891) / Temp:   3789 (0x0ECD) / Current:29 (0x00001D) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 401 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 26 (0x00001A) / Vbus:   2198 (0x000896) / Temp:   3787 (0x0ECB) / Current:28 (0x00001C) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.016 | benchmark raw sample |
| 402 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 29 (0x00001D) / Vbus:   2184 (0x000888) / Temp:   3787 (0x0ECB) / Current:19 (0x000013) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 403 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 15 (0x00000F) / Vbus:   2191 (0x00088F) / Temp:   3788 (0x0ECC) / Current:16 (0x000010) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 404 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 20 (0x000014) / Vbus:   2185 (0x000889) / Temp:   3790 (0x0ECE) / Current:21 (0x000015) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 405 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 23 (0x000017) / Vbus:   2199 (0x000897) / Temp:   3790 (0x0ECE) / Current:25 (0x000019) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 406 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 15 (0x00000F) / Vbus:   2191 (0x00088F) / Temp:   3787 (0x0ECB) / Current:16 (0x000010) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.015 | benchmark raw sample |
| 407 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 14 (0x00000E) / Vbus:   2199 (0x000897) / Temp:   3788 (0x0ECC) / Current:21 (0x000015) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 408 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 22 (0x000016) / Vbus:   2192 (0x000890) / Temp:   3787 (0x0ECB) / Current:24 (0x000018) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 409 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 23 (0x000017) / Vbus:   2194 (0x000892) / Temp:   3787 (0x0ECB) / Current:25 (0x000019) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.016 | benchmark raw sample |
| 410 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 25 (0x000019) / Vbus:   2198 (0x000896) / Temp:   3788 (0x0ECC) / Current:27 (0x00001B) / Power:  0 (0x000000) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 411 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 27 (0x00001B) / Vbus:   2197 (0x000895) / Temp:   3787 (0x0ECB) / Current:19 (0x000013) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 412 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 27 (0x00001B) / Vbus:   2196 (0x000894) / Temp:   3786 (0x0ECA) / Current:29 (0x00001D) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 413 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 29 (0x00001D) / Vbus:   2196 (0x000894) / Temp:   3787 (0x0ECB) / Current:31 (0x00001F) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 414 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 28 (0x00001C) / Vbus:   2183 (0x000887) / Temp:   3787 (0x0ECB) / Current:30 (0x00001E) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.015 | benchmark raw sample |
| 415 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 31 (0x00001F) / Vbus:   2197 (0x000895) / Temp:   3787 (0x0ECB) / Current:18 (0x000012) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 416 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 11 (0x00000B) / Vbus:   2196 (0x000894) / Temp:   3787 (0x0ECB) / Current:12 (0x00000C) / Power:  1 (0x000001) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 417 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 34 (0x000022) / Vbus:   2196 (0x000894) / Temp:   3787 (0x0ECB) / Current:37 (0x000025) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 418 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 15 (0x00000F) / Vbus:   2184 (0x000888) / Temp:   3791 (0x0ECF) / Current:16 (0x000010) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 419 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 31 (0x00001F) / Vbus:   2192 (0x000890) / Temp:   3787 (0x0ECB) / Current:22 (0x000016) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 420 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 17 (0x000011) / Vbus:   2204 (0x00089C) / Temp:   3788 (0x0ECC) / Current:18 (0x000012) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 421 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 24 (0x000018) / Vbus:   2197 (0x000895) / Temp:   3791 (0x0ECF) / Current:26 (0x00001A) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 422 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 29 (0x00001D) / Vbus:   2193 (0x000891) / Temp:   3788 (0x0ECC) / Current:31 (0x00001F) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 423 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 21 (0x000015) / Vbus:   2191 (0x00088F) / Temp:   3788 (0x0ECC) / Current:21 (0x000015) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 424 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 31 (0x00001F) / Vbus:   2195 (0x000893) / Temp:   3786 (0x0ECA) / Current:33 (0x000021) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 425 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 35 (0x000023) / Vbus:   2205 (0x00089D) / Temp:   3788 (0x0ECC) / Current:38 (0x000026) / Power:  5 (0x000005) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 426 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 33 (0x000021) / Vbus:   2196 (0x000894) / Temp:   3787 (0x0ECB) / Current:36 (0x000024) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 427 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 27 (0x00001B) / Vbus:   2186 (0x00088A) / Temp:   3787 (0x0ECB) / Current:29 (0x00001D) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.016 | benchmark raw sample |
| 428 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 36 (0x000024) / Vbus:   2187 (0x00088B) / Temp:   3787 (0x0ECB) / Current:39 (0x000027) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 429 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 31 (0x00001F) / Vbus:   2186 (0x00088A) / Temp:   3787 (0x0ECB) / Current:33 (0x000021) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 430 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 29 (0x00001D) / Vbus:   2195 (0x000893) / Temp:   3786 (0x0ECA) / Current:31 (0x00001F) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.016 | benchmark raw sample |
| 431 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 26 (0x00001A) / Vbus:   2194 (0x000892) / Temp:   3786 (0x0ECA) / Current:28 (0x00001C) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 432 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 12 (0x00000C) / Vbus:   2188 (0x00088C) / Temp:   3789 (0x0ECD) / Current:40 (0x000028) / Power:  1 (0x000001) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 433 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 24 (0x000018) / Vbus:   2197 (0x000895) / Temp:   3788 (0x0ECC) / Current:26 (0x00001A) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 434 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 14 (0x00000E) / Vbus:   2195 (0x000893) / Temp:   3788 (0x0ECC) / Current:15 (0x00000F) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 435 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 26 (0x00001A) / Vbus:   2191 (0x00088F) / Temp:   3788 (0x0ECC) / Current:28 (0x00001C) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.015 | benchmark raw sample |
| 436 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 35 (0x000023) / Vbus:   2193 (0x000891) / Temp:   3788 (0x0ECC) / Current:21 (0x000015) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 437 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 24 (0x000018) / Vbus:   2190 (0x00088E) / Temp:   3787 (0x0ECB) / Current:26 (0x00001A) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 438 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 36 (0x000024) / Vbus:   2191 (0x00088F) / Temp:   3788 (0x0ECC) / Current:39 (0x000027) / Power:  5 (0x000005) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 439 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 29 (0x00001D) / Vbus:   2191 (0x00088F) / Temp:   3788 (0x0ECC) / Current:31 (0x00001F) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 440 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 23 (0x000017) / Vbus:   2195 (0x000893) / Temp:   3790 (0x0ECE) / Current:25 (0x000019) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 441 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 28 (0x00001C) / Vbus:   2191 (0x00088F) / Temp:   3788 (0x0ECC) / Current:26 (0x00001A) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 442 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 21 (0x000015) / Vbus:   2200 (0x000898) / Temp:   3791 (0x0ECF) / Current:22 (0x000016) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 443 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 24 (0x000018) / Vbus:   2196 (0x000894) / Temp:   3792 (0x0ED0) / Current:26 (0x00001A) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.015 | benchmark raw sample |
| 444 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 23 (0x000017) / Vbus:   2184 (0x000888) / Temp:   3788 (0x0ECC) / Current:25 (0x000019) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 445 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 33 (0x000021) / Vbus:   2193 (0x000891) / Temp:   3787 (0x0ECB) / Current:28 (0x00001C) / Power:  4 (0x000004) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 446 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 26 (0x00001A) / Vbus:   2189 (0x00088D) / Temp:   3786 (0x0ECA) / Current:28 (0x00001C) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 447 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 26 (0x00001A) / Vbus:   2193 (0x000891) / Temp:   3790 (0x0ECE) / Current:28 (0x00001C) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 448 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 26 (0x00001A) / Vbus:   2192 (0x000890) / Temp:   3790 (0x0ECE) / Current:28 (0x00001C) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.016 | benchmark raw sample |
| 449 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 28 (0x00001C) / Vbus:   2207 (0x00089F) / Temp:   3792 (0x0ED0) / Current:19 (0x000013) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 450 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 26 (0x00001A) / Vbus:   2197 (0x000895) / Temp:   3789 (0x0ECD) / Current:28 (0x00001C) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 451 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 26 (0x00001A) / Vbus:   2202 (0x00089A) / Temp:   3787 (0x0ECB) / Current:28 (0x00001C) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.015 | benchmark raw sample |
| 452 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 22 (0x000016) / Vbus:   2200 (0x000898) / Temp:   3789 (0x0ECD) / Current:24 (0x000018) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 453 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 17 (0x000011) / Vbus:   2195 (0x000893) / Temp:   3788 (0x0ECC) / Current:18 (0x000012) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 454 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 23 (0x000017) / Vbus:   2197 (0x000895) / Temp:   3788 (0x0ECC) / Current:31 (0x00001F) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 455 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 31 (0x00001F) / Vbus:   2198 (0x000896) / Temp:   3790 (0x0ECE) / Current:33 (0x000021) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 456 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 22 (0x000016) / Vbus:   2188 (0x00088C) / Temp:   3787 (0x0ECB) / Current:24 (0x000018) / Power:  3 (0x000003) / Energy: 0 ... | PASS | 0.016 | benchmark raw sample |
| 457 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 7 (0x000007) / Vbus:   2192 (0x000890) / Temp:   3788 (0x0ECC) / Current:7 (0x000007) / Power:  0 (0x000000) / Energy: 0 / Ch... | PASS | 0.000 | benchmark raw sample |
| 458 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 21 (0x000015) / Vbus:   2190 (0x00088E) / Temp:   3787 (0x0ECB) / Current:25 (0x000019) / Power:  2 (0x000002) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 459 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 21 (0x000015) / Vbus:   2196 (0x000894) / Temp:   3788 (0x0ECC) / Current:9 (0x000009) / Power:  2 (0x000002) / Energy: 0 / C... | PASS | 0.000 | benchmark raw sample |
| 460 | benchmark | `raw` | Raw Registers, Vbus | > === Raw Registers === / Vshunt: 20 (0x000014) / Vbus:   2192 (0x000890) / Temp:   3791 (0x0ECF) / Current:21 (0x000015) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | benchmark raw sample |
| 461 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 24 (0x000018) / Vbus:   2204 (0x00089C) / Temp:   3787 (0x0ECB) / Current:26 (0x00001A) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.000 | benchmark raw sample |
| 462 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 463 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     427 mV / Shunt:   9 uV / Temp:    29578 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 464 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   5 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.015 | benchmark integer sample |
| 465 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   9 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 466 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 467 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   9 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 468 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     430 mV / Shunt:   5 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 469 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   5 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.016 | benchmark integer sample |
| 470 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   9 uV / Temp:    29609 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 471 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   8 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 472 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   3 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 473 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 474 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   6 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.015 | benchmark integer sample |
| 475 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     427 mV / Shunt:   9 uV / Temp:    29578 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 476 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     427 mV / Shunt:   8 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 477 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 478 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   5 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 479 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     430 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.016 | benchmark integer sample |
| 480 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 481 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   9 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 482 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 483 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   10 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.000 | benchmark integer sample |
| 484 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     430 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.016 | benchmark integer sample |
| 485 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     427 mV / Shunt:   10 uV / Temp:    29578 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.000 | benchmark integer sample |
| 486 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     427 mV / Shunt:   5 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 487 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   11 uV / Temp:    29578 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | benchmark integer sample |
| 488 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 489 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   6 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.015 | benchmark integer sample |
| 490 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 491 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   4 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 492 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   7 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 493 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   10 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | benchmark integer sample |
| 494 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   6 uV / Temp:    29602 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.016 | benchmark integer sample |
| 495 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   5 uV / Temp:    29625 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 496 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 497 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 498 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     430 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 499 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     430 mV / Shunt:   8 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 500 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 501 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 502 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29617 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 503 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     427 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 504 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     431 mV / Shunt:   6 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.015 | benchmark integer sample |
| 505 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   5 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 506 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     431 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 507 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 508 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   11 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.000 | benchmark integer sample |
| 509 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.016 | benchmark integer sample |
| 510 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   6 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 511 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   11 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | benchmark integer sample |
| 512 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 513 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   4 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 514 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   5 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.015 | benchmark integer sample |
| 515 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   10 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.000 | benchmark integer sample |
| 516 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 517 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29602 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 518 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 519 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     430 mV / Shunt:   9 uV / Temp:    29578 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.016 | benchmark integer sample |
| 520 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 521 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   10 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | benchmark integer sample |
| 522 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   6 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 523 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   9 uV / Temp:    29617 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 524 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   9 uV / Temp:    29617 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.016 | benchmark integer sample |
| 525 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     431 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 526 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 527 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   6 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 528 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   12 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | benchmark integer sample |
| 529 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     430 mV / Shunt:   6 uV / Temp:    29602 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.015 | benchmark integer sample |
| 530 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29617 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 531 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 532 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     427 mV / Shunt:   10 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | benchmark integer sample |
| 533 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 534 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29609 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 535 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 536 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   6 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 537 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     427 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 538 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   6 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 539 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   9 uV / Temp:    29578 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.016 | benchmark integer sample |
| 540 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 541 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   7 uV / Temp:    29625 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 542 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   5 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 543 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 544 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     431 mV / Shunt:   9 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.015 | benchmark integer sample |
| 545 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 546 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   6 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 547 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 548 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29625 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 549 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   5 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.016 | benchmark integer sample |
| 550 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   10 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.000 | benchmark integer sample |
| 551 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 552 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 553 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     429 mV / Shunt:   8 uV / Temp:    29625 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 554 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   10 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | benchmark integer sample |
| 555 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     426 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 556 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   10 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.000 | benchmark integer sample |
| 557 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   10 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | benchmark integer sample |
| 558 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     428 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 559 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     430 mV / Shunt:   6 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 560 | benchmark | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   6 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | benchmark integer sample |
| 561 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   4 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | benchmark integer sample |
| 562 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4285 V / Vshunt:  0.0000084 V / Temp:    29.60 C / Current: 0.000553 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 563 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4279 V / Vshunt:  0.0000088 V / Temp:    29.59 C / Current: 0.000572 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 564 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4268 V / Vshunt:  0.0000078 V / Temp:    29.58 C / Current: 0.000515 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 565 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4270 V / Vshunt:  0.0000066 V / Temp:    29.59 C / Current: 0.000420 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 566 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4299 V / Vshunt:  0.0000075 V / Temp:    29.60 C / Current: 0.000496 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 567 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000100 V / Temp:    29.61 C / Current: 0.000648 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 568 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000072 V / Temp:    29.61 C / Current: 0.000496 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 569 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4291 V / Vshunt:  0.0000116 V / Temp:    29.59 C / Current: 0.000515 A / Power:   0.000305 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 570 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4297 V / Vshunt:  0.0000094 V / Temp:    29.59 C / Current: 0.000610 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 571 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4260 V / Vshunt:  0.0000066 V / Temp:    29.59 C / Current: 0.000420 A / Power:   0.000122 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 572 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4275 V / Vshunt:  0.0000088 V / Temp:    29.59 C / Current: 0.000572 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.015 | benchmark aggregate |
| 573 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000091 V / Temp:    29.60 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 574 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4281 V / Vshunt:  0.0000084 V / Temp:    29.60 C / Current: 0.000744 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 575 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4281 V / Vshunt:  0.0000119 V / Temp:    29.59 C / Current: 0.000782 A / Power:   0.000305 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 576 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000128 V / Temp:    29.59 C / Current: 0.000839 A / Power:   0.000305 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 577 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000081 V / Temp:    29.58 C / Current: 0.000534 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 578 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4283 V / Vshunt:  0.0000094 V / Temp:    29.59 C / Current: 0.000610 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 579 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000091 V / Temp:    29.59 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 580 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4277 V / Vshunt:  0.0000094 V / Temp:    29.62 C / Current: 0.000610 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 581 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4273 V / Vshunt:  0.0000062 V / Temp:    29.59 C / Current: 0.000534 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 582 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4262 V / Vshunt:  0.0000081 V / Temp:    29.59 C / Current: 0.000534 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.015 | benchmark aggregate |
| 583 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4307 V / Vshunt:  0.0000091 V / Temp:    29.59 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 584 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000094 V / Temp:    29.59 C / Current: 0.000610 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 585 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4295 V / Vshunt:  0.0000097 V / Temp:    29.59 C / Current: 0.000629 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 586 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4275 V / Vshunt:  0.0000078 V / Temp:    29.59 C / Current: 0.000515 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 587 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4289 V / Vshunt:  0.0000078 V / Temp:    29.59 C / Current: 0.000496 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 588 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4303 V / Vshunt:  0.0000106 V / Temp:    29.61 C / Current: 0.000515 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 589 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000059 V / Temp:    29.59 C / Current: 0.000381 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 590 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000072 V / Temp:    29.59 C / Current: 0.000477 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 591 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4291 V / Vshunt:  0.0000097 V / Temp:    29.61 C / Current: 0.000629 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 592 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4277 V / Vshunt:  0.0000075 V / Temp:    29.58 C / Current: 0.000496 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.015 | benchmark aggregate |
| 593 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4285 V / Vshunt:  0.0000078 V / Temp:    29.59 C / Current: 0.000515 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 594 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4289 V / Vshunt:  0.0000100 V / Temp:    29.58 C / Current: 0.000343 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 595 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4289 V / Vshunt:  0.0000069 V / Temp:    29.59 C / Current: 0.000458 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 596 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000091 V / Temp:    29.60 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 597 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000097 V / Temp:    29.59 C / Current: 0.000629 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 598 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4277 V / Vshunt:  0.0000059 V / Temp:    29.59 C / Current: 0.000381 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.015 | benchmark aggregate |
| 599 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4275 V / Vshunt:  0.0000081 V / Temp:    29.59 C / Current: 0.000534 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 600 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4303 V / Vshunt:  0.0000050 V / Temp:    29.62 C / Current: 0.000324 A / Power:   0.000122 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 601 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4283 V / Vshunt:  0.0000084 V / Temp:    29.58 C / Current: 0.000401 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 602 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4268 V / Vshunt:  0.0000059 V / Temp:    29.59 C / Current: 0.000381 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 603 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4277 V / Vshunt:  0.0000091 V / Temp:    29.59 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 604 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4301 V / Vshunt:  0.0000097 V / Temp:    29.61 C / Current: 0.000629 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 605 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4305 V / Vshunt:  0.0000088 V / Temp:    29.59 C / Current: 0.000572 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 606 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4275 V / Vshunt:  0.0000109 V / Temp:    29.60 C / Current: 0.000725 A / Power:   0.000305 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 607 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4260 V / Vshunt:  0.0000072 V / Temp:    29.62 C / Current: 0.000477 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 608 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4268 V / Vshunt:  0.0000078 V / Temp:    29.58 C / Current: 0.000591 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.016 | benchmark aggregate |
| 609 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4275 V / Vshunt:  0.0000072 V / Temp:    29.61 C / Current: 0.000477 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 610 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4289 V / Vshunt:  0.0000069 V / Temp:    29.60 C / Current: 0.000458 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 611 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000072 V / Temp:    29.61 C / Current: 0.000477 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 612 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4281 V / Vshunt:  0.0000084 V / Temp:    29.62 C / Current: 0.000553 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 613 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000078 V / Temp:    29.60 C / Current: 0.000420 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 614 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4289 V / Vshunt:  0.0000075 V / Temp:    29.60 C / Current: 0.000496 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 615 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4283 V / Vshunt:  0.0000075 V / Temp:    29.62 C / Current: 0.000496 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.015 | benchmark aggregate |
| 616 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4283 V / Vshunt:  0.0000088 V / Temp:    29.62 C / Current: 0.000572 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 617 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000078 V / Temp:    29.59 C / Current: 0.000515 A / Power:   0.000122 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 618 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4283 V / Vshunt:  0.0000041 V / Temp:    29.63 C / Current: 0.000267 A / Power:   0.000061 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 619 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4275 V / Vshunt:  0.0000069 V / Temp:    29.62 C / Current: 0.000458 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 620 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4281 V / Vshunt:  0.0000044 V / Temp:    29.59 C / Current: 0.000286 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 621 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4285 V / Vshunt:  0.0000072 V / Temp:    29.59 C / Current: 0.000477 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 622 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4279 V / Vshunt:  0.0000069 V / Temp:    29.59 C / Current: 0.000458 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 623 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4279 V / Vshunt:  0.0000069 V / Temp:    29.59 C / Current: 0.000381 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 624 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4299 V / Vshunt:  0.0000081 V / Temp:    29.59 C / Current: 0.000629 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 625 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4266 V / Vshunt:  0.0000066 V / Temp:    29.58 C / Current: 0.000420 A / Power:   0.000122 W / Energy:  0.00000000... | PASS | 0.015 | benchmark aggregate |
| 626 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000088 V / Temp:    29.58 C / Current: 0.000572 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 627 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000059 V / Temp:    29.59 C / Current: 0.000381 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 628 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000078 V / Temp:    29.59 C / Current: 0.000515 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.016 | benchmark aggregate |
| 629 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000088 V / Temp:    29.59 C / Current: 0.000648 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 630 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4289 V / Vshunt:  0.0000081 V / Temp:    29.62 C / Current: 0.000534 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 631 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4275 V / Vshunt:  0.0000053 V / Temp:    29.62 C / Current: 0.000343 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 632 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4264 V / Vshunt:  0.0000053 V / Temp:    29.60 C / Current: 0.000343 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 633 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4301 V / Vshunt:  0.0000091 V / Temp:    29.59 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 634 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4281 V / Vshunt:  0.0000069 V / Temp:    29.60 C / Current: 0.000458 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 635 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4275 V / Vshunt:  0.0000059 V / Temp:    29.62 C / Current: 0.000381 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 636 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4277 V / Vshunt:  0.0000075 V / Temp:    29.62 C / Current: 0.000496 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.016 | benchmark aggregate |
| 637 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4266 V / Vshunt:  0.0000103 V / Temp:    29.62 C / Current: 0.000687 A / Power:   0.000061 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 638 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4303 V / Vshunt:  0.0000091 V / Temp:    29.62 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 639 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4279 V / Vshunt:  0.0000069 V / Temp:    29.62 C / Current: 0.000458 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 640 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4281 V / Vshunt:  0.0000062 V / Temp:    29.58 C / Current: 0.000401 A / Power:   0.000122 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 641 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000088 V / Temp:    29.62 C / Current: 0.000820 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 642 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4285 V / Vshunt:  0.0000084 V / Temp:    29.61 C / Current: 0.000553 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 643 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000088 V / Temp:    29.58 C / Current: 0.000572 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 644 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4291 V / Vshunt:  0.0000069 V / Temp:    29.59 C / Current: 0.000458 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 645 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4295 V / Vshunt:  0.0000100 V / Temp:    29.59 C / Current: 0.000648 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 646 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4289 V / Vshunt:  0.0000091 V / Temp:    29.59 C / Current: 0.000248 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.015 | benchmark aggregate |
| 647 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4275 V / Vshunt:  0.0000081 V / Temp:    29.59 C / Current: 0.000534 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 648 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000066 V / Temp:    29.60 C / Current: 0.000420 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 649 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4301 V / Vshunt:  0.0000056 V / Temp:    29.59 C / Current: 0.000362 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 650 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4293 V / Vshunt:  0.0000100 V / Temp:    29.60 C / Current: 0.000648 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 651 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000116 V / Temp:    29.61 C / Current: 0.000763 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 652 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4291 V / Vshunt:  0.0000059 V / Temp:    29.59 C / Current: 0.000381 A / Power:   0.000122 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 653 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4291 V / Vshunt:  0.0000106 V / Temp:    29.60 C / Current: 0.000706 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.016 | benchmark aggregate |
| 654 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4271 V / Vshunt:  0.0000056 V / Temp:    29.59 C / Current: 0.000362 A / Power:   0.000122 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 655 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000094 V / Temp:    29.58 C / Current: 0.000687 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 656 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000047 V / Temp:    29.59 C / Current: 0.000305 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.015 | benchmark aggregate |
| 657 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4279 V / Vshunt:  0.0000056 V / Temp:    29.59 C / Current: 0.000362 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 658 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4283 V / Vshunt:  0.0000097 V / Temp:    29.60 C / Current: 0.000629 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 659 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4287 V / Vshunt:  0.0000062 V / Temp:    29.59 C / Current: 0.000401 A / Power:   0.000122 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 660 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4285 V / Vshunt:  0.0000094 V / Temp:    29.58 C / Current: 0.000324 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | benchmark aggregate |
| 661 | benchmark | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4279 V / Vshunt:  0.0000053 V / Temp:    29.59 C / Current: 0.000343 A / Power:   0.000122 W / Energy:  0.000000... | PASS | 0.000 | benchmark aggregate |
| 662 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 663 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 664 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 665 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   7 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 666 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   6 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 667 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   9 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 668 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 669 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   7 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 670 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   6 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 671 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   9 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 672 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   8 uV / Temp:    29617 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 673 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   6 uV / Temp:    29617 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 674 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   6 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 675 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.016 | benchmark fixed-step sample |
| 676 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 677 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 678 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   10 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.000 | benchmark fixed-step sample |
| 679 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   10 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.000 | benchmark fixed-step sample |
| 680 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   13 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.016 | benchmark fixed-step sample |
| 681 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   7 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 682 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   9 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 683 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 684 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 685 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   6 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.015 | benchmark fixed-step sample |
| 686 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 687 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     426 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 688 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   7 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 689 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 690 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.016 | benchmark fixed-step sample |
| 691 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 692 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   8 uV / Temp:    29617 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 693 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   12 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | benchmark fixed-step sample |
| 694 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   7 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 695 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   6 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.016 | benchmark fixed-step sample |
| 696 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   7 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 697 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   12 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | benchmark fixed-step sample |
| 698 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   10 uV / Temp:    29578 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.000 | benchmark fixed-step sample |
| 699 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 700 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.015 | benchmark fixed-step sample |
| 701 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   3 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 702 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   7 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 703 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   5 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 704 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   11 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.000 | benchmark fixed-step sample |
| 705 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.016 | benchmark fixed-step sample |
| 706 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   11 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | benchmark fixed-step sample |
| 707 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   10 uV / Temp:    29609 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | benchmark fixed-step sample |
| 708 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 709 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 710 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.015 | benchmark fixed-step sample |
| 711 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   6 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 712 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   6 uV / Temp:    29625 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 713 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   5 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 714 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 715 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   8 uV / Temp:    29578 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.016 | benchmark fixed-step sample |
| 716 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 717 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   7 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 718 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   9 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 719 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   6 uV / Temp:    29617 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 720 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   6 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 721 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   5 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 722 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   10 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 m... | PASS | 0.000 | benchmark fixed-step sample |
| 723 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   7 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 724 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   10 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | benchmark fixed-step sample |
| 725 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   11 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | benchmark fixed-step sample |
| 726 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   9 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 727 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   4 uV / Temp:    29602 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.015 | benchmark fixed-step sample |
| 728 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   10 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.000 | benchmark fixed-step sample |
| 729 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   6 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 730 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 731 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   9 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 732 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   7 uV / Temp:    29578 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.016 | benchmark fixed-step sample |
| 733 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 734 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   4 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 735 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   6 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 736 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   10 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | benchmark fixed-step sample |
| 737 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29617 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.016 | benchmark fixed-step sample |
| 738 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   5 uV / Temp:    29602 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 739 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   8 uV / Temp:    29602 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 740 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   7 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 741 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   10 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | benchmark fixed-step sample |
| 742 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   8 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.015 | benchmark fixed-step sample |
| 743 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 744 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29594 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 745 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   6 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 746 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 747 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   6 uV / Temp:    29586 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.016 | benchmark fixed-step sample |
| 748 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     426 mV / Shunt:   7 uV / Temp:    29609 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 749 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   7 uV / Temp:    29625 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 750 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   6 uV / Temp:    29609 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 751 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 752 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     427 mV / Shunt:   9 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.016 | benchmark fixed-step sample |
| 753 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   9 uV / Temp:    29578 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 754 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   5 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 755 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 756 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 757 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   11 uV / Temp:    29602 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.015 | benchmark fixed-step sample |
| 758 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   10 uV / Temp:    29609 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | benchmark fixed-step sample |
| 759 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29617 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 760 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   8 uV / Temp:    29586 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | benchmark fixed-step sample |
| 761 | benchmark | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   7 uV / Temp:    29594 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | benchmark fixed-step sample |
| 762 | soak | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | soak raw diagnostics |
| 763 | soak | `ready` | Conversion ready | > [I] Conversion ready: yes / > [I] | PASS | 0.000 | soak readiness |
| 764 | soak | `integer` | Integer Sample | > === Integer Sample === / Bus:     431 mV / Shunt:   11 uV / Temp:    29508 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | soak integer sample |
| 765 | soak | `vshunt` | Vshunt | > [I] Vshunt: 0.0000053 V / > [I] | PASS | 0.015 | soak shunt voltage |
| 766 | soak | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.000 | soak recover |
| 767 | soak | `diagsnap` | DIAG_ALRT Snapshot | > / [runner] recovered missing HILMARK HIL18509838590000000 with retry 1 | UNKNOWN | 10.031 | soak diagnostic snapshot |
| 768 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 933339 / Total failures: 0 / Success... | PASS | 0.000 | soak health |
| 769 | soak | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   7 uV / Temp:    29430 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.015 | soak fixed-step sample |
| 770 | soak | `current` | Current | [I] Current: 0.000572 A / > [I] | PASS | 0.000 | soak current |
| 771 | soak | `stress_mix 50` | stress_mix summary, fail= | > === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 24 ms / Rate: 2083.33 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curr... | PASS | 0.031 | soak mixed stress |
| 772 | soak | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | soak raw diagnostics |
| 773 | soak | `ready` | Conversion ready | > [I] Conversion ready: yes / > [I] | PASS | 0.000 | soak readiness |
| 774 | soak | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   6 uV / Temp:    29805 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.016 | soak integer sample |
| 775 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0000100 V / > [I] | PASS | 0.000 | soak shunt voltage |
| 776 | soak | `recover` | Status: OK | [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Co... | PASS | 0.016 | soak recover |
| 777 | soak | `ready` | Conversion ready | > [I] Conv / [runner] recovered missing HILMARK HIL18556711870000000 with retry 1 | UNKNOWN | 10.031 | soak readiness |
| 778 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 2328339 / Total failures: 0 / Succes... | PASS | 0.000 | soak health |
| 779 | soak | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   3 uV / Temp:    29734 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | soak fixed-step sample |
| 780 | soak | `current` | Current | > [I] Current: 0.000362 A / > [I] | PASS | 0.000 | soak current |
| 781 | soak | `current` | Current | > / [runner] recovered missing HILMARK HIL18574352960000000 with retry 1 | UNKNOWN | 10.032 | soak current |
| 782 | soak | `stress_mix 50` | stress_mix summary, fail= | > === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 23 ms / Rate: 2173.91 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curr... | PASS | 0.032 | soak mixed stress |
| 783 | soak | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | soak raw diagnostics |
| 784 | soak | `ready` | Conversion ready | > [I] Conversion ready: yes / > [I] | PASS | 0.000 | soak readiness |
| 785 | soak | `ready` | Conversion ready | [runner] recovered missing HILMARK HIL18587845780000000 with retry 1 | UNKNOWN | 10.047 | soak readiness |
| 786 | soak | `integer` | Integer Sample | === Integer Sample === / Bus:     427 mV / Shunt:   6 uV / Temp:    29734 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | soak integer sample |
| 787 | soak | `vshunt` | Vshunt | > [I] Vshunt: 0.0000066 V / > [I] | PASS | 0.000 | soak shunt voltage |
| 788 | soak | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.015 | soak recover |
| 789 | soak | `vshunt` | Vshunt | [runner] recovered missing HILMARK HIL18603659530000000 with retry 1 | UNKNOWN | 10.031 | soak shunt voltage |
| 790 | soak | `drv` | Driver Health | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 3723339 / Total failures: 0 / Succ... | PASS | 0.000 | soak health |
| 791 | soak | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   4 uV / Temp:    29773 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | soak fixed-step sample |
| 792 | soak | `current` | Current | [I] Current: 0.000477 A / > [I] | PASS | 0.000 | soak current |
| 793 | soak | `stress_mix 50` | stress_mix summary, fail= | > === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 24 ms / Rate: 2083.33 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curr... | PASS | 0.031 | soak mixed stress |
| 794 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | soak raw diagnostics |
| 795 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / [runner] recovered missing HILMARK HIL18633767960000000 with retry 1 | UNKNOWN | 10.000 | soak stress |
| 796 | soak | `ready` | Conversion ready | > [I] Conversion ready: yes / > [I] | PASS | 0.015 | soak readiness |
| 797 | soak | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   3 uV / Temp:    29758 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | soak integer sample |
| 798 | soak | `vshunt` | Vshunt | > [I] Vshunt: 0.0000100 V / > [I] | PASS | 0.000 | soak shunt voltage |
| 799 | soak | `recover` | Status: OK | [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Co... | PASS | 0.000 | soak recover |
| 800 | soak | `power` | Power | [runner] recovered missing HILMARK HIL18655916090000000 with retry 1 | UNKNOWN | 10.047 | soak power |
| 801 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 5118339 / Total failures: 0 / Succes... | PASS | 0.016 | soak health |
| 802 | soak | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     428 mV / Shunt:   9 uV / Temp:    29727 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | soak fixed-step sample |
| 803 | soak | `current` | Current | > [I] Current: 0.000591 A / > [I] | PASS | 0.000 | soak current |
| 804 | soak | `stress_mix 50` | stress_mix summary, fail= | > === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 24 ms / Rate: 2083.33 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curr... | PASS | 0.016 | soak mixed stress |
| 805 | soak | `current` | Current | > / [runner] recovered missing HILMARK HIL18678133120000000 with retry 1 | UNKNOWN | 10.047 | soak current |
| 806 | soak | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | soak raw diagnostics |
| 807 | soak | `ready` | Conversion ready | > [I] Conversion ready: yes / > [I] | PASS | 0.000 | soak readiness |
| 808 | soak | `recover` | Status: OK | [I] Attempting recovery... / [runner] recovered missing HILMARK HIL18689144210000000 with retry 1 | UNKNOWN | 10.047 | soak recover |
| 809 | soak | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   7 uV / Temp:    29641 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.016 | soak integer sample |
| 810 | soak | `vshunt` | Vshunt | > [I] Vshunt: 0.0000088 V / > [I] | PASS | 0.016 | soak shunt voltage |
| 811 | soak | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.000 | soak recover |
| 812 | soak | `drv` | Driver Health | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 6513339 / Total failures: 0 / Succ... | PASS | 0.000 | soak health |
| 813 | soak | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   6 uV / Temp:    29656 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | soak fixed-step sample |
| 814 | soak | `drv` | Driver Health | > / [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18714189210000000 | UNKNOWN | 20.063 | soak health |
| 815 | soak | `diagsnap` | DIAG_ALRT Snapshot | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18714390930000000 | UNKNOWN | 20.063 | soak diagnostic snapshot |
| 816 | soak | `diagraw` | DIAG_ALRT raw | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18714592500000000 | UNKNOWN | 20.031 | soak raw diagnostics |
| 817 | soak | `probe` | Status: OK | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18714793750000000 | UNKNOWN | 20.046 | soak probe |
| 818 | soak | `recover` | Status: OK | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18714995150000000 | UNKNOWN | 20.063 | soak recover |
| 819 | soak | `stress 50` | Stress Summary, Errors: | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18715196870000000 | UNKNOWN | 20.063 | soak stress |
| 820 | soak | `stress_mix 50` | stress_mix summary, fail= | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18715398590000000 | UNKNOWN | 20.062 | soak mixed stress |
| 821 | soak | `vbus` | Vbus | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18715600150000000 | UNKNOWN | 20.078 | soak bus voltage |
| 822 | soak | `vshunt` | Vshunt | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18715801870000000 | UNKNOWN | 20.063 | soak shunt voltage |
| 823 | soak | `temp` | Temp | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18716003590000000 | UNKNOWN | 20.062 | soak temperature |
| 824 | soak | `current` | Current | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18716205150000000 | UNKNOWN | 20.047 | soak current |
| 825 | soak | `power` | Power | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18716406710000000 | UNKNOWN | 20.032 | soak power |
| 826 | soak | `integer` | Integer Sample | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18716607960000000 | UNKNOWN | 20.016 | soak integer sample |
| 827 | soak | `raw` | Raw Registers, Vbus | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18716809210000000 | UNKNOWN | 20.047 | soak raw |
| 828 | soak | `sample_step 5` | Power Sample Step Result | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18717010780000000 | UNKNOWN | 20.047 | soak fixed-step sample |
| 829 | soak | `read` | Vbus, Power | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18717212180000000 | UNKNOWN | 20.063 | soak aggregate |
| 830 | soak | `ready` | Conversion ready | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18717413900000000 | UNKNOWN | 20.016 | soak readiness |
| 831 | soak | `settings` | Active Settings | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18717615150000000 | UNKNOWN | 20.094 | soak settings |
| 832 | soak | `drv` | Driver Health | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18717817030000000 | UNKNOWN | 20.062 | soak health |
| 833 | soak | `diagsnap` | DIAG_ALRT Snapshot | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18718018750000000 | UNKNOWN | 20.031 | soak diagnostic snapshot |
| 834 | soak | `diagraw` | DIAG_ALRT raw | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18718220000000000 | UNKNOWN | 20.046 | soak raw diagnostics |
| 835 | soak | `probe` | Status: OK | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18718421400000000 | UNKNOWN | 20.047 | soak probe |
| 836 | soak | `recover` | Status: OK | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18718622960000000 | UNKNOWN | 20.032 | soak recover |
| 837 | soak | `stress 50` | Stress Summary, Errors: | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18718824370000000 | UNKNOWN | 20.016 | soak stress |
| 838 | soak | `stress_mix 50` | stress_mix summary, fail= | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18719025620000000 | UNKNOWN | 20.031 | soak mixed stress |
| 839 | soak | `vbus` | Vbus | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18719226870000000 | UNKNOWN | 20.047 | soak bus voltage |
| 840 | soak | `vshunt` | Vshunt | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18719428430000000 | UNKNOWN | 20.047 | soak shunt voltage |
| 841 | soak | `temp` | Temp | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18719629840000000 | UNKNOWN | 20.047 | soak temperature |
| 842 | soak | `current` | Current | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18719831250000000 | UNKNOWN | 20.031 | soak current |
| 843 | soak | `power` | Power | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18720032500000000 | UNKNOWN | 20.078 | soak power |
| 844 | soak | `integer` | Integer Sample | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18720234370000000 | UNKNOWN | 20.047 | soak integer sample |
| 845 | soak | `raw` | Raw Registers, Vbus | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18720435780000000 | UNKNOWN | 20.062 | soak raw |
| 846 | soak | `sample_step 5` | Power Sample Step Result | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18720637340000000 | UNKNOWN | 20.078 | soak fixed-step sample |
| 847 | soak | `read` | Vbus, Power | [runner] marker retry 1 failed: / [runner] missing HILMARK HIL18720839060000000 | UNKNOWN | 20.062 | soak aggregate |
| 848 | soak | `ready` | Conversion ready | [runner] recovered missing HILMARK HIL18721040620000000 with retry 1 | UNKNOWN | 10.000 | soak readiness |
| 849 | soak | `read` | Vbus, Power | > [I] Reading all measurements: / [runner] recovered missing HILMARK HIL18723259060000000 with retry 1 | UNKNOWN | 10.015 | soak aggregate |
| 850 | soak | `current` | Current | [I] Current: 0.000496 A / > [I] | PASS | 0.000 | soak current |
| 851 | soak | `vshunt` | Vshunt | > [I] Vs / [runner] recovered missing HILMARK HIL18730318590000000 with retry 1 | UNKNOWN | 10.047 | soak shunt voltage |
| 852 | soak | `stress_mix 50` | stress_mix summary, fail= | > === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 23 ms / Rate: 2173.91 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curr... | PASS | 0.031 | soak mixed stress |
| 853 | soak | `vshunt` | Vshunt | > / [runner] recovered missing HILMARK HIL18732106560000000 with retry 1 | UNKNOWN | 10.031 | soak shunt voltage |
| 854 | soak | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | soak raw diagnostics |
| 855 | soak | `ready` | Conversion ready | > [I] Conversion ready: yes / > [I] | PASS | 0.000 | soak readiness |
| 856 | soak | `integer` | Integer Sample | > === Integer Sample === / Bus:     428 mV / Shunt:   6 uV / Temp:    29742 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | soak integer sample |
| 857 | soak | `vshunt` | Vshunt | > [I] Vshunt: 0.0000081 V / > [I] | PASS | 0.000 | soak shunt voltage |
| 858 | soak | `current` | Current | > / [runner] recovered missing HILMARK HIL18758425150000000 with retry 1 | UNKNOWN | 10.031 | soak current |
| 859 | soak | `recover` | Status: OK | [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Co... | PASS | 0.000 | soak recover |
| 860 | soak | `drv` | Driver Health | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 7908339 / Total failures: 0 / Succ... | PASS | 0.000 | soak health |
| 861 | not-run | `<fixture: disconnected target>` | safe absent-device fixture | requires safe disconnect or switched fixture | NOT RUN | 0.000 | requires safe disconnect or switched fixture |
| 862 | not-run | `<fixture: bus fault injection>` | safe fault-injection fixture | requires safe NACK/timeout/bus-error injection | NOT RUN | 0.000 | requires safe NACK/timeout/bus-error injection |
| 863 | not-run | `<fixture: alert pin capture>` | alert pin instrumentation | requires alert-pin wiring and safe threshold stimulus | NOT RUN | 0.000 | requires alert-pin wiring and safe threshold stimulus |
| 864 | not-run | `<fixture: MCU reset or power cycle>` | controlled reset/power fixture | requires explicit reset/power-cycle control | NOT RUN | 0.000 | requires explicit reset/power-cycle control |

## Soak Summary

- Requested duration: 28800.0 s
- Executed soak commands: 256218
- Recorded soak rows: 99
- Soak PASS row storage stride: every 5000 PASS row(s), plus all FAIL/UNKNOWN rows
- Soak verdict counts: PASS=256170, FAIL=0, UNKNOWN=48
- Soak latency min/mean/max: 0.000 / 0.012 / 20.094 s
- Command mix:
  - `current`: 14235
  - `diagraw`: 14234
  - `diagsnap`: 14234
  - `drv`: 14234
  - `integer`: 14235
  - `power`: 14235
  - `probe`: 14234
  - `raw`: 14234
  - `read`: 14234
  - `ready`: 14234
  - `recover`: 14234
  - `sample_step 5`: 14234
  - `settings`: 14234
  - `stress 50`: 14234
  - `stress_mix 50`: 14234
  - `temp`: 14235
  - `vbus`: 14235
  - `vshunt`: 14235
- Non-PASS soak command counts:
  - `current`: 5
  - `diagraw`: 2
  - `diagsnap`: 3
  - `drv`: 2
  - `integer`: 2
  - `power`: 3
  - `probe`: 2
  - `raw`: 2
  - `read`: 3
  - `ready`: 4
  - `recover`: 3
  - `sample_step 5`: 2
  - `settings`: 1
  - `stress 50`: 3
  - `stress_mix 50`: 2
  - `temp`: 2
  - `vbus`: 2
  - `vshunt`: 5

## Limitations

- Hardware safety and fixture details must be filled in by the operator.
- This runner records serial CLI evidence only; external instruments must be logged separately.
- Staged `maxInstructions` coverage is limited to the example CLI commands; backend transfer counts need external instrumentation.

## Anomalies

- The 8 hour soak completed the requested duration and recorded 256218 soak
  commands. No command returned FAIL.
- The soak recorded 48 UNKNOWN command rows. All UNKNOWN rows were caused by
  missing `HILMARK` command framing tokens or missing expected CLI output before
  the bounded host timeout. The runner either recovered the prompt with the
  bounded marker retry or continued with subsequent commands.
- The largest observed dropout cluster started near `soak#216017` and affected
  consecutive CLI commands until prompt/marker recovery resumed. Later commands,
  including `current`, `diagraw`, `ready`, `integer`, `vshunt`, `recover`, and
  `drv`, returned PASS again.
- The final recorded `drv` command showed `State: READY`, `Consecutive
  failures: 0`, and `Total failures: 0`, so this run did not produce evidence
  of an INA228 I2C transaction failure or persistent driver-health degradation.
- The UNKNOWN rows are therefore classified as HIL serial/CLI communication
  durability failures unless reproduced with independent bus instrumentation or
  firmware-side evidence.

## Audit Findings

### Medium: Serial CLI HIL Framing Is Not Durable Enough For Long Soaks

- Evidence: 48 UNKNOWN rows in the 8 hour run, all involving missing framing
  markers or missing expected CLI output; several bounded marker retries
  recovered the prompt and later driver-health checks passed.
- Current behavior: the HIL runner drives one text command per operation over
  the example serial console. Under sustained long-duration traffic, the host
  can miss command output or marker output even though the firmware remains
  responsive later.
- Risk: HIL results can conflate host/USB/serial console loss with library or
  chip behavior, especially for soak verdicts and timing outliers.
- Simple fix: keep the current text CLI for manual diagnostics, but add a small
  on-device batched soak command that runs bounded command mixes locally and
  emits compact periodic summaries with sequence numbers, health counters,
  elapsed time, and per-command failure counts.
- Required native test: parser test for the summary format and timeout handling.
- Required HIL regression: run the batched soak for at least 8 hours and verify
  monotonic sequence numbers, bounded summary intervals, zero internal driver
  failures, and no host framing UNKNOWN rows.
- Implemented in this pass: added bounded host-side command framing with
  `hilmark`, marker retry, explicit UNKNOWN classification, transcript capture,
  and full-duration continuation. The batched on-device soak is left as a
  follow-up because it changes the example command surface more substantially.

### Low: Fixture Does Not Cover Fault Injection Or External Timing Evidence

- Evidence: four required fixture-dependent rows were marked NOT RUN:
  disconnected target, bus fault injection, alert pin capture, and MCU
  reset/power-cycle control.
- Current behavior: the run validates the connected benign fixture at address
  `0x41`, but cannot prove timeout/NACK/bus-error classification, alert-pin
  electrical behavior, power-cycle behavior, or backend transfer counts.
- Risk: the library can pass this HIL run while still having untested behavior
  under fault injection or external alert/timing conditions.
- Simple fix: add a switched I2C fault fixture, alert-pin capture, and
  controlled MCU/chip reset or power control. Keep those tests opt-in and
  bounded.
- Required native test: no new native test required for the fixture itself;
  keep parser/report tests for NOT RUN classification.
- Required HIL regression: rerun the fault-injection and reset subsets with the
  fixture connected and record exact wiring and limits.

### Low: Local ESP-IDF Build Was Not Reproduced On This Machine

- Evidence: local `idf.py` was not available. The repository CI workflow has
  ESP-IDF example build jobs, but this run could not execute them locally.
- Current behavior: PlatformIO Arduino builds and native tests passed locally;
  ESP-IDF example verification depends on CI or a local ESP-IDF install.
- Risk: ESP-IDF-specific regressions may be caught only by remote CI.
- Simple fix: install the project-supported ESP-IDF toolchain locally or rely
  on the GitHub Actions ESP-IDF matrix and link the CI run in future reports.
- Required native test: none.
- Required HIL regression: not applicable unless the HIL firmware is built from
  the ESP-IDF example.

## Fixes Implemented During This Run

- Added example CLI commands for fixed-unit integer samples, DIAG_ALRT snapshot
  inspection, fixed-step readiness/sample/reset/calibration jobs, and `hilmark`
  command framing.
- Extended the Python HIL runner with bounded command timeouts, boot capture,
  transcript capture, dry-run/parser self-test modes, benchmark mode,
  full-duration soak support, pass/fail/unknown/not-run classification,
  per-step elapsed times, marker retry, and Markdown report generation.
- Kept all HIL additions outside the core library transport model; the core
  remains callback-only and framework-neutral.

## Local Verification And CI Notes

- `python -m py_compile tools\run_i2c_hil.py`: PASS after the final soak.
- `python tools\run_i2c_hil.py --parser-self-test`: PASS after the final soak.
- `python tools\check_cli_contract.py`: PASS.
- `python tools\check_idf_example_contract.py`: PASS.
- `python tools\check_core_timing_guard.py`: PASS.
- `python scripts\generate_version.py check`: PASS.
- `pio test -e native`: PASS, 132 tests.
- `pio run -e esp32s3dev`: PASS.
- `pio run -e esp32s2dev`: PASS.
- `pio pkg pack`: PASS; generated package archive was removed afterward.
- `git diff --check`: PASS, with only CRLF normalization warnings.
- PlatformIO builds emitted the existing warning that
  `examples/common/CliStyle.h` uses an inline variable requiring C++17.
- Local `idf.py`: NOT RUN; command was unavailable on this machine.
- Remote CI: NOT CHECKED; no PR was found for this local branch and `gh` was
  unavailable in this environment. The checked workflow contains PlatformIO
  builds, native tests, CLI contract checks, version/package checks, and
  ESP-IDF example build jobs.
