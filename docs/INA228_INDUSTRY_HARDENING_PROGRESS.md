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
- Chunk 01 commit: `836c32bf7fdb7c05dc330132ea469d6d4265e449`.

## Chunk 02 - DIAG_ALRT and Alert Configuration Side-Effect Safety

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting commit: `836c32bf7fdb7c05dc330132ea469d6d4265e449`
Commit: pending at report-update time; final response records the committed hash.

### Scope

Chunk 02 fixed `DIAG_ALRT` side-effect safety for internal conversion-ready polling and alert configuration setters. It did not attempt the broader triggered freshness redesign planned for Chunk 03.

### Subagent Findings

Four read-only agents were used before implementation:

| Agent | Finding summary |
| --- | --- |
| `diag-alrt-datasheet-agent` | Confirmed `DIAG_ALRT` mixes R/W config bits with live status bits, and conservative handling must treat reads as potentially clearing `CNVRF` and latched alert flags. |
| `diag-alrt-code-agent` | Mapped all `REG_DIAG_ALRT` reads/writes in core, tests, and examples; identified `isConversionReady()` and alert setters as the key unsafe paths. |
| `diag-alrt-test-agent` | Proposed fake-bus clear-on-read modeling and tests for preserved evidence, non-reading setters, and destructive public reads. |
| `api-review-agent` | Recommended keeping existing public read names, documenting destructive side effects, and adding one cache-only preserved-evidence accessor. |

### Changes

- Added `DIAG_CONFIG_MASK` and `DIAG_CLEAR_ON_READ_MASK` constants.
- Added `DiagAlertSnapshot` and `getDiagAlertSnapshot()` as a cache-only API for preserved `DIAG_ALRT` evidence.
- Centralized parsing/capture of destructive `DIAG_ALRT` reads.
- Updated `begin()`, `recover()`, `isConversionReady()`, `readDiagAlert()`, `readDiagAlertRaw()`, and `readRegister16(REG_DIAG_ALRT)` to preserve the full raw `DIAG_ALRT` value when they read it.
- Reworked `setAlertLatch()`, `setConversionReadyAlert()`, `setSlowAlert()`, and `setAlertPolarity()` to write cached alert configuration bits without first reading live `DIAG_ALRT` status.
- Reapplied cached alert configuration through `_applyConfig()`.
- Updated README and Doxygen comments to state that public `DIAG_ALRT` reads are destructive and snapshots are non-I2C preserved evidence.

### API Changes

- Added public struct `DiagAlertSnapshot`.
- Added public method `Status getDiagAlertSnapshot(DiagAlertSnapshot& out) const`.
- Added public constants `cmd::DIAG_CONFIG_MASK` and `cmd::DIAG_CLEAR_ON_READ_MASK`.
- Existing `readDiagAlert()` and `readDiagAlertRaw()` names remain, but their destructive-read contract is now explicit.

### Tests Added Or Updated

- Updated the native fake bus to model `DIAG_ALRT` clear-on-read behavior.
- Added native tests for:
  - `tick()` preserving `DIAG_ALRT` evidence while polling `CNVRF`.
  - measurement readiness preserving alert evidence while gating triggered reads.
  - alert configuration setters not reading live `DIAG_ALRT`.
  - structured public diagnostic reads being destructive and preserved.
  - raw public diagnostic reads being destructive.

### Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed modified implementation/docs/test files before commit |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | PASS: 53 tests passed in native environment. PlatformIO warned that obsolete Core 6.1.18 is used and a previous 6.1.19 also exists. |
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

- No hardware validation was performed or claimed.
- Pure ESP-IDF build proof is still missing because `idf.py` is unavailable locally.
- Chunk 03 still needs to define broader triggered-mode freshness and `tick(nowMs)` semantics.
- Chunk 04 still needs ENERGY/CHARGE validity and overflow policy.
- Public generic `writeRegister16(REG_DIAG_ALRT, value)` remains diagnostic raw access and can desynchronize user expectations; this is documented as raw-register risk and remains part of the broader API cleanup.

## Chunk 03 - Triggered Conversion, Freshness, and Timing Semantics

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting commit: `7ca345abc72eda124842db89805a78a5ce35f78d`
Commit: pending at report-update time; final response records the committed hash.

### Scope

Chunk 03 defined and implemented deterministic triggered-conversion freshness semantics. It did not attempt ENERGY/CHARGE validity, calibration/ADCRANGE atomicity, reset accumulator policy, or broader API cleanup planned for later chunks.

### Subagent Findings

Four read-only agents were used before implementation:

