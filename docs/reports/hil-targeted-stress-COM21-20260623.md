# INA228 HIL Validation Report

- Date/time: 2026-06-23T07:50:15.053136+02:00 to 2026-06-23T07:56:37.809759+02:00
- Elapsed: 382.8 s
- Port: COM21
- Baud: 115200
- Suite: targeted
- Soak requested: 600.0 s
- Operator: Codex
- Board/environment: ESP32S3_COM21 / esp32s3dev_Arduino
- Fixture: INA228_0x41_low_voltage_no_fault_injection
- Safety assumptions: benign_fixture_no_fault_stimulus
- OS: Windows-11-10.0.26200-SP0
- Python: 3.12.10
- HIL command: `tools\run_i2c_hil.py --port COM21 --baud 115200 --suite targeted --timeout-s 12 --idle-s 0.3 --boot-settle-s 1 --boot-capture-s 4 --prompt-token >  --empty-retries 1 --marker-retries 1 --command-pause-s 0.05 --soak-seconds 600 --soak-store-every 25 --soak-progress-every 25 --stop-on-non-pass --include-not-run --report docs\reports\hil-targeted-stress-COM21-20260623.md --transcript docs\reports\hil-targeted-stress-COM21-20260623.log --operator Codex --board ESP32S3_COM21 --environment esp32s3dev_Arduino --fixture INA228_0x41_low_voltage_no_fault_injection --safety benign_fixture_no_fault_stimulus --notes targeted_10min_instruction_budget_active_job_stress --fail-on-unknown`
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

- Transcript: `docs/reports/hil-targeted-stress-COM21-20260623.log`
- Notes: targeted_10min_instruction_budget_active_job_stress

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 6787 | 0 | 1 | 4 |

## Timing Summary

- Commands executed: 6792
- Commands recorded in detail: 463
- Soak commands executed: 6593
- Soak rows recorded in detail: 264
- Recorded command latency min/mean/max: 0.000 / 0.028 / 12.031 s

