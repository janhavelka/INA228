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

## Chunk 05 - Calibration, ADCRANGE Atomicity, Scaling Vectors, and Numeric Safety

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting commit: `430986d2e190caa0829313e1bd908186e513a8b5`
Commit: pending at report-update time; final response records the committed hash.

### Scope

Chunk 05 makes calibration and ADCRANGE scaling coherent across `SHUNT_CAL`, `CURRENT_LSB`, shunt resistance, max expected current, and converted current-derived APIs. It adds a dirty hardware/cache state for unresolved partial writes and exact native vectors for calibration and signed/unsigned conversion behavior. It does not perform hardware validation.

### Subagent Findings

Four read-only agents were used before implementation:

| Agent | Finding summary |
| --- | --- |
| `calibration-datasheet-agent` | Confirmed `CURRENT_LSB = maxExpectedCurrent / 2^19`, `SHUNT_CAL = 13107.2e6 * CURRENT_LSB * RSHUNT`, with a 4x multiplier for ADCRANGE=1. Verified the required 0.0162 ohm / 10 A vectors: `0x0FD2` for ADCRANGE=0 and `0x3F48` for ADCRANGE=1. |
| `numeric-agent` | Found cache/hardware divergence risk in `setAdcRange()`, stale nonzero `SHUNT_CAL` when uncalibrated, missing clamp/dirty diagnostics, and implementation-defined signed conversion risks. |
| `atomicity-agent` | Recommended precomputing calibration before hardware range writes, rollback-or-dirty handling for partial multi-register failures, dirty blocking for converted reads, and `recover()` clearing dirty only after a full config replay. |
| `test-vector-agent` | Proposed exact calibration vectors, positive and negative raw conversion vectors, scripted write-failure cases, dirty/recover tests, invalid-input tests, and threshold scale-change diagnostics. |

### Changes

- Added append-only `Err::HARDWARE_DIRTY`.
- Added calibration and dirty-state diagnostics to `SettingsSnapshot`: `calibrated`, `calibrationClamped`, `maxCurrentExceedsShuntRange`, `hardwareDirty`, `dirtyRegisterMask`, and `thresholdsDirty`.
- Made calibration computation report clamp state and whether requested max current exceeds the selected shunt-voltage full-scale range.
- Kept compatible clamp behavior, but `currentLsb()` now reflects the actual clamped `SHUNT_CAL` and the clamp is visible in settings.
- Made uncalibrated configuration write `SHUNT_CAL=0` so converted current-derived reads cannot use stale hardware calibration.
- Made converted current, power, energy, charge, and power-limit APIs require both calibration and clean hardware/cache state.
- Added finite-output checks for converted current and power float outputs; overflow returns `MATH_OVERFLOW` without overwriting caller outputs.
- Reworked `setAdcRange()` as a guarded CONFIG + SHUNT_CAL transaction: precompute new calibration, write CONFIG, write SHUNT_CAL, roll CONFIG back if SHUNT_CAL fails, and mark CONFIG/SHUNT_CAL dirty if rollback also fails.
- Added dirty-state helpers. Measurement readiness and calibration checks now return `HARDWARE_DIRTY` before touching I2C when cache/hardware coherence is unresolved.
- Made `recover()` and `softReset()` clear dirty only after cached config and calibration replay succeeds.
- Made `_applyConfig()` always replay `SHUNT_TEMPCO`, including zero, so recovery can resync raw diagnostic writes to that register.
- Marked engineering-unit thresholds dirty after ADCRANGE or calibration scale changes; thresholds are not re-encoded automatically.
- Replaced 20-bit and 40-bit sign extension with portable unsigned two's-complement logic.
- Updated Arduino and ESP-IDF example `settings` output and error string tables for the new diagnostics/status.
- Updated README and Doxygen for calibration coherence, dirty state, uncalibrated behavior, and threshold dirty policy.

### API Changes

- Added `Err::HARDWARE_DIRTY`.
- Added fields at the end of `SettingsSnapshot`: `calibrated`, `calibrationClamped`, `maxCurrentExceedsShuntRange`, `hardwareDirty`, `dirtyRegisterMask`, and `thresholdsDirty`.
- Converted current-derived APIs can now return `HARDWARE_DIRTY` if a partial config/calibration update left device registers possibly inconsistent with cache.
- `begin()` with no calibration now programs `SHUNT_CAL=0`; current, power, energy, charge, and power-limit helpers still require a valid calibration.
- `setAdcRange()` can return the original transport error after a failed SHUNT_CAL write while preserving the old cache; if rollback fails, settings expose dirty registers and converted reads fail with `HARDWARE_DIRTY`.

### Tests Added Or Updated

