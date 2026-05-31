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


# Chunk 01 — Branch, Baseline, AGENTS, and Implementation Plan

## Goal

Start the actual hardening work cleanly. Do not implement functional fixes yet except minimal `AGENTS.md` / report setup. Establish branch, baseline status, progress report, and a precise implementation plan from the exploration report.

## Required startup

```bash
pwd
git rev-parse --show-toplevel
git branch --show-current
git status --short
git remote -v
```

If the worktree is dirty, stop and report the dirty files. Do not switch branches or edit files until the user approves.

If clean, create the hardening branch:

```bash
git checkout -b hardening/ina228-industry-readiness
```

If the branch already exists, do not delete it. Switch to it only if it is clearly the intended implementation branch.

## Subagents

Spawn:

1. `planning-agent`: turn the exploration report into an ordered P0/P1/P2 implementation map.
2. `codebase-map-agent`: map relevant files/functions to the issues.
3. `test-baseline-agent`: identify all available tests/checks/build commands.
4. `risk-review-agent`: confirm no implementation starts prematurely.

## Update AGENTS.md

Add or refine INA228-specific rules:

- core remains framework-neutral;
- injected/non-owning I2C transport only;
- no hidden platform delays in core;
- DIAG_ALRT is destructive/status-sensitive and must be treated carefully;
- SHUNT_CAL/current-LSB/ADCRANGE must be one coherent contract;
- ENERGY/CHARGE validity depends on mode and accumulation semantics;
- multi-register writes need dirty/rollback/resync policy;
- public APIs are not ISR-safe unless proven;
- instances are not thread-safe unless protected externally;
- high-voltage examples require prominent safety disclaimers;
- no validation claim without logs/results.

## Progress report

Create `docs/INA228_INDUSTRY_HARDENING_PROGRESS.md` including branch, starting commit, exploration summary, ordered chunk plan, known blockers, and baseline commands run.

## Baseline checks

Run available checks. Do not try to fix failures in this chunk unless they are caused only by the new docs.

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

## Commit

Commit only docs/setup changes:

```bash
git status --short
git add AGENTS.md docs/INA228_INDUSTRY_HARDENING_PROGRESS.md
git commit -m "docs: start INA228 industry readiness hardening"
git push -u origin hardening/ina228-industry-readiness
```

If push is unavailable, report that clearly.
