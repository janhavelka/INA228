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


# Chunk 08 — Native Tests, Fake Bus, and Datasheet Vector Coverage

## Goal

Consolidate and expand native test coverage after functional chunks. This chunk is test-heavy and should reduce regression risk before examples/CI/docs work.

## Subagents

Spawn:

1. `coverage-agent`: map exploration findings to test coverage.
2. `fake-bus-agent`: inspect and improve fake I2C transport behavior.
3. `datasheet-vector-agent`: verify exact formula vectors.
4. `fault-injection-agent`: add systematic failure-at-transaction-position tests.
5. `regression-agent`: run native tests repeatedly and identify flaky behavior.

## Required test coverage

Ensure native tests cover:

### Register and identity

- DEVICE_ID success/mismatch.
- MANUFACTURER_ID success/mismatch.
- MEMSTAT success/failure.
- byte order for 16/24/40-bit helpers.
- signed 20-bit and signed 40-bit sign extension.
- reserved-bit preservation where applicable.

### Calibration/scaling

- normal SHUNT_CAL vectors.
- ADCRANGE=0 and ADCRANGE=1.
- current LSB clamping or rejection policy.
- invalid shunt resistance.
- invalid max expected current.
- uncalibrated path.
- positive and negative shunt/current/charge.
- temperature negative and positive.
- power/energy/charge exact vectors.
- overflow/non-finite handling.

### DIAG/alerts

- clear-on-read fake behavior.
- CNVRF preservation.
- alert setters not consuming live flags.
- overflow flags.
- threshold flags.
- destructive public diagnostic read semantics.
- alert config cache after recover.

### Triggered/freshness/timing

- begin-time triggered mode.
- explicit trigger and not-ready before deadline.
- ready after fake CNVRF.
- tick with/without configured time hook according to chosen contract.
- wraparound timing.
- continuous latest semantics.
- conversion-delay and averaging estimate vectors.

### Partial failures and recovery

- failure at each multi-register write position.
- dirty state set/cleared.
- recover success/failure.
- reset success/failure.
- reset accumulator validity state.
- all-or-nothing output assignment.

### API contracts

- copy/move deleted.
- no core framework leakage.
- error mapping precision.
- not-initialized/offline/degraded behavior.
- raw register APIs behave as documented.

## Test quality rules

- Tests must be deterministic.
- Do not mock away dangerous behavior; fake bus should model clear-on-read and scripted failures accurately.
- Avoid sleeping in native tests.
- Prefer exact integer/raw-register vectors over fuzzy float-only tests.
- Use tolerances only where floating conversion requires them.

## Commands and commit

Run:

```bash
python tools/check_core_timing_guard.py
python -m platformio test -e native
python -m platformio test -e native
python scripts/generate_version.py check
```

Update progress report with old/new test count and coverage map, then commit:

```bash
git status --short
git add .
git commit -m "test: expand INA228 datasheet and fault coverage"
git push
```
