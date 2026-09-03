# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

- Triggered completion no longer rewrites cached MODE to shutdown: MODE mirrors
  `ADC_CONFIG`, while a separate flag records whether the one-shot is pending.
  Owners must select shutdown or a continuous mode before starting an
  instantaneous sample after a triggered conversion. A failed destructive DIAG
  read during verification now clears uncertain trigger timing without
  invalidating otherwise verified configuration.
- Instantaneous samples include a bounded whole-device timing margin before the
  single CNVRF check, with no new transfers or retries.
- Read-only configuration verification preserves synchronized hardware and the
  accumulator epoch after inconclusive transport errors, while identity,
  presence, revision, MEMSTAT, and register mismatches still require verified
  reinitialization.
- MATHOF errors and public Doxygen consistently describe the latch, its
  `MATH_OVERFLOW` result, and which DIAG_ALRT flags a destructive read does and
  does not clear.
- Hardware APIs distinguish an active cooperative owner from an unconsumed
  terminal result in `BUSY` diagnostics.
- Conversion and reset waits are no longer skipped when the monotonic clock
  advances during the blocking register write that arms them. The wait origin is
  sampled after the write returns, so it can be newer than the timestamp the
  caller sampled before `pollJob()`; the unsigned comparison underflowed and
  consumed the whole wait, letting an instantaneous-sample job read all five
  channels before the conversion completed and commit them as verified.
- Software and raw `CONFIG.RST` resets mark all reset-restored registers and
  alert thresholds dirty before an ambiguous write can return. Invalidation
  preserves the first dirty cause and register set; MEMSTAT sample failures,
  failed threshold writes, rebinding, and uncalibrated ADC-range changes now
  maintain their documented dirty/calibration snapshots.
- Examples: an operation-deadline timeout no longer wedges the CLI permanently in
  `BUSY`; `mode 1`–`mode 7` report `IN_PROGRESS` as acceptance rather than
  failure; the ESP-IDF poll loop no longer busy-spins at the default FreeRTOS
  tick rate; the Arduino transport re-probes the address on a short read so an
  absent device is reported as an address NACK instead of a generic I2C error;
  and a raw `wreg16` write invalidates cached hardware state.
- Example limit queries consistently warn when engineering-unit thresholds need
  reapplication, and verbose stress mode reports failures on both platforms.
  Arduino counts short-read probes and partial-buffer discard transactions as
  real bus traffic. ESP-IDF retains failed temporary-device removals for bounded
  cleanup retry instead of leaking handles when a transfer also fails.
- Tooling wires `--require-framed` through `run_step`, ignores historical
  counters in any driver-health block while retaining live consecutive failures,
  emits report-induced NOT RUN rows in dry-run plans, and permits parser tests
  and dry runs from source exports. Static guards scan every native-IDF source,
  balance nested CMake calls, check owner guards per command, and detect quoted
  framework includes in core code.
- Importing `scripts/generate_version.py` from ordinary Python is read-only;
  automatic synchronization is limited to PlatformIO pre-build use or explicit
  commands. Internal audit records are excluded from exported packages and
  guarded by CI.

### Removed

- Retired `hilmark` CLI commands and the runner's unused legacy-marker mode.
- Generated per-run hardware reports under `docs/validation/hardware/`, which
  described an unreconstructable dirty worktree based on a reachable commit.
  `docs/validation/hardware-evidence.md` retains the durable summary of each run.
- `tools/INA228_HIL_COMMAND_SEQUENCE.md`; its unique transfer-budget command
  sequences moved into `docs/validation/hardware-validation-procedure.md`.

## [3.0.3] - 2026-08-04

### Changed

- Consolidated duplicated conversion-wait, ADC-configuration, alert-bit,
  threshold-encoding, calibration-planning, measurement-read preparation,
  job-invalidation, owner-step, and CLI stress-summary paths. Removed orphaned
  private state, helpers, constants, includes, and redundant lifecycle
  invalidations. Public API, transfer bounds, device wait durations, and retry
  behavior are unchanged.
- Clarified triggered-timestamp ownership, idle `tick()` behavior, accumulator
  evidence lifecycle, and the current release installation pin in user and API
  documentation.
