# ESP-IDF Integration

The repository root can be used as an ESP-IDF component. The diagnostic example
in `examples/esp_idf/basic` is a native ESP-IDF application, not an Arduino
wrapper.

## Boundary

- Core files in `include/INA228/` and `src/` are framework-neutral.
- The driver receives all I2C access through `Config::i2cWrite` and
  `Config::i2cWriteRead`.
- `Config::nowMs` is optional; when omitted, health timestamps use `0`.
- The ESP-IDF example owns bus setup, device handles, console input, timing,
  timeout mapping, and recovery commands.
- The example is single-owner diagnostic glue. Shared-bus or multitask
  applications need their own bus manager, locking, timeout policy, stable
  handle lifetime, and recovery policy.

Forbidden in the pure ESP-IDF example path: `Arduino.h`, `Wire.h`, `String`,
`Serial`, `TwoWire`, Arduino compatibility facades, and Arduino example sources.

## Component Use

Add this repository as an extra component directory or use the component manager
metadata in `idf_component.yml`. `include/INA228/Version.h` is generated from
`library.json` and committed, so a clean ESP-IDF checkout can include the
library without running PlatformIO first.

## Transport Status Mapping

The example adapter preserves raw `esp_err_t` values in `Status::detail`.

| ESP-IDF result | INA228 status | Notes |
| --- | --- | --- |
| `ESP_OK` | `OK` | Transaction succeeded. |
| `ESP_ERR_TIMEOUT` | `I2C_TIMEOUT` | Timeout remains distinguishable. |
| Probe `ESP_ERR_INVALID_RESPONSE` / `ESP_ERR_NOT_FOUND` | `I2C_NACK_ADDR` | Address phase is known for probe. |
| Transfer `ESP_ERR_INVALID_RESPONSE` | `I2C_NACK_ADDR` only after a confirming probe; otherwise `I2C_ERROR` | Transaction API does not expose address vs data phase. |
| `ESP_ERR_INVALID_ARG` | `INVALID_PARAM` | Adapter/API argument failure. |
| `ESP_ERR_INVALID_STATE` | `I2C_BUS` | Driver or bus state fault. |
| Other `esp_err_t` | `I2C_BUS` | Conservative fallback with raw detail. |

## Local Checks

CI uses ESP-IDF v6.0.1. Local builds should use ESP-IDF v6.0.x, with v6.0.1
preferred when reproducing CI.

Run from the repository root:

```bash
idf.py --version
python tools/check_idf_example_contract.py
python tools/check_core_timing_guard.py
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

ESP-IDF build output is written under:

```text
examples/esp_idf/basic/build/
```

Do not commit generated ESP-IDF build artifacts.

`tools/check_idf_example_contract.py` is a static contract check for the
example structure and Arduino-free boundary. It is useful, but it is not a
substitute for a real `idf.py` build log.

## CI

GitHub Actions job `esp-idf-basic` builds `examples/esp_idf/basic` for
`esp32s3` and `esp32s2` with `espressif/esp-idf-ci-action@v1` and ESP-IDF
v6.0.1. Configured CI is not proof by itself; reviewed workflow logs or local
`idf.py` output are required before claiming ESP-IDF build verification.

These build checks are not hardware validation.