- Updated the native fake bus with queued per-register write failures.
- Added exact calibration vector tests for `R=0.0162 ohm`, `max=10 A`, ADCRANGE=0 and ADCRANGE=1.
- Added ADCRANGE multiplier tests for recomputed `SHUNT_CAL` and unchanged actual `CURRENT_LSB`.
- Added positive raw vector tests for VSHUNT, VBUS, DIETEMP, CURRENT, POWER, ENERGY, and CHARGE conversions.
- Added negative raw and scalar conversion tests for 20-bit and 40-bit signed values.
- Added invalid begin and invalid `setCalibration()` tests proving no I2C/cache mutation on invalid inputs.
- Added float overflow tests for `readPower()` and `readMeasurement()`.
- Added uncalibrated begin test proving `SHUNT_CAL=0`.
- Added `setAdcRange()` scripted failure tests for successful rollback and rollback-failure dirty state.
- Added dirty/recover test for raw diagnostic writes to cached config registers.
- Added threshold dirty test after scale changes.
- Updated the settings snapshot test for calibration and dirty diagnostic fields.

### Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed modified implementation/docs/example/test files before commit |
| `git diff --check` | PASS; only Git CRLF working-copy warnings were printed |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | PASS: 83 tests passed in native environment. PlatformIO warned that obsolete Core 6.1.18 is used and a previous 6.1.19 also exists. |
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
- `maxCurrentExceedsShuntRange` is diagnostic only; the driver still accepts the mathematically valid calibration vector even when the requested max current exceeds the selected ADC shunt-voltage full-scale range.
- Threshold dirty tracking is global, not per-threshold; applications must deliberately reapply engineering-unit thresholds after scale changes before relying on ALERT comparisons.
- `MATHOF` is already enforced for accumulator reads, but broader current/power semantic policy from live DIAG_ALRT remains a later hardening topic.
- Full reset/RSTACC hardware timing and output atomicity policy remains for later chunks.

## Chunk 06 - Reset, RSTACC, Recovery, and Partial-State Handling

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting commit: `d9d41ccd9be5928b8904910d4aa70a1e2ffce3c3`
Commit: pending at report-update time; final response records the committed hash.

### Scope

Chunk 06 defines bounded software reset verification, explicit accumulator reset handling, and recovery replay semantics for partial hardware/cache state. It also locks aggregate measurement output assignment to all-or-nothing behavior under I2C failures. No hardware validation was performed.

### Subagent Findings

Four read-only agents were used before implementation:

| Agent | Finding summary |
| --- | --- |
| `reset-datasheet-agent` | Confirmed software reset via `CONFIG.RST` is POR-equivalent and self-clearing, POR startup is 300 us, `DIAG_ALRT` reset is `0x0001`, `MEMSTAT=1` means trim memory OK, and `RSTACC` clears ENERGY/CHARGE but self-clear is not explicitly proven. Recommended no hidden delay and finite readback verification. |
| `partial-state-agent` | Found `softReset()` and `recover()` could return after partial replay failure without marking clean cache/hardware divergence dirty, threshold registers are reset by software reset but not replayed, and raw register dirty coverage missed some threshold registers and reset-domain writes. |
| `recover-agent` | Found `recover()` could read live `DIAG_ALRT` for MEMSTAT and accidentally overwrite desired cached alert config before replay. Recommended preserving desired DIAG config across verification and replaying cached state with ADC shutdown first and final ADC mode last. |
| `test-agent` | Proposed fake-bus reset modeling, nth read/write failures, reset success/failure tests, `RSTACC` clear/readback tests, recover partial-failure tests, and aggregate all-or-nothing output tests. |

### Changes

- Added `SettingsSnapshot::hardwareDirtyCause` to preserve the first status that made hardware/cache state dirty.
- Added bounded `softReset()` verification:
  - writes `CONFIG.RST`;
  - marks reset-domain config/calibration state dirty until replay succeeds;
  - performs finite `CONFIG.RST/RSTACC` clear readback attempts;
  - verifies manufacturer ID, device ID, and `MEMSTAT`;
  - preserves desired cached DIAG_ALRT alert config while reading live DIAG_ALRT for MEMSTAT;
  - replays cached hardware state and clears dirty only after full success.
- Added cached-hardware resync order for `recover()` and `softReset()`:
  - force `ADC_CONFIG` shutdown first;
  - write cached `CONFIG`, DIAG_ALRT config bits, and `SHUNT_TEMPCO`;
  - write `SHUNT_CAL`;
  - write final cached `ADC_CONFIG` last.
