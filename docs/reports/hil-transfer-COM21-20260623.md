# INA228 HIL Validation Report

- Date/time: 2026-06-23T11:46:30.149155+02:00 to 2026-06-23T11:46:38.094611+02:00
- Elapsed: 7.9 s
- Port: COM21
- Baud: 115200
- Suite: transfer
- Soak requested: 0.0 s
- Operator: Codex
- Board/environment: ESP32S3_COM21 / esp32s3dev_Arduino
- Fixture: INA228_0x41_low_voltage_no_fault_injection
- Safety assumptions: benign_fixture_no_fault_stimulus
- OS: Windows-11-10.0.26200-SP0
- Python: 3.12.10
- HIL command: `tools\run_i2c_hil.py --port COM21 --baud 115200 --suite transfer --timeout-s 12 --idle-s 0.3 --boot-settle-s 1 --boot-capture-s 4 --command-pause-s 0.05 --require-framed --fail-on-unknown --include-not-run --report docs\reports\hil-transfer-COM21-20260623.md --transcript docs\reports\hil-transfer-COM21-20260623.log --operator Codex --board ESP32S3_COM21 --environment esp32s3dev_Arduino --fixture INA228_0x41_low_voltage_no_fault_injection --safety benign_fixture_no_fault_stimulus --notes framed_transfer_count_closure`
- Branch: hardening/ina228-industry-readiness
- Commit: 29228f1122dfd0cb3bc8d82c5b92535a2f042ca8
- Dirty status:

```text
M .github/workflows/ci.yml
 M CHANGELOG.md
 M README.md
 M docs/README.md
 M docs/validation/hardware-validation-procedure.md
 M docs/validation/release-checklist.md
 M docs/validation/validation-status.md
 M examples/01_basic_bringup_cli/main.cpp
 M examples/common/HealthDiag.h
 M examples/common/I2cTransport.h
 M examples/esp_idf/basic/main/Ina228IdfI2cTransport.cpp
 M examples/esp_idf/basic/main/Ina228IdfI2cTransport.h
 M examples/esp_idf/basic/main/main.cpp
 M include/INA228/INA228.h
 M include/INA228/Status.h
 M test/test_basic.cpp
 M tools/check_cli_contract.py
 M tools/check_idf_example_contract.py
 M tools/run_i2c_hil.py
?? docs/prompts/05-production-grade-closure.md
?? docs/reports/hil-targeted-framed-COM21-20260623.md
?? docs/reports/hil-transfer-COM21-20260623.md
?? docs/validation/hardware-evidence.md
```

- Transcript: `docs/reports/hil-transfer-COM21-20260623.log`
- Notes: framed_transfer_count_closure

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 53 | 0 | 0 | 5 |

## Timing Summary

- Commands executed: 58
- Commands recorded in detail: 58
- Soak commands executed: 0
- Soak rows recorded in detail: 0
- Recorded command latency min/mean/max: 0.000 / 0.004 / 0.125 s

