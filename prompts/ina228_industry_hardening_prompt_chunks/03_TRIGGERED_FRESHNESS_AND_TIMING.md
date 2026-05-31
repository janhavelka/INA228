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


# Chunk 03 — Triggered Conversion, Freshness, and Timing Semantics

## Goal

Make triggered conversion behavior deterministic, bounded, and honest. Reads must not silently return stale registers when the caller expects a fresh triggered measurement.

This chunk covers exploration finding H3 and timing/freshness issues.

## Problem summary

The exploration report found:

- `begin()` accepts `Config::mode`, but triggered state is not tracked if the initial config mode is triggered.
- `_trigPending` is initialized false and is mainly set by `setMode()` / `triggerConversion()`.
- `tick(nowMs)` accepts a timestamp but readiness helpers also call `_nowMs()`, so missing `Config::nowMs` can stall completion.
- Output registers remain until replaced, so "latest" and "fresh" are different concepts.

## Subagents

Spawn:

1. `triggered-datasheet-agent`: verify triggered modes, CNVRF, conversion-ready, conversion-delay, and output-register replacement semantics.
2. `timing-code-agent`: trace `_trigPending`, `_nextReadyAtMs`, `tick()`, `isConversionReady()`, `_ensureMeasurementReadyForRead()`, and `estimateConversionTimeUs()`.
3. `api-contract-agent`: propose clear public semantics for latest vs fresh vs blocking/polling.
4. `tests-agent`: design wraparound and missing-clock tests.

## Implementation requirements

1. Define and document mode/freshness contract.
   - Continuous reads return latest available register contents; not guaranteed fresh since last call unless a freshness API says so.
   - Triggered APIs must return not-ready until completion, or stale/latest behavior must be explicitly named.

2. Fix begin-time triggered modes.
   - If `Config::mode` is a triggered mode during `begin()`, either reject without a clock, mark the pending conversion correctly, or force a safe baseline and require explicit `triggerConversion()`.
   - Pick the cleanest existing-API-compatible behavior and document it.

3. Fix `tick(nowMs)` vs `Config::nowMs`.
   - A caller-provided `tick(nowMs)` must not depend on `Config::nowMs` for the same readiness decision.

4. Consider adding a status-returning poll method if current `tick()` hides failure.

5. Review/correct conversion timing estimates, including first conversion vs steady state, `CONVDLY`, averaging, enabled conversions, and worst-case 1024 averages.

## Required tests

Add native tests for:

1. `begin()` with each `TRIG_*` mode.
2. Reads before triggered completion return the selected not-ready/stale status, not silent stale data.
3. `tick(nowMs)` works without `Config::nowMs` if that is the chosen contract, or rejects configuration explicitly if not.
4. Wraparound-safe deadline comparisons.
5. Public diagnostic read of `DIAG_ALRT` consuming CNVRF does not strand a pending conversion.
6. `estimateConversionTimeUs()` defaults and maximums.

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
git commit -m "fix: define INA228 triggered conversion freshness"
git push
```
