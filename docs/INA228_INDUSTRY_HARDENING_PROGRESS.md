# INA228 Industry Hardening Progress

## Chunk 01 - Branch, Baseline, AGENTS, and Implementation Plan

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting branch: `audit/ina228-industry-readiness-exploration`
Starting commit: `a98c43d337d87eb8f89e9075dc71bdddde72fc73`
Exploration report: `docs/INA228_INDUSTRY_READINESS_EXPLORATION.md`
Classification: engineering-grade with major gaps.

### Scope

Chunk 01 is setup only. No functional driver, API, test, example, or build-system fixes were started.

### Startup Baseline

Commands run before branch creation:

| Command | Result |
| --- | --- |
| `pwd` | `C:\Users\Honza\Documents\Projects\INA228` |
| `git rev-parse --show-toplevel` | `C:/Users/Honza/Documents/Projects/INA228` |
| `git branch --show-current` | `audit/ina228-industry-readiness-exploration` |
| `git status --short` | clean |
| `git remote -v` | `origin https://github.com/janhavelka/INA228.git` fetch/push |
| `git branch --list hardening/ina228-industry-readiness` | no existing local branch |
| `git rev-parse HEAD` | `a98c43d337d87eb8f89e9075dc71bdddde72fc73` |
| `git checkout -b hardening/ina228-industry-readiness` | created and switched to branch |

### Subagent Findings

Four read-only agents were used before edits:

| Agent | Finding summary |
| --- | --- |
| `planning-agent` | Ordered the work so setup stays in Chunk 01, with `DIAG_ALRT` safety before triggered freshness and overflow work. |
| `codebase-map-agent` | Mapped issue anchors to `include/INA228/INA228.h`, `include/INA228/Config.h`, `include/INA228/Status.h`, `include/INA228/CommandTable.h`, `src/INA228.cpp`, `test/test_basic.cpp`, and example transports. |
| `test-baseline-agent` | Confirmed standard guard scripts exist, PlatformIO/native environments are present, and `idf.py` is not on PATH. |
| `risk-review-agent` | Confirmed no functional implementation should start in Chunk 01 and validation claims must stay conservative. |

### Exploration Summary

Strong existing foundations:

- Framework-neutral core with injected I2C transport.
- Precise `Status` object and granular transport error enum values.
- Device identity and MEMSTAT checks.
- Managed health tracking with READY/DEGRADED/OFFLINE states.
- Native tests, Arduino CLI example, and native ESP-IDF example structure.

Major gaps to harden before industry-readiness claims:

- `DIAG_ALRT` clear-on-read/status-sensitive behavior can destroy diagnostic evidence.
- Triggered conversion freshness and `tick(nowMs)` semantics can return stale data or stall.
- ENERGY/CHARGE can be reported when datasheet operating assumptions make them invalid.
- ADCRANGE and calibration updates can leave hardware/cache scaling incoherent after partial failure.
- Reset/RSTACC, overflow, threshold scaling, copy/move, and output atomicity need stricter contracts and tests.
- Pure ESP-IDF build proof and dated hardware validation logs are not present.
- High-voltage safety documentation must be prominent and explicit.

### Ordered Chunk Plan

P0:

1. Chunk 02 - `DIAG_ALRT` and alert configuration side-effect safety.
2. Chunk 03 - Triggered conversion, freshness, and timing semantics.
3. Chunk 04 - ENERGY, CHARGE, and overflow validity.
4. Chunk 05 - Calibration, ADCRANGE atomicity, scaling vectors, and numeric safety.
5. Chunk 06 - Reset, RSTACC, recovery, and partial-state policy.
6. Chunk 08 - Native fake-bus and datasheet-vector test expansion.
7. Chunk 10 - Documentation safety and validation matrix.

P1:

1. Chunk 07 - Status/error preservation, copy/move, output atomicity, and API contract cleanup.
2. Chunk 09 - ESP-IDF/Arduino examples and CI proof, including pure ESP-IDF builds if tooling is available.
3. Remaining threshold invalidation/reapply and uncalibrated hardware behavior from Chunks 05/06.

P2:

1. Chunk 11 - Final integration review and release verdict.
2. Optional sanitizer/coverage jobs.
3. Optional fixed-point or raw-plus-scale APIs.
4. Optional production ESP-IDF adapter pattern with external bus manager and locking.
5. Expanded Doxygen pages and richer diagnostic snapshots.

### Known Blockers And Caveats

- No hardware validation was performed in this chunk.
- No production, industry-ready, ESP-IDF-ready, or hardware-validated claim is made by this report.
- Pure ESP-IDF builds require `idf.py`; local availability is checked below.
- PlatformIO ESP32-S2/S3 baseline builds may fail due to local framework/toolchain behavior; failures are recorded and not fixed in Chunk 01 unless caused only by docs.

### Changes

- Updated `AGENTS.md` with explicit rules for hidden platform waits, destructive/status-sensitive `DIAG_ALRT`, multi-step dirty/rollback/resync policy, energy/charge validity, and validation-claim evidence.
- Created this progress report.

### API Changes

None.

### Tests Added Or Updated

None.

### Baseline Commands

| Command | Result |
| --- | --- |
| `git status --short` | `M AGENTS.md`; `?? docs/INA228_INDUSTRY_HARDENING_PROGRESS.md` |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | PASS: 48 tests passed in native environment. PlatformIO warned that obsolete Core 6.1.18 is used and a previous 6.1.19 also exists. |
| `python -m platformio run -e esp32s3dev` | PASS: ESP32-S3 Arduino build succeeded. |
| `python -m platformio run -e esp32s2dev` | PASS: ESP32-S2 Arduino build succeeded. |
| `python -m platformio pkg pack` | PASS: wrote `INA228-1.3.0.tar.gz`; generated tarball was removed after recording the result. |
| `idf.py --version` | NOT AVAILABLE: PowerShell reported `idf.py : The term 'idf.py' is not recognized as the name of a cmdlet, function, script file, or operable program.` |

### Commands Not Run

| Command | Reason |
| --- | --- |
| `idf.py -C examples/esp_idf/basic set-target esp32s3 build` | Not run because `idf.py --version` failed; ESP-IDF is not available on PATH in this environment. |
| `idf.py -C examples/esp_idf/basic set-target esp32s2 build` | Not run because `idf.py --version` failed; ESP-IDF is not available on PATH in this environment. |

### Remaining Risks

- Chunk 01 did not fix any functional INA228 issues.
- `DIAG_ALRT`, triggered freshness, ENERGY/CHARGE validity, ADCRANGE/calibration atomicity, reset/RSTACC semantics, transport error preservation, and exact datasheet vectors remain open for later chunks.
- PlatformIO Arduino S2/S3 builds passed locally in this chunk, but pure ESP-IDF build proof is still missing.
- No hardware validation was performed or claimed.
- Commit hash is pending until this docs/setup commit is created.