- Maximum consecutive FAIL verdicts: 0

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | INA228 library version | === Version Info === / Example firmware build: Jun 23 2026 11:46:12 / INA228 library version: 2.0.0 / INA228 library full: 2.0.0 (29228f1, 2026-06-23 11:46:1... | PASS | 0.000 | version |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / 00:                         -- -- -- -- -- -- -- -- / 10: ... | PASS | 0.125 | scan |
| 3 | smoke | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.016 | probe |
| 4 | smoke | `settings` | Active Settings, State:, Address: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | settings |
| 5 | smoke | `drv` | Driver Health, State:, Online: | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 0 / Total failures: 0 / Success rate... | PASS | 0.015 | health |
| 6 | smoke | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | DIAG_ALRT |
| 7 | smoke | `raw` | Raw Registers, Vbus, Temp | === Raw Registers === / Vshunt: 38 (0x000026) / Vbus:   2210 (0x0008A2) / Temp:   3736 (0x0E98) / Current:41 (0x000029) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.000 | conversion raw read |
| 8 | transfer | `verbose 0` | Verbose mode | [I] Verbose mode: OFF / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | reduce CLI chatter |
| 9 | transfer | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | continuous-all mode for stable reads |
| 10 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | reset transfer counters |
| 11 | transfer | `ready_step 0` | INVALID_PARAM | [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status... | PASS | 0.000 | zero-budget readiness is bus-silent |
| 12 | transfer | `xfer_assert 0 0 0` | XFER_ASSERT PASS | XFER_ASSERT PASS read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | ready_step 0 transfer count |
| 13 | transfer | `trigger 7` | triggerConversion | [I] triggerConversion(7): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | start triggered conversion for readiness poll |
| 14 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | reset transfer counters |
| 15 | transfer | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | single-instruction readiness poll |
| 16 | transfer | `xfer_stats` | XFER_STATS | XFER_STATS read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | ready_step 1 transfer count snapshot |
| 17 | transfer | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore continuous mode |
| 18 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset transfer counters |
| 19 | transfer | `sample_step 0` | INVALID_PARAM | [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status=INVALI... | PASS | 0.000 | zero-budget sample is bus-silent |
| 20 | transfer | `xfer_assert 0 0 0` | XFER_ASSERT PASS | XFER_ASSERT PASS read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | sample_step 0 transfer count |
| 21 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset transfer counters |
| 22 | transfer | `sample_step 1` | readPowerSampleRawStep, IN_PROGRESS | [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | fresh sample job budget one |
| 23 | transfer | `xfer_assert 1 0 1` | XFER_ASSERT PASS | XFER_ASSERT PASS read=1 write=0 total=1 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | sample_step 1 transfer count |
| 24 | transfer | `sample_step 255` | Power Sample Step Result | [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     440 mV / Shunt:   10 uV / Temp:    29180 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | finish partial sample job |
| 25 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset transfer counters |
| 26 | transfer | `sample_step 2` | readPowerSampleRawStep | [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.000 | fresh sample job budget two |
| 27 | transfer | `xfer_assert 2 0 2` | XFER_ASSERT PASS | XFER_ASSERT PASS read=2 write=0 total=2 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | sample_step 2 transfer count |
| 28 | transfer | `sample_step 255` | Power Sample Step Result | [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     441 mV / Shunt:   9 uV / Temp:    29172 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.015 | finish partial sample job |
| 29 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset transfer counters |
| 30 | transfer | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     439 mV / Shunt:   8 uV / Temp:    29172 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | fresh sample job full budget |
| 31 | transfer | `xfer_assert 5 0 5` | XFER_ASSERT PASS | XFER_ASSERT PASS read=5 write=0 total=5 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | sample_step 5 transfer count |
| 32 | transfer | `apply_start` | startConfigReplayJob, IN_PROGRESS | [I] startConfigReplayJob(): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | config replay job start |
| 33 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset transfer counters |
| 34 | transfer | `apply_step 0` | INVALID_PARAM | [I] pollConfigReplayJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status=INVALID_P... | PASS | 0.000 | zero-budget config replay is bus-silent |
| 35 | transfer | `xfer_assert 0 0 0` | XFER_ASSERT PASS | XFER_ASSERT PASS read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | apply_step 0 transfer count |
| 36 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset transfer counters |
| 37 | transfer | `apply_step 1` | pollConfigReplayJob, IN_PROGRESS | [I] pollConfigReplayJob(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | config replay budget one |
| 38 | transfer | `xfer_assert 0 1 1` | XFER_ASSERT PASS | XFER_ASSERT PASS read=0 write=1 total=1 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | apply_step 1 transfer count |
| 39 | transfer | `apply_step 6` | pollConfigReplayJob, OK | [I] pollConfigReplayJob(6): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.000 | finish partial config replay |
| 40 | transfer | `apply_start` | startConfigReplayJob, IN_PROGRESS | [I] startConfigReplayJob(): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | config replay full-budget restart |
| 41 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset transfer counters |
| 42 | transfer | `apply_step 6` | pollConfigReplayJob, OK | [I] pollConfigReplayJob(6): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.000 | config replay full budget |
| 43 | transfer | `xfer_stats` | XFER_STATS | XFER_STATS read=7 write=6 total=13 / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | apply_step 6 transfer count snapshot |
| 44 | transfer | `reset_start` | startResetJob, IN_PROGRESS | [I] startResetJob(): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | reset job start |
| 45 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset transfer counters |
| 46 | transfer | `reset_step 0` | INVALID_PARAM | [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status=INVALID_PARAM fr... | PASS | 0.000 | zero-budget reset is bus-silent |
| 47 | transfer | `xfer_assert 0 0 0` | XFER_ASSERT PASS | XFER_ASSERT PASS read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset_step 0 transfer count |
| 48 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset transfer counters |
| 49 | transfer | `reset_step 1` | pollResetJob, IN_PROGRESS | [I] pollResetJob(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | reset job budget one |
| 50 | transfer | `xfer_stats` | XFER_STATS | XFER_STATS read=11 write=1 total=12 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset_step 1 transfer count snapshot |
| 51 | transfer | `reset_step 16` | pollResetJob, OK | [I] pollResetJob(16): OK / [runner] frame_status=OK frame_elapsed_ms=3 | PASS | 0.000 | finish reset job |
| 52 | transfer | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | restore continuous mode after reset |
| 53 | transfer | `drv` | Driver Health, State: READY | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 118 / Total failures: 0 / Success ra... | PASS | 0.000 | final health |
| 54 | not-run | `<fixture: disconnected target>` | safe absent-device fixture | requires safe disconnect or switched fixture | NOT RUN | 0.000 | requires safe disconnect or switched fixture |
| 55 | not-run | `<fixture: bus fault injection>` | safe fault-injection fixture | requires safe NACK/timeout/bus-error injection | NOT RUN | 0.000 | requires safe NACK/timeout/bus-error injection |
| 56 | not-run | `<fixture: alert pin capture>` | alert pin instrumentation | requires alert-pin wiring and safe threshold stimulus | NOT RUN | 0.000 | requires alert-pin wiring and safe threshold stimulus |
| 57 | not-run | `<fixture: MCU reset or power cycle>` | controlled reset/power fixture | requires explicit reset/power-cycle control | NOT RUN | 0.000 | requires explicit reset/power-cycle control |
| 58 | not-run | `<8-hour soak>` | --soak-hours 8 | soak not requested for this run | NOT RUN | 0.000 | soak not requested for this run |

## Limitations

- Hardware safety and fixture details must be filled in by the operator.
- This runner records serial CLI evidence only; external instruments must be logged separately.
- Staged `maxInstructions` coverage is limited to the example CLI commands. The `transfer` suite records example callback counts, not logic-analyzer bus bytes. Example `tick()` calls between serial commands can add readiness reads; exact assertions are kept to deterministic paths and other paths record snapshots.
- Soak test was not requested in this run.
