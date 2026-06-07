# INA228 Release Prep Report

Date: 2026-06-07
Branch: `hardening/ina228-industry-readiness`
Starting commit: `0f9d5bc72304264ff428ff82dee67084ce409b1f`

## Summary

This focused pass prepared the hardening branch for merge review without making
hardware-validation claims. It verified the CI configuration, ran the available
local checks, made the SemVer/package metadata consistent with the breaking
API/behavior changes, pinned the PlatformIO ESP32 platform, and kept release
wording conservative.

## Files Changed

- `library.json`
- `include/INA228/Version.h`
- `idf_component.yml`
- `platformio.ini`
- `CHANGELOG.md`
- `README.md`
- `docs/INA228_INDUSTRY_HARDENING_FINAL_REPORT.md`
- `docs/INA228_RELEASE_PREP_REPORT.md`

## Version Decision

Target version is `2.0.0`.

This is a major bump under the repository SemVer policy because the hardening
branch deletes `INA228::INA228` copy/move operations and changes public
measurement behavior to return explicit statuses for not-ready, dirty,
calibration-invalid, accumulation-invalid, accumulation-overflow, and math
overflow conditions instead of silently returning stale or invalid converted
values.

Updated consistently:

- `library.json`: `2.0.0`
- generated `include/INA228/Version.h`: `2.0.0`
- `idf_component.yml`: `2.0.0`
- `README.md`: `2.0.0` hardening merge candidate wording
- `CHANGELOG.md`: Unreleased target version and major-bump rationale

## Changelog Changes

The Unreleased section now summarizes `DIAG_ALRT` destructive-read hardening,
triggered freshness semantics, ENERGY/CHARGE validity guards, `MATHOF`
handling, calibration/ADCRANGE dirty-state protection, reset/RSTACC/recovery
hardening, status/error precision, deleted copy/move operations, safety docs,
114 native tests, local Arduino S2/S3 build pass, ESP-IDF CI configuration, and
hardware validation still `NOT RUN`.

## CI Coverage Status

`.github/workflows/ci.yml` includes:

- native PlatformIO tests through `pio test -e native`;
- Arduino ESP32-S3 build through `pio run -e esp32s3dev`;
- Arduino ESP32-S2 build through `pio run -e esp32s2dev`;
- core timing guard;
- CLI contract guard;
- IDF example contract guard;
- generated version check;
- package validation through `pio pkg pack`;
- pure ESP-IDF `idf.py` builds for `esp32s3` and `esp32s2` using
  `espressif/esp-idf-ci-action@v1`, ESP-IDF `v6.0.1`, and
  `path: examples/esp_idf/basic`;
- `workflow_dispatch` for manual CI runs.

Current CI result status was not proven locally. `gh --version` failed because
GitHub CLI is not installed. The public GitHub Actions API request for branch
`hardening/ina228-industry-readiness` returned `total_count=0`, so no current
branch workflow run was available to review from this shell.

## PlatformIO Pinning

`platformio.ini` now pins Arduino ESP32 environments to
`platformio/espressif32@7.0.1`.

Reason: unpinned `platform = espressif32` made local and CI package resolution
depend on the registry state at build time. `platformio/espressif32` `7.0.1`
is the current public registry version reported locally, and both ESP32-S2 and
ESP32-S3 builds pass with the pin.

## Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | Startup was clean before edits; final status showed intended modified files. |
| `git branch --show-current` | `hardening/ina228-industry-readiness` |
| `git log --oneline -5` | HEAD `0f9d5bc ci: verify INA228 ESP-IDF example builds`; prior hardening commits present. |
| `gh --version` | FAIL: `gh` is not recognized as a command. |
| GitHub Actions API for branch | PASS request; `total_count=0`, no current branch CI run found. |
| `git ls-remote origin refs/heads/hardening/ina228-industry-readiness refs/heads/main` | PASS; origin hardening branch at `0f9d5bc72304264ff428ff82dee67084ce409b1f`, main at `27fb6978b8fecca40b267d2236fe87a4651843c0`. |
| `python -m platformio platform show espressif32` | PASS; public `platformio/espressif32` version `7.0.1` reported. PlatformIO warned that obsolete Core `6.1.18` is active. |
| `git diff --check` | PASS; Git reported CRLF normalization warnings only. |
| `python tools/check_core_timing_guard.py` | PASS. |
| `python tools/check_cli_contract.py` | PASS. |
| `python tools/check_idf_example_contract.py` | PASS. |
| `python scripts/generate_version.py check` | PASS; `Version.h` up to date. |
| `python -m platformio test -e native` | PASS: 114/114 tests. PlatformIO warned that obsolete Core `6.1.18` is active. |
| `python -m platformio run -e esp32s3dev` | PASS with `platformio/espressif32@7.0.1`; existing `CliStyle.h` C++17 inline-variable warning emitted. |
| `python -m platformio run -e esp32s2dev` | PASS with `platformio/espressif32@7.0.1`; existing `CliStyle.h` C++17 inline-variable warning emitted. |
| `python -m platformio pkg pack` | PASS; wrote `INA228-2.0.0.tar.gz`, removed afterward. |
| `idf.py --version` | FAIL: `idf.py` is not recognized as a command. |