- Made `recover()` use the same verified replay path and mark replay failures dirty instead of returning with ambiguous clean state.
- Made `resetAccumulators()` explicitly write cached `CONFIG` again with `RSTACC` clear, verify reset bits are clear, and invalidate accumulation on attempted hardware reset.
- Extended raw `writeRegister16()` dirty policy for raw `CONFIG.RST`, raw `CONFIG.RSTACC`, and all alert threshold registers.
- Added dirty-state guards to typed configuration, alert, threshold, and accumulator-reset setters; `recover()`, `softReset()`, and raw diagnostics remain the intentional dirty-state escape paths.
- Kept `readMeasurement()` and `readRawSample()` all-or-nothing through local result structs and added tests proving caller outputs remain unchanged after failures at each register read point.
- Updated README, Doxygen comments, and Arduino/ESP-IDF settings output for dirty cause and reset/RSTACC behavior.

### API Changes

- Added `SettingsSnapshot::hardwareDirtyCause`.
- `softReset()` can now return `TIMEOUT` if finite reset-bit readback does not observe `CONFIG.RST/RSTACC` clear.
- `softReset()` and `recover()` leave `hardwareDirty=true` and a dirty register mask if reset/replay cannot be verified.
- `resetAccumulators()` now verifies `RSTACC` clear and can return `HARDWARE_DIRTY` on later typed reads if the reset write or clear/readback path leaves CONFIG uncertain.
- Typed config/alert/threshold setters now return `HARDWARE_DIRTY` without touching I2C while cache/hardware state is unresolved.

### Tests Added Or Updated

- Extended the native fake bus with:
  - attempted write history;
  - per-register nth write failures;
  - per-register nth read failures;
  - CONFIG reset modeling with configurable reset-bit clear behavior;
  - helpers for clearing write/read histories and counting register writes.
- Added native tests for:
  - successful `softReset()` reset-bit verification, identity/MEMSTAT verification, cached DIAG config preservation, replay order, and dirty clear;
  - software reset write failure dirty-domain handling;
  - software reset timeout without replay;
  - `resetAccumulators()` clearing accumulators, clearing overflow flags, explicitly clearing `RSTACC`, and invalidating accumulation until the next continuous CNVRF;
  - `resetAccumulators()` write failure dirty/invalidation handling;
  - soft-reset replay failure at each relevant write position;
  - recover replay failure preserving dirty state until a full successful recover;
  - `readMeasurement()` output unchanged after failures at VSHUNT, VBUS, DIETEMP, CURRENT, POWER, DIAG_ALRT, ENERGY, and CHARGE;
  - `readRawSample()` output unchanged after the same failure points.
- Updated settings snapshot tests for `hardwareDirtyCause`.

### Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed modified implementation/docs/example/test files before commit |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | First run failed because one new nth-write-failure test did not reset fake-bus match counters after begin; test setup was corrected. Final run PASS: 92 tests passed in native environment. PlatformIO warned that obsolete Core 6.1.18 is used and a previous 6.1.19 also exists. |
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
- `softReset()` uses finite readback attempts with no hidden delay; on real hardware an application may need to call it only after bus/device timing is suitable or retry after a `TIMEOUT`.
- Threshold dirty tracking is still global, not per-threshold, and threshold registers are not replayed after software reset.
- RSTACC self-clear remains unproven from local docs, so the driver explicitly clears and verifies CONFIG reset bits instead of relying on self-clear.

## Chunk 07 - Status Errors, Copy/Move Semantics, and Public API Contracts

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting commit: `f1987ddf21246d2a9fbc75d9e45ccda38569f1f4`
Commit: pending at report-update time; final response records the committed hash.

### Scope

Chunk 07 tightens general production contracts after the INA228-specific fixes:
precise startup/probe transport errors, deleted copy/move operations, explicit
public API safety contracts, and status/output-contract tests. No hardware
validation was performed.

### Subagent Findings

Four read-only agents were used before implementation:

| Agent | Finding summary |
| --- | --- |
| `status-agent` | Found `begin()` and `probe()` collapsed raw identity/DIAG_ALRT read failures to `DEVICE_NOT_FOUND`; recover, softReset, and public register helpers already preserved transport codes. Recommended mapping only address NACK to device-not-found and returning timeout/data NACK/bus/generic errors unchanged. |
| `api-agent` | Found Doxygen/README gaps for not-thread-safe/not-ISR-safe behavior, external bus locking, callback re-entry, raw register side effects, aggregate calibration requirements, and output assignment rules. |
| `copy-agent` | Confirmed implicit copy and move were generated and unsafe because copied instances would share transport/user pointers while diverging health/cache/trigger/diagnostic state. Recommended explicit default constructor and deleted copy/move special members with compile-time tests. |
| `compatibility-agent` | Noted that deleting copy/move is source-breaking and granular `begin()`/`probe()` statuses can affect callers that only handled `DEVICE_NOT_FOUND`. Recommended changelog and migration notes. |

