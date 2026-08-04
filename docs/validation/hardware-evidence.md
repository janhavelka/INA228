# Hardware Evidence Summary

This file summarizes dated HIL evidence without upgrading its claim level.
Current detailed reports remain under `docs/validation/hardware/`; older report
summaries are retained below.

## Current v3 Migration Evidence

On 2026-07-31, physical board serial `24:58:7C:DB:DB:AC` was addressed initially
as COM3 and re-enumerated as COM4 after the ESP32-S3 USB boot reset. Runtime
output confirmed ESP32-S3 revision 1, 4 MB flash, 2 MB PSRAM, Arduino-ESP32
3.3.11, and ESP-IDF v5.5.5. The INA228 was identified at `0x41` with
manufacturer ID `0x5449`, device ID `0x2281`, and healthy MEMSTAT.

| Run | Base commit/worktree | Duration | PASS | FAIL | UNKNOWN | NOT RUN | Notes |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| PIOArduino 55.03.311 exhaustive + benchmark | `4c32312` + dirty migration changes | 9.0 s | 851 | 0 | 0 | 5 | Full synchronous and cooperative feature sweep, exact transfer budgets, 100 iterations across six read paths, 1,000 measurement samples, and 1,000 mixed operations. |
| PIOArduino 55.03.311 shakedown soak | `4c32312` + dirty migration changes | 63.3 s | 5,940 | 0 | 0 | 4 | Eight smoke checks plus 5,932 soak commands over the requested 60-second interval; not the release-gate 8-hour soak. |

Reports and full serial transcripts:
`hardware/2026-07-31/4c32312-dirty-pioarduino-55.03.311-esp32s3/`.

This evidence used a low-voltage connected fixture. It did not include
controlled fault injection, ALERT-pin capture, reference-instrument accuracy
measurements, controlled MCU/INA228 power cycling, the alternate low-current
profile needed to exercise ADCRANGE=1 physically, ESP32-S2/ESP-IDF physical
runs, or a clean 8-hour soak. It is not release-grade hardware validation.

## Historical v2 Evidence

All entries below predate the v3 cooperative owner contract. They are historical
v2 evidence only and must not be cited as v3 hardware validation.

Fixture: `INA228_0x41_low_voltage_no_fault_injection`.

| Run | Commit | Duration | PASS | FAIL | UNKNOWN | NOT RUN | Notes |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 2026-06-22 exhaustive low-voltage run | `5840497166200a3b1eb9e8f68f447b6d7182367a` | 28913.0 s | 256931 | 0 | 48 | 4 | 8-hour run; UNKNOWN rows were serial/HIL framing loss; final health READY with zero consecutive failures. |
| 2026-06-23 targeted feature sweep | `29228f1122dfd0cb3bc8d82c5b92535a2f042ca8` | not retained | 196 | 0 | 0 | 5 | Framed sweep covered modes, conversion times, averaging, ADC range, delay/temp compensation, calibration, alerts, staged jobs, trigger, accumulator reset, raw reads, invalid input, and end/reinit. |
| 2026-06-23 transfer-count suite | `29228f1122dfd0cb3bc8d82c5b92535a2f042ca8` | not retained | 53 | 0 | 0 | 5 | Validated example callback transfer counts for deterministic fixed-step paths; not logic-analyzer bus-byte evidence. |
| 2026-06-23 targeted stress | `5840497166200a3b1eb9e8f68f447b6d7182367a` | 382.8 s | 6787 | 0 | 1 | 4 | Requested 600 s stress stopped early after one UNKNOWN serial framing row. |
| 2026-06-23 20-hour soak attempt | `851ac4c` | 14159.7 s | 128416 | 0 | 1 | 4 | 20-hour request did not complete; one UNKNOWN serial framing row; not clean soak evidence. |

## Release-Grade Evidence Still Required

- Clean git status before build and upload.
- Clean-commit framed exhaustive suite with `0 FAIL` and `0 UNKNOWN`, including
  the feature sweep and exact fixed-step callback-budget assertions.
- Framed 8-hour soak with `0 FAIL` and `0 UNKNOWN`.
- Fault-injection fixture coverage for address NACK, data/unknown-phase NACK,
  timeout, stuck-bus/bus error, OFFLINE behavior, and recovery.
- ALERT-pin capture for polarity, latch/transparent behavior, conversion-ready
  routing, and safe threshold crossings.
- Controlled MCU reset and INA228 power-cycle evidence.
- Reviewed CI logs for the final revision under test. The v3.0.2 release commit
  satisfied this software gate in
  [CI run 30815246708](https://github.com/janhavelka/INA228/actions/runs/30815246708);
  the current Unreleased cleanup worktree has not yet run in CI.

## Serial/HIL Runner Finding

Historical v2 host-driven runs exposed serial framing fragility; those UNKNOWN
rows did not prove a core library or I2C transaction failure. The v3 runner now
uses framed `hilrun` commands, bounded writes, explicit DTR/RTS handling, and
exact frame-status parsing. Those controls produced zero UNKNOWN rows in the
short migration runs, but they still require a clean eight-hour proof.
