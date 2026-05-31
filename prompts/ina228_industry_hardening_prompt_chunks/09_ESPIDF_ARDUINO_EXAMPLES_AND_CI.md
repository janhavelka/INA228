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


# Chunk 09 — ESP-IDF, Arduino Examples, and CI Build Matrix

## Goal

Make build and example claims honest and reproducible. Add pure ESP-IDF build proof in CI where feasible, label examples correctly, and fix local/CI build gaps without hiding failures.

This chunk covers H8, M8, L1, and build-readiness gaps.

## Problem summary

The exploration report found:

- `library.json` declares ESP-IDF support.
- Native IDF example exists and passes static guard.
- CI lacks `idf.py` builds.
- Local `idf.py` was not installed.
- PlatformIO Arduino S2/S3 builds failed locally in framework object compilation, while CI reportedly has jobs.
- IDF adapter uses single-owner/global example glue and is not a production shared-bus manager.
- ESP-IDF help text for raw register write is weaker than Arduino help.

## Subagents

Spawn:

1. `ci-agent`: inspect existing CI and propose minimal ESP-IDF matrix.
2. `idf-agent`: inspect ESP-IDF component/example for native IDF purity, bus ownership, timeouts, error mapping, and build commands.
3. `arduino-agent`: inspect Arduino CLI/builds and local failure mode.
4. `docs-agent`: align example labeling and help text.
5. `review-agent`: make sure examples do not leak framework code into core.

## Implementation requirements

1. Add pure ESP-IDF CI build coverage if practical:

```bash
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

Use an official/reliable ESP-IDF CI container/action consistent with repo style. If not feasible, document pending commands and why.

2. Label ESP-IDF example honestly:
   - diagnostic/single-owner unless it truly provides production bus management;
   - global `gTransport` is example glue;
   - multitask/shared-bus systems need external bus manager and lock.

3. Audit/fix IDF error mapping:
   - `ESP_ERR_TIMEOUT` -> timeout;
   - NACK/address failure if distinguishable;
   - bus errors;
   - invalid args;
   - preserve detail codes.

4. Investigate PlatformIO S2/S3 build failures:
   - capture full verbose output;
   - determine local environment vs source-level failure;
   - fix source-level failures;
   - document local environment failures honestly.

5. Align Arduino and ESP-IDF help text:
   - raw writes are diagnostic;
   - high-voltage safety;
   - accumulation validity;
   - destructive DIAG reads.

6. Update guard scripts if needed.

## Required checks

Run all available relevant commands, including PlatformIO and `idf.py` if present. Do not invent pass results.

## Commit

Update progress report, then commit:

```bash
git status --short
git add .
git commit -m "ci: add INA228 ESP-IDF build coverage"
git push
```

If no CI files changed but examples/docs changed, adjust the commit message accordingly.
