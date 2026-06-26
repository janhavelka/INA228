# INA228 Driver Library

Framework-neutral INA228 85-V, 20-bit I2C power/energy/charge monitor driver for ESP32 (Arduino/PlatformIO and ESP-IDF component use). Release-grade hardware validation is not claimed.

Library version: `v2.0.0`.

## Features

- **Injected I2C transport** - no Wire dependency in library code
- **Framework-neutral core** - `include/` and `src/` do not include Arduino or ESP-IDF driver headers
- **Health monitoring** - automatic state tracking (READY/DEGRADED/OFFLINE)
- **Deterministic behavior** - no unbounded loops, no heap allocations
- **Managed synchronous lifecycle** - blocking I2C ops with tick-based architecture
- **INA228 coverage** - ADC modes, guarded triggered conversions, calibration, alerts, energy/charge accumulation

## Installation

### PlatformIO (recommended)

Add to `platformio.ini`:

```ini
lib_deps = 
  https://github.com/janhavelka/INA228.git#v2.0.0
```

### Manual

Copy `include/INA228/` and `src/` to your project.

### ESP-IDF

The repository root is an ESP-IDF component. Add it through `EXTRA_COMPONENT_DIRS`
or the component manager metadata, then provide `Config::i2cWrite`,
`Config::i2cWriteRead`, and `Config::nowMs` from your application-owned I2C
adapter. The native example in `examples/esp_idf/basic` uses ESP-IDF
`driver/i2c_master.h`, `app_main`, `esp_timer`, FreeRTOS delays, and fixed
command buffers while preserving Arduino CLI command coverage. Its transport is
single-owner diagnostic example glue; production shared-bus or multitask
systems should provide an external bus manager, locking, stable device handles,
and application-specific recovery policy.

Reproducible local and CI build commands are documented in
[`docs/integration/esp-idf.md`](docs/integration/esp-idf.md). The
`tools/check_idf_example_contract.py` guard is a static contract check; it is
not a substitute for a real `idf.py` build log.

## Version 2.0.0

- Uses a major SemVer target because this release deletes copy/move
  operations and changes several public failure semantics to fail closed.
- Hardens destructive `DIAG_ALRT` handling, alert configuration side effects,
  triggered freshness, ENERGY/CHARGE validity, `MATHOF` handling, calibration,
  ADCRANGE, reset/RSTACC, recovery, dirty-state, and status precision behavior.
- Documents that current, power, energy, and charge require valid calibration,
  clean hardware/cache state, and datasheet-supported accumulation conditions.
- Includes native fake-bus tests, local PlatformIO Arduino ESP32-S2/S3 build
  coverage, and compact partial low-voltage Arduino ESP32-S3 HIL evidence.
- Configures GitHub Actions pure ESP-IDF `idf.py` builds for ESP32-S2 and
  ESP32-S3. Review CI logs for the exact release commit before claiming a
  dated CI pass.
- Release-grade hardware validation remains unclaimed until clean-commit
  framed HIL evidence, an 8-hour clean framed soak, fault-injection fixture
  coverage, alert-pin capture, controlled reset/power-cycle evidence, and
  reviewed CI logs are available.

## High-Voltage Safety

INA228 can monitor bus voltages up to 85 V, but this library and its examples
do not make high-voltage systems safe. They do not provide isolation, fusing,
creepage/clearance, enclosure safety, shunt power/thermal design, mains safety,
or protection against USB-ground hazards. Use qualified hardware design and
validation before connecting non-isolated, mains-derived, or high-energy rails.

The 85 V number is an IC input capability, not a system safety rating. Never
connect unsafe voltages to development boards or USB-connected PCs without
proper isolation and protection. Account for transients, inductive kickback,
fault current, creepage/clearance, connector ratings, shunt heating, enclosure
access, and the grounding relationship between the measured rail, ESP32 board,
debug probe, USB cable, and host computer. INA228 measurements and ALERT output
are monitoring signals; they are not a fuse, protective relay, or certified
safety function.

## Quick Start

