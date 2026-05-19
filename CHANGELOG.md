# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- ESP-IDF component metadata, root `CMakeLists.txt`, and a basic
  `examples/esp_idf/basic` application using the ESP-IDF new I2C master driver.
- IDF port implementation notes documenting the framework-neutral core boundary
  and validation status.

### Changed
- Removed the Arduino `millis()` fallback from the driver core. Applications
  should provide `Config::nowMs` when health timestamps or triggered-conversion
  waits need wall-clock time.
- Declared `espidf` framework support in PlatformIO metadata while keeping the
  Arduino examples unchanged.

## [1.2.0] - 2026-05-17

### Changed
- Bring-up CLI no-argument alert commands now read back the active chip state:
  `alatch`, `cnvralert`, `alslow`, `apol`, `sovl`, `suvl`, `bovl`, `buvl`,
  `tmplim`, and `pwrlim`.
- README bring-up notes now document shared-bus probing behavior, shunt
  calibration commands, and alert command query/set forms more explicitly.
- Public Doxygen comments now describe alert flag readback, threshold scaling,
  calibration persistence, and accumulator reset behavior more precisely.

### Fixed
- INA228 address-window probing in the bring-up CLI now performs an address ACK
  preflight before register-level ID reads, avoiding noisy ESP-IDF NACK logs for
  known-empty addresses on mixed I2C buses.
- CLI help now shows alert commands as query-or-set commands instead of
  setter-only commands.

## [1.1.0] - 2026-05-14

### Added
- `SettingsSnapshot` and `getSettings(SettingsSnapshot&)` for cache-only config, calibration, trigger, and health inspection.
- Bring-up CLI `settings`, alert-limit `limits`, and no-argument query forms for `convtime`, `averaging`, and `adcrange`.
- Native tests for calibration clamping, setter rollback, triggered conversion readiness, unsigned raw bus/energy values, and threshold validation.
- Native coverage proving latched `OFFLINE` blocks normal I2C operations without touching the bus while `recover()` remains the explicit recovery path.

### Changed
- Triggered conversions now return `IN_PROGRESS` and block measurement reads with `MEASUREMENT_NOT_READY` until conversion time has elapsed and CNVRF is set.
- Explicit recovery/reset bypass internals now use the shared `ScopedOfflineI2cAllowance` / `_reassertOfflineLatch()` procedure so failed recovery attempts that begin from `OFFLINE` keep the latch asserted.
- Reference documentation now uses human-readable vendor PDF names and separates compact power-monitor notes from full PDF/application-note extractions under `docs/extracted-md/` and `docs/pdf-extracted-md/`.
- Calibration now adjusts `currentLsb()` to the actual programmed `SHUNT_CAL` value when register clamping occurs.
- `SHUNT_TEMPCO` is programmed from the configured coefficient even when temperature compensation is disabled; the enable flag controls use of that coefficient.
- Raw bus voltage and raw energy API types now reflect unsigned device registers.
- Failed `begin()` clears stale config/calibration/trigger state, successful startup no longer increments health counters, and `IN_PROGRESS` remains health-neutral.
- Health behavior is now standardized on latched `OFFLINE`: normal public I2C operations return `BUSY` with `Driver is offline; call recover()` and do not touch I2C until `recover()` succeeds.

### Fixed
- Config and calibration setters no longer commit cached values when I2C writes fail.
- `begin()` and threshold APIs now reject invalid enum values, NaN/Inf, and out-of-range values before touching I2C.
- `recover()` now records identity and MEMSTAT failures in health state.
- CLI trigger reporting now treats `IN_PROGRESS` as an accepted operation and prints granular `I2C_*` errors.

## [1.0.0] - 2026-04-05

### Added
- Initial public release of the INA228 driver library.
- Full register map support for all 20 INA228 registers.
- Managed synchronous driver with 4-state health tracking (UNINIT/READY/DEGRADED/OFFLINE).
- Injected I2C transport (no Wire dependency in library code).
- All ADC operating modes: shutdown, triggered (7 variants), continuous (7 variants).
- Configurable conversion times (50 µs to 4120 µs) and averaging (1 to 1024 samples).
- Shunt full-scale range selection (±163.84 mV or ±40.96 mV).
- Automatic calibration (SHUNT_CAL) based on shunt resistance and max expected current.
- Shunt temperature compensation support (SHUNT_TEMPCO register).
- Configurable conversion delay (0–510 ms in 2 ms steps).
- Measurement API: shunt voltage, bus voltage, temperature, current, power, energy, charge.
- Raw sample readout for all channels.
- 6 alert threshold registers (shunt OV/UV, bus OV/UV, temperature, power).
- Full DIAG_ALRT register support with parsed flag struct.
- Device identity verification (manufacturer ID 0x5449, device ID 0x2281).
- NV memory checksum (MEMSTAT) validation during begin().
- Conversion time estimation based on current configuration.
- Software reset and accumulator reset support.
- probe() diagnostic (raw I2C, no health tracking) and recover() with config reapply.
- Example bringup CLI with interactive serial command interface.
- Native Unity tests for lifecycle, health tracking, and transport layer.
- Auto-generated Version.h from library.json.
- Public lifecycle/config introspection helpers: `isInitialized()` and `getConfig()`.
- Public tracked raw-register helpers: `readRegister16()`, `readRegister24()`, `readRegister40()`, and `writeRegister16()`.
- `Err::CONVERSION_NOT_READY` alias for cross-library uniformity.
- Bringup CLI address-selection, low-level register access, calibration, temperature-compensation, conversion-delay, and alert-threshold commands.

### Changed
- `recover()` now re-validates manufacturer ID, device ID, and MEMSTAT before reapplying cached configuration and calibration.
- Bringup `scan` now includes an INA228-specific address probe, and startup can auto-detect a single healthy INA228 on `0x40..0x4F`.

[Unreleased]: https://github.com/janhavelka/INA228/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/janhavelka/INA228/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/janhavelka/INA228/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/janhavelka/INA228/releases/tag/v1.0.0
