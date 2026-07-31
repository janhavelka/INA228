# PIOArduino 55.03.311 migration review

Date: 2026-07-31

This review covers the Arduino PlatformIO migration from
`platformio/espressif32@7.0.1` to the versioned PIOArduino `55.03.311` release
asset. It does not change the native ESP-IDF example's independently managed
SDK/CI version.

## Version delta

| Component | Previous Arduino build stack | Current pinned stack |
| --- | --- | --- |
| PlatformIO Core | earlier local/CI installation | 6.1.19 |
| Platform | PlatformIO Espressif32 7.0.1 | PIOArduino 55.03.311 |
| Arduino-ESP32 | 2.0.17 | 3.3.11 |
| ESP-IDF under Arduino | 4.4.7 | 5.5.5 |
| Xtensa toolchain | older 8.4-era package | GCC 14.2.0 package reported by the build |
| esptool | pre-v5 | 5.3.0 |

Primary references:

- [PIOArduino 55.03.311 release](https://github.com/pioarduino/platform-espressif32/releases/tag/55.03.311)
- [Arduino-ESP32 3.3.11 release](https://github.com/espressif/arduino-esp32/releases/tag/3.3.11)
- [Arduino-ESP32 2.x to 3.0 migration guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/2.x_to_3.0.html)
- [esptool v5 migration guide](https://docs.espressif.com/projects/esptool/en/latest/esp32p4/migration-guide.html)

## Compatibility audit and actions

| Area reviewed | INA228 exposure | Result/action |
| --- | --- | --- |
| Core library headers and implementation | Injected function-pointer transport only; no Arduino, Wire, ESP-IDF, FreeRTOS, USB, or platform delay dependency | No framework compatibility shim required. Native tests remain the primary core regression layer. |
| Arduino Wire adapter | `begin`, `setClock`, `setTimeOut`, `beginTransmission`, `write`, `endTransmission`, `requestFrom`, `available`, and `read`; INA228 transfers are at most five data bytes | Source-compatible on Arduino-ESP32 3.3.11. S3 HIL passed identity, all register widths, stress, recovery, and exact callback-count checks. Existing bounded length/error checks remain. |
| I2C ownership and timeout policy | Wire exists only in `examples/common`; core receives precise mapped `Status` values | Ownership contract is unchanged. No Arduino/IDF error type leaks into the core. |
| USB CDC / serial HIL | Native USB CDC on the S3 and framed serial commands | Runtime framework evidence was added to `version`. The host runner now opens the port with DTR/RTS deasserted and a bounded write timeout, avoiding unintended reset/control-line behavior. |
| ESP32-S3 memory configuration | Physical target has 4 MB embedded flash and 2 MB QSPI PSRAM | Added explicit `qio_qspi` Arduino memory type and `qspi` PSRAM type, retained explicit 4 MB flash, and removed the obsolete classic-ESP32 `-mfix-esp32-psram-cache-issue` flag. Runtime reported all 2 MB PSRAM available. |
| ESP32-S2 upload reset | esptool 5 option spelling | Changed `no_reset_stub` to `no-reset-stub`. |
| Compiler/language | C++17 library and example compiled by GCC 14.2.0 | Both S2 and S3 builds pass. Existing warnings remain enabled; no source workaround was necessary. |
| Calibration under the new HIL firmware | Documented 15 mOhm/10 A fixed-unit profile | Hardware initialization exposed downward SHUNT_CAL quantization. The planner now uses the signed 20-bit positive limit and rounds the derived calibration upward; native regression and physical initialization pass. |
| HIL coverage | The old exhaustive selector omitted a legacy broad sweep and used stale pre-v3 cooperative expectations | The exhaustive suite now includes the broad synchronous sweep, v3 job semantics, exact transfer-budget suite, 1,000-sample and 1,000-mixed-operation stress, and runtime framework checks. |

## Validation performed

- Static CLI, owner, timing, native-IDF-example, and HIL parser contracts passed.
- Native suite passed 101/101 tests.
- Arduino ESP32-S3 and ESP32-S2 builds passed on the pinned stack.
- The S3 firmware upload completed with esptool 5.3.0, whose console reported
  successful data-hash verification. The upload console log was not retained.
- Runtime confirmed Arduino-ESP32 3.3.11 and ESP-IDF v5.5.5.
- Exhaustive/benchmark HIL recorded 851 PASS, 0 FAIL, 0 UNKNOWN, four explicit
  fixture/tooling NOT RUN rows, and one NOT RUN row for the 8-hour soak.
- The separate 60-second shakedown recorded 5,940 PASS, 0 FAIL, 0 UNKNOWN,
  including 5,932 soak commands.

Detailed reports and transcripts are under
`docs/validation/hardware/2026-07-31/4c32312-dirty-pioarduino-55.03.311-esp32s3/`.

## Limits of this evidence

The HIL worktree was intentionally dirty with the migration changes under test.
No physical S2 or native ESP-IDF run was performed. The connected 15 mOhm/10 A
profile correctly rejects the INA228 low shunt range because 10 A would exceed
40.96 mV; low-range register/scaling vectors are native-tested but require a
separate compatible physical calibration profile. Controlled address/data NACK,
timeout/stuck-bus injection, ALERT-pin stimulus/capture, reference-instrument
accuracy comparison, controlled MCU/device power cycles, high-voltage testing,
and the clean 8-hour release soak remain open gates.