```cpp
#include <Wire.h>
#include "INA228/INA228.h"
#include "common/I2cTransport.h"

INA228::INA228 device;

void setup() {
  Serial.begin(115200);
  transport::initWire(8, 9, 400000, 50);
  
  INA228::Config cfg;
  cfg.i2cWrite = transport::wireWrite;
  cfg.i2cWriteRead = transport::wireWriteRead;
  cfg.i2cUser = &Wire;
  cfg.nowMs = [](void*) { return millis(); };
  cfg.i2cAddress = 0x40;
  cfg.mode = INA228::Mode::CONT_ALL;
  cfg.shuntResistanceOhm = 0.015f;  // 15 milliohm shunt
  cfg.maxExpectedCurrentA = 10.0f;   // 10 A calibration design point
  
  auto status = device.begin(cfg);
  if (!status.ok()) {
    Serial.printf("Init failed: %s\n", status.msg);
    return;
  }
  
  Serial.println("INA228 initialized!");
}

void loop() {
  device.tick(millis());
  
  INA228::Measurement m{};
  auto st = device.readMeasurement(m);
  if (st.ok()) {
    Serial.printf("Vbus=%.3fV  I=%.4fA  P=%.3fW\n",
                  m.busVoltageV, m.currentA, m.powerW);
  }
  
  delay(1000);
}
```

For `0.015 ohm` at `10 A`, the shunt dissipates `I^2 * R = 1.5 W` before
layout and ambient derating. The code cannot make an undersized shunt,
development board, wiring, or enclosure safe.

## Shunt, Layout, And Calibration

- Select the shunt so `Imax * Rshunt` stays inside the selected shunt range:
  `+/-163.84 mV` for `ADCRANGE=0` or `+/-40.96 mV` for `ADCRANGE=1`. The
  narrower range gives four times finer shunt-voltage resolution but less
  overcurrent headroom.
- Check shunt power with `P = I^2 * R`, including pulse rating, thermal
  derating, PCB copper temperature rise, tolerance, and temperature coefficient.
  Calibration improves reported units; it does not protect the hardware from
  overcurrent or overheating.
- Use Kelvin connections to the shunt sense terminals. Keep IN+ and IN- traces
  short, balanced, and outside the load-current path; follow the shunt
  manufacturer's footprint guidance for four-terminal parts.
- Consider input filtering, TVS/Zener/transient protection, fusing, and
  current-limited supplies for inductive or high-energy systems. Keep any
  filter values within datasheet limits so they do not corrupt measurements.
- `Config::shuntResistanceOhm` and `Config::maxExpectedCurrentA` define the
  requested calibration. The driver exposes the actual `currentLsb()`,
  `SHUNT_CAL`, clamp state, dirty state, and threshold-dirty state through
  `SettingsSnapshot`.
- If calibration is omitted, `begin()` writes `SHUNT_CAL=0`. Raw
  voltage/temperature/register reads remain available; converted current,
  power, energy, charge, and power-limit APIs return calibration errors.
- Reapply engineering-unit thresholds after changing `ADCRANGE`, calibration,
  or after `softReset()`. `SettingsSnapshot::thresholdsDirty` is a sticky
  advisory that tells service code previously programmed thresholds may no
  longer match the active scale.

## API Overview

### Lifecycle

| Method | Description |
|--------|-------------|
| `begin(config)` | Initialize with configuration (validates, verifies device ID + MEMSTAT; preserves transport error codes except address NACK maps to device-not-found) |
| `tick(nowMs)` | Process pending operations (call from loop) |
| `end()` | End the driver session and clear local runtime state without I2C; call `setMode(SHUTDOWN)` first when hardware shutdown must be observed |
| `isInitialized()` | True after successful `begin()` until `end()` |
| `getConfig()` | Return the driver's cached configuration snapshot |
| `getSettings(snap)` | Populate a `SettingsSnapshot` with cached config, calibration, conversion, trigger, and health state without I2C |

### Measurements

| Method | Description |
|--------|-------------|
| `readMeasurement(m)` | Read all converted channels with energy/charge validity flags; requires calibration because current/power fields are included |
| `readRawSample(raw)` | Diagnostic raw register read; preserves `DIAG_ALRT` first, then reads raw accumulators with validity/overflow flags |
| `readIntegerSample(sample)` | Read fixed-unit `uV/mV/milliC/mA/mW` values without ENERGY/CHARGE accumulator reads; still a multi-transfer convenience API |
| `convertRawSample(raw, sample)` | Convert staged raw register values to fixed-unit integers without I2C |
| `readBusVoltage(v)` | Bus voltage in volts (0-85 V) |
| `readShuntVoltage(v)` | Shunt voltage in volts |
| `readTemperature(t)` | Die temperature in °C |
| `readCurrent(i)` | Current in amperes (requires calibration) |
| `readPower(p)` | Power in watts (requires calibration) |
| `readEnergy(e)` | Accumulated energy in joules; fails when accumulation is invalid or overflowed |
| `readCharge(q)` | Accumulated charge in coulombs; fails when accumulation is invalid or overflowed |
| `isConversionReady(r)` | Check CNVRF using `Config::nowMs`; clears `r` before polling |
| `pollConversionReady(nowMs, r)` | Check CNVRF using a caller-supplied timestamp; clears `r` before polling |

