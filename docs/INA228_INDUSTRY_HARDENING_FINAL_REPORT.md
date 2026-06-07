# INA228 Industry Hardening Final Report

Date: 2026-06-05
Branch: `hardening/ina228-industry-readiness`
Branch base: `27fb6978b8fecca40b267d2236fe87a4651843c0`
Report starting commit: `f99e2273a3d729e873310aa683622c9eab01992d`
ESP-IDF build-proof update: 2026-06-07; see
`docs/INA228_ESPIDF_BUILD_PROOF_REPORT.md`.
Release-prep update: 2026-06-07; version metadata now targets `2.0.0` as an
unreleased hardening merge candidate.

## Executive Summary

Chunks 01-11 moved the driver from an engineering-grade baseline with major
INA228 semantic gaps to an industry-readiness hardened merge candidate. The core
remains framework-neutral and transport-injected. Native fake-bus tests now
cover destructive diagnostics, triggered freshness, accumulation validity,
calibration/range scaling, reset/recovery, dirty hardware state, status
precision, and API contracts.

The branch is not release-ready. Local native tests and PlatformIO Arduino
ESP32-S2/S3 builds pass, but local pure ESP-IDF builds were not run because
`idf.py` is unavailable, current branch CI results still need review, and all
hardware validation matrix rows remain `NOT RUN`.

## What Changed

- Added INA228-specific project rules and progress tracking.
- Reworked `DIAG_ALRT` handling so alert config writes do not read live status,
  destructive reads preserve observable evidence, and `tick()` no longer polls
  `DIAG_ALRT` after there is no driver-owned trigger or accumulator-readiness
  state to advance.
- Defined latest-versus-fresh measurement semantics and made triggered reads
  return not-ready until completion is observed.
- Guarded ENERGY/CHARGE validity by mode, calibration, continuous CNVRF
  readiness, accumulator reset, and overflow flags.
- Hardened calibration, ADCRANGE, `SHUNT_CAL`, `CURRENT_LSB`, threshold-dirty,
  and dirty hardware/cache state behavior.
- Made reset/RSTACC/recovery bounded and testable, with all-or-nothing
  aggregate output assignment.
- Preserved transport error precision and deleted copy/move operations.
- Added pure ESP-IDF example/CI coverage configuration and tightened example
  contracts.
- Expanded safety, validation, and release-honesty documentation.
- Fixed final-review findings: `MATHOF` now blocks scalar current/power and
  aggregate converted reads; ambiguous cached-register write failures mark
  hardware dirty; older application-note/manual wording no longer overclaims
  ALERT protection, high-voltage suitability, or raw accumulator validity.

## Public API Changes

- Added/expanded diagnostic state in `Measurement`, `RawSample`,
  `SettingsSnapshot`, and `DiagAlertSnapshot`.
- Added `getDiagAlertSnapshot()` and `pollConversionReady(nowMs, ready)`.
- Added or consistently uses `ACCUMULATION_INVALID`,
  `ACCUMULATION_OVERFLOW`, `HARDWARE_DIRTY`, and `MATH_OVERFLOW` statuses for
  semantic failures.
- Deleted `INA228` copy and move constructors/assignments.
- Public Doxygen now states non-thread-safe/non-ISR-safe behavior, external bus
  ownership/locking, callback re-entry restrictions, destructive diagnostic
  reads, raw-register risks, calibration requirements, and hardware validation
  limits.

## Datasheet-Correctness Improvements

- `DIAG_ALRT` is treated as configuration plus destructive/live status.
- CNVRF reads complete pending triggered conversions without stranding state.
- ENERGY/CHARGE are invalid outside supported continuous accumulation modes.
- ENERGYOF/CHARGEOF/MATHOF are surfaced before destructive accumulator reads.
- `MATHOF` blocks current-derived converted reads.
- `SHUNT_CAL`, ADCRANGE, current LSB, shunt resistance, and max current are a
  coherent contract with exact vectors and dirty-state protection.
- Signed 20-bit VSHUNT/CURRENT, unsigned VBUS/POWER/ENERGY, signed 40-bit
  CHARGE, and temperature vectors are covered.
- Reset/RSTACC/recover paths verify bounded readback and replay cached state
  with explicit dirty state on partial failure.

## Safety Improvements

- README and public headers now warn that the 85 V value is an IC input
  capability, not a system safety rating.
- Documentation calls out isolation, fusing, grounding, USB-ground hazards,
  transients, creepage/clearance, shunt dissipation, Kelvin layout, and
  qualified handling.
- ALERT and measurement outputs are documented as monitoring aids, not certified
  safety functions.
- Hardware validation matrix requires dated commit-linked logs before any
  hardware or field-readiness claim.

## Tests Added

