# INA228 Industry-Readiness Exploration

Date: 2026-05-31
Repository: `C:/Users/HonzovoSpectre/Documents/Projects/INA228`
Branch: `audit/ina228-industry-readiness-exploration`
Audit mode: exploration only / no implementation

## Executive Summary

The INA228 repository is a serious engineering-grade driver, not a toy driver.
The strongest parts are the framework-neutral core, injected I2C transport,
precise `Status` object, identity/MEMSTAT checks, typed configuration API,
health tracking, native tests, Arduino CLI, and a native ESP-IDF example.

It is not ready to call industry-standard or production-grade yet. The main
blockers are not broad architecture problems; they are device-specific edge
cases that can corrupt interpretation of real hardware state:

- `DIAG_ALRT` is both configuration and live status/clear-on-read state. Current
  conversion-ready polling and alert read-modify-write setters can clear and
  discard alert evidence.
- Triggered conversion bookkeeping has stale/freshness holes, especially when
  `Config::nowMs` is unset or when `Config::mode` starts in a triggered mode.
- ENERGY and CHARGE are exposed in modes where the datasheet says the
  accumulation registers are invalid.
- `setAdcRange()` can leave the hardware range changed while the software cache
  is rolled back after a calibration failure.
- Hardware validation and pure ESP-IDF build proof are not present in CI or in a
  dated validation matrix.
- High-voltage safety documentation is too thin for an 85 V monitor.

Implementation can start immediately. The local datasheet and extracted docs
are present; no missing datasheet blocks the work. Hardware validation should
wait until the P0 correctness issues and test gaps are fixed.

## Readiness Classification

**Engineering-grade with major gaps.**

The core design is broadly sound and many datasheet basics are implemented, but
there are high-severity datasheet-semantics, cache-consistency, validation, and
documentation gaps that prevent a production/field claim.

## Scope Reviewed

- Public core headers: `include/INA228/CommandTable.h`, `Config.h`, `INA228.h`,
  `Status.h`, generated `Version.h`
- Core implementation: `src/INA228.cpp`
- Arduino example and example glue: `examples/01_basic_bringup_cli/`,
  `examples/common/`
- Native ESP-IDF example and adapter: `examples/esp_idf/basic/`
- Tests and stubs: `test/test_basic.cpp`, `test/stubs/`
- Build/package files: `platformio.ini`, `library.json`, `idf_component.yml`,
  root `CMakeLists.txt`
- CI/check scripts: `.github/workflows/ci.yml`, `tools/*.py`,
  `scripts/generate_version.py`
- Documentation: `README.md`, `CHANGELOG.md`, `docs/`, `_txt/`,
  `INA228_power_monitor_implementation_manual.md`, `AGENTS.md`

## Datasheet / Documentation Sources Found

- `docs/INA228_datasheet.pdf`
- `docs/pdf-extracted-md/INA228_datasheet.md`
  - Register map: around line 1036
  - Conversion result freshness: lines 657-659
  - CNVRF clearing: lines 669-671
  - Triggered-mode accumulation invalidity: lines 673-676
  - CONVDLY field: lines 1099-1102
  - DIAG_ALRT flags and overflow behavior: around line 1322
  - Manufacturer/device ID registers: around line 1453
  - SHUNT_CAL formula and ADCRANGE multiplier: lines 1522-1525
- `_txt/datasheet_INA228.txt`
- `INA228_power_monitor_implementation_manual.md`
- Curated extracted notes in `docs/extracted-md/`
- Application-note summaries in `docs/application_notes/` and
  `docs/pdf-extracted-md/`

Datasheet coverage is present and adequate for implementation work.

## INA228 Datasheet-Correctness Matrix

