# INA228 ESP-IDF Build Proof Status Report

Date: 2026-06-07
Branch: `hardening/ina228-industry-readiness`
Starting commit: `4f70ef4b0019d14fdd26a24bc01a809d3ac5f45f`

## Summary

This focused pass reviewed and hardened the pure ESP-IDF build path without
changing device semantics. The repository already had a GitHub Actions
`esp-idf-basic` matrix using `espressif/esp-idf-ci-action@v1` and ESP-IDF
v6.0.1 for `esp32s3` and `esp32s2`. This pass added a manual workflow trigger,
made the IDF build step label explicit, documented local reproduction commands,
and extended the static ESP-IDF contract guard to require the build guide and
CI matrix shape.

Pure ESP-IDF build proof is still configuration-only in this shell. Local
`idf.py` is not installed or not on `PATH`, and current remote CI results could
not be checked because `gh` is not installed.

## Files Changed

- `.github/workflows/ci.yml`
- `README.md`
- `docs/ESP_IDF_BUILD.md`
- `docs/IDF_PORT.md`
- `docs/IDF_PORT_IMPLEMENTATION.md`
- `docs/INA228_ESPIDF_BUILD_PROOF_REPORT.md`
- `docs/INA228_INDUSTRY_HARDENING_FINAL_REPORT.md`
- `tools/check_idf_example_contract.py`

## Inspection Findings

- Root `CMakeLists.txt` registers only `src/INA228.cpp` and `include/` for the
  INA228 component.
- `idf_component.yml` declares ESP-IDF `>=6.0.0` and targets `esp32s2` and
  `esp32s3`.
- `examples/esp_idf/basic` uses native ESP-IDF APIs: `app_main`,
  `driver/i2c_master.h`, `esp_timer`, FreeRTOS delays, stdio input, and fixed
  command buffers.
- The ESP-IDF example does not include Arduino example sources, `Arduino.h`,
  `Wire.h`, `String`, `Serial`, or compatibility facades.
- The ESP-IDF transport maps probe NACK to `I2C_NACK_ADDR`, timeout to
  `I2C_TIMEOUT`, invalid argument to `INVALID_PARAM`, and other bus failures to
  bus/generic I2C statuses with detail codes.
- The ESP-IDF transport and docs label the example as diagnostic single-owner
  glue, not a production shared-bus or multitask manager.

## CI Coverage Status

- Native PlatformIO tests: configured in `.github/workflows/ci.yml`.
- Arduino ESP32-S3 build: configured as `esp32s3dev`.
- Arduino ESP32-S2 build: configured as `esp32s2dev`.
- Core timing guard: configured in `native-tests`.
- CLI contract guard: configured in `validate-library`.
- IDF example contract guard: configured in `validate-library`.
- Generated version check: configured in `validate-library`.
- Package validation: configured in `validate-library` via `pio pkg pack`.
- Pure ESP-IDF builds: configured in `esp-idf-basic` matrix for `esp32s3` and
  `esp32s2` using `espressif/esp-idf-ci-action@v1`, ESP-IDF v6.0.1, and
  `path: examples/esp_idf/basic`.
- Manual workflow run: enabled with `workflow_dispatch`.

Remote CI status was not checked locally because `gh --version` failed with
`gh` not recognized as a command. No remote CI logs were reviewed in this pass.

## Local Commands Run

| Command | Result |
| --- | --- |
| `git status --short` | PASS at startup: clean worktree |
| `git branch --show-current` | `hardening/ina228-industry-readiness` |
| `gh --version` | FAIL: `gh` is not recognized as a command |
| `git diff --check` | PASS; Git reported CRLF normalization warnings |
| `python tools/check_idf_example_contract.py` | PASS |
| `python tools/check_core_timing_guard.py` | PASS |
| `python scripts/generate_version.py check` | PASS; `Version.h` up to date |
| `python -m platformio test -e native` | PASS: 114/114 tests; PlatformIO reported an obsolete-core warning |
| `idf.py --version` | FAIL: `idf.py` is not recognized as a command |

