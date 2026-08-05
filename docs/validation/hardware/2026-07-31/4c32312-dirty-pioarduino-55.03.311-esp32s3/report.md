# INA228 HIL Validation Report

- Date/time: 2026-07-31T13:09:47.672362+02:00 to 2026-07-31T13:09:56.643834+02:00
- Elapsed: 9.0 s
- Port: COM4
- Baud: 115200
- Suite: exhaustive
- Soak requested: 0.0 s
- Operator: Codex
- Board/environment: ESP32-S3 QFN56 rev 0.1, 4 MB flash, 2 MB PSRAM, USB Serial/JTAG / PIOArduino 55.03.311; Arduino-ESP32 3.3.11; ESP-IDF 5.5.5; GCC 14.2.0; esptool 5.3.0
- Fixture: INA228 detected at 0x41; 15 mOhm shunt; low-voltage connected bench fixture
- Safety assumptions: Low-voltage bench run only; no high-voltage validation performed
- OS: Windows-11-10.0.26200-SP0
- Python: 3.12.10
- HIL command: `tools/run_i2c_hil.py --port COM4 --suite exhaustive --require-framed --fail-on-unknown --include-not-run --benchmark-count 100 --timeout-s 12 --report docs/validation/hardware/2026-07-31/4c32312-dirty-pioarduino-55.03.311-esp32s3/report.md --transcript docs/validation/hardware/2026-07-31/4c32312-dirty-pioarduino-55.03.311-esp32s3/transcript.txt --operator Codex --board ESP32-S3 QFN56 rev 0.1, 4 MB flash, 2 MB PSRAM, USB Serial/JTAG --environment PIOArduino 55.03.311; Arduino-ESP32 3.3.11; ESP-IDF 5.5.5; GCC 14.2.0; esptool 5.3.0 --fixture INA228 detected at 0x41; 15 mOhm shunt; low-voltage connected bench fixture --safety Low-voltage bench run only; no high-voltage validation performed --notes Physical board serial 24:58:7C:DB:DB:AC re-enumerated from requested COM3 to COM4 after USB boot reset; source tree intentionally dirty with migration changes under test. Low ADC range is correctly rejected by the bound 15 mOhm/10 A calibration profile and is covered by native range-vector tests, not this physical profile.`
- Branch: main
- Commit: 4c32312b32a59bf38192080c9043170d2a001d33
- Dirty status:

```text
M examples/01_basic_bringup_cli/main.cpp
 M platformio.ini
 M src/INA228.cpp
 M test/test_basic.cpp
 M tools/run_i2c_hil.py
?? docs/validation/hardware/
```

- Transcript: not retained; this report preserves the run metadata, summary,
  and command results.
- Notes: Physical board serial 24:58:7C:DB:DB:AC re-enumerated from requested COM3 to COM4 after USB boot reset; source tree intentionally dirty with migration changes under test. Low ADC range is correctly rejected by the bound 15 mOhm/10 A calibration profile and is covered by native range-vector tests, not this physical profile.

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 851 | 0 | 0 | 5 |

## Timing Summary

- Commands executed: 856
- Commands recorded in detail: 856
- Soak commands executed: 0
- Soak rows recorded in detail: 0
- Recorded command latency min/mean/max: 0.000 / 0.007 / 1.516 s

