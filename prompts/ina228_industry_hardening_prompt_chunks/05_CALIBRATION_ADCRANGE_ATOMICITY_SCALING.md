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


# Chunk 05 — Calibration, ADCRANGE Atomicity, Scaling Vectors, and Numeric Safety

## Goal

Make INA228 calibration and scaling production-safe. `SHUNT_CAL`, `currentLsb`, `ADCRANGE`, shunt resistance, and max expected current must be one coherent contract.

This chunk covers H5 plus the calibration/scaling deep dive.

## Problem summary

The exploration report found:

- `setAdcRange()` can write CONFIG with the new range, then fail while applying calibration, leaving hardware in the new range but cache rolled back.
- Basic formulas exist, but exact nonzero vectors, negative vectors, ADCRANGE=1 multiplier, and overflow boundaries are missing.
- Clamp behavior weakens the `maxExpectedCurrentA` contract.
- Uncalibrated behavior leaves hardware `SHUNT_CAL` nonzero/stale.

## Subagents

Spawn:

1. `calibration-datasheet-agent`: verify `SHUNT_CAL`, `CURRENT_LSB`, ADCRANGE multiplier, and scaling constants.
2. `numeric-agent`: inspect all conversion formulas for overflow, sign extension, precision loss, and invalid values.
3. `atomicity-agent`: design safe update order/dirty-state or rollback model.
4. `test-vector-agent`: implement datasheet-style positive/negative/extreme vectors.

## Implementation requirements

1. Define calibration state in diagnostics/snapshot:
   - calibrated vs uncalibrated;
   - requested shunt resistance;
   - requested max expected current;
   - actual current LSB;
   - actual SHUNT_CAL;
   - clamped/rejected state;
   - hardware/config dirty state.

2. Make `setAdcRange()` safe.
   - Compute and validate new calibration before hardware range changes.
   - If a multi-step write partially fails, either roll back hardware or mark hardware dirty.
   - No silent cache/hardware divergence.
   - `recover()` or `resync()` clears dirty only after full success.

3. Clarify clamp policy.
   - Reject impossible values, or clamp but expose actual behavior and status/detail warning.

4. Uncalibrated behavior.
   - Prevent converted current/power/energy/charge reads.
   - Consider writing `SHUNT_CAL=0` when uncalibrated.
   - Document raw-register behavior.

5. Make sign extension portable if current implementation relies on implementation-defined casts.

6. Add exact test vectors:
   - `R=0.0162 ohm`, `max=10 A`, ADCRANGE=0 -> `CURRENT_LSB=0.000019073486328125`, `SHUNT_CAL=0x0FD2`.
   - Same inputs ADCRANGE=1 -> `SHUNT_CAL=0x3F48`.
   - Positive raw vectors: `VSHUNT=0x4BF000`, `VBUS=0x3C0000`, `DIETEMP=0x0C80`, `CURRENT=0x4CCCC0`, `POWER=0x48000C`, `ENERGY=0x003F480000`, `CHARGE=0x0043800000`.
   - Negative vectors: `VSHUNT=0xB41000`, `CURRENT=0x800000`, `CURRENT=0xFFFFF0`, `CHARGE=0xFFFFFFFFFF`, `CHARGE=0x8000000000`.

7. Define threshold behavior after ADCRANGE/calibration changes: reapply from engineering units, mark dirty/invalid, or document required user reconfiguration.

## Required tests

Add native tests for calibration formula vectors, ADCRANGE multiplier, negative raw values, overflow/non-finite output policy, invalid configuration, `setAdcRange()` scripted failure, dirty/recover behavior, and threshold invalidation/reapply policy.

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
git commit -m "fix: harden INA228 calibration and ADCRANGE state"
git push
```