- Kept the Arduino and native ESP-IDF diagnostic CLIs on one checked command
  and alias contract, made raw writes require explicit confirmation, and
  documented command parsing, destructive reads, framing, verdicts, and
  firmware provenance in one CLI/HIL reference.
- Tightened public API Doxygen coverage with extraction limited to documented
  interfaces and warnings treated as errors; added focused READMEs for both
  diagnostic CLI examples.
- Migrated GitHub-owned CI actions to their Node.js 24 runtime majors after the
  v3.0.3 release-candidate run exposed Node.js 20 deprecation warnings.

### Fixed

- Arduino and native ESP-IDF CLI status output no longer repeats a successful
  `OK` as a yellow warning-style `Message`; non-OK diagnostic messages remain.
- HIL reports now exclude explicit `NOT RUN` rows from `Commands executed`
  while retaining them in the detailed and verdict counts.
- `tick()` is now bus-silent when no driver-tracked conversion is pending; it
  no longer performs non-advancing, status-clearing `DIAG_ALRT` reads merely
  because the accumulator epoch is invalid.
- Hookless synchronous recovery and accumulator-reset conveniences no longer
  fabricate timestamp zero or consume an unrelated deferred trigger origin.
- A verified accumulator reset clears obsolete conversion-ready and
  accumulator-overflow bits from the latest diagnostic snapshot while keeping
  sticky diagnostic event history intact.
- Legacy calibration planning now rejects finite current-LSB values that do
  not fit the public nanoampere fields before any narrowing conversion or I2C.
- Failed or ambiguous ADC-configuration writes and raw conversion-invalidating
  writes now discard pending trigger timing, so an older deadline cannot be
  trusted after hardware state becomes uncertain.
- HIL soak report generation no longer references a removed summary field;
  the parser self-test now exercises report construction as well as parsing.
- CLI integer, floating-point, boolean, and enum arguments now reject malformed,
  overflowed, negative-unsigned, and non-finite input instead of accepting
  partial strings or silently converting junk to zero. Local command rejection
  now produces an explicit `INVALID_PARAM` HIL status.
- Expected-invalid HIL cases no longer hide unrelated transport failure text,
  and maximum-consecutive-failure accounting now resets on every non-failure
  verdict, including unstored soak passes. Framed runs use only the matching
  payload to satisfy expectations and provenance, while stale or trailing
  failure text cannot produce a pass. They reject mismatched library versions,
  exact 12-character commit identities, source status, and profile-specific
  framework versions.
- Native ESP-IDF argument splitting rejects overlong tokens instead of silently
  truncating them, and its generated commit/status provenance refreshes before
  every incremental build.

### Validation

- Added native fake-bus regressions for idle-tick bus silence, hookless
  synchronous jobs with a deferred trigger origin, and accumulator-reset
  diagnostic cleanup, ambiguous trigger invalidation, legacy range-boundary
  planning, and failed calibration/alert writes preserving committed cache
  state while marking the affected hardware registers dirty.
- Added standalone HIL parser regressions and strengthened CLI/ESP-IDF static
  contracts to cover every published alias, strict parsing, raw-write
  confirmation, exact cross-framework help parity, and embedded provenance.
  The final local software evidence is recorded in
  `docs/validation/validation-status.md`. A clean-commit targeted ESP32-S3 HIL
  run and a dirty-worktree smoke confirmation of the CLI-output fix are
  also recorded there as local, partial evidence; release-grade hardware
  validation is not claimed.

## [3.0.2] - 2026-08-03

### Changed

- Valid triggered `Config::mode` values can now be initialized and verified;
  the cooperative instantaneous-sample job continues to require a shutdown or
  continuous active base mode because it owns its one-shot trigger.

### Fixed

- Cooperative sample, reset, and configured-trigger time origins are now
  established only after the successful blocking register write completes.
  With `Config::nowMs`, the driver samples the post-write time immediately.
- Without the optional timing hook, the next explicit caller timestamp
  establishes the origin and the full device wait follows. This adds one
  bus-silent owner activation instead of reusing a pre-write timestamp.
- Direct trigger and triggered-mode setter paths share the same post-write
  timing behavior, so hookless applications no longer fabricate timestamp zero
  as a conversion origin.
- Device wait durations, transfer budgets, retry behavior, and public APIs are
  unchanged.

### Validation