| Feature | Status | Evidence | Risk | Required action |
| --- | --- | --- | --- | --- |
| DEVICE_ID / MANUFACTURER_ID | PASS | Constants in `include/INA228/CommandTable.h:39`; checked in `src/INA228.cpp:229` and following ID reads. Datasheet ID docs at `docs/pdf-extracted-md/INA228_datasheet.md:1453`. | Low | Preserve checks and add begin-time mismatch tests. |
| Register map / widths | PASS | Register constants in `CommandTable.h:14`; 16/24/40-bit helpers in `src/INA228.cpp:1297`. | Low | Add byte-count/endian tests. |
| SHUNT_CAL / current LSB | PARTIAL | Formula in `src/INA228.cpp:97`; ADCRANGE multiplier in `src/INA228.cpp:111`; datasheet formula at `INA228_datasheet.md:1522`. | Silent clamp weakens `maxExpectedCurrentA` contract. | Add explicit calibration status, normal vectors, clamp/underflow policy tests. |
| ADCRANGE | PARTIAL | Setter starts at `src/INA228.cpp:834`; VSHUNT LSB uses cached range in `src/INA228.cpp:461`. | Hardware/cache can diverge if calibration reapply fails. | Make range/calibration update atomic or dirty/rollback-visible. |
| Shunt voltage scaling | PASS/PARTIAL | Signed 20-bit extension in `src/INA228.cpp:1572`; scaling in `src/INA228.cpp:461`. | Negative/full-scale paths not tested. | Add negative/full-scale vectors for both ranges. |
| Bus voltage scaling | PASS | `RawSample::vbus` is unsigned in `include/INA228/INA228.h:36`; README says raw bus voltage is unsigned. | Prior AGENTS rule incorrectly called VBUS signed. | Correct future-agent rule and keep unsigned tests. |
| Temperature scaling | PARTIAL | `readTemperature()` starts at `src/INA228.cpp:587`; TEMP LSB in `CommandTable.h:136`. | Negative temperature not tested. | Add signed temperature vectors. |
| Current scaling | PARTIAL | `readCurrent()` starts at `src/INA228.cpp:604`; signed extension in `src/INA228.cpp:620`. | Negative current and exact formulas not tested. | Add datasheet positive/negative vectors. |
| Power scaling | PARTIAL | `readPower()` starts at `src/INA228.cpp:625`; coefficient in `CommandTable.h:138`. | `MATHOF` not checked; float overflow possible with extreme accepted calibration. | Add overflow and non-finite output handling policy. |
| Energy scaling | WRONG/PARTIAL | `readEnergy()` starts at `src/INA228.cpp:645`; datasheet says triggered ENERGY invalid at `INA228_datasheet.md:673`. | API can return invalid accumulation data. | Reject or explicitly mark invalid outside continuous modes; check overflow before reads. |
| Charge scaling | WRONG/PARTIAL | `readCharge()` starts at `src/INA228.cpp:666`; datasheet triggered invalidity at `INA228_datasheet.md:673`. | Same as energy, plus signed 40-bit edge cases untested. | Add continuous-mode guard and signed vectors. |
| Conversion mode/freshness | PARTIAL | `_ensureMeasurementReadyForRead()` returns OK when no trigger pending at `src/INA228.cpp:1472`; datasheet output registers remain until replaced at `INA228_datasheet.md:657`. | Fresh/stale semantics are not exposed; begin with `TRIG_*` is not tracked. | Add explicit freshness API/contract and trigger tracking from begin. |
| Averaging/conversion timing | PARTIAL | `estimateConversionTimeUs()` at `src/INA228.cpp:1207`; `CONVDLY` docs at `INA228_datasheet.md:1099`. | First-ready latency and steady-state period are conflated. | Document or split timing estimates. |
| DIAG_ALRT / alerts | WRONG/PARTIAL | `readDiagAlert()` at `src/INA228.cpp:941`; alert setters at `src/INA228.cpp:976`, `993`, `1010`, `1027`; CNVRF clear on read at `INA228_datasheet.md:669`. | Polling and configuration can clear/discard flags. | Maintain cached config bits or write-only masks; model clear-on-read in tests. |
| Threshold registers | PARTIAL | Threshold setters start at `src/INA228.cpp:1044`; power limit at `src/INA228.cpp:1126`. | Existing thresholds become stale after range/calibration changes. | Track/invalidate/reapply thresholds when scale changes. |
| Reset/shutdown | PARTIAL | `softReset()` at `src/INA228.cpp:1151`; `resetAccumulators()` at `src/INA228.cpp:1179`. | Reset reapply has no POR wait; RSTACC semantics not confirmed. | Add bounded reset wait/readback policy and tests. |
| Address mapping | PASS | Address validation is `0x40..0x4F` in `src/INA228.cpp:35`; README table covers all straps. | Low | Keep scan/probe tests and hardware validation. |

## Architecture Scorecard

| Area | Rating | Notes |
| --- | --- | --- |
| Core framework neutrality | Strong | `include/` and `src/` avoid Arduino, Wire, ESP-IDF, FreeRTOS, logging, and platform timing APIs. |
| I2C ownership/injection | Strong | Callback injection and user context in `Config.h`; core does not own bus or pins. |
| Status/error model | Good | `Status { Err, detail, msg }` is strong, but begin/probe collapse some transport failures to `DEVICE_NOT_FOUND`. |
| Calibration contract | Medium | Formula is present, but clamp/uncalibrated/range-change behavior needs clearer policy. |
| Scaling/math correctness | Medium | Basic formulas exist; exact nonzero/negative/overflow vectors are missing. |
| Timing/determinism | Medium | No core delays or loops, but `tick(nowMs)` and `_nowMs()` contract is inconsistent for triggered mode. |
| Partial hardware state | Weak | Multi-step config writes can leave cache/hardware diverged; output structs can be partially updated on error. |
| Health/recovery | Good | READY/DEGRADED/OFFLINE model is useful; recovery reapply paths need deeper partial-failure tests. |
| Thread/ISR contract | Medium | README says single-threaded, but public header lacks explicit not-thread-safe/not-ISR-safe contract. |
| Tests/fault injection | Medium | Native suite is valuable but misses alert side effects, negative vectors, exact formulas, copy/move, and partial failures. |
| ESP-IDF readiness | Medium/Weak | Native example exists and is statically guarded; no `idf.py` CI build and local `idf.py` unavailable. |
| Arduino ESP32-S2/S3 readiness | Unknown locally | CI has PlatformIO Arduino jobs; local `python -m platformio run` failed for both S2/S3 in framework object builds during this audit. |
| Documentation honesty | Medium | README is useful but overstates "production-grade" relative to missing validation and safety docs. |
| Hardware validation | Weak | Hardware logs/matrix are not in repo and no hardware commands were run in this audit. |