### Changes

- Added startup/probe presence-read normalization that maps only
  `Err::I2C_NACK_ADDR` to `Err::DEVICE_NOT_FOUND`.
- Preserved `I2C_NACK_DATA`, `I2C_TIMEOUT`, `I2C_BUS`, `I2C_ERROR`, and generic
  `TIMEOUT` from `begin()` identity/MEMSTAT reads and `probe()`.
- Deleted `INA228::INA228` copy construction, copy assignment, move
  construction, and move assignment while explicitly preserving default
  construction.
- Documented public API contracts:
  - instances are not thread-safe or ISR-safe;
  - shared-bus locking belongs to the application;
  - transport/time callbacks must not re-enter the same instance;
  - output parameters are committed only on `Status::Ok()` unless documented otherwise;
  - raw register access is diagnostic/service-level and can consume status evidence or desynchronize cache;
  - aggregate converted current-derived fields require calibration.
- Added changelog entries for granular startup/probe statuses, deleted
  copy/move, and tightened public contracts.

### API Changes

- `INA228::INA228` remains default-constructible but is no longer copyable or
  movable. Applications should keep instances stable and pass by pointer or
  reference.
- `begin()` and `probe()` now return precise transport failures instead of
  converting all read failures to `DEVICE_NOT_FOUND`. Definite address NACK
  remains mapped to `DEVICE_NOT_FOUND`.
- No new status enum values were added.

### Tests Added Or Updated

- Added compile-time assertions that the driver is default-constructible and
  not copy/move constructible or assignable.
- Updated the failed-`begin()` cache-reset test to expect preserved `TIMEOUT`.
- Updated the probe health-neutral failure test to expect preserved `I2C_ERROR`.
- Added native fake-bus table tests for `begin()` failures at manufacturer ID,
  device ID, and `DIAG_ALRT` startup reads across address NACK, data NACK,
  I2C timeout, bus error, generic I2C error, and generic timeout.
- Added native fake-bus table tests for `probe()` failures at manufacturer ID
  and device ID reads across the same status set, while proving no health
  counters/state change.
- Added public raw-register status propagation tests for tracked 16/24/40-bit
  reads and 16-bit writes.
- Added a status consistency test covering calibration invalid,
  accumulation invalid, and dirty hardware state output preservation.
- Existing destructive `DIAG_ALRT` tests and aggregate all-or-nothing output
  tests from earlier chunks remain in place.

### Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed modified source/header/docs/test files before commit |
| `git diff --check` | PASS; only Git CRLF normalization warnings were printed |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | First run failed because Unity double precision assertions are disabled; the new test was changed to the existing float-within style. Final run PASS: 96 tests passed. PlatformIO warned that obsolete Core 6.1.18 is used and a previous 6.1.19 also exists. |
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
- Pure ESP-IDF build proof is still missing because `idf.py` is unavailable
  locally.
- Deleting copy/move is source-breaking for callers that copied driver objects;
  this is intentional because copied hardware/cache/health state is unsafe.
- Example-level ESP-IDF error mapping and scanner display precision were
  identified by the status agent but left for a later focused chunk to keep this
  chunk scoped to core/public API contracts.

## Chunk 08 - Native Tests, Fake Bus, and Datasheet Vector Coverage

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting commit: `6a987150d7b9ba1f7c46412f1cc7c590171fbe85`
Commit: pending at report-update time; final response records the committed hash.

### Scope

Chunk 08 consolidates and expands native fake-bus coverage after the functional
hardening chunks. The production driver API and implementation were not changed;
all edits are in `test/test_basic.cpp`. No hardware validation was performed.

### Subagent Findings

Five read-only agents were used before implementation:

| Agent | Finding summary |
| --- | --- |
| `coverage-agent` | Reported broad coverage for DIAG_ALRT, triggered freshness, accumulator validity, calibration/scaling, reset/recovery, output atomicity, and public API contracts. Prioritized begin mismatch, threshold vectors, begin/recover partial writes, negative temperature, direct byte-order helper tests, broader timing matrices, and setter write-failure coverage. |
| `fake-bus-agent` | Found useful fake-bus behavior but noted DIAG clear-on-read is opt-in, RSTACC did not clear stale CNVRF, public 24/40-bit helper reads only used zero vectors, and callbacks did not record the configured I2C address. |
| `datasheet-vector-agent` | Verified formula alignment for result bits, signedness, calibration, and timing. Recommended byte-order helper vectors, 20-bit edge vectors, negative temperature vectors, scalar power/energy/charge LSB vectors, timing tables, and config register encoding vectors. |
| `fault-injection-agent` | Found strong existing read/write fault coverage, then prioritized begin apply-write failures, recover replay positions, soft-reset read failures, reset-accumulator later failures, ADCRANGE first-write failure, calibration dirty/recover behavior, alert/threshold write failures, and triggered readiness-gate failures. |
| `regression-agent` | Ran the existing native suite 3 times before edits: 96/96 passed each time, no flake observed, suite time about 1.05-1.14 s, with the recurring PlatformIO Core 6.1.18 obsolete-core warning. |