- Native fake-bus regressions cover exact post-write boundaries for sample,
  reset, configured-trigger initialization/reinitialization, hookless anchoring,
  `uint32_t` wraparound, failed/ambiguous writes, and cancellation/timeout
  lifetime. Software gate results are recorded in the validation status; no new
  hardware validation is claimed.

## [3.0.1] - 2026-07-31

### Changed

- Pinned Arduino builds to PIOArduino `55.03.311` (Arduino-ESP32 `3.3.11`,
  ESP-IDF `5.5.5`) with PlatformIO Core `6.1.19`, and declared the tested
  ESP32-S3 QIO/QSPI PSRAM layout.
- Updated the ESP32-S2 post-upload reset spelling for esptool 5 and removed the
  obsolete classic-ESP32 PSRAM cache workaround from the ESP32-S3 flags.
- Added Arduino core, ESP-IDF, MCU, flash, and PSRAM details to the diagnostic
  CLI version output.
- Reconciled the HIL runner with the v3 cooperative-owner contract and restored
  comprehensive mode, timing, averaging, alert, raw-width, validation, job,
  transfer-budget, and stress coverage to the exhaustive suite.
- Consolidated release HIL guidance on one framed exhaustive suite, tightened
  report taxonomy and evidence claims, and completed installation, package,
  Doxygen, and release-checklist documentation.
- Removed completed implementation-audit and migration-review documents after
  preserving their durable release and validation conclusions here.
- Simplified release metadata generation so `library.json` synchronizes only
  this library's `Version.h`, ESP-IDF component version, and Doxygen version.
- Removed unused example helper layers, no-op transport/address helpers,
  duplicate bus recovery, dead logging levels, and stale HIL retry/parser state.
- Pinned CI to PlatformIO Core `6.1.19` and added Doxygen plus standalone
  exported-package compile checks.

### Fixed

- Maximum-current-derived calibration now divides by the signed 20-bit positive
  limit and rounds `SHUNT_CAL` upward. This prevents register quantization from
  reducing the representable current below the requested maximum; the documented
  15 mOhm/10 A example profile is covered by a native regression test.

### Validation

- On 2026-07-31, native tests passed 101/101 and Arduino ESP32-S2/S3 builds
  passed with PIOArduino `55.03.311`, GCC `14.2.0`, and esptool `5.3.0`.
- A low-voltage ESP32-S3/INA228 run from the dirty migration worktree passed
  851 executable exhaustive/benchmark checks with zero FAIL/UNKNOWN results,
  including 1,000 measurement samples and 1,000 mixed operations. A separate
  60-second shakedown passed 5,932 soak commands with zero FAIL/UNKNOWN results.
  This is implementation evidence, not clean-commit release qualification.
- The cleanup worktree passed 101/101 native tests, exhaustive cppcheck,
  warnings-as-errors Doxygen generation, standalone exported-package compile,
  both Arduino target builds, and a fresh 851-check framed S3 exhaustive HIL
  with zero FAIL/UNKNOWN results. The HIL report was temporary, so it is not
  checked-in release evidence.

## [3.0.0] - 2026-07-19

This breaking release adds a bounded, fixed-memory cooperative operation model
for external I2C-owner tasks. The application remains responsible for the bus,
serialization, transfer timeouts, operation deadlines, retries, health policy,
and recovery.

### Added

- Zero-I2C `bind()` and typed cooperative starts for initialization,
  reinitialization, configuration verification, instantaneous sampling,
  software reset, and accumulator reset.
- `pollJob(nowMs, maxTransfers)` with a hard per-call transport-callback budget,
  valid zero-budget/bus-silent wait advancement, and no driver retry.
- Cache-only job snapshots, nonzero operation IDs, caller request tokens, and
  exactly-once terminal result delivery with stale-result rejection.
- Bus-silent, idempotent cancellation and timeout with explicit
  `NONE`, `CONFIRMED`, `PARTIAL`, and `INDETERMINATE` hardware-effect reporting.
- `HardwareState` and explicit invalidation so desired cache is not presented as
  synchronized hardware until full readback verification succeeds.
- Fixed-unit calibration modes for maximum-current-derived and explicit current
  LSB planning, including quantization, clamping, current-register range, and
  shunt-voltage range results. Unsafe plans require explicit opt-in.