## What Is Strong

- The public core is framework-neutral. `Config` provides injected
  `I2cWriteFn`, `I2cWriteReadFn`, and optional `NowMsFn` in
  `include/INA228/Config.h`.
- The core does not own I2C, pins, bus clock, or platform handles.
- The status model is richer than boolean returns. `Status` and `Err` are
  defined in `include/INA228/Status.h`.
- `begin()` validates configuration, then checks manufacturer ID, device ID,
  and MEMSTAT before declaring READY (`src/INA228.cpp:145` and following).
- Register constants, widths, reset values, LSB constants, ADC conversion-time
  table, and averaging table are centralized in `CommandTable.h`.
- The managed health model is useful: tracked wrappers update success/failure
  counters, last error, timestamps, and READY/DEGRADED/OFFLINE state.
- ESP-IDF example code is a native IDF implementation using `app_main`,
  `driver/i2c_master.h`, `esp_timer`, FreeRTOS delay, and fixed command buffers.
- Native Unity tests cover many lifecycle, health, recover, transport-mapping,
  calibration rollback, and register-helper paths.

## High-Severity Findings

### H1. DIAG_ALRT clear-on-read side effects are not protected

Severity: High

Evidence:
- `tick()` calls `isConversionReady()` at `src/INA228.cpp:282`.
- `isConversionReady()` reads `DIAG_ALRT` and keeps only `CNVRF` state at
  `src/INA228.cpp:687`.
- `_ensureMeasurementReadyForRead()` also uses this path at
  `src/INA228.cpp:1472`.
- Datasheet: `CNVRF` clears when `DIAG_ALRT` is read at
  `docs/pdf-extracted-md/INA228_datasheet.md:669`.

Impact:
- Polling for conversion readiness can clear latched alert flags before the
  application reads them.
- A completed triggered conversion can be stranded if another public DIAG read
  consumes CNVRF first.

Recommended remediation:
- Treat `DIAG_ALRT` as a destructive status register. Cache/return the full
  raw value whenever the driver reads it internally.
- Consider a private helper that handles CNVRF and alert state atomically.
- Document which APIs clear flags.

Suggested tests:
- Fake bus models CNVRF and alert clear-on-read.
- Trigger conversion, set CNVRF plus alert flag, call `tick()`, verify alert
  evidence remains observable.
- Trigger conversion, call public `readDiagAlertRaw()` before measurement read,
  verify the pending conversion does not stay stuck forever.

### H2. Alert configuration uses read-modify-write on a live status register

Severity: High

Evidence:
- `setAlertLatch()` starts at `src/INA228.cpp:976`.
- `setConversionReadyAlert()` starts at `src/INA228.cpp:993`.
- `setSlowAlert()` starts at `src/INA228.cpp:1010`.
- `setAlertPolarity()` starts at `src/INA228.cpp:1027`.
- These setters read and then write `DIAG_ALRT`, which also contains live flags.

Impact:
- Merely configuring alert behavior can clear or rewrite stale alert/status
  bits.
- Field diagnostics can lose root-cause evidence.

Recommended remediation:
- Keep a cached alert-config bitfield separate from live flags.
- Write only intended configuration bits, with reserved/status bits controlled
  according to datasheet requirements.
- Add explicit "read and clear" APIs for destructive status operations.

Suggested tests:
- Seed alert flags in fake bus, call each alert setter, assert flags are not
  accidentally consumed or rewritten contrary to the defined policy.

### H3. Triggered conversion state can be stale or stuck

Severity: High

Evidence:
- `begin()` accepts `Config::mode`, but `_trigPending` is initialized false and
  triggered mode is only tracked in `setMode()`/`triggerConversion()`
  (`src/INA228.cpp:145`, `713`, `746`).
- `tick(nowMs)` gates on its argument, then `isConversionReady()` gates again
  using `_nowMs()` (`src/INA228.cpp:282`, `687`, `_nowMs()` at `1543`).
- `Config.h` states that without `nowMs`, triggered helpers cannot advance from
  wall time.

Impact:
- `begin()` in a `TRIG_*` mode can return a driver that allows stale register
  reads instead of `MEASUREMENT_NOT_READY`.
- Without `Config::nowMs`, `tick(nowMs)` can receive an advanced timestamp but
  still never poll `DIAG_ALRT`.

