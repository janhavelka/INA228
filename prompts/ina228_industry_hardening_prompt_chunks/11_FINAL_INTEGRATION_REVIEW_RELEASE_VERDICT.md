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


# Chunk 11 — Final Integration Review and Release/Merge Verdict

## Goal

Do a full final review after chunks 01-10. Fix only legitimate integration issues discovered by review. Produce a final report that clearly states what was done, what improved, what still needs work, and whether the repo is merge/release ready.

## Subagents

Spawn:

1. `final-code-review-agent`: inspect all diffs against branch base.
2. `datasheet-review-agent`: re-check INA228-specific behavior against the datasheet.
3. `test-review-agent`: verify all tests/checks and summarize coverage gaps.
4. `docs-review-agent`: check README/docs/examples for overclaims.
5. `release-verdict-agent`: produce final merge/release readiness verdict.

## Required review areas

1. Code architecture:
   - core framework-neutral;
   - no Arduino/ESP-IDF/FreeRTOS leakage into `include/` or `src/`;
   - injected transport still non-owning;
   - no hidden heap-heavy framework types in core;
   - copy/move contract correct;
   - public APIs documented.

2. INA228 correctness:
   - `DIAG_ALRT`;
   - alerts;
   - triggered freshness;
   - `tick(nowMs)`;
   - energy/charge validity;
   - overflow flags;
   - calibration;
   - ADCRANGE;
   - SHUNT_CAL;
   - signed 20/40-bit values;
   - reset/RSTACC;
   - partial hardware state;
   - recovery/resync.

3. Tests/checks:

```bash
python tools/check_core_timing_guard.py
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python scripts/generate_version.py check
python -m platformio test -e native
python -m platformio run -e esp32s3dev
python -m platformio run -e esp32s2dev
python -m platformio pkg pack
```

If available:

```bash
idf.py --version
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

Do not invent results.

4. Hardware validation:
   - Do not run dangerous hardware tests unless explicit hardware setup and user approval exist.
   - If no hardware was run, mark validation pending and keep release wording conservative.

5. Fix final issues only if focused and legitimate. Do not start new feature work.

## Final report

Create `docs/INA228_INDUSTRY_HARDENING_FINAL_REPORT.md` with:

- Executive Summary
- What Changed
- Public API Changes
- Datasheet-Correctness Improvements
- Safety Improvements
- Tests Added
- CI / Build Coverage
- Commands Run
- Commands Not Run
- Hardware Validation Status
- Remaining Risks
- Merge Readiness
- Release Wording
- Follow-up Work

Merge readiness must choose one:

- Not ready
- Ready after listed fixes
- Ready to merge, not ready to release
- Ready to merge and pre-release/tag
- Ready for industry-grade release only after hardware validation

## Final commit

```bash
git status --short
git add .
git commit -m "docs: finalize INA228 industry hardening report"
git push
```

If no changes after final review, do not make an empty commit unless repo conventions require it.

## Final response to user

Return branch, final commit hash, tests/checks run, pass/fail status, merge readiness, release readiness, remaining blockers, and files changed.
