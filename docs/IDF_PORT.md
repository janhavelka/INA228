# INA228 ESP-IDF v6.0.1 Port Audit

Last audited: 2026-05-19

This started as a readiness audit and now records the ESP-IDF implementation on
branch `idf-port`. The driver core is framework-neutral, and the native ESP-IDF
example exposes the same user-visible CLI as the Arduino example. See
`docs/IDF_PORT_IMPLEMENTATION.md` for the implemented file-level summary and
validation notes.

Official ESP-IDF references:
- I2C master driver: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html
- ESP-IDF v6.0 peripheral migration guide: https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/migration-guides/release-6.x/6.0/peripherals.html

## Current Framework/Library State

- `library.json` version is `1.2.0`; the package declares `arduino` and
  `espidf` framework support on `espressif32`.
- `platformio.ini` builds Arduino examples for ESP32-S3 and ESP32-S2, includes
  `lib_deps = Wire`, and has a native Unity test environment.
- Public API is under `include/INA228/` and is already callback-based at the
  I2C boundary.
- `include/INA228/Config.h` exposes `I2cWriteFn`, `I2cWriteReadFn`, and
  optional `NowMsFn`; there is no GPIO or cooperative-yield hook in this driver.
- `include/INA228/INA228.h` exposes `Status begin(const Config&)`,
  `void tick(uint32_t nowMs)`, `void end()`, measurement reads, triggered
  conversion tracking, alert configuration, accumulator reset, raw register
  access, and four-state health tracking.
- `src/INA228.cpp` routes all I2C through `_i2cWriteReadRaw`, `_i2cWriteRaw`,
  `_i2cWriteReadTracked`, and `_i2cWriteTracked`; health is updated from
  tracked wrappers.
- The library core no longer includes `<Arduino.h>` and `_nowMs()` no longer
  calls `millis()`; applications should provide `Config::nowMs` for meaningful
  health timestamps and triggered-conversion timing.
- Arduino-only glue lives in `examples/common/I2cTransport.h`,
  `I2cScanner.h`, `BoardConfig.h`, and the CLI example; those files select
  ESP-IDF-native shims when `INA228_EXAMPLE_PLATFORM_IDF` is defined.

Readiness verdict: the driver core is framework-neutral and the ESP-IDF example
uses the new I2C master driver plus the full bring-up CLI. Final readiness still
requires an ESP-IDF 6.0.1 build and hardware validation.

## Portability Blockers

- ESP-IDF compilation has not been verified in this shell because `idf.py` was
  unavailable.
- Hardware validation remains outstanding.
- `examples/01_basic_bringup_cli/main.cpp` is shared by Arduino and ESP-IDF.
  Arduino builds use `Serial`, `String`, `Wire`, `millis()`, and `delay()`;
  the ESP-IDF example uses example-local console/String/timing and I2C shims.
- `platformio.ini` remains Arduino-focused; `library.json` now declares ESP-IDF
  support.
- IDF v6.0.1 warning profiles can expose issues around floating-point
  conversions, `std::round`, signed/unsigned comparisons, and unused locals.

## Exact Files/APIs Changed

- `src/INA228.cpp`
  - Removed the unconditional `#include <Arduino.h>`.
  - Keep `_i2cWriteReadRaw()`, `_i2cWriteRaw()`,
    `_i2cWriteReadTracked()`, and `_i2cWriteTracked()` as the only transport
    path.
  - `_nowMs()` no longer calls a framework fallback; applications provide
    `Config::nowMs` when timestamps or triggered conversion timing matter.
  - Do not add direct `i2c_master_*` calls to measurement or register helpers.
- `include/INA228/Config.h`
  - Preserve `I2cWriteFn` and `I2cWriteReadFn`; their ABI is already suitable
    for ESP-IDF.
  - Document that pure IDF users should set `nowMs`.
  - Do not include `driver/i2c_master.h` in this public core header.
- `include/INA228/INA228.h`
  - Preserve namespace, class name, enums, `Status`, raw register APIs, and
    health state API.
