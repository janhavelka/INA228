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


# Chunk 07 — Status Precision, Copy/Move Semantics, and Public API Contracts

## Goal

Tighten general production contracts after the core device-specific fixes: precise errors, copy/move prevention, public Doxygen, and API output contracts.

This chunk covers M1, M2, M3, M6, L2, and related public API contract gaps.

## Problem summary

The exploration report found:

- `begin()` and `probe()` sometimes collapse transport failures to `DEVICE_NOT_FOUND`.
- `readMeasurement()` / `readRawSample()` partially update caller outputs on failure.
- `INA228` copy/move special members are implicit despite mutable hardware/cache/health state.
- raw register write warnings are too weak.
- public header lacks explicit not-thread-safe / not-ISR-safe contract.

Some output atomicity may already have been handled in Chunk 06. Verify before editing.

## Subagents

Spawn:

1. `status-agent`: audit all places where transport errors are converted.
2. `api-agent`: inspect public API comments and contracts.
3. `copy-agent`: decide delete/define copy/move behavior.
4. `compatibility-agent`: note breaking changes and migration impact.

## Implementation requirements

1. Preserve transport error precision.
   - Definite address NACK can map to `DEVICE_NOT_FOUND`.
   - Timeouts remain timeouts.
   - Bus errors remain bus errors.
   - Data NACK remains data NACK.
   - Detail fields preserve register/address context where useful.

2. Delete copy and move unless strongly justified:

```cpp
INA228(const INA228&) = delete;
INA228& operator=(const INA228&) = delete;
INA228(INA228&&) = delete;
INA228& operator=(INA228&&) = delete;
```

3. Update public API comments:
   - not thread-safe;
   - not ISR-safe;
   - external locking required for shared buses;
   - callbacks must not re-enter the same instance;
   - raw register APIs are diagnostic and can desynchronize cache or clear flags;
   - which APIs are destructive/status-clearing;
   - calibration required for converted current/power/energy/charge.

4. Ensure new statuses are documented and consistently used.

## Required tests

Native tests:

1. Probe/begin preserve address NACK vs timeout vs bus error.
2. Copy/move prevention compile-time assertions.
3. Status for calibration invalid / accumulation invalid / dirty state is consistent.
4. Public destructive diagnostic read semantics are tested/documented.

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
git commit -m "fix: tighten INA228 public API contracts"
git push
```
