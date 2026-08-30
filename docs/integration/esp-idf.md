# Native ESP-IDF integration

## Boundary

INA228 is a framework-neutral component. An ESP-IDF application owns the
`i2c_master_bus_handle_t`, device handles, pins, clock, serialization, callback
timeout, job deadline, retry, health, and bus-recovery policy. The library stores
only the callback functions and opaque user pointer.

The native example under `examples/esp_idf/basic` uses `app_main`,
`driver/i2c_master.h`, `esp_timer`, FreeRTOS task delays, fixed command buffers,
and a native callback adapter. It contains no Arduino compatibility layer.

## Component use

Add this repository as an ESP-IDF component or use `idf_component.yml`. The core
requires C++17, and the component manifest declares `idf >= 6.0.0`; CI builds the
native example against ESP-IDF v6.0.1. The example CMake project builds the library sources and public
headers directly so CI can validate both ESP32-S2 and ESP32-S3.

The callback adapter should retain the IDF handles in application-owned context:

The declarations below are an adapter sketch. The application provides the
function bodies, bus lifetime, locking, timeout enforcement, and error mapping.

```cpp
struct Ina228TransportContext {
  i2c_master_bus_handle_t bus;
  i2c_master_dev_handle_t device;
};

INA228::Status writeRead(uint8_t address,
                         const uint8_t* tx, size_t txLength,
                         uint8_t* rx, size_t rxLength,
                         uint32_t timeoutMs, void* user);
```

Do not expose `esp_err_t` or an IDF handle through a public INA228 type. Do not
reconfigure or recover the bus inside a driver callback. A callback is one
physical transfer attempt and must honor `timeoutMs`.

## Fixed-unit configuration

This fragment assumes application-defined `write`, `writeRead`, `transport`,
and `monotonicMilliseconds` symbols.

```cpp
INA228::Config config;
config.i2cWrite = write;
config.i2cWriteRead = writeRead;
config.i2cUser = &transport;
config.nowMs = monotonicMilliseconds;
config.timeUser = nullptr;
config.i2cAddress = 0x41;
config.i2cTimeoutMs = 20;
config.mode = INA228::Mode::CONT_ALL;
config.healthPolicy = INA228::HealthPolicy::PASSIVE;

config.calibration.shuntMicroOhms = 25000;
config.calibration.mode = INA228::CalibrationMode::FROM_MAXIMUM_CURRENT;
config.calibration.maxCurrentMilliAmps = 2500;

config.alerts.latched = false;
config.alerts.conversionReady = false;
config.alerts.slowAlert = false;
config.alerts.activeHigh = false;
```

The values above illustrate the current TunnelMonitor electrical limits; they
are not universal defaults. Select and validate the calibration against the
actual shunt, ADC range, maximum current, and desired current LSB.

## External owner lifecycle

Use the cooperative API from the one task that owns the I2C bus:

```cpp
INA228::Status status = monitor.bind(config); // validates, zero I2C
uint32_t operationId = 0;
if (status.ok()) {
  status = monitor.startInitialize(requestToken, operationId); // zero I2C
}

// One owner activation, at most one callback.
if (status.ok() || status.inProgress()) {
  status = monitor.pollJob(nowMs, 1);
}

// A zero budget is valid and bus-silent; it can advance an elapsed wait gate.
status = monitor.pollJob(nowMs, 0);
```

The owner must maintain an operation deadline independently of the library.
Before starting, use `getJobLimits()` to ensure the configured conversion wait
plus callback/scheduling allowance fits that deadline. Progress does not renew
the deadline. On expiry call `timeoutJob()`; on cancellation call `cancelJob()`.
Both are idempotent and perform no I2C.

The `config.nowMs` hook and every explicit `nowMs` passed to the library must
come from the same wrap-safe monotonic millisecond domain. For a trigger or
reset write, the library samples the hook only after the successful blocking
transport callback returns. If the hook is omitted, the next timestamped owner
activation establishes the post-write origin without I2C, then the complete
unchanged device wait elapses. This adds one bus-silent activation; it does not
change transfer budgets, device wait durations, or retries. With no hook,
`isConversionReady()` cannot advance an unresolved trigger; call
`pollConversionReady(nowMs, ...)` or `tick(nowMs)`.

After terminal state, call `takeJobResult(expectedOperationId, result)` once.
A successful take reports delivery, not necessarily job success. Inspect
`result.job.state`, `result.job.status`, and `result.job.effect`. If effect is
`PARTIAL` or `INDETERMINATE`, do not publish the measurement or blindly retry a
side-effecting transfer. `INDETERMINATE` also covers a destructive
`DIAG_ALRT` read that may have consumed evidence before its callback failed.
Reconcile through `invalidateHardwareState()` and verified
reinitialization/readback under application policy.

For normal measurement use `startInstantaneousSample()`. The returned sample is
atomic and contains request/operation identity, configuration generation,
valid-channel flags, fixed-unit values, raw data, and correlated diagnostics.
Do not publish a partial or stale result.

## Status mapping

Map IDF failures as precisely as the active API permits:

| IDF result/context | INA228 status |
|---|---|
| success | `OK` |
| definite address NACK | `I2C_NACK_ADDR` |
| definite data NACK | `I2C_NACK_DATA` |
| NACK with phase unavailable | `I2C_NACK_UNKNOWN_PHASE` |
| transfer timeout | `I2C_TIMEOUT` |
| arbitration/bus error | `I2C_BUS` |
| other transport failure | `I2C_ERROR` with `esp_err_t` in `detail` |

Do not map all `ESP_FAIL` values to address NACK unless the adapter has definite
phase information. Initialization maps only definite address NACK during
identity access to device-not-found semantics; timeout and bus faults remain
distinguishable.

With the ESP-IDF 6.x master-bus API used by the example,
`ESP_ERR_INVALID_RESPONSE` is the documented transfer NACK result. A combined
transmit/receive call does not expose whether that NACK came from the address or
data phase, so normal transfers map it to `I2C_NACK_UNKNOWN_PHASE`; the explicit
`i2c_master_probe()` path can map the same response to `I2C_NACK_ADDR`. Other
unclassified `esp_err_t` values map to `I2C_ERROR`, not an invented bus or NACK
category.

## Removal, recovery, and retries

The INA228 core does not call an IDF bus-recovery API. On removal, a bus fault,
or suspected device reset:

1. stop publishing the affected result;
2. cancel/timeout and consume any active inner job;
3. perform application-owned bus recovery if policy permits;
4. call `invalidateHardwareState(cause)` without I2C;
5. start reinitialization and advance it under the original/new owner deadline.

Read retry policy belongs to the application. A callback invoked for a write is
one attempt; if it fails ambiguously, preserve the `INDETERMINATE` effect and
reconcile rather than retrying blindly.

## Local checks

CI currently builds with ESP-IDF v6.0.1. Confirm the locally selected toolchain
before comparing results:

```sh
idf.py --version
python tools/check_idf_example_contract.py
python tools/check_owner_contract.py
python tools/test_run_i2c_hil_parser.py
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

Build output is created under `examples/esp_idf/basic/build/`. CI performs both
native ESP-IDF builds and the static contract check. A successful build proves
framework and target compilation only; it is not hardware validation.
