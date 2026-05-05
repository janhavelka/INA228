# INA228 Driver Library

Production-grade INA228 85-V, 20-bit I2C power/energy/charge monitor driver for ESP32 (Arduino/PlatformIO).

## Features

- **Injected I2C transport** - no Wire dependency in library code
- **Health monitoring** - automatic state tracking (READY/DEGRADED/OFFLINE)
- **Deterministic behavior** - no unbounded loops, no heap allocations
- **Managed synchronous lifecycle** - blocking I2C ops with tick-based architecture
- **INA228 coverage** - ADC modes, guarded triggered conversions, calibration, alerts, energy/charge accumulation

## Installation

### PlatformIO (recommended)

Add to `platformio.ini`:

```ini
lib_deps = 
  https://github.com/janhavelka/INA228.git
```

### Manual

Copy `include/INA228/` and `src/` to your project.

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

### Measurements

| Method | Description |
|--------|-------------|
| `readMeasurement(m)` | Read all channels (V, I, T, P, E, Q) |
| `readBusVoltage(v)` | Bus voltage in volts (0–85 V) |
| `readShuntVoltage(v)` | Shunt voltage in volts |
| `readTemperature(t)` | Die temperature in °C |
| `readCurrent(i)` | Current in amperes (requires calibration) |
| `readPower(p)` | Power in watts (requires calibration) |
| `readEnergy(e)` | Accumulated energy in joules |
| `readCharge(q)` | Accumulated charge in coulombs |
| `isConversionReady(r)` | Check CNVRF flag |

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
| `setCalibration(ohm, A)` | Update shunt calibration |
| `setShuntTempCoeff(ppm)` | Shunt temperature coefficient |
| `softReset()` | Full software reset |
| `resetAccumulators()` | Clear energy/charge registers |

### Health & Diagnostics

| Method | Description |
|--------|-------------|
| `state()` | Current driver state (UNINIT/READY/DEGRADED/OFFLINE) |
| `isOnline()` | True if READY or DEGRADED |
| `probe()` | Check device presence (no health tracking) |
| `recover()` | Re-validate manufacturer ID, device ID, MEMSTAT, then re-apply config/calibration |
| `readDiagAlert(diag)` | Read all diagnostic/alert flags |

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
4. **Memory behavior**: No heap allocation after `begin()`.
5. **Error handling**: All fallible APIs return `Status`. Check with `st.ok()`.
6. **Recovery model**: `OFFLINE` is latched. Supervisors should call `recover()` after applying any bus-level recovery policy.

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
- `addr [0x40..0x4F]` selects the target device address used by the example
- `init [addr]` re-initializes at the selected address; startup can auto-detect a single healthy INA228 if the default address fails
- `cal`, `tempco`, `tempcomp`, `delay`, alert-threshold commands, and `reg16` / `reg24` / `reg40` / `wreg16` expose service-level diagnostics

Raw register writes are intended for diagnostics and bring-up. They bypass the typed config helpers, so use `recover()` or `begin()` to restore cached settings after manual register edits.

## Calibration And Triggered Reads

- `setCalibration(ohm, A)` validates finite positive inputs and programs `SHUNT_CAL`.
- If the computed calibration exceeds the 15-bit register range, the driver clamps `SHUNT_CAL` and adjusts `currentLsb()` to the actual programmed value so current, power, energy, and charge scaling stay consistent with hardware.
- Calibration and config setters update cached values only after required I2C writes succeed.
- Triggered modes return `IN_PROGRESS`; measurement reads return `MEASUREMENT_NOT_READY` until the conversion time has elapsed and `CNVRF` is observed.
- Bus voltage and raw energy are treated as unsigned values; shunt voltage, current, and charge remain signed.

## License

MIT License. See [LICENSE](LICENSE) for details.