### Coverage Map

| Area | Before Chunk 08 | Added In Chunk 08 |
| --- | --- | --- |
| Register and identity | Startup/probe transport errors, public ID reads, recover identity mismatch | Begin-time manufacturer mismatch, device mismatch, MEMSTAT failure, configured I2C address propagation, nonzero 16/24/40-bit helper byte-order vectors, raw 16-bit write value capture |
| Calibration/scaling | Exact prompt calibration vectors, ADCRANGE multiplier, clamp, invalid inputs, uncalibrated path, positive/negative prompt conversions | 20-bit edge vectors `+1`, `-1`, max positive, min negative, low-nibble masking, ADCRANGE initial CONFIG-write failure, calibration write-failure dirty/recover assertions |
| Measurement vectors | Positive full vector, negative shunt/current/charge sign extension, overflow handling | Negative/positive temperature scalar vectors and scalar power/energy/charge LSB vectors |
| DIAG/alerts | Clear-on-read tests, CNVRF preservation, alert setters not reading live DIAG_ALRT, public destructive reads | Fake RSTACC now clears stale CNVRF so accumulation requires a new conversion-ready event |
| Triggered/timing | Begin-time triggered modes, not-ready gating, CNVRF completion, no-now hook, wraparound | Conversion-time enum table, averaging table, delay vector, ADC_CONFIG and CONFIG encoding vectors |
| Partial failure/recovery | Soft-reset replay write positions, recover one replay failure, reset accumulator first-write failure, aggregate output all-or-nothing | Begin apply-write failure table, recover identity/MEMSTAT read-failure table, recover replay write-position table, soft-reset read-failure table, reset-accumulator second-write/readback/stuck-bit failures |
| API contracts | Copy/move deletion, precise startup/probe errors, raw register status propagation | Threshold exact encoding and threshold write-failure preservation across all six threshold registers |

### Changes

- Extended the fake bus to record the last I2C address passed to write and
  write-read callbacks.
- Added a configurable fake CONFIG read override for reset-bit-stuck tests.
- Tightened fake RSTACC behavior so accumulator reset also clears stale CNVRF
  in the fake device state.
- Added 15 native tests, increasing the suite from 96 to 111 `RUN_TEST`s.

### API Changes

- None.

### Tests Added Or Updated

- Added begin-time manufacturer-ID mismatch, device-ID mismatch, and MEMSTAT
  failure tests proving no configuration writes occur before identity is valid.
- Added begin apply-write failure coverage for CONFIG, DIAG_ALRT, SHUNT_TEMPCO,
  ADC_CONFIG, and SHUNT_CAL.
- Added configured I2C address propagation coverage.
- Added recover read-failure coverage for manufacturer ID, device ID, and
  DIAG_ALRT/MEMSTAT reads.
- Added recover replay write-failure coverage for ADC_CONFIG shutdown, CONFIG,
  DIAG_ALRT, SHUNT_TEMPCO, SHUNT_CAL, and final ADC_CONFIG writes.
- Added soft-reset read-failure coverage for reset-bit CONFIG read and
  post-reset manufacturer/device/DIAG_ALRT reads.
- Added reset-accumulator second-write, readback I2C failure, and reset-bit
  stuck timeout tests.
- Added nonzero public raw helper byte-order vectors:
  - `readRegister16(REG_SHUNT_TEMPCO) = 0xA5C3`;
  - `readRegister24(REG_POWER) = 0x123456`;
  - `readRegister40(REG_ENERGY) = 0x0102030405`;
  - `writeRegister16(REG_SHUNT_TEMPCO, 0xBEEF)`.
- Added 20-bit edge vectors and low-nibble masking coverage.
- Added negative/positive temperature vectors:
  `0xFFFF`, `0xF380`, `0xEC00`, and `0x3E80`.
- Added scalar power/energy/charge LSB vectors.
- Added conversion-time enum, averaging, and conversion-delay tables.
- Added ADC_CONFIG/CONFIG register encoding vectors.
- Added `setAdcRange()` initial CONFIG-write failure coverage.
- Extended calibration write-failure coverage to assert dirty state and
  successful recovery.