### Configuration

| Method | Description |
|--------|-------------|
| `setMode(mode)` | Set ADC operating mode |
| `getMode(mode)` | Read the cached operating mode |
| `triggerConversion(mode)` | Start single-shot conversion |
| `setVbusConvTime(ct)` | Bus voltage conversion time |
| `setVshuntConvTime(ct)` | Shunt voltage conversion time |
| `setTempConvTime(ct)` | Temperature conversion time |
| `setAveraging(avg)` | Averaging count (1–1024) |
| `setAdcRange(range)` | Shunt full-scale range (±163.84 or ±40.96 mV) |
| `setCalibration(ohm, A)` | Update shunt calibration for the installed shunt resistor and expected current |
| `setShuntTempCoeff(ppm)` | Shunt temperature coefficient |
| `setTempCompensation(enable)` | Enable or disable shunt temperature compensation |
| `setConversionDelay(steps2ms)` | Set conversion delay in 2 ms steps |
| `softReset()` | Bounded software reset with identity/MEMSTAT/reset-bit verification and cached replay |
| `resetAccumulators()` | Clear energy/charge registers, then explicitly clear/verify `RSTACC` |

`setShuntTempCoeff(ppm)` writes the configured `SHUNT_TEMPCO` coefficient even
when temperature compensation is disabled. `tempCompEnabled` controls whether
the coefficient participates in calibration behavior; the register value remains
explicitly programmed for deterministic readback.

### Health & Diagnostics

| Method | Description |
|--------|-------------|
| `state()` | Current driver state (UNINIT/READY/DEGRADED/OFFLINE) |
| `driverState()` | Compatibility alias for `state()` |
| `isOnline()` | True if READY or DEGRADED |
| `probe()` | Check device presence (no health tracking; preserves transport error codes except address NACK maps to device-not-found) |
| `recover()` | Re-validate manufacturer ID, device ID, MEMSTAT, then re-apply config/calibration |
| `readManufacturerId(id)` | Read manufacturer ID, expected `0x5449` |
| `readDeviceId(id)` | Read device ID, expected `0x2281` |
| `readDiagAlert(diag)` | Read and consume current `DIAG_ALRT` flags |
| `readDiagAlertRaw(raw)` | Read and consume raw `DIAG_ALRT` register value |
| `getDiagAlertSnapshot(snapshot)` | Return preserved `DIAG_ALRT` evidence without I2C |
| `setAlertLatch(latch)` | Configure latched or transparent alert behavior |
| `setConversionReadyAlert(enable)` | Route conversion-ready events to ALERT |
| `setSlowAlert(enable)` | Compare alert thresholds against averaged values |
| `setAlertPolarity(activeHigh)` | Configure ALERT pin polarity |
| `setShuntOvervoltageThreshold(v)` | Set shunt overvoltage threshold |
| `setShuntUndervoltageThreshold(v)` | Set shunt undervoltage threshold |
| `setBusOvervoltageThreshold(v)` | Set bus overvoltage threshold |
| `setBusUndervoltageThreshold(v)` | Set bus undervoltage threshold |
| `setTemperatureOverlimitThreshold(c)` | Set die temperature over-limit threshold |
| `setPowerOverlimitThreshold(w)` | Set power over-limit threshold |

`DIAG_ALRT` is a live, status-sensitive register. Reading it can clear `CNVRF`
and latched alert flags, so internal conversion-ready polling preserves the
observed event evidence in `getDiagAlertSnapshot()`. Alert configuration setters write
cached configuration bits without first reading live `DIAG_ALRT` status.
Accumulator reads first preserve `DIAG_ALRT` when they may touch `ENERGY` or
`CHARGE`, because reading those registers can clear `ENERGYOF` or `CHARGEOF`.

