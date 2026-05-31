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


# Chunk 02 — DIAG_ALRT and Alert Configuration Side-Effect Safety

## Goal

Fix the highest-risk INA228 correctness issue: `DIAG_ALRT` is both configuration and live/destructive status. Internal conversion polling and alert configuration must not silently destroy field diagnostic evidence.

This chunk covers exploration findings H1 and H2.

## Problem summary

The exploration report found:

- `tick()` calls `isConversionReady()`.
- `isConversionReady()` reads `DIAG_ALRT` and keeps only `CNVRF`.
- `_ensureMeasurementReadyForRead()` also uses this path.
- Datasheet says `CNVRF` clears when `DIAG_ALRT` is read.
- Alert setters read-modify-write `DIAG_ALRT`, which also contains live status flags.

Production risks: polling conversion readiness can clear latched alert evidence, configuring alert behavior can consume/rewrite live flags, and a triggered conversion can get stranded if public diagnostic reads consume `CNVRF`.

## Subagents

Spawn:

1. `diag-alrt-datasheet-agent`: re-read datasheet sections for `DIAG_ALRT`, `CNVRF`, overflow flags, alert-config bits, and clear-on-read behavior.
2. `diag-alrt-code-agent`: map every current read/write of `REG_DIAG_ALRT`.
3. `diag-alrt-test-agent`: design fake-bus clear-on-read tests.
4. `api-review-agent`: decide public naming/contract for destructive and non-destructive diagnostics.

## Implementation requirements

1. Separate alert configuration from live status.
   - Keep private cached config bits for alert latch, conversion-ready alert enable, slow-alert setting, alert polarity, and any other config-only bits in `DIAG_ALRT`.
   - Do not build config writes by reading the live `DIAG_ALRT` register unless the read side effects are intentional and documented.
   - Preserve reserved bits correctly.

2. Make destructive reads explicit.
   - Public diagnostic APIs that read `DIAG_ALRT` must document clear-on-read side effects.
   - Prefer clear names such as `readDiagAlertRaw()` / `consumeDiagAlert()` only if they fit the existing API style.

3. Preserve internal alert evidence.
   - When internal code reads `DIAG_ALRT` for `CNVRF`, preserve the full raw status somewhere observable.
   - Do not lose alert evidence merely because the driver polled readiness.

4. Do not fully solve triggered freshness in this chunk unless required; keep the design compatible with the next chunk.

5. Update README/Doxygen minimally for `DIAG_ALRT` read side effects and safe alert setter behavior.

## Required tests

Add native fake-bus tests:

1. Fake bus sets `DIAG_ALRT = CNVRF | ALERT_FLAG`.
2. Call `tick()` or the readiness path.
3. Assert conversion-ready is recognized and `ALERT_FLAG` remains observable through the driver diagnostic API.
4. Seed alert flags, call each alert configuration setter, assert setter does not consume/rewrite live flags contrary to policy.
5. Public raw/structured diagnostic reads are tested as destructive if that is the chosen contract.

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
git commit -m "fix: preserve INA228 diagnostic alert side effects"
git push
```