- Added exact threshold encodings for SOVL, SUVL, BOVL, BUVL, TEMP_LIMIT, and
  PWR_LIMIT plus write-failure preservation for all six threshold registers.

### Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed `M test/test_basic.cpp` before commit |
| `git diff --check` | PASS; only Git CRLF normalization warning was printed |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | Pre-edit regression agent ran 3 passes at 96/96. After edits, first local run failed because the new threshold write-failure test assumed reset defaults that the fake bus only applies after reset; test setup was corrected. Final two runs PASS: 111/111 each. PlatformIO warned that obsolete Core 6.1.18 is used and a previous 6.1.19 also exists. |
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
- Pure ESP-IDF build proof is still missing because `idf.py` is unavailable
  locally.
- The fake bus still does not enforce a full register metadata model for
  read/write permissions or invalid register widths.
- DIAG_ALRT clear-on-read remains opt-in for tests that need destructive status
  behavior, rather than the fake bus defaulting all DIAG reads to destructive.
- Example-level ESP-IDF NACK mapping and scanner display precision remain for a
  later focused chunk.

## Chunk 09 - ESP-IDF, Arduino Examples, and CI Build Matrix

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting commit: `0814ecebbfe3e54c48f913bd3201d46ee44fb5c2`
Commit: pending at report-update time; final response records the committed hash.

### Scope

Chunk 09 made build and example claims more reproducible and explicit. It added
CI coverage for pure ESP-IDF builds, tightened the native IDF example boundary,
aligned Arduino/ESP-IDF CLI warnings, and documented local validation limits.
No hardware validation was performed.

### Subagent Findings

Five read-only agents were used before implementation:

| Agent | Finding summary |
| --- | --- |
| `ci-agent` | Existing CI ran PlatformIO S2/S3 builds, native tests, static contract checks, and package validation but no `idf.py` build. Recommended an ESP-IDF `esp32s3`/`esp32s2` matrix using Espressif's CI action and IDF v6.0.1. |
| `idf-agent` | Confirmed the native IDF example is pure IDF, but probe NACK mapping was generic, `scanina` hid `I2C_ERROR` as an empty address, the example was not labeled as single-owner glue, and `std::fgets()` blocks periodic `tick()` progress. |
| `arduino-agent` | Confirmed local PlatformIO/Arduino structure and API usage looked source-correct. Noted unpinned `platform = espressif32`, weaker destructive/accumulation help text, and unconditional aggregate output without validity flags. |
| `docs-agent` | Recommended a prominent high-voltage safety warning, clearer validation wording, single-owner IDF adapter labeling, destructive DIAG help text, accumulation validity help/output, and IDF raw-write help parity. |
| `review-agent` | Confirmed no core framework leak, but CI evidence was weaker than ESP-IDF support claims, IDF main CMake exposed the repo root include path, and the core timing guard was too narrow. |

### Changes

- Added `.github/workflows/ci.yml` job `esp-idf-basic` for `esp32s3` and
  `esp32s2` using `espressif/esp-idf-ci-action@v1` with ESP-IDF `v6.0.1`.
- Narrowed the ESP-IDF example main component `INCLUDE_DIRS` to `"."` and rely
  on `REQUIRES INA228` for library headers.
- Labeled the ESP-IDF transport as single-owner diagnostic example glue, not a
  production shared-bus/multitask manager.
- Split ESP-IDF error mapping into probe and transfer paths:
  - `i2c_master_probe()` `ESP_ERR_INVALID_RESPONSE`/`ESP_ERR_NOT_FOUND` maps to
    `I2C_NACK_ADDR`;
  - transfer-time `ESP_ERR_INVALID_RESPONSE` remains `I2C_ERROR` because the
    standard API does not prove address versus data phase;
  - `ESP_ERR_TIMEOUT` remains timeout and `ESP_ERR_INVALID_ARG` maps to
    `INVALID_PARAM`.
- Updated IDF `scanina` so only `I2C_NACK_ADDR` is shown as an empty address;
  timeouts, bus errors, and generic I2C errors remain visible.
- Aligned Arduino and ESP-IDF CLI help/runtime text for:
  - destructive/status-clearing `DIAG_ALRT` reads;
  - `scanina`, `probe`, and `selftest` consuming DIAG_ALRT evidence;
  - continuous-accumulation-only energy/charge reads;
  - raw register diagnostic access and raw write cache-desync risk;
  - high-voltage safety warnings.
- Added energy/charge validity and overflow flag output to Arduino and ESP-IDF
  aggregate measurement/raw sample views.
- Neutralized package metadata and README wording from "Production-grade" to
  "Framework-neutral" until validation evidence is broader.