- Maximum consecutive FAIL verdicts: 0

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | INA228 library version | === Version Info === / Example firmware build: Jun 22 2026 21:16:34 / INA228 library version: 2.0.0 / INA228 library full: 2.0.0 (5840497, 2026-06-22 21:16:3... | PASS | 0.000 | version |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | > [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / 00:                         -- -- -- -- -- -- -- -- / 10... | PASS | 0.141 | scan |
| 3 | smoke | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.015 | probe |
| 4 | smoke | `settings` | Active Settings, State:, Address: | > === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ... | PASS | 0.000 | settings |
| 5 | smoke | `drv` | Driver Health, State:, Online: | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 1 / Total failures: 0 / Success ra... | PASS | 0.000 | health |
| 6 | smoke | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | DIAG_ALRT |
| 7 | smoke | `raw` | Raw Registers, Vbus, Temp | > === Raw Registers === / Vshunt: 14 (0x00000E) / Vbus:   2202 (0x00089A) / Temp:   3885 (0x0F2D) / Current:15 (0x00000F) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | conversion raw read |
| 8 | targeted | `verbose 0` | Verbose mode | > [I] Verbose mode: OFF / > [I] | PASS | 0.000 | reduce CLI chatter |
| 9 | targeted | `help` | mode [0..15], sample_step <budget>, limits | > / === INA228 CLI Help === / [W] Safety: this example does not make 85 V systems safe. Use qualified design practices, isolation where needed, fusing, creep... | PASS | 0.000 | targeted CLI surface check |
| 10 | targeted | `drv` | Driver Health, State:, Online: | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 10 / Total failures: 0 / Success r... | PASS | 0.000 | initial health before mutation |
| 11 | targeted | `settings` | Active Settings, Mode:, ADC range: | > === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ... | PASS | 0.000 | initial settings snapshot |
| 12 | targeted | `mode 0` | setMode | > [I] setMode(0 = SHUTDOWN): OK / > [I] | PASS | 0.016 | set operating mode 0 |
| 13 | targeted | `mode 1` | setMode | [I] setMode(1 = TRIG_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 1 |
| 14 | targeted | `mode 2` | setMode | > [I] setMode(2 = TRIG_SHUNT): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 2 |
| 15 | targeted | `mode 3` | setMode | > [I] setMode(3 = TRIG_SHUNT_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 3 |
| 16 | targeted | `mode 4` | setMode | [I] setMode(4 = TRIG_TEMP): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 4 |
| 17 | targeted | `mode 5` | setMode | > [I] setMode(5 = TRIG_TEMP_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 5 |
| 18 | targeted | `mode 6` | setMode | > [I] setMode(6 = TRIG_TEMP_SHUNT): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 6 |
| 19 | targeted | `mode 7` | setMode | > [I] setMode(7 = TRIG_ALL): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 7 |
| 20 | targeted | `mode 8` | setMode | [I] setMode(8 = SHUTDOWN2): OK / > [I] | PASS | 0.000 | set operating mode 8 |
| 21 | targeted | `mode 9` | setMode | > [I] setMode(9 = CONT_BUS): OK / > [I] | PASS | 0.000 | set operating mode 9 |
| 22 | targeted | `mode 10` | setMode | > [I] setMode(10 = CONT_SHUNT): OK / > [I] | PASS | 0.000 | set operating mode 10 |
| 23 | targeted | `mode 11` | setMode | > [I] setMode(11 = CONT_SHUNT_BUS): OK / > [I] | PASS | 0.000 | set operating mode 11 |
| 24 | targeted | `mode 12` | setMode | > [I] setMode(12 = CONT_TEMP): OK / > [I] | PASS | 0.000 | set operating mode 12 |
| 25 | targeted | `mode 13` | setMode | [I] setMode(13 = CONT_TEMP_BUS): OK / > [I] | PASS | 0.016 | set operating mode 13 |
| 26 | targeted | `mode 14` | setMode | > [I] setMode(14 = CONT_TEMP_SHUNT): OK / > [I] | PASS | 0.000 | set operating mode 14 |
| 27 | targeted | `mode 15` | setMode | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | set operating mode 15 |
| 28 | targeted | `mode` | Mode: | Mode: CONT_ALL (15) / > [I] | PASS | 0.000 | query mode after full mode sweep |
| 29 | targeted | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | restore continuous-all mode |
| 30 | targeted | `convtime vbus 0` | setConvTime | [I] setConvTime(vbus, 50us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 0 |
| 31 | targeted | `convtime vbus 1` | setConvTime | > [I] setConvTime(vbus, 84us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 1 |
| 32 | targeted | `convtime vbus 2` | setConvTime | > [I] setConvTime(vbus, 150us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 2 |
| 33 | targeted | `convtime vbus 3` | setConvTime | > [I] setConvTime(vbus, 280us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 3 |
| 34 | targeted | `convtime vbus 4` | setConvTime | > [I] setConvTime(vbus, 540us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 4 |
| 35 | targeted | `convtime vbus 5` | setConvTime | > [I] setConvTime(vbus, 1052us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 5 |
| 36 | targeted | `convtime vbus 6` | setConvTime | [I] setConvTime(vbus, 2074us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 6 |
| 37 | targeted | `convtime vbus 7` | setConvTime | > [I] setConvTime(vbus, 4120us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 7 |
| 38 | targeted | `convtime vsh 0` | setConvTime | > [I] setConvTime(vsh, 50us): OK / > [I] | PASS | 0.000 | set vsh conversion time index 0 |
| 39 | targeted | `convtime vsh 1` | setConvTime | > [I] setConvTime(vsh, 84us): OK / > [I] | PASS | 0.000 | set vsh conversion time index 1 |
| 40 | targeted | `convtime vsh 2` | setConvTime | > [I] setConvTime(vsh, 150us): OK / > [I] | PASS | 0.000 | set vsh conversion time index 2 |
| 41 | targeted | `convtime vsh 3` | setConvTime | > [I] setConvTime(vsh, 280us): OK / > [I] | PASS | 0.000 | set vsh conversion time index 3 |
| 42 | targeted | `convtime vsh 4` | setConvTime | > [I] setConvTime(vsh, 540us): OK / > [I] | PASS | 0.000 | set vsh conversion time index 4 |
| 43 | targeted | `convtime vsh 5` | setConvTime | > [I] setConvTime(vsh, 1052us): OK / > [I] | PASS | 0.000 | set vsh conversion time index 5 |
| 44 | targeted | `convtime vsh 6` | setConvTime | [I] setConvTime(vsh, 2074us): OK / > [I] | PASS | 0.000 | set vsh conversion time index 6 |
| 45 | targeted | `convtime vsh 7` | setConvTime | > [I] setConvTime(vsh, 4120us): OK / > [I] | PASS | 0.000 | set vsh conversion time index 7 |
| 46 | targeted | `convtime temp 0` | setConvTime | > [I] setConvTime(temp, 50us): OK / > [I] | PASS | 0.000 | set temp conversion time index 0 |
| 47 | targeted | `convtime temp 1` | setConvTime | > [I] setConvTime(temp, 84us): OK / > [I] | PASS | 0.000 | set temp conversion time index 1 |
| 48 | targeted | `convtime temp 2` | setConvTime | > [I] setConvTime(temp, 150us): OK / > [I] | PASS | 0.000 | set temp conversion time index 2 |
| 49 | targeted | `convtime temp 3` | setConvTime | [I] setConvTime(temp, 280us): OK / > [I] | PASS | 0.000 | set temp conversion time index 3 |
| 50 | targeted | `convtime temp 4` | setConvTime | > [I] setConvTime(temp, 540us): OK / > [I] | PASS | 0.016 | set temp conversion time index 4 |
| 51 | targeted | `convtime temp 5` | setConvTime | > [I] setConvTime(temp, 1052us): OK / > [I] | PASS | 0.000 | set temp conversion time index 5 |
| 52 | targeted | `convtime temp 6` | setConvTime | [I] setConvTime(temp, 2074us): OK / > [I] | PASS | 0.000 | set temp conversion time index 6 |
| 53 | targeted | `convtime temp 7` | setConvTime | > [I] setConvTime(temp, 4120us): OK / > [I] | PASS | 0.016 | set temp conversion time index 7 |
| 54 | targeted | `convtime` | Conversion times | > Conversion times: VBUS=4120us  VSHUNT=4120us  TEMP=4120us / > [I] | PASS | 0.000 | query conversion times |
| 55 | targeted | `convtime vbus 5` | setConvTime, OK | [I] setConvTime(vbus, 1052us): OK / > [I] | PASS | 0.000 | restore VBUS conversion time |
| 56 | targeted | `convtime vsh 5` | setConvTime, OK | > [I] setConvTime(vsh, 1052us): OK / > [I] | PASS | 0.000 | restore VSHUNT conversion time |
| 57 | targeted | `convtime temp 5` | setConvTime, OK | [I] setConvTime(temp, 1052us): OK / > [I] | PASS | 0.000 | restore TEMP conversion time |
| 58 | targeted | `averaging 0` | setAveraging | > [I] setAveraging(1): OK / > [I] | PASS | 0.000 | set averaging index 0 |
| 59 | targeted | `averaging 1` | setAveraging | > [I] setAveraging(4): OK / > [I] | PASS | 0.000 | set averaging index 1 |
| 60 | targeted | `averaging 2` | setAveraging | [I] setAveraging(16): OK / > [I] | PASS | 0.000 | set averaging index 2 |
| 61 | targeted | `averaging 3` | setAveraging | > [I] setAveraging(64): OK / > [I] | PASS | 0.000 | set averaging index 3 |
| 62 | targeted | `averaging 4` | setAveraging | > [I] setAveraging(128): OK / > [I] | PASS | 0.000 | set averaging index 4 |
| 63 | targeted | `averaging 5` | setAveraging | > [I] setAveraging(256): OK / > [I] | PASS | 0.000 | set averaging index 5 |
| 64 | targeted | `averaging 6` | setAveraging | > [I] setAveraging(512): OK / > [I] | PASS | 0.000 | set averaging index 6 |
| 65 | targeted | `averaging 7` | setAveraging | > [I] setAveraging(1024): OK / > [I] | PASS | 0.000 | set averaging index 7 |
| 66 | targeted | `averaging` | Averaging: | > Averaging: 1024 samples / > [I] | PASS | 0.000 | query averaging |
| 67 | targeted | `averaging 0` | setAveraging, OK | > [I] setAveraging(1): OK / > [I] | PASS | 0.000 | restore averaging 1 |
| 68 | targeted | `adcrange 1` | setAdcRange, OK | > [I] setAdcRange(+/-40.96mV): OK / > [I] | PASS | 0.000 | switch low shunt range |
| 69 | targeted | `integer` | Integer Sample, Shunt: | > === Integer Sample === / Bus:     438 mV / Shunt:   6 uV / Temp:    30352 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | integer sample after low range |
| 70 | targeted | `adcrange 0` | setAdcRange, OK | [I] setAdcRange(+/-163.84mV): OK / > [I] | PASS | 0.000 | restore default shunt range |
| 71 | targeted | `adcrange` | ADC range: | ADC range: +/-163.84mV / > [I] | PASS | 0.000 | query ADC range |
| 72 | targeted | `delay 0` | setConversionDelay, OK | > [I] setConversionDelay(0): OK / > [I] | PASS | 0.000 | conversion delay min |
| 73 | targeted | `delay 1` | setConversionDelay, OK | [I] setConversionDelay(1): OK / > [I] | PASS | 0.000 | conversion delay small |
| 74 | targeted | `delay 127` | setConversionDelay, OK | > [I] setConversionDelay(127): OK / > [I] | PASS | 0.000 | conversion delay middle |
| 75 | targeted | `delay 255` | setConversionDelay, OK | > [I] setConversionDelay(255): OK / > [I] | PASS | 0.000 | conversion delay max |
| 76 | targeted | `delay` | Conversion delay: | > [I] Conversion delay: 255 x 2 ms (510 ms) / > [I] | PASS | 0.000 | query conversion delay |
| 77 | targeted | `delay 0` | setConversionDelay, OK | [I] setConversionDelay(0): OK / > [I] | PASS | 0.000 | restore zero conversion delay |
| 78 | targeted | `tempco 1` | setShuntTempCoeff, OK | > [I] setShuntTempCoeff(1): OK / > [I] | PASS | 0.000 | set small tempco |
| 79 | targeted | `tempco 16383` | setShuntTempCoeff, OK | > [I] setShuntTempCoeff(16383): OK / > [I] | PASS | 0.000 | set max tempco |
| 80 | targeted | `tempco` | Shunt temp coeff: | > [I] Shunt temp coeff: 16383 ppm/degC / > [I] | PASS | 0.000 | query tempco |
| 81 | targeted | `tempco 0` | setShuntTempCoeff, OK | [I] setShuntTempCoeff(0): OK / > [I] | PASS | 0.000 | restore tempco |
| 82 | targeted | `tempcomp 1` | setTempCompensation, OK | > [I] setTempCompensation(yes): OK / > [I] | PASS | 0.000 | enable temp compensation |
| 83 | targeted | `tempcomp` | Temperature compensation: | > [I] Temperature compensation: yes / > [I] | PASS | 0.000 | query temp compensation |
| 84 | targeted | `tempcomp 0` | setTempCompensation, OK | [I] setTempCompensation(no): OK / > [I] | PASS | 0.000 | restore temp compensation |
| 85 | targeted | `cal 0.015 10` | setCalibration, OK | > [I] setCalibration(0.015000, 10.000000): OK / > [I] | PASS | 0.016 | reapply nominal calibration |
| 86 | targeted | `cal` | Calibration:, CURRENT_LSB | > Calibration: Rshunt=0.015000 ohm  MaxCurrent=10.000000 A  CURRENT_LSB=0.000019073 A / > [I] | PASS | 0.000 | query calibration |
| 87 | targeted | `alatch 1` | setAlertLatch, OK | > [I] setAlertLatch(yes): OK / > [I] | PASS | 0.000 | enable alert latch |
| 88 | targeted | `alatch 0` | setAlertLatch, OK | > [I] setAlertLatch(no): OK / > [I] | PASS | 0.000 | restore alert latch |
| 89 | targeted | `cnvralert 1` | setConversionReadyAlert, OK | [I] setConversionReadyAlert(yes): OK / > [I] | PASS | 0.000 | enable conversion-ready alert bit |
| 90 | targeted | `cnvralert 0` | setConversionReadyAlert, OK | > [I] setConversionReadyAlert(no): OK / > [I] | PASS | 0.000 | restore conversion-ready alert bit |
| 91 | targeted | `alslow 1` | setSlowAlert, OK | [I] setSlowAlert(yes): OK / > [I] | PASS | 0.000 | enable slow alert |
| 92 | targeted | `alslow 0` | setSlowAlert, OK | > [I] setSlowAlert(no): OK / > [I] | PASS | 0.000 | restore slow alert |
| 93 | targeted | `apol 1` | setAlertPolarity, OK | > [I] setAlertPolarity(yes): OK / > [I] | PASS | 0.000 | set active-high alert |
| 94 | targeted | `apol 0` | setAlertPolarity, OK | > [I] setAlertPolarity(no): OK / > [I] | PASS | 0.000 | restore active-low alert |
| 95 | targeted | `sovl 0.001` | setShuntOvervoltageThreshold, OK | > [I] setShuntOvervoltageThreshold(0.0010000): OK / > [I] | PASS | 0.000 | safe shunt overvoltage threshold |
| 96 | targeted | `suvl -0.001` | setShuntUndervoltageThreshold, OK | [I] setShuntUndervoltageThreshold(-0.0010000): OK / > [I] | PASS | 0.000 | safe shunt undervoltage threshold |
| 97 | targeted | `bovl 1.0` | setBusOvervoltageThreshold, OK | > [I] setBusOvervoltageThreshold(1.0000): OK / > [I] | PASS | 0.000 | safe bus overvoltage threshold |
| 98 | targeted | `buvl 0.1` | setBusUndervoltageThreshold, OK | [I] setBusUndervoltageThreshold(0.1000): OK / > [I] | PASS | 0.000 | safe bus undervoltage threshold |
| 99 | targeted | `tmplim 100` | setTemperatureOverlimitThreshold, OK | [I] setTemperatureOverlimitThreshold(100.00): OK / > [I] | PASS | 0.000 | safe temperature threshold |
| 100 | targeted | `pwrlim 0.01` | setPowerOverlimitThreshold, OK | > [I] setPowerOverlimitThreshold(0.010000): OK / > [I] | PASS | 0.015 | safe power threshold |
| 101 | targeted | `limits` | Alert Limits | > === Alert Limits === / SOVL:      0x00C8  1.000 mV / SUVL:      0xFF38  -1.000 mV / BOVL:      0x0140  1.0000 V / BUVL:      0x0020  0.1000 V / TEMP_LIMIT:... | PASS | 0.000 | query alert limits |
| 102 | targeted | `sovl 0.163835` | setShuntOvervoltageThreshold, OK | > [I] setShuntOvervoltageThreshold(0.1638350): OK / > [I] | PASS | 0.000 | restore shunt overvoltage default |
| 103 | targeted | `suvl -0.16384` | setShuntUndervoltageThreshold, OK | > [I] setShuntUndervoltageThreshold(-0.1638400): OK / > [I] | PASS | 0.000 | restore shunt undervoltage default |
| 104 | targeted | `bovl 85` | setBusOvervoltageThreshold, OK | > [I] setBusOvervoltageThreshold(85.0000): OK / > [I] | PASS | 0.000 | restore safe max bus overvoltage |
| 105 | targeted | `buvl 0` | setBusUndervoltageThreshold, OK | > [I] setBusUndervoltageThreshold(0.0000): OK / > [I] | PASS | 0.016 | restore bus undervoltage default |
| 106 | targeted | `tmplim 255.99` | setTemperatureOverlimitThreshold, OK | > [I] setTemperatureOverlimitThreshold(255.99): OK / > [I] | PASS | 0.000 | restore safe max temperature threshold |
| 107 | targeted | `pwrlim 800` | setPowerOverlimitThreshold, OK | [I] setPowerOverlimitThreshold(800.000000): OK / > [I] | PASS | 0.000 | restore power threshold |
| 108 | targeted | `ready_step 0` | INVALID_PARAM | > [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | ready zero-budget rejection |
| 109 | targeted | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready budget one |
| 110 | targeted | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | ready budget two |
| 111 | targeted | `ready_step 255` | pollMeasurementReady | > [I] pollMeasurementReady(255): OK ready=yes / > [I] | PASS | 0.000 | ready max budget |
| 112 | targeted | `sample_step 0` | INVALID_PARAM | > [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | sample zero-budget rejection |
| 113 | targeted | `sample_step 1` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | sample budget one |
| 114 | targeted | `sample_step 2` | readPowerSampleRawStep | [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | sample budget two |
| 115 | targeted | `sample_step 3` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     434 mV / Shunt:   5 uV / Temp:    30359 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | sample budget three |
| 116 | targeted | `sample_step 4` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | sample budget four |
| 117 | targeted | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     432 mV / Shunt:   10 uV / Temp:    30367 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | sample full budget |
| 118 | targeted | `sample_step 255` | Power Sample Step Result | > [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     431 mV / Shunt:   8 uV / Temp:    30367 mdegC / Current: 1 mA / Power:   ... | PASS | 0.000 | sample max budget |
| 119 | targeted | `apply_start` | startApplyCalibration, IN_PROGRESS | > [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | calibration job start |
| 120 | targeted | `apply_step 0` | INVALID_PARAM | > [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | calibration zero-budget rejection |
| 121 | targeted | `apply_step 1` | pollApplyCalibration | > [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.016 | calibration budget one |
| 122 | targeted | `apply_step 2` | pollApplyCalibration | [I] pollApplyCalibration(2): IN_PROGRESS / > [I] | PASS | 0.000 | calibration budget two |
| 123 | targeted | `apply_step 3` | pollApplyCalibration | [I] pollApplyCalibration(3): OK / > [I] | PASS | 0.000 | calibration budget three |
| 124 | targeted | `apply_step 6` | BUSY | > [I] pollApplyCalibration(6): BUSY / Status: BUSY (code=11, detail=0) / Message: No apply calibration job active / > [I] | PASS | 0.000 | post-completion calibration poll reports BUSY |
| 125 | targeted | `apply_start` | startApplyCalibration, IN_PROGRESS | [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | calibration full-budget restart |
| 126 | targeted | `apply_step 6` | pollApplyCalibration, OK | > [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.000 | calibration full-budget completion |
| 127 | targeted | `reset_start` | startResetJob, IN_PROGRESS | > [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.000 | reset job start |
| 128 | targeted | `reset_step 0` | INVALID_PARAM | > [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | reset zero-budget rejection |
| 129 | targeted | `reset_step 1` | pollResetJob, IN_PROGRESS | [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | reset budget one |
| 130 | targeted | `reset_step 1` | pollResetJob | [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.016 | reset budget one repeated |
| 131 | targeted | `reset_step 2` | pollResetJob | > [I] pollResetJob(2): IN_PROGRESS / > [I] | PASS | 0.000 | reset budget two |
| 132 | targeted | `reset_step 16` | pollResetJob, OK | > [I] pollResetJob(16): OK / > [I] | PASS | 0.000 | reset completion budget |
| 133 | targeted | `trigger 1` | triggerConversion | > [I] triggerConversion(1): IN_PROGRESS / > [I] | PASS | 0.016 | trigger mode 1 |
| 134 | targeted | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 1 |
| 135 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     434 mV / Shunt:   9 uV / Temp:    30352 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | sample after trigger 1 |
| 136 | targeted | `trigger 2` | triggerConversion | > [I] triggerConversion(2): IN_PROGRESS / > [I] | PASS | 0.015 | trigger mode 2 |
| 137 | targeted | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 2 |
| 138 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     434 mV / Shunt:   8 uV / Temp:    30352 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | sample after trigger 2 |
| 139 | targeted | `trigger 3` | triggerConversion | > [I] triggerConversion(3): IN_PROGRESS / > [I] | PASS | 0.016 | trigger mode 3 |
| 140 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 3 |
| 141 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     449 mV / Shunt:   10 uV / Temp:    30352 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | sample after trigger 3 |
| 142 | targeted | `trigger 4` | triggerConversion | > [I] triggerConversion(4): IN_PROGRESS / > [I] | PASS | 0.016 | trigger mode 4 |
| 143 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 4 |
| 144 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     449 mV / Shunt:   10 uV / Temp:    30242 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | sample after trigger 4 |
| 145 | targeted | `trigger 5` | triggerConversion | > [I] triggerConversion(5): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 5 |
| 146 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 5 |
| 147 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     465 mV / Shunt:   10 uV / Temp:    30211 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | sample after trigger 5 |
| 148 | targeted | `trigger 6` | triggerConversion | > [I] triggerConversion(6): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 6 |
| 149 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 6 |
| 150 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     465 mV / Shunt:   10 uV / Temp:    30242 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | sample after trigger 6 |
| 151 | targeted | `trigger 7` | triggerConversion | > [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 7 |
| 152 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 7 |
| 153 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     476 mV / Shunt:   6 uV / Temp:    30219 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | sample after trigger 7 |
| 154 | targeted | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | restore continuous mode after triggers |
| 155 | targeted | `rstacc` | resetAccumulators, OK | [I] resetAccumulators(): OK / > [I] | PASS | 0.000 | reset accumulators |
| 156 | targeted | `energy` | Energy | > [I] Energy: 0.000000000 J / > [I] | PASS | 0.000 | read energy after accumulator reset |
| 157 | targeted | `charge` | Charge | > [I] Charge: 0.000057220 C / > [I] | PASS | 0.000 | read charge after accumulator reset |
| 158 | targeted | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | raw diagnostics destructive read |
| 159 | targeted | `diagsnap` | DIAG_ALRT Snapshot | > === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   90418 ms / MEMSTAT:   ... | PASS | 0.000 | cache-only diagnostic snapshot |
| 160 | targeted | `reg16 0x00` | 0x | >   Reg 0x00 = 0x0000 (0) / > [I] | PASS | 0.000 | read CONFIG raw16 |
| 161 | targeted | `reg16 0x01` | 0x | >   Reg 0x01 = 0xFB68 (64360) / > [I] | PASS | 0.000 | read ADC_CONFIG raw16 |
| 162 | targeted | `reg16 0x02` | 0x | >   Reg 0x02 = 0x0EA6 (3750) / > [I] | PASS | 0.000 | read SHUNT_CAL raw16 |
| 163 | targeted | `reg24 0x04` | 0x | >   Reg 0x04 = 0x000160 (352) / > [I] | PASS | 0.000 | read VSHUNT raw24 |
| 164 | targeted | `reg24 0x05` | 0x | >   Reg 0x05 = 0x009570 (38256) / > [I] | PASS | 0.000 | read VBUS raw24 |
| 165 | targeted | `reg24 0x06` | 0x | >   Reg 0x06 = 0x0F23FF (992255) / > [I] | PASS | 0.000 | read DIETEMP raw24 |
| 166 | targeted | `reg24 0x07` | 0x | >   Reg 0x07 = 0x0001C0 (448) / > [I] | PASS | 0.000 | read CURRENT raw24 |
| 167 | targeted | `reg24 0x08` | 0x | >   Reg 0x08 = 0x000003 (3) / > [I] | PASS | 0.000 | read POWER raw24 |
| 168 | targeted | `reg40 0x09` | 0x | Reg 0x09 = 0x0000000000 / > [I] | PASS | 0.000 | read ENERGY raw40 |
| 169 | targeted | `reg40 0x0A` | 0x | >   Reg 0x0A = 0x0000000015 / > [I] | PASS | 0.000 | read CHARGE raw40 |
| 170 | targeted | `mode -1` | Invalid mode | > [W] Invalid mode (0-15) / > [I] | PASS | 0.000 | reject invalid negative mode |
| 171 | targeted | `mode 16` | Invalid mode | > [W] Invalid mode (0-15) / > [I] | PASS | 0.000 | reject invalid high mode |
| 172 | targeted | `trigger 8` | Invalid trigger mode | > [W] Invalid trigger mode (0-7 for TRIG_* modes) / > [I] | PASS | 0.000 | reject invalid trigger mode |
| 173 | targeted | `convtime vbus 8` | Invalid conversion time | > [W] Invalid conversion time index (0-7) / > [I] | PASS | 0.000 | reject invalid conversion index |
| 174 | targeted | `convtime bogus 1` | Invalid target | > [W] Invalid target: bogus (use vbus\|vsh\|temp) / > [I] | PASS | 0.000 | reject invalid conversion target |
| 175 | targeted | `averaging 8` | Invalid averaging | > [W] Invalid averaging index (0-7) / > [I] | PASS | 0.000 | reject invalid averaging |
| 176 | targeted | `adcrange 2` | Invalid ADC range | > [W] Invalid ADC range (0 or 1) / > [I] | PASS | 0.000 | reject invalid ADC range |
| 177 | targeted | `delay 256` | Usage: delay | [W] Usage: delay <0..255> / > [I] | PASS | 0.015 | reject invalid conversion delay |
| 178 | targeted | `tempco 16384` | Usage: tempco | > [W] Usage: tempco <0..16383> / > [I] | PASS | 0.000 | reject invalid tempco |
| 179 | targeted | `tempcomp 2` | Usage: tempcomp | [W] Usage: tempcomp <0\|1> / > [I] | PASS | 0.000 | reject invalid temp compensation |
| 180 | targeted | `alatch 2` | Usage: alatch | [W] Usage: alatch <0\|1> / > [I] | PASS | 0.000 | reject invalid latch |
| 181 | targeted | `cnvralert 2` | Usage: cnvralert | > [W] Usage: cnvralert <0\|1> / > [I] | PASS | 0.015 | reject invalid conversion alert |
| 182 | targeted | `alslow 2` | Usage: alslow | > [W] Usage: alslow <0\|1> / > [I] | PASS | 0.000 | reject invalid slow alert |
| 183 | targeted | `apol 2` | Usage: apol | > [W] Usage: apol <0\|1> / > [I] | PASS | 0.000 | reject invalid alert polarity |
| 184 | targeted | `cal 0 10` | Usage: cal | > [W] Usage: cal <shunt_ohm> <max_current_a> / > [I] | PASS | 0.000 | reject zero shunt calibration |
| 185 | targeted | `reg16 0x100` | Usage: reg16 | > [W] Usage: reg16 <addr> / > [I] | PASS | 0.000 | reject invalid raw16 register |
| 186 | targeted | `reg24 0x100` | Usage: reg24 | > [W] Usage: reg24 <addr> / > [I] | PASS | 0.000 | reject invalid raw24 register |
| 187 | targeted | `reg40 0x100` | Usage: reg40 | [W] Usage: reg40 <addr> / > [I] | PASS | 0.000 | reject invalid raw40 register |
| 188 | targeted | `init 0x50` | Invalid address | > [W] Invalid address. Use init 0x40-0x4F / > [I] | PASS | 0.000 | reject invalid init address |
| 189 | targeted | `end` | Device shut down | [I] Device shut down. / > [I] | PASS | 0.000 | end driver |
| 190 | targeted | `vbus` | NOT_INITIALIZED | >   Status: NOT_INITIALIZED (code=1, detail=0) / Message: begin() not called / > [I] | PASS | 0.000 | read after end must fail visibly |
| 191 | targeted | `init 0x41` | begin, OK | > [I] begin(0x41): OK / > [I] | PASS | 0.000 | reinitialize known device |
| 192 | targeted | `recover` | Status: OK | [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Co... | PASS | 0.000 | manual recovery after reinit |
| 193 | targeted | `settings` | Active Settings, State:, READY | > === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ... | PASS | 0.000 | final settings |
| 194 | targeted | `drv` | Driver Health, State: READY, Consecutive failures: 0 | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 9 / Total failures: 0 / Success ra... | PASS | 0.000 | final health must be clean |
| 195 | targeted | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4502 V / Vshunt:  0.0000066 V / Temp:    30.33 C / Current: 0.000362 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | final aggregate read |
| 196 | soak | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     446 mV / Shunt:   5 uV / Temp:    30328 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | targeted soak sample after trigger |
| 197 | soak | `apply_step 0` | INVALID_PARAM | [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak apply zero-budget rejection |
| 198 | soak | `ready_step 0` | INVALID_PARAM | > [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak ready zero-budget rejection |
| 199 | soak | `trigger 7` | triggerConversion | > [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger all |
| 200 | soak | `apply_step 1` | pollApplyCalibration | > [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget one |
| 201 | soak | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget one |
| 202 | soak | `ready_step 2` | pollMeasurementReady | [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after all trigger |
| 203 | soak | `apply_step 2` | pollApplyCalibration | > [I] pollApplyCalibration(2): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget two |
| 204 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget two |
| 205 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     457 mV / Shunt:   9 uV / Temp:    30289 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample after all trigger |
| 206 | soak | `apply_step 6` | pollApplyCalibration, OK | > [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.015 | targeted soak apply completion |
| 207 | soak | `ready_step 255` | pollMeasurementReady | [I] pollMeasurementReady(255): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready max budget |
| 208 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode after trigger |
| 209 | soak | `reset_start` | startResetJob, IN_PROGRESS | > [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset start |
| 210 | soak | `sample_step 0` | INVALID_PARAM | > [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak sample zero-budget rejection |
| 211 | soak | `adcrange 1` | setAdcRange, OK | [I] setAdcRange(+/-40.96mV): OK / > [I] | PASS | 0.000 | targeted soak switch low range |
| 212 | soak | `reset_step 0` | INVALID_PARAM | [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak reset zero-budget rejection |
| 213 | soak | `sample_step 1` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget one |
| 214 | soak | `integer` | Integer Sample | > === Integer Sample === / Bus:     462 mV / Shunt:   8 uV / Temp:    30336 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | targeted soak integer sample low range |
| 215 | soak | `reset_step 1` | pollResetJob, IN_PROGRESS | [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.016 | targeted soak reset budget one |
| 216 | soak | `sample_step 2` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget two |
| 217 | soak | `adcrange 0` | setAdcRange, OK | > [I] setAdcRange(+/-163.84mV): OK / > [I] | PASS | 0.000 | targeted soak restore range |
| 218 | soak | `reset_step 1` | pollResetJob | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.015 | targeted soak reset budget one repeated |
| 219 | soak | `sample_step 3` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     453 mV / Shunt:   8 uV / Temp:    30414 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample budget three |
| 220 | soak | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | targeted soak raw diagnostics |
| 221 | soak | `reset_step 16` | pollResetJob, OK | > [I] pollResetJob(16): OK / > [I] | PASS | 0.016 | targeted soak reset completion |
| 222 | soak | `sample_step 4` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget four |
| 223 | soak | `diagsnap` | DIAG_ALRT Snapshot | > === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   128733 ms / MEMSTAT:  ... | PASS | 0.000 | targeted soak diagnostic snapshot |
| 224 | soak | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode |
| 225 | soak | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     455 mV / Shunt:   7 uV / Temp:    30438 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | targeted soak sample full budget |
| 226 | soak | `drv` | Driver Health, State: READY | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 2874 / Total failures: 0 / Success... | PASS | 0.000 | targeted soak health check |
| 227 | soak | `trigger 1` | triggerConversion | > [I] triggerConversion(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger bus |
| 228 | soak | `sample_step 255` | Power Sample Step Result | [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     453 mV / Shunt:   11 uV / Temp:    30453 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | targeted soak sample max budget |
| 229 | soak | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.015 | targeted soak manual recovery |
| 230 | soak | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after trigger |
| 231 | soak | `apply_start` | startApplyCalibration, IN_PROGRESS | > [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply start |
| 232 | soak | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4578 V / Vshunt:  0.0000075 V / Temp:    30.43 C / Current: 0.000496 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | targeted soak aggregate read |
| 233 | soak | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     451 mV / Shunt:   9 uV / Temp:    30445 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | targeted soak sample after trigger |
| 234 | soak | `apply_step 0` | INVALID_PARAM | > [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak apply zero-budget rejection |
| 235 | soak | `ready_step 0` | INVALID_PARAM | [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak ready zero-budget rejection |
| 236 | soak | `trigger 7` | triggerConversion | > [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger all |
| 237 | soak | `apply_step 1` | pollApplyCalibration | > [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget one |
| 238 | soak | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget one |
| 239 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after all trigger |
| 240 | soak | `apply_step 2` | pollApplyCalibration | > [I] pollApplyCalibration(2): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget two |
| 241 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget two |
| 242 | soak | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     458 mV / Shunt:   7 uV / Temp:    30383 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.015 | targeted soak sample after all trigger |
| 243 | soak | `apply_step 6` | pollApplyCalibration, OK | [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.000 | targeted soak apply completion |
| 244 | soak | `ready_step 255` | pollMeasurementReady | [I] pollMeasurementReady(255): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready max budget |
| 245 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode after trigger |
| 246 | soak | `reset_start` | startResetJob, IN_PROGRESS | [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset start |
| 247 | soak | `sample_step 0` | INVALID_PARAM | [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak sample zero-budget rejection |
| 248 | soak | `adcrange 1` | setAdcRange, OK | > [I] setAdcRange(+/-40.96mV): OK / > [I] | PASS | 0.016 | targeted soak switch low range |
| 249 | soak | `reset_step 0` | INVALID_PARAM | > [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak reset zero-budget rejection |
| 250 | soak | `sample_step 1` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget one |
| 251 | soak | `integer` | Integer Sample | === Integer Sample === / Bus:     462 mV / Shunt:   9 uV / Temp:    30414 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.016 | targeted soak integer sample low range |
| 252 | soak | `reset_step 1` | pollResetJob, IN_PROGRESS | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one |
| 253 | soak | `sample_step 2` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget two |
| 254 | soak | `adcrange 0` | setAdcRange, OK | > [I] setAdcRange(+/-163.84mV): OK / > [I] | PASS | 0.000 | targeted soak restore range |
| 255 | soak | `reset_step 1` | pollResetJob | [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one repeated |
| 256 | soak | `sample_step 3` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     454 mV / Shunt:   5 uV / Temp:    30453 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample budget three |
| 257 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | targeted soak raw diagnostics |
| 258 | soak | `reset_step 16` | pollResetJob, OK | > [I] pollResetJob(16): OK / > [I] | PASS | 0.000 | targeted soak reset completion |
| 259 | soak | `sample_step 4` | readPowerSampleRawStep | [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.016 | targeted soak sample budget four |
| 260 | soak | `diagsnap` | DIAG_ALRT Snapshot | > === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   188961 ms / MEMSTAT:  ... | PASS | 0.000 | targeted soak diagnostic snapshot |
| 261 | soak | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode |
| 262 | soak | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     452 mV / Shunt:   11 uV / Temp:    30469 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.000 | targeted soak sample full budget |
| 263 | soak | `drv` | Driver Health, State: READY | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 6294 / Total failures: 0 / Success... | PASS | 0.000 | targeted soak health check |
| 264 | soak | `trigger 1` | triggerConversion | > [I] triggerConversion(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger bus |
| 265 | soak | `sample_step 255` | Power Sample Step Result | > [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     453 mV / Shunt:   6 uV / Temp:    30461 mdegC / Current: 0 mA / Power:   ... | PASS | 0.000 | targeted soak sample max budget |
| 266 | soak | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.000 | targeted soak manual recovery |
| 267 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after trigger |
| 268 | soak | `apply_start` | startApplyCalibration, IN_PROGRESS | [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply start |
| 269 | soak | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4580 V / Vshunt:  0.0000084 V / Temp:    30.43 C / Current: 0.000553 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.000 | targeted soak aggregate read |
| 270 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     450 mV / Shunt:   8 uV / Temp:    30461 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample after trigger |
| 271 | soak | `apply_step 0` | INVALID_PARAM | > [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak apply zero-budget rejection |
| 272 | soak | `ready_step 0` | INVALID_PARAM | > [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak ready zero-budget rejection |
| 273 | soak | `trigger 7` | triggerConversion | [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger all |
| 274 | soak | `apply_step 1` | pollApplyCalibration | [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget one |
| 275 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget one |
| 276 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after all trigger |
| 277 | soak | `apply_step 2` | pollApplyCalibration | > [I] pollApplyCalibration(2): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget two |
| 278 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget two |
| 279 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     457 mV / Shunt:   6 uV / Temp:    30367 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.016 | targeted soak sample after all trigger |
| 280 | soak | `apply_step 6` | pollApplyCalibration, OK | > [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.000 | targeted soak apply completion |
| 281 | soak | `ready_step 255` | pollMeasurementReady | [I] pollMeasurementReady(255): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready max budget |
| 282 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode after trigger |
| 283 | soak | `reset_start` | startResetJob, IN_PROGRESS | [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset start |
| 284 | soak | `sample_step 0` | INVALID_PARAM | > [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak sample zero-budget rejection |
| 285 | soak | `adcrange 1` | setAdcRange, OK | [I] setAdcRange(+/-40.96mV): OK / > [I] | PASS | 0.000 | targeted soak switch low range |
| 286 | soak | `reset_step 0` | INVALID_PARAM | > [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak reset zero-budget rejection |
| 287 | soak | `sample_step 1` | readPowerSampleRawStep | [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget one |
| 288 | soak | `integer` | Integer Sample | > === Integer Sample === / Bus:     463 mV / Shunt:   5 uV / Temp:    30359 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | targeted soak integer sample low range |
| 289 | soak | `reset_step 1` | pollResetJob, IN_PROGRESS | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one |
| 290 | soak | `sample_step 2` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.016 | targeted soak sample budget two |
| 291 | soak | `adcrange 0` | setAdcRange, OK | > [I] setAdcRange(+/-163.84mV): OK / > [I] | PASS | 0.000 | targeted soak restore range |
| 292 | soak | `reset_step 1` | pollResetJob | [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one repeated |
| 293 | soak | `sample_step 3` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     454 mV / Shunt:   7 uV / Temp:    30438 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample budget three |
| 294 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | targeted soak raw diagnostics |
| 295 | soak | `reset_step 16` | pollResetJob, OK | [I] pollResetJob(16): OK / > [I] | PASS | 0.000 | targeted soak reset completion |
| 296 | soak | `sample_step 4` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.015 | targeted soak sample budget four |
| 297 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   237143 ms / MEMSTAT:    ... | PASS | 0.000 | targeted soak diagnostic snapshot |
| 298 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode |
| 299 | soak | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     451 mV / Shunt:   5 uV / Temp:    30430 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample full budget |
| 300 | soak | `drv` | Driver Health, State: READY | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 9715 / Total failures: 0 / Success r... | PASS | 0.000 | targeted soak health check |
| 301 | soak | `trigger 1` | triggerConversion | > [I] triggerConversion(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger bus |
| 302 | soak | `sample_step 255` | Power Sample Step Result | > [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     453 mV / Shunt:   8 uV / Temp:    30453 mdegC / Current: 1 mA / Power:   ... | PASS | 0.016 | targeted soak sample max budget |
| 303 | soak | `recover` | Status: OK | [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Co... | PASS | 0.000 | targeted soak manual recovery |
| 304 | soak | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after trigger |
| 305 | soak | `apply_start` | startApplyCalibration, IN_PROGRESS | [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply start |
| 306 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    0.4570 V / Vshunt:  0.0000094 V / Temp:    30.42 C / Current: 0.000610 A / Power:   0.000244 W / Energy:  0.00000000... | PASS | 0.000 | targeted soak aggregate read |
| 307 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     451 mV / Shunt:   8 uV / Temp:    30406 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample after trigger |
| 308 | soak | `apply_step 0` | INVALID_PARAM | [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak apply zero-budget rejection |
| 309 | soak | `ready_step 0` | INVALID_PARAM | [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak ready zero-budget rejection |
| 310 | soak | `trigger 7` | triggerConversion | > [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger all |
| 311 | soak | `apply_step 1` | pollApplyCalibration | [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.016 | targeted soak apply budget one |
| 312 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget one |
| 313 | soak | `ready_step 2` | pollMeasurementReady | [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after all trigger |
| 314 | soak | `apply_step 2` | pollApplyCalibration | > [I] pollApplyCalibration(2): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget two |
| 315 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget two |
| 316 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     460 mV / Shunt:   7 uV / Temp:    30352 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample after all trigger |
| 317 | soak | `apply_step 6` | pollApplyCalibration, OK | > [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.000 | targeted soak apply completion |
| 318 | soak | `ready_step 255` | pollMeasurementReady | [I] pollMeasurementReady(255): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready max budget |
| 319 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode after trigger |
| 320 | soak | `reset_start` | startResetJob, IN_PROGRESS | > [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.015 | targeted soak reset start |
| 321 | soak | `sample_step 0` | INVALID_PARAM | [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak sample zero-budget rejection |
| 322 | soak | `adcrange 1` | setAdcRange, OK | > [I] setAdcRange(+/-40.96mV): OK / > [I] | PASS | 0.000 | targeted soak switch low range |
| 323 | soak | `reset_step 0` | INVALID_PARAM | > [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak reset zero-budget rejection |
| 324 | soak | `sample_step 1` | readPowerSampleRawStep | [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget one |
| 325 | soak | `integer` | Integer Sample | > === Integer Sample === / Bus:     462 mV / Shunt:   8 uV / Temp:    30359 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | targeted soak integer sample low range |
| 326 | soak | `reset_step 1` | pollResetJob, IN_PROGRESS | [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one |
| 327 | soak | `sample_step 2` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget two |
| 328 | soak | `adcrange 0` | setAdcRange, OK | > [I] setAdcRange(+/-163.84mV): OK / > [I] | PASS | 0.000 | targeted soak restore range |
| 329 | soak | `reset_step 1` | pollResetJob | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one repeated |
| 330 | soak | `sample_step 3` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     453 mV / Shunt:   10 uV / Temp:    30367 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | targeted soak sample budget three |
| 331 | soak | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | targeted soak raw diagnostics |
| 332 | soak | `reset_step 16` | pollResetJob, OK | > [I] pollResetJob(16): OK / > [I] | PASS | 0.000 | targeted soak reset completion |
| 333 | soak | `sample_step 4` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget four |
| 334 | soak | `diagsnap` | DIAG_ALRT Snapshot | > === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   285319 ms / MEMSTAT:  ... | PASS | 0.000 | targeted soak diagnostic snapshot |
| 335 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode |
| 336 | soak | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     454 mV / Shunt:   10 uV / Temp:    30359 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | targeted soak sample full budget |
| 337 | soak | `drv` | Driver Health, State: READY | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 13138 / Total failures: 0 / Succes... | PASS | 0.000 | targeted soak health check |
| 338 | soak | `trigger 1` | triggerConversion | > [I] triggerConversion(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger bus |
| 339 | soak | `sample_step 255` | Power Sample Step Result | > [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     452 mV / Shunt:   9 uV / Temp:    30367 mdegC / Current: 1 mA / Power:   ... | PASS | 0.000 | targeted soak sample max budget |
| 340 | soak | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.000 | targeted soak manual recovery |
| 341 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.016 | targeted soak ready after trigger |
| 342 | soak | `apply_start` | startApplyCalibration, IN_PROGRESS | [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply start |
| 343 | soak | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4576 V / Vshunt:  0.0000106 V / Temp:    30.36 C / Current: 0.000706 A / Power:   0.000305 W / Energy:  0.000000... | PASS | 0.015 | targeted soak aggregate read |
| 344 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     453 mV / Shunt:   8 uV / Temp:    30406 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.015 | targeted soak sample after trigger |
| 345 | soak | `apply_step 0` | INVALID_PARAM | [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak apply zero-budget rejection |
| 346 | soak | `ready_step 0` | INVALID_PARAM | > [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak ready zero-budget rejection |
| 347 | soak | `trigger 7` | triggerConversion | [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger all |
| 348 | soak | `apply_step 1` | pollApplyCalibration | > [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget one |
| 349 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget one |
| 350 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after all trigger |
| 351 | soak | `apply_step 2` | pollApplyCalibration | > [I] pollApplyCalibration(2): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget two |
| 352 | soak | `ready_step 2` | pollMeasurementReady | [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget two |
| 353 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     458 mV / Shunt:   7 uV / Temp:    30367 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample after all trigger |
| 354 | soak | `apply_step 6` | pollApplyCalibration, OK | [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.000 | targeted soak apply completion |
| 355 | soak | `ready_step 255` | pollMeasurementReady | > [I] pollMeasurementReady(255): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready max budget |
| 356 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode after trigger |
| 357 | soak | `reset_start` | startResetJob, IN_PROGRESS | > [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset start |
| 358 | soak | `sample_step 0` | INVALID_PARAM | > [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak sample zero-budget rejection |
| 359 | soak | `adcrange 1` | setAdcRange, OK | > [I] setAdcRange(+/-40.96mV): OK / > [I] | PASS | 0.000 | targeted soak switch low range |
| 360 | soak | `reset_step 0` | INVALID_PARAM | > [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak reset zero-budget rejection |
| 361 | soak | `sample_step 1` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget one |
| 362 | soak | `integer` | Integer Sample | === Integer Sample === / Bus:     463 mV / Shunt:   4 uV / Temp:    30398 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power rea... | PASS | 0.000 | targeted soak integer sample low range |
| 363 | soak | `reset_step 1` | pollResetJob, IN_PROGRESS | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one |
| 364 | soak | `sample_step 2` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget two |
| 365 | soak | `adcrange 0` | setAdcRange, OK | > [I] setAdcRange(+/-163.84mV): OK / > [I] | PASS | 0.000 | targeted soak restore range |
| 366 | soak | `reset_step 1` | pollResetJob | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one repeated |
| 367 | soak | `sample_step 3` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     452 mV / Shunt:   5 uV / Temp:    30453 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample budget three |
| 368 | soak | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | targeted soak raw diagnostics |
| 369 | soak | `reset_step 16` | pollResetJob, OK | [I] pollResetJob(16): OK / > [I] | PASS | 0.000 | targeted soak reset completion |
| 370 | soak | `sample_step 4` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget four |
| 371 | soak | `diagsnap` | DIAG_ALRT Snapshot | > === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   333505 ms / MEMSTAT:  ... | PASS | 0.000 | targeted soak diagnostic snapshot |
| 372 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode |
| 373 | soak | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     454 mV / Shunt:   8 uV / Temp:    30438 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | targeted soak sample full budget |
| 374 | soak | `drv` | Driver Health, State: READY | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 16566 / Total failures: 0 / Succes... | PASS | 0.000 | targeted soak health check |
| 375 | soak | `trigger 1` | triggerConversion | > [I] triggerConversion(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger bus |
| 376 | soak | `sample_step 255` | Power Sample Step Result | > [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     454 mV / Shunt:   8 uV / Temp:    30445 mdegC / Current: 0 mA / Power:   ... | PASS | 0.000 | targeted soak sample max budget |
| 377 | soak | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.000 | targeted soak manual recovery |
| 378 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after trigger |
| 379 | soak | `apply_start` | startApplyCalibration, IN_PROGRESS | > [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply start |
| 380 | soak | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4592 V / Vshunt:  0.0000072 V / Temp:    30.38 C / Current: 0.000477 A / Power:   0.000183 W / Energy:  0.000000... | PASS | 0.000 | targeted soak aggregate read |
| 381 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     453 mV / Shunt:   11 uV / Temp:    30438 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | targeted soak sample after trigger |
| 382 | soak | `apply_step 0` | INVALID_PARAM | [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak apply zero-budget rejection |
| 383 | soak | `ready_step 0` | INVALID_PARAM | > [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak ready zero-budget rejection |
| 384 | soak | `trigger 7` | triggerConversion | > [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger all |
| 385 | soak | `apply_step 1` | pollApplyCalibration | [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget one |
| 386 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget one |
| 387 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after all trigger |
| 388 | soak | `apply_step 2` | pollApplyCalibration | > [I] pollApplyCalibration(2): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget two |
| 389 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget two |
| 390 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     459 mV / Shunt:   6 uV / Temp:    30313 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample after all trigger |
| 391 | soak | `apply_step 6` | pollApplyCalibration, OK | > [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.000 | targeted soak apply completion |
| 392 | soak | `ready_step 255` | pollMeasurementReady | [I] pollMeasurementReady(255): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready max budget |
| 393 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode after trigger |
| 394 | soak | `reset_start` | startResetJob, IN_PROGRESS | [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset start |
| 395 | soak | `sample_step 0` | INVALID_PARAM | [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak sample zero-budget rejection |
| 396 | soak | `adcrange 1` | setAdcRange, OK | > [I] setAdcRange(+/-40.96mV): OK / > [I] | PASS | 0.000 | targeted soak switch low range |
| 397 | soak | `reset_step 0` | INVALID_PARAM | > [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak reset zero-budget rejection |
| 398 | soak | `sample_step 1` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget one |
| 399 | soak | `integer` | Integer Sample | > === Integer Sample === / Bus:     462 mV / Shunt:   10 uV / Temp:    30336 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power ... | PASS | 0.000 | targeted soak integer sample low range |
| 400 | soak | `reset_step 1` | pollResetJob, IN_PROGRESS | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one |
| 401 | soak | `sample_step 2` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget two |
| 402 | soak | `adcrange 0` | setAdcRange, OK | > [I] setAdcRange(+/-163.84mV): OK / > [I] | PASS | 0.016 | targeted soak restore range |
| 403 | soak | `reset_step 1` | pollResetJob | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one repeated |
| 404 | soak | `sample_step 3` | readPowerSampleRawStep | [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     454 mV / Shunt:   5 uV / Temp:    30375 mdegC / Current: 0 mA / Power:   0 mW... | PASS | 0.000 | targeted soak sample budget three |
| 405 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | targeted soak raw diagnostics |
| 406 | soak | `reset_step 16` | pollResetJob, OK | > [I] pollResetJob(16): OK / > [I] | PASS | 0.000 | targeted soak reset completion |
| 407 | soak | `sample_step 4` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.015 | targeted soak sample budget four |
| 408 | soak | `diagsnap` | DIAG_ALRT Snapshot | > === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   381688 ms / MEMSTAT:  ... | PASS | 0.000 | targeted soak diagnostic snapshot |
| 409 | soak | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode |
| 410 | soak | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     453 mV / Shunt:   8 uV / Temp:    30391 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.015 | targeted soak sample full budget |
| 411 | soak | `drv` | Driver Health, State: READY | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 19991 / Total failures: 0 / Success ... | PASS | 0.000 | targeted soak health check |
| 412 | soak | `trigger 1` | triggerConversion | > [I] triggerConversion(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger bus |
| 413 | soak | `sample_step 255` | Power Sample Step Result | [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     451 mV / Shunt:   10 uV / Temp:    30422 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | targeted soak sample max budget |
| 414 | soak | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.000 | targeted soak manual recovery |
| 415 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after trigger |
| 416 | soak | `apply_start` | startApplyCalibration, IN_PROGRESS | > [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply start |
| 417 | soak | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4576 V / Vshunt:  0.0000106 V / Temp:    30.34 C / Current: 0.000706 A / Power:   0.000305 W / Energy:  0.000000... | PASS | 0.000 | targeted soak aggregate read |
| 418 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     450 mV / Shunt:   11 uV / Temp:    30391 mdegC / Current: 1 mA / Power:   0... | PASS | 0.015 | targeted soak sample after trigger |
| 419 | soak | `apply_step 0` | INVALID_PARAM | > [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak apply zero-budget rejection |
| 420 | soak | `ready_step 0` | INVALID_PARAM | > [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak ready zero-budget rejection |
| 421 | soak | `trigger 7` | triggerConversion | > [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger all |
| 422 | soak | `apply_step 1` | pollApplyCalibration | > [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget one |
| 423 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget one |
| 424 | soak | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after all trigger |
| 425 | soak | `apply_step 2` | pollApplyCalibration | > [I] pollApplyCalibration(2): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply budget two |
| 426 | soak | `ready_step 2` | pollMeasurementReady | [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready budget two |
| 427 | soak | `sample_step 5` | readPowerSampleRawStep | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     458 mV / Shunt:   12 uV / Temp:    30297 mdegC / Current: 1 mA / Power:   0 m... | PASS | 0.000 | targeted soak sample after all trigger |
| 428 | soak | `apply_step 6` | pollApplyCalibration, OK | > [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.016 | targeted soak apply completion |
| 429 | soak | `ready_step 255` | pollMeasurementReady | > [I] pollMeasurementReady(255): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready max budget |
| 430 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode after trigger |
| 431 | soak | `reset_start` | startResetJob, IN_PROGRESS | [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset start |
| 432 | soak | `sample_step 0` | INVALID_PARAM | > [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak sample zero-budget rejection |
| 433 | soak | `adcrange 1` | setAdcRange, OK | > [I] setAdcRange(+/-40.96mV): OK / > [I] | PASS | 0.000 | targeted soak switch low range |
| 434 | soak | `reset_step 0` | INVALID_PARAM | > [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak reset zero-budget rejection |
| 435 | soak | `sample_step 1` | readPowerSampleRawStep | [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget one |
| 436 | soak | `integer` | Integer Sample | === Integer Sample === / Bus:     463 mV / Shunt:   11 uV / Temp:    30320 mdegC / Current: 1 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power re... | PASS | 0.000 | targeted soak integer sample low range |
| 437 | soak | `reset_step 1` | pollResetJob, IN_PROGRESS | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one |
| 438 | soak | `sample_step 2` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget two |
| 439 | soak | `adcrange 0` | setAdcRange, OK | > [I] setAdcRange(+/-163.84mV): OK / > [I] | PASS | 0.000 | targeted soak restore range |
| 440 | soak | `reset_step 1` | pollResetJob | [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak reset budget one repeated |
| 441 | soak | `sample_step 3` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     456 mV / Shunt:   6 uV / Temp:    30391 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample budget three |
| 442 | soak | `diagraw` | DIAG_ALRT raw | > [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | targeted soak raw diagnostics |
| 443 | soak | `reset_step 16` | pollResetJob, OK | > [I] pollResetJob(16): OK / > [I] | PASS | 0.000 | targeted soak reset completion |
| 444 | soak | `sample_step 4` | readPowerSampleRawStep | [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | targeted soak sample budget four |
| 445 | soak | `diagsnap` | DIAG_ALRT Snapshot | > === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   429865 ms / MEMSTAT:  ... | PASS | 0.000 | targeted soak diagnostic snapshot |
| 446 | soak | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | targeted soak restore continuous mode |
| 447 | soak | `sample_step 5` | Power Sample Step Result | [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     455 mV / Shunt:   8 uV / Temp:    30438 mdegC / Current: 1 mA / Power:   0 mW... | PASS | 0.000 | targeted soak sample full budget |
| 448 | soak | `drv` | Driver Health, State: READY | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 23412 / Total failures: 0 / Succes... | PASS | 0.000 | targeted soak health check |
| 449 | soak | `trigger 1` | triggerConversion | > [I] triggerConversion(1): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak trigger bus |
| 450 | soak | `sample_step 255` | Power Sample Step Result | > [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     454 mV / Shunt:   8 uV / Temp:    30438 mdegC / Current: 1 mA / Power:   ... | PASS | 0.000 | targeted soak sample max budget |
| 451 | soak | `recover` | Status: OK | > [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / ... | PASS | 0.000 | targeted soak manual recovery |
| 452 | soak | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | targeted soak ready after trigger |
| 453 | soak | `apply_start` | startApplyCalibration, IN_PROGRESS | [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | targeted soak apply start |
| 454 | soak | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4602 V / Vshunt:  0.0000088 V / Temp:    30.42 C / Current: 0.000572 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.015 | targeted soak aggregate read |
| 455 | soak | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     451 mV / Shunt:   8 uV / Temp:    30445 mdegC / Current: 0 mA / Power:   0 ... | PASS | 0.000 | targeted soak sample after trigger |
| 456 | soak | `apply_step 0` | INVALID_PARAM | > [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak apply zero-budget rejection |
| 457 | soak | `ready_step 0` | INVALID_PARAM | > [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | targeted soak ready zero-budget rejection |
| 458 | soak | `trigger 7` | triggerConversion | [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.015 | targeted soak trigger all |
| 459 | soak | `sample_step 2` | readPowerSampleRawStep | > [I] readPowerSampleRawSte / [runner] recovered missing HILMARK HIL18778967340000000 with retry 1 | UNKNOWN | 12.031 | targeted soak sample budget two |
| 460 | not-run | `<fixture: disconnected target>` | safe absent-device fixture | requires safe disconnect or switched fixture | NOT RUN | 0.000 | requires safe disconnect or switched fixture |
| 461 | not-run | `<fixture: bus fault injection>` | safe fault-injection fixture | requires safe NACK/timeout/bus-error injection | NOT RUN | 0.000 | requires safe NACK/timeout/bus-error injection |
| 462 | not-run | `<fixture: alert pin capture>` | alert pin instrumentation | requires alert-pin wiring and safe threshold stimulus | NOT RUN | 0.000 | requires alert-pin wiring and safe threshold stimulus |
| 463 | not-run | `<fixture: MCU reset or power cycle>` | controlled reset/power fixture | requires explicit reset/power-cycle control | NOT RUN | 0.000 | requires explicit reset/power-cycle control |

## Soak Summary

- Requested duration: 600.0 s
- Executed soak commands: 6593
- Recorded soak rows: 264
- Soak PASS row storage stride: every 25 PASS row(s), plus all FAIL/UNKNOWN rows
- Soak verdict counts: PASS=6592, FAIL=0, UNKNOWN=1
- Soak latency min/mean/max: 0.000 / 0.005 / 12.047 s
- Command mix:
  - `adcrange 0`: 178
  - `adcrange 1`: 178
  - `apply_start`: 178
  - `apply_step 0`: 178
  - `apply_step 1`: 178
  - `apply_step 2`: 178
  - `apply_step 6`: 178
  - `diagraw`: 178
  - `diagsnap`: 178
  - `drv`: 178
  - `integer`: 178
  - `mode 15`: 356
  - `read`: 178
  - `ready_step 0`: 179
  - `ready_step 1`: 357
  - `ready_step 2`: 357
  - `ready_step 255`: 179
  - `recover`: 178
  - `reset_start`: 178
  - `reset_step 0`: 178
  - `reset_step 1`: 356
  - `reset_step 16`: 178
  - `sample_step 0`: 179
  - `sample_step 1`: 179
  - `sample_step 2`: 179
  - `sample_step 255`: 178
  - `sample_step 3`: 178
  - `sample_step 4`: 178
  - `sample_step 5`: 534
  - `trigger 1`: 178
  - `trigger 7`: 178
- Non-PASS soak command counts:
  - `sample_step 2`: 1

## Targeted Stress Analysis

- The run intentionally did not repeat the previous 8-hour soak. It used a 37-command rotating stress mix focused on staged APIs, active-job behavior, zero-budget rejection, small `maxInstructions` budgets, full-budget completion, trigger/readiness transitions, reset jobs, calibration jobs, ADC range switching, DIAG_ALRT reads, cache-only diagnostic snapshots, manual recovery, integer reads, and aggregate reads.
- The one-shot targeted suite before the soak covered the full exposed command surface for modes, conversion times, averaging, ADC range, conversion delay, temperature compensation, calibration, alert toggles and thresholds, raw register helpers, trigger modes, accumulator reset, invalid inputs, `end`, and reinitialization.
- No library or chip crash was observed. Recorded health checkpoints during the soak stayed `READY` with `Consecutive failures: 0` and `Total failures: 0`.
- The soak stopped early because `--stop-on-non-pass` and `--fail-on-unknown` were enabled. The only non-pass row was a truncated serial CLI response for `sample_step 2`: the transcript contains `readPowerSampleRawSte` and the runner then recovered the missing marker on retry.
- That UNKNOWN is evidence of host/serial CLI framing fragility under sustained high-rate command traffic. It is not direct evidence of an I2C failure: no `Status` failure was captured, no driver health failure was recorded, and the preceding health checkpoint reported `READY`.
- Manual post-stop recovery completed a pending `sample_step 5`, restored `mode 15`, ran `recover`, and confirmed `drv` showed `State: READY`, `Online: yes`, `Consecutive failures: 0`, `Total failures: 0`, and `Mode: CONT_ALL`.
- `IN_PROGRESS` from triggered modes and staged one/two-instruction polls is expected behavior and was treated as PASS when the command contract expected a pending active job.
- Post-completion staged calibration polling returned `BUSY` when no job was active. This is visible, bounded behavior and was treated as PASS for the explicit post-completion check.

## Weaknesses And Proposed Fixes

| Severity | Area | Evidence | Proposed fix | Regression coverage |
| --- | --- | --- | --- | --- |
| Medium | HIL CLI/runner framing | One truncated `sample_step 2` response after 6592 PASS soak commands; marker retry recovered but verdict was UNKNOWN. | Add an on-device batched stress command or a stronger framed protocol with sequence numbers and explicit command ACK/result delimiters. Keep per-command deadlines and stop-on-non-pass behavior. | Add parser self-tests for truncated frames and a bounded HIL run using the framed/batched path. |
| Low | Backend transfer visibility | The suite validates `maxInstructions` behavior through CLI-visible statuses but cannot directly count backend transfers. | Add optional example-only instrumentation counters around the injected transport, reported by a CLI command. Keep counters out of production core paths. | HIL check asserting expected transfer count ranges for `ready_step`, `sample_step`, `apply_step`, and `reset_step`. |
| Low | Fixture-limited fault coverage | No safe disconnected target, bus fault injector, alert-pin capture, or controlled power cycle was present. | Use a switched fixture or fault-injection harness before claiming NACK/timeout/bus-fault and reset/persistence HIL coverage. | Mark current tests NOT RUN until fixture evidence is available. |

## Limitations

- Hardware safety and fixture details must be filled in by the operator.
- This runner records serial CLI evidence only; external instruments must be logged separately.
- Staged `maxInstructions` coverage is limited to the example CLI commands; backend transfer counts need external instrumentation.
- The 600-second targeted soak did not complete because the runner stopped on the first UNKNOWN serial framing event.
