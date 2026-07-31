# AGENTS.md - INA228 Production Embedded Guidelines

## Role and Target
You are a professional embedded software engineer building a production-grade INA228 power monitor library.

- Target: ESP32-S2 / ESP32-S3, Arduino and ESP-IDF consumers, PlatformIO/ESP-IDF.
- Goals: deterministic behavior, long-term stability, clean API contracts, portability, no surprises in the field.
- These rules are binding.

---

## Repository Model (Single Library)

```
include/INA228/         - Public API headers only (Doxygen)
  CommandTable.h        - Register addresses and bit masks
  Status.h
  Config.h
  INA228.h
  Version.h             - Auto-generated (do not edit)
src/                    - Implementation (.cpp)
examples/
  01_*/
  common/               - Example-only helpers (Log.h, BoardConfig.h, I2cTransport.h,
                          I2cScanner.h, CliStyle.h)
platformio.ini
library.json
README.md
CHANGELOG.md
AGENTS.md
```

Rules:
- `examples/common/` is NOT part of the library. It simulates project glue and keeps examples self-contained.
- No board-specific pins/bus in library code; only in `Config`.
- Public headers only in `include/INA228/`.
- Examples demonstrate usage and may use `examples/common/BoardConfig.h`.
- Arduino examples may use Arduino APIs and `examples/common/` helpers.
- ESP-IDF examples must be native IDF code. They must use `app_main`,
  `driver/i2c_master.h`, `esp_timer`, FreeRTOS delays, and fixed C buffers or
  native console APIs. They must not include Arduino example sources or use
  `Arduino.h`, `Wire.h`, `String`, `Serial`, `TwoWire`, `ArduinoCompat`, or
  `IdfArduinoCompat` facades.
- Keep the layout boring and predictable.

---

## Core Engineering Rules (Mandatory)

- Prefer simplicity, clarity, correctness, robustness, safety, and readability
  over clever abstractions or speculative flexibility.
- Before coding, inspect whether existing code can be simplified, reused, or
  deleted.
- Prefer deleting unnecessary code over adding new code.
- Prefer extending existing owners, modules, APIs, and contracts over creating
  parallel abstractions.
- Before adding a new service, class, file, interface, manager, registry, or
  layer, confirm there is a concrete current need and a clear caller or test.
- Do not add placeholder classes, future stubs, empty managers, broad
  frameworks, plugin systems, registries, or speculative extension points unless
  the current task explicitly requires them.
- Keep changes tightly scoped to the user's request.
- Preserve dirty user changes and never revert unrelated work.
- Deterministic: no unbounded loops/waits; all timeouts via deadlines, never `delay()` in library code.
- No unbounded retries, allocations, queues, or buffers in steady paths.
- Every hardware operation that can block must have a timeout and an observable
  failure path.
- Recovery logic must be bounded, deterministic, and testable.
- Prefer explicit state, explicit ownership, and small local helpers over hidden
  global state.
- Do not hide hardware failures behind silent retries or fake success.
- Non-blocking lifecycle: `Status begin(const Config&)`, `void tick(uint32_t nowMs)`, `void end()`.
- Any I/O that can exceed ~1-2 ms must be split into state machine steps driven by `tick()`.
- No heap allocation in steady state (no `String`, `std::vector`, `new` in normal ops).
- Avoid dynamic allocation in steady embedded paths unless it is already an
  accepted local pattern and the bound is clear.
- No logging in library code; examples may log.
- No macros for constants; use `static constexpr`. Macros only for conditional compile or logging helpers.
- Public/core library headers and `src/` must not require Arduino or ESP-IDF
  framework headers unless a platform-specific adapter is explicitly documented.

---

## INA228 development rules

