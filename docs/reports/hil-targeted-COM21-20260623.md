# INA228 HIL Validation Report

- Date/time: 2026-06-23T07:48:42.697549+02:00 to 2026-06-23T07:48:57.990631+02:00
- Elapsed: 15.3 s
- Port: COM21
- Baud: 115200
- Suite: targeted
- Soak requested: 0.0 s
- Operator: Codex
- Board/environment: ESP32S3_COM21 / esp32s3dev_Arduino
- Fixture: INA228_0x41_low_voltage_no_fault_injection
- Safety assumptions: benign_fixture_no_fault_stimulus
- OS: Windows-11-10.0.26200-SP0
- Python: 3.12.10
- HIL command: `tools\run_i2c_hil.py --port COM21 --baud 115200 --suite targeted --timeout-s 12 --idle-s 0.3 --boot-settle-s 1 --boot-capture-s 4 --prompt-token >  --empty-retries 1 --marker-retries 1 --command-pause-s 0.05 --include-not-run --report docs\reports\hil-targeted-COM21-20260623.md --transcript docs\reports\hil-targeted-COM21-20260623.log --operator Codex --board ESP32S3_COM21 --environment esp32s3dev_Arduino --fixture INA228_0x41_low_voltage_no_fault_injection --safety benign_fixture_no_fault_stimulus --notes targeted_boundary_and_staged_api_stress_no_long_soak --fail-on-unknown`
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

- Transcript: `docs/reports/hil-targeted-COM21-20260623.log`
- Notes: targeted_boundary_and_staged_api_stress_no_long_soak

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 195 | 0 | 0 | 5 |

## Timing Summary

- Commands executed: 200
- Commands recorded in detail: 200
- Soak commands executed: 0
- Soak rows recorded in detail: 0
- Recorded command latency min/mean/max: 0.000 / 0.002 / 0.125 s

