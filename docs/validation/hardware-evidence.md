# Hardware Evidence Summary

This file preserves the release-relevant facts from generated HIL reports that
were removed during repository cleanup. The retained evidence is useful, but it
does not upgrade the claim level: all runs used a low-voltage Arduino
ESP32-S3/COM21 fixture, were produced from dirty worktrees, and did not include
fault injection, ALERT-pin capture, controlled MCU reset, or INA228 power-cycle
control.

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
- Clean-commit framed targeted feature suite with `0 FAIL` and `0 UNKNOWN`.
- Clean-commit framed transfer-count suite proving fixed-step polling budgets
  with `0 FAIL` and `0 UNKNOWN`.
- Framed 8-hour soak with `0 FAIL` and `0 UNKNOWN`.
- Fault-injection fixture coverage for address NACK, data/unknown-phase NACK,
  timeout, stuck-bus/bus error, OFFLINE behavior, and recovery.
- ALERT-pin capture for polarity, latch/transparent behavior, conversion-ready
  routing, and safe threshold crossings.
- Controlled MCU reset and INA228 power-cycle evidence.
- Reviewed CI logs for the exact branch, PR, or release commit.

## Serial/HIL Runner Finding

Long host-driven serial HIL runs exposed framing fragility in the example/HIL
runner path. The evidence does not prove a core library or I2C transaction
failure. A future release-grade soak should use a clean commit and either
stronger host framing or an MCU-side soak command that reports bounded summary
results.
