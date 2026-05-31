# Shared Rules for INA228 Industry-Readiness Hardening

You are working in the INA228 I2C library repository.

The exploration report classified the repo as **engineering-grade with major gaps**. The core architecture is good, but several INA228-specific semantics must be fixed before the library can be honestly called industry-standard.

Use the ADS1115 workflow only as a quality/style reference. Do not blindly copy ADS1115 names, APIs, health model, or implementation details.

## Always follow these rules

- Work only in the INA228 repository.
- Keep the core framework-neutral: no Arduino, Wire, ESP-IDF, FreeRTOS, logging framework, heap-heavy framework types, or platform delays in `include/` or `src/`.
- The core must not own the I2C bus. Bus creation, locking, recovery, and platform handles belong to injected transports or examples.
- Preserve precise transport errors where possible: address NACK, data NACK, timeout, bus error, generic I2C error.
- No hardware-validation claims unless hardware was actually run and logs are captured.
- No pure ESP-IDF readiness claim unless `idf.py` builds or CI prove it.
- Do not hide uncertainty. If something is not tested, say so.
- Use subagents where available and have them report factual findings before implementation.
- Keep each chunk focused. Do not start later chunks early unless a small dependency is unavoidable.
- At the end of each chunk, run relevant checks, update the progress report, commit, and push/sync if available.

## Standard end-of-chunk checks

Run what is available and relevant. Do not invent results.

```bash
git status --short
python tools/check_core_timing_guard.py
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python scripts/generate_version.py check
python -m platformio test -e native
python -m platformio run -e esp32s3dev
python -m platformio run -e esp32s2dev
python -m platformio pkg pack
```

If ESP-IDF is available, also run:

```bash
idf.py --version
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

If a command does not exist or fails due to environment/toolchain, report the exact failure and continue only when safe.

## Standard progress report

Maintain or create:

```text
docs/INA228_INDUSTRY_HARDENING_PROGRESS.md
```

For every chunk append: chunk number/title, commit hash if committed, what changed, API changes, tests added/updated, commands run/results, commands not run and why, remaining risks.


# Chunk 06 — Reset, RSTACC, Recovery, and Partial-State Handling

## Goal

Make reset/recovery behavior and multi-register partial-state handling explicit, bounded, and testable.

This chunk covers H6 and the partial-state/cache-consistency assessment.

## Problem summary

The exploration report found:

- `softReset()` writes reset and immediately reapplies config/calibration.
- Reset equivalent to POR likely needs bounded settle/readback behavior.
- `resetAccumulators()` writes `RSTACC`, but clear/readback semantics are not confirmed.
- `_applyConfig()` and `_applyCalibration()` are multi-step, and partial failure can leave hardware/cache inconsistent.
- Multi-register outputs can be partially updated on read failure.

## Subagents

Spawn:

1. `reset-datasheet-agent`: verify reset, POR/startup timing, RSTACC behavior, MEMSTAT, and reset-related flags.
2. `partial-state-agent`: trace every multi-register write path and classify partial failure risk.
3. `recover-agent`: design recover/resync behavior for dirty hardware state.
4. `test-agent`: implement scripted transaction failure tests.

## Implementation requirements

1. Define bounded reset policy:
   - Is a monotonic clock required?
   - Is reset completion verified by reading DEVICE_ID / MANUFACTURER_ID / CONFIG / MEMSTAT?
   - What timeout applies?
   - Avoid unbounded waits and hidden platform delays.

2. Define and test RSTACC semantics:
   - self-clear or manual clear;
   - accumulator validity reset;
   - energy/charge validity after reset.

3. Add dirty hardware state for multi-step writes if not already implemented.
   - Include whether hardware may differ from cache and original failure status.
   - Clear only after full recovery/resync.

4. Ensure `recover()` revalidates identity/MEMSTAT and reapplies config/calibration/alert config safely.

5. Make `readMeasurement()` and `readRawSample()` all-or-nothing on output assignment, or document partial-output semantics and add validity fields. Prefer all-or-nothing.

## Required tests

Native tests:

1. `softReset()` success path with bounded verification.
2. `softReset()` timeout/failure path.
3. `resetAccumulators()` clears validity according to policy.
4. Partial failure at every write position in `_applyConfig()`.
5. Partial failure during `recover()`.
6. Dirty flag set/cleared correctly.
7. `readMeasurement()` does not partially modify output after failure.
8. Same for `readRawSample()`.

## Checks and commit

Run:

```bash
python tools/check_core_timing_guard.py
python -m platformio test -e native
python scripts/generate_version.py check
```

Append progress report, then commit:

```bash
git status --short
git add .
git commit -m "fix: define INA228 reset and dirty-state recovery"
git push
```
