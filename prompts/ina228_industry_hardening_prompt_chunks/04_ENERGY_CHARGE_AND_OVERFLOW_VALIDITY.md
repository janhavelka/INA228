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


# Chunk 04 — ENERGY, CHARGE, and Overflow Validity

## Goal

Prevent invalid ENERGY/CHARGE data from being reported as valid, and define a safe overflow/diagnostic policy.

This chunk covers H4 and M4 from the exploration report.

## Problem summary

The exploration report found:

- `readMeasurement()` reads energy.
- `readEnergy()` and `readCharge()` are available.
- Datasheet says ENERGY and CHARGE are invalid in triggered mode because the device does not track elapsed time.
- Accumulator overflow flags can be cleared when corresponding registers are read.
- `MATHOF`, `ENERGYOF`, and `CHARGEOF` are not connected to measurement validity.

## Subagents

Spawn:

1. `accumulation-datasheet-agent`: verify ENERGY/CHARGE validity, triggered-mode limitation, overflow flag clearing, and reset behavior.
2. `measurement-api-agent`: inspect `Measurement`, `readMeasurement()`, `readEnergy()`, `readCharge()`, overflow handling, and status design.
3. `compatibility-agent`: propose minimal API changes that avoid invalid data while preserving users where possible.
4. `test-agent`: create exact tests for invalid modes and overflow flags.

## Implementation requirements

1. Guard accumulation validity.
   - Strict option: `readEnergy()`, `readCharge()`, and `readMeasurement()` return a specific error/status when not in a continuous accumulation-valid mode.
   - Structured option: `Measurement` includes validity flags. Do not silently return invalid accumulation as valid.

2. Reject or mark invalid energy/charge in shutdown mode, triggered modes, after reset before valid continuous accumulation, and after accumulator reset until valid.

3. Define overflow policy for `ENERGYOF`, `CHARGEOF`, and `MATHOF`.
   - Provide diagnostic snapshot before destructive accumulator reads when needed.
   - Return dedicated statuses or validity flags when overflow is observed.
   - Document reads that clear overflow evidence.

4. Coordinate accumulator reset validity with Chunk 06.

5. Update README/Doxygen for energy/charge validity.

## Required tests

Native fake-bus tests:

1. Continuous mode allows energy/charge when calibrated.
2. Triggered modes reject or mark invalid energy/charge.
3. Shutdown mode rejects or marks invalid energy/charge.
4. `readMeasurement()` cannot silently return invalid energy/charge.
5. Overflow flags are surfaced according to policy.
6. Reading energy/charge does not hide overflow without preserving it.
7. Accumulator reset makes validity state deterministic.

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
git commit -m "fix: guard INA228 accumulation validity"
git push
```