- Added README/Doxygen safety wording for high-voltage systems.
- Strengthened `tools/check_core_timing_guard.py` to reject core Wire/Arduino,
  ESP-IDF/FreeRTOS includes, example includes, platform timing calls, and common
  framework tokens in `include/` and `src/`.
- Strengthened `tools/check_idf_example_contract.py` to require the ESP-IDF CI
  matrix, narrowed CMake include boundary, IDF probe NACK mapping tokens, and
  aligned CLI warning tokens.

### API Changes

- No functional core API change.
- Public Doxygen gained a high-voltage safety warning.
- `library.json` and `idf_component.yml` description text changed from
  "Production-grade" to "Framework-neutral".

### Tests Added Or Updated

- No native driver tests were added in this chunk.
- Static contract checks were extended to cover the new CI/example contracts.

### Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed only intended modifications before commit; generated package tarball was removed |
| `git diff --check` | PASS; only Git CRLF normalization warnings were printed |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | PASS: 111/111 native tests succeeded in 1.245 s; PlatformIO warned obsolete Core 6.1.18 is used and 6.1.19 also exists |
| `python -m platformio run -e esp32s3dev` | PASS: ESP32-S3 Arduino build succeeded with PlatformIO espressif32 54.3.20, Arduino 3.2.0, IDF libs 5.4.0 |
| `python -m platformio run -e esp32s2dev` | PASS: ESP32-S2 Arduino build succeeded with PlatformIO espressif32 54.3.20, Arduino 3.2.0, IDF libs 5.4.0 |
| `python -m platformio pkg pack` | PASS: wrote `INA228-1.3.0.tar.gz`; tarball was removed after recording the result |
| `idf.py --version` | NOT AVAILABLE: PowerShell reported `idf.py : The term 'idf.py' is not recognized as the name of a cmdlet, function, script file, or operable program.` |

### Commands Not Run

| Command | Reason |
| --- | --- |
| `idf.py -C examples/esp_idf/basic set-target esp32s3 build` | Not run because `idf.py --version` failed; ESP-IDF is not available on PATH in this environment. CI is now configured to run this command-equivalent target. |
| `idf.py -C examples/esp_idf/basic set-target esp32s2 build` | Not run because `idf.py --version` failed; ESP-IDF is not available on PATH in this environment. CI is now configured to run this command-equivalent target. |

### Remaining Risks

- No hardware validation was performed or claimed.
- Local pure ESP-IDF build proof is still missing because `idf.py` is
  unavailable locally; the new GitHub Actions job must be reviewed after push.
- `espressif/esp-idf-ci-action@v1` is a moving major action tag and adds CI time.
- The IDF example CLI still uses blocking `std::fgets()`, so `tick()` advances
  between commands rather than on a periodic console loop.
- ESP-IDF transfer-time NACKs remain `I2C_ERROR` because the standard transfer
  APIs do not expose address versus data phase.
- `platform = espressif32` remains unpinned; local S2/S3 builds passed here, but
  future PlatformIO framework changes can affect reproducibility.

## Chunk 10 - Documentation, High-Voltage Safety, and Hardware Validation Matrix

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Starting commit: `1da6bd0a91d85c846703dd7275aea74fd8d71426`
Commit: pending at report-update time; final response records the committed hash.

### Scope

Chunk 10 aligned user-facing documentation with the hardened driver behavior and
created the hardware validation matrix required before any field-proven or
fully production-grade claim. No hardware validation was performed.

### Subagent Findings

Four read-only agents were used before implementation:

| Agent | Finding summary |
| --- | --- |
| `safety-doc-agent` | Found that high-voltage safety existed but was too compressed, shunt power/Kelvin/transient guidance was not surfaced, ALERT/measurements needed explicit non-safety-function wording, and "owner hardware-test coverage" was unsupported without checked-in logs. |
| `api-doc-agent` | Found README/Doxygen gaps around `readMeasurement()` requiring calibration, `readRawSample()` accumulator side effects, DIAG_ALRT reads during begin/recover/reset, `nowMs` versus `tick(nowMs)` triggered timing, readiness output clearing, `MATH_OVERFLOW`, sticky `thresholdsDirty`, cached `currentLsb()`, and omitted public APIs. |
| `validation-agent` | Designed `docs/INA228_HARDWARE_VALIDATION_MATRIX.md` with required metadata, command/log presets, evidence-path pattern, and initial `NOT RUN` rows for address scan, identity, MEMSTAT, known voltage/current vectors, ADCRANGE, timing, modes, accumulators, alerts, faults, soak, Arduino S2/S3, and pure ESP-IDF S2/S3. |
| `release-honesty-agent` | Found unsupported "owner hardware-test coverage" claims in README, CHANGELOG, and IDF docs; found no remaining positive claims of "fully industry-grade", "field-proven", "85 V safe", or "validated on ESP-IDF"; recommended qualified wording and changelog updates. |