## Commands Not Run

| Command | Reason |
| --- | --- |
| `idf.py -C examples/esp_idf/basic set-target esp32s3 build` | Not run because `idf.py --version` failed locally. |
| `idf.py -C examples/esp_idf/basic set-target esp32s2 build` | Not run because `idf.py --version` failed locally. |

## Proof Status

- Local pure ESP-IDF build proof: not proven.
- Remote CI pure ESP-IDF build proof: configured only; current logs not
  reviewed in this shell.
- Static ESP-IDF contract proof: passed locally.
- Hardware validation: not run and not claimed.

## Remaining Limitations

- Install and activate ESP-IDF v6.0.x, preferably v6.0.1, then run the commands
  in `docs/ESP_IDF_BUILD.md`.
- Review GitHub Actions logs for both `esp32s3` and `esp32s2` before claiming
  remote ESP-IDF build verification.
- No hardware validation was performed; all hardware validation claims remain
  pending dated, commit-linked logs.

## Explicit CI Command Update

Date: 2026-06-07
Starting commit: `25b580478812a742c2656081185947ecb5c4ecf6`

This follow-up pass made the ESP-IDF CI command explicit without changing
driver or device semantics. The `esp-idf-basic` workflow job still uses
`espressif/esp-idf-ci-action@v1` with ESP-IDF `v6.0.1`, `target:
${{ matrix.target }}`, and `path: examples/esp_idf/basic`, but now also passes:

```yaml
command: idf.py set-target ${{ matrix.target }} build
```

That command runs inside the example project directory selected by the action
`path`. The equivalent repository-root commands remain:

```bash
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

Files changed in this follow-up:

- `.github/workflows/ci.yml`
- `docs/ESP_IDF_BUILD.md`
- `docs/IDF_PORT.md`
- `docs/INA228_ESPIDF_BUILD_PROOF_REPORT.md`
- `tools/check_idf_example_contract.py`

Inspection findings:

- Root `CMakeLists.txt` still registers only `src/INA228.cpp` and `include/`
  for the framework-neutral core component.
- `examples/esp_idf/basic` still owns all ESP-IDF bus setup, timing, console
  input, and transport mapping.
- The ESP-IDF example uses native `app_main`, `driver/i2c_master.h`,
  `esp_timer`, FreeRTOS delay APIs, stdio input, and fixed command buffers.
- No Arduino headers, `Wire`, `String`, `Serial`, `TwoWire`, or compatibility
  facades are used in the ESP-IDF example path.
- The ESP-IDF adapter maps timeout, address-probe NACK, invalid parameters,
  transfer NACK-with-unknown-phase, and bus/generic failures to distinct
  `INA228::Status` results while preserving `esp_err_t` detail values.
- The ESP-IDF transport remains documented as diagnostic single-owner example
  glue, not production shared-bus management.

Local commands run:

| Command | Result |
| --- | --- |
| `git status --short` | PASS at startup: clean worktree. |
| `git branch --show-current` | `hardening/ina228-industry-readiness` |
| `python tools/check_idf_example_contract.py` | PASS. |
| `python tools/check_core_timing_guard.py` | PASS. |
| `python -m platformio test -e native` | PASS: 114/114 tests. PlatformIO warned that obsolete Core `6.1.18` is active. |
| `gh --version` | FAIL: `gh` is not recognized as a command. |
| GitHub Actions API for branch | PASS request; `total_count=0`, no current branch CI run found. |
| `git ls-remote origin refs/heads/hardening/ina228-industry-readiness refs/heads/main` | PASS; origin hardening branch at `25b580478812a742c2656081185947ecb5c4ecf6`, main at `27fb6978b8fecca40b267d2236fe87a4651843c0`. |
| `idf.py --version` | FAIL: `idf.py` is not recognized as a command. |

Commands not run:

| Command | Reason |
| --- | --- |
| `idf.py -C examples/esp_idf/basic set-target esp32s3 build` | Not run because `idf.py --version` failed locally. |
| `idf.py -C examples/esp_idf/basic set-target esp32s2 build` | Not run because `idf.py --version` failed locally. |

Proof status after this update:

- Local pure ESP-IDF build proof: not proven.
- CI pure ESP-IDF proof: configured with an explicit `idf.py set-target ...
  build` command for ESP32-S3 and ESP32-S2, pending remote CI logs.
- Remote CI pure ESP-IDF build proof: not proven in this shell; no current
  branch run was returned by the public Actions API.
- Hardware validation: not run and not claimed.

## Revalidation Update

Date: 2026-06-08
Starting commit: `3c69b10afc02d4679bff2d4f5edb0835843a84ce`

This follow-up pass rechecked the pure ESP-IDF build-proof path and found no
additional source or CI changes required. The existing GitHub Actions job
already checks out the repository, uses `espressif/esp-idf-ci-action@v1`, uses
ESP-IDF `v6.0.1`, builds matrix targets `esp32s3` and `esp32s2`, runs the
explicit command `idf.py set-target ${{ matrix.target }} build` under
`examples/esp_idf/basic`, and does not require hardware.

CI file changes in this pass:

- None. The existing workflow already contains the explicit ESP-IDF build
  command and target matrix.

Inspection findings:

- Root `CMakeLists.txt` still registers only `src/INA228.cpp` and `include/`
  as the framework-neutral INA228 component.
- `examples/esp_idf/basic/main/CMakeLists.txt` builds only `main.cpp` and
  `Ina228IdfI2cTransport.cpp` and requires `INA228`, `esp_driver_i2c`,
  `esp_driver_gpio`, `esp_timer`, and `freertos`.
- The ESP-IDF example path uses native `app_main`, `driver/i2c_master.h`,
  `esp_timer`, FreeRTOS delay APIs, stdio input, and fixed command buffers.
- The ESP-IDF example path does not use Arduino headers, `Wire`, `String`,
  `Serial`, `TwoWire`, or compatibility facades.
- The ESP-IDF adapter maps timeout, address-probe NACK, invalid parameters,
  transfer NACK-with-unknown-phase, and bus/generic failures to distinct
  `INA228::Status` values while preserving `esp_err_t` details.
- The ESP-IDF transport remains documented as diagnostic single-owner example
  glue, not production shared-bus management.

Local commands run:

| Command | Result |
| --- | --- |
| `git status --short` | PASS at startup: clean worktree. |
| `git branch --show-current` | `hardening/ina228-industry-readiness` |
| `python tools/check_idf_example_contract.py` | PASS. |
| `python tools/check_core_timing_guard.py` | PASS. |
| `python -m platformio test -e native` | PASS: 114/114 tests. PlatformIO warned that obsolete Core `6.1.18` is active. |
| `gh --version` | FAIL: `gh` is not recognized as a command. |
| GitHub Actions API for branch | PASS request; `total_count=0`, no current branch CI run found. |
| `git ls-remote origin refs/heads/hardening/ina228-industry-readiness refs/heads/main` | PASS; origin hardening branch at `3c69b10afc02d4679bff2d4f5edb0835843a84ce`, main at `27fb6978b8fecca40b267d2236fe87a4651843c0`. |
| `idf.py --version` | FAIL: `idf.py` is not recognized as a command. |

Commands not run:

| Command | Reason |
| --- | --- |
| `idf.py -C examples/esp_idf/basic set-target esp32s3 build` | Not run because `idf.py --version` failed locally. |
| `idf.py -C examples/esp_idf/basic set-target esp32s2 build` | Not run because `idf.py --version` failed locally. |

Proof status after revalidation:

- Local pure ESP-IDF build proof: not proven.
- CI pure ESP-IDF proof: configured with an explicit `idf.py set-target ...
  build` command for ESP32-S3 and ESP32-S2.
- Remote CI pure ESP-IDF build proof: not proven in this shell; no current
  branch run was returned by the public Actions API.
- Hardware validation: not run and not claimed.