- Maximum consecutive FAIL verdicts: 0

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | INA228 library version | === Version Info === / Example firmware build: Jun 22 2026 21:16:34 / INA228 library version: 2.0.0 / INA228 library full: 2.0.0 (5840497, 2026-06-22 21:16:3... | PASS | 0.000 | version |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | > [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / 00:                         -- -- -- -- -- -- -- -- / 10... | PASS | 0.125 | scan |
| 3 | smoke | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | probe |
| 4 | smoke | `settings` | Active Settings, State:, Address: | > === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ... | PASS | 0.000 | settings |
| 5 | smoke | `drv` | Driver Health, State:, Online: | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 1 / Total failures: 0 / Success ra... | PASS | 0.015 | health |
| 6 | smoke | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | DIAG_ALRT |
| 7 | smoke | `raw` | Raw Registers, Vbus, Temp | > === Raw Registers === / Vshunt: 21 (0x000015) / Vbus:   2201 (0x000899) / Temp:   3884 (0x0F2C) / Current:15 (0x00000F) / Power:  2 (0x000002) / Energy: 0 ... | PASS | 0.000 | conversion raw read |
| 8 | targeted | `verbose 0` | Verbose mode | > [I] Verbose mode: OFF / > [I] | PASS | 0.000 | reduce CLI chatter |
| 9 | targeted | `help` | mode [0..15], sample_step <budget>, limits | > / === INA228 CLI Help === / [W] Safety: this example does not make 85 V systems safe. Use qualified design practices, isolation where needed, fusing, creep... | PASS | 0.016 | targeted CLI surface check |
| 10 | targeted | `drv` | Driver Health, State:, Online: | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 10 / Total failures: 0 / Success rat... | PASS | 0.000 | initial health before mutation |
| 11 | targeted | `settings` | Active Settings, Mode:, ADC range: | > === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ... | PASS | 0.015 | initial settings snapshot |
| 12 | targeted | `mode 0` | setMode | > [I] setMode(0 = SHUTDOWN): OK / > [I] | PASS | 0.000 | set operating mode 0 |
| 13 | targeted | `mode 1` | setMode | > [I] setMode(1 = TRIG_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 1 |
| 14 | targeted | `mode 2` | setMode | > [I] setMode(2 = TRIG_SHUNT): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.016 | set operating mode 2 |
| 15 | targeted | `mode 3` | setMode | > [I] setMode(3 = TRIG_SHUNT_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 3 |
| 16 | targeted | `mode 4` | setMode | [I] setMode(4 = TRIG_TEMP): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 4 |
| 17 | targeted | `mode 5` | setMode | > [I] setMode(5 = TRIG_TEMP_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 5 |
| 18 | targeted | `mode 6` | setMode | [I] setMode(6 = TRIG_TEMP_SHUNT): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 6 |
| 19 | targeted | `mode 7` | setMode | > [I] setMode(7 = TRIG_ALL): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / > [I] | PASS | 0.000 | set operating mode 7 |
| 20 | targeted | `mode 8` | setMode | [I] setMode(8 = SHUTDOWN2): OK / > [I] | PASS | 0.000 | set operating mode 8 |
| 21 | targeted | `mode 9` | setMode | > [I] setMode(9 = CONT_BUS): OK / > [I] | PASS | 0.000 | set operating mode 9 |
| 22 | targeted | `mode 10` | setMode | > [I] setMode(10 = CONT_SHUNT): OK / > [I] | PASS | 0.000 | set operating mode 10 |
| 23 | targeted | `mode 11` | setMode | > [I] setMode(11 = CONT_SHUNT_BUS): OK / > [I] | PASS | 0.000 | set operating mode 11 |
| 24 | targeted | `mode 12` | setMode | [I] setMode(12 = CONT_TEMP): OK / > [I] | PASS | 0.000 | set operating mode 12 |
| 25 | targeted | `mode 13` | setMode | [I] setMode(13 = CONT_TEMP_BUS): OK / > [I] | PASS | 0.000 | set operating mode 13 |
| 26 | targeted | `mode 14` | setMode | > [I] setMode(14 = CONT_TEMP_SHUNT): OK / > [I] | PASS | 0.000 | set operating mode 14 |
| 27 | targeted | `mode 15` | setMode | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | set operating mode 15 |
| 28 | targeted | `mode` | Mode: | > Mode: CONT_ALL (15) / > [I] | PASS | 0.000 | query mode after full mode sweep |
| 29 | targeted | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | restore continuous-all mode |
| 30 | targeted | `convtime vbus 0` | setConvTime | [I] setConvTime(vbus, 50us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 0 |
| 31 | targeted | `convtime vbus 1` | setConvTime | > [I] setConvTime(vbus, 84us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 1 |
| 32 | targeted | `convtime vbus 2` | setConvTime | > [I] setConvTime(vbus, 150us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 2 |
| 33 | targeted | `convtime vbus 3` | setConvTime | > [I] setConvTime(vbus, 280us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 3 |
| 34 | targeted | `convtime vbus 4` | setConvTime | > [I] setConvTime(vbus, 540us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 4 |
| 35 | targeted | `convtime vbus 5` | setConvTime | [I] setConvTime(vbus, 1052us): OK / > [I] | PASS | 0.000 | set vbus conversion time index 5 |
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
| 46 | targeted | `convtime temp 0` | setConvTime | [I] setConvTime(temp, 50us): OK / > [I] | PASS | 0.016 | set temp conversion time index 0 |
| 47 | targeted | `convtime temp 1` | setConvTime | > [I] setConvTime(temp, 84us): OK / > [I] | PASS | 0.000 | set temp conversion time index 1 |
| 48 | targeted | `convtime temp 2` | setConvTime | > [I] setConvTime(temp, 150us): OK / > [I] | PASS | 0.000 | set temp conversion time index 2 |
| 49 | targeted | `convtime temp 3` | setConvTime | > [I] setConvTime(temp, 280us): OK / > [I] | PASS | 0.000 | set temp conversion time index 3 |
| 50 | targeted | `convtime temp 4` | setConvTime | [I] setConvTime(temp, 540us): OK / > [I] | PASS | 0.000 | set temp conversion time index 4 |
| 51 | targeted | `convtime temp 5` | setConvTime | > [I] setConvTime(temp, 1052us): OK / > [I] | PASS | 0.000 | set temp conversion time index 5 |
| 52 | targeted | `convtime temp 6` | setConvTime | [I] setConvTime(temp, 2074us): OK / > [I] | PASS | 0.015 | set temp conversion time index 6 |
| 53 | targeted | `convtime temp 7` | setConvTime | > [I] setConvTime(temp, 4120us): OK / > [I] | PASS | 0.000 | set temp conversion time index 7 |
| 54 | targeted | `convtime` | Conversion times | > Conversion times: VBUS=4120us  VSHUNT=4120us  TEMP=4120us / > [I] | PASS | 0.000 | query conversion times |
| 55 | targeted | `convtime vbus 5` | setConvTime, OK | > [I] setConvTime(vbus, 1052us): OK / > [I] | PASS | 0.000 | restore VBUS conversion time |
| 56 | targeted | `convtime vsh 5` | setConvTime, OK | > [I] setConvTime(vsh, 1052us): OK / > [I] | PASS | 0.000 | restore VSHUNT conversion time |
| 57 | targeted | `convtime temp 5` | setConvTime, OK | [I] setConvTime(temp, 1052us): OK / > [I] | PASS | 0.000 | restore TEMP conversion time |
| 58 | targeted | `averaging 0` | setAveraging | > [I] setAveraging(1): OK / > [I] | PASS | 0.000 | set averaging index 0 |
| 59 | targeted | `averaging 1` | setAveraging | > [I] setAveraging(4): OK / > [I] | PASS | 0.000 | set averaging index 1 |
| 60 | targeted | `averaging 2` | setAveraging | [I] setAveraging(16): OK / > [I] | PASS | 0.000 | set averaging index 2 |
| 61 | targeted | `averaging 3` | setAveraging | > [I] setAveraging(64): OK / > [I] | PASS | 0.000 | set averaging index 3 |
| 62 | targeted | `averaging 4` | setAveraging | [I] setAveraging(128): OK / > [I] | PASS | 0.000 | set averaging index 4 |
| 63 | targeted | `averaging 5` | setAveraging | > [I] setAveraging(256): OK / > [I] | PASS | 0.000 | set averaging index 5 |
| 64 | targeted | `averaging 6` | setAveraging | > [I] setAveraging(512): OK / > [I] | PASS | 0.000 | set averaging index 6 |
| 65 | targeted | `averaging 7` | setAveraging | > [I] setAveraging(1024): OK / > [I] | PASS | 0.000 | set averaging index 7 |
| 66 | targeted | `averaging` | Averaging: | > Averaging: 1024 samples / > [I] | PASS | 0.000 | query averaging |
| 67 | targeted | `averaging 0` | setAveraging, OK | > [I] setAveraging(1): OK / > [I] | PASS | 0.000 | restore averaging 1 |
| 68 | targeted | `adcrange 1` | setAdcRange, OK | > [I] setAdcRange(+/-40.96mV): OK / > [I] | PASS | 0.000 | switch low shunt range |
| 69 | targeted | `integer` | Integer Sample, Shunt: | > === Integer Sample === / Bus:     438 mV / Shunt:   7 uV / Temp:    30344 mdegC / Current: 0 mA / Power:   0 mW / DIAG_ALRT snapshot before current/power r... | PASS | 0.000 | integer sample after low range |
| 70 | targeted | `adcrange 0` | setAdcRange, OK | [I] setAdcRange(+/-163.84mV): OK / > [I] | PASS | 0.000 | restore default shunt range |
| 71 | targeted | `adcrange` | ADC range: | ADC range: +/-163.84mV / > [I] | PASS | 0.000 | query ADC range |
| 72 | targeted | `delay 0` | setConversionDelay, OK | > [I] setConversionDelay(0): OK / > [I] | PASS | 0.000 | conversion delay min |
| 73 | targeted | `delay 1` | setConversionDelay, OK | [I] setConversionDelay(1): OK / > [I] | PASS | 0.000 | conversion delay small |
| 74 | targeted | `delay 127` | setConversionDelay, OK | > [I] setConversionDelay(127): OK / > [I] | PASS | 0.000 | conversion delay middle |
| 75 | targeted | `delay 255` | setConversionDelay, OK | > [I] setConversionDelay(255): OK / > [I] | PASS | 0.000 | conversion delay max |
| 76 | targeted | `delay` | Conversion delay: | > [I] Conversion delay: 255 x 2 ms (510 ms) / > [I] | PASS | 0.000 | query conversion delay |
| 77 | targeted | `delay 0` | setConversionDelay, OK | > [I] setConversionDelay(0): OK / > [I] | PASS | 0.000 | restore zero conversion delay |
| 78 | targeted | `tempco 1` | setShuntTempCoeff, OK | > [I] setShuntTempCoeff(1): OK / > [I] | PASS | 0.000 | set small tempco |
| 79 | targeted | `tempco 16383` | setShuntTempCoeff, OK | > [I] setShuntTempCoeff(16383): OK / > [I] | PASS | 0.000 | set max tempco |
| 80 | targeted | `tempco` | Shunt temp coeff: | > [I] Shunt temp coeff: 16383 ppm/degC / > [I] | PASS | 0.000 | query tempco |
| 81 | targeted | `tempco 0` | setShuntTempCoeff, OK | [I] setShuntTempCoeff(0): OK / > [I] | PASS | 0.000 | restore tempco |
| 82 | targeted | `tempcomp 1` | setTempCompensation, OK | > [I] setTempCompensation(yes): OK / > [I] | PASS | 0.000 | enable temp compensation |
| 83 | targeted | `tempcomp` | Temperature compensation: | > [I] Temperature compensation: yes / > [I] | PASS | 0.000 | query temp compensation |
| 84 | targeted | `tempcomp 0` | setTempCompensation, OK | [I] setTempCompensation(no): OK / > [I] | PASS | 0.000 | restore temp compensation |
| 85 | targeted | `cal 0.015 10` | setCalibration, OK | > [I] setCalibration(0.015000, 10.000000): OK / > [I] | PASS | 0.000 | reapply nominal calibration |
| 86 | targeted | `cal` | Calibration:, CURRENT_LSB | > Calibration: Rshunt=0.015000 ohm  MaxCurrent=10.000000 A  CURRENT_LSB=0.000019073 A / > [I] | PASS | 0.000 | query calibration |
| 87 | targeted | `alatch 1` | setAlertLatch, OK | > [I] setAlertLatch(yes): OK / > [I] | PASS | 0.000 | enable alert latch |
| 88 | targeted | `alatch 0` | setAlertLatch, OK | > [I] setAlertLatch(no): OK / > [I] | PASS | 0.000 | restore alert latch |
| 89 | targeted | `cnvralert 1` | setConversionReadyAlert, OK | [I] setConversionReadyAlert(yes): OK / > [I] | PASS | 0.000 | enable conversion-ready alert bit |
| 90 | targeted | `cnvralert 0` | setConversionReadyAlert, OK | > [I] setConversionReadyAlert(no): OK / > [I] | PASS | 0.016 | restore conversion-ready alert bit |
| 91 | targeted | `alslow 1` | setSlowAlert, OK | > [I] setSlowAlert(yes): OK / > [I] | PASS | 0.000 | enable slow alert |
| 92 | targeted | `alslow 0` | setSlowAlert, OK | > [I] setSlowAlert(no): OK / > [I] | PASS | 0.000 | restore slow alert |
| 93 | targeted | `apol 1` | setAlertPolarity, OK | > [I] setAlertPolarity(yes): OK / > [I] | PASS | 0.000 | set active-high alert |
| 94 | targeted | `apol 0` | setAlertPolarity, OK | > [I] setAlertPolarity(no): OK / > [I] | PASS | 0.000 | restore active-low alert |
| 95 | targeted | `sovl 0.001` | setShuntOvervoltageThreshold, OK | > [I] setShuntOvervoltageThreshold(0.0010000): OK / > [I] | PASS | 0.000 | safe shunt overvoltage threshold |
| 96 | targeted | `suvl -0.001` | setShuntUndervoltageThreshold, OK | [I] setShuntUndervoltageThreshold(-0.0010000): OK / > [I] | PASS | 0.000 | safe shunt undervoltage threshold |
| 97 | targeted | `bovl 1.0` | setBusOvervoltageThreshold, OK | > [I] setBusOvervoltageThreshold(1.0000): OK / > [I] | PASS | 0.000 | safe bus overvoltage threshold |
| 98 | targeted | `buvl 0.1` | setBusUndervoltageThreshold, OK | > [I] setBusUndervoltageThreshold(0.1000): OK / > [I] | PASS | 0.000 | safe bus undervoltage threshold |
| 99 | targeted | `tmplim 100` | setTemperatureOverlimitThreshold, OK | [I] setTemperatureOverlimitThreshold(100.00): OK / > [I] | PASS | 0.016 | safe temperature threshold |
| 100 | targeted | `pwrlim 0.01` | setPowerOverlimitThreshold, OK | > [I] setPowerOverlimitThreshold(0.010000): OK / > [I] | PASS | 0.000 | safe power threshold |
| 101 | targeted | `limits` | Alert Limits | > === Alert Limits === / SOVL:      0x00C8  1.000 mV / SUVL:      0xFF38  -1.000 mV / BOVL:      0x0140  1.0000 V / BUVL:      0x0020  0.1000 V / TEMP_LIMIT:... | PASS | 0.000 | query alert limits |
| 102 | targeted | `sovl 0.163835` | setShuntOvervoltageThreshold, OK | > [I] setShuntOvervoltageThreshold(0.1638350): OK / > [I] | PASS | 0.000 | restore shunt overvoltage default |
| 103 | targeted | `suvl -0.16384` | setShuntUndervoltageThreshold, OK | > [I] setShuntUndervoltageThreshold(-0.1638400): OK / > [I] | PASS | 0.000 | restore shunt undervoltage default |
| 104 | targeted | `bovl 85` | setBusOvervoltageThreshold, OK | > [I] setBusOvervoltageThreshold(85.0000): OK / > [I] | PASS | 0.000 | restore safe max bus overvoltage |
| 105 | targeted | `buvl 0` | setBusUndervoltageThreshold, OK | > [I] setBusUndervoltageThreshold(0.0000): OK / > [I] | PASS | 0.000 | restore bus undervoltage default |
| 106 | targeted | `tmplim 255.99` | setTemperatureOverlimitThreshold, OK | > [I] setTemperatureOverlimitThreshold(255.99): OK / > [I] | PASS | 0.000 | restore safe max temperature threshold |
| 107 | targeted | `pwrlim 800` | setPowerOverlimitThreshold, OK | [I] setPowerOverlimitThreshold(800.000000): OK / > [I] | PASS | 0.000 | restore power threshold |
| 108 | targeted | `ready_step 0` | INVALID_PARAM | > [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | ready zero-budget rejection |
| 109 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready budget one |
| 110 | targeted | `ready_step 2` | pollMeasurementReady | > [I] pollMeasurementReady(2): OK ready=yes / > [I] | PASS | 0.000 | ready budget two |
| 111 | targeted | `ready_step 255` | pollMeasurementReady | > [I] pollMeasurementReady(255): OK ready=yes / > [I] | PASS | 0.000 | ready max budget |
| 112 | targeted | `sample_step 0` | INVALID_PARAM | > [I] readPowerSampleRawStep(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | sample zero-budget rejection |
| 113 | targeted | `sample_step 1` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(1): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | sample budget one |
| 114 | targeted | `sample_step 2` | readPowerSampleRawStep | [I] readPowerSampleRawStep(2): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | sample budget two |
| 115 | targeted | `sample_step 3` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(3): OK / === Power Sample Step Result === / Bus:     435 mV / Shunt:   7 uV / Temp:    30359 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.000 | sample budget three |
| 116 | targeted | `sample_step 4` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(4): IN_PROGRESS / Step result pending; outputs are not committed yet. / > [I] | PASS | 0.000 | sample budget four |
| 117 | targeted | `sample_step 5` | Power Sample Step Result | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     434 mV / Shunt:   13 uV / Temp:    30352 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | sample full budget |
| 118 | targeted | `sample_step 255` | Power Sample Step Result | > [I] readPowerSampleRawStep(255): OK / === Power Sample Step Result === / Bus:     433 mV / Shunt:   10 uV / Temp:    30359 mdegC / Current: 1 mA / Power:  ... | PASS | 0.000 | sample max budget |
| 119 | targeted | `apply_start` | startApplyCalibration, IN_PROGRESS | > [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | calibration job start |
| 120 | targeted | `apply_step 0` | INVALID_PARAM | > [I] pollApplyCalibration(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | calibration zero-budget rejection |
| 121 | targeted | `apply_step 1` | pollApplyCalibration | > [I] pollApplyCalibration(1): IN_PROGRESS / > [I] | PASS | 0.000 | calibration budget one |
| 122 | targeted | `apply_step 2` | pollApplyCalibration | > [I] pollApplyCalibration(2): IN_PROGRESS / > [I] | PASS | 0.000 | calibration budget two |
| 123 | targeted | `apply_step 3` | pollApplyCalibration | [I] pollApplyCalibration(3): OK / > [I] | PASS | 0.000 | calibration budget three |
| 124 | targeted | `apply_step 6` | BUSY | > [I] pollApplyCalibration(6): BUSY / Status: BUSY (code=11, detail=0) / Message: No apply calibration job active / > [I] | PASS | 0.000 | post-completion calibration poll reports BUSY |
| 125 | targeted | `apply_start` | startApplyCalibration, IN_PROGRESS | [I] startApplyCalibration(): IN_PROGRESS / > [I] | PASS | 0.000 | calibration full-budget restart |
| 126 | targeted | `apply_step 6` | pollApplyCalibration, OK | > [I] pollApplyCalibration(6): OK / > [I] | PASS | 0.000 | calibration full-budget completion |
| 127 | targeted | `reset_start` | startResetJob, IN_PROGRESS | > [I] startResetJob(): IN_PROGRESS / > [I] | PASS | 0.000 | reset job start |
| 128 | targeted | `reset_step 0` | INVALID_PARAM | > [I] pollResetJob(0): INVALID_PARAM / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / > [I] | PASS | 0.000 | reset zero-budget rejection |
| 129 | targeted | `reset_step 1` | pollResetJob, IN_PROGRESS | [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.015 | reset budget one |
| 130 | targeted | `reset_step 1` | pollResetJob | > [I] pollResetJob(1): IN_PROGRESS / > [I] | PASS | 0.000 | reset budget one repeated |
| 131 | targeted | `reset_step 2` | pollResetJob | > [I] pollResetJob(2): IN_PROGRESS / > [I] | PASS | 0.000 | reset budget two |
| 132 | targeted | `reset_step 16` | pollResetJob, OK | > [I] pollResetJob(16): OK / > [I] | PASS | 0.016 | reset completion budget |
| 133 | targeted | `trigger 1` | triggerConversion | > [I] triggerConversion(1): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 1 |
| 134 | targeted | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 1 |
| 135 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     437 mV / Shunt:   10 uV / Temp:    30344 mdegC / Current: 1 mA / Power:   0... | PASS | 0.016 | sample after trigger 1 |
| 136 | targeted | `trigger 2` | triggerConversion | > [I] triggerConversion(2): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 2 |
| 137 | targeted | `ready_step 1` | pollMeasurementReady | > [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 2 |
| 138 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     437 mV / Shunt:   10 uV / Temp:    30344 mdegC / Current: 1 mA / Power:   0... | PASS | 0.000 | sample after trigger 2 |
| 139 | targeted | `trigger 3` | triggerConversion | > [I] triggerConversion(3): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 3 |
| 140 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 3 |
| 141 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     452 mV / Shunt:   10 uV / Temp:    30344 mdegC / Current: 1 mA / Power:   0... | PASS | 0.015 | sample after trigger 3 |
| 142 | targeted | `trigger 4` | triggerConversion | > [I] triggerConversion(4): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 4 |
| 143 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 4 |
| 144 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     452 mV / Shunt:   10 uV / Temp:    30242 mdegC / Current: 1 mA / Power:   0... | PASS | 0.016 | sample after trigger 4 |
| 145 | targeted | `trigger 5` | triggerConversion | > [I] triggerConversion(5): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 5 |
| 146 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 5 |
| 147 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     463 mV / Shunt:   10 uV / Temp:    30242 mdegC / Current: 1 mA / Power:   0... | PASS | 0.016 | sample after trigger 5 |
| 148 | targeted | `trigger 6` | triggerConversion | > [I] triggerConversion(6): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 6 |
| 149 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 6 |
| 150 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     463 mV / Shunt:   10 uV / Temp:    30227 mdegC / Current: 1 mA / Power:   0... | PASS | 0.016 | sample after trigger 6 |
| 151 | targeted | `trigger 7` | triggerConversion | > [I] triggerConversion(7): IN_PROGRESS / > [I] | PASS | 0.000 | trigger mode 7 |
| 152 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / > [I] | PASS | 0.000 | ready poll after trigger 7 |
| 153 | targeted | `sample_step 5` | readPowerSampleRawStep | > [I] readPowerSampleRawStep(5): OK / === Power Sample Step Result === / Bus:     475 mV / Shunt:   9 uV / Temp:    30234 mdegC / Current: 1 mA / Power:   0 ... | PASS | 0.015 | sample after trigger 7 |
| 154 | targeted | `mode 15` | setMode, OK | > [I] setMode(15 = CONT_ALL): OK / > [I] | PASS | 0.000 | restore continuous mode after triggers |
| 155 | targeted | `rstacc` | resetAccumulators, OK | [I] resetAccumulators(): OK / > [I] | PASS | 0.000 | reset accumulators |
| 156 | targeted | `energy` | Energy | [I] Energy: 0.000000000 J / > [I] | PASS | 0.000 | read energy after accumulator reset |
| 157 | targeted | `charge` | Charge | > [I] Charge: 0.000038147 C / > [I] | PASS | 0.000 | read charge after accumulator reset |
| 158 | targeted | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / > [I] | PASS | 0.000 | raw diagnostics destructive read |
| 159 | targeted | `diagsnap` | DIAG_ALRT Snapshot | > === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   110269 ms / MEMSTAT:  ... | PASS | 0.000 | cache-only diagnostic snapshot |
| 160 | targeted | `reg16 0x00` | 0x | Reg 0x00 = 0x0000 (0) / > [I] | PASS | 0.000 | read CONFIG raw16 |
| 161 | targeted | `reg16 0x01` | 0x | Reg 0x01 = 0xFB68 (64360) / > [I] | PASS | 0.000 | read ADC_CONFIG raw16 |
| 162 | targeted | `reg16 0x02` | 0x | >   Reg 0x02 = 0x0EA6 (3750) / > [I] | PASS | 0.000 | read SHUNT_CAL raw16 |
| 163 | targeted | `reg24 0x04` | 0x | >   Reg 0x04 = 0x0001D0 (464) / > [I] | PASS | 0.000 | read VSHUNT raw24 |
| 164 | targeted | `reg24 0x05` | 0x | >   Reg 0x05 = 0x009640 (38464) / > [I] | PASS | 0.000 | read VBUS raw24 |
| 165 | targeted | `reg24 0x06` | 0x | >   Reg 0x06 = 0x0F23FF (992255) / > [I] | PASS | 0.000 | read DIETEMP raw24 |
| 166 | targeted | `reg24 0x07` | 0x | >   Reg 0x07 = 0x000220 (544) / > [I] | PASS | 0.000 | read CURRENT raw24 |
| 167 | targeted | `reg24 0x08` | 0x | >   Reg 0x08 = 0x000004 (4) / > [I] | PASS | 0.000 | read POWER raw24 |
| 168 | targeted | `reg40 0x09` | 0x | Reg 0x09 = 0x0000000000 / > [I] | PASS | 0.000 | read ENERGY raw40 |
| 169 | targeted | `reg40 0x0A` | 0x | >   Reg 0x0A = 0x0000000014 / > [I] | PASS | 0.000 | read CHARGE raw40 |
| 170 | targeted | `mode -1` | Invalid mode | > [W] Invalid mode (0-15) / > [I] | PASS | 0.000 | reject invalid negative mode |
| 171 | targeted | `mode 16` | Invalid mode | > [W] Invalid mode (0-15) / > [I] | PASS | 0.000 | reject invalid high mode |
| 172 | targeted | `trigger 8` | Invalid trigger mode | > [W] Invalid trigger mode (0-7 for TRIG_* modes) / > [I] | PASS | 0.000 | reject invalid trigger mode |
| 173 | targeted | `convtime vbus 8` | Invalid conversion time | > [W] Invalid conversion time index (0-7) / > [I] | PASS | 0.000 | reject invalid conversion index |
| 174 | targeted | `convtime bogus 1` | Invalid target | [W] Invalid target: bogus (use vbus\|vsh\|temp) / > [I] | PASS | 0.000 | reject invalid conversion target |
| 175 | targeted | `averaging 8` | Invalid averaging | > [W] Invalid averaging index (0-7) / > [I] | PASS | 0.000 | reject invalid averaging |
| 176 | targeted | `adcrange 2` | Invalid ADC range | [W] Invalid ADC range (0 or 1) / > [I] | PASS | 0.000 | reject invalid ADC range |
| 177 | targeted | `delay 256` | Usage: delay | [W] Usage: delay <0..255> / > [I] | PASS | 0.000 | reject invalid conversion delay |
| 178 | targeted | `tempco 16384` | Usage: tempco | > [W] Usage: tempco <0..16383> / > [I] | PASS | 0.000 | reject invalid tempco |
| 179 | targeted | `tempcomp 2` | Usage: tempcomp | > [W] Usage: tempcomp <0\|1> / > [I] | PASS | 0.000 | reject invalid temp compensation |
| 180 | targeted | `alatch 2` | Usage: alatch | [W] Usage: alatch <0\|1> / > [I] | PASS | 0.000 | reject invalid latch |
| 181 | targeted | `cnvralert 2` | Usage: cnvralert | > [W] Usage: cnvralert <0\|1> / > [I] | PASS | 0.000 | reject invalid conversion alert |
| 182 | targeted | `alslow 2` | Usage: alslow | > [W] Usage: alslow <0\|1> / > [I] | PASS | 0.000 | reject invalid slow alert |
| 183 | targeted | `apol 2` | Usage: apol | > [W] Usage: apol <0\|1> / > [I] | PASS | 0.000 | reject invalid alert polarity |
| 184 | targeted | `cal 0 10` | Usage: cal | > [W] Usage: cal <shunt_ohm> <max_current_a> / > [I] | PASS | 0.000 | reject zero shunt calibration |
| 185 | targeted | `reg16 0x100` | Usage: reg16 | > [W] Usage: reg16 <addr> / > [I] | PASS | 0.000 | reject invalid raw16 register |
| 186 | targeted | `reg24 0x100` | Usage: reg24 | [W] Usage: reg24 <addr> / > [I] | PASS | 0.000 | reject invalid raw24 register |
| 187 | targeted | `reg40 0x100` | Usage: reg40 | > [W] Usage: reg40 <addr> / > [I] | PASS | 0.000 | reject invalid raw40 register |
| 188 | targeted | `init 0x50` | Invalid address | > [W] Invalid address. Use init 0x40-0x4F / > [I] | PASS | 0.000 | reject invalid init address |
| 189 | targeted | `end` | Device shut down | > [I] Device shut down. / > [I] | PASS | 0.000 | end driver |
| 190 | targeted | `vbus` | NOT_INITIALIZED | >   Status: NOT_INITIALIZED (code=1, detail=0) / Message: begin() not called / > [I] | PASS | 0.000 | read after end must fail visibly |
| 191 | targeted | `init 0x41` | begin, OK | > [I] begin(0x41): OK / > [I] | PASS | 0.000 | reinitialize known device |
| 192 | targeted | `recover` | Status: OK | [I] Attempting recovery... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Co... | PASS | 0.000 | manual recovery after reinit |
| 193 | targeted | `settings` | Active Settings, State:, READY | > === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ... | PASS | 0.000 | final settings |
| 194 | targeted | `drv` | Driver Health, State: READY, Consecutive failures: 0 | > === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 9 / Total failures: 0 / Success ra... | PASS | 0.000 | final health must be clean |
| 195 | targeted | `read` | Vbus, Power | > [I] Reading all measurements: / Vbus:    0.4496 V / Vshunt:  0.0000100 V / Temp:    30.34 C / Current: 0.000610 A / Power:   0.000244 W / Energy:  0.000000... | PASS | 0.015 | final aggregate read |
| 196 | not-run | `<fixture: disconnected target>` | safe absent-device fixture | requires safe disconnect or switched fixture | NOT RUN | 0.000 | requires safe disconnect or switched fixture |
| 197 | not-run | `<fixture: bus fault injection>` | safe fault-injection fixture | requires safe NACK/timeout/bus-error injection | NOT RUN | 0.000 | requires safe NACK/timeout/bus-error injection |
| 198 | not-run | `<fixture: alert pin capture>` | alert pin instrumentation | requires alert-pin wiring and safe threshold stimulus | NOT RUN | 0.000 | requires alert-pin wiring and safe threshold stimulus |
| 199 | not-run | `<fixture: MCU reset or power cycle>` | controlled reset/power fixture | requires explicit reset/power-cycle control | NOT RUN | 0.000 | requires explicit reset/power-cycle control |
| 200 | not-run | `<8-hour soak>` | --soak-hours 8 | soak not requested for this run | NOT RUN | 0.000 | soak not requested for this run |

## Targeted Feature Sweep Analysis

- This smaller HIL pass targeted all chip/library features exposed by the example CLI without repeating the long soak.
- It covered all operating mode values, all exposed conversion-time indexes for bus/shunt/temperature, all averaging indexes, both ADC ranges, conversion-delay boundaries, temperature coefficient boundaries, temperature compensation enable/disable, calibration reapply/query, alert mode toggles, safe alert threshold writes/restores, staged readiness/sample/calibration/reset jobs, trigger modes, accumulator reset, raw 16/24/40-bit register reads, invalid input handling, `end`, reinitialization, final settings, final health, and final aggregate read.
- Result: 195 PASS, 0 FAIL, 0 UNKNOWN, and 5 NOT RUN due fixture limitations.
- Final health showed `State: READY`, `Consecutive failures: 0`, and `Total failures: 0`.
- Triggered mode commands returning `IN_PROGRESS` were expected because the conversion job had started. Zero-budget staged calls returning `INVALID_PARAM` and read-after-`end` returning `NOT_INITIALIZED` were also expected.

## Limitations

- Hardware safety and fixture details must be filled in by the operator.
- This runner records serial CLI evidence only; external instruments must be logged separately.
- Staged `maxInstructions` coverage is limited to the example CLI commands; backend transfer counts need external instrumentation.
- Soak test was not requested in this run.
