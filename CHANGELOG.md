# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-06-26

This major release deletes copy/move operations for `INA228::INA228`, adds
public statuses/snapshots, and changes several measurement APIs to return
explicit semantic errors instead of silently returning stale or invalid
converted values.

### Added
- Hardware validation matrix template documenting required board, shunt,
  equipment, framework, command, evidence-path, and pass/fail fields. All
  initial hardware rows are marked `NOT RUN` until dated logs are checked in.
- Expanded high-voltage safety, shunt dissipation, Kelvin layout, measurement
  validity, and validation-honesty documentation.
- Release checklist and HIL command-sequence template to keep hardware
  validation repeatable and release claims blocked until logs exist.
- `getDiagAlertSnapshot()` and `pollConversionReady(nowMs, ready)` public
  helpers for explicit diagnostic status and caller-timestamped readiness
  polling.
- Explicit status coverage for accumulation invalidity, accumulation overflow,
  dirty hardware/cache state, and INA228 math overflow.
- Native fake-bus coverage increased to 138 tests, including datasheet-style
  calibration/scaling vectors, destructive diagnostic reads, scripted
  transaction failures, reset/recovery, output atomicity, and copy/move
  prevention.
- Pure ESP-IDF build guide and CI matrix documentation for reproducible
  `idf.py` ESP32-S2/ESP32-S3 checks.
- `Err::I2C_NACK_UNKNOWN_PHASE` and `errName(Err)` for precise status names
  and ESP-IDF transfers whose NACK phase is not reported by the platform.
- Example-only framed HIL command support (`hilrun`) and transfer counters
  (`xfer_reset`, `xfer_stats`, `xfer_assert`) for serial framing and
  max-instruction transfer-budget evidence.
- Compatible `startConfigReplayJob()` / `pollConfigReplayJob()` aliases for
  the fixed-step cached config/calibration replay job.

### Changed
- `DIAG_ALRT` handling now separates cached alert configuration from
  destructive live status reads, preserves evidence observed by internal
  conversion polling, and documents status-clearing public reads.
- Triggered conversion APIs now distinguish latest register contents from fresh
  triggered measurements; driver-tracked triggered reads return
  `MEASUREMENT_NOT_READY` until completion is observed.
- ENERGY and CHARGE reads are guarded by calibration, mode, accumulation
  readiness, accumulator reset, and overflow state instead of reporting
  datasheet-invalid values as valid.
- `MATHOF` now blocks scalar current/power and aggregate converted measurement
  reads.
- Calibration, `ADCRANGE`, `SHUNT_CAL`, current LSB, shunt resistance, maximum
  expected current, and threshold scaling are treated as one coherent dirty
  state contract.
- Reset, `RSTACC`, and `recover()` behavior is bounded and replays cached state
  with explicit dirty-state reporting after partial failures.
- `begin()` and `probe()` now preserve timeout, data NACK, bus, and generic I2C
  transport errors instead of collapsing them to `DEVICE_NOT_FOUND`; definite
  address NACK still maps to device-not-found for presence checks.
- Public API documentation now states the non-thread-safe/non-ISR-safe contract,
  callback re-entry restrictions, output-parameter commit rules, raw-register
  diagnostic risks, and calibration requirements for converted channels.
- Public docs now describe the library with conservative validation claims,
  compact partial low-voltage Arduino ESP32-S3 HIL evidence, and no
  release-grade hardware-validation claim.
- PlatformIO Arduino ESP32-S2/S3 builds are pinned to
  `platformio/espressif32@7.0.1` for more reproducible local and CI package
  resolution.
- Hardware validation matrix now uses per-row evidence columns for setup,
  procedure, expected/actual result, commit, date/time, equipment, log path,
  and operator notes. All hardware rows remain `NOT RUN`.

### Removed
- Deleted implicit copy and move operations for `INA228::INA228`; keep driver
  instances stable and pass them by pointer or reference.
- Removed internal process artifacts and generated raw extraction archives from
  the public docs tree; retained release-relevant status in README,
  `docs/validation/`, `docs/integration/`, compact reference notes, and source
  PDFs.

### Validation
- Local native tests pass with 138/138 tests on this release checkout.
- Local PlatformIO Arduino builds pass for `esp32s2dev` and `esp32s3dev`.
- GitHub Actions is configured for pure ESP-IDF ESP32-S2/S3 builds, but current
  CI results must be reviewed before claiming ESP-IDF build verification.
- Compact HIL evidence summarizes dirty-worktree Arduino ESP32-S3/COM21 runs:
  a targeted feature sweep with 196 PASS, 0 FAIL, 0 UNKNOWN; a transfer suite
  with 53 PASS, 0 FAIL, 0 UNKNOWN; an 8-hour low-voltage run with 256931 PASS,
  0 FAIL, 48 UNKNOWN; and an incomplete 20-hour attempt with 128416 PASS,
  0 FAIL, 1 UNKNOWN. These remain partial low-voltage evidence only;
  release-grade hardware validation, field validation, high-voltage validation,
  clean-commit soak evidence, and hardware-safety validation are not claimed.

## [1.3.0] - 2026-05-20

### Added
- ESP-IDF component metadata, root `CMakeLists.txt`, and a native
  `examples/esp_idf/basic` application using the ESP-IDF new I2C master driver
  with the same user-visible CLI coverage as the Arduino example.
- `tools/check_idf_example_contract.py` to guard ESP-IDF example structure,
  native-driver dependencies, and CLI parity.
- IDF port implementation notes documenting the framework-neutral core boundary
  and validation status.

### Changed
- Removed the Arduino `millis()` fallback from the driver core. Applications
  should provide `Config::nowMs` when health timestamps or triggered-conversion
  waits need wall-clock time.
- Declared `espidf` framework support in PlatformIO metadata while keeping the
  driver core framework-neutral.
- The ESP-IDF example now exposes the same commands, help, address scan,
  measurement, configuration, calibration, alert, raw-register, health,
  stress, and self-test workflows as the Arduino CLI without including Arduino
  CLI sources or compatibility facades.
- `examples/common/` is now Arduino example glue only; the IDF example owns its
  native stdio CLI, scan/probe, timing, and transport code.
- Release metadata, README installation instructions, and Doxygen project
  metadata now target `v1.3.0`.

### Fixed
- Corrected validation notes: Arduino and ESP-IDF examples provide matching CLI
  coverage, but no checked-in hardware validation artifacts are present.

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
- Reference documentation now uses human-readable vendor PDF names and compact power-monitor notes under `docs/reference/`.
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

[2.0.0]: https://github.com/janhavelka/INA228/compare/v1.3.0...v2.0.0
[1.3.0]: https://github.com/janhavelka/INA228/compare/v1.2.0...v1.3.0
[1.2.0]: https://github.com/janhavelka/INA228/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/janhavelka/INA228/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/janhavelka/INA228/releases/tag/v1.0.0