- Atomic triggered instantaneous samples with request/operation provenance,
  configuration generation, channel validity, raw and fixed-unit values, and
  correlated diagnostic evidence.
- Fixed-memory diagnostic event lifecycle with new/sticky evidence,
  first-observed timestamps, and bus-silent acknowledgement.
- Pure DEVICE_ID parsing that separates DIEID `0x228` from the revision nibble,
  plus configurable supported-revision policy.
- Exact job limits for operation class, maximum transfers, additional wait, and
  retry count.
- Atomic identity publication: initialization/reinitialization keeps parsed
  identity in job-local scratch, and failed/cancelled reconciliation or active
  invalidation revokes previously synchronized identity state.
- Static owner-contract guard and CI coverage for the cooperative API, example
  usage, framework neutrality, passive health, and effective C++17 flags.

### Changed

- `HealthPolicy::PASSIVE` is the default so diagnostic health never takes
  scheduling or recovery authority from an external owner. The legacy latched
  OFFLINE policy remains explicit opt-in behavior.
- Initialization writes deterministic alert configuration and verifies
  identity, revision, MEMSTAT, configuration, ADC profile, calibration,
  temperature coefficient, and writable DIAG_ALRT state.
- Software reset now has an owner-visible zero-I2C startup wait and a verified
  full initialization sequence.
- Accumulator validity is generation/epoch based. Range, calibration,
  mode/timing, triggered operation, temperature compensation, and reset changes
  invalidate energy/charge until a verified accumulator reset establishes a
  coherent epoch.
- Arduino and native ESP-IDF examples use the cooperative lifecycle for
  initialization, fixed-unit sampling, reset, and reinitialization.
- Arduino builds now remove the framework's GNU++11 flag and apply GNU++17
  effectively; native code remains C++17.
- Documentation now separates owner-safe production operations, pure helpers,
  synchronous convenience APIs, diagnostics, implemented behavior, build/native
  evidence, historical HIL, and external hardware-validation gates.

### Compatibility

- The release is major because configuration gained fixed-unit calibration,
  deterministic alerts, revision, and health-policy contracts, and the
  recommended lifecycle changes from synchronous `begin()`/`recover()` to
  bind/start/poll/take.
- Existing synchronous typed functionality remains available as bounded
  convenience/diagnostic APIs. Legacy stepped helper names remain for source
  compatibility but are not the recommended external-owner path.
- No TunnelMonitor-node source or dependency pin is changed by this release.
  Product calibration choice, private adapter integration, immutable exact pin,
  and HIL remain external gates.

### Validation

- Native fake-bus tests inject failures at every distinct cooperative transfer
  phase, including verification, reset, and accumulator reset; they also cover
  zero-I2C waits, exact budgets, cancellation/timeout, ambiguous writes,
  stale/exactly-once results, revision policy, deterministic alerts, calibration
  boundaries, atomic reinitialization identity, retained synchronous wrappers,
  diagnostic lifecycle, accumulator epochs, passive health, and explicit
  legacy offline policy.
- CI is configured for native tests, Arduino ESP32-S2/S3 builds, native ESP-IDF
  ESP32-S2/S3 builds, static contract guards, version synchronization, and
  package validation.
- Historical v2 low-voltage HIL evidence is not v3 validation. Version 3 has no
  release-grade hardware-validation claim until the documented hardware gates
  are completed on an exact clean commit.

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

[Unreleased]: https://github.com/janhavelka/INA228/compare/v3.0.3...HEAD
[3.0.3]: https://github.com/janhavelka/INA228/compare/v3.0.2...v3.0.3
[3.0.2]: https://github.com/janhavelka/INA228/compare/v3.0.1...v3.0.2
[3.0.1]: https://github.com/janhavelka/INA228/compare/v3.0.0...v3.0.1
[3.0.0]: https://github.com/janhavelka/INA228/compare/v2.0.0...v3.0.0
[2.0.0]: https://github.com/janhavelka/INA228/compare/v1.3.0...v2.0.0
[1.3.0]: https://github.com/janhavelka/INA228/compare/v1.2.0...v1.3.0
[1.2.0]: https://github.com/janhavelka/INA228/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/janhavelka/INA228/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/janhavelka/INA228/releases/tag/v1.0.0