Recommended remediation:
- Either require `Config::nowMs` for triggered mode or make `tick(nowMs)` pass
  the same timestamp into readiness checking.
- Track triggered mode when `begin()` writes a triggered `ADC_CONFIG`.
- Add a status-returning polling method or document `tick()` failure reporting.

Suggested tests:
- Begin with each `TRIG_*` mode and verify reads gate until ready.
- Use `tick(nowMs)` without `Config::nowMs` and verify completion behavior is
  either rejected or deterministic.

### H4. ENERGY and CHARGE are returned when the datasheet says they are invalid

Severity: High

Evidence:
- `readMeasurement()` reads energy at `src/INA228.cpp:489`.
- `readEnergy()` starts at `src/INA228.cpp:645`.
- `readCharge()` starts at `src/INA228.cpp:666`.
- Datasheet says ENERGY and CHARGE are invalid in triggered mode because the
  device does not track elapsed time:
  `docs/pdf-extracted-md/INA228_datasheet.md:673`.
- Header comments for `readEnergy()` and `readCharge()` mention continuous
  mode, but `readMeasurement()` does not.

Impact:
- Applications can log or act on invalid accumulation values after triggered or
  shutdown operation.

Recommended remediation:
- Reject energy/charge reads outside continuous modes, or return a status that
  marks accumulation invalid.
- Document accumulator validity for `Measurement`.

Suggested tests:
- In triggered and shutdown modes, `readEnergy()`, `readCharge()`, and
  `readMeasurement()` should fail or mark invalid according to the chosen API.

### H5. `setAdcRange()` can leave cache and hardware scaling incoherent

Severity: High

Evidence:
- `setAdcRange()` starts at `src/INA228.cpp:834`.
- It writes CONFIG with the new range, then calls `_applyCalibration()`.
- If calibration reapply fails, it restores only cached `_config.adcRange`,
  `_currentLsb`, and `_shuntCal`, not the device CONFIG register.
- VSHUNT scaling uses cached range in `src/INA228.cpp:461`; SHUNT_CAL depends
  on range at `src/INA228.cpp:111`.

Impact:
- A later shunt-voltage/current/power read can be scaled for the old range while
  hardware is operating in the new range. That can be a 4x error.

Recommended remediation:
- Compute/validate the new calibration before writing CONFIG.
- If any later write fails, attempt a rollback write and expose dirty hardware
  state if rollback fails.
- Add a cache/hardware dirty state or require `recover()`.

Suggested tests:
- Script failure on SHUNT_CAL write during `setAdcRange()` and assert hardware
  CONFIG, cache, current LSB, and health state are coherent.

### H6. Reset and accumulator reset semantics are incomplete

Severity: High

Evidence:
- `softReset()` writes reset and immediately reapplies config/calibration at
  `src/INA228.cpp:1151`.
- Datasheet documents reset equivalent to POR and startup timing; extracted
  docs show POR/startup context around `INA228_datasheet.md:341` and reset
  field around `1088`.
- `resetAccumulators()` writes `CONFIG_RSTACC` at `src/INA228.cpp:1179`.
- The implementation manual flags RSTACC behavior ambiguity around
  `INA228_power_monitor_implementation_manual.md:1239`.

Impact:
- Configuration may be reapplied before reset settles.
- RSTACC behavior may be latched or unclear without readback/clear policy.

Recommended remediation:
- Add bounded reset deadline/readback behavior using the configured time source
  or a caller-driven state.
- Define and test RSTACC clear/readback semantics.

Suggested tests:
- Fake reset busy/readback path.
- Verify no unbounded wait and no config writes before allowed reset interval.

### H7. High-voltage safety guidance is insufficient

Severity: High

Evidence:
- README leads with "85-V" (`README.md:3`) and lists `readBusVoltage` as
  `0-85 V` (`README.md:114`).
- There is no prominent user-facing warning about mains/non-isolated supplies,
  USB-ground hazards, creepage/clearance, fusing, shunt power, or qualified
  users.
- Internal manual has relevant electrical and layout notes around
  `INA228_power_monitor_implementation_manual.md:157`, `910`, `939`, and `1153`.

Impact:
- Users may treat an IC rating as a system safety approval.

Recommended remediation:
- Add README and Doxygen safety sections.
- Clearly state that the library and examples do not make 85 V systems safe.
- Add shunt power, Kelvin routing, input-filter, and protection guidance.

Suggested tests:
- Documentation review gate requiring safety section and example disclaimers.

### H8. Hardware validation and pure ESP-IDF readiness are not evidenced

Severity: High

Evidence:
- `library.json` declares `espidf` support.
- CI runs PlatformIO Arduino builds and native tests, plus static IDF contract
  checks, but no `idf.py` build job (`.github/workflows/ci.yml`).
- `docs/IDF_PORT.md` and `docs/IDF_PORT_IMPLEMENTATION.md` still state IDF
  builds/hardware validation are pending.