- Native Unity tests increased to 114.
- Fake bus models DIAG clear-on-read, accumulator overflow clearing, scripted
  read/write failures, reset behavior, and exact raw-register vectors.
- Coverage includes DIAG evidence preservation, alert config side effects,
  triggered timing/freshness, accumulation validity/overflow, calibration and
  ADCRANGE vectors, dirty/recover behavior, output atomicity, status precision,
  copy/move deletion, and public destructive diagnostic semantics.

## CI / Build Coverage

- Local PlatformIO native tests pass.
- Local PlatformIO Arduino builds pass for `esp32s3dev` and `esp32s2dev`.
- Package packing passes and generated tarball was removed.
- GitHub Actions includes an ESP-IDF S2/S3 build matrix, but current remote CI
  run results were not verified in this shell.
- Local pure ESP-IDF builds were not run because `idf.py` is not installed or
  not on PATH.
- The 2026-06-07 ESP-IDF build-proof pass added a reproducible build guide,
  manual workflow trigger, and guard coverage for the ESP-IDF build contract.
  Local `idf.py` remained unavailable and remote CI status was not checked
  because `gh` is not installed in this shell.
- Release-prep metadata now targets `2.0.0` and PlatformIO Arduino builds are
  pinned to `platformio/espressif32@7.0.1` for reproducible package resolution.

## Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | showed intended modified files before final report |
| `git diff --check` | PASS; only Git CRLF normalization warnings |
| `python tools/check_core_timing_guard.py` | PASS |
| `python tools/check_cli_contract.py` | PASS |
| `python tools/check_idf_example_contract.py` | PASS |
| `python scripts/generate_version.py check` | PASS; `Version.h` up to date |
| `python -m platformio test -e native` | PASS: 114/114 native tests |
| `python -m platformio run -e esp32s3dev` | PASS |
| `python -m platformio run -e esp32s2dev` | PASS |
| `python -m platformio pkg pack` | PASS; wrote package tarball for the then-current version, removed afterward |
| `idf.py --version` | FAIL: `idf.py` is not recognized as a command |

## Commands Not Run

| Command | Reason |
| --- | --- |
| `idf.py -C examples/esp_idf/basic set-target esp32s3 build` | Not run because `idf.py --version` failed locally. |
| `idf.py -C examples/esp_idf/basic set-target esp32s2 build` | Not run because `idf.py --version` failed locally. |
| Hardware validation procedures | Not run; no approved hardware setup/log capture was available. |

## Hardware Validation Status

No hardware validation was performed or claimed. All rows in
`docs/INA228_HARDWARE_VALIDATION_MATRIX.md` remain `NOT RUN`.

## Remaining Risks

- Current GitHub Actions results for this branch still need review.
- Local pure ESP-IDF build proof is missing.
- `library.json`, generated `Version.h`, and `idf_component.yml` now target
  `2.0.0`, but this remains an unreleased merge candidate until CI and release
  checks are reviewed.
- No real-device timing, alert-pin, reset/brownout, NACK/unplug, bidirectional
  current, high-voltage, or 24-72h soak logs are checked in.
- PlatformIO Arduino environments are pinned to `platformio/espressif32@7.0.1`;
  future maintainers should update the pin deliberately with CI evidence.
- Native tests are deterministic and broad, but they are not hardware-in-loop,
  sanitizer, or multi-OS coverage.

## Merge Readiness

Ready to merge, not ready to release.

This branch is suitable for PR review and merge after current CI is green. It
should not be tagged or advertised as release-ready until the release blockers
below are handled.

## Release Wording

Allowed wording:

- "Framework-neutral INA228 driver."
- "Industry-readiness hardened pre-production candidate pending checked-in
  hardware validation."
- "Native tests and PlatformIO Arduino S2/S3 builds pass locally on this
  checkout."
- "ESP-IDF example/component support is implemented and CI is configured,
  pending current CI result review."
- "Hardware validation matrix exists; hardware validation remains NOT RUN."

Avoid wording:

- "Production-ready", "fully industry-grade", "field-proven", or
  "hardware validated".
- "85 V safe" or wording implying system safety certification.
- "ESP-IDF build verified" without actual local or CI `idf.py` logs.
- "Ready for release/tag" before CI, package, and validation decisions.

## Follow-up Work

- Open or update a PR and confirm green CI, including ESP-IDF jobs.
- Install ESP-IDF locally or capture CI logs for both pure ESP-IDF targets.
- Review current CI results for the `2.0.0` merge candidate before tagging.
- Run the hardware validation matrix with dated logs, equipment details, board
  details, shunt metadata, pass/fail results, and commit hash.
- Consider optional sanitizer/coverage jobs.
- Consider a production ESP-IDF shared-bus adapter example if that becomes a
  supported deliverable.
