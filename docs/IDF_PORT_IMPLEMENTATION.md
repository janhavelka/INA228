# INA228 ESP-IDF Port Implementation

Implemented after the ESP-IDF port branch was merged into `main`.

## Core Boundary

- `include/` and `src/` are framework-neutral and do not include Arduino,
  `Wire`, `Serial`, ESP-IDF I2C, or GPIO headers.
- The driver still receives all I2C access through `Config::i2cWrite` and
  `Config::i2cWriteRead`.
- `Config::nowMs` remains optional. If it is not supplied, health timestamps use
  `0`; Arduino and ESP-IDF applications should supply their own monotonic
  millisecond callback.

## ESP-IDF Additions

- Root `CMakeLists.txt` registers the library as an ESP-IDF component.
- `idf_component.yml` declares component-manager metadata for ESP-IDF 6.x and
  the ESP32-S2/S3 targets.
- `examples/esp_idf/basic` demonstrates application-owned bus/device setup with
  the new `driver/i2c_master.h` API and maps `esp_err_t` values into
  `INA228::Status`.
- The ESP-IDF transport in the example is single-owner diagnostic glue. It is
  not a shared-bus or multitask manager; production projects should add an
  external bus manager, locking, stable handles, and recovery policy.
- The ESP-IDF entry point uses native `app_main`, `esp_timer`, FreeRTOS delays,
  stdio input, and fixed command buffers. It implements the same address scan,
  measurements, calibration, alerts, raw-register diagnostics, health/recovery,
  stress, and self-test workflows as the Arduino example without including
  Arduino CLI sources or compatibility facades.
- The IDF adapter supports the CLI's `scanina` and `init <addr>` behavior by
  probing `0x40..0x4F` on the bus and reselecting the active device handle
  without putting bus ownership into the driver core.
- Added `tools/check_idf_example_contract.py` to guard native IDF dependencies,
  ban Arduino compatibility facades, and verify Arduino/IDF CLI command parity.
- CI is configured to build `examples/esp_idf/basic` with ESP-IDF 6.0.1 for
  ESP32-S3 and ESP32-S2.

## Validation

- Static check target: `rg "<Arduino.h>|<Wire.h>|millis\\(|delay\\(" include src`
  should return no matches.
- Static parity checks:
  - `python tools/check_cli_contract.py`
  - `python tools/check_idf_example_contract.py`
  - `python tools/check_core_timing_guard.py`
- Arduino examples remain under `examples/01_basic_bringup_cli` and continue to
  provide `Wire` and `millis()` through example-local callbacks.
- IDF builds were not run in this environment because `idf.py` was not on PATH.
  GitHub Actions has an ESP-IDF build matrix; do not claim local IDF build
  results unless `idf.py` output is captured.
- Arduino and ESP-IDF examples provide matching bring-up CLI coverage, but no
  checked-in dated hardware validation logs are present for either framework.
  ESP-IDF hardware validation has not been performed in this environment.

## Remaining Hardware Work

- Review the GitHub Actions ESP-IDF build results for ESP32-S3 and ESP32-S2,
  and rerun locally when `idf.py` is available.
- Validate manufacturer ID `0x5449`, device ID `0x2281`, MEMSTAT, measurement
  scaling, alert APIs, and health/recovery behavior on hardware. Record results
  in `docs/INA228_HARDWARE_VALIDATION_MATRIX.md` with logs.