### Changes

- Expanded README safety guidance to state the 85 V rating is an IC capability,
  not a system safety rating, and to call out isolation, fusing, transient
  protection, creepage/clearance, shunt heating, grounding, USB-ground hazards,
  and qualified handling.
- Added README shunt/layout/calibration guidance:
  - shunt range limits for both ADCRANGE settings;
  - `I^2 * R` dissipation and derating;
  - Kelvin connections and balanced sense traces;
  - input filtering/transient protection within datasheet limits;
  - actual versus requested calibration fields in `SettingsSnapshot`;
  - uncalibrated behavior and threshold reapply policy.
- Tightened README API/behavior contracts:
  - `readMeasurement()` requires calibration;
  - `readRawSample()` is diagnostic/raw and can consume accumulator overflow
    evidence;
  - readiness APIs clear their `ready` output before polling;
  - triggered reads need `tick(nowMs)` or `pollConversionReady(nowMs, ready)`
    when `Config::nowMs` is unset;
  - added omitted public APIs to the overview.
- Updated public Doxygen in `include/INA228/INA228.h` and `Config.h` for:
  - high-voltage and bus-ownership warnings;
  - measurements/ALERT not being safety functions;
  - DIAG_ALRT verification side effects during begin/recover/soft reset;
  - raw sample and accumulator side effects;
  - `MATH_OVERFLOW` on energy/charge reads;
  - sticky `thresholdsDirty`;
  - cached `currentLsb()` caveat.
- Created `docs/INA228_HARDWARE_VALIDATION_MATRIX.md` with required hardware
  validation rows all marked `NOT RUN`.
- Removed unsupported "owner hardware-test coverage" wording from README,
  CHANGELOG, `docs/IDF_PORT.md`, and `docs/IDF_PORT_IMPLEMENTATION.md`.
- Added a historical-state note to the exploration report so stale pre-hardening
  findings are not mistaken for current validation status.
- Updated CHANGELOG Unreleased entries for safety docs, validation matrix, and
  honesty wording.

### API Changes

- No functional API change.
- Public Doxygen comments were expanded and corrected.

### Tests Added Or Updated

- No native driver tests were added in this documentation chunk.
- Added a documentation-only hardware validation matrix with all hardware rows
  marked `NOT RUN`.

### Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed only intended documentation/header edits and new `docs/INA228_HARDWARE_VALIDATION_MATRIX.md` before commit |
| `git diff --check` | PASS; only Git CRLF normalization warnings were printed |
| `python tools/check_core_timing_guard.py` | PASS: `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | PASS: `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | PASS: `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | PASS: `Up to date: C:\Users\Honza\Documents\Projects\INA228\include\INA228\Version.h` |
| `python -m platformio test -e native` | PASS: 111/111 native tests succeeded in 1.106 s; PlatformIO warned obsolete Core 6.1.18 is used and 6.1.19 also exists |
| `python -m platformio run -e esp32s3dev` | PASS: ESP32-S3 Arduino build succeeded with PlatformIO espressif32 54.3.20, Arduino 3.2.0, IDF libs 5.4.0 |
| `python -m platformio run -e esp32s2dev` | PASS: ESP32-S2 Arduino build succeeded with PlatformIO espressif32 54.3.20, Arduino 3.2.0, IDF libs 5.4.0 |
| `python -m platformio pkg pack` | PASS: wrote `INA228-1.3.0.tar.gz`; tarball was removed after recording the result |
| `idf.py --version` | NOT AVAILABLE: PowerShell reported `idf.py : The term 'idf.py' is not recognized as the name of a cmdlet, function, script file, or operable program.` |

### Commands Not Run

| Command | Reason |
| --- | --- |
| `idf.py -C examples/esp_idf/basic set-target esp32s3 build` | Not run because `idf.py --version` failed; ESP-IDF is not available on PATH in this environment. CI is configured to build this target. |
| `idf.py -C examples/esp_idf/basic set-target esp32s2 build` | Not run because `idf.py --version` failed; ESP-IDF is not available on PATH in this environment. CI is configured to build this target. |

### Remaining Risks

- No hardware validation was performed or claimed; every hardware matrix row is
  currently `NOT RUN`.
- Local pure ESP-IDF build proof is still missing because `idf.py` is not
  available in this shell.
- GitHub Actions ESP-IDF results must still be reviewed after push.
- The matrix is a template until dated logs, equipment metadata, board/module
  details, and pass/fail evidence are checked in.