- `README.md:46` and `CHANGELOG.md:37` mention owner hardware coverage, but no
  dated hardware logs/matrix exist in the repo.

Impact:
- Release claims can exceed reproducible evidence.

Recommended remediation:
- Add `idf.py` build matrix for ESP32-S2/S3.
- Add a checked-in hardware validation matrix with commit/date/equipment/logs.
- Reword README/library claims until build and hardware evidence exist.

Suggested tests:
- CI: `idf.py -C examples/esp_idf/basic set-target esp32s3 build`.
- CI: `idf.py -C examples/esp_idf/basic set-target esp32s2 build`.

## Medium-Severity Findings

### M1. Transport failures are sometimes collapsed

Severity: Medium

Evidence:
- `begin()` maps manufacturer-ID read failure to `DEVICE_NOT_FOUND` at
  `src/INA228.cpp:229`.
- `probe()` similarly maps raw read failures to device-not-found style errors.

Impact:
- Supervisors cannot distinguish timeout, address NACK, data NACK, and bus
  error for retry/backoff policy.

Recommended remediation:
- Preserve original transport `Err` where possible or encode it consistently in
  `detail`.

### M2. Multi-register reads partially update caller output

Severity: Medium

Evidence:
- `readMeasurement()` starts writing fields immediately at `src/INA228.cpp:444`.
- `readRawSample()` follows the same style.

Impact:
- Callers can receive an error with partially refreshed data and no way to know
  which fields are fresh.

Recommended remediation:
- Fill local temporaries and assign to `out` only after all reads succeed, or
  document partial-output semantics explicitly.

### M3. Copy/move semantics are implicit

Severity: Medium

Evidence:
- `INA228` has default special members in `include/INA228/INA228.h:92`.
- It stores mutable state, health counters, pending trigger state, and callback
  context pointers starting near `include/INA228/INA228.h:470`.

Impact:
- Accidental copies can create two driver objects controlling one device with
  diverged health/cache state.

Recommended remediation:
- Delete copy/move constructors and assignment, or explicitly define and
  document supported semantics.

### M4. Overflow and MATHOF are not connected to measurement validity

Severity: Medium

Evidence:
- ENERGYOF and CHARGEOF flags clear when corresponding registers are read
  according to datasheet `INA228_datasheet.md:1322` and following.
- `readMeasurement()`, `readEnergy()`, and `readCharge()` read accumulators
  before any required overflow check.
- `MATHOF` is parsed in `readDiagAlert()` but not checked by current/power APIs.

Impact:
- Applications can miss accumulator overflow or math overflow evidence.

Recommended remediation:
- Define overflow policy: read diagnostics first, include flags in measurement
  result, or return dedicated overflow status.

### M5. Uncalibrated mode leaves hardware SHUNT_CAL nonzero/stale

Severity: Medium

Evidence:
- `Config::shuntResistanceOhm` says `0 = uncalibrated`.
- `_applyCalibration()` returns without writing `REG_SHUNT_CAL` when
  uncalibrated at `src/INA228.cpp:1515`.
- `SHUNT_CAL` reset is `0x1000` in `CommandTable.h`.

Impact:
- Converted APIs reject uncalibrated reads, but raw current/power/energy/charge
  registers can still reflect reset or stale hardware calibration.

Recommended remediation:
- If uncalibrated means invalid current path, explicitly write `SHUNT_CAL=0` or
  document the raw-register behavior.

### M6. Signed conversion relies on implementation-defined casts

Severity: Medium

Evidence:
- `_signExtend20()` and `_signExtend40()` are at `src/INA228.cpp:1572` and
  `1582`.

Impact:
- Current targets are two's-complement GCC, but portable C++ should avoid
  relying on unsigned-to-signed implementation details for out-of-range values.

Recommended remediation:
- Implement sign extension using arithmetic that does not rely on
  implementation-defined casts.

### M7. Alert threshold semantics are not tied to later scale changes

Severity: Medium

Evidence:
- Threshold setters encode shunt thresholds using current ADCRANGE and power
  threshold using current LSB.
- `setAdcRange()` and `setCalibration()` do not reapply or invalidate existing
  alert thresholds.

Impact:
- Previously configured thresholds can silently change physical meaning after
  range/calibration updates.

Recommended remediation:
- Cache threshold values in engineering units and reapply after scale changes,
  or mark threshold registers dirty/invalid.

### M8. ESP-IDF adapter and example are single-owner but not labeled that way

Severity: Medium

Evidence:
- IDF adapter uses global `gTransport` in
  `examples/esp_idf/basic/main/Ina228IdfI2cTransport.cpp:11`.
- It creates/deletes bus/device handles and reselects addresses without a lock.

Impact:
- The example is fine for a single `app_main` CLI but unsafe as production
  shared-bus/multitask code without external locking.

Recommended remediation:
- Label the IDF adapter as single-owner example glue.
- Provide a production adapter sketch showing external bus manager and mutex.

## Low-Severity Findings

