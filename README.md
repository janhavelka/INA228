# INA228 Driver Library

Production-grade INA228 85-V, 20-bit I2C power/energy/charge monitor driver for ESP32 (Arduino/PlatformIO and ESP-IDF component use).

Library version: `v1.3.0`

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
  https://github.com/janhavelka/INA228.git#v1.3.0
```

### Manual

Copy `include/INA228/` and `src/` to your project.

### ESP-IDF

The repository root is an ESP-IDF component. Add it through `EXTRA_COMPONENT_DIRS`
or the component manager metadata, then provide `Config::i2cWrite`,
`Config::i2cWriteRead`, and `Config::nowMs` from your application-owned I2C
adapter. The native example in `examples/esp_idf/basic` uses ESP-IDF
`driver/i2c_master.h`, `app_main`, `esp_timer`, FreeRTOS delays, and fixed
command buffers while preserving Arduino CLI command coverage.

## Release 1.3.0 Highlights

- Adds the repository root ESP-IDF component metadata and root `CMakeLists.txt`.
- Adds the native ESP-IDF `examples/esp_idf/basic` CLI using `driver/i2c_master.h`, `app_main`, `esp_timer`, FreeRTOS delays, and fixed command buffers.
- Preserves Arduino and ESP-IDF user-visible CLI parity for scan/probe, measurements, calibration, alert limits, raw register diagnostics, stress, and self-test workflows.
- Keeps the driver core framework-neutral; hardware access remains callback-injected and timing comes from application-provided `Config::nowMs`.
- Arduino example behavior has owner hardware-test coverage and remains the
  reference behavior. ESP-IDF support is implemented and statically guarded,
  but still requires an ESP-IDF build and hardware validation before release.

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
  cfg.shuntResistanceOhm = 0.015f;  // 15 mΩ shunt
  cfg.maxExpectedCurrentA = 10.0f;   // 10 A max
  
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

## API Overview

### Lifecycle

| Method | Description |
|--------|-------------|
| `begin(config)` | Initialize with configuration (validates, verifies device ID + MEMSTAT) |
| `tick(nowMs)` | Process pending operations (call from loop) |
| `end()` | Shutdown and release resources |
| `isInitialized()` | True after successful `begin()` until `end()` |
| `getConfig()` | Return the driver's cached configuration snapshot |
| `getSettings(snap)` | Populate a `SettingsSnapshot` with cached config, calibration, conversion, trigger, and health state without I2C |

### Measurements

| Method | Description |
|--------|-------------|
| `readMeasurement(m)` | Read all channels with energy/charge validity flags |
| `readBusVoltage(v)` | Bus voltage in volts (0–85 V) |
| `readShuntVoltage(v)` | Shunt voltage in volts |
| `readTemperature(t)` | Die temperature in °C |
| `readCurrent(i)` | Current in amperes (requires calibration) |
| `readPower(p)` | Power in watts (requires calibration) |
| `readEnergy(e)` | Accumulated energy in joules; fails when accumulation is invalid or overflowed |
| `readCharge(q)` | Accumulated charge in coulombs; fails when accumulation is invalid or overflowed |
| `isConversionReady(r)` | Check CNVRF flag using `Config::nowMs` for pending-trigger deadline gating |
| `pollConversionReady(nowMs, r)` | Check CNVRF using a caller-supplied timestamp |

### Configuration

| Method | Description |
|--------|-------------|
| `setMode(mode)` | Set ADC operating mode |
| `triggerConversion(mode)` | Start single-shot conversion |
| `setVbusConvTime(ct)` | Bus voltage conversion time |
| `setVshuntConvTime(ct)` | Shunt voltage conversion time |
| `setTempConvTime(ct)` | Temperature conversion time |
| `setAveraging(avg)` | Averaging count (1–1024) |
| `setAdcRange(range)` | Shunt full-scale range (±163.84 or ±40.96 mV) |
| `setCalibration(ohm, A)` | Update shunt calibration for the installed shunt resistor and expected current |
| `setShuntTempCoeff(ppm)` | Shunt temperature coefficient |
| `softReset()` | Full software reset |
| `resetAccumulators()` | Clear energy/charge registers |

`setShuntTempCoeff(ppm)` writes the configured `SHUNT_TEMPCO` coefficient even
when temperature compensation is disabled. `tempCompEnabled` controls whether
the coefficient participates in calibration behavior; the register value remains
explicitly programmed for deterministic readback.

### Health & Diagnostics

| Method | Description |
|--------|-------------|
| `state()` | Current driver state (UNINIT/READY/DEGRADED/OFFLINE) |
| `isOnline()` | True if READY or DEGRADED |
| `probe()` | Check device presence (no health tracking) |
| `recover()` | Re-validate manufacturer ID, device ID, MEMSTAT, then re-apply config/calibration |
| `readDiagAlert(diag)` | Read and consume current `DIAG_ALRT` flags |
| `readDiagAlertRaw(raw)` | Read and consume raw `DIAG_ALRT` register value |
| `getDiagAlertSnapshot(snapshot)` | Return last preserved `DIAG_ALRT` evidence without I2C |
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
full raw value in `getDiagAlertSnapshot()`. Alert configuration setters write
cached configuration bits without first reading live `DIAG_ALRT` status.
Accumulator reads first preserve `DIAG_ALRT` when they may touch `ENERGY` or
`CHARGE`, because reading those registers can clear `ENERGYOF` or `CHARGEOF`.

### Raw Register Access

| Method | Description |
|--------|-------------|
| `readRegister16(reg, value)` | Read a tracked 16-bit register |
| `readRegister24(reg, value)` | Read a tracked 24-bit register |
| `readRegister40(reg, value)` | Read a tracked 40-bit register |
| `writeRegister16(reg, value)` | Write a tracked 16-bit register |

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

1. **Threading model**: Single-threaded. All API calls from one task/loop.
2. **Timing model**: `tick()` is bounded; all I2C operations are blocking.
3. **Resource ownership**: I2C bus owned by application; library receives transport callbacks.
4. **Framework boundary**: Core code does not call `Wire`, `Serial`, `delay()`, `yield()`, `millis()`, or ESP-IDF peripheral APIs directly. Arduino examples and native ESP-IDF examples provide those hooks externally.
5. **Memory behavior**: No heap allocation after `begin()`.
6. **Error handling**: All fallible APIs return `Status`. Check with `st.ok()`.
7. **Recovery model**: `OFFLINE` is latched. Supervisors should call `recover()` after applying any bus-level recovery policy.
8. **Measurement freshness**: Continuous-mode reads return the latest hardware register contents at read time; they are not guaranteed fresh since the previous API call. Driver-tracked triggered conversions return `MEASUREMENT_NOT_READY` until the software deadline has elapsed and CNVRF is observed. `tick(nowMs)` and `pollConversionReady(nowMs, ready)` use the supplied timestamp, so they can advance pending triggered conversions even when `Config::nowMs` is unset.
9. **Accumulator validity**: ENERGY requires continuous shunt-and-bus conversion, CHARGE requires continuous shunt conversion, and both require calibration plus a continuous CNVRF observed after begin/reset/accumulator reset. Scalar `readEnergy()` and `readCharge()` return `ACCUMULATION_INVALID`, `ACCUMULATION_OVERFLOW`, or `MATH_OVERFLOW` instead of returning invalid data. `readMeasurement()` and `readRawSample()` keep their aggregate read shape but mark `energyValid` and `chargeValid` false when those fields are not valid.
10. **Calibration coherence**: `SHUNT_CAL`, `currentLsb()`, `shuntResistanceOhm`, `maxExpectedCurrentA`, and `adcRange` are treated as one scaling contract. Converted current, power, energy, charge, and power-limit APIs require a valid calibration and a clean hardware/cache state. If a multi-register range/calibration update cannot be rolled back after an I2C failure, those APIs return `HARDWARE_DIRTY` until `recover()` successfully replays the cached configuration.

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

Raw register writes are intended for diagnostics and bring-up. They bypass the typed config helpers, so use `recover()` or `begin()` to restore cached settings after manual register edits.

## Validation

```bash
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python tools/check_core_timing_guard.py
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
- `setAdcRange(range)` precomputes the new calibration, writes CONFIG and SHUNT_CAL as a guarded sequence, and rolls CONFIG back if SHUNT_CAL fails. If rollback also fails, converted current-derived APIs return `HARDWARE_DIRTY` until `recover()`.
- Calibration and config setters update cached values only after required I2C writes succeed.
- Uncalibrated `begin()` writes `SHUNT_CAL=0`; raw voltage/temperature/register reads remain available, while current, power, energy, charge, and power-limit helpers return calibration errors.
- Changing ADCRANGE or calibration marks engineering-unit alert thresholds dirty. Reapply thresholds after scale changes before relying on ALERT comparisons.
- Triggered modes return `IN_PROGRESS`; measurement reads return `MEASUREMENT_NOT_READY` until the conversion time has elapsed and `CNVRF` is observed.
- ENERGY and CHARGE are not valid in triggered or shutdown modes. After `resetAccumulators()`, they remain invalid until a continuous CNVRF is observed.
- Bus voltage and raw energy are treated as unsigned values; shunt voltage, current, and charge remain signed.

## License

MIT License. See [LICENSE](LICENSE) for details.
