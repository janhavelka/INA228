# AGENTS.md - INA228 Contributor Guidelines

These are the binding engineering rules for this repository. `CONTRIBUTING.md`
covers the day-to-day workflow; this file covers the design constraints that
changes must respect.

## Scope

- Target: ESP32-S2 / ESP32-S3, Arduino and ESP-IDF consumers, PlatformIO/ESP-IDF.
- The library itself is framework-neutral C++17 and is meant to be usable both
  as a standalone bring-up/test tool for the INA228 and as a component inside a
  larger firmware.
- Goals: deterministic behavior, long-term stability, clean API contracts,
  portability, no surprises in the field.

## Build Tooling

On Windows, use `.\scripts\pio.cmd <arguments>`; it selects the current user's
VS Code-managed PlatformIO installation instead of a second Core install.

---

## Repository Model (Single Library)

```
include/INA228/         - Public API headers only (Doxygen)
  CommandTable.h        - Register addresses, masks, and scaling constants
  Status.h              - Err codes and Status
  Config.h              - Config, transport callbacks, enums
  INA228.h              - Driver class
  Version.h             - Auto-generated (do not edit)
src/                    - Implementation (.cpp)
test/                   - Native Unity tests and framework stubs
examples/
  01_basic_bringup_cli/ - Arduino/PlatformIO bring-up CLI
  esp_idf/basic/        - Native ESP-IDF bring-up CLI
  common/               - Example-only helpers (Log.h, BuildConfig.h,
                          BoardConfig.h, I2cTransport.h, I2cScanner.h,
                          CliStyle.h)
docs/                   - Integration, device reference, validation evidence
tools/                  - HIL runner and static contract checks (not packaged)
scripts/                - Version generation and the PlatformIO wrapper
platformio.ini, library.json, idf_component.yml, CMakeLists.txt, Doxyfile
README.md, CHANGELOG.md, CONTRIBUTING.md, SECURITY.md, AGENTS.md
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
- Deterministic: no unbounded loops/waits; all timeouts via deadlines, never `delay()` in library code.
- No unbounded retries, allocations, queues, or buffers in steady paths.
- Every hardware operation that can block must have a timeout and an observable
  failure path.
- Recovery logic must be bounded, deterministic, and testable.
- Prefer explicit state, explicit ownership, and small local helpers over hidden
  global state.
- Do not hide hardware failures behind silent retries or fake success.
- Cooperative lifecycle: `bind()`, `start*()`, `pollJob(nowMs, maxTransfers)`,
  `takeJobResult()`, `end()`. `begin()`/`tick()` remain as bounded synchronous
  conveniences layered on the same engine.
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
- Check device identity by reading MANUFACTURER_ID (`0x5449`) and DEVICE_ID.
  Validate `DEVICE_ID[15:4]` (DIEID) against `0x228` and check `DEVICE_ID[3:0]`
  (revision) against `Config::supportedRevisionMask`. Do not compare the whole
  DEVICE_ID against `0x2281`.
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

## Driver Architecture: Cooperative Owner Driver

The application owns the I2C bus and the clock. The driver exposes one
cooperative operation at a time and never blocks or retries on its own.

### Cooperative model (primary)

```
bind(config)                 - validate + cache desired state, no I2C
start<Operation>(token, &id) - arm one job, no I2C
pollJob(nowMs, maxTransfers) - advance with a bounded transfer budget
takeJobResult(id, &result)   - consume the terminal result exactly once
```

- `getJobLimits(kind, &limits)` declares the exact worst-case transfer count and
  wait time for the currently bound profile. Retries are always zero.
- Waits are gated on caller-supplied `nowMs`; a zero budget is legal and
  bus-silent.
- Terminal results are delivered exactly once and stay available until consumed.

### Synchronous conveniences (legacy, retained)

`begin()`, `recover()`, `resetAccumulators()`, and the scalar `read*()` calls
drive the same engine with a bounded internal poll loop. They are convenient for
bring-up and for single-owner firmware, but they take the bus for the whole
operation. Prefer the cooperative API in a shared-bus firmware.

### HardwareState

```cpp
enum class HardwareState : uint8_t { UNBOUND, UNKNOWN, SYNCHRONIZED, RESYNC_REQUIRED };
```

Converted current/power/energy/charge require `SYNCHRONIZED` plus a clean
calibration contract. Any partial or ambiguous write marks the driver dirty and
forces a verified reinitialization before those values are trusted again.

### DriverState (transport health)

```cpp
enum class DriverState : uint8_t {
  UNINIT,    // bind()/begin() not completed, or end() called
  READY,     // Operational, consecutiveFailures == 0
  DEGRADED,  // 1 <= consecutiveFailures < offlineThreshold
  OFFLINE    // consecutiveFailures >= offlineThreshold
};
```

`HealthPolicy::PASSIVE` is the default: health is observed but never suppresses
owner-requested I2C. `HealthPolicy::LATCH_OFFLINE` is a legacy opt-in that
latches OFFLINE and requires `recover()`.

### Transport Wrapper Architecture

All I2C goes through layered wrappers:

```
Public API (readShuntVoltage, readBusVoltage, pollJob, ...)
    v
Register helpers (readReg16, readReg24, readReg40, writeReg16)
    v
TRACKED wrappers (_i2cWriteReadTracked, _i2cWriteTracked)
    v  <- _updateHealth() called here ONLY
RAW wrappers (_i2cWriteReadRaw, _i2cWriteRaw)
    v
Transport callbacks (Config::i2cWrite, i2cWriteRead)
```

**Rules:**
- Public API methods NEVER call `_updateHealth()` directly.
- `readReg*()`/`writeReg*()` use TRACKED wrappers -> health updated automatically.
- `probe()` uses RAW wrappers -> no health tracking (diagnostic only).
- Health transitions are guarded by `_initialized`, and are not applied to
  config/param validation or precondition errors.

### Health Tracking Fields

- `_lastOkMs` - timestamp of last successful I2C operation
- `_lastErrorMs` - timestamp of last failed I2C operation
- `_lastError` - most recent error Status
- `_consecutiveFailures` - failures since last success (resets on success)
- `_totalFailures` / `_totalSuccess` - lifetime counters (saturate at max)

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