### L1. ESP-IDF raw write help is less explicit than Arduino help

Severity: Low

Evidence:
- Arduino help marks `wreg16` as diagnostic-only and cache-desynchronizing.
- ESP-IDF help only says "Write 16-bit register".

Recommended remediation:
- Align ESP-IDF help text with Arduino and README warnings.

### L2. Public raw register write docs need stronger warnings

Severity: Low

Evidence:
- `writeRegister16()` is public in `include/INA228/INA228.h:365`.
- README warns raw writes are diagnostic, but public Doxygen comments are terse.

Recommended remediation:
- Document that raw writes can desynchronize cache, clear flags, or alter alert
  state.

## Calibration and Scaling Deep Dive

Current behavior:
- Users configure `shuntResistanceOhm` and `maxExpectedCurrentA`.
- Requested `CURRENT_LSB` is `maxExpectedCurrentA / 524288`.
- `SHUNT_CAL = 13107.2e6 * CURRENT_LSB * RSHUNT`, multiplied by 4 when
  ADCRANGE is `MV_40_96`.
- `SHUNT_CAL` values above `0x7FFF` are clamped and actual `currentLsb()` is
  recomputed from the clamped register.

Missing/weak pieces:
- The API stores requested `maxExpectedCurrentA` even after clamp, so config and
  actual full-scale behavior diverge.
- Normal exact formula vectors are missing.
- ADCRANGE=1 calibration multiplier is not directly tested.
- Negative current/charge and full-scale signed values are not tested.
- Extreme finite calibration values can lead to public `float` overflows.

Future test vectors:
- `R=0.0162 ohm`, `max=10 A`, ADCRANGE=0 -> `CURRENT_LSB=0.000019073486328125`, `SHUNT_CAL=0x0FD2`.
- Same inputs ADCRANGE=1 -> `SHUNT_CAL=0x3F48`.
- Datasheet-style nonzero scaling: `VSHUNT=0x4BF000`, `VBUS=0x3C0000`,
  `DIETEMP=0x0C80`, `CURRENT=0x4CCCC0`, `POWER=0x48000C`,
  `ENERGY=0x003F480000`, `CHARGE=0x0043800000`.
- Negative values: `VSHUNT=0xB41000`, `CURRENT=0x800000`,
  `CURRENT=0xFFFFF0`, `CHARGE=0xFFFFFFFFFF`, `CHARGE=0x8000000000`.
- ADCRANGE rollback failure with scripted SHUNT_CAL write failure.
- Calibration underflow and rounding boundary cases.

## Conversion Timing / Freshness Model

Current estimate:

```text
C_us = AVG * sum(enabled VSHUNT/VBUS/VTEMP conversion times)
       + convDelayMs2 * 2000
C_ms = ceil(C_us / 1000)
```

Default `CONT_ALL`, `1052 us` for all three channels, `AVG_1`, no delay:
`3156 us`, rounded to `4 ms`.

Maximum all-channel estimate with `4120 us`, `AVG_1024`, and `510 ms`
conversion delay: `13,166,640 us`, rounded to `13,167 ms`.

Important caveats:
- Public measurement APIs do not block for ADC conversion time. They perform I2C
  transactions only and return latest registers unless a tracked triggered
  conversion is pending.
- Continuous-mode reads have no freshness marker. The datasheet says output
  registers remain until replaced.
- `CONVDLY` is "initial ADC conversion" delay, so using it in steady-state
  continuous period estimates may be wrong or at least ambiguous.
- `tick(nowMs)` accepts a timestamp but readiness helpers use `_nowMs()`, so
  missing `Config::nowMs` can stall triggered completion.

Latency table:

| API | I2C transactions | Other waits | Worst-case bound | Notes |
| --- | ---: | --- | --- | --- |
| `begin()` | up to 7 | none | `7 * i2cTimeoutMs` | ID/MEMSTAT/config/calibration; no reset delay. |
| `recover()` | up to 7 | none | `7 * i2cTimeoutMs` | Revalidates IDs/MEMSTAT and reapplies config. |
| `tick()` | 0 or 1 | none | `i2cTimeoutMs` | Void API hides immediate Status. |
| `readMeasurement()` | 7 continuous, up to 8 after trigger | none | `8 * i2cTimeoutMs` | Can partially update output on error. |
| `readRawSample()` | 7 continuous, up to 8 after trigger | none | `8 * i2cTimeoutMs` | Same partial-output risk. |
| Single voltage/temp/current/power read | 1 continuous, up to 2 after trigger | none | `2 * i2cTimeoutMs` | Current/power also require calibration. |
| `readEnergy()` / `readCharge()` | 1 continuous, up to 2 after trigger | none | `2 * i2cTimeoutMs` | Accumulation validity not guarded. |
| `isConversionReady()` | 0 before software deadline, else 1 | none | `i2cTimeoutMs` | Reads destructive `DIAG_ALRT`. |
| Config setters | usually 1 | none | `i2cTimeoutMs` | Alert setters are 2 transactions. |
| `setAdcRange()` | up to 2 | none | `2 * i2cTimeoutMs` | Partial-state risk. |
| `softReset()` | up to 5 | none | `5 * i2cTimeoutMs` | Missing POR wait/readback. |
| `end()` | up to 1 raw write | none | `i2cTimeoutMs` | Best effort, no health tracking. |