### Raw Register Access

| Method | Description |
|--------|-------------|
| `readRegister16(reg, value)` | Diagnostic tracked 16-bit read; status-sensitive registers can have read side effects |
| `readRegister24(reg, value)` | Diagnostic tracked 24-bit read |
| `readRegister40(reg, value)` | Diagnostic tracked 40-bit read; accumulator reads can affect overflow evidence and do not pre-preserve `DIAG_ALRT` |
| `writeRegister16(reg, value)` | Diagnostic tracked 16-bit write; can desynchronize typed cache from hardware |

### Timing And Scale Introspection

| Method | Description |
|--------|-------------|
| `estimateConversionTimeUs()` | Estimate configured conversion time in microseconds |
| `estimateConversionTimeMs()` | Estimate configured conversion time in milliseconds, rounded up |
| `currentLsb()` | Return cached CURRENT_LSB; check `SettingsSnapshot::calibrated` and `hardwareDirty` before treating it as usable |

## Driver State Machine

```
begin() success --> READY
         |            |
         |       I2C failure
         |            v
         |        DEGRADED (1..N-1 failures)
         |            |
         |       threshold reached
         |            v
         |         OFFLINE
         |
end() --------> UNINIT
```

- Any successful tracked I2C operation in DEGRADED returns to READY.
- OFFLINE is latched. Normal public I2C operations return `BUSY` with `Driver is offline; call recover()` without touching the bus.
- `probe()` uses raw I2C and does NOT affect health counters.
- `recover()` is the explicit path out of OFFLINE; it uses tracked I2C and updates health.

## Behavioral Contracts

1. **Threading model**: Instances are not thread-safe or ISR-safe. Serialize calls externally, protect shared I2C buses outside the driver, and do not let transport/time callbacks re-enter the same `INA228` instance.
2. **Timing model**: `tick()` is bounded; all I2C operations are blocking.
3. **Resource ownership**: I2C bus owned by application; library receives transport callbacks.
4. **Framework boundary**: Core code does not call `Wire`, `Serial`, `delay()`, `yield()`, `millis()`, or ESP-IDF peripheral APIs directly. Arduino examples and native ESP-IDF examples provide those hooks externally.
5. **Memory behavior**: No heap allocation after `begin()`.
6. **Error handling**: All fallible APIs return `Status`. Check with `st.ok()`. `begin()` and `probe()` map only definite address NACK to `DEVICE_NOT_FOUND`; timeout, data NACK, bus, and generic I2C errors remain precise. Unless a method documents otherwise, output parameters are committed only on `Status::Ok()` and remain unchanged on non-OK status. Readiness APIs clear their `ready` output before polling, including on error.
7. **Recovery model**: `OFFLINE` is latched. Supervisors should call `recover()` after applying any bus-level recovery policy.
8. **Measurement freshness**: Continuous-mode reads return the latest hardware register contents at read time; they are not guaranteed fresh since the previous API call. Driver-tracked triggered conversions return `MEASUREMENT_NOT_READY` until the software deadline has elapsed and CNVRF is observed. `tick(nowMs)` and `pollConversionReady(nowMs, ready)` use the supplied timestamp, so they can advance pending triggered conversions even when `Config::nowMs` is unset. `tick()` only polls `DIAG_ALRT` when it can advance driver-owned triggered or accumulator-readiness state; use `pollConversionReady()` for an explicit destructive readiness poll. If `Config::nowMs` is unset, call `tick(nowMs)` or `pollConversionReady(nowMs, ready)` before triggered measurement reads.
9. **Accumulator validity and side effects**: ENERGY requires continuous shunt-and-bus conversion, CHARGE requires continuous shunt conversion, and both require calibration plus a continuous CNVRF observed after begin/reset/accumulator reset. Scalar `readCurrent()`, `readPower()`, `readEnergy()`, `readCharge()`, aggregate `readMeasurement()`, and fixed-unit `readIntegerSample()` return `MATH_OVERFLOW` instead of reporting current-derived values when `DIAG_ALRT.MATHOF` is observed. Scalar `readEnergy()` and `readCharge()` also return `ACCUMULATION_INVALID` or `ACCUMULATION_OVERFLOW` instead of returning invalid data. `readMeasurement()` requires calibration because it includes current and power. `readRawSample()` is diagnostic/raw: it preserves `DIAG_ALRT` first, then reads raw accumulators; raw current/power/energy/charge fields may be invalid and accumulator reads can consume overflow evidence. `readIntegerSample()` reads only the instant voltage/temperature/current/power registers and does not touch `ENERGY` or `CHARGE`; it is still a multi-transfer convenience API. `convertRawSample()` performs no I2C and is suitable after staged raw reads.
10. **Calibration coherence**: `SHUNT_CAL`, `currentLsb()`, `shuntResistanceOhm`, `maxExpectedCurrentA`, and `adcRange` are treated as one scaling contract. Converted current, power, energy, charge, and power-limit APIs require a valid calibration and a clean hardware/cache state. If a multi-register range/calibration/reset update cannot be rolled back or fully replayed after an I2C failure, those APIs return `HARDWARE_DIRTY` until `recover()` or `softReset()` successfully replays the cached configuration. `SettingsSnapshot::hardwareDirtyCause` preserves the first status that made the state dirty.
11. **Reset policy**: `softReset()` contains no platform delay or unbounded wait. It writes `CONFIG.RST`, performs a finite reset-bit readback check, verifies manufacturer ID, device ID, and `MEMSTAT`, then replays cached config/calibration with ADC shutdown first and final ADC mode last. If verification or replay fails, reset-domain dirty state remains visible. `resetAccumulators()` treats `RSTACC` self-clear as unproven: it writes `RSTACC`, writes cached `CONFIG` again with reset bits clear, verifies readback, and invalidates accumulation until the next continuous CNVRF.
12. **Object lifetime**: `INA228::INA228` is default-constructible but not copyable or movable. Keep each instance at a stable address and pass it by pointer or reference.