- Core library code in `include/` and `src/` must remain framework-neutral: no Arduino, Wire, ESP-IDF, FreeRTOS, logging framework, heap-heavy framework types, or platform delays.
- Core code must not hide platform waits in helper callbacks, busy loops, framework delay calls, or implicit retries. Any wait policy belongs to the injected transport or application and must be bounded by an explicit deadline.
- The core must not own the I2C bus. Bus setup, locking, timeout policy, recovery, and platform handles belong to injected transports or examples.
- Public fallible APIs must return precise status/error information. Do not collapse timeout, address NACK, data NACK, bus error, invalid configuration, not-ready, overflow, or calibration-invalid states into generic failure.
- `DIAG_ALRT` is both configuration and live/destructive diagnostic state. Treat reads as status-sensitive and clear-on-read where the datasheet says so; internal polling and alert setters must preserve evidence or intentionally consume it through a documented API.
- Do not use read-modify-write on live/status-sensitive registers such as `DIAG_ALRT` unless the side effects, reserved-bit handling, and evidence-preservation policy are explicit and tested.
- Do not expose current, power, energy, or charge helpers unless SHUNT_CAL/current-LSB behavior is well-defined and tested.
- Treat SHUNT_CAL, ADCRANGE, shunt resistance, max expected current, and current LSB as one coherent contract.
- Energy and charge accumulation are valid only under the datasheet-supported operating assumptions. Mode, accumulator reset, overflow flags, elapsed-time semantics, and triggered-mode limitations must be documented and enforced by API behavior.
- Preserve signedness and register width exactly. INA228 has wide measurement registers; VSHUNT, CURRENT, CHARGE, and DIETEMP are signed, VBUS/POWER/ENERGY are unsigned, and byte ordering must be tested.
- Do not write multi-register configurations without considering partial hardware state and cache consistency.
- Multi-register or multi-step hardware updates need a dirty, rollback, or resync policy. Do not silently leave software cache and hardware state divergent after partial failure.
- DEVICE_ID and MANUFACTURER_ID checks must be explicit and tested.
- Public APIs are not ISR-safe unless explicitly documented and proven. Instances are not thread-safe unless protected by the application.
- High-voltage measurement examples must include safety disclaimers and must not imply the board/library itself makes 85 V systems safe.
- Documentation must separate implemented behavior, native tests, build validation, hardware validation, and remaining limitations.
- Do not claim production readiness, industry readiness, ESP-IDF readiness, or hardware validation without dated, commit-linked logs or reproducible CI results.

---

## I2C Manager + Transport (Required)

- The I2C bus must have one clear owner.
- The library MUST NOT own I2C. It never touches `Wire` directly.
- Device drivers must not directly own or reconfigure a shared bus unless this
  repository's architecture explicitly says so.
- `Config` MUST accept a transport adapter (function pointers or abstract interface).
- Transport errors MUST map to `Status` (no leaking `Wire`, `esp_err_t`, etc.).
- I2C transactions must be timeout-bounded and report errors clearly.
- The library MUST NOT configure bus timeouts or pins.
- Do not implement chip protocols manually if an existing hardened project
  library already provides the needed timeout, recovery, and testability
  behavior.
- Keep chip-level protocol code inside the driver or wrapper. Keep application
  policy outside the chip driver.
- Do not add fake devices, simulated buses, or test doubles to production paths.
- ESP-IDF adapters/examples own IDF bus handles and map `esp_err_t` to
  `Status`; the core must never expose or store IDF handles.

---

## Status / Error Handling (Mandatory)

All fallible APIs return `Status`:

```cpp
struct Status {
  Err code;
  int32_t detail;
  const char* msg;  // static string only
};
```

- Silent failure is unacceptable.
- No exceptions.

---

## INA228 Driver Requirements

- I2C address configurable: 16 addresses from 0x40 to 0x4F (A0/A1 pins: GND, VS, SDA, SCL).
- Check device presence in `begin()` by reading MANUFACTURER_ID (0x5449) and DEVICE_ID (0x2281).
- Verify MEMSTAT bit in DIAG_ALRT register for NV trim memory health.
- Support shunt full-scale range selection: ±163.84 mV (ADCRANGE=0) or ±40.96 mV (ADCRANGE=1).
- Configurable ADC conversion times: 50, 84, 150, 280, 540, 1052, 2074, 4120 µs.
- Configurable averaging: 1, 4, 16, 64, 128, 256, 512, 1024 samples.
- Support operating modes:
  - **Shutdown**: ADC off, lowest power.
  - **Triggered (one-shot)**: Single conversion then shutdown.
  - **Continuous**: Continuous conversion loop (default).
