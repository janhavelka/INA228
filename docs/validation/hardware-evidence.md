# Hardware Evidence Index

This index summarizes checked-in hardware-in-loop reports without upgrading
their claim level. These runs are useful partial evidence, but none is
release-grade hardware validation because each was produced from a dirty
worktree and the fixture did not include fault injection, alert-pin capture,
controlled MCU reset, or INA228 power-cycle control.

| Report | Commit | Dirty | Framework/board | Port | PASS | FAIL | UNKNOWN | NOT RUN | Release-grade? | Notes |
| --- | --- | --- | --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| `docs/reports/hil-validation-COM21-20260622.md` | `5840497166200a3b1eb9e8f68f447b6d7182367a` | Yes | Arduino `esp32s3dev` / ESP32-S3 | COM21 | 256931 | 0 | 48 | 4 | No | 8-hour low-voltage soak; UNKNOWN serial framing rows remain. |
| `docs/reports/hil-targeted-COM21-20260623-attempt1.md` | `5840497166200a3b1eb9e8f68f447b6d7182367a` | Yes | Arduino `esp32s3dev` / ESP32-S3 | COM21 | 182 | 10 | 1 | 5 | No | Failed targeted attempt retained for debugging history. |
| `docs/reports/hil-targeted-COM21-20260623.md` | `5840497166200a3b1eb9e8f68f447b6d7182367a` | Yes | Arduino `esp32s3dev` / ESP32-S3 | COM21 | 195 | 0 | 0 | 5 | No | Targeted feature sweep passed, but no transfer-count evidence or fixture fault coverage. |
| `docs/reports/hil-targeted-stress-COM21-20260623.md` | `5840497166200a3b1eb9e8f68f447b6d7182367a` | Yes | Arduino `esp32s3dev` / ESP32-S3 | COM21 | 6787 | 0 | 1 | 4 | No | Short targeted stress stopped with one UNKNOWN serial framing row. |
| `docs/reports/hil-transfer-COM21-20260623.md` | `29228f1122dfd0cb3bc8d82c5b92535a2f042ca8` | Yes | Arduino `esp32s3dev` / ESP32-S3 | COM21 | 53 | 0 | 0 | 5 | No | Framed fixed-step transfer suite passed on hardware; exact assertions cover deterministic paths, and snapshots record paths where example `tick()` can add background reads. |
| `docs/reports/hil-targeted-framed-COM21-20260623.md` | `29228f1122dfd0cb3bc8d82c5b92535a2f042ca8` | Yes | Arduino `esp32s3dev` / ESP32-S3 | COM21 | 196 | 0 | 0 | 5 | No | Framed targeted feature sweep passed on hardware with no FAIL/UNKNOWN rows, including unknown-command framed-status coverage. |

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
