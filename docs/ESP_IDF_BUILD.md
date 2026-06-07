# INA228 ESP-IDF Build Guide

This guide documents the reproducible pure ESP-IDF build path for the native
INA228 example. It is a build check only, not hardware validation.

## Scope

- Root component: `CMakeLists.txt` registers only `src/INA228.cpp` and
  `include/` as the INA228 component.
- Example: `examples/esp_idf/basic` owns ESP-IDF bus setup, timing, console
  input, and transport mapping.
- The example is diagnostic single-owner glue. Production shared-bus or
  multitask applications need their own bus manager, locking, stable device
  handles, timeout policy, and recovery policy.
- The core library remains framework-neutral and does not include ESP-IDF,
  Arduino, FreeRTOS, or board-specific headers.

## Toolchain

CI uses ESP-IDF v6.0.1 through `espressif/esp-idf-ci-action@v1`. Local builds
should use ESP-IDF v6.0.x, with ESP-IDF v6.0.1 preferred to match CI. The
component metadata in `idf_component.yml` declares `idf: ">=6.0.0"` and targets
`esp32s2` and `esp32s3`.

Before running local builds, install and activate ESP-IDF so `idf.py` is on
`PATH`.

```bash
idf.py --version
```

If this command is not found, the local machine cannot prove the pure ESP-IDF
build. The static contract check still helps catch accidental Arduino/facade
leakage, but it is not a substitute for a real `idf.py` build.

## Local Build Commands

Run these from the repository root:

```bash
python tools/check_idf_example_contract.py
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

The ESP-IDF build output is written under:

```text
examples/esp_idf/basic/build/
```

Do not commit generated ESP-IDF build artifacts.

## CI Build

GitHub Actions job `esp-idf-basic` builds the same example for both targets
without hardware. The workflow sets `path: examples/esp_idf/basic` and runs
this explicit action command inside that project directory:

```bash
idf.py set-target ${{ matrix.target }} build
```

The equivalent repository-root commands are:

```bash
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

The workflow uses `espressif/esp-idf-ci-action@v1`, ESP-IDF v6.0.1, target
matrix entries `esp32s3` and `esp32s2`, `path: examples/esp_idf/basic`, and
`command: idf.py set-target ${{ matrix.target }} build`. The workflow can run
on pull requests to `main`, pushes to `main`, or manual `workflow_dispatch`.

## Claim Policy

- "CI is configured to build the pure ESP-IDF example" is valid when the
  workflow file contains the ESP-IDF matrix.
- "ESP-IDF build verified" requires actual local `idf.py` output or reviewed
  remote CI logs for both `esp32s3` and `esp32s2`.
- Passing `tools/check_idf_example_contract.py` is a static contract check only.
- None of these build checks are hardware validation.