- Added a root `CMakeLists.txt`.
- Added an IDF example and adapter under `examples/esp_idf/basic/`.
- Added `examples/common/IdfArduinoCompat.h` and guarded example common files
  so the Arduino CLI source can be reused by the ESP-IDF entry point.

## Proposed Architecture Preserving Arduino Compatibility

- Keep the INA228 core callback-based and framework-neutral.
- Keep `examples/common/I2cTransport.h` as the Arduino `Wire` adapter in
  Arduino builds and as a selector for the ESP-IDF adapter when
  `INA228_EXAMPLE_PLATFORM_IDF` is defined.
- Keep the IDF adapter outside the driver core. It owns the IDF I2C bus/device
  handles, supports address-window probes for the CLI, and supplies callbacks
  to `INA228::Config`.
- Keep bus setup, pins, pull-ups, clock speed, and bus lifetime in the
  application/example, not in the INA228 class.
- Preserve existing health semantics:
  - `probe()` uses raw I2C and does not update health;
  - register helpers use tracked wrappers;
  - validation errors do not count as transport failures;
  - `recover()` uses tracked operations.
- Share the CLI implementation through small example-local shims so command
  behavior cannot drift between frameworks.

## IDF Transport Adapter Contract

The adapter should use the ESP-IDF v6.0.1 new I2C master driver only:

```cpp
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_timer.h"

struct Ina228IdfI2c {
  i2c_master_bus_handle_t bus = nullptr;
  i2c_master_dev_handle_t dev = nullptr;
  uint8_t address = 0x40;
};
```

Callback behavior:
- `i2cWrite(addr, data, len, timeoutMs, user)` calls
  `i2c_master_transmit(dev, data, len, timeoutMs)`.
- `i2cWriteRead(addr, txData, txLen, rxData, rxLen, timeoutMs, user)` calls
  `i2c_master_transmit_receive(dev, txData, txLen, rxData, rxLen, timeoutMs)`.
- The callbacks must be synchronous from the driver point of view. Do not
  register `i2c_master_register_event_callbacks()` on this handle unless the
  adapter waits for completion before returning; tracked wrappers update health
  immediately after the callback returns.
- Reject addresses other than the configured device address. INA228 valid
  addresses are `0x40` through `0x4F`.
- Map `ESP_OK` to `INA228::Status::Ok()`.
- Map `ESP_ERR_TIMEOUT` to `INA228::Err::I2C_TIMEOUT`.
- Map `ESP_ERR_INVALID_RESPONSE` to an I2C NACK-related status. The simple
  ESP-IDF master APIs do not distinguish address and data phase, so prefer
  `INA228::Err::I2C_ERROR` with `Status.detail = ESP_ERR_INVALID_RESPONSE`
  unless a custom adapter can prove the phase.
- Map invalid adapter state to `INA228::Err::I2C_BUS` or
  `INA228::Err::INVALID_CONFIG`, depending on whether setup or runtime detected
  it.
- Preserve raw `esp_err_t` in `Status::detail`.
- Clamp or reject `timeoutMs` before passing it to ESP-IDF's signed
  `xfer_timeout_ms`; never allow overflow to become `-1` because `-1` waits
  forever.
- `nowMs(user)` should return `esp_timer_get_time() / 1000`.

## Component/CMake Layout

Recommended component layout:

```text
INA228/
  CMakeLists.txt
  include/INA228/*.h
  src/INA228.cpp
  examples/esp_idf/basic/
    CMakeLists.txt
    main/CMakeLists.txt
    main/main.cpp
    main/Ina228IdfI2cTransport.cpp
```

Core-only component:

```cmake
idf_component_register(
  SRCS "src/INA228.cpp"
  INCLUDE_DIRS "include"
)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
```

If an IDF adapter is shipped in the component, include its source and add
`PRIV_REQUIRES esp_driver_i2c esp_driver_gpio esp_timer esp_rom freertos log vfs`.
The adapter currently lives only in the example, so those dependencies are
declared by the example component.