## Alert / Diagnostic Model

The driver parses diagnostic flags and exposes raw and structured reads, which
is good. The model is not safe enough for production because it does not
separate:

- Alert configuration bits (`ALATCH`, `CNVR`, `SLOWALERT`, `APOL`)
- Live status bits (`ENERGYOF`, `CHARGEOF`, `MATHOF`, threshold flags, `CNVRF`)
- Clear-on-read side effects

Required direction:
- Cache alert config bits and write config without consuming live flags.
- Make destructive reads explicit.
- Preserve full raw DIAG snapshots when internally reading CNVRF.
- Add fake-bus clear-on-read behavior and tests for all flags and setters.

## Partial-State / Cache Consistency Assessment

The driver already avoids committing many cached config values until writes
succeed, which is good. Gaps remain:

- `setAdcRange()` can commit hardware CONFIG and roll back only software cache.
- `_applyConfig()` writes CONFIG, ADC_CONFIG, optional SHUNT_TEMPCO, and DIAG
  config bits across multiple operations with no dirty-state marker.
- `_applyCalibration()` may fail after some previous config writes succeeded.
- `softReset()` immediately reapplies config/calibration without modeling reset
  readiness.
- Multi-register reads write caller output incrementally.

Recommended model:
- For multi-step writes, compute all new derived values first.
- Write in an order that minimizes unsafe partial state.
- On partial failure, either roll back hardware or expose a dirty/unknown state.
- Add `resync()`/`recover()` semantics that explicitly read back or reapply all
  cached state.

## ESP-IDF / Arduino / PlatformIO Assessment

Arduino / PlatformIO:
- `platformio.ini` defines ESP32-S3 and ESP32-S2 Arduino environments.
- CI builds those environments on Ubuntu.
- Local audit results: both `python -m platformio run -e esp32s3dev` and
  `python -m platformio run -e esp32s2dev` failed in this environment while
  compiling framework objects; no useful source-level diagnostic was captured.
- Therefore this audit cannot claim local Arduino ESP32-S2/S3 build readiness.

ESP-IDF:
- Root `CMakeLists.txt` registers the core component.
- `idf_component.yml` declares ESP32-S2/S3 and IDF dependency.
- Native example uses IDF APIs and avoids Arduino facades.
- Static contract check passed.
- CI does not build the IDF example with `idf.py`.
- Local `idf.py` is not installed/on PATH, so no IDF build was run.
- IDF adapter is example-grade single-owner glue, not a production shared-bus
  manager.

## Tests and CI Assessment

Run locally:

| Command | Result |
| --- | --- |
| `bash -lc "...startup..."` | Failed because WSL has no installed distributions. Repeated with PowerShell equivalents. |
| `git branch --show-current` | `audit/ina228-industry-readiness-exploration` after branch creation. |
| `git status --short` | Clean before report edits; generated package tarball was removed. |
| `python --version` | `Python 3.13.12` |
| `pio --version` | Failed: `pio` not recognized. |
| `python -m platformio --version` | `PlatformIO Core, version 6.1.19` |
| `python tools/check_core_timing_guard.py` | `Core timing guard PASSED` |
| `python tools/check_cli_contract.py` | `CLI contract PASSED` |
| `python tools/check_idf_example_contract.py` | `IDF example contract PASSED` |
| `python scripts/generate_version.py check` | `Up to date: ... include/INA228/Version.h` |
| `python -m platformio test -e native` | Passed, 48 test cases succeeded in 5.946 s. |
| `python -m platformio run -e esp32s3dev` | Failed, 1 failed in 35.09 s; verbose retry failed in 37.23 s. Captured diagnostics only showed object build `Error 1` in framework/source compilation, not a useful source error. |
| `python -m platformio run -e esp32s2dev` | Failed, 1 failed in 28.866 s; captured diagnostic ended at `FrameworkArduino/wiring_shift.c.o Error 1`. |
| `python -m platformio pkg pack` | Succeeded and wrote `INA228-1.3.0.tar.gz`; tarball removed to keep audit changes report-only. |
| `idf.py --version` | Failed: `idf.py` not recognized. |

Present in CI:
- PlatformIO Arduino ESP32-S3/S2 builds.
- Native PlatformIO Unity tests.
- Core timing guard.
- CLI contract checks.
- Generated version check.
- PlatformIO package validation.

Missing from CI:
- Pure ESP-IDF `idf.py` builds for ESP32-S2/S3.
- Sanitizer/coverage jobs.
- Windows/macOS native builds.
- Hardware-in-loop or attached validation artifacts.