| Agent | Finding summary |
| --- | --- |
| `triggered-datasheet-agent` | Confirmed triggered modes perform one conversion sequence then shutdown, output registers retain prior contents until replaced, CNVRF is set after conversions/averaging complete, CNVRF can be cleared by DIAG_ALRT reads, and CONVDLY participates in first-conversion timing. |
| `timing-code-agent` | Found begin-time triggered modes were not marked pending, `tick(nowMs)` delegated to readiness code using `Config::nowMs`, public DIAG_ALRT reads could consume CNVRF while a trigger remained pending, and ADC_CONFIG timing writes could restart triggered conversion timing without cache reset. |
| `api-contract-agent` | Recommended a backward-compatible contract: continuous reads return latest registers, pending triggered reads return `MEASUREMENT_NOT_READY`, `begin(TRIG_*)` marks pending, and a status-returning `pollConversionReady(nowMs, ready)` should back `tick(nowMs)`. |
| `tests-agent` | Proposed native fake-bus tests for begin-time triggered modes, stale-register prevention, caller-timestamp polling without `Config::nowMs`, wraparound deadlines, DIAG_ALRT CNVRF consumption, and conversion-time limits. |

### Changes

- Added `pollConversionReady(uint32_t nowMs, bool& ready)` and changed `tick(nowMs)` to use caller-supplied time directly.
- Marked `begin()` with a triggered initial mode as pending instead of silently treating stale output registers as readable.
- Centralized triggered start/completion handling so `setMode()`, `triggerConversion()`, recover reapply, soft reset reapply, and ADC_CONFIG timing setters keep pending state and start timestamps coherent.
- Made completion respond to any preserved DIAG_ALRT read that observes CNVRF, so a public diagnostic read cannot strand a pending conversion after consuming CNVRF.
- Kept continuous measurement reads as latest-register reads and made pending triggered reads return `MEASUREMENT_NOT_READY` before output mutation.
- Updated Doxygen, `Config::nowMs` notes, and README behavioral contracts for latest-vs-fresh and caller-supplied polling semantics.
- Updated the native fake bus so ADC_CONFIG writes to active conversion modes clear CNVRF, matching the datasheet side effect.

### API Changes

- Added public method `Status pollConversionReady(uint32_t nowMs, bool& ready)`.
- Existing `tick(uint32_t nowMs)` now advances triggered polling using the supplied timestamp and no longer depends on `Config::nowMs`.
- Existing measurement APIs now explicitly document that continuous reads are latest-register reads and pending triggered reads return `MEASUREMENT_NOT_READY`.

### Tests Added Or Updated

- Added native tests for:
  - every begin-time triggered mode marking a pending conversion.
  - triggered reads refusing stale register contents before CNVRF.
  - `tick(nowMs)` completing a trigger without a `Config::nowMs` hook.
  - wraparound-safe trigger deadlines.
  - public DIAG_ALRT raw reads consuming CNVRF without stranding pending conversion state.
  - default and maximum conversion-time estimates.
- Updated existing triggered/DIAG_ALRT tests to run against the new completion path.

### Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed modified implementation/docs/test files before commit |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | PASS: 59 tests passed in native environment. PlatformIO warned that obsolete Core 6.1.18 is used and a previous 6.1.19 also exists. |
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

- No hardware validation was performed or claimed.
- Pure ESP-IDF build proof is still missing because `idf.py` is unavailable locally.
- `isConversionReady()` still relies on `Config::nowMs` when a trigger is pending; use `pollConversionReady(nowMs, ready)` or `tick(nowMs)` when no clock hook is installed.
- Conversion timing is still a software readiness estimate plus CNVRF verification, not a hardware timing guarantee validated on physical INA228 devices.
- ENERGY/CHARGE validity, overflow policy, calibration/ADCRANGE atomicity, reset/RSTACC semantics, and broader output atomicity remain for later chunks.

## Chunk 04 - ENERGY, CHARGE, and Overflow Validity

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting commit: `346e2926533a70367a8c72d903db989b9d4f6713`
Commit: pending at report-update time; final response records the committed hash.

### Scope

Chunk 04 prevents ENERGY and CHARGE from being reported as valid when the INA228 cannot provide valid accumulation data. It also preserves overflow evidence before accumulator reads. It does not attempt the full calibration/ADCRANGE atomicity work planned for Chunk 05 or the broader reset policy planned for Chunk 06.

### Subagent Findings

Four read-only agents were used before implementation:

