# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-07-04

### Added
- Initial release of INA228 driver library.
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