- Shunt calibration register must be programmed for current/power/energy/charge calculations.
- Temperature compensation support via shunt temperature coefficient register.
- Conversion delay support for multi-device synchronization.
- Read all measurement outputs: shunt voltage, bus voltage, temperature, current, power, energy, charge.
- Proper 20-bit signed result handling for shunt voltage and current; proper
  20-bit unsigned result handling for bus voltage.
- Proper 24-bit unsigned power, 40-bit unsigned energy, 40-bit signed charge.
- Voltage/current/power/energy/charge conversion using datasheet formulas.
- Alert system: configurable thresholds for shunt OV/UV, bus OV/UV, temperature OL, power OL.
- Alert configuration: latch/transparent mode, polarity, conversion ready, slow alert.
- Diagnostic flags: energy/charge overflow, math overflow, memory status.
- Software reset and accumulator reset support.

---

## Driver Architecture: Managed Synchronous Driver

The driver follows a **managed synchronous** model with health tracking:

- All public I2C operations are **blocking** (no async - INA228 has no EEPROM writes).
- `tick()` may be used for triggered conversion wait or continuous mode polling.
- Health is tracked via **tracked transport wrappers** -- public API never calls `_updateHealth()` directly.
- Recovery is **manual** via `recover()` - the application controls retry strategy.

### DriverState (4 states only)

```cpp
enum class DriverState : uint8_t {
  UNINIT,    // begin() not called or end() called
  READY,     // Operational, consecutiveFailures == 0
  DEGRADED,  // 1 <= consecutiveFailures < offlineThreshold
  OFFLINE    // consecutiveFailures >= offlineThreshold
};
```

State transitions:
- `begin()` success -> READY
- Any I2C failure in READY -> DEGRADED
- Success in DEGRADED/OFFLINE -> READY
- Failures reach `offlineThreshold` -> OFFLINE
- `end()` -> UNINIT

### Transport Wrapper Architecture

All I2C goes through layered wrappers:

```
Public API (readShuntVoltage, readBusVoltage, etc.)
    ↓
Register helpers (readReg16, readReg24, readReg40, writeReg16)
    ↓
TRACKED wrappers (_i2cWriteReadTracked, _i2cWriteTracked)
    ↓  <- _updateHealth() called here ONLY
RAW wrappers (_i2cWriteReadRaw, _i2cWriteRaw)
    ↓
Transport callbacks (Config::i2cWrite, i2cWriteRead)
```

**Rules:**
- Public API methods NEVER call `_updateHealth()` directly
- `readReg*()`/`writeReg*()` use TRACKED wrappers -> health updated automatically
- `probe()` uses RAW wrappers -> no health tracking (diagnostic only)
- `recover()` tracks probe failures (driver is initialized, so failures count)

### Health Tracking Rules

- `_updateHealth()` called ONLY inside tracked transport wrappers.
- State transitions guarded by `_initialized` (no DEGRADED/OFFLINE before `begin()` succeeds).
- NOT called for config/param validation errors (INVALID_CONFIG, INVALID_PARAM).
- NOT called for precondition errors (NOT_INITIALIZED).
- `probe()` uses raw I2C and does NOT update health (diagnostic only).

### Health Tracking Fields

- `_lastOkMs` - timestamp of last successful I2C operation
- `_lastErrorMs` - timestamp of last failed I2C operation
- `_lastError` - most recent error Status
- `_consecutiveFailures` - failures since last success (resets on success)
- `_totalFailures` / `_totalSuccess` - lifetime counters (wrap at max)

---

## Versioning and Releases

Single source of truth: `library.json`. `Version.h` is auto-generated and must never be edited.

SemVer:
- MAJOR: breaking API/Config/enum changes.
- MINOR: new backward-compatible features or error codes (append only).
- PATCH: bug fixes, refactors, docs.

Release steps:
1. Update `library.json`.
2. Update `CHANGELOG.md` (Added/Changed/Fixed/Removed).
3. Update `README.md` if API or examples changed.
4. Commit and tag: `Release vX.Y.Z`.

---

## Naming Conventions

- Member variables: `_camelCase`
- Methods/Functions: `camelCase`
- Constants: `CAPS_CASE`
- Enum values: `CAPS_CASE` or short forms
- Locals/params: `camelCase`
- Config fields: `camelCase`