## INA228 Address Configuration

| A1 | A0 | Address |
|----|----|---------|
| GND | GND | 0x40 |
| GND | VS  | 0x41 |
| GND | SDA | 0x42 |
| GND | SCL | 0x43 |
| VS  | GND | 0x44 |
| VS  | VS  | 0x45 |
| VS  | SDA | 0x46 |
| VS  | SCL | 0x47 |
| SDA | GND | 0x48 |
| SDA | VS  | 0x49 |
| SDA | SDA | 0x4A |
| SDA | SCL | 0x4B |
| SCL | GND | 0x4C |
| SCL | VS  | 0x4D |
| SCL | SDA | 0x4E |
| SCL | SCL | 0x4F |

## Bringup CLI Notes

The canonical bringup example now includes address-aware diagnostics for shared buses:

- `scan` prints the general I2C table and probes `0x40..0x4F` for valid INA228 identity/MEMSTAT
- `scanina` probes only the INA228 address window
- INA228-specific probes skip register reads on addresses that do not ACK, so
  mixed buses do not produce noisy expected-NACK logs for empty `0x40..0x4F`
  slots.
- `addr [0x40..0x4F]` selects the target device address used by the example
- `init [addr]` re-initializes at the selected address; startup can auto-detect a single healthy INA228 if the default address fails
- `cal <shunt_ohm> <max_current_a>` updates calibration for the installed shunt
  resistor; `cal` without arguments prints the active `Rshunt`,
  `MaxCurrent`, and `CURRENT_LSB`.
- `tempco`, `tempcomp`, `delay`, alert-threshold commands, and `reg16` /
  `reg24` / `reg40` / `wreg16` expose service-level diagnostics
- `settings` prints the cached `SettingsSnapshot`; `limits` reads alert limit
  registers with decoded units.
- `convtime`, `averaging`, `adcrange`, `alatch`, `cnvralert`, `alslow`,
  `apol`, `sovl`, `suvl`, `bovl`, `buvl`, `tmplim`, and `pwrlim` can be run
  without arguments to query the active configuration/registers, or with an
  argument to update them.
- `examples/esp_idf/basic` exposes the same serial command surface through a
  native ESP-IDF console/timing/I2C shim, including address-window probing,
  dynamic `init <addr>`, calibration, alert limits, raw register diagnostics,
  stress tests, and self-test output.

Raw register access is intended for diagnostics and bring-up. Reads of status-sensitive registers such as `DIAG_ALRT` can consume diagnostic evidence, and reads of `ENERGY` or `CHARGE` can affect overflow evidence. Writes bypass typed config helpers and can desynchronize cached state from hardware, so use `recover()`, `softReset()`, or `begin()` to restore cached settings after manual register edits.

