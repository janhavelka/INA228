# INA228 HIL Validation Report

- Date/time: 2026-07-31T13:10:11.134752+02:00 to 2026-07-31T13:11:14.423349+02:00
- Elapsed: 63.3 s
- Port: COM4
- Baud: 115200
- Suite: smoke
- Soak requested: 60.0 s
- Operator: Codex
- Board/environment: ESP32-S3 QFN56 rev 0.1, 4 MB flash, 2 MB PSRAM, USB Serial/JTAG / PIOArduino 55.03.311; Arduino-ESP32 3.3.11; ESP-IDF 5.5.5
- Fixture: INA228 at 0x41; 15 mOhm shunt; low-voltage connected bench fixture
- Safety assumptions: Low-voltage bench run only; no high-voltage validation performed
- OS: Windows-11-10.0.26200-SP0
- Python: 3.12.10
- HIL command: `tools/run_i2c_hil.py --port COM4 --suite smoke --require-framed --fail-on-unknown --include-not-run --soak-seconds 60 --soak-store-every 25 --soak-progress-every 25 --timeout-s 12 --report docs/validation/hardware/2026-07-31/4c32312-dirty-pioarduino-55.03.311-esp32s3/soak-60s-report.md --transcript docs/validation/hardware/2026-07-31/4c32312-dirty-pioarduino-55.03.311-esp32s3/soak-60s-transcript.txt --operator Codex --board ESP32-S3 QFN56 rev 0.1, 4 MB flash, 2 MB PSRAM, USB Serial/JTAG --environment PIOArduino 55.03.311; Arduino-ESP32 3.3.11; ESP-IDF 5.5.5 --fixture INA228 at 0x41; 15 mOhm shunt; low-voltage connected bench fixture --safety Low-voltage bench run only; no high-voltage validation performed --notes 60-second shakedown soak; this is not the documented 8-hour release soak.`
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
- Notes: 60-second shakedown soak; this is not the documented 8-hour release soak.

## Summary

| PASS | FAIL | UNKNOWN | NOT RUN |
| ---: | ---: | ---: | ---: |
| 5940 | 0 | 0 | 4 |

## Timing Summary

- Commands executed: 5944
- Commands recorded in detail: 249
- Soak commands executed: 5932
- Soak rows recorded in detail: 237
- Recorded command latency min/mean/max: 0.000 / 0.010 / 0.125 s

- Maximum consecutive FAIL verdicts: 0

## Steps

| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| 1 | smoke | `version` | Arduino-ESP32: 3.3.11, ESP-IDF: v5.5.5, INA228 library version | === Version Info === / Example firmware build: Jul 31 2026 13:06:21 / MCU: ESP32-S3 rev 1, flash 4194304 bytes, PSRAM ready (2097152 bytes) / Arduino-ESP32: ... | PASS | 0.000 | version and framework stack |
| 2 | smoke | `scan` | INA228 Address Probe, Healthy INA228 devices | [I] Scanning I2C bus (timeout=50ms)... / [I]      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F / 00:                         -- -- -- -- -- -- -- -- / 10: ... | PASS | 0.125 | scan |
| 3 | smoke | `init` | initialize, OK | [I] bind + initialize(0x41): OK / [runner] frame_status=OK frame_elapsed_ms=14 | PASS | 0.016 | initialize discovered INA228 |
| 4 | smoke | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | probe |
| 5 | smoke | `settings` | Active Settings, State:, Address: | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.016 | settings |
| 6 | smoke | `drv` | Driver Health, State:, Online: | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 0 / Total failures: 0 / Success rate... | PASS | 0.000 | health |
| 7 | smoke | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | DIAG_ALRT |
| 8 | smoke | `raw` | Raw Registers, Vbus, Temp | === Raw Registers === / Vshunt: 3575 (0x000DF7) / Vbus:   61394 (0x00EFD2) / Temp:   3972 (0x0F84) / Current:3903 (0x000F3F) / Power:  14625 (0x003921) / Ene... | PASS | 0.000 | conversion raw read |
| 9 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9918 V / Vshunt:  0.0011809 V / Temp:    31.05 C / Current: 0.078718 A / Power:   0.943916 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 10 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9904 avg=11.9918 ... | PASS | 0.078 | soak stress |
| 11 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3717 (0x000E85) / Vbus:   61408 (0x00EFE0) / Temp:   3972 (0x0F84) / Current:4058 (0x000FDA) / Power:  15209 (0x003B69) / Ene... | PASS | 0.000 | soak raw |
| 12 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | soak verified reinitialization |
| 13 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 371 request: 371 generation: 8 / Bus:     11993 mV / Shunt:   895 uV / Temp:    31063 mdegC / Current: ... | PASS | 0.016 | soak atomic sample |
| 14 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 15 | soak | `power` | Power | [I] Power: 0.993734 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 16 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 17 | soak | `current` | Current | [I] Current: 0.087036 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 18 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   219215 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 19 | soak | `temp` | Temp | [I] Temp: 31.05 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak temperature |
| 20 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 9069 / Total failures: 0 / Success r... | PASS | 0.000 | soak health |
| 21 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0012213 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak shunt voltage |
| 22 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 23 | soak | `vbus` | Vbus | [I] Vbus: 11.9922 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 24 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 25 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.031 | soak mixed stress |
| 26 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9914 V / Vshunt:  0.0011847 V / Temp:    31.03 C / Current: 0.078966 A / Power:   0.946908 W / Energy:  0.0000000... | PASS | 0.015 | soak aggregate |
| 27 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9896 avg=11.9915 ... | PASS | 0.079 | soak stress |
| 28 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3691 (0x000E6B) / Vbus:   61403 (0x00EFDB) / Temp:   3971 (0x0F83) / Current:4030 (0x000FBE) / Power:  15103 (0x003AFF) / Ene... | PASS | 0.000 | soak raw |
| 29 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | soak verified reinitialization |
| 30 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 421 request: 421 generation: 33 / Bus:     11993 mV / Shunt:   899 uV / Temp:    31031 mdegC / Current:... | PASS | 0.016 | soak atomic sample |
| 31 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.016 | soak probe |
| 32 | soak | `power` | Power | [I] Power: 1.004540 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | soak power |
| 33 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | soak raw diagnostics |
| 34 | soak | `current` | Current | [I] Current: 0.097320 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 35 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   223524 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 36 | soak | `temp` | Temp | [I] Temp: 31.01 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | soak temperature |
| 37 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 22344 / Total failures: 0 / Success ... | PASS | 0.016 | soak health |
| 38 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0011756 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.015 | soak shunt voltage |
| 39 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.016 | soak settings |
| 40 | soak | `vbus` | Vbus | [I] Vbus: 11.9912 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 41 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 42 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.031 | soak mixed stress |
| 43 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9916 V / Vshunt:  0.0011812 V / Temp:    30.98 C / Current: 0.078737 A / Power:   0.944160 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 44 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9904 avg=11.9920 ... | PASS | 0.078 | soak stress |
| 45 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3689 (0x000E69) / Vbus:   61401 (0x00EFD9) / Temp:   3968 (0x0F80) / Current:4028 (0x000FBC) / Power:  15095 (0x003AF7) / Ene... | PASS | 0.000 | soak raw |
| 46 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | soak verified reinitialization |
| 47 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 471 request: 471 generation: 58 / Bus:     11995 mV / Shunt:   943 uV / Temp:    31008 mdegC / Current:... | PASS | 0.015 | soak atomic sample |
| 48 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 49 | soak | `power` | Power | [I] Power: 0.973953 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 50 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 51 | soak | `current` | Current | [I] Current: 0.090852 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 52 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   227824 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 53 | soak | `temp` | Temp | [I] Temp: 31.00 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak temperature |
| 54 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 35619 / Total failures: 0 / Success ... | PASS | 0.000 | soak health |
| 55 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0011763 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak shunt voltage |
| 56 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 57 | soak | `vbus` | Vbus | [I] Vbus: 11.9910 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | soak bus voltage |
| 58 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 59 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.032 | soak mixed stress |
| 60 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9918 V / Vshunt:  0.0011816 V / Temp:    31.00 C / Current: 0.078756 A / Power:   0.944405 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 61 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9891 avg=11.9915 ... | PASS | 0.078 | soak stress |
| 62 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3687 (0x000E67) / Vbus:   61404 (0x00EFDC) / Temp:   3968 (0x0F80) / Current:4026 (0x000FBA) / Power:  15088 (0x003AF0) / Ene... | PASS | 0.015 | soak raw |
| 63 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.015 | soak verified reinitialization |
| 64 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 521 request: 521 generation: 83 / Bus:     11993 mV / Shunt:   1012 uV / Temp:    30992 mdegC / Current... | PASS | 0.031 | soak atomic sample |
| 65 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 66 | soak | `power` | Power | [I] Power: 0.926211 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | soak power |
| 67 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | soak raw diagnostics |
| 68 | soak | `current` | Current | [I] Current: 0.067424 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | soak current |
| 69 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   232135 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 70 | soak | `temp` | Temp | [I] Temp: 31.00 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.015 | soak temperature |
| 71 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 48894 / Total failures: 0 / Success ... | PASS | 0.000 | soak health |
| 72 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0012234 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak shunt voltage |
| 73 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 74 | soak | `vbus` | Vbus | [I] Vbus: 11.9916 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 75 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 76 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.031 | soak mixed stress |
| 77 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9928 V / Vshunt:  0.0011769 V / Temp:    30.99 C / Current: 0.078451 A / Power:   0.940803 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 78 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 77 ms / Rate: 649.35 samples/s / Vbus V:    min=11.9904 avg=11.9918 ... | PASS | 0.078 | soak stress |
| 79 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3693 (0x000E6D) / Vbus:   61409 (0x00EFE1) / Temp:   3966 (0x0F7E) / Current:4032 (0x000FC0) / Power:  15112 (0x003B08) / Ene... | PASS | 0.000 | soak raw |
| 80 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.015 | soak verified reinitialization |
| 81 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 571 request: 571 generation: 108 / Bus:     11992 mV / Shunt:   931 uV / Temp:    30977 mdegC / Current... | PASS | 0.016 | soak atomic sample |
| 82 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 83 | soak | `power` | Power | [I] Power: 0.983661 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 84 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 85 | soak | `current` | Current | [I] Current: 0.077211 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 86 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   236434 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 87 | soak | `temp` | Temp | [I] Temp: 30.97 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak temperature |
| 88 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 62169 / Total failures: 0 / Success ... | PASS | 0.000 | soak health |
| 89 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0011766 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak shunt voltage |
| 90 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 91 | soak | `vbus` | Vbus | [I] Vbus: 11.9920 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 92 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 93 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.047 | soak mixed stress |
| 94 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9908 V / Vshunt:  0.0011809 V / Temp:    30.98 C / Current: 0.078718 A / Power:   0.943855 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 95 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9900 avg=11.9920 ... | PASS | 0.078 | soak stress |
| 96 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3721 (0x000E89) / Vbus:   61405 (0x00EFDD) / Temp:   3965 (0x0F7D) / Current:4063 (0x000FDF) / Power:  15227 (0x003B7B) / Ene... | PASS | 0.000 | soak raw |
| 97 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.015 | soak verified reinitialization |
| 98 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 621 request: 621 generation: 133 / Bus:     11992 mV / Shunt:   1008 uV / Temp:    30977 mdegC / Curren... | PASS | 0.016 | soak atomic sample |
| 99 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 100 | soak | `power` | Power | [I] Power: 0.965650 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 101 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 102 | soak | `current` | Current | [I] Current: 0.081580 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 103 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   240732 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 104 | soak | `temp` | Temp | [I] Temp: 30.99 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak temperature |
| 105 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 75444 / Total failures: 0 / Success ... | PASS | 0.000 | soak health |
| 106 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0012250 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak shunt voltage |
| 107 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 108 | soak | `vbus` | Vbus | [I] Vbus: 11.9906 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 109 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 110 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.047 | soak mixed stress |
| 111 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9914 V / Vshunt:  0.0011881 V / Temp:    30.98 C / Current: 0.079195 A / Power:   0.949655 W / Energy:  0.0000000... | PASS | 0.016 | soak aggregate |
| 112 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 77 ms / Rate: 649.35 samples/s / Vbus V:    min=11.9891 avg=11.9915 ... | PASS | 0.079 | soak stress |
| 113 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3686 (0x000E66) / Vbus:   61402 (0x00EFDA) / Temp:   3966 (0x0F7E) / Current:4025 (0x000FB9) / Power:  15084 (0x003AEC) / Ene... | PASS | 0.000 | soak raw |
| 114 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | soak verified reinitialization |
| 115 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 671 request: 671 generation: 158 / Bus:     11993 mV / Shunt:   1015 uV / Temp:    30969 mdegC / Curren... | PASS | 0.016 | soak atomic sample |
| 116 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.016 | soak probe |
| 117 | soak | `power` | Power | [I] Power: 1.002953 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 118 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 119 | soak | `current` | Current | [I] Current: 0.081714 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 120 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   245037 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 121 | soak | `temp` | Temp | [I] Temp: 31.00 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak temperature |
| 122 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 88719 / Total failures: 0 / Success ... | PASS | 0.000 | soak health |
| 123 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0010741 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak shunt voltage |
| 124 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 125 | soak | `vbus` | Vbus | [I] Vbus: 11.9891 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 126 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 127 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.032 | soak mixed stress |
| 128 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9914 V / Vshunt:  0.0011825 V / Temp:    31.00 C / Current: 0.078833 A / Power:   0.945259 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 129 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 77 ms / Rate: 649.35 samples/s / Vbus V:    min=11.9896 avg=11.9916 ... | PASS | 0.078 | soak stress |
| 130 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3692 (0x000E6C) / Vbus:   61393 (0x00EFD1) / Temp:   3967 (0x0F7F) / Current:4031 (0x000FBF) / Power:  15104 (0x003B00) / Ene... | PASS | 0.000 | soak raw |
| 131 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | soak verified reinitialization |
| 132 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 721 request: 721 generation: 183 / Bus:     11994 mV / Shunt:   950 uV / Temp:    30992 mdegC / Current... | PASS | 0.016 | soak atomic sample |
| 133 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 134 | soak | `power` | Power | [I] Power: 1.067973 W / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak power |
| 135 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 136 | soak | `current` | Current | [I] Current: 0.083679 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 137 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   249342 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 138 | soak | `temp` | Temp | [I] Temp: 30.99 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak temperature |
| 139 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 101994 / Total failures: 0 / Success... | PASS | 0.000 | soak health |
| 140 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0012222 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak shunt voltage |
| 141 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 142 | soak | `vbus` | Vbus | [I] Vbus: 11.9916 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 143 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 144 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.031 | soak mixed stress |
| 145 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9898 V / Vshunt:  0.0011881 V / Temp:    31.01 C / Current: 0.079195 A / Power:   0.949533 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 146 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9891 avg=11.9917 ... | PASS | 0.079 | soak stress |
| 147 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3702 (0x000E76) / Vbus:   61407 (0x00EFDF) / Temp:   3966 (0x0F7E) / Current:4042 (0x000FCA) / Power:  15149 (0x003B2D) / Ene... | PASS | 0.000 | soak raw |
| 148 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | soak verified reinitialization |
| 149 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 771 request: 771 generation: 208 / Bus:     11992 mV / Shunt:   1001 uV / Temp:    31016 mdegC / Curren... | PASS | 0.016 | soak atomic sample |
| 150 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 151 | soak | `power` | Power | [I] Power: 0.982256 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 152 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 153 | soak | `current` | Current | [I] Current: 0.084327 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 154 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   253655 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 155 | soak | `temp` | Temp | [I] Temp: 31.01 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak temperature |
| 156 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 115269 / Total failures: 0 / Success... | PASS | 0.000 | soak health |
| 157 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0011822 V / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak shunt voltage |
| 158 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 159 | soak | `vbus` | Vbus | [I] Vbus: 11.9920 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 160 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 161 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.047 | soak mixed stress |
| 162 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9914 V / Vshunt:  0.0011922 V / Temp:    31.00 C / Current: 0.079462 A / Power:   0.952830 W / Energy:  0.0000000... | PASS | 0.016 | soak aggregate |
| 163 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9898 avg=11.9917 ... | PASS | 0.078 | soak stress |
| 164 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3723 (0x000E8B) / Vbus:   61392 (0x00EFD0) / Temp:   3970 (0x0F82) / Current:4065 (0x000FE1) / Power:  15231 (0x003B7F) / Ene... | PASS | 0.015 | soak raw |
| 165 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | soak verified reinitialization |
| 166 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 821 request: 821 generation: 233 / Bus:     11993 mV / Shunt:   900 uV / Temp:    31016 mdegC / Current... | PASS | 0.015 | soak atomic sample |
| 167 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.016 | soak probe |
| 168 | soak | `power` | Power | [I] Power: 0.972000 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 169 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | soak raw diagnostics |
| 170 | soak | `current` | Current | [I] Current: 0.087532 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 171 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   257958 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 172 | soak | `temp` | Temp | [I] Temp: 31.00 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak temperature |
| 173 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 128544 / Total failures: 0 / Success... | PASS | 0.000 | soak health |
| 174 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0011828 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak shunt voltage |
| 175 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 176 | soak | `vbus` | Vbus | [I] Vbus: 11.9920 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 177 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 178 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.032 | soak mixed stress |
| 179 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9930 V / Vshunt:  0.0011919 V / Temp:    31.00 C / Current: 0.079443 A / Power:   0.952708 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 180 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9906 avg=11.9920 ... | PASS | 0.078 | soak stress |
| 181 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3731 (0x000E93) / Vbus:   61393 (0x00EFD1) / Temp:   3968 (0x0F80) / Current:4074 (0x000FEA) / Power:  15265 (0x003BA1) / Ene... | PASS | 0.000 | soak raw |
| 182 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.015 | soak verified reinitialization |
| 183 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 871 request: 871 generation: 258 / Bus:     11992 mV / Shunt:   960 uV / Temp:    31000 mdegC / Current... | PASS | 0.016 | soak atomic sample |
| 184 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 185 | soak | `power` | Power | [I] Power: 0.937872 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 186 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 187 | soak | `current` | Current | [I] Current: 0.096080 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 188 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   262263 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 189 | soak | `temp` | Temp | [I] Temp: 31.00 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak temperature |
| 190 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 141819 / Total failures: 0 / Success... | PASS | 0.000 | soak health |
| 191 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0010891 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak shunt voltage |
| 192 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 193 | soak | `vbus` | Vbus | [I] Vbus: 11.9918 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 194 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 195 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.031 | soak mixed stress |
| 196 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9906 V / Vshunt:  0.0011866 V / Temp:    31.01 C / Current: 0.079100 A / Power:   0.948434 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 197 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9900 avg=11.9917 ... | PASS | 0.078 | soak stress |
| 198 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3716 (0x000E84) / Vbus:   61403 (0x00EFDB) / Temp:   3968 (0x0F80) / Current:4057 (0x000FD9) / Power:  15204 (0x003B64) / Ene... | PASS | 0.000 | soak raw |
| 199 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | soak verified reinitialization |
| 200 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 921 request: 921 generation: 283 / Bus:     11993 mV / Shunt:   1004 uV / Temp:    31008 mdegC / Curren... | PASS | 0.015 | soak atomic sample |
| 201 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 202 | soak | `power` | Power | [I] Power: 0.992025 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 203 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 204 | soak | `current` | Current | [I] Current: 0.081923 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.016 | soak current |
| 205 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   266571 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 206 | soak | `temp` | Temp | [I] Temp: 31.00 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | soak temperature |
| 207 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 155094 / Total failures: 0 / Success... | PASS | 0.000 | soak health |
| 208 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0012244 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.015 | soak shunt voltage |
| 209 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.015 | soak settings |
| 210 | soak | `vbus` | Vbus | [I] Vbus: 11.9904 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 211 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | soak readiness |
| 212 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.031 | soak mixed stress |
| 213 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9918 V / Vshunt:  0.0011812 V / Temp:    31.01 C / Current: 0.078737 A / Power:   0.944160 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 214 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9904 avg=11.9921 ... | PASS | 0.078 | soak stress |
| 215 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3692 (0x000E6C) / Vbus:   61406 (0x00EFDE) / Temp:   3967 (0x0F7F) / Current:4031 (0x000FBF) / Power:  15107 (0x003B03) / Ene... | PASS | 0.000 | soak raw |
| 216 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.016 | soak verified reinitialization |
| 217 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 971 request: 971 generation: 308 / Bus:     11993 mV / Shunt:   1008 uV / Temp:    31000 mdegC / Curren... | PASS | 0.015 | soak atomic sample |
| 218 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 219 | soak | `power` | Power | [I] Power: 0.804902 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 220 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak raw diagnostics |
| 221 | soak | `current` | Current | [I] Current: 0.081771 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 222 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   270873 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 223 | soak | `temp` | Temp | [I] Temp: 31.00 C / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak temperature |
| 224 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 168369 / Total failures: 0 / Success... | PASS | 0.000 | soak health |
| 225 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0010822 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak shunt voltage |
| 226 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 227 | soak | `vbus` | Vbus | [I] Vbus: 11.9920 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 228 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 229 | soak | `stress_mix 50` | stress_mix summary, fail= | === stress_mix summary === / Total: ok=50 fail=0 (100.00%) / Duration: 32 ms / Rate: 1562.50 ops/s / measure    ok=8 fail=0 / vbus       ok=7 fail=0 / curren... | PASS | 0.031 | soak mixed stress |
| 230 | soak | `read` | Vbus, Power | [I] Reading all measurements: / Vbus:    11.9906 V / Vshunt:  0.0011928 V / Temp:    30.99 C / Current: 0.079519 A / Power:   0.953440 W / Energy:  0.0000000... | PASS | 0.000 | soak aggregate |
| 231 | soak | `stress 50` | Stress Summary, Errors: | === Stress Summary === / Target: 50 / Attempts: 50 / Success: 50 / Errors: 0 / Duration: 76 ms / Rate: 657.89 samples/s / Vbus V:    min=11.9898 avg=11.9918 ... | PASS | 0.078 | soak stress |
| 232 | soak | `raw` | Raw Registers, Vbus | === Raw Registers === / Vshunt: 3698 (0x000E72) / Vbus:   61393 (0x00EFD1) / Temp:   3967 (0x0F7F) / Current:4038 (0x000FC6) / Power:  15130 (0x003B1A) / Ene... | PASS | 0.000 | soak raw |
| 233 | soak | `recover` | Invalidating cached hardware state, Status: OK | [I] Invalidating cached hardware state and reinitializing... / Status: OK (code=0, detail=0) / Message: OK / === Driver Health === / Configured address: 0x41... | PASS | 0.015 | soak verified reinitialization |
| 234 | soak | `integer` | Cooperative Instantaneous Sample | === Cooperative Instantaneous Sample === / Operation: 1021 request: 1021 generation: 333 / Bus:     11991 mV / Shunt:   1005 uV / Temp:    31016 mdegC / Curr... | PASS | 0.016 | soak atomic sample |
| 235 | soak | `probe` | Status: OK | [I] Probing address 0x41 (raw, no health tracking; reads DIAG_ALRT)... / [W] DIAG_ALRT reads can clear CNVRF and latched evidence. / Status: OK (code=0, deta... | PASS | 0.000 | soak probe |
| 236 | soak | `power` | Power | [I] Power: 0.925784 W / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak power |
| 237 | soak | `diagraw` | DIAG_ALRT raw | [I] DIAG_ALRT raw: 0x0003 / [W] DIAG_ALRT reads are destructive/status-clearing. / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.016 | soak raw diagnostics |
| 238 | soak | `current` | Current | [I] Current: 0.081599 A / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak current |
| 239 | soak | `diagsnap` | DIAG_ALRT Snapshot | === DIAG_ALRT Snapshot === / Note: cache-only; this command does not touch I2C. / Valid:      yes / Raw:        0x0003 / Captured:   275177 ms / MEMSTAT:    ... | PASS | 0.000 | soak diagnostic snapshot |
| 240 | soak | `temp` | Temp | [I] Temp: 31.00 C / [runner] frame_status=OK frame_elapsed_ms=1 | PASS | 0.000 | soak temperature |
| 241 | soak | `drv` | Driver Health | === Driver Health === / Configured address: 0x41 / State: READY / Online: yes / Consecutive failures: 0 / Total success: 181644 / Total failures: 0 / Success... | PASS | 0.000 | soak health |
| 242 | soak | `vshunt` | Vshunt | [I] Vshunt: 0.0010759 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak shunt voltage |
| 243 | soak | `settings` | Active Settings | === Active Settings === / Initialized:      yes / State:            READY / Address:          0x41 / I2C timeout:      50 ms / Offline threshold:5 / nowMs ho... | PASS | 0.000 | soak settings |
| 244 | soak | `vbus` | Vbus | [I] Vbus: 11.9908 V / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak bus voltage |
| 245 | soak | `ready` | Conversion ready | [I] Conversion ready: yes / [runner] frame_status=OK frame_elapsed_ms=0 | PASS | 0.000 | soak readiness |
| 246 | not-run | `<fixture: disconnected target>` | safe absent-device fixture | requires safe disconnect or switched fixture | NOT RUN | 0.000 | requires safe disconnect or switched fixture |
| 247 | not-run | `<fixture: bus fault injection>` | safe fault-injection fixture | requires safe NACK/timeout/bus-error injection | NOT RUN | 0.000 | requires safe NACK/timeout/bus-error injection |
| 248 | not-run | `<fixture: alert pin capture>` | alert pin instrumentation | requires alert-pin wiring and safe threshold stimulus | NOT RUN | 0.000 | requires alert-pin wiring and safe threshold stimulus |
| 249 | not-run | `<fixture: MCU reset or power cycle>` | controlled reset/power fixture | requires explicit reset/power-cycle control | NOT RUN | 0.000 | requires explicit reset/power-cycle control |

## Soak Summary

- Requested duration: 60.0 s
- Executed soak commands: 5932
- Recorded soak rows: 237
- Soak PASS row storage stride: every 25 PASS row(s), plus all FAIL/UNKNOWN rows
- Soak verdict counts: PASS=5932, FAIL=0, UNKNOWN=0
- Empty framed response retries: 0
- Soak latency min/mean/max: 0.000 / 0.010 / 0.094 s
- Command mix:
  - `current`: 349
  - `diagraw`: 349
  - `diagsnap`: 349
  - `drv`: 349
  - `integer`: 349
  - `power`: 349
  - `probe`: 349
  - `raw`: 349
  - `read`: 349
  - `ready`: 349
  - `recover`: 349
  - `settings`: 349
  - `stress 50`: 349
  - `stress_mix 50`: 348
  - `temp`: 349
  - `vbus`: 349
  - `vshunt`: 349

## Limitations

- Hardware safety and fixture details must be filled in by the operator.
- This runner records serial CLI evidence only; external instruments must be logged separately.
- Staged `maxInstructions` coverage is limited to the example CLI commands. The `transfer` suite records example callback counts, not logic-analyzer bus bytes. Example `tick()` calls between serial commands can add readiness reads; exact assertions are kept to deterministic paths and other paths record snapshots.