## Example Plan

- Keep the existing Arduino CLI example building with PlatformIO for ESP32-S2
  and ESP32-S3.
- `examples/esp_idf/basic/main/main.cpp` defines `INA228_EXAMPLE_PLATFORM_IDF`,
  includes the example-local compatibility layer, and then includes
  `examples/01_basic_bringup_cli/main.cpp`.
- The ESP-IDF CLI exposes the same help grouping, color output, address scan,
  measurements, triggered conversion, conversion timing, averaging, ADC range,
  calibration, temp compensation, alert limits, raw registers, health,
  probe/recover, stress, and self-test flows as the Arduino CLI.

## Test/Validation Plan

- Static checks:
  - `python tools/check_cli_contract.py`
  - `python tools/check_idf_example_contract.py`
  - `python tools/check_core_timing_guard.py`
  - `rg "<Arduino.h>|<Wire.h>|millis\\(|delay\\(" include src` should find no
    unguarded Arduino dependencies in the ESP-IDF build path.
  - `rg "driver/i2c.h|i2c_cmd_link|i2c_driver_install" include src examples CMakeLists.txt`
    should not find legacy I2C driver usage in IDF code.
- Arduino regression:
  - `pio test -e native`
  - `pio run -e esp32s3dev`
  - `pio run -e esp32s2dev`
- IDF build:
  - `idf.py set-target esp32s3 build` from `examples/esp_idf/basic`
  - `idf.py set-target esp32s2 build` from `examples/esp_idf/basic`
- Hardware validation:
  - `begin()` verifies manufacturer ID `0x5449`, device ID `0x2281`, and
    MEMSTAT.
  - Read shunt voltage, bus voltage, temperature, current, power, energy, and
    charge with expected units and sign handling.
  - Validate `setCalibration()` and `currentLsb()` across both ADCRANGE modes.
  - Validate alert thresholds and DIAG_ALRT clear behavior.
  - Inject NACK, timeout, and bus errors and verify `lastError()`,
    `consecutiveFailures()`, OFFLINE latch, and `recover()`.

## ESP-IDF v6.0.1 Migration Hazards

- Do not use legacy `<driver/i2c.h>` or command-link APIs. New code must use
  `<driver/i2c_master.h>` and declare `esp_driver_i2c`.
- `ESP_ERR_INVALID_RESPONSE` is the new-driver NACK indication; map it
  consistently and keep the numeric detail.
- ESP-IDF components must declare split driver dependencies explicitly.
- Arduino `Wire` timeouts do not exist in IDF; honor the `timeoutMs` callback
  argument through the I2C master API after clamping it to a finite signed
  millisecond value.
- IDF v6 warning-as-error profiles can fail on implicit float/double and
  integer conversions in calibration and threshold code. Fix warnings in the
  component before enabling CI gating.
- Keep bus ownership outside the driver. The IDF application configures SDA,
  SCL, clock speed, pull-ups, and bus lifetime.

## Ordered Implementation Checklist

1. Done: add the root `CMakeLists.txt` for the core component.
2. Done: remove the Arduino include and `_nowMs()` fallback in
   `src/INA228.cpp`.
3. Done: add the IDF I2C adapter using `<driver/i2c_master.h>`.
4. Done: add `examples/esp_idf/basic` with the shared full CLI.
5. Done: add static Arduino/IDF CLI contract checks.
6. Pending local ESP-IDF toolchain: build `examples/esp_idf/basic` for ESP32-S3.
7. Pending local ESP-IDF toolchain: build `examples/esp_idf/basic` for ESP32-S2.
8. Done: run PlatformIO native and Arduino example builds as regression checks.
9. Validate identity, MEMSTAT, measurements, calibration, and alert APIs on
   hardware.
10. Inject I2C failures and verify health/recovery behavior.
11. Add final `espidf` metadata/build matrix coverage and keep generated
    `Version.h` synchronized with `library.json`.