## Validation

These are validation targets, not hardware-validation claims. Evidence is
tracked by level so release wording cannot overstate what was actually run.

| Evidence level | Current status |
| --- | --- |
| Implemented | Core library, Arduino example, pure ESP-IDF example, CI configuration, and docs are present. |
| Tested by native fake-bus | Native Unity tests cover device semantics; run `pio test -e native` for current evidence. |
| Locally built | PlatformIO native, Arduino ESP32-S2/S3, and package commands are documented below; retain logs before claiming a dated local pass. |
| CI configured | GitHub Actions includes native tests, Arduino S2/S3 builds, package checks, guards, and ESP-IDF S2/S3 builds. |
| CI verified | Not claimed here; workflow logs for the current branch or PR must be reviewed before saying CI is green. |
| ESP-IDF build verified | Not claimed here unless local `idf.py` output or reviewed CI logs are captured. |
| Partial low-voltage HIL evidence | Compact evidence in `docs/validation/hardware-evidence.md` summarizes Arduino ESP32-S3 CLI-visible behavior and fixed-step transfer budgets from dirty worktrees. |
| Release-grade hardware validated | Not claimed; clean-commit framed HIL, 8-hour clean framed soak, fault-injection fixture coverage, alert-pin capture, controlled reset/power-cycle evidence, and reviewed CI logs are still required. |

Hardware validation is tracked separately in
[`docs/validation/hardware-validation-procedure.md`](docs/validation/hardware-validation-procedure.md).
Release-grade hardware validation remains unclaimed until dated clean-commit
logs, equipment, board, module, shunt, rail/load, commands, and pass/fail notes
are checked in. Use
[`tools/INA228_HIL_COMMAND_SEQUENCE.md`](tools/INA228_HIL_COMMAND_SEQUENCE.md)
for repeatable CLI transcripts and
[`docs/validation/release-checklist.md`](docs/validation/release-checklist.md) before
tagging.

```bash
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python tools/check_core_timing_guard.py
python tools/run_i2c_hil.py --parser-self-test
python tools/run_i2c_hil.py --dry-run --suite targeted
python tools/run_i2c_hil.py --dry-run --suite transfer
pio test -e native
pio run -e esp32s3dev
pio run -e esp32s2dev
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

## Calibration And Triggered Reads

- `setCalibration(ohm, A)` validates finite positive inputs and programs `SHUNT_CAL`.
- If the computed calibration exceeds the 15-bit register range, the driver clamps `SHUNT_CAL` and adjusts `currentLsb()` to the actual programmed value so current, power, energy, and charge scaling stay consistent with hardware.
- `SettingsSnapshot` exposes `calibrated`, `calibrationClamped`, `maxCurrentExceedsShuntRange`, `hardwareDirty`, `dirtyRegisterMask`, and `thresholdsDirty` for service diagnostics.
- `hardwareDirtyCause` in `SettingsSnapshot` reports the first status that made cached hardware state uncertain.
- `setAdcRange(range)` precomputes the new calibration, writes CONFIG and SHUNT_CAL as a guarded sequence, and rolls CONFIG back if SHUNT_CAL fails. The failed SHUNT_CAL write leaves conservative dirty evidence even when CONFIG rollback succeeds; converted current-derived APIs return `HARDWARE_DIRTY` until `recover()`. If rollback also fails, CONFIG is marked dirty too.
- Calibration and config setters update cached values only after required I2C writes succeed.
- Uncalibrated `begin()` writes `SHUNT_CAL=0`; raw voltage/temperature/register reads remain available, while current, power, energy, charge, and power-limit helpers return calibration errors.
- Changing ADCRANGE or calibration marks engineering-unit alert thresholds dirty. Reapply thresholds after scale changes before relying on ALERT comparisons.
- Triggered modes return `IN_PROGRESS`; measurement reads return `MEASUREMENT_NOT_READY` until the conversion time has elapsed and `CNVRF` is observed.
- ENERGY and CHARGE are not valid in triggered or shutdown modes. After `resetAccumulators()`, they remain invalid until a continuous CNVRF is observed.
- `softReset()` resets device threshold registers; the driver marks `thresholdsDirty` so applications can reapply service limits deliberately.
- Bus voltage and raw energy are treated as unsigned values; shunt voltage, current, and charge remain signed.

## License

MIT License. See [LICENSE](LICENSE) for details.