## Commands Not Run

| Command | Reason |
| --- | --- |
| `idf.py -C examples/esp_idf/basic set-target esp32s3 build` | Not run because `idf.py --version` failed locally. |
| `idf.py -C examples/esp_idf/basic set-target esp32s2 build` | Not run because `idf.py --version` failed locally. |
| Hardware validation procedures | Not run; no approved hardware setup or log-capture procedure was available. |

## Remaining Release Blockers

- Review a current green CI run for this branch or PR, including ESP-IDF jobs.
- Capture local or CI `idf.py` logs before claiming ESP-IDF build verification.
- Hardware validation matrix remains `NOT RUN`; no hardware or field-readiness
  claim is allowed.
- Decide final release timing and tag policy after CI review.
- Optional: update the local PlatformIO Core installation to remove the
  obsolete-core warning.

## Merge Verdict

Ready to merge after current CI is green.

The local no-hardware checks available in this shell pass, and release metadata
is now internally consistent for a `2.0.0` hardening merge candidate.

## Release / Tag Verdict

Not ready to release or tag as field-ready or industry-grade.

`2.0.0` is the correct target version for the breaking hardening changes, but a
tag should wait for current CI review and release-owner approval. Any
industry-grade or hardware-validated wording requires dated, commit-linked
hardware validation logs.

## Revalidation Update

Date: 2026-06-07
Starting commit: `f18e70648df3866971227a6a16f0a74124772dfa`

This follow-up pass found no additional non-hardware release blockers and made
no code or metadata changes. The existing `2.0.0` version decision,
`platformio/espressif32@7.0.1` pin, changelog summary, and conservative release
wording remain appropriate.

Additional commands run:

| Command | Result |
| --- | --- |
| `git status --short` | PASS at startup: clean worktree. |
| `git branch --show-current` | `hardening/ina228-industry-readiness` |
| `git log --oneline -5` | HEAD `f18e706 chore: prepare INA228 hardening branch for merge`. |
| `gh --version` | FAIL: `gh` is not recognized as a command. |
| GitHub Actions API for branch | PASS request; `total_count=0`, no current branch CI run found. |
| `git ls-remote origin refs/heads/hardening/ina228-industry-readiness refs/heads/main` | PASS; origin hardening branch at `f18e70648df3866971227a6a16f0a74124772dfa`, main at `27fb6978b8fecca40b267d2236fe87a4651843c0`. |
| `git diff --check` | PASS. |
| `python tools/check_core_timing_guard.py` | PASS. |
| `python tools/check_cli_contract.py` | PASS. |
| `python tools/check_idf_example_contract.py` | PASS. |
| `python scripts/generate_version.py check` | PASS; `Version.h` up to date. |
| `python -m platformio test -e native` | PASS: 114/114 tests. PlatformIO warned that obsolete Core `6.1.18` is active. |
| `python -m platformio run -e esp32s3dev` | PASS with `platformio/espressif32@7.0.1`; existing `CliStyle.h` C++17 inline-variable warning emitted. |
| `python -m platformio run -e esp32s2dev` | PASS with `platformio/espressif32@7.0.1`; existing `CliStyle.h` C++17 inline-variable warning emitted. |
| `python -m platformio pkg pack` | PASS; wrote `INA228-2.0.0.tar.gz`, removed afterward. |
| `idf.py --version` | FAIL: `idf.py` is not recognized as a command. |

Commands still not run:

| Command | Reason |
| --- | --- |
| `idf.py -C examples/esp_idf/basic set-target esp32s3 build` | Not run because `idf.py --version` failed locally. |
| `idf.py -C examples/esp_idf/basic set-target esp32s2 build` | Not run because `idf.py --version` failed locally. |
| Hardware validation procedures | Not run; no approved hardware setup or log-capture procedure was available. |