| Agent | Finding summary |
| --- | --- |
| `accumulation-datasheet-agent` | Confirmed ENERGY/CHARGE require continuous conversion semantics, are invalid in triggered mode because elapsed time is not tracked, ENERGYOF clears when ENERGY is read, CHARGEOF clears when CHARGE is read, MATHOF indicates arithmetic overflow, and RSTACC clears ENERGY/CHARGE. |
| `measurement-api-agent` | Found `readMeasurement()`, `readEnergy()`, and `readCharge()` only checked initialization, triggered readiness, and calibration; accumulator reads could clear overflow evidence without a prior DIAG_ALRT snapshot. |
| `compatibility-agent` | Recommended strict scalar accumulator APIs plus structured validity flags in aggregate reads so voltage/current/power users are preserved while energy/charge validity is explicit. |
| `test-agent` | Proposed fake-bus side effects and native tests for invalid modes, overflow preservation/statuses, MATHOF handling, and accumulator reset readiness. |

### Changes

- Added append-only status codes `ACCUMULATION_INVALID` and `ACCUMULATION_OVERFLOW`.
- Added energy/charge validity, overflow, MATHOF, and captured DIAG_ALRT fields to `Measurement` and `RawSample`.
- Added driver-side accumulation readiness tracking. Readiness is invalidated by begin/end/recover reapply, triggered starts/completion, mode changes, ADC timing changes, conversion delay changes, calibration/range changes, soft reset, and accumulator reset.
- Marked accumulation ready only after a continuous-mode DIAG_ALRT read observes CNVRF.
- Made `readEnergy()` and `readCharge()` fail with `ACCUMULATION_INVALID`, `ACCUMULATION_OVERFLOW`, or `MATH_OVERFLOW` instead of returning invalid accumulator data.
- Made `readMeasurement()` keep returning other converted channels while marking energy/charge invalid or overflowed through explicit fields.
- Made `readRawSample()` capture DIAG_ALRT before raw accumulator reads and expose validity/overflow fields.
- Preserved DIAG_ALRT overflow evidence before any typed accumulator read can touch ENERGY or CHARGE.
- Updated README and Doxygen for accumulator validity and reset behavior.

### API Changes

- Added `Err::ACCUMULATION_INVALID`.
- Added `Err::ACCUMULATION_OVERFLOW`.
- Added fields at the end of `Measurement`: `energyValid`, `chargeValid`, `energyOverflow`, `chargeOverflow`, `mathOverflow`, `diagAlertValid`, and `diagAlertRaw`.
- Added the same validity/overflow fields at the end of `RawSample`.
- `readEnergy()` and `readCharge()` now return non-OK statuses for invalid/overflowed accumulation rather than returning stale or invalid converted values.
- `readMeasurement()` uses structured validity flags instead of failing the whole aggregate read for invalid energy/charge.

### Tests Added Or Updated

- Updated the native fake bus to:
  - track register read order in a fixed array;
  - clear ENERGYOF when ENERGY is read;
  - clear CHARGEOF when CHARGE is read;
  - clear fake ENERGY/CHARGE and overflow flags on RSTACC writes.
- Added native tests for:
  - continuous calibrated energy/charge after CNVRF;
  - triggered modes rejecting scalar accumulator reads without accumulator I2C;
  - shutdown modes rejecting scalar accumulator reads without accumulator I2C;
  - `readMeasurement()` marking invalid accumulation without reading accumulators;
  - DIAG_ALRT surfacing and preserving ENERGYOF/CHARGEOF/MATHOF;
  - energy overflow status and preserved evidence;
  - charge overflow status and preserved evidence;
  - MATHOF blocking accumulator reads;
  - RSTACC invalidating accumulation until the next continuous CNVRF.

### Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed modified implementation/docs/test files before commit |
| `git diff --check` | PASS; only Git CRLF working-copy warnings were printed |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | First run failed because new tests used disabled Unity double-precision assertions; tests were changed to enabled float-compatible assertions. Final run PASS: 68 tests passed in native environment. PlatformIO warned that obsolete Core 6.1.18 is used and a previous 6.1.19 also exists. |
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

- No hardware validation was performed or claimed.
- Pure ESP-IDF build proof is still missing because `idf.py` is unavailable locally.
- Generic raw register reads of ENERGY or CHARGE can still be destructive diagnostic access; typed accumulator APIs now preserve evidence first.
- `MATHOF` is now enforced for accumulator reads and surfaced in aggregate flags, but broader current/power validity policy remains part of later API cleanup.
- Calibration/ADCRANGE coherency and multi-register partial-state policy remain for Chunk 05.
- Full reset/RSTACC hardware timing and readback policy remains for Chunk 06.