- Maximum consecutive FAIL verdicts: 0

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | Arduino-ESP32: 3.3.11, ESP-IDF: v5.5.5, INA228 library version | === Version Info === / Example firmware build: Jul 31 2026 13:06:21 / MCU: ESP32-S3 rev 1, flash 4194304 bytes, PSRAM ready (2097152 bytes) / Arduino-ESP32: ... | PASS | 0.000 | version and framework stack |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / 00:                         -- -- -- -- -- -- -- -- / 10: ... | PASS | 0.125 | scan |
| 3 | smoke | `init` | initialize, OK | [I] bind + initialize(0x41): OK / [runner] frame_status=OK frame_elapsed_ms=14 | PASS | 0.015 | initialize discovered INA228 |
| 4 | smoke | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | probe |
| 5 | smoke | `settings` | Active Settings, State:, Address: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | settings |
| 6 | smoke | `drv` | Driver Health, State:, Online: | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 0 / Total failures: 0 / Success rate... | PASS | 0.000 | health |
| 7 | smoke | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | DIAG_ALRT |
| 8 | smoke | `raw` | Raw Registers, Vbus, Temp | === Raw Registers === / Vshunt: 4007 (0x000FA7) / Vbus:   61388 (0x00EFCC) / Temp:   3968 (0x0F80) / Current:3947 (0x000F6B) / Power:  16392 (0x004008) / Ene... | PASS | 0.000 | conversion raw read |
| 9 | functional | `help` | INA228 CLI Help, read, raw | === INA228 CLI Help === / [W] Safety: this example does not make 85 V systems safe. Use qualified design practices, isolation where needed, fusing, creepage/... | PASS | 0.000 | help |
| 10 | functional | `scanina` | INA228 Address Probe, Healthy INA228 devices | === INA228 Address Probe (0x40-0x4F) === / Note: INA228 probes read DIAG_ALRT for MEMSTAT and can clear CNVRF/latched diagnostic evidence. / 0x40: device ID ... | PASS | 0.016 | INA scan |
| 11 | functional | `mfgid` | Manufacturer ID | [I] Manufacturer ID: 0x5449 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | manufacturer ID |
| 12 | functional | `devid` | Device ID | [I] Device ID: 0x2281 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | device ID |
| 13 | functional | `timing` | Conversion ready, Estimated conversion time | Conversion ready: YES / Estimated conversion time: 3156 us (4 ms) / CURRENT_LSB: 0.000019079 A / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | timing |
| 14 | functional | `vbus` | Vbus | [I] Vbus: 11.9906 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | bus voltage |
| 15 | functional | `vshunt` | Vshunt | [I] Vshunt: 0.0010884 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | shunt voltage |
| 16 | functional | `temp` | Temp | [I] Temp: 31.01 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | die temperature |
| 17 | functional | `current` | Current | [I] Current: 0.067691 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.015 | current |
| 18 | functional | `power` | Power | [I] Power: 0.811740 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | power |
| 19 | functional | `rstacc` | accumulator reset, OK | [I] cooperative accumulator reset: OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.000 | establish valid accumulator epoch |
| 20 | functional | `energy` | Energy | [I] Energy: 0.001953646 J / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | energy |
| 21 | functional | `charge` | Charge | [I] Charge: 0.000476964 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | charge |
| 22 | functional | `read` | Vbus, Power, Accum | [I] Reading all measurements: / Vbus:    11.9896 V / Vshunt:  0.0012181 V / Temp:    30.98 C / Current: 0.081198 A / Power:   0.973526 W / Energy:  0.0048841... | PASS | 0.000 | aggregate read |
| 23 | functional | `diag` | DIAG_ALRT Flags, MEMSTAT | === DIAG_ALRT Flags === / Note: this read is destructive/status-clearing for CNVRF and latched diagnostic evidence. / MEMSTAT:   yes / CNVRF:     yes / ALATC... | PASS | 0.000 | parsed diagnostics |
| 24 | functional | `limits` | Alert Limits | === Alert Limits === / SOVL:      0x7FFF  163.835 mV / SUVL:      0x8000  -163.840 mV / BOVL:      0x7FFF  102.3969 V / BUVL:      0x0000  0.0000 V / TEMP_LI... | PASS | 0.016 | alert limits |
| 25 | functional | `alatch` | Alert latch | [I] Alert latch: no / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | alert latch query |
| 26 | functional | `cnvralert` | Conversion-ready alert | [I] Conversion-ready alert: no / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | conversion alert query |
| 27 | functional | `alslow` | Slow alert | [I] Slow alert: no / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | slow alert query |
| 28 | functional | `apol` | Alert polarity | [I] Alert polarity: active-low / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | alert polarity query |
| 29 | functional | `mode` | Mode | Mode: CONT_ALL (15) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | mode query |
| 30 | functional | `convtime` | Conversion times | Conversion times: VBUS=1052us  VSHUNT=1052us  TEMP=1052us / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | conversion time query |
| 31 | functional | `averaging` | Averaging | Averaging: 1 samples / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | averaging query |
| 32 | functional | `adcrange` | ADC range | ADC range: +/-163.84mV / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | ADC range query |
| 33 | functional | `cal` | CURRENT_LSB | Calibration: Rshunt=0.000000 ohm  MaxCurrent=0.000000 A  CURRENT_LSB=0.000019079 A / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | calibration query |
| 34 | functional | `tempco` | Shunt temp coeff | [I] Shunt temp coeff: 0 ppm/degC / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | temperature coefficient query |
| 35 | functional | `tempcomp` | Temperature compensation | [I] Temperature compensation: no / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | temperature compensation query |
| 36 | functional | `delay` | Conversion delay | [I] Conversion delay: 0 x 2 ms (0 ms) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | conversion delay query |
| 37 | functional | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | readiness |
| 38 | functional | `reg16 0x3E` | 0x | Reg 0x3E = 0x5449 (21577) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | manufacturer register raw16 |
| 39 | functional | `reg16 0x3F` | 0x | Reg 0x3F = 0x2281 (8833) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | device register raw16 |
| 40 | functional | `reg24 0x05` | 0x | Reg 0x05 = 0x0EFD30 (982320) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | VBUS raw24 |
| 41 | functional | `reg40 0x09` | 0x | Reg 0x09 = 0x0000000023 / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | ENERGY raw40 |
| 42 | functional | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.015 | owner invalidation and verified reinitialization |
| 43 | functional | `selftest` | INA228 selftest | === INA228 selftest (diagnostic commands; reads DIAG_ALRT) === / Note: DIAG_ALRT reads can clear CNVRF and latched evidence. / [PASS] probe responds / [PASS]... | PASS | 0.031 | self-test |
| 44 | functional | `stress 1000` | Stress Summary, Errors: 0 | === Stress Summary === / Target: 1000 / Attempts: 1000 / Success: 1000 / Errors: 0 / Duration: 1522 ms / Rate: 657.03 samples/s / Vbus V:    min=11.9889 avg=... | PASS | 1.516 | 1000-sample measurement stress |
| 45 | functional | `stress_mix 1000` | stress_mix summary, fail=0 | === stress_mix summary === / Total: ok=1000 fail=0 (100.00%) / Duration: 607 ms / Rate: 1647.45 ops/s / measure    ok=143 fail=0 / vbus       ok=143 fail=0 /... | PASS | 0.609 | 1000-operation mixed API stress |
| 46 | targeted | `verbose 0` | Verbose mode | [I] Verbose mode: OFF / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reduce CLI chatter |
| 47 | targeted | `help` | mode [0..15], sample_step <budget>, limits | === INA228 CLI Help === / [W] Safety: this example does not make 85 V systems safe. Use qualified design practices, isolation where needed, fusing, creepage/... | PASS | 0.016 | targeted CLI surface check |
| 48 | targeted | `drv` | Driver Health, State:, Online: | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 9925 / Total failures: 0 / Success r... | PASS | 0.000 | initial health before mutation |
| 49 | targeted | `settings` | Active Settings, Mode:, ADC range: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | initial settings snapshot |
| 50 | targeted | `mode 0` | setMode | [I] setMode(0 = SHUTDOWN): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set operating mode 0 |
| 51 | targeted | `mode 1` | setMode | [I] setMode(1 = TRIG_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame_elap... | PASS | 0.016 | set operating mode 1 |
| 52 | targeted | `mode 2` | setMode | [I] setMode(2 = TRIG_SHUNT): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame_el... | PASS | 0.000 | set operating mode 2 |
| 53 | targeted | `mode 3` | setMode | [I] setMode(3 = TRIG_SHUNT_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS fram... | PASS | 0.000 | set operating mode 3 |
| 54 | targeted | `mode 4` | setMode | [I] setMode(4 = TRIG_TEMP): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame_ela... | PASS | 0.000 | set operating mode 4 |
| 55 | targeted | `mode 5` | setMode | [I] setMode(5 = TRIG_TEMP_BUS): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame... | PASS | 0.000 | set operating mode 5 |
| 56 | targeted | `mode 6` | setMode | [I] setMode(6 = TRIG_TEMP_SHUNT): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS fra... | PASS | 0.000 | set operating mode 6 |
| 57 | targeted | `mode 7` | setMode | [I] setMode(7 = TRIG_ALL): IN_PROGRESS / Status: IN_PROGRESS (code=12, detail=0) / Message: Conversion started / [runner] frame_status=IN_PROGRESS frame_elap... | PASS | 0.000 | set operating mode 7 |
| 58 | targeted | `mode 8` | setMode | [I] setMode(8 = SHUTDOWN2): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set operating mode 8 |
| 59 | targeted | `mode 9` | setMode | [I] setMode(9 = CONT_BUS): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set operating mode 9 |
| 60 | targeted | `mode 10` | setMode | [I] setMode(10 = CONT_SHUNT): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | set operating mode 10 |
| 61 | targeted | `mode 11` | setMode | [I] setMode(11 = CONT_SHUNT_BUS): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set operating mode 11 |
| 62 | targeted | `mode 12` | setMode | [I] setMode(12 = CONT_TEMP): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set operating mode 12 |
| 63 | targeted | `mode 13` | setMode | [I] setMode(13 = CONT_TEMP_BUS): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set operating mode 13 |
| 64 | targeted | `mode 14` | setMode | [I] setMode(14 = CONT_TEMP_SHUNT): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set operating mode 14 |
| 65 | targeted | `mode 15` | setMode | [I] setMode(15 = CONT_ALL): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set operating mode 15 |
| 66 | targeted | `mode` | Mode: | Mode: CONT_ALL (15) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | query mode after full mode sweep |
| 67 | targeted | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | restore continuous-all mode |
| 68 | targeted | `convtime vbus 0` | setConvTime | [I] setConvTime(vbus, 50us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | set vbus conversion time index 0 |
| 69 | targeted | `convtime vbus 1` | setConvTime | [I] setConvTime(vbus, 84us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vbus conversion time index 1 |
| 70 | targeted | `convtime vbus 2` | setConvTime | [I] setConvTime(vbus, 150us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vbus conversion time index 2 |
| 71 | targeted | `convtime vbus 3` | setConvTime | [I] setConvTime(vbus, 280us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vbus conversion time index 3 |
| 72 | targeted | `convtime vbus 4` | setConvTime | [I] setConvTime(vbus, 540us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vbus conversion time index 4 |
| 73 | targeted | `convtime vbus 5` | setConvTime | [I] setConvTime(vbus, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vbus conversion time index 5 |
| 74 | targeted | `convtime vbus 6` | setConvTime | [I] setConvTime(vbus, 2074us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vbus conversion time index 6 |
| 75 | targeted | `convtime vbus 7` | setConvTime | [I] setConvTime(vbus, 4120us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vbus conversion time index 7 |
| 76 | targeted | `convtime vsh 0` | setConvTime | [I] setConvTime(vsh, 50us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | set vsh conversion time index 0 |
| 77 | targeted | `convtime vsh 1` | setConvTime | [I] setConvTime(vsh, 84us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vsh conversion time index 1 |
| 78 | targeted | `convtime vsh 2` | setConvTime | [I] setConvTime(vsh, 150us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vsh conversion time index 2 |
| 79 | targeted | `convtime vsh 3` | setConvTime | [I] setConvTime(vsh, 280us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vsh conversion time index 3 |
| 80 | targeted | `convtime vsh 4` | setConvTime | [I] setConvTime(vsh, 540us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vsh conversion time index 4 |
| 81 | targeted | `convtime vsh 5` | setConvTime | [I] setConvTime(vsh, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vsh conversion time index 5 |
| 82 | targeted | `convtime vsh 6` | setConvTime | [I] setConvTime(vsh, 2074us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vsh conversion time index 6 |
| 83 | targeted | `convtime vsh 7` | setConvTime | [I] setConvTime(vsh, 4120us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set vsh conversion time index 7 |
| 84 | targeted | `convtime temp 0` | setConvTime | [I] setConvTime(temp, 50us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set temp conversion time index 0 |
| 85 | targeted | `convtime temp 1` | setConvTime | [I] setConvTime(temp, 84us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | set temp conversion time index 1 |
| 86 | targeted | `convtime temp 2` | setConvTime | [I] setConvTime(temp, 150us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set temp conversion time index 2 |
| 87 | targeted | `convtime temp 3` | setConvTime | [I] setConvTime(temp, 280us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set temp conversion time index 3 |
| 88 | targeted | `convtime temp 4` | setConvTime | [I] setConvTime(temp, 540us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set temp conversion time index 4 |
| 89 | targeted | `convtime temp 5` | setConvTime | [I] setConvTime(temp, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set temp conversion time index 5 |
| 90 | targeted | `convtime temp 6` | setConvTime | [I] setConvTime(temp, 2074us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set temp conversion time index 6 |
| 91 | targeted | `convtime temp 7` | setConvTime | [I] setConvTime(temp, 4120us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set temp conversion time index 7 |
| 92 | targeted | `convtime` | Conversion times | Conversion times: VBUS=4120us  VSHUNT=4120us  TEMP=4120us / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | query conversion times |
| 93 | targeted | `convtime vbus 5` | setConvTime, OK | [I] setConvTime(vbus, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore VBUS conversion time |
| 94 | targeted | `convtime vsh 5` | setConvTime, OK | [I] setConvTime(vsh, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | restore VSHUNT conversion time |
| 95 | targeted | `convtime temp 5` | setConvTime, OK | [I] setConvTime(temp, 1052us): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore TEMP conversion time |
| 96 | targeted | `averaging 0` | setAveraging | [I] setAveraging(1): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set averaging index 0 |
| 97 | targeted | `averaging 1` | setAveraging | [I] setAveraging(4): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set averaging index 1 |
| 98 | targeted | `averaging 2` | setAveraging | [I] setAveraging(16): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set averaging index 2 |
| 99 | targeted | `averaging 3` | setAveraging | [I] setAveraging(64): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set averaging index 3 |
| 100 | targeted | `averaging 4` | setAveraging | [I] setAveraging(128): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set averaging index 4 |
| 101 | targeted | `averaging 5` | setAveraging | [I] setAveraging(256): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set averaging index 5 |
| 102 | targeted | `averaging 6` | setAveraging | [I] setAveraging(512): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | set averaging index 6 |
| 103 | targeted | `averaging 7` | setAveraging | [I] setAveraging(1024): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set averaging index 7 |
| 104 | targeted | `averaging` | Averaging: | Averaging: 1024 samples / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | query averaging |
| 105 | targeted | `averaging 0` | setAveraging, OK | [I] setAveraging(1): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore averaging 1 |
| 106 | targeted | `adcrange 1` | setAdcRange, INVALID_CONFIG | [I] setAdcRange(+/-40.96mV): INVALID_CONFIG / Status: INVALID_CONFIG (code=2, detail=0) / Message: Maximum current exceeds shunt range / [runner] frame_statu... | PASS | 0.000 | reject low range incompatible with the bound 10 A profile |
| 107 | targeted | `integer` | Cooperative Instantaneous Sample, Shunt: | === Cooperative Instantaneous Sample === / Operation: 243 request: 243 generation: 3 / Bus:     11993 mV / Shunt:   1019 uV / Temp:    31000 mdegC / Current:... | PASS | 0.016 | sample after rejected range mutation |
| 108 | targeted | `adcrange 0` | setAdcRange, OK | [I] setAdcRange(+/-163.84mV): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | restore default shunt range |
| 109 | targeted | `adcrange` | ADC range: | ADC range: +/-163.84mV / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | query ADC range |
| 110 | targeted | `delay 0` | setConversionDelay, OK | [I] setConversionDelay(0): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | conversion delay min |
| 111 | targeted | `delay 1` | setConversionDelay, OK | [I] setConversionDelay(1): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | conversion delay small |
| 112 | targeted | `delay 127` | setConversionDelay, OK | [I] setConversionDelay(127): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | conversion delay middle |
| 113 | targeted | `delay 255` | setConversionDelay, OK | [I] setConversionDelay(255): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | conversion delay max |
| 114 | targeted | `delay` | Conversion delay: | [I] Conversion delay: 255 x 2 ms (510 ms) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | query conversion delay |
| 115 | targeted | `delay 0` | setConversionDelay, OK | [I] setConversionDelay(0): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore zero conversion delay |
| 116 | targeted | `tempco 1` | setShuntTempCoeff, OK | [I] setShuntTempCoeff(1): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set small tempco |
| 117 | targeted | `tempco 16383` | setShuntTempCoeff, OK | [I] setShuntTempCoeff(16383): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set max tempco |
| 118 | targeted | `tempco` | Shunt temp coeff: | [I] Shunt temp coeff: 16383 ppm/degC / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | query tempco |
| 119 | targeted | `tempco 0` | setShuntTempCoeff, OK | [I] setShuntTempCoeff(0): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore tempco |
| 120 | targeted | `tempcomp 1` | setTempCompensation, OK | [I] setTempCompensation(yes): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | enable temp compensation |
| 121 | targeted | `tempcomp` | Temperature compensation: | [I] Temperature compensation: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | query temp compensation |
| 122 | targeted | `tempcomp 0` | setTempCompensation, OK | [I] setTempCompensation(no): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | restore temp compensation |
| 123 | targeted | `cal 0.015 10` | setCalibration, INVALID_CONFIG | [I] setCalibration(0.015000, 10.000000): INVALID_CONFIG / Status: INVALID_CONFIG (code=2, detail=0) / Message: Rebind to change a fixed-unit calibration cont... | PASS | 0.000 | reject mutation of the bound fixed-unit calibration contract |
| 124 | targeted | `cal` | Calibration:, CURRENT_LSB | Calibration: Rshunt=0.000000 ohm  MaxCurrent=0.000000 A  CURRENT_LSB=0.000019079 A / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | query calibration |
| 125 | targeted | `alatch 1` | setAlertLatch, OK | [I] setAlertLatch(yes): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | enable alert latch |
| 126 | targeted | `alatch 0` | setAlertLatch, OK | [I] setAlertLatch(no): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore alert latch |
| 127 | targeted | `cnvralert 1` | setConversionReadyAlert, OK | [I] setConversionReadyAlert(yes): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | enable conversion-ready alert bit |
| 128 | targeted | `cnvralert 0` | setConversionReadyAlert, OK | [I] setConversionReadyAlert(no): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore conversion-ready alert bit |
| 129 | targeted | `alslow 1` | setSlowAlert, OK | [I] setSlowAlert(yes): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | enable slow alert |
| 130 | targeted | `alslow 0` | setSlowAlert, OK | [I] setSlowAlert(no): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | restore slow alert |
| 131 | targeted | `apol 1` | setAlertPolarity, OK | [I] setAlertPolarity(yes): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | set active-high alert |
| 132 | targeted | `apol 0` | setAlertPolarity, OK | [I] setAlertPolarity(no): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore active-low alert |
| 133 | targeted | `sovl 0.001` | setShuntOvervoltageThreshold, OK | [I] setShuntOvervoltageThreshold(0.0010000): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | safe shunt overvoltage threshold |
| 134 | targeted | `suvl -0.001` | setShuntUndervoltageThreshold, OK | [I] setShuntUndervoltageThreshold(-0.0010000): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | safe shunt undervoltage threshold |
| 135 | targeted | `bovl 1.0` | setBusOvervoltageThreshold, OK | [I] setBusOvervoltageThreshold(1.0000): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | safe bus overvoltage threshold |
| 136 | targeted | `buvl 0.1` | setBusUndervoltageThreshold, OK | [I] setBusUndervoltageThreshold(0.1000): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | safe bus undervoltage threshold |
| 137 | targeted | `tmplim 100` | setTemperatureOverlimitThreshold, OK | [I] setTemperatureOverlimitThreshold(100.00): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | safe temperature threshold |
| 138 | targeted | `pwrlim 0.01` | setPowerOverlimitThreshold, OK | [I] setPowerOverlimitThreshold(0.010000): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | safe power threshold |
| 139 | targeted | `limits` | Alert Limits | === Alert Limits === / SOVL:      0x00C8  1.000 mV / SUVL:      0xFF38  -1.000 mV / BOVL:      0x0140  1.0000 V / BUVL:      0x0020  0.1000 V / TEMP_LIMIT:0x... | PASS | 0.000 | query alert limits |
| 140 | targeted | `sovl 0.163835` | setShuntOvervoltageThreshold, OK | [I] setShuntOvervoltageThreshold(0.1638350): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | restore shunt overvoltage default |
| 141 | targeted | `suvl -0.16384` | setShuntUndervoltageThreshold, OK | [I] setShuntUndervoltageThreshold(-0.1638400): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | restore shunt undervoltage default |
| 142 | targeted | `bovl 85` | setBusOvervoltageThreshold, OK | [I] setBusOvervoltageThreshold(85.0000): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | restore safe max bus overvoltage |
| 143 | targeted | `buvl 0` | setBusUndervoltageThreshold, OK | [I] setBusUndervoltageThreshold(0.0000): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | restore bus undervoltage default |
| 144 | targeted | `tmplim 255.99` | setTemperatureOverlimitThreshold, OK | [I] setTemperatureOverlimitThreshold(255.99): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.015 | restore safe max temperature threshold |
| 145 | targeted | `pwrlim 800` | setPowerOverlimitThreshold, OK | [I] setPowerOverlimitThreshold(800.000000): OK / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | restore power threshold |
| 146 | targeted | `ready_step 0` | INVALID_PARAM | [I] pollMeasurementReady(0): INVALID_PARAM ready=no / Status: INVALID_PARAM (code=5, detail=0) / Message: maxInstructions must be > 0 / [runner] frame_status... | PASS | 0.000 | ready zero-budget rejection |
| 147 | targeted | `ready_step 1` | pollMeasurementReady | [I] pollMeasurementReady(1): OK ready=yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | ready single-transfer budget |
| 148 | targeted | `trigger 1` | triggerConversion | [I] triggerConversion(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | trigger mode 1 |
| 149 | targeted | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | ready poll after trigger 1 |
| 150 | targeted | `trigger 2` | triggerConversion | [I] triggerConversion(2): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | trigger mode 2 |
| 151 | targeted | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | ready poll after trigger 2 |
| 152 | targeted | `trigger 3` | triggerConversion | [I] triggerConversion(3): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | trigger mode 3 |
| 153 | targeted | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | ready poll after trigger 3 |
| 154 | targeted | `trigger 4` | triggerConversion | [I] triggerConversion(4): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | trigger mode 4 |
| 155 | targeted | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | ready poll after trigger 4 |
| 156 | targeted | `trigger 5` | triggerConversion | [I] triggerConversion(5): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | trigger mode 5 |
| 157 | targeted | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | ready poll after trigger 5 |
| 158 | targeted | `trigger 6` | triggerConversion | [I] triggerConversion(6): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | trigger mode 6 |
| 159 | targeted | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | ready poll after trigger 6 |
| 160 | targeted | `trigger 7` | triggerConversion | [I] triggerConversion(7): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | trigger mode 7 |
| 161 | targeted | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | ready poll after trigger 7 |
| 162 | targeted | `mode 15` | setMode, OK | [I] setMode(15 = CONT_ALL): OK / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | restore continuous mode after triggers |
| 163 | targeted | `rstacc` | accumulator reset, OK | [I] cooperative accumulator reset: OK / [runner] frame_status=OK frame_elapsed_ms=2 | PASS | 0.000 | reset accumulators |
| 164 | targeted | `energy` | Energy | [I] Energy: 0.001953646 J / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | read energy after accumulator reset |
| 165 | targeted | `charge` | Charge | [I] Charge: 0.000476964 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | read charge after accumulator reset |
| 166 | targeted | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0001 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | raw diagnostics destructive read |
| 167 | targeted | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   195875 ms / MEMSTAT:    ... | PASS | 0.000 | cache-only diagnostic snapshot |
| 168 | targeted | `reg16 0x00` | 0x | Reg 0x00 = 0x0000 (0) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | read CONFIG raw16 |
| 169 | targeted | `reg16 0x01` | 0x | Reg 0x01 = 0xFB68 (64360) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | read ADC_CONFIG raw16 |
| 170 | targeted | `reg16 0x02` | 0x | Reg 0x02 = 0x0EA7 (3751) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | read SHUNT_CAL raw16 |
| 171 | targeted | `reg24 0x04` | 0x | Reg 0x04 = 0x00EEB0 (61104) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | read VSHUNT raw24 |
| 172 | targeted | `reg24 0x05` | 0x | Reg 0x05 = 0x0EFCD0 (982224) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | read VBUS raw24 |
| 173 | targeted | `reg24 0x06` | 0x | Reg 0x06 = 0x0F7AFF (1014527) / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | read DIETEMP raw24 |
| 174 | targeted | `reg24 0x07` | 0x | Reg 0x07 = 0x0106F0 (67312) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | read CURRENT raw24 |
| 175 | targeted | `reg24 0x08` | 0x | Reg 0x08 = 0x003D95 (15765) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | read POWER raw24 |
| 176 | targeted | `reg40 0x09` | 0x | Reg 0x09 = 0x0000000017 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | read ENERGY raw40 |
| 177 | targeted | `reg40 0x0A` | 0x | Reg 0x0A = 0x0000000072 / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | read CHARGE raw40 |
| 178 | targeted | `mode -1` | Invalid mode | [W] Invalid mode (0-15) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid negative mode |
| 179 | targeted | `mode 16` | Invalid mode | [W] Invalid mode (0-15) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid high mode |
| 180 | targeted | `trigger 8` | Invalid trigger mode | [W] Invalid trigger mode (0-7 for TRIG_* modes) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid trigger mode |
| 181 | targeted | `convtime vbus 8` | Invalid conversion time | [W] Invalid conversion time index (0-7) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid conversion index |
| 182 | targeted | `convtime bogus 1` | Invalid target | [W] Invalid target: bogus (use vbus\|vsh\|temp) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid conversion target |
| 183 | targeted | `averaging 8` | Invalid averaging | [W] Invalid averaging index (0-7) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid averaging |
| 184 | targeted | `adcrange 2` | Invalid ADC range | [W] Invalid ADC range (0 or 1) / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid ADC range |
| 185 | targeted | `delay 256` | Usage: delay | [W] Usage: delay <0..255> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | reject invalid conversion delay |
| 186 | targeted | `tempco 16384` | Usage: tempco | [W] Usage: tempco <0..16383> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid tempco |
| 187 | targeted | `tempcomp 2` | Usage: tempcomp | [W] Usage: tempcomp <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid temp compensation |
| 188 | targeted | `alatch 2` | Usage: alatch | [W] Usage: alatch <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid latch |
| 189 | targeted | `cnvralert 2` | Usage: cnvralert | [W] Usage: cnvralert <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid conversion alert |
| 190 | targeted | `alslow 2` | Usage: alslow | [W] Usage: alslow <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid slow alert |
| 191 | targeted | `apol 2` | Usage: apol | [W] Usage: apol <0\|1> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid alert polarity |
| 192 | targeted | `cal 0 10` | Usage: cal | [W] Usage: cal <shunt_ohm> <max_current_a> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject zero shunt calibration |
| 193 | targeted | `reg16 0x100` | Usage: reg16 | [W] Usage: reg16 <addr> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid raw16 register |
| 194 | targeted | `reg24 0x100` | Usage: reg24 | [W] Usage: reg24 <addr> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid raw24 register |
| 195 | targeted | `reg40 0x100` | Usage: reg40 | [W] Usage: reg40 <addr> / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reject invalid raw40 register |
| 196 | targeted | `unknown_hil_command` | Unknown command, INVALID_PARAM | [W] Unknown command: unknown_hil_command / [runner] frame_status=INVALID_PARAM frame_elapsed_ms=0 / Status: INVALID_PARAM | PASS | 0.000 | reject unknown command with framed status |
| 197 | targeted | `init 0x50` | Invalid address | [W] Invalid address. Use init 0x40-0x4F / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | reject invalid init address |
| 198 | targeted | `end` | Device shut down | [I] Device shut down. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | end driver |
| 199 | targeted | `vbus` | NOT_INITIALIZED | Status: NOT_INITIALIZED (code=1, detail=0) / Message: begin() not called / [runner] frame_status=NOT_INITIALIZED frame_elapsed_ms=0 / Status: NOT_INITIALIZED | PASS | 0.000 | read after end must fail visibly |
| 200 | targeted | `init` | initialize, OK | [I] bind + initialize(0x41): OK / [runner] frame_status=OK frame_elapsed_ms=14 | PASS | 0.016 | reinitialize configured device |
| 201 | targeted | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.015 | manual recovery after reinit |
| 202 | targeted | `settings` | Active Settings, State:, READY | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | final settings |
| 203 | targeted | `drv` | Driver Health, State: READY, Consecutive failures: 0 | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 0 / Total failures: 0 / Success rate... | PASS | 0.000 | final health must be clean |
| 204 | targeted | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9924 V / Vshunt:  0.0011994 V / Temp:    30.96 C / Current: 0.087151 A / Power:   0.958874 W / Energy:  0.0000000... | PASS | 0.000 | final aggregate read |
| 205 | targeted | `help` | sample_step <budget>, reset_start, apply_start | === INA228 CLI Help === / [W] Safety: this example does not make 85 V systems safe. Use qualified design practices, isolation where needed, fusing, creepage/... | PASS | 0.016 | v3 cooperative CLI surface |
| 206 | targeted | `integer` | Cooperative Instantaneous Sample, Operation:, Current: | === Cooperative Instantaneous Sample === / Operation: 247 request: 247 generation: 2 / Bus:     11993 mV / Shunt:   1012 uV / Temp:    30969 mdegC / Current:... | PASS | 0.015 | bounded atomic sample |
| 207 | targeted | `sample_step 0` | pollJob(0), IN_PROGRESS | [I] startInstantaneousSample(): OK operation=248 / [I] pollJob(0): IN_PROGRESS / Job pending; no partial sample is exposed. / [runner] frame_status=OK frame_... | PASS | 0.000 | zero-budget sample start |
| 208 | targeted | `sample_step 3` | pollJob(3), IN_PROGRESS | [I] pollJob(3): IN_PROGRESS / Job pending; no partial sample is exposed. / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | sample verify/trigger budget |
| 209 | targeted | `sample_step 8` | Cooperative Sample Result | [I] pollJob(8): OK / === Cooperative Sample Result === / Bus:     11991 mV / Shunt:   1211 uV / Temp:    30969 mdegC / Current: 81 mA / Power:   968 mW / DIA... | PASS | 0.000 | sample wait/read/restore completion |
| 210 | targeted | `apply_start` | startReinitialize, OK | [I] startReinitialize(): OK operation=249 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | verified reinitialization start |
| 211 | targeted | `apply_step 0` | pollJob(0), IN_PROGRESS | [I] pollJob(0): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | reinitialization zero budget |
| 212 | targeted | `apply_step 14` | pollJob(14), OK, terminal result consumed | [I] pollJob(14): OK terminal result consumed / [runner] frame_status=OK frame_elapsed_ms=3 | PASS | 0.000 | verified reinitialization completion |
| 213 | targeted | `reset_start` | startReset, OK | [I] startReset(): OK operation=250 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | maintenance reset start |
| 214 | targeted | `reset_step 1` | pollJob(1), IN_PROGRESS | [I] pollJob(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset write |
| 215 | targeted | `reset_step 0` | pollJob(0), IN_PROGRESS | [I] pollJob(0): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | reset wait zero budget |
| 216 | targeted | `reset_step 15` | pollJob(15), OK, terminal result consumed | [I] pollJob(15): OK terminal result consumed / [runner] frame_status=OK frame_elapsed_ms=3 | PASS | 0.015 | reset verification and initialization |
| 217 | targeted | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | application-owned recovery boundary |
| 218 | targeted | `diagsnap` | DIAG_ALRT Snapshot, cache-only | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   196123 ms / MEMSTAT:    ... | PASS | 0.000 | cache-only diagnostic evidence |
| 219 | targeted | `selftest` | Selftest result, fail=0 | === INA228 selftest (diagnostic commands; reads DIAG_ALRT) === / Note: DIAG_ALRT reads can clear CNVRF and latched evidence. / [PASS] probe responds / [PASS]... | PASS | 0.016 | device self-test |
| 220 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 221 | transfer | `sample_step 0` | pollJob(0), IN_PROGRESS | [I] startInstantaneousSample(): OK operation=253 / [I] pollJob(0): IN_PROGRESS / Job pending; no partial sample is exposed. / [runner] frame_status=OK frame_... | PASS | 0.000 | sample zero budget |
| 222 | transfer | `xfer_assert 0 0 0` | XFER_ASSERT PASS | XFER_ASSERT PASS read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | sample start is bus-silent |
| 223 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 224 | transfer | `sample_step 1` | pollJob(1), IN_PROGRESS | [I] pollJob(1): IN_PROGRESS / Job pending; no partial sample is exposed. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | sample budget one |
| 225 | transfer | `xfer_assert 1 0 1` | XFER_ASSERT PASS | XFER_ASSERT PASS read=1 write=0 total=1 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | sample budget-one count |
| 226 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 227 | transfer | `sample_step 2` | pollJob(2), IN_PROGRESS | [I] pollJob(2): IN_PROGRESS / Job pending; no partial sample is exposed. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | sample calibration/trigger |
| 228 | transfer | `xfer_assert 1 1 2` | XFER_ASSERT PASS | XFER_ASSERT PASS read=1 write=1 total=2 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | sample trigger count |
| 229 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 230 | transfer | `sample_step 8` | Cooperative Sample Result | [I] pollJob(8): OK / === Cooperative Sample Result === / Bus:     11990 mV / Shunt:   1078 uV / Temp:    30961 mdegC / Current: 72 mA / Power:   861 mW / DIA... | PASS | 0.000 | sample completion |
| 231 | transfer | `xfer_assert 7 1 8` | XFER_ASSERT PASS | XFER_ASSERT PASS read=7 write=1 total=8 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | sample completion count |
| 232 | transfer | `apply_start` | startReinitialize, OK | [I] startReinitialize(): OK operation=254 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reinitialization start |
| 233 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 234 | transfer | `apply_step 0` | pollJob(0), IN_PROGRESS | [I] pollJob(0): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reinitialization zero budget |
| 235 | transfer | `xfer_assert 0 0 0` | XFER_ASSERT PASS | XFER_ASSERT PASS read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reinitialization zero count |
| 236 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 237 | transfer | `apply_step 1` | pollJob(1), IN_PROGRESS | [I] pollJob(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reinitialization budget one |
| 238 | transfer | `xfer_assert 1 0 1` | XFER_ASSERT PASS | XFER_ASSERT PASS read=1 write=0 total=1 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reinitialization first read |
| 239 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 240 | transfer | `apply_step 13` | pollJob(13), OK | [I] pollJob(13): OK terminal result consumed / [runner] frame_status=OK frame_elapsed_ms=3 | PASS | 0.015 | reinitialization completion |
| 241 | transfer | `xfer_assert 7 6 13` | XFER_ASSERT PASS | XFER_ASSERT PASS read=7 write=6 total=13 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reinitialization remaining count |
| 242 | transfer | `reset_start` | startReset, OK | [I] startReset(): OK operation=255 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset start |
| 243 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 244 | transfer | `reset_step 0` | pollJob(0), IN_PROGRESS | [I] pollJob(0): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset zero budget |
| 245 | transfer | `xfer_assert 0 0 0` | XFER_ASSERT PASS | XFER_ASSERT PASS read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | reset zero count |
| 246 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 247 | transfer | `reset_step 1` | pollJob(1), IN_PROGRESS | [I] pollJob(1): IN_PROGRESS / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset write |
| 248 | transfer | `xfer_assert 0 1 1` | XFER_ASSERT PASS | XFER_ASSERT PASS read=0 write=1 total=1 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset write count |
| 249 | transfer | `xfer_reset` | XFER_RESET | XFER_RESET read=0 write=0 total=0 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset counters |
| 250 | transfer | `reset_step 15` | pollJob(15), OK | [I] pollJob(15): OK terminal result consumed / [runner] frame_status=OK frame_elapsed_ms=3 | PASS | 0.016 | reset completion |
| 251 | transfer | `xfer_assert 9 6 15` | XFER_ASSERT PASS | XFER_ASSERT PASS read=9 write=6 total=15 / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | reset remaining count |
| 252 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9930 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 253 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9930 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 254 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9900 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 255 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9900 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 256 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 257 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 258 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 259 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9883 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | benchmark bus voltage |
| 260 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9883 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 261 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9893 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 262 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9924 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 263 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9924 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 264 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9924 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 265 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9932 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 266 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9932 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 267 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9932 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 268 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9932 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 269 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9914 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | benchmark bus voltage |
| 270 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9914 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 271 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9926 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 272 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9926 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 273 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9926 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 274 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9875 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 275 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9875 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 276 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 277 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 278 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 279 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 280 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9904 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark bus voltage |
| 281 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9904 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 282 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9918 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 283 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9916 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 284 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9916 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 285 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9898 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 286 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9898 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 287 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9898 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 288 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9922 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 289 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9922 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 290 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9922 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 291 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9918 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | benchmark bus voltage |
| 292 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9918 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 293 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9918 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 294 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9918 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 295 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9912 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 296 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9912 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 297 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9922 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 298 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 299 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 300 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9916 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark bus voltage |
| 301 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9916 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 302 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9916 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 303 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9914 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 304 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9914 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 305 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9914 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 306 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9896 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 307 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9896 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 308 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9893 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 309 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9893 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 310 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9928 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 311 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9928 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark bus voltage |
| 312 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9891 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 313 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9891 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 314 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9914 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 315 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9914 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 316 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9908 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 317 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9908 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 318 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9895 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 319 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9895 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 320 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9895 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 321 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9920 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 322 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9920 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 323 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9889 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.015 | benchmark bus voltage |
| 324 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9889 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 325 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9924 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 326 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9924 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 327 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9893 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 328 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9893 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 329 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9916 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 330 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9916 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 331 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9922 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 332 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9922 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 333 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9924 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark bus voltage |
| 334 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9924 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 335 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9924 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 336 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9887 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 337 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9887 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 338 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9943 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 339 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9943 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 340 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9943 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark bus voltage |
| 341 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9895 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 342 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9895 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 343 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9918 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 344 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9918 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 345 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9926 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark bus voltage |
| 346 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9926 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 347 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9936 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 348 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9936 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 349 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9912 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 350 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9912 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 351 | benchmark | `vbus` | Vbus | [I] Vbus: 11.9930 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark bus voltage |
| 352 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012228 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 353 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011772 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 354 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011772 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 355 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011772 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.015 | benchmark shunt voltage |
| 356 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013181 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 357 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009784 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 358 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009784 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 359 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012112 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 360 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012112 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 361 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012031 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 362 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012031 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 363 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012031 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 364 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011950 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 365 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011950 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 366 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013738 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | benchmark shunt voltage |
| 367 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013738 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 368 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013738 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 369 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011872 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 370 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011872 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 371 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012363 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 372 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012363 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 373 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012119 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 374 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012119 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 375 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012691 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 376 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012691 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 377 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012691 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.015 | benchmark shunt voltage |
| 378 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012672 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 379 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012672 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 380 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0010078 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 381 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0010078 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 382 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012200 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 383 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012200 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 384 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012100 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 385 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012100 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 386 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012441 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 387 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012441 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 388 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0010791 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark shunt voltage |
| 389 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0010791 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 390 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0010791 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 391 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011819 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 392 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011819 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 393 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011991 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 394 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011991 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 395 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011891 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 396 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011891 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 397 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011891 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 398 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012878 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark shunt voltage |
| 399 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009794 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 400 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009794 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 401 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012087 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 402 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012087 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 403 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012137 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 404 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012137 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 405 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012137 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 406 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011775 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 407 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011775 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 408 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013544 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 409 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013544 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | benchmark shunt voltage |
| 410 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009669 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 411 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009669 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 412 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012116 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 413 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012116 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 414 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011672 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 415 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011672 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 416 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011787 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 417 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011787 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 418 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011787 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 419 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013853 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 420 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013853 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark shunt voltage |
| 421 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009622 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 422 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009622 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 423 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012184 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 424 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012184 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 425 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011341 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 426 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011341 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 427 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011853 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 428 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011853 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 429 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0011853 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 430 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0014053 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 431 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0014053 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark shunt voltage |
| 432 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009703 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 433 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009703 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 434 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012297 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 435 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012297 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 436 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0010853 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 437 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0010853 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 438 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012025 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 439 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012025 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 440 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012025 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 441 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013809 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 442 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0013809 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | benchmark shunt voltage |
| 443 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009928 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 444 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0009928 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 445 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012656 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 446 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012656 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 447 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0010519 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 448 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0010519 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 449 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012031 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 450 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012031 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark shunt voltage |
| 451 | benchmark | `vshunt` | Vshunt | [I] Vshunt: 0.0012575 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark shunt voltage |
| 452 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 453 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark temperature |
| 454 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 455 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 456 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 457 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 458 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 459 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 460 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 461 | benchmark | `temp` | Temp | [I] Temp: 30.95 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 462 | benchmark | `temp` | Temp | [I] Temp: 30.95 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 463 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | benchmark temperature |
| 464 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 465 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 466 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 467 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 468 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 469 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 470 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 471 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 472 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 473 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 474 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 475 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | benchmark temperature |
| 476 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 477 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 478 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 479 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 480 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 481 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 482 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 483 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 484 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 485 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 486 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark temperature |
| 487 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 488 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 489 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 490 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 491 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 492 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 493 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 494 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 495 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 496 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 497 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | benchmark temperature |
| 498 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 499 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 500 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 501 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 502 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 503 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 504 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 505 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 506 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 507 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark temperature |
| 508 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 509 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 510 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 511 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 512 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 513 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 514 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 515 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 516 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 517 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 518 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark temperature |
| 519 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 520 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 521 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 522 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 523 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 524 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 525 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 526 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 527 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 528 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 529 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | benchmark temperature |
| 530 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 531 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 532 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 533 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 534 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 535 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 536 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 537 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 538 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 539 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 540 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 541 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | benchmark temperature |
| 542 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 543 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 544 | benchmark | `temp` | Temp | [I] Temp: 30.98 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 545 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 546 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 547 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 548 | benchmark | `temp` | Temp | [I] Temp: 30.96 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | benchmark temperature |
| 549 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 550 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 551 | benchmark | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | benchmark temperature |
| 552 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3490 (0x000DA2) / Vbus:   61397 (0x00EFD5) / Temp:   3964 (0x0F7C) / Current:4791 (0x0012B7) / Power:  14277 (0x0037C5) / Ene... | PASS | 0.015 | benchmark raw sample |
| 553 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3674 (0x000E5A) / Vbus:   61401 (0x00EFD9) / Temp:   3964 (0x0F7C) / Current:4011 (0x000FAB) / Power:  17954 (0x004622) / Ene... | PASS | 0.000 | benchmark raw sample |
| 554 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3749 (0x000EA5) / Vbus:   61404 (0x00EFDC) / Temp:   3963 (0x0F7B) / Current:4093 (0x000FFD) / Power:  15339 (0x003BEB) / Ene... | PASS | 0.000 | benchmark raw sample |
| 555 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3763 (0x000EB3) / Vbus:   61393 (0x00EFD1) / Temp:   3963 (0x0F7B) / Current:4109 (0x00100D) / Power:  15396 (0x003C24) / Ene... | PASS | 0.000 | benchmark raw sample |
| 556 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3881 (0x000F29) / Vbus:   61386 (0x00EFCA) / Temp:   3965 (0x0F7D) / Current:3814 (0x000EE6) / Power:  15874 (0x003E02) / Ene... | PASS | 0.016 | benchmark raw sample |
| 557 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3353 (0x000D19) / Vbus:   61397 (0x00EFD5) / Temp:   3964 (0x0F7C) / Current:3661 (0x000E4D) / Power:  13719 (0x003597) / Ene... | PASS | 0.000 | benchmark raw sample |
| 558 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3744 (0x000EA0) / Vbus:   61404 (0x00EFDC) / Temp:   3963 (0x0F7B) / Current:4088 (0x000FF8) / Power:  15321 (0x003BD9) / Ene... | PASS | 0.000 | benchmark raw sample |
| 559 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3827 (0x000EF3) / Vbus:   61402 (0x00EFDA) / Temp:   3963 (0x0F7B) / Current:4178 (0x001052) / Power:  15657 (0x003D29) / Ene... | PASS | 0.000 | benchmark raw sample |
| 560 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3840 (0x000F00) / Vbus:   61397 (0x00EFD5) / Temp:   3963 (0x0F7B) / Current:3672 (0x000E58) / Power:  15712 (0x003D60) / Ene... | PASS | 0.016 | benchmark raw sample |
| 561 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3459 (0x000D83) / Vbus:   61400 (0x00EFD8) / Temp:   3966 (0x0F7E) / Current:3777 (0x000EC1) / Power:  14153 (0x003749) / Ene... | PASS | 0.000 | benchmark raw sample |
| 562 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3790 (0x000ECE) / Vbus:   61400 (0x00EFD8) / Temp:   3965 (0x0F7D) / Current:4138 (0x00102A) / Power:  15507 (0x003C93) / Ene... | PASS | 0.000 | benchmark raw sample |
| 563 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3859 (0x000F13) / Vbus:   61393 (0x00EFD1) / Temp:   3963 (0x0F7B) / Current:4213 (0x001075) / Power:  15786 (0x003DAA) / Ene... | PASS | 0.000 | benchmark raw sample |
| 564 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3775 (0x000EBF) / Vbus:   61399 (0x00EFD7) / Temp:   3963 (0x0F7B) / Current:3566 (0x000DEE) / Power:  15447 (0x003C57) / Ene... | PASS | 0.015 | benchmark raw sample |
| 565 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3571 (0x000DF3) / Vbus:   61395 (0x00EFD3) / Temp:   3964 (0x0F7C) / Current:3899 (0x000F3B) / Power:  14612 (0x003914) / Ene... | PASS | 0.000 | benchmark raw sample |
| 566 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3788 (0x000ECC) / Vbus:   61407 (0x00EFDF) / Temp:   3964 (0x0F7C) / Current:4136 (0x001028) / Power:  15501 (0x003C8D) / Ene... | PASS | 0.000 | benchmark raw sample |
| 567 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3886 (0x000F2E) / Vbus:   61393 (0x00EFD1) / Temp:   3965 (0x0F7D) / Current:4243 (0x001093) / Power:  15899 (0x003E1B) / Ene... | PASS | 0.000 | benchmark raw sample |
| 568 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3663 (0x000E4F) / Vbus:   61396 (0x00EFD4) / Temp:   3965 (0x0F7D) / Current:3564 (0x000DEC) / Power:  14985 (0x003A89) / Ene... | PASS | 0.016 | benchmark raw sample |
| 569 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3688 (0x000E68) / Vbus:   61405 (0x00EFDD) / Temp:   3965 (0x0F7D) / Current:4027 (0x000FBB) / Power:  15092 (0x003AF4) / Ene... | PASS | 0.000 | benchmark raw sample |
| 570 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3824 (0x000EF0) / Vbus:   61389 (0x00EFCD) / Temp:   3965 (0x0F7D) / Current:4175 (0x00104F) / Power:  15643 (0x003D1B) / Ene... | PASS | 0.000 | benchmark raw sample |
| 571 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3889 (0x000F31) / Vbus:   61388 (0x00EFCC) / Temp:   3965 (0x0F7D) / Current:4246 (0x001096) / Power:  15909 (0x003E25) / Ene... | PASS | 0.000 | benchmark raw sample |
| 572 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3315 (0x000CF3) / Vbus:   61391 (0x00EFCF) / Temp:   3963 (0x0F7B) / Current:3619 (0x000E23) / Power:  13562 (0x0034FA) / Ene... | PASS | 0.016 | benchmark raw sample |
| 573 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3747 (0x000EA3) / Vbus:   61399 (0x00EFD7) / Temp:   3964 (0x0F7C) / Current:4091 (0x000FFB) / Power:  15331 (0x003BE3) / Ene... | PASS | 0.000 | benchmark raw sample |
| 574 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3832 (0x000EF8) / Vbus:   61399 (0x00EFD7) / Temp:   3963 (0x0F7B) / Current:4184 (0x001058) / Power:  15679 (0x003D3F) / Ene... | PASS | 0.000 | benchmark raw sample |
| 575 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3873 (0x000F21) / Vbus:   61400 (0x00EFD8) / Temp:   3964 (0x0F7C) / Current:3661 (0x000E4D) / Power:  15848 (0x003DE8) / Ene... | PASS | 0.000 | benchmark raw sample |
| 576 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3415 (0x000D57) / Vbus:   61393 (0x00EFD1) / Temp:   3964 (0x0F7C) / Current:3729 (0x000E91) / Power:  13973 (0x003695) / Ene... | PASS | 0.015 | benchmark raw sample |
| 577 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3786 (0x000ECA) / Vbus:   61400 (0x00EFD8) / Temp:   3965 (0x0F7D) / Current:4134 (0x001026) / Power:  15492 (0x003C84) / Ene... | PASS | 0.000 | benchmark raw sample |
| 578 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3885 (0x000F2D) / Vbus:   61401 (0x00EFD9) / Temp:   3964 (0x0F7C) / Current:4242 (0x001092) / Power:  15897 (0x003E19) / Ene... | PASS | 0.000 | benchmark raw sample |
| 579 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3778 (0x000EC2) / Vbus:   61399 (0x00EFD7) / Temp:   3964 (0x0F7C) / Current:3567 (0x000DEF) / Power:  15458 (0x003C62) / Ene... | PASS | 0.000 | benchmark raw sample |
| 580 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3545 (0x000DD9) / Vbus:   61400 (0x00EFD8) / Temp:   3964 (0x0F7C) / Current:3871 (0x000F1F) / Power:  14507 (0x0038AB) / Ene... | PASS | 0.016 | benchmark raw sample |
| 581 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3808 (0x000EE0) / Vbus:   61393 (0x00EFD1) / Temp:   3965 (0x0F7D) / Current:4158 (0x00103E) / Power:  15580 (0x003CDC) / Ene... | PASS | 0.000 | benchmark raw sample |
| 582 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3895 (0x000F37) / Vbus:   61399 (0x00EFD7) / Temp:   3964 (0x0F7C) / Current:4253 (0x00109D) / Power:  15938 (0x003E42) / Ene... | PASS | 0.000 | benchmark raw sample |
| 583 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3648 (0x000E40) / Vbus:   61394 (0x00EFD2) / Temp:   3965 (0x0F7D) / Current:3545 (0x000DD9) / Power:  14925 (0x003A4D) / Ene... | PASS | 0.015 | benchmark raw sample |
| 584 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3675 (0x000E5B) / Vbus:   61394 (0x00EFD2) / Temp:   3965 (0x0F7D) / Current:4013 (0x000FAD) / Power:  15036 (0x003ABC) / Ene... | PASS | 0.000 | benchmark raw sample |
| 585 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3825 (0x000EF1) / Vbus:   61404 (0x00EFDC) / Temp:   3964 (0x0F7C) / Current:4176 (0x001050) / Power:  15650 (0x003D22) / Ene... | PASS | 0.000 | benchmark raw sample |
| 586 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3883 (0x000F2B) / Vbus:   61389 (0x00EFCD) / Temp:   3965 (0x0F7D) / Current:4240 (0x001090) / Power:  15886 (0x003E0E) / Ene... | PASS | 0.000 | benchmark raw sample |
| 587 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3318 (0x000CF6) / Vbus:   61388 (0x00EFCC) / Temp:   3965 (0x0F7D) / Current:3623 (0x000E27) / Power:  14395 (0x00383B) / Ene... | PASS | 0.016 | benchmark raw sample |
| 588 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3719 (0x000E87) / Vbus:   61412 (0x00EFE4) / Temp:   3965 (0x0F7D) / Current:4061 (0x000FDD) / Power:  15221 (0x003B75) / Ene... | PASS | 0.000 | benchmark raw sample |
| 589 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3846 (0x000F06) / Vbus:   61398 (0x00EFD6) / Temp:   3964 (0x0F7C) / Current:4199 (0x001067) / Power:  15735 (0x003D77) / Ene... | PASS | 0.000 | benchmark raw sample |
| 590 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3865 (0x000F19) / Vbus:   61394 (0x00EFD2) / Temp:   3964 (0x0F7C) / Current:3689 (0x000E69) / Power:  15813 (0x003DC5) / Ene... | PASS | 0.000 | benchmark raw sample |
| 591 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3398 (0x000D46) / Vbus:   61403 (0x00EFDB) / Temp:   3964 (0x0F7C) / Current:3710 (0x000E7E) / Power:  13901 (0x00364D) / Ene... | PASS | 0.016 | benchmark raw sample |
| 592 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3789 (0x000ECD) / Vbus:   61401 (0x00EFD9) / Temp:   3964 (0x0F7C) / Current:4137 (0x001029) / Power:  15503 (0x003C8F) / Ene... | PASS | 0.000 | benchmark raw sample |
| 593 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3853 (0x000F0D) / Vbus:   61400 (0x00EFD8) / Temp:   3966 (0x0F7E) / Current:4207 (0x00106F) / Power:  15765 (0x003D95) / Ene... | PASS | 0.000 | benchmark raw sample |
| 594 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3780 (0x000EC4) / Vbus:   61395 (0x00EFD3) / Temp:   3965 (0x0F7D) / Current:3596 (0x000E0C) / Power:  15464 (0x003C68) / Ene... | PASS | 0.000 | benchmark raw sample |
| 595 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3543 (0x000DD7) / Vbus:   61389 (0x00EFCD) / Temp:   3964 (0x0F7C) / Current:3868 (0x000F1C) / Power:  14495 (0x00389F) / Ene... | PASS | 0.015 | benchmark raw sample |
| 596 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3802 (0x000EDA) / Vbus:   61405 (0x00EFDD) / Temp:   3966 (0x0F7E) / Current:4151 (0x001037) / Power:  15557 (0x003CC5) / Ene... | PASS | 0.000 | benchmark raw sample |
| 597 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3882 (0x000F2A) / Vbus:   61395 (0x00EFD3) / Temp:   3963 (0x0F7B) / Current:4239 (0x00108F) / Power:  15884 (0x003E0C) / Ene... | PASS | 0.000 | benchmark raw sample |
| 598 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3691 (0x000E6B) / Vbus:   61396 (0x00EFD4) / Temp:   3965 (0x0F7D) / Current:3550 (0x000DDE) / Power:  15101 (0x003AFD) / Ene... | PASS | 0.000 | benchmark raw sample |
| 599 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3644 (0x000E3C) / Vbus:   61392 (0x00EFD0) / Temp:   3965 (0x0F7D) / Current:3979 (0x000F8B) / Power:  14912 (0x003A40) / Ene... | PASS | 0.016 | benchmark raw sample |
| 600 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3830 (0x000EF6) / Vbus:   61400 (0x00EFD8) / Temp:   3965 (0x0F7D) / Current:4182 (0x001056) / Power:  15672 (0x003D38) / Ene... | PASS | 0.000 | benchmark raw sample |
| 601 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3888 (0x000F30) / Vbus:   61394 (0x00EFD2) / Temp:   3966 (0x0F7E) / Current:4245 (0x001095) / Power:  15906 (0x003E22) / Ene... | PASS | 0.000 | benchmark raw sample |
| 602 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3285 (0x000CD5) / Vbus:   61392 (0x00EFD0) / Temp:   3966 (0x0F7E) / Current:3587 (0x000E03) / Power:  14587 (0x0038FB) / Ene... | PASS | 0.000 | benchmark raw sample |
| 603 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3740 (0x000E9C) / Vbus:   61405 (0x00EFDD) / Temp:   3965 (0x0F7D) / Current:4083 (0x000FF3) / Power:  15302 (0x003BC6) / Ene... | PASS | 0.016 | benchmark raw sample |
| 604 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3808 (0x000EE0) / Vbus:   61402 (0x00EFDA) / Temp:   3965 (0x0F7D) / Current:4158 (0x00103E) / Power:  15582 (0x003CDE) / Ene... | PASS | 0.000 | benchmark raw sample |
| 605 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3850 (0x000F0A) / Vbus:   61398 (0x00EFD6) / Temp:   3967 (0x0F7F) / Current:3736 (0x000E98) / Power:  15754 (0x003D8A) / Ene... | PASS | 0.000 | benchmark raw sample |
| 606 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3392 (0x000D40) / Vbus:   61388 (0x00EFCC) / Temp:   3965 (0x0F7D) / Current:3703 (0x000E77) / Power:  13875 (0x003633) / Ene... | PASS | 0.000 | benchmark raw sample |
| 607 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3781 (0x000EC5) / Vbus:   61405 (0x00EFDD) / Temp:   3966 (0x0F7E) / Current:4128 (0x001020) / Power:  15471 (0x003C6F) / Ene... | PASS | 0.015 | benchmark raw sample |
| 608 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3854 (0x000F0E) / Vbus:   61405 (0x00EFDD) / Temp:   3964 (0x0F7C) / Current:4208 (0x001070) / Power:  15771 (0x003D9B) / Ene... | PASS | 0.000 | benchmark raw sample |
| 609 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3807 (0x000EDF) / Vbus:   61392 (0x00EFD0) / Temp:   3963 (0x0F7B) / Current:3592 (0x000E08) / Power:  15576 (0x003CD8) / Ene... | PASS | 0.000 | benchmark raw sample |
| 610 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3521 (0x000DC1) / Vbus:   61396 (0x00EFD4) / Temp:   3964 (0x0F7C) / Current:3844 (0x000F04) / Power:  14405 (0x003845) / Ene... | PASS | 0.000 | benchmark raw sample |
| 611 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3798 (0x000ED6) / Vbus:   61407 (0x00EFDF) / Temp:   3964 (0x0F7C) / Current:4147 (0x001033) / Power:  15542 (0x003CB6) / Ene... | PASS | 0.016 | benchmark raw sample |
| 612 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3882 (0x000F2A) / Vbus:   61412 (0x00EFE4) / Temp:   3966 (0x0F7E) / Current:4239 (0x00108F) / Power:  15889 (0x003E11) / Ene... | PASS | 0.000 | benchmark raw sample |
| 613 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3691 (0x000E6B) / Vbus:   61402 (0x00EFDA) / Temp:   3964 (0x0F7C) / Current:3532 (0x000DCC) / Power:  15103 (0x003AFF) / Ene... | PASS | 0.000 | benchmark raw sample |
| 614 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3642 (0x000E3A) / Vbus:   61404 (0x00EFDC) / Temp:   3963 (0x0F7B) / Current:3976 (0x000F88) / Power:  14900 (0x003A34) / Ene... | PASS | 0.000 | benchmark raw sample |
| 615 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3810 (0x000EE2) / Vbus:   61398 (0x00EFD6) / Temp:   3966 (0x0F7E) / Current:4160 (0x001040) / Power:  15589 (0x003CE5) / Ene... | PASS | 0.015 | benchmark raw sample |
| 616 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3890 (0x000F32) / Vbus:   61395 (0x00EFD3) / Temp:   3965 (0x0F7D) / Current:4247 (0x001097) / Power:  15914 (0x003E2A) / Ene... | PASS | 0.000 | benchmark raw sample |
| 617 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3276 (0x000CCC) / Vbus:   61398 (0x00EFD6) / Temp:   3964 (0x0F7C) / Current:3577 (0x000DF9) / Power:  14611 (0x003913) / Ene... | PASS | 0.000 | benchmark raw sample |
| 618 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3715 (0x000E83) / Vbus:   61407 (0x00EFDF) / Temp:   3965 (0x0F7D) / Current:4056 (0x000FD8) / Power:  15201 (0x003B61) / Ene... | PASS | 0.000 | benchmark raw sample |
| 619 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3823 (0x000EEF) / Vbus:   61405 (0x00EFDD) / Temp:   3966 (0x0F7E) / Current:4174 (0x00104E) / Power:  15643 (0x003D1B) / Ene... | PASS | 0.000 | benchmark raw sample |
| 620 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3873 (0x000F21) / Vbus:   61405 (0x00EFDD) / Temp:   3966 (0x0F7E) / Current:3725 (0x000E8D) / Power:  15849 (0x003DE9) / Ene... | PASS | 0.000 | benchmark raw sample |
| 621 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3361 (0x000D21) / Vbus:   61385 (0x00EFC9) / Temp:   3967 (0x0F7F) / Current:3670 (0x000E56) / Power:  13751 (0x0035B7) / Ene... | PASS | 0.000 | benchmark raw sample |
| 622 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3777 (0x000EC1) / Vbus:   61400 (0x00EFD8) / Temp:   3965 (0x0F7D) / Current:4124 (0x00101C) / Power:  15454 (0x003C5E) / Ene... | PASS | 0.016 | benchmark raw sample |
| 623 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3855 (0x000F0F) / Vbus:   61408 (0x00EFE0) / Temp:   3965 (0x0F7D) / Current:4209 (0x001071) / Power:  15775 (0x003D9F) / Ene... | PASS | 0.000 | benchmark raw sample |
| 624 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3830 (0x000EF6) / Vbus:   61396 (0x00EFD4) / Temp:   3966 (0x0F7E) / Current:3591 (0x000E07) / Power:  15671 (0x003D37) / Ene... | PASS | 0.000 | benchmark raw sample |
| 625 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3497 (0x000DA9) / Vbus:   61389 (0x00EFCD) / Temp:   3965 (0x0F7D) / Current:3818 (0x000EEA) / Power:  14309 (0x0037E5) / Ene... | PASS | 0.000 | benchmark raw sample |
| 626 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3796 (0x000ED4) / Vbus:   61409 (0x00EFE1) / Temp:   3964 (0x0F7C) / Current:4145 (0x001031) / Power:  15535 (0x003CAF) / Ene... | PASS | 0.015 | benchmark raw sample |
| 627 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3871 (0x000F1F) / Vbus:   61399 (0x00EFD7) / Temp:   3965 (0x0F7D) / Current:4227 (0x001083) / Power:  15840 (0x003DE0) / Ene... | PASS | 0.000 | benchmark raw sample |
| 628 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3691 (0x000E6B) / Vbus:   61391 (0x00EFCF) / Temp:   3966 (0x0F7E) / Current:3553 (0x000DE1) / Power:  15100 (0x003AFC) / Ene... | PASS | 0.000 | benchmark raw sample |
| 629 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3607 (0x000E17) / Vbus:   61398 (0x00EFD6) / Temp:   3965 (0x0F7D) / Current:3938 (0x000F62) / Power:  14757 (0x0039A5) / Ene... | PASS | 0.000 | benchmark raw sample |
| 630 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3809 (0x000EE1) / Vbus:   61401 (0x00EFD9) / Temp:   3965 (0x0F7D) / Current:4159 (0x00103F) / Power:  15586 (0x003CE2) / Ene... | PASS | 0.016 | benchmark raw sample |
| 631 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3886 (0x000F2E) / Vbus:   61395 (0x00EFD3) / Temp:   3965 (0x0F7D) / Current:4243 (0x001093) / Power:  15899 (0x003E1B) / Ene... | PASS | 0.000 | benchmark raw sample |
| 632 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3260 (0x000CBC) / Vbus:   61396 (0x00EFD4) / Temp:   3965 (0x0F7D) / Current:3559 (0x000DE7) / Power:  14558 (0x0038DE) / Ene... | PASS | 0.000 | benchmark raw sample |
| 633 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3709 (0x000E7D) / Vbus:   61404 (0x00EFDC) / Temp:   3965 (0x0F7D) / Current:4050 (0x000FD2) / Power:  15178 (0x003B4A) / Ene... | PASS | 0.000 | benchmark raw sample |
| 634 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3820 (0x000EEC) / Vbus:   61394 (0x00EFD2) / Temp:   3965 (0x0F7D) / Current:4171 (0x00104B) / Power:  15629 (0x003D0D) / Ene... | PASS | 0.016 | benchmark raw sample |
| 635 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3871 (0x000F1F) / Vbus:   61395 (0x00EFD3) / Temp:   3965 (0x0F7D) / Current:3755 (0x000EAB) / Power:  15839 (0x003DDF) / Ene... | PASS | 0.000 | benchmark raw sample |
| 636 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3352 (0x000D18) / Vbus:   61402 (0x00EFDA) / Temp:   3965 (0x0F7D) / Current:3660 (0x000E4C) / Power:  13712 (0x003590) / Ene... | PASS | 0.000 | benchmark raw sample |
| 637 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3755 (0x000EAB) / Vbus:   61413 (0x00EFE5) / Temp:   3965 (0x0F7D) / Current:4100 (0x001004) / Power:  15368 (0x003C08) / Ene... | PASS | 0.000 | benchmark raw sample |
| 638 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3851 (0x000F0B) / Vbus:   61407 (0x00EFDF) / Temp:   3964 (0x0F7C) / Current:4205 (0x00106D) / Power:  15760 (0x003D90) / Ene... | PASS | 0.015 | benchmark raw sample |
| 639 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3837 (0x000EFD) / Vbus:   61398 (0x00EFD6) / Temp:   3966 (0x0F7E) / Current:3615 (0x000E1F) / Power:  15698 (0x003D52) / Ene... | PASS | 0.000 | benchmark raw sample |
| 640 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3490 (0x000DA2) / Vbus:   61394 (0x00EFD2) / Temp:   3966 (0x0F7E) / Current:3810 (0x000EE2) / Power:  14275 (0x0037C3) / Ene... | PASS | 0.000 | benchmark raw sample |
| 641 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3814 (0x000EE6) / Vbus:   61406 (0x00EFDE) / Temp:   3964 (0x0F7C) / Current:4164 (0x001044) / Power:  15606 (0x003CF6) / Ene... | PASS | 0.000 | benchmark raw sample |
| 642 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3871 (0x000F1F) / Vbus:   61402 (0x00EFDA) / Temp:   3964 (0x0F7C) / Current:4227 (0x001083) / Power:  15841 (0x003DE1) / Ene... | PASS | 0.016 | benchmark raw sample |
| 643 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3725 (0x000E8D) / Vbus:   61399 (0x00EFD7) / Temp:   3964 (0x0F7C) / Current:3555 (0x000DE3) / Power:  15241 (0x003B89) / Ene... | PASS | 0.000 | benchmark raw sample |
| 644 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3585 (0x000E01) / Vbus:   61394 (0x00EFD2) / Temp:   3964 (0x0F7C) / Current:3914 (0x000F4A) / Power:  14668 (0x00394C) / Ene... | PASS | 0.000 | benchmark raw sample |
| 645 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3820 (0x000EEC) / Vbus:   61395 (0x00EFD3) / Temp:   3965 (0x0F7D) / Current:4171 (0x00104B) / Power:  15629 (0x003D0D) / Ene... | PASS | 0.000 | benchmark raw sample |
| 646 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3888 (0x000F30) / Vbus:   61400 (0x00EFD8) / Temp:   3965 (0x0F7D) / Current:4245 (0x001095) / Power:  15908 (0x003E24) / Ene... | PASS | 0.015 | benchmark raw sample |
| 647 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3276 (0x000CCC) / Vbus:   61384 (0x00EFC8) / Temp:   3966 (0x0F7E) / Current:3577 (0x000DF9) / Power:  14843 (0x0039FB) / Ene... | PASS | 0.000 | benchmark raw sample |
| 648 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3710 (0x000E7E) / Vbus:   61399 (0x00EFD7) / Temp:   3964 (0x0F7C) / Current:4051 (0x000FD3) / Power:  15181 (0x003B4D) / Ene... | PASS | 0.000 | benchmark raw sample |
| 649 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3833 (0x000EF9) / Vbus:   61394 (0x00EFD2) / Temp:   3964 (0x0F7C) / Current:4185 (0x001059) / Power:  15682 (0x003D42) / Ene... | PASS | 0.000 | benchmark raw sample |
| 650 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3878 (0x000F26) / Vbus:   61400 (0x00EFD8) / Temp:   3965 (0x0F7D) / Current:3738 (0x000E9A) / Power:  15867 (0x003DFB) / Ene... | PASS | 0.016 | benchmark raw sample |
| 651 | benchmark | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3348 (0x000D14) / Vbus:   61392 (0x00EFD0) / Temp:   3964 (0x0F7C) / Current:3655 (0x000E47) / Power:  13696 (0x003580) / Ene... | PASS | 0.000 | benchmark raw sample |
| 652 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 256 request: 256 generation: 8 / Bus:     11993 mV / Shunt:   1009 uV / Temp:    30984 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 653 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 257 request: 257 generation: 8 / Bus:     11993 mV / Shunt:   1015 uV / Temp:    30977 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 654 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 258 request: 258 generation: 8 / Bus:     11993 mV / Shunt:   1017 uV / Temp:    30984 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 655 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 259 request: 259 generation: 8 / Bus:     11995 mV / Shunt:   1014 uV / Temp:    30977 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 656 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 260 request: 260 generation: 8 / Bus:     11992 mV / Shunt:   1014 uV / Temp:    30977 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 657 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 261 request: 261 generation: 8 / Bus:     11991 mV / Shunt:   1014 uV / Temp:    30977 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 658 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 262 request: 262 generation: 8 / Bus:     11992 mV / Shunt:   1014 uV / Temp:    30977 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 659 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 263 request: 263 generation: 8 / Bus:     11991 mV / Shunt:   1015 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 660 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 264 request: 264 generation: 8 / Bus:     11993 mV / Shunt:   1017 uV / Temp:    30969 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 661 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 265 request: 265 generation: 8 / Bus:     11992 mV / Shunt:   1011 uV / Temp:    30977 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 662 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 266 request: 266 generation: 8 / Bus:     11993 mV / Shunt:   1017 uV / Temp:    30969 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 663 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 267 request: 267 generation: 8 / Bus:     11993 mV / Shunt:   1016 uV / Temp:    30969 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 664 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 268 request: 268 generation: 8 / Bus:     11992 mV / Shunt:   1016 uV / Temp:    30969 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 665 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 269 request: 269 generation: 8 / Bus:     11993 mV / Shunt:   1011 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 666 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 270 request: 270 generation: 8 / Bus:     11994 mV / Shunt:   1014 uV / Temp:    30977 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 667 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 271 request: 271 generation: 8 / Bus:     11992 mV / Shunt:   1011 uV / Temp:    30969 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 668 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 272 request: 272 generation: 8 / Bus:     11994 mV / Shunt:   1017 uV / Temp:    30969 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 669 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 273 request: 273 generation: 8 / Bus:     11993 mV / Shunt:   1007 uV / Temp:    30961 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 670 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 274 request: 274 generation: 8 / Bus:     11993 mV / Shunt:   1018 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 671 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 275 request: 275 generation: 8 / Bus:     11992 mV / Shunt:   1010 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 672 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 276 request: 276 generation: 8 / Bus:     11993 mV / Shunt:   1015 uV / Temp:    30961 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 673 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 277 request: 277 generation: 8 / Bus:     11994 mV / Shunt:   1015 uV / Temp:    30969 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 674 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 278 request: 278 generation: 8 / Bus:     11994 mV / Shunt:   1017 uV / Temp:    30953 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 675 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 279 request: 279 generation: 8 / Bus:     11992 mV / Shunt:   1013 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 676 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 280 request: 280 generation: 8 / Bus:     11992 mV / Shunt:   1014 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 677 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 281 request: 281 generation: 8 / Bus:     11995 mV / Shunt:   1012 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 678 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 282 request: 282 generation: 8 / Bus:     11992 mV / Shunt:   1014 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 679 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 283 request: 283 generation: 8 / Bus:     11994 mV / Shunt:   1012 uV / Temp:    30953 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 680 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 284 request: 284 generation: 8 / Bus:     11993 mV / Shunt:   1015 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 681 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 285 request: 285 generation: 8 / Bus:     11994 mV / Shunt:   1014 uV / Temp:    30938 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 682 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 286 request: 286 generation: 8 / Bus:     11993 mV / Shunt:   1015 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 683 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 287 request: 287 generation: 8 / Bus:     11994 mV / Shunt:   1012 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 684 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 288 request: 288 generation: 8 / Bus:     11994 mV / Shunt:   1017 uV / Temp:    30945 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 685 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 289 request: 289 generation: 8 / Bus:     11994 mV / Shunt:   1012 uV / Temp:    30953 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 686 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 290 request: 290 generation: 8 / Bus:     11993 mV / Shunt:   1016 uV / Temp:    30953 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 687 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 291 request: 291 generation: 8 / Bus:     11993 mV / Shunt:   1010 uV / Temp:    30945 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 688 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 292 request: 292 generation: 8 / Bus:     11993 mV / Shunt:   1006 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 689 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 293 request: 293 generation: 8 / Bus:     11992 mV / Shunt:   1011 uV / Temp:    30945 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 690 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 294 request: 294 generation: 8 / Bus:     11994 mV / Shunt:   1015 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 691 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 295 request: 295 generation: 8 / Bus:     11993 mV / Shunt:   1014 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 692 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 296 request: 296 generation: 8 / Bus:     11993 mV / Shunt:   1019 uV / Temp:    30938 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 693 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 297 request: 297 generation: 8 / Bus:     11993 mV / Shunt:   1012 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 694 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 298 request: 298 generation: 8 / Bus:     11994 mV / Shunt:   1019 uV / Temp:    30977 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 695 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 299 request: 299 generation: 8 / Bus:     11991 mV / Shunt:   1010 uV / Temp:    30961 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 696 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 300 request: 300 generation: 8 / Bus:     11994 mV / Shunt:   1019 uV / Temp:    30969 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 697 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 301 request: 301 generation: 8 / Bus:     11992 mV / Shunt:   1008 uV / Temp:    30961 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 698 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 302 request: 302 generation: 8 / Bus:     11992 mV / Shunt:   1019 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 699 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 303 request: 303 generation: 8 / Bus:     11993 mV / Shunt:   1013 uV / Temp:    30930 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 700 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 304 request: 304 generation: 8 / Bus:     11993 mV / Shunt:   1008 uV / Temp:    30938 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 701 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 305 request: 305 generation: 8 / Bus:     11992 mV / Shunt:   1016 uV / Temp:    30938 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 702 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 306 request: 306 generation: 8 / Bus:     11994 mV / Shunt:   1012 uV / Temp:    30938 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 703 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 307 request: 307 generation: 8 / Bus:     11993 mV / Shunt:   1017 uV / Temp:    30938 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 704 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 308 request: 308 generation: 8 / Bus:     11991 mV / Shunt:   1017 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 705 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 309 request: 309 generation: 8 / Bus:     11991 mV / Shunt:   1009 uV / Temp:    30938 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 706 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 310 request: 310 generation: 8 / Bus:     11992 mV / Shunt:   1014 uV / Temp:    30938 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 707 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 311 request: 311 generation: 8 / Bus:     11991 mV / Shunt:   1009 uV / Temp:    30930 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 708 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 312 request: 312 generation: 8 / Bus:     11994 mV / Shunt:   1015 uV / Temp:    30938 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 709 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 313 request: 313 generation: 8 / Bus:     11992 mV / Shunt:   1015 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 710 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 314 request: 314 generation: 8 / Bus:     11993 mV / Shunt:   1012 uV / Temp:    30914 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 711 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 315 request: 315 generation: 8 / Bus:     11993 mV / Shunt:   1013 uV / Temp:    30922 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 712 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 316 request: 316 generation: 8 / Bus:     11995 mV / Shunt:   1016 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 713 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 317 request: 317 generation: 8 / Bus:     11994 mV / Shunt:   1006 uV / Temp:    30914 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 714 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 318 request: 318 generation: 8 / Bus:     11993 mV / Shunt:   1015 uV / Temp:    30938 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 715 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 319 request: 319 generation: 8 / Bus:     11990 mV / Shunt:   1012 uV / Temp:    30930 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 716 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 320 request: 320 generation: 8 / Bus:     11992 mV / Shunt:   1014 uV / Temp:    30930 mdegC / Current:... | PASS | 0.031 | benchmark atomic sample |
| 717 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 321 request: 321 generation: 8 / Bus:     11993 mV / Shunt:   1012 uV / Temp:    30930 mdegC / Current:... | PASS | 0.000 | benchmark atomic sample |
| 718 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 322 request: 322 generation: 8 / Bus:     11993 mV / Shunt:   1013 uV / Temp:    30945 mdegC / Current:... | PASS | 0.031 | benchmark atomic sample |
| 719 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 323 request: 323 generation: 8 / Bus:     11993 mV / Shunt:   1014 uV / Temp:    30953 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 720 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 324 request: 324 generation: 8 / Bus:     11994 mV / Shunt:   1016 uV / Temp:    30945 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 721 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 325 request: 325 generation: 8 / Bus:     11993 mV / Shunt:   1008 uV / Temp:    30922 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 722 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 326 request: 326 generation: 8 / Bus:     11992 mV / Shunt:   1016 uV / Temp:    30930 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 723 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 327 request: 327 generation: 8 / Bus:     11993 mV / Shunt:   1012 uV / Temp:    30914 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 724 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 328 request: 328 generation: 8 / Bus:     11992 mV / Shunt:   1016 uV / Temp:    30961 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 725 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 329 request: 329 generation: 8 / Bus:     11994 mV / Shunt:   1013 uV / Temp:    30953 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 726 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 330 request: 330 generation: 8 / Bus:     11993 mV / Shunt:   1015 uV / Temp:    30938 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 727 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 331 request: 331 generation: 8 / Bus:     11993 mV / Shunt:   1013 uV / Temp:    30914 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 728 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 332 request: 332 generation: 8 / Bus:     11992 mV / Shunt:   1015 uV / Temp:    30898 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 729 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 333 request: 333 generation: 8 / Bus:     11993 mV / Shunt:   1016 uV / Temp:    30930 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 730 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 334 request: 334 generation: 8 / Bus:     11992 mV / Shunt:   1022 uV / Temp:    30938 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 731 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 335 request: 335 generation: 8 / Bus:     11993 mV / Shunt:   1012 uV / Temp:    30922 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 732 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 336 request: 336 generation: 8 / Bus:     11992 mV / Shunt:   1014 uV / Temp:    30938 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 733 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 337 request: 337 generation: 8 / Bus:     11991 mV / Shunt:   1014 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 734 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 338 request: 338 generation: 8 / Bus:     11994 mV / Shunt:   1018 uV / Temp:    30914 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 735 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 339 request: 339 generation: 8 / Bus:     11992 mV / Shunt:   1008 uV / Temp:    30906 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 736 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 340 request: 340 generation: 8 / Bus:     11992 mV / Shunt:   1016 uV / Temp:    30930 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 737 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 341 request: 341 generation: 8 / Bus:     11992 mV / Shunt:   1008 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 738 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 342 request: 342 generation: 8 / Bus:     11992 mV / Shunt:   1016 uV / Temp:    30930 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 739 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 343 request: 343 generation: 8 / Bus:     11991 mV / Shunt:   1015 uV / Temp:    30938 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 740 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 344 request: 344 generation: 8 / Bus:     11992 mV / Shunt:   1018 uV / Temp:    30930 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 741 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 345 request: 345 generation: 8 / Bus:     11993 mV / Shunt:   1008 uV / Temp:    30922 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 742 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 346 request: 346 generation: 8 / Bus:     11993 mV / Shunt:   1016 uV / Temp:    30914 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 743 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 347 request: 347 generation: 8 / Bus:     11992 mV / Shunt:   1016 uV / Temp:    30938 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 744 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 348 request: 348 generation: 8 / Bus:     11993 mV / Shunt:   1016 uV / Temp:    30914 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 745 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 349 request: 349 generation: 8 / Bus:     11993 mV / Shunt:   1010 uV / Temp:    30930 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 746 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 350 request: 350 generation: 8 / Bus:     11993 mV / Shunt:   1013 uV / Temp:    30922 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 747 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 351 request: 351 generation: 8 / Bus:     11993 mV / Shunt:   1015 uV / Temp:    30922 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 748 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 352 request: 352 generation: 8 / Bus:     11993 mV / Shunt:   1015 uV / Temp:    30922 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 749 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 353 request: 353 generation: 8 / Bus:     11989 mV / Shunt:   1017 uV / Temp:    30945 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 750 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 354 request: 354 generation: 8 / Bus:     11993 mV / Shunt:   1016 uV / Temp:    30938 mdegC / Current:... | PASS | 0.015 | benchmark atomic sample |
| 751 | benchmark | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 355 request: 355 generation: 8 / Bus:     11993 mV / Shunt:   1011 uV / Temp:    30922 mdegC / Current:... | PASS | 0.016 | benchmark atomic sample |
| 752 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9908 V / Vshunt:  0.0011591 V / Temp:    30.91 C / Current: 0.077268 A / Power:   0.926455 W / Energy:  0.0000000... | PASS | 0.015 | benchmark aggregate |
| 753 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9928 V / Vshunt:  0.0012053 V / Temp:    30.91 C / Current: 0.080340 A / Power:   0.963453 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 754 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9912 V / Vshunt:  0.0011384 V / Temp:    30.93 C / Current: 0.075895 A / Power:   0.910033 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 755 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9926 V / Vshunt:  0.0012216 V / Temp:    30.89 C / Current: 0.081427 A / Power:   0.976518 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 756 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9887 V / Vshunt:  0.0012109 V / Temp:    30.89 C / Current: 0.080721 A / Power:   0.967726 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 757 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9896 V / Vshunt:  0.0011884 V / Temp:    30.91 C / Current: 0.079214 A / Power:   0.949716 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 758 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9930 V / Vshunt:  0.0011888 V / Temp:    30.91 C / Current: 0.079233 A / Power:   0.950204 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 759 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9926 V / Vshunt:  0.0011741 V / Temp:    30.90 C / Current: 0.078260 A / Power:   0.938483 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 760 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9924 V / Vshunt:  0.0011756 V / Temp:    30.90 C / Current: 0.078375 A / Power:   0.939887 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 761 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9924 V / Vshunt:  0.0011722 V / Temp:    30.93 C / Current: 0.078146 A / Power:   0.937139 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 762 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9908 V / Vshunt:  0.0012059 V / Temp:    30.91 C / Current: 0.080378 A / Power:   0.963758 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 763 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0011913 V / Temp:    30.94 C / Current: 0.079405 A / Power:   0.952158 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 764 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9918 V / Vshunt:  0.0012047 V / Temp:    30.91 C / Current: 0.080302 A / Power:   0.962903 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 765 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9900 V / Vshunt:  0.0011713 V / Temp:    30.91 C / Current: 0.078070 A / Power:   0.936041 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 766 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9922 V / Vshunt:  0.0011725 V / Temp:    30.92 C / Current: 0.078165 A / Power:   0.937323 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 767 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9922 V / Vshunt:  0.0011775 V / Temp:    30.90 C / Current: 0.078489 A / Power:   0.941230 W / Energy:  0.0000000... | PASS | 0.015 | benchmark aggregate |
| 768 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9920 V / Vshunt:  0.0012041 V / Temp:    30.91 C / Current: 0.080264 A / Power:   0.962476 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 769 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9906 V / Vshunt:  0.0012044 V / Temp:    30.93 C / Current: 0.080283 A / Power:   0.962598 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 770 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9912 V / Vshunt:  0.0011944 V / Temp:    30.94 C / Current: 0.079615 A / Power:   0.954661 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 771 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0011787 V / Temp:    30.95 C / Current: 0.075856 A / Power:   0.942085 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 772 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9898 V / Vshunt:  0.0011381 V / Temp:    30.95 C / Current: 0.075856 A / Power:   0.909483 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 773 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9908 V / Vshunt:  0.0012172 V / Temp:    30.95 C / Current: 0.081141 A / Power:   0.972916 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 774 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9928 V / Vshunt:  0.0012147 V / Temp:    30.91 C / Current: 0.080969 A / Power:   0.971023 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 775 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9906 V / Vshunt:  0.0011975 V / Temp:    30.91 C / Current: 0.070190 A / Power:   0.957103 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 776 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0010531 V / Temp:    30.92 C / Current: 0.081561 A / Power:   0.841655 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 777 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9934 V / Vshunt:  0.0012234 V / Temp:    30.91 C / Current: 0.080473 A / Power:   0.978166 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 778 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9924 V / Vshunt:  0.0012072 V / Temp:    30.91 C / Current: 0.080473 A / Power:   0.965040 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 779 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9922 V / Vshunt:  0.0012022 V / Temp:    30.92 C / Current: 0.080130 A / Power:   0.960888 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 780 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9920 V / Vshunt:  0.0012100 V / Temp:    30.93 C / Current: 0.080664 A / Power:   0.967299 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 781 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9910 V / Vshunt:  0.0012006 V / Temp:    30.91 C / Current: 0.080035 A / Power:   0.959667 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 782 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9926 V / Vshunt:  0.0011906 V / Temp:    30.95 C / Current: 0.072365 A / Power:   0.951792 W / Energy:  0.0000000... | PASS | 0.015 | benchmark aggregate |
| 783 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9906 V / Vshunt:  0.0010856 V / Temp:    30.91 C / Current: 0.081694 A / Power:   0.867663 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 784 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9930 V / Vshunt:  0.0012256 V / Temp:    30.91 C / Current: 0.081694 A / Power:   0.979753 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 785 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9912 V / Vshunt:  0.0012063 V / Temp:    30.92 C / Current: 0.080416 A / Power:   0.964246 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 786 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0012047 V / Temp:    30.91 C / Current: 0.080302 A / Power:   0.962903 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 787 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9930 V / Vshunt:  0.0012050 V / Temp:    30.95 C / Current: 0.080321 A / Power:   0.963269 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 788 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9920 V / Vshunt:  0.0012019 V / Temp:    30.91 C / Current: 0.080111 A / Power:   0.960644 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 789 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9924 V / Vshunt:  0.0011800 V / Temp:    30.93 C / Current: 0.073815 A / Power:   0.943306 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 790 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9914 V / Vshunt:  0.0011075 V / Temp:    30.93 C / Current: 0.081733 A / Power:   0.885124 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 791 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0012262 V / Temp:    30.92 C / Current: 0.081733 A / Power:   0.980059 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 792 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9902 V / Vshunt:  0.0012072 V / Temp:    30.93 C / Current: 0.080473 A / Power:   0.964857 W / Energy:  0.0000000... | PASS | 0.015 | benchmark aggregate |
| 793 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9918 V / Vshunt:  0.0011975 V / Temp:    30.95 C / Current: 0.079825 A / Power:   0.957225 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 794 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9934 V / Vshunt:  0.0012000 V / Temp:    30.93 C / Current: 0.079996 A / Power:   0.959423 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 795 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9922 V / Vshunt:  0.0011850 V / Temp:    30.96 C / Current: 0.078985 A / Power:   0.947152 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 796 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9910 V / Vshunt:  0.0011834 V / Temp:    30.96 C / Current: 0.078890 A / Power:   0.945931 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 797 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9914 V / Vshunt:  0.0011722 V / Temp:    30.95 C / Current: 0.078146 A / Power:   0.937078 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 798 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9910 V / Vshunt:  0.0011687 V / Temp:    30.92 C / Current: 0.077898 A / Power:   0.934026 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 799 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9902 V / Vshunt:  0.0011750 V / Temp:    30.93 C / Current: 0.078318 A / Power:   0.939032 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 800 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9896 V / Vshunt:  0.0011781 V / Temp:    30.92 C / Current: 0.078527 A / Power:   0.941474 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 801 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9896 V / Vshunt:  0.0011794 V / Temp:    30.92 C / Current: 0.078623 A / Power:   0.942817 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 802 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9924 V / Vshunt:  0.0011884 V / Temp:    30.92 C / Current: 0.079214 A / Power:   0.949838 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 803 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9912 V / Vshunt:  0.0011953 V / Temp:    30.92 C / Current: 0.079672 A / Power:   0.955333 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 804 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9928 V / Vshunt:  0.0011594 V / Temp:    30.95 C / Current: 0.077287 A / Power:   0.926883 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 805 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9914 V / Vshunt:  0.0012306 V / Temp:    30.91 C / Current: 0.082038 A / Power:   0.983722 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 806 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9920 V / Vshunt:  0.0011441 V / Temp:    30.91 C / Current: 0.076257 A / Power:   0.914428 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 807 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9898 V / Vshunt:  0.0011913 V / Temp:    30.91 C / Current: 0.079405 A / Power:   0.952036 W / Energy:  0.0000000... | PASS | 0.015 | benchmark aggregate |
| 808 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9898 V / Vshunt:  0.0011797 V / Temp:    30.96 C / Current: 0.078642 A / Power:   0.942878 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 809 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9947 V / Vshunt:  0.0011806 V / Temp:    30.95 C / Current: 0.078699 A / Power:   0.943916 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 810 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9902 V / Vshunt:  0.0012231 V / Temp:    30.91 C / Current: 0.081523 A / Power:   0.977433 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 811 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9912 V / Vshunt:  0.0011400 V / Temp:    30.95 C / Current: 0.075990 A / Power:   0.911193 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 812 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9934 V / Vshunt:  0.0011822 V / Temp:    30.94 C / Current: 0.078795 A / Power:   0.944954 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 813 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9908 V / Vshunt:  0.0011812 V / Temp:    30.92 C / Current: 0.078737 A / Power:   0.944099 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 814 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9908 V / Vshunt:  0.0011831 V / Temp:    30.93 C / Current: 0.078871 A / Power:   0.945687 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 815 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9928 V / Vshunt:  0.0011894 V / Temp:    30.94 C / Current: 0.079291 A / Power:   0.950876 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 816 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0012234 V / Temp:    30.91 C / Current: 0.081561 A / Power:   0.978044 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 817 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9924 V / Vshunt:  0.0011416 V / Temp:    30.95 C / Current: 0.076085 A / Power:   0.912414 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 818 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9918 V / Vshunt:  0.0011888 V / Temp:    30.94 C / Current: 0.079233 A / Power:   0.950143 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 819 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0011734 V / Temp:    30.94 C / Current: 0.078222 A / Power:   0.937994 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 820 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0011744 V / Temp:    30.92 C / Current: 0.078279 A / Power:   0.938666 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 821 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9924 V / Vshunt:  0.0011919 V / Temp:    30.95 C / Current: 0.079443 A / Power:   0.952708 W / Energy:  0.0000000... | PASS | 0.015 | benchmark aggregate |
| 822 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9908 V / Vshunt:  0.0012253 V / Temp:    30.91 C / Current: 0.081675 A / Power:   0.979326 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 823 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9937 V / Vshunt:  0.0011250 V / Temp:    30.96 C / Current: 0.074998 A / Power:   0.899471 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 824 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9920 V / Vshunt:  0.0011969 V / Temp:    30.91 C / Current: 0.079787 A / Power:   0.956798 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 825 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9902 V / Vshunt:  0.0012009 V / Temp:    30.95 C / Current: 0.080054 A / Power:   0.959851 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 826 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9918 V / Vshunt:  0.0011959 V / Temp:    30.96 C / Current: 0.079710 A / Power:   0.955821 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 827 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9910 V / Vshunt:  0.0011719 V / Temp:    30.96 C / Current: 0.078108 A / Power:   0.936590 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 828 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9912 V / Vshunt:  0.0011694 V / Temp:    30.92 C / Current: 0.077955 A / Power:   0.934758 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 829 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9928 V / Vshunt:  0.0011728 V / Temp:    30.93 C / Current: 0.078184 A / Power:   0.937628 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 830 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0011853 V / Temp:    30.95 C / Current: 0.079004 A / Power:   0.947335 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 831 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0012116 V / Temp:    30.92 C / Current: 0.070724 A / Power:   0.968398 W / Energy:  0.0000000... | PASS | 0.015 | benchmark aggregate |
| 832 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9936 V / Vshunt:  0.0010609 V / Temp:    30.94 C / Current: 0.081561 A / Power:   0.848188 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 833 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9914 V / Vshunt:  0.0012234 V / Temp:    30.94 C / Current: 0.080588 A / Power:   0.977983 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 834 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9898 V / Vshunt:  0.0012091 V / Temp:    30.92 C / Current: 0.079920 A / Power:   0.966200 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 835 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9908 V / Vshunt:  0.0009938 V / Temp:    30.93 C / Current: 0.066241 A / Power:   0.794157 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 836 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9895 V / Vshunt:  0.0012278 V / Temp:    30.94 C / Current: 0.081847 A / Power:   0.794157 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 837 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9926 V / Vshunt:  0.0010737 V / Temp:    30.96 C / Current: 0.071583 A / Power:   0.858444 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 838 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9926 V / Vshunt:  0.0012278 V / Temp:    30.96 C / Current: 0.081847 A / Power:   0.981402 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 839 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9939 V / Vshunt:  0.0011750 V / Temp:    30.91 C / Current: 0.078318 A / Power:   0.939276 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 840 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0012216 V / Temp:    30.93 C / Current: 0.081427 A / Power:   0.976396 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 841 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9922 V / Vshunt:  0.0011409 V / Temp:    30.95 C / Current: 0.076047 A / Power:   0.911925 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 842 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9918 V / Vshunt:  0.0011756 V / Temp:    30.92 C / Current: 0.078375 A / Power:   0.939826 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 843 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9922 V / Vshunt:  0.0011772 V / Temp:    30.92 C / Current: 0.078470 A / Power:   0.940986 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 844 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9937 V / Vshunt:  0.0012259 V / Temp:    30.95 C / Current: 0.081714 A / Power:   0.979998 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 845 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9898 V / Vshunt:  0.0011609 V / Temp:    30.95 C / Current: 0.077383 A / Power:   0.927799 W / Energy:  0.0000000... | PASS | 0.015 | benchmark aggregate |
| 846 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9941 V / Vshunt:  0.0011900 V / Temp:    30.95 C / Current: 0.079329 A / Power:   0.951425 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 847 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9912 V / Vshunt:  0.0011784 V / Temp:    30.92 C / Current: 0.078546 A / Power:   0.941840 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 848 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9922 V / Vshunt:  0.0011919 V / Temp:    30.93 C / Current: 0.079443 A / Power:   0.952647 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 849 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9926 V / Vshunt:  0.0012053 V / Temp:    30.93 C / Current: 0.070362 A / Power:   0.963453 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 850 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0010556 V / Temp:    30.94 C / Current: 0.081485 A / Power:   0.843731 W / Energy:  0.0000000... | PASS | 0.016 | benchmark aggregate |
| 851 | benchmark | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9930 V / Vshunt:  0.0012225 V / Temp:    30.93 C / Current: 0.080569 A / Power:   0.977189 W / Energy:  0.0000000... | PASS | 0.000 | benchmark aggregate |
| 852 | not-run | `<fixture: disconnected target>` | safe absent-device fixture | requires safe disconnect or switched fixture | NOT RUN | 0.000 | requires safe disconnect or switched fixture |
| 853 | not-run | `<fixture: bus fault injection>` | safe fault-injection fixture | requires safe NACK/timeout/bus-error injection | NOT RUN | 0.000 | requires safe NACK/timeout/bus-error injection |
| 854 | not-run | `<fixture: alert pin capture>` | alert pin instrumentation | requires alert-pin wiring and safe threshold stimulus | NOT RUN | 0.000 | requires alert-pin wiring and safe threshold stimulus |
| 855 | not-run | `<fixture: MCU reset or power cycle>` | controlled reset/power fixture | requires explicit reset/power-cycle control | NOT RUN | 0.000 | requires explicit reset/power-cycle control |
| 856 | not-run | `<8-hour soak>` | --soak-hours 8 | soak not requested for this run | NOT RUN | 0.000 | soak not requested for this run |

## Limitations

- Hardware safety and fixture details must be filled in by the operator.
- This runner records serial CLI evidence only; external instruments must be logged separately.
- Staged `maxInstructions` coverage is limited to the example CLI commands. The `transfer` suite records example callback counts, not logic-analyzer bus bytes. Example `tick()` calls between serial commands can add readiness reads; exact assertions are kept to deterministic paths and other paths record snapshots.
- Soak test was not requested in this run.