Missing native test coverage:
- Negative signed VSHUNT/CURRENT/CHARGE and negative temperature.
- Exact nonzero datasheet scaling vectors.
- ADCRANGE=1 calibration multiplier.
- `setAdcRange()` partial failure rollback.
- DIAG_ALRT clear-on-read behavior.
- Alert setters and threshold scale invalidation.
- Triggered mode started by `begin()`.
- `tick()` behavior with and without `Config::nowMs`.
- Copy/move prevention.
- Reset/RSTACC behavior.

## Hardware Validation Matrix

No hardware validation was performed in this audit session. No dated,
commit-linked hardware validation matrix is present in the repo.

Required matrix before production/field claim:

| Area | Required validation |
| --- | --- |
| Address scan | Probe 0x40-0x4F with empty slots, non-INA devices, and one/multiple INA228 devices. |
| Identity | Read `MANUFACTURER_ID=0x5449`, `DEVICE_ID=0x2281`, and MEMSTAT on every supported board/framework. |
| Bus voltage | Known-source 0 V, mid-range, high-range below system safety limit; compare against DMM. |
| Shunt voltage | Known differential source/load across ADCRANGE 0 and 1. |
| Current calibration | Known shunt/load; verify CURRENT_LSB, SHUNT_CAL, current, power. |
| Negative/bidirectional current | Validate negative shunt/current/charge if hardware supports bidirectional flow. |
| Conversion timing | Measure CNVRF timing for all conversion times, averaging extremes, CONVDLY. |
| Continuous mode | Verify freshness/data age, no unexpected blocking, long-run stability. |
| Triggered mode | Verify one-shot completion, stale protection, `tick()`, and `nowMs` behavior. |
| Energy/charge | Continuous-mode accumulation over known time/load; overflow/reset behavior. |
| Alerts | Thresholds, latch/transparent, polarity, CNVR alert, slow alert, pin electrical behavior. |
| Fault injection | NACK, unplug/replug, timeout, bus stuck, wrong ID, MEMSTAT fault if simulatable. |
| Reset/brownout | Device reset while MCU stays up; MCU reset while INA228 keeps running; recovery/resync. |
| Soak | 24-72 hour continuous read/stress with health counters and error logs. |
| Frameworks | Arduino ESP32-S2/S3 and pure ESP-IDF ESP32-S2/S3, same commit and hardware. |

## Recommended Implementation Plan

### P0 - Must fix before production claim

1. Rework `DIAG_ALRT` handling so internal CNVRF polling and alert setters do
   not destroy live diagnostic evidence.
2. Fix triggered-mode state: begin-time triggered modes, `tick(nowMs)` vs
   `Config::nowMs`, and CNVRF consumption by public diagnostic reads.
3. Guard ENERGY/CHARGE validity by mode and overflow status.
4. Fix `setAdcRange()` partial-state behavior.
5. Define reset/RSTACC bounded behavior and tests.
6. Add high-voltage safety docs and an explicit hardware validation matrix.
7. Add exact calibration/scaling/signedness test vectors.

### P1 - Should fix before release/merge

1. Preserve transport error specificity in begin/probe.
2. Delete or define copy/move semantics.
3. Make multi-register output assignment all-or-nothing or document partial
   output semantics.
4. Clarify and test uncalibrated SHUNT_CAL hardware behavior.
5. Add threshold invalidation/reapply policy for ADCRANGE/calibration changes.
6. Add pure ESP-IDF CI builds.
7. Reword README/library claims to distinguish implemented, CI-built,
   hardware-validated, and pending behavior.

### P2 - Nice hardening / later

1. Coverage/sanitizer builds.
2. Optional fixed-point or raw-plus-scale APIs for high precision.
3. Production IDF adapter pattern with external bus manager and lock.
4. Doxygen hardware/calibration/timing pages.
5. Richer diagnostic snapshots including last raw DIAG_ALRT.

## Suggested Future Implementation Branch

```text
hardening/ina228-industry-readiness
```

## Proposed Implementation Prompt Chunks

1. Core architecture/status cleanup: copy/move semantics, error preservation,
   all-or-nothing read outputs.
2. DIAG_ALRT, alerts, and diagnostic side-effect safety.
3. Triggered conversion, freshness, timing, and `tick(nowMs)` semantics.
4. Calibration, ADCRANGE atomicity, scaling vectors, and numeric safety.
5. Energy/charge validity, overflow handling, reset/RSTACC behavior.
6. Native fake-bus fault injection expansion and exact datasheet tests.
7. ESP-IDF/Arduino example labeling and CI build matrix.
8. Documentation: safety, calibration, timing, hardware validation matrix.

## Final Verdict

Implementation should proceed immediately. The repository has enough local
datasheet material and a solid architecture to harden without waiting for more
documents. It should not be marketed or tagged as production/industry-grade
until P0 correctness work, pure ESP-IDF build proof, Arduino build proof in the
current environment/CI, and a dated hardware validation matrix are complete.
