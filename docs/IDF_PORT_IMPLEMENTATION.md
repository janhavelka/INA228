# INA228 ESP-IDF Port Implementation

Implemented on branch `idf-port`.

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
- `idf_component.yml` declares component-manager metadata for ESP-IDF 6.x.
- `examples/esp_idf/basic` demonstrates application-owned bus/device setup with
  the new `driver/i2c_master.h` API and maps `esp_err_t` values into
  `INA228::Status`.

## Validation

- Static check target: `rg "<Arduino.h>|<Wire.h>|millis\\(|delay\\(" include src`
  should return no matches.
- Arduino examples remain under `examples/01_basic_bringup_cli` and continue to
  provide `Wire` and `millis()` through example-local callbacks.
- IDF builds were not run in this environment because `idf.py` was not on PATH.

## Remaining Hardware Work

- Build `examples/esp_idf/basic` for ESP32-S3 and ESP32-S2 with ESP-IDF 6.0.1.
- Validate manufacturer ID `0x5449`, device ID `0x2281`, MEMSTAT, measurement
  scaling, alert APIs, and health/recovery behavior on hardware.
