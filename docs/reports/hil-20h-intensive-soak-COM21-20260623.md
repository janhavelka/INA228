# INA228 HIL Validation Report

- Date/time: 2026-06-23T14:59:22.894567+02:00 to 2026-06-23T15:13:58.947558+02:00
- Elapsed: 876.1 s
- Port: COM21
- Baud: 115200
- Suite: targeted
- Soak requested: 72000.0 s
- Operator: Codex
- Board/environment: ESP32S3_COM21 / esp32s3dev_Arduino
- Fixture: INA228_0x41_low_voltage_no_fault_injection
- Safety assumptions: benign_fixture_no_fault_stimulus_unattended_low_voltage
- OS: Windows-11-10.0.26200-SP0
- Python: 3.12.10
- HIL command: `tools\run_i2c_hil.py --port COM21 --baud 115200 --suite targeted --timeout-s 12 --idle-s 0.3 --boot-settle-s 1 --boot-capture-s 4 --command-pause-s 0.05 --post-frame-drain-s 0.05 --empty-retries 2 --require-framed --fail-on-unknown --stop-on-non-pass --include-not-run --soak-hours 20 --soak-store-every 5000 --soak-progress-every 5000 --benchmark-count 50 --report docs\reports\hil-20h-intensive-soak-COM21-20260623.md --transcript docs\reports\hil-20h-intensive-soak-COM21-20260623.log --operator Codex --board ESP32S3_COM21 --environment esp32s3dev_Arduino --fixture INA228_0x41_low_voltage_no_fault_injection --safety benign_fixture_no_fault_stimulus_unattended_low_voltage --notes 20h_targeted_intensive_soak_framed_hil_post_frame_drain_empty_response_retries_cli_96_bytes`
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
?? docs/reports/hil-post-shakedown-failure-smoke-COM21-20260623.md
?? docs/reports/hil-post-soak-failure-smoke-COM21-20260623.md
?? docs/reports/hil-post-soak-failure2-smoke-COM21-20260623.md
?? docs/reports/hil-recover-contract-check-COM21-20260623.md
```

- Transcript: `docs/reports/hil-20h-intensive-soak-COM21-20260623.log`
- Notes: 20h_targeted_intensive_soak_framed_hil_post_frame_drain_empty_response_retries_cli_96_bytes

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 7851 | 1 | 0 | 4 |

## Timing Summary

- Commands executed: 7856
- Commands recorded in detail: 552
- Soak commands executed: 7306
- Soak rows recorded in detail: 2
- Recorded command latency min/mean/max: 0.062 / 0.085 / 12.156 s

- Maximum consecutive FAIL verdicts: 1

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | INA228 library version | === Version Info === / Example firmware build: Jun 23 2026 14:04:48 / INA228 library version: 2.0.0 / INA228 library full: 2.0.0 (851ac4c, 2026-06-23 14:04:4... | PASS | 0.063 | version |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / 00:                         -- -- -- -- -- -- -- -- / 10: ... | PASS | 0.187 | scan |
| 3 | smoke | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.063 | probe |
| 4 | smoke | `settings` | Active Settings, State:, Address: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.062 | settings |
| 5 | smoke | `drv` | Driver Health, State:, Online: | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 1 / Total failures: 0 / Success rate... | PASS | 0.062 | health |
| 6 | smoke | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | DIAG_ALRT |
| 7 | smoke | `raw` | Raw Registers, Vbus, Temp | === Raw Registers === / Vshunt: 33 (0x000021) / Vbus:   2207 (0x00089F) / Temp:   3624 (0x0E28) / Current:36 (0x000024) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.062 | conversion raw read |
| 8 | targeted | `verbose 0` | Verbose mode | [I] Verbose mode: OFF / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | reduce CLI chatter |
| 9 | targeted | `help` | mode [0..15], sample_step <budget>, limits | === INA228 CLI Help === / [W] Safety: this example does not make 85 V systems safe. Use qualified design practices, isolation where needed, fusing, creepage/... | PASS | 0.079 | targeted CLI surface check |
| 10 | targeted | `drv` | Driver Health, State:, Online: | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 10 / Total failures: 0 / Success rat... | PASS | 0.063 | initial health before mutation |
| 11 | targeted | `settings` | Active Settings, Mode:, ADC range: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.062 | initial settings snapshot |
| 12 | targeted | `mode 0` | setMode | [I] setMode(0 = SHUTDOWN): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set operating mode 0 |
| 13 | targeted | `mode 1` | setMode | [I] setMode(1 = TRIG_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame_elap... | PASS | 0.062 | set operating mode 1 |
| 14 | targeted | `mode 2` | setMode | [I] setMode(2 = TRIG_SHUNT): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame_el... | PASS | 0.062 | set operating mode 2 |
| 15 | targeted | `mode 3` | setMode | [I] setMode(3 = TRIG_SHUNT_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS fram... | PASS | 0.063 | set operating mode 3 |
| 16 | targeted | `mode 4` | setMode | [I] setMode(4 = TRIG_TEMP): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame_ela... | PASS | 0.062 | set operating mode 4 |
| 17 | targeted | `mode 5` | setMode | [I] setMode(5 = TRIG_TEMP_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame... | PASS | 0.063 | set operating mode 5 |
| 18 | targeted | `mode 6` | setMode | [I] setMode(6 = TRIG_TEMP_SHUNT): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS fra... | PASS | 0.063 | set operating mode 6 |
| 19 | targeted | `mode 7` | setMode | [I] setMode(7 = TRIG_ALL): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame_elap... | PASS | 0.062 | set operating mode 7 |
| 20 | targeted | `mode 8` | setMode | [I] setMode(8 = SHUTDOWN2): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set operating mode 8 |
| 21 | targeted | `mode 9` | setMode | [I] setMode(9 = CONT_BUS): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set operating mode 9 |
| 22 | targeted | `mode 10` | setMode | [I] setMode(10 = CONT_SHUNT): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set operating mode 10 |
| 23 | targeted | `mode 11` | setMode | [I] setMode(11 = CONT_SHUNT_BUS): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set operating mode 11 |
| 24 | targeted | `mode 12` | setMode | [I] setMode(12 = CONT_TEMP): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | set operating mode 12 |
| 25 | targeted | `mode 13` | setMode | [I] setMode(13 = CONT_TEMP_BUS): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set operating mode 13 |
| 26 | targeted | `mode 14` | setMode | [I] setMode(14 = CONT_TEMP_SHUNT): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set operating mode 14 |
| 27 | targeted | `mode 15` | setMode | [I] setMode(15 = CONT_ALL): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | set operating mode 15 |
| 28 | targeted | `mode` | Mode: | Mode: CONT_ALL (15) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | query mode after full mode sweep |
| 29 | targeted | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | restore continuous-all mode |
| 30 | targeted | `convtime vbus 0` | setConvTime | [I] setConvTime(vbus, 50us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set vbus conversion time index 0 |
| 31 | targeted | `convtime vbus 1` | setConvTime | [I] setConvTime(vbus, 84us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | set vbus conversion time index 1 |
| 32 | targeted | `convtime vbus 2` | setConvTime | [I] setConvTime(vbus, 150us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set vbus conversion time index 2 |
| 33 | targeted | `convtime vbus 3` | setConvTime | [I] setConvTime(vbus, 280us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set vbus conversion time index 3 |
| 34 | targeted | `convtime vbus 4` | setConvTime | [I] setConvTime(vbus, 540us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | set vbus conversion time index 4 |
| 35 | targeted | `convtime vbus 5` | setConvTime | [I] setConvTime(vbus, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set vbus conversion time index 5 |
| 36 | targeted | `convtime vbus 6` | setConvTime | [I] setConvTime(vbus, 2074us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set vbus conversion time index 6 |
| 37 | targeted | `convtime vbus 7` | setConvTime | [I] setConvTime(vbus, 4120us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | set vbus conversion time index 7 |
| 38 | targeted | `convtime vsh 0` | setConvTime | [I] setConvTime(vsh, 50us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | set vsh conversion time index 0 |
| 39 | targeted | `convtime vsh 1` | setConvTime | [I] setConvTime(vsh, 84us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | set vsh conversion time index 1 |
| 40 | targeted | `convtime vsh 2` | setConvTime | [I] setConvTime(vsh, 150us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | set vsh conversion time index 2 |
| 41 | targeted | `convtime vsh 3` | setConvTime | [I] setConvTime(vsh, 280us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | set vsh conversion time index 3 |
| 42 | targeted | `convtime vsh 4` | setConvTime | [I] setConvTime(vsh, 540us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set vsh conversion time index 4 |
| 43 | targeted | `convtime vsh 5` | setConvTime | [I] setConvTime(vsh, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | set vsh conversion time index 5 |
| 44 | targeted | `convtime vsh 6` | setConvTime | [I] setConvTime(vsh, 2074us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set vsh conversion time index 6 |
| 45 | targeted | `convtime vsh 7` | setConvTime | [I] setConvTime(vsh, 4120us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set vsh conversion time index 7 |
| 46 | targeted | `convtime temp 0` | setConvTime | [I] setConvTime(temp, 50us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set temp conversion time index 0 |
| 47 | targeted | `convtime temp 1` | setConvTime | [I] setConvTime(temp, 84us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | set temp conversion time index 1 |
| 48 | targeted | `convtime temp 2` | setConvTime | [I] setConvTime(temp, 150us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | set temp conversion time index 2 |
| 49 | targeted | `convtime temp 3` | setConvTime | [I] setConvTime(temp, 280us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set temp conversion time index 3 |
| 50 | targeted | `convtime temp 4` | setConvTime | [I] setConvTime(temp, 540us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | set temp conversion time index 4 |
| 51 | targeted | `convtime temp 5` | setConvTime | [I] setConvTime(temp, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set temp conversion time index 5 |
| 52 | targeted | `convtime temp 6` | setConvTime | [I] setConvTime(temp, 2074us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set temp conversion time index 6 |
| 53 | targeted | `convtime temp 7` | setConvTime | [I] setConvTime(temp, 4120us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set temp conversion time index 7 |
| 54 | targeted | `convtime` | Conversion times | Conversion times: VBUS=4120us  VSHUNT=4120us  TEMP=4120us / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | query conversion times |
| 55 | targeted | `convtime vbus 5` | setConvTime, OK | [I] setConvTime(vbus, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | restore VBUS conversion time |
| 56 | targeted | `convtime vsh 5` | setConvTime, OK | [I] setConvTime(vsh, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | restore VSHUNT conversion time |
| 57 | targeted | `convtime temp 5` | setConvTime, OK | [I] setConvTime(temp, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | restore TEMP conversion time |
| 58 | targeted | `averaging 0` | setAveraging | [I] setAveraging(1): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | set averaging index 0 |
| 59 | targeted | `averaging 1` | setAveraging | [I] setAveraging(4): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | set averaging index 1 |
| 60 | targeted | `averaging 2` | setAveraging | [I] setAveraging(16): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set averaging index 2 |
| 61 | targeted | `averaging 3` | setAveraging | [I] setAveraging(64): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set averaging index 3 |
| 62 | targeted | `averaging 4` | setAveraging | [I] setAveraging(128): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set averaging index 4 |
| 63 | targeted | `averaging 5` | setAveraging | [I] setAveraging(256): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | set averaging index 5 |
| 64 | targeted | `averaging 6` | setAveraging | [I] setAveraging(512): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set averaging index 6 |
| 65 | targeted | `averaging 7` | setAveraging | [I] setAveraging(1024): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set averaging index 7 |
| 66 | targeted | `averaging` | Averaging: | Averaging: 1024 samples / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | query averaging |
| 67 | targeted | `averaging 0` | setAveraging, OK | [I] setAveraging(1): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | restore averaging 1 |
| 68 | targeted | `adcrange 1` | setAdcRange, OK | [I] setAdcRange(+/-40.96mV): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | switch low shunt range |
| 69 | targeted | `integer` | Integer Sample, Shunt: | === Integer Sample === / Bus:     437 mV / Shunt:   9 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.062 | integer sample after low range |
| 70 | targeted | `adcrange 0` | setAdcRange, OK | [I] setAdcRange(+/-163.84mV): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | restore default shunt range |
| 71 | targeted | `adcrange` | ADC range: | ADC range: +/-163.84mV / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | query ADC range |
| 72 | targeted | `delay 0` | setConversionDelay, OK | [I] setConversionDelay(0): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | conversion delay min |
| 73 | targeted | `delay 1` | setConversionDelay, OK | [I] setConversionDelay(1): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | conversion delay small |
| 74 | targeted | `delay 127` | setConversionDelay, OK | [I] setConversionDelay(127): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | conversion delay middle |
| 75 | targeted | `delay 255` | setConversionDelay, OK | [I] setConversionDelay(255): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | conversion delay max |
| 76 | targeted | `delay` | Conversion delay: | [I] Conversion delay: 255 x 2 ms (510 ms) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | query conversion delay |
| 77 | targeted | `delay 0` | setConversionDelay, OK | [I] setConversionDelay(0): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | restore zero conversion delay |
| 78 | targeted | `tempco 1` | setShuntTempCoeff, OK | [I] setShuntTempCoeff(1): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set small tempco |
| 79 | targeted | `tempco 16383` | setShuntTempCoeff, OK | [I] setShuntTempCoeff(16383): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | set max tempco |
| 80 | targeted | `tempco` | Shunt temp coeff: | [I] Shunt temp coeff: 16383 ppm/degC / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | query tempco |
| 81 | targeted | `tempco 0` | setShuntTempCoeff, OK | [I] setShuntTempCoeff(0): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | restore tempco |
| 82 | targeted | `tempcomp 1` | setTempCompensation, OK | [I] setTempCompensation(yes): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | enable temp compensation |
| 83 | targeted | `tempcomp` | Temperature compensation: | [I] Temperature compensation: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | query temp compensation |
| 84 | targeted | `tempcomp 0` | setTempCompensation, OK | [I] setTempCompensation(no): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | restore temp compensation |
| 85 | targeted | `cal 0.015 10` | setCalibration, OK | [I] setCalibration(0.015000, 10.000000): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | reapply nominal calibration |
| 86 | targeted | `cal` | Calibration:, CURRENT_LSB | Calibration: Rshunt=0.015000 ohm  MaxCurrent=10.000000 A  CURRENT_LSB=0.000019073 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | query calibration |
| 87 | targeted | `alatch 1` | setAlertLatch, OK | [I] setAlertLatch(yes): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | enable alert latch |
| 88 | targeted | `alatch 0` | setAlertLatch, OK | [I] setAlertLatch(no): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | restore alert latch |
| 89 | targeted | `cnvralert 1` | setConversionReadyAlert, OK | [I] setConversionReadyAlert(yes): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | enable conversion-ready alert bit |
| 90 | targeted | `cnvralert 0` | setConversionReadyAlert, OK | [I] setConversionReadyAlert(no): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | restore conversion-ready alert bit |
| 91 | targeted | `alslow 1` | setSlowAlert, OK | [I] setSlowAlert(yes): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | enable slow alert |
| 92 | targeted | `alslow 0` | setSlowAlert, OK | [I] setSlowAlert(no): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | restore slow alert |
| 93 | targeted | `apol 1` | setAlertPolarity, OK | [I] setAlertPolarity(yes): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | set active-high alert |
| 94 | targeted | `apol 0` | setAlertPolarity, OK | [I] setAlertPolarity(no): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | restore active-low alert |
| 95 | targeted | `sovl 0.001` | setShuntOvervoltageThreshold, OK | [I] setShuntOvervoltageThreshold(0.0010000): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | safe shunt overvoltage threshold |
| 96 | targeted | `suvl -0.001` | setShuntUndervoltageThreshold, OK | [I] setShuntUndervoltageThreshold(-0.0010000): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | safe shunt undervoltage threshold |
| 97 | targeted | `bovl 1.0` | setBusOvervoltageThreshold, OK | [I] setBusOvervoltageThreshold(1.0000): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | safe bus overvoltage threshold |
| 98 | targeted | `buvl 0.1` | setBusUndervoltageThreshold, OK | [I] setBusUndervoltageThreshold(0.1000): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | safe bus undervoltage threshold |
| 99 | targeted | `tmplim 100` | setTemperatureOverlimitThreshold, OK | [I] setTemperatureOverlimitThreshold(100.00): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | safe temperature threshold |
| 100 | targeted | `pwrlim 0.01` | setPowerOverlimitThreshold, OK | [I] setPowerOverlimitThreshold(0.010000): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | safe power threshold |
| 101 | targeted | `limits` | Alert Limits | === Alert Limits === / SOVL:      0x00C8  1.000 mV / SUVL:      0xFF38  -1.000 mV / BOVL:      0x0140  1.0000 V / BUVL:      0x0020  0.1000 V / TEMP_LIMIT:0x... | PASS | 0.062 | query alert limits |
| 102 | targeted | `sovl 0.163835` | setShuntOvervoltageThreshold, OK | [I] setShuntOvervoltageThreshold(0.1638350): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | restore shunt overvoltage default |
| 103 | targeted | `suvl -0.16384` | setShuntUndervoltageThreshold, OK | [I] setShuntUndervoltageThreshold(-0.1638400): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | restore shunt undervoltage default |
| 104 | targeted | `bovl 85` | setBusOvervoltageThreshold, OK | [I] setBusOvervoltageThreshold(85.0000): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | restore safe max bus overvoltage |
| 105 | targeted | `buvl 0` | setBusUndervoltageThreshold, OK | [I] setBusUndervoltageThreshold(0.0000): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | restore bus undervoltage default |
| 106 | targeted | `tmplim 255.99` | setTemperatureOverlimitThreshold, OK | [I] setTemperatureOverlimitThreshold(255.99): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | restore safe max temperature threshold |
| 107 | targeted | `pwrlim 800` | setPowerOverlimitThreshold, OK | [I] setPowerOverlimitThreshold(800.000000): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | restore power threshold |
| 108 | targeted | `ready_step 0` | INVALID_PARAM | [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status... | PASS | 0.063 | ready zero-budget rejection |
| 109 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | ready budget one |
| 110 | targeted | `ready_step 2` | pollMeasurementReady | [I] pollMeasurementReady(2): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | ready budget two |
| 111 | targeted | `ready_step 255` | pollMeasurementReady | [I] pollMeasurementReady(255): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | ready max budget |
| 112 | targeted | `sample_step 0` | INVALID_PARAM | [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status=INVALI... | PASS | 0.062 | sample zero-budget rejection |
| 113 | targeted | `sample_step 1` | readPowerSampleRawStep | [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | sample budget one |
| 114 | targeted | `sample_step 2` | readPowerSampleRawStep | [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | sample budget two |
| 115 | targeted | `sample_step 3` | readPowerSampleRawStep | [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     433 mV / Shunt:   12 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | sample budget three |
| 116 | targeted | `sample_step 4` | readPowerSampleRawStep | [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | sample budget four |
| 117 | targeted | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     433 mV / Shunt:   12 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | sample full budget |
| 118 | targeted | `sample_step 255` | Power Sample Step Result | [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   13 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0... | PASS | 0.062 | sample max budget |
| 119 | targeted | `apply_start` | startConfigReplayJob, IN_PROGRESS | [I] startConfigReplayJob(): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | calibration job start |
| 120 | targeted | `apply_step 0` | INVALID_PARAM | [I] pollConfigReplayJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status=INVALID_P... | PASS | 0.062 | calibration zero-budget rejection |
| 121 | targeted | `apply_step 1` | pollConfigReplayJob | [I] pollConfigReplayJob(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | calibration budget one |
| 122 | targeted | `apply_step 2` | pollConfigReplayJob | [I] pollConfigReplayJob(2): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | calibration budget two |
| 123 | targeted | `apply_step 3` | pollConfigReplayJob | [I] pollConfigReplayJob(3): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | calibration budget three |
| 124 | targeted | `apply_step 6` | BUSY | [I] pollConfigReplayJob(6): BUSY / Status: BUSY (code=11, detail=0) / Message: No apply calibration job active / [runner] frame_status=BUSY frame_elapsed_ms=... | PASS | 0.063 | post-completion calibration poll reports BUSY |
| 125 | targeted | `apply_start` | startConfigReplayJob, IN_PROGRESS | [I] startConfigReplayJob(): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | calibration full-budget restart |
| 126 | targeted | `apply_step 6` | pollConfigReplayJob, OK | [I] pollConfigReplayJob(6): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | calibration full-budget completion |
| 127 | targeted | `reset_start` | startResetJob, IN_PROGRESS | [I] startResetJob(): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | reset job start |
| 128 | targeted | `reset_step 0` | INVALID_PARAM | [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status=INVALID_PARAM fr... | PASS | 0.062 | reset zero-budget rejection |
| 129 | targeted | `reset_step 1` | pollResetJob, IN_PROGRESS | [I] pollResetJob(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | reset budget one |
| 130 | targeted | `reset_step 1` | pollResetJob | [I] pollResetJob(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | reset budget one repeated |
| 131 | targeted | `reset_step 2` | pollResetJob | [I] pollResetJob(2): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | reset budget two |
| 132 | targeted | `reset_step 16` | pollResetJob, OK | [I] pollResetJob(16): OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | reset completion budget |
| 133 | targeted | `trigger 1` | triggerConversion | [I] triggerConversion(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | trigger mode 1 |
| 134 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | ready poll after trigger 1 |
| 135 | targeted | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     440 mV / Shunt:   9 uV / Temp:    28227 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.063 | sample after trigger 1 |
| 136 | targeted | `trigger 2` | triggerConversion | [I] triggerConversion(2): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | trigger mode 2 |
| 137 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | ready poll after trigger 2 |
| 138 | targeted | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     440 mV / Shunt:   11 uV / Temp:    28227 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | sample after trigger 2 |
| 139 | targeted | `trigger 3` | triggerConversion | [I] triggerConversion(3): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | trigger mode 3 |
| 140 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | ready poll after trigger 3 |
| 141 | targeted | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     468 mV / Shunt:   11 uV / Temp:    28227 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | sample after trigger 3 |
| 142 | targeted | `trigger 4` | triggerConversion | [I] triggerConversion(4): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | trigger mode 4 |
| 143 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | ready poll after trigger 4 |
| 144 | targeted | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     468 mV / Shunt:   11 uV / Temp:    28094 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | sample after trigger 4 |
| 145 | targeted | `trigger 5` | triggerConversion | [I] triggerConversion(5): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | trigger mode 5 |
| 146 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | ready poll after trigger 5 |
| 147 | targeted | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     490 mV / Shunt:   11 uV / Temp:    28078 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | sample after trigger 5 |
| 148 | targeted | `trigger 6` | triggerConversion | [I] triggerConversion(6): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | trigger mode 6 |
| 149 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | ready poll after trigger 6 |
| 150 | targeted | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     490 mV / Shunt:   12 uV / Temp:    28063 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | sample after trigger 6 |
| 151 | targeted | `trigger 7` | triggerConversion | [I] triggerConversion(7): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | trigger mode 7 |
| 152 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | ready poll after trigger 7 |
| 153 | targeted | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     505 mV / Shunt:   13 uV / Temp:    28047 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | sample after trigger 7 |
| 154 | targeted | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | restore continuous mode after triggers |
| 155 | targeted | `rstacc` | resetAccumulators, OK | [I] resetAccumulators(): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | reset accumulators |
| 156 | targeted | `energy` | Energy | [I] Energy: 0.000000000 J / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | read energy after accumulator reset |
| 157 | targeted | `charge` | Charge | [I] Charge: 0.000152588 C / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | read charge after accumulator reset |
| 158 | targeted | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | raw diagnostics destructive read |
| 159 | targeted | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   238591 ms / MEMSTAT:    ... | PASS | 0.063 | cache-only diagnostic snapshot |
| 160 | targeted | `reg16 0x00` | 0x | Reg 0x00 = 0x0000 (0) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | read CONFIG raw16 |
| 161 | targeted | `reg16 0x01` | 0x | Reg 0x01 = 0xFB68 (64360) / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | read ADC_CONFIG raw16 |
| 162 | targeted | `reg16 0x02` | 0x | Reg 0x02 = 0x0EA6 (3750) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | read SHUNT_CAL raw16 |
| 163 | targeted | `reg24 0x04` | 0x | Reg 0x04 = 0x000190 (400) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | read VSHUNT raw24 |
| 164 | targeted | `reg24 0x05` | 0x | Reg 0x05 = 0x009AF0 (39664) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | read VBUS raw24 |
| 165 | targeted | `reg24 0x06` | 0x | Reg 0x06 = 0x0E18FF (923903) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | read DIETEMP raw24 |
| 166 | targeted | `reg24 0x07` | 0x | Reg 0x07 = 0x0001E0 (480) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | read CURRENT raw24 |
| 167 | targeted | `reg24 0x08` | 0x | Reg 0x08 = 0x000004 (4) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | read POWER raw24 |
| 168 | targeted | `reg40 0x09` | 0x | Reg 0x09 = 0x0000000000 / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | read ENERGY raw40 |
| 169 | targeted | `reg40 0x0A` | 0x | Reg 0x0A = 0x000000003B / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | read CHARGE raw40 |
| 170 | targeted | `mode -1` | Invalid mode | [W] Invalid mode (0-15) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | reject invalid negative mode |
| 171 | targeted | `mode 16` | Invalid mode | [W] Invalid mode (0-15) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | reject invalid high mode |
| 172 | targeted | `trigger 8` | Invalid trigger mode | [W] Invalid trigger mode (0-7 for TRIG_* modes) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | reject invalid trigger mode |
| 173 | targeted | `convtime vbus 8` | Invalid conversion time | [W] Invalid conversion time index (0-7) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | reject invalid conversion index |
| 174 | targeted | `convtime bogus 1` | Invalid target | [W] Invalid target: bogus (use vbus\|vsh\|temp) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | reject invalid conversion target |
| 175 | targeted | `averaging 8` | Invalid averaging | [W] Invalid averaging index (0-7) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | reject invalid averaging |
| 176 | targeted | `adcrange 2` | Invalid ADC range | [W] Invalid ADC range (0 or 1) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | reject invalid ADC range |
| 177 | targeted | `delay 256` | Usage: delay | [W] Usage: delay <0..255> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | reject invalid conversion delay |
| 178 | targeted | `tempco 16384` | Usage: tempco | [W] Usage: tempco <0..16383> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | reject invalid tempco |
| 179 | targeted | `tempcomp 2` | Usage: tempcomp | [W] Usage: tempcomp <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | reject invalid temp compensation |
| 180 | targeted | `alatch 2` | Usage: alatch | [W] Usage: alatch <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | reject invalid latch |
| 181 | targeted | `cnvralert 2` | Usage: cnvralert | [W] Usage: cnvralert <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | reject invalid conversion alert |
| 182 | targeted | `alslow 2` | Usage: alslow | [W] Usage: alslow <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | reject invalid slow alert |
| 183 | targeted | `apol 2` | Usage: apol | [W] Usage: apol <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | reject invalid alert polarity |
| 184 | targeted | `cal 0 10` | Usage: cal | [W] Usage: cal <shunt_ohm> <max_current_a> / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | reject zero shunt calibration |
| 185 | targeted | `reg16 0x100` | Usage: reg16 | [W] Usage: reg16 <addr> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | reject invalid raw16 register |
| 186 | targeted | `reg24 0x100` | Usage: reg24 | [W] Usage: reg24 <addr> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.063 | reject invalid raw24 register |
| 187 | targeted | `reg40 0x100` | Usage: reg40 | [W] Usage: reg40 <addr> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | reject invalid raw40 register |
| 188 | targeted | `unknown_hil_command` | Unknown command, INVALID_PARAM | [W] Unknown command: unknown_hil_command / [runner] frame_status=INVALID_PARAM frame_elapsed_ms=1 / Status: INVALID_PARAM | PASS | 0.063 | reject unknown command with framed status |
| 189 | targeted | `init 0x50` | Invalid address | [W] Invalid address. Use init 0x40-0x4F / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | reject invalid init address |
| 190 | targeted | `end` | Device shut down | [I] Device shut down. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.062 | end driver |
| 191 | targeted | `vbus` | NOT_INITIALIZED | Status: NOT_INITIALIZED (code=1, detail=0) / Message: begin() not called / [runner] frame_status=NOT_INITIALIZED frame_elapsed_ms=0 / Status: NOT_INITIALIZED | PASS | 0.063 | read after end must fail visibly |
| 192 | targeted | `init 0x41` | begin, OK | [I] begin(0x41): OK / [runner] frame_status=OK frame_elapsed_ms=3 | PASS | 0.062 | reinitialize known device |
| 193 | targeted | `recover` | Attempting recovery, frame_status=OK | [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Co... | PASS | 0.063 | manual recovery after reinit |
| 194 | targeted | `settings` | Active Settings, State:, READY | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.063 | final settings |
| 195 | targeted | `drv` | Driver Health, State: READY, Consecutive failures: 0 | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 9 / Total failures: 0 / Success rate... | PASS | 0.062 | final health must be clean |
| 196 | targeted | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4424 V / Vshunt:  0.0000113 V / Temp:    28.24 C / Current: 0.000744 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | final aggregate read |
| 197 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4414 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 198 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4402 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 199 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4406 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 200 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4408 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 201 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4408 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 202 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4404 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 203 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4393 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark bus voltage |
| 204 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4387 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 205 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4387 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 206 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4396 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark bus voltage |
| 207 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4385 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 208 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4373 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 209 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4375 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark bus voltage |
| 210 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4367 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 211 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4373 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark bus voltage |
| 212 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4365 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 213 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4352 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark bus voltage |
| 214 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4361 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark bus voltage |
| 215 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4344 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 216 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4340 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 217 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4355 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 218 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4352 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 219 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4355 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 220 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4361 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark bus voltage |
| 221 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4334 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark bus voltage |
| 222 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4328 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark bus voltage |
| 223 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4330 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 224 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4354 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark bus voltage |
| 225 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4354 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 226 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4314 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 227 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4379 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 228 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4338 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 229 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4344 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 230 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4332 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 231 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4328 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 232 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4338 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 233 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4344 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 234 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4344 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 235 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4330 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 236 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4326 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark bus voltage |
| 237 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4322 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 238 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4332 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 239 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4330 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 240 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4338 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 241 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4338 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 242 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4326 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 243 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4334 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 244 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4313 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark bus voltage |
| 245 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4322 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark bus voltage |
| 246 | benchmark | `vbus` | Vbus | [I] Vbus: 0.4328 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark bus voltage |
| 247 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000116 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 248 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000091 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 249 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000116 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 250 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000103 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 251 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000109 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 252 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000050 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 253 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000113 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 254 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000128 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 255 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000128 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 256 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000103 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 257 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000153 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 258 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000138 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 259 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000122 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark shunt voltage |
| 260 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000109 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 261 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000128 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 262 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000097 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 263 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000125 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 264 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000106 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 265 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000100 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark shunt voltage |
| 266 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000125 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 267 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000062 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 268 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000131 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 269 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000113 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 270 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000141 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 271 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000119 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 272 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000103 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 273 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000138 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 274 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000150 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark shunt voltage |
| 275 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000100 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 276 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000116 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 277 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000128 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 278 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000134 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 279 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000128 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 280 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000119 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 281 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000084 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 282 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000088 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark shunt voltage |
| 283 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000134 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 284 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000106 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 285 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000141 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 286 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000134 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 287 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000116 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 288 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000116 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 289 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000106 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark shunt voltage |
| 290 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000131 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 291 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000131 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark shunt voltage |
| 292 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000106 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark shunt voltage |
| 293 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000116 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark shunt voltage |
| 294 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000103 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 295 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000128 V / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark shunt voltage |
| 296 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0000147 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark shunt voltage |
| 297 | benchmark | `temp` | Temp | [I] Temp: 28.29 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 298 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 299 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.062 | benchmark temperature |
| 300 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 301 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 302 | benchmark | `temp` | Temp | [I] Temp: 28.24 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 303 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 304 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 305 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark temperature |
| 306 | benchmark | `temp` | Temp | [I] Temp: 28.23 C / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark temperature |
| 307 | benchmark | `temp` | Temp | [I] Temp: 28.24 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 308 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 309 | benchmark | `temp` | Temp | [I] Temp: 28.28 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 310 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 311 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 312 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 313 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark temperature |
| 314 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 315 | benchmark | `temp` | Temp | [I] Temp: 28.23 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 316 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 317 | benchmark | `temp` | Temp | [I] Temp: 28.24 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 318 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 319 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 320 | benchmark | `temp` | Temp | [I] Temp: 28.28 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 321 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 322 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark temperature |
| 323 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 324 | benchmark | `temp` | Temp | [I] Temp: 28.23 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 325 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 326 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 327 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 328 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 329 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 330 | benchmark | `temp` | Temp | [I] Temp: 28.23 C / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark temperature |
| 331 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 332 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 333 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 334 | benchmark | `temp` | Temp | [I] Temp: 28.23 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 335 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 336 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 337 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 338 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 339 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 340 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 341 | benchmark | `temp` | Temp | [I] Temp: 28.24 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 342 | benchmark | `temp` | Temp | [I] Temp: 28.24 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 343 | benchmark | `temp` | Temp | [I] Temp: 28.27 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 344 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.062 | benchmark temperature |
| 345 | benchmark | `temp` | Temp | [I] Temp: 28.26 C / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.063 | benchmark temperature |
| 346 | benchmark | `temp` | Temp | [I] Temp: 28.25 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.063 | benchmark temperature |
| 347 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 32 (0x000020) / Vbus:   2211 (0x0008A3) / Temp:   3618 (0x0E22) / Current:34 (0x000022) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 348 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 37 (0x000025) / Vbus:   2209 (0x0008A1) / Temp:   3615 (0x0E1F) / Current:40 (0x000028) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 349 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 37 (0x000025) / Vbus:   2212 (0x0008A4) / Temp:   3616 (0x0E20) / Current:42 (0x00002A) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 350 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 32 (0x000020) / Vbus:   2198 (0x000896) / Temp:   3614 (0x0E1E) / Current:34 (0x000022) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 351 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 47 (0x00002F) / Vbus:   2208 (0x0008A0) / Temp:   3616 (0x0E20) / Current:51 (0x000033) / Power:  6 (0x000006) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 352 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 36 (0x000024) / Vbus:   2208 (0x0008A0) / Temp:   3616 (0x0E20) / Current:44 (0x00002C) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 353 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 30 (0x00001E) / Vbus:   2209 (0x0008A1) / Temp:   3616 (0x0E20) / Current:32 (0x000020) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 354 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 30 (0x00001E) / Vbus:   2213 (0x0008A5) / Temp:   3616 (0x0E20) / Current:32 (0x000020) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 355 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 36 (0x000024) / Vbus:   2207 (0x00089F) / Temp:   3616 (0x0E20) / Current:39 (0x000027) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 356 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 22 (0x000016) / Vbus:   2221 (0x0008AD) / Temp:   3616 (0x0E20) / Current:24 (0x000018) / Power:  3 (0x000003) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 357 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 40 (0x000028) / Vbus:   2210 (0x0008A2) / Temp:   3617 (0x0E21) / Current:43 (0x00002B) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 358 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 43 (0x00002B) / Vbus:   2207 (0x00089F) / Temp:   3618 (0x0E22) / Current:46 (0x00002E) / Power:  6 (0x000006) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 359 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 35 (0x000023) / Vbus:   2206 (0x00089E) / Temp:   3617 (0x0E21) / Current:38 (0x000026) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 360 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 41 (0x000029) / Vbus:   2206 (0x00089E) / Temp:   3616 (0x0E20) / Current:32 (0x000020) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 361 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 44 (0x00002C) / Vbus:   2203 (0x00089B) / Temp:   3616 (0x0E20) / Current:48 (0x000030) / Power:  6 (0x000006) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 362 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 32 (0x000020) / Vbus:   2216 (0x0008A8) / Temp:   3614 (0x0E1E) / Current:34 (0x000022) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 363 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 36 (0x000024) / Vbus:   2205 (0x00089D) / Temp:   3615 (0x0E1F) / Current:39 (0x000027) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 364 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 36 (0x000024) / Vbus:   2209 (0x0008A1) / Temp:   3616 (0x0E20) / Current:39 (0x000027) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 365 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 40 (0x000028) / Vbus:   2205 (0x00089D) / Temp:   3615 (0x0E1F) / Current:43 (0x00002B) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 366 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 29 (0x00001D) / Vbus:   2195 (0x000893) / Temp:   3618 (0x0E22) / Current:31 (0x00001F) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 367 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 35 (0x000023) / Vbus:   2212 (0x0008A4) / Temp:   3615 (0x0E1F) / Current:38 (0x000026) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 368 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 41 (0x000029) / Vbus:   2205 (0x00089D) / Temp:   3620 (0x0E24) / Current:44 (0x00002C) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 369 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 39 (0x000027) / Vbus:   2216 (0x0008A8) / Temp:   3620 (0x0E24) / Current:42 (0x00002A) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 370 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 32 (0x000020) / Vbus:   2218 (0x0008AA) / Temp:   3615 (0x0E1F) / Current:34 (0x000022) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 371 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 43 (0x00002B) / Vbus:   2203 (0x00089B) / Temp:   3616 (0x0E20) / Current:46 (0x00002E) / Power:  6 (0x000006) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 372 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 37 (0x000025) / Vbus:   2200 (0x000898) / Temp:   3619 (0x0E23) / Current:31 (0x00001F) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 373 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 35 (0x000023) / Vbus:   2218 (0x0008AA) / Temp:   3616 (0x0E20) / Current:38 (0x000026) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 374 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 31 (0x00001F) / Vbus:   2208 (0x0008A0) / Temp:   3615 (0x0E1F) / Current:33 (0x000021) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 375 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 37 (0x000025) / Vbus:   2207 (0x00089F) / Temp:   3617 (0x0E21) / Current:33 (0x000021) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 376 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 34 (0x000022) / Vbus:   2210 (0x0008A2) / Temp:   3616 (0x0E20) / Current:37 (0x000025) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 377 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 33 (0x000021) / Vbus:   2213 (0x0008A5) / Temp:   3620 (0x0E24) / Current:36 (0x000024) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 378 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 40 (0x000028) / Vbus:   2203 (0x00089B) / Temp:   3615 (0x0E1F) / Current:43 (0x00002B) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 379 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 37 (0x000025) / Vbus:   2208 (0x0008A0) / Temp:   3615 (0x0E1F) / Current:40 (0x000028) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 380 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 42 (0x00002A) / Vbus:   2201 (0x000899) / Temp:   3616 (0x0E20) / Current:45 (0x00002D) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 381 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 29 (0x00001D) / Vbus:   2197 (0x000895) / Temp:   3617 (0x0E21) / Current:36 (0x000024) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 382 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 40 (0x000028) / Vbus:   2195 (0x000893) / Temp:   3615 (0x0E1F) / Current:43 (0x00002B) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 383 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 35 (0x000023) / Vbus:   2203 (0x00089B) / Temp:   3616 (0x0E20) / Current:38 (0x000026) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 384 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 34 (0x000022) / Vbus:   2208 (0x0008A0) / Temp:   3620 (0x0E24) / Current:37 (0x000025) / Power:  4 (0x000004) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 385 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 44 (0x00002C) / Vbus:   2201 (0x000899) / Temp:   3617 (0x0E21) / Current:48 (0x000030) / Power:  6 (0x000006) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 386 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 46 (0x00002E) / Vbus:   2209 (0x0008A1) / Temp:   3619 (0x0E23) / Current:50 (0x000032) / Power:  6 (0x000006) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 387 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 47 (0x00002F) / Vbus:   2222 (0x0008AE) / Temp:   3617 (0x0E21) / Current:51 (0x000033) / Power:  6 (0x000006) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 388 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 36 (0x000024) / Vbus:   2203 (0x00089B) / Temp:   3616 (0x0E20) / Current:39 (0x000027) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 389 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 50 (0x000032) / Vbus:   2200 (0x000898) / Temp:   3616 (0x0E20) / Current:37 (0x000025) / Power:  7 (0x000007) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 390 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 35 (0x000023) / Vbus:   2201 (0x000899) / Temp:   3616 (0x0E20) / Current:38 (0x000026) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 391 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 27 (0x00001B) / Vbus:   2204 (0x00089C) / Temp:   3616 (0x0E20) / Current:29 (0x00001D) / Power:  6 (0x000006) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 392 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 44 (0x00002C) / Vbus:   2206 (0x00089E) / Temp:   3617 (0x0E21) / Current:48 (0x000030) / Power:  6 (0x000006) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 393 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 39 (0x000027) / Vbus:   2206 (0x00089E) / Temp:   3616 (0x0E20) / Current:42 (0x00002A) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 394 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 37 (0x000025) / Vbus:   2211 (0x0008A3) / Temp:   3621 (0x0E25) / Current:40 (0x000028) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 395 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 35 (0x000023) / Vbus:   2206 (0x00089E) / Temp:   3620 (0x0E24) / Current:38 (0x000026) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.062 | benchmark raw sample |
| 396 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 41 (0x000029) / Vbus:   2206 (0x00089E) / Temp:   3616 (0x0E20) / Current:44 (0x00002C) / Power:  5 (0x000005) / Energy: 0 / ... | PASS | 0.063 | benchmark raw sample |
| 397 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   13 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 398 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   13 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 399 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   10 uV / Temp:    28266 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 400 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   12 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 401 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   15 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 402 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     432 mV / Shunt:   9 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.063 | benchmark integer sample |
| 403 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     432 mV / Shunt:   12 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 404 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     432 mV / Shunt:   11 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 405 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     433 mV / Shunt:   13 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 406 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   13 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 407 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     432 mV / Shunt:   11 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 408 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   12 uV / Temp:    28234 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 409 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   12 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 410 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     432 mV / Shunt:   13 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 411 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   13 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 412 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   10 uV / Temp:    28266 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 413 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   12 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 414 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   10 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 415 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     433 mV / Shunt:   15 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 416 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   10 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 417 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   13 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 418 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     434 mV / Shunt:   12 uV / Temp:    28297 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 419 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   13 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 420 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   11 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 421 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   11 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 422 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   10 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 423 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     429 mV / Shunt:   13 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 424 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   10 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 425 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   11 uV / Temp:    28266 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 426 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   8 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.063 | benchmark integer sample |
| 427 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     433 mV / Shunt:   13 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 428 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     433 mV / Shunt:   12 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 429 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   13 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 430 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   13 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 431 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   12 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 432 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   11 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 433 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   8 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.063 | benchmark integer sample |
| 434 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   11 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 435 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   13 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 436 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   8 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.063 | benchmark integer sample |
| 437 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   12 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 438 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     433 mV / Shunt:   14 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 439 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     432 mV / Shunt:   13 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 440 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     433 mV / Shunt:   12 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 441 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     430 mV / Shunt:   12 uV / Temp:    28242 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 442 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     432 mV / Shunt:   13 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 443 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     431 mV / Shunt:   13 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 444 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     432 mV / Shunt:   13 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.063 | benchmark integer sample |
| 445 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     432 mV / Shunt:   12 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.062 | benchmark integer sample |
| 446 | benchmark | `integer` | Integer Sample | === Integer Sample === / Bus:     433 mV / Shunt:   9 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.062 | benchmark integer sample |
| 447 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4326 V / Vshunt:  0.0000144 V / Temp:    28.24 C / Current: 0.000954 A / Power:   0.000366 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 448 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4320 V / Vshunt:  0.0000125 V / Temp:    28.24 C / Current: 0.000820 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 449 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4318 V / Vshunt:  0.0000125 V / Temp:    28.26 C / Current: 0.000820 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 450 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4314 V / Vshunt:  0.0000156 V / Temp:    28.25 C / Current: 0.001030 A / Power:   0.000427 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 451 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000106 V / Temp:    28.26 C / Current: 0.000706 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 452 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000103 V / Temp:    28.27 C / Current: 0.000687 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 453 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4309 V / Vshunt:  0.0000134 V / Temp:    28.28 C / Current: 0.000877 A / Power:   0.000366 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 454 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4305 V / Vshunt:  0.0000166 V / Temp:    28.26 C / Current: 0.000629 A / Power:   0.000427 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 455 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4297 V / Vshunt:  0.0000128 V / Temp:    28.26 C / Current: 0.000839 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 456 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4299 V / Vshunt:  0.0000113 V / Temp:    28.26 C / Current: 0.000744 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 457 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4336 V / Vshunt:  0.0000103 V / Temp:    28.28 C / Current: 0.000801 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 458 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000113 V / Temp:    28.28 C / Current: 0.000744 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 459 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4316 V / Vshunt:  0.0000147 V / Temp:    28.30 C / Current: 0.000973 A / Power:   0.000366 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 460 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4309 V / Vshunt:  0.0000119 V / Temp:    28.29 C / Current: 0.000782 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 461 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000147 V / Temp:    28.26 C / Current: 0.000973 A / Power:   0.000366 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 462 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4314 V / Vshunt:  0.0000116 V / Temp:    28.29 C / Current: 0.000763 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 463 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4316 V / Vshunt:  0.0000088 V / Temp:    28.27 C / Current: 0.000572 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 464 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4313 V / Vshunt:  0.0000113 V / Temp:    28.27 C / Current: 0.000744 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 465 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4318 V / Vshunt:  0.0000113 V / Temp:    28.24 C / Current: 0.000744 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 466 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4313 V / Vshunt:  0.0000091 V / Temp:    28.27 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 467 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4320 V / Vshunt:  0.0000094 V / Temp:    28.30 C / Current: 0.000610 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 468 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4303 V / Vshunt:  0.0000119 V / Temp:    28.28 C / Current: 0.000782 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 469 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4301 V / Vshunt:  0.0000119 V / Temp:    28.26 C / Current: 0.000782 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 470 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000134 V / Temp:    28.27 C / Current: 0.000877 A / Power:   0.000366 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 471 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000069 V / Temp:    28.26 C / Current: 0.000458 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 472 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4313 V / Vshunt:  0.0000119 V / Temp:    28.26 C / Current: 0.000782 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 473 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4326 V / Vshunt:  0.0000144 V / Temp:    28.26 C / Current: 0.000954 A / Power:   0.000366 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 474 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000131 V / Temp:    28.25 C / Current: 0.000858 A / Power:   0.000366 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 475 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4309 V / Vshunt:  0.0000084 V / Temp:    28.30 C / Current: 0.000553 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 476 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4309 V / Vshunt:  0.0000100 V / Temp:    28.26 C / Current: 0.000648 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 477 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4301 V / Vshunt:  0.0000119 V / Temp:    28.27 C / Current: 0.000782 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 478 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4316 V / Vshunt:  0.0000100 V / Temp:    28.27 C / Current: 0.000648 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 479 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4313 V / Vshunt:  0.0000081 V / Temp:    28.25 C / Current: 0.000534 A / Power:   0.000183 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 480 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4326 V / Vshunt:  0.0000103 V / Temp:    28.27 C / Current: 0.000877 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 481 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4318 V / Vshunt:  0.0000106 V / Temp:    28.27 C / Current: 0.001049 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 482 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4305 V / Vshunt:  0.0000125 V / Temp:    28.27 C / Current: 0.000820 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 483 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4326 V / Vshunt:  0.0000134 V / Temp:    28.27 C / Current: 0.000877 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 484 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000125 V / Temp:    28.28 C / Current: 0.000820 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 485 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4307 V / Vshunt:  0.0000122 V / Temp:    28.28 C / Current: 0.000801 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 486 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4313 V / Vshunt:  0.0000131 V / Temp:    28.27 C / Current: 0.000858 A / Power:   0.000366 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 487 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4305 V / Vshunt:  0.0000066 V / Temp:    28.29 C / Current: 0.000420 A / Power:   0.000122 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 488 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4330 V / Vshunt:  0.0000119 V / Temp:    28.27 C / Current: 0.000782 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 489 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4309 V / Vshunt:  0.0000097 V / Temp:    28.29 C / Current: 0.000629 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 490 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4309 V / Vshunt:  0.0000125 V / Temp:    28.25 C / Current: 0.000820 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 491 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4313 V / Vshunt:  0.0000109 V / Temp:    28.29 C / Current: 0.000725 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 492 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4301 V / Vshunt:  0.0000091 V / Temp:    28.25 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 493 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4314 V / Vshunt:  0.0000119 V / Temp:    28.29 C / Current: 0.000782 A / Power:   0.000305 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 494 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4307 V / Vshunt:  0.0000163 V / Temp:    28.25 C / Current: 0.001068 A / Power:   0.000427 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 495 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000091 V / Temp:    28.28 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.063 | benchmark aggregate |
| 496 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4311 V / Vshunt:  0.0000091 V / Temp:    28.26 C / Current: 0.000591 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.062 | benchmark aggregate |
| 497 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   10 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 498 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   11 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 499 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   12 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 500 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   12 uV / Temp:    28297 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 501 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   12 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 502 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   12 uV / Temp:    28305 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 503 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   11 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 504 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     433 mV / Shunt:   11 uV / Temp:    28297 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 505 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   11 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 506 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   11 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 507 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   15 uV / Temp:    28297 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 508 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   12 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 509 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   11 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 510 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   14 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 511 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   12 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 512 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   9 uV / Temp:    28297 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.062 | benchmark fixed-step sample |
| 513 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     433 mV / Shunt:   10 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 514 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   13 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 515 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   13 uV / Temp:    28266 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 516 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   13 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 517 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   8 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.062 | benchmark fixed-step sample |
| 518 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   13 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 519 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   10 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 520 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   13 uV / Temp:    28266 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 521 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   10 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 522 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   13 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 523 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   14 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 524 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     433 mV / Shunt:   13 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 525 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   13 uV / Temp:    28266 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 526 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   12 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 527 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     433 mV / Shunt:   6 uV / Temp:    28297 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.063 | benchmark fixed-step sample |
| 528 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   10 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 529 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   13 uV / Temp:    28305 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 530 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   12 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 531 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   13 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 532 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     434 mV / Shunt:   14 uV / Temp:    28266 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 533 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   14 uV / Temp:    28250 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 534 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   12 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 535 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   11 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 536 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   14 uV / Temp:    28266 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 537 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   12 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 538 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     429 mV / Shunt:   9 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.063 | benchmark fixed-step sample |
| 539 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     430 mV / Shunt:   13 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 540 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   11 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 541 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   8 uV / Temp:    28305 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.062 | benchmark fixed-step sample |
| 542 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   10 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 543 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   12 uV / Temp:    28281 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 544 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   11 uV / Temp:    28273 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.062 | benchmark fixed-step sample |
| 545 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   11 uV / Temp:    28289 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 546 | benchmark | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   12 uV / Temp:    28258 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.063 | benchmark fixed-step sample |
| 547 | soak | `sample_step 0` | INVALID_PARAM | [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status=INVALI... | PASS | 0.063 | targeted soak sample zero-budget rejection |
| 548 | soak | `reset_start` | startResetJob, IN_PROGRESS | [runner] empty framed response attempt 1; retrying / [I] startResetJob(): BUSY / Status: BUSY (code=11, detail=0) / Message: Another fixed-step job is active... | FAIL | 12.156 | targeted soak reset start |
| 549 | not-run | `<fixture: disconnected target>` | safe absent-device fixture | requires safe disconnect or switched fixture | NOT RUN | 0.000 | requires safe disconnect or switched fixture |
| 550 | not-run | `<fixture: bus fault injection>` | safe fault-injection fixture | requires safe NACK/timeout/bus-error injection | NOT RUN | 0.000 | requires safe NACK/timeout/bus-error injection |
| 551 | not-run | `<fixture: alert pin capture>` | alert pin instrumentation | requires alert-pin wiring and safe threshold stimulus | NOT RUN | 0.000 | requires alert-pin wiring and safe threshold stimulus |
| 552 | not-run | `<fixture: MCU reset or power cycle>` | controlled reset/power fixture | requires explicit reset/power-cycle control | NOT RUN | 0.000 | requires explicit reset/power-cycle control |

## Soak Summary

- Requested duration: 72000.0 s
- Executed soak commands: 7306
- Recorded soak rows: 2
- Soak PASS row storage stride: every 5000 PASS row(s), plus all FAIL/UNKNOWN rows
- Soak verdict counts: PASS=7305, FAIL=1, UNKNOWN=0
- Empty framed response retries: 1
- Soak latency min/mean/max: 0.062 / 0.064 / 12.156 s
- Command mix:
  - `adcrange 0`: 197
  - `adcrange 1`: 197
  - `apply_start`: 198
  - `apply_step 0`: 198
  - `apply_step 1`: 198
  - `apply_step 2`: 198
  - `apply_step 6`: 198
  - `diagraw`: 197
  - `diagsnap`: 197
  - `drv`: 197
  - `integer`: 197
  - `mode 15`: 394
  - `read`: 197
  - `ready_step 0`: 198
  - `ready_step 1`: 395
  - `ready_step 2`: 395
  - `ready_step 255`: 198
  - `recover`: 197
  - `reset_start`: 198
  - `reset_step 0`: 197
  - `reset_step 1`: 394
  - `reset_step 16`: 197
  - `sample_step 0`: 198
  - `sample_step 1`: 198
  - `sample_step 2`: 198
  - `sample_step 255`: 198
  - `sample_step 3`: 198
  - `sample_step 4`: 198
  - `sample_step 5`: 592
  - `trigger 1`: 197
  - `trigger 7`: 197
- Non-PASS soak command counts:
  - `reset_start`: 1
- Empty framed response retry counts:
  - `reset_start`: 1

## Limitations

- Hardware safety and fixture details must be filled in by the operator.
- This runner records serial CLI evidence only; external instruments must be logged separately.
- Staged `maxInstructions` coverage is limited to the example CLI commands. The `transfer` suite records example callback counts, not logic-analyzer bus bytes. Example `tick()` calls between serial commands can add readiness reads; exact assertions are kept to deterministic paths and other paths record snapshots.
