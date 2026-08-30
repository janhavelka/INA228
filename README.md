# INA228

Framework-neutral C++17 driver for the Texas Instruments INA228 power monitor.
Version 3 introduces one bounded cooperative operation model for applications
whose I2C bus is owned by an external scheduler or task.

The library does not create, configure, lock, retry, or recover an I2C bus. The
application injects bounded transport callbacks and retains authority over pins,
bus handles, serialization, transfer timeouts, operation deadlines, retries,
health policy, and recovery.

> Safety: INA228 measurements and ALERT output are monitoring signals, not
> certified safety functions. This library and its examples do not make an
> 85 V system safe. Use appropriate isolation, fusing, creepage/clearance,
> shunt-power analysis, independent protection, and qualified design review.

## Supported environments

- ESP32-S2 and ESP32-S3
- Arduino/PlatformIO
- Native ESP-IDF component use; `idf_component.yml` declares `idf >= 6.0.0`
- Host/native tests
- C++17 or GNU++17

Core files under `include/INA228/` and `src/` contain no Arduino, ESP-IDF,
FreeRTOS, logging, platform delay, or bus-handle dependencies.

## Installation

For a PlatformIO application, pin the current release tag in `platformio.ini`:

```ini
lib_deps =
  https://github.com/janhavelka/INA228.git#v3.0.3
build_unflags =
  -std=gnu++11
build_flags =
  -std=gnu++17
```

If the application already selects C++17 or newer effectively, keep its
existing language-standard settings instead. The INA228 public headers and
implementation require C++17.

Then include the public header:

```cpp
#include <INA228/INA228.h>
```

For native ESP-IDF, place the tagged repository under the application's
`components/INA228/` directory. The root `CMakeLists.txt` registers the library
as an ESP-IDF component and requires C++17. The application still supplies its
own `driver/i2c_master.h` transport; see the native example and
[ESP-IDF integration guide](docs/integration/esp-idf.md).

Release tags are appropriate for normal consumers. Integrators testing an
unreleased fix should pin an exact reviewed commit rather than tracking a
branch.

## Recommended cooperative owner API

The external owner first calls `bind()`, which validates and stores the desired
configuration without touching I2C. It then starts one operation and advances it
with `pollJob(nowMs, maxTransfers)`.

The following is an integration fragment: `appI2cWrite`, `appI2cWriteRead`,
`busContext`, `requestToken`, and `monotonicNowMs` are application-owned values,
not library globals.

```cpp
INA228::INA228 monitor;
INA228::Config config;
config.i2cWrite = appI2cWrite;
config.i2cWriteRead = appI2cWriteRead;
config.i2cUser = &busContext;
config.i2cAddress = 0x41;
config.i2cTimeoutMs = 20;  // bound for each transport callback
config.mode = INA228::Mode::CONT_ALL;
config.healthPolicy = INA228::HealthPolicy::PASSIVE;

config.calibration.shuntMicroOhms = 25000;
config.calibration.mode = INA228::CalibrationMode::FROM_MAXIMUM_CURRENT;
config.calibration.maxCurrentMilliAmps = 2500;

INA228::Status status = monitor.bind(config);  // zero I2C
uint32_t operationId = 0;
if (status.ok()) {
  status = monitor.startInitialize(requestToken, operationId);  // zero I2C
}

// Repeat this block on successive owner activations until the job terminates
// or the application-owned deadline expires. One call cannot finish init.
// A budget of one permits at most one transport callback per activation.
if (status.ok() || status.inProgress()) {
  status = monitor.pollJob(monotonicNowMs, 1);
}

INA228::JobSnapshot snapshot;
monitor.getJobState(snapshot);  // cache-only
if (snapshot.state != INA228::JobState::ACTIVE && snapshot.resultAvailable) {
  INA228::JobResult result;
  INA228::Status take = monitor.takeJobResult(operationId, result); // zero I2C
  if (take.ok()) {
    // take success means delivery succeeded. Inspect result.job.status/state/effect.
  }
}
```

Only one cooperative job or unconsumed terminal result exists per instance.
Every successful start returns a nonzero `operationId` and retains the caller's
`requestToken`. `takeJobResult()` checks the operation ID and delivers the
terminal result exactly once. A mismatched ID returns `STALE_RESULT`; a second
take returns `RESULT_NOT_AVAILABLE`. Hardware APIs return `BUSY` throughout the
active job and unconsumed-terminal-result window; cache-only inspection,
result take, `end()`, and explicit invalidation remain available.

### Job kinds and declared bounds

`getJobLimits()` reports the exact bound for the bound profile. All driver retry
counts are zero.

| Job | Class | Maximum transfers | Additional wait |
|---|---|---:|---:|
| `INITIALIZE` | multi-step runtime | 14 | 0 |
| `REINITIALIZE` | multi-step runtime | 14 | 0 |
| `VERIFY_CONFIGURATION` | multi-step runtime | 8 | 0 |
| `INSTANTANEOUS_SAMPLE` | steady-state sample | 11 | derived conversion time plus wake/delay |
| `RESET` | maintenance | 16 | reset startup |
| `ACCUMULATOR_RESET` | multi-step runtime | 2 | 0 |

The transfer count is a maximum, not a requirement. Failure can terminate a job
earlier. `pollJob(..., 0)` is valid and bus-silent; it can advance an elapsed
time gate but cannot call the transport. A positive budget is a maximum for that
call. The library performs no automatic retry.

`Config::nowMs` and explicit timestamps supplied to cooperative/readiness APIs
must use the same wrap-safe monotonic millisecond domain. A successful trigger,
reset, or configured-trigger write starts its software wait only after the
blocking callback returns. With `Config::nowMs`, the driver samples that
post-write time immediately. Without the optional hook, the next explicit
caller timestamp establishes the origin in one bus-silent activation, and the
full unchanged device wait follows. Hookless `isConversionReady()` cannot
advance an unresolved origin; use `pollConversionReady(nowMs, ...)` or
`tick(nowMs)`.

Internal synchronous conveniences never treat a missing timing hook's zero
fallback as an explicit caller timestamp, so they do not consume an unrelated
deferred trigger origin. `tick(nowMs)` is bus-silent when no driver-tracked
trigger is pending; a verified accumulator reset establishes its new epoch
directly and does not require background diagnostic polling.

The caller owns the operation deadline. `timeoutJob()` and `cancelJob()` are
idempotent and bus-silent. Their terminal `JobEffect` distinguishes:

- `NONE`: no unresolved hardware mutation or evidence loss; any successful
  destructive status read was captured in the diagnostic cache;
- `CONFIRMED`: the desired effect was verified;
- `PARTIAL`: an earlier write succeeded but final verification did not;
- `INDETERMINATE`: a failed side-effecting callback may still have changed
  hardware or consumed destructive diagnostic evidence.

After a partial or indeterminate configuration effect, hardware state is
`RESYNC_REQUIRED`. Call `invalidateHardwareState()` after removal, bus recovery,
suspected device reset, or any external raw mutation, then run reinitialization
or verification as appropriate. Cache state is not described as synchronized
until readback succeeds.

### Atomic instantaneous sample

`startInstantaneousSample()` is accepted only after successful calibration and
verified hardware synchronization. `Config::mode` may select any valid INA228
mode for initialization, including a triggered mode. The instantaneous-sample
job itself still requires the active configured base mode to be shutdown or
continuous because that job owns its one-shot transition.

The job verifies critical configuration, triggers all channels, waits without
I2C, captures `DIAG_ALRT`, reads the five instantaneous channels while the
device is shut down, restores the configured mode, and verifies the restore.
Only the completed `InstantaneousSample` is published. It carries operation ID,
request token, configuration generation, channel-validity bits, raw values,
fixed-unit values, capture time, and diagnostic evidence. If a sample fails
after `DIAG_ALRT` was captured, the correlated terminal result still retains
that diagnostic evidence; no partial measurement is marked successful.

## Calibration contract

Prefer the fixed-unit `CalibrationConfig`:

- `NONE`: voltage and temperature only;
- `FROM_MAXIMUM_CURRENT`: choose the smallest representable current LSB for the
  declared maximum current;
- `EXPLICIT_CURRENT_LSB`: use the declared nanoampere-per-bit design value.

`shuntMicroOhms`, `maxCurrentMilliAmps`, `currentLsbNanoAmps`, `ADCRANGE`, and
`SHUNT_CAL` are planned as one contract. `calculateCalibration()` is pure and
reports the selected/effective LSB, register value, quantization, clamping,
current-register range, and shunt-voltage range. Unsafe plans fail unless the
caller explicitly sets `allowUnsafePlan`.

Current, power, energy, charge, and power-threshold conversions are unavailable
without valid calibration. Changing range, calibration, mode/timing, triggered
operation, temperature-compensation state, or resetting the device invalidates
the accumulator epoch until the required reset/verified sequence establishes a
new coherent scale. A verified accumulator reset also removes obsolete
conversion-ready and accumulator-overflow bits from the latest diagnostic
snapshot. Sticky diagnostic event history remains available until explicitly
acknowledged.

## Identity and diagnostics

Initialization requires manufacturer ID `0x5449`, parses DEVICE_ID DIEID bits
15:4 as `0x228`, and treats bits 3:0 as a separate revision. Revision acceptance
is controlled by `supportedRevisionMask`; the default accepts revision 1.
Initialization also requires `DIAG_ALRT.MEMSTAT`.

`DIAG_ALRT` contains configuration and clear-on-read/live evidence. Cooperative
operations use deterministic configuration writes rather than an unsafe
read-modify-write. `DiagnosticEvents` retains latest, newly observed, sticky,
and first-observed timestamps in fixed storage. Reading or acknowledging this
cache is bus-silent. Acknowledgement does not claim that hardware has been read.

## Health, ownership, and concurrency

`HealthPolicy::PASSIVE` is the default. Transfer counters and last
success/error information remain diagnostic, but the library does not suppress
an owner-requested transaction or take recovery authority. The legacy
`LATCH_OFFLINE` policy is available only for compatibility.

Instances are neither thread-safe nor ISR-safe. The application must serialize
all calls and must not re-enter an instance from its transport or time callback.
The core owns no task, queue, mutex, allocation, delay, retry loop, or bus.
Transport callbacks must honor `i2cTimeoutMs` and return the most precise status
available (`I2C_NACK_ADDR`, `I2C_NACK_DATA`, `I2C_NACK_UNKNOWN_PHASE`,
`I2C_TIMEOUT`, `I2C_BUS`, or `I2C_ERROR`).

Every fallible API returns `Status { code, detail, msg }`. `msg` always points
to static storage; `detail` carries transport- or implementation-specific
numeric context and must not be treated as a portable error code. `ok()` means
only `code == OK`; `IN_PROGRESS` is a nonterminal state exposed separately by
`inProgress()`. Output parameters remain unchanged on non-OK status unless the
specific API explicitly documents readiness-output clearing.

## Other API groups

The public surface remains complete, but integration code should distinguish:

- Recommended cooperative owner operations: `bind`, cooperative starts, `pollJob`,
  cancellation/timeout, cache-only state/result/diagnostic access, and explicit
  invalidation.
- Pure helpers: calibration planning, identity parsing, register encoding,
  conversion, and timing calculations.
- Synchronous convenience and typed feature access: legacy `begin`, typed
  scalar reads, alert thresholds, mode/timing setters, reset, accumulator,
  energy, and charge APIs. These share validation/state semantics but can use
  multiple bounded callbacks in one call; use them only where that latency fits
  the owner's policy.
- Diagnostic/advanced operations: raw register access and destructive status or
  accumulator reads. Raw writes can desynchronize cache and hardware; explicitly
  invalidate/reconcile afterward.

No public hardware API is safe to call during an active cooperative job or an
unconsumed terminal-result window. Only APIs documented as cache-only,
bus-silent, or lifecycle cleanup may be used there.

## Arduino and ESP-IDF examples

- `examples/01_basic_bringup_cli/` is an Arduino diagnostic CLI. It uses the
  cooperative lifecycle for initialization, integer sampling, resets, and
  reinitialization. Board pins and `Wire` transport live under `examples/common`.
- `examples/esp_idf/basic/` is native ESP-IDF code using `app_main`,
  `driver/i2c_master.h`, `esp_timer`, FreeRTOS delays, fixed C buffers, and an
  injected native transport. It does not use Arduino compatibility types.

Both examples use a 15 mOhm/10 A demonstration profile. Replace it with the
actual Kelvin-sensed shunt and validated maximum current for your hardware.
The Arduino example defaults to GPIO8 SDA, GPIO9 SCL, and address `0x40`, then
attempts INA228 discovery during startup. Override the pins, bus settings,
address, and calibration for the actual board before connecting a load.

The Arduino and native ESP-IDF CLIs expose the same command/alias contract.
Inputs are parsed strictly, diagnostic reads are identified as destructive,
and raw writes require an explicit trailing `confirm`. Successful status output
contains one green `Status: OK` line; a separate `Message:` line is reserved for
non-OK diagnostic context. See the
<a href="docs/CLI.md">CLI and HIL reference</a> and the example-specific
READMEs before using service commands or collecting validation evidence.

On Windows, build, upload, and monitor this repository's Arduino example
through the checked-in wrapper:

```powershell
.\scripts\pio.cmd run -e esp32s3dev
.\scripts\pio.cmd run -e esp32s3dev -t upload --upload-port COMx
.\scripts\pio.cmd device monitor -e esp32s3dev -p COMx
```

Use `esp32s2dev` for the supplied ESP32-S2 environment.

See [ESP-IDF integration](docs/integration/esp-idf.md), the
[device reference](docs/reference/ina228-device-reference.md), and the
[validation status](docs/validation/validation-status.md).

## Build and validation

Arduino builds require PlatformIO Core `6.1.19` and are pinned to PIOArduino
`55.03.311`, which supplies
Arduino-ESP32 `3.3.11` and ESP-IDF `5.5.5`. The Arduino diagnostic CLI `version`
command prints the running framework versions and detected flash/PSRAM so HIL
evidence can verify the firmware stack instead of relying only on build logs.

```powershell
.\scripts\pio.cmd test -e native
.\scripts\pio.cmd run -e esp32s2dev
.\scripts\pio.cmd run -e esp32s3dev
python tools/check_core_timing_guard.py
python tools/check_owner_contract.py
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python tools/test_run_i2c_hil_parser.py
python scripts/generate_version.py check
doxygen Doxyfile
```

The native ESP-IDF example is built by CI for ESP32-S2 and ESP32-S3. Physical
hardware validation is separate from native tests and build validation; see the
validation documents for dated evidence and remaining gates. Framework and
toolchain changes are recorded in the changelog and validation status.

## Versioning

`library.json` is the version source of truth. `include/INA228/Version.h` is
generated; do not edit it directly.

```sh
python scripts/generate_version.py bump patch
# Or set an explicit release version:
python scripts/generate_version.py set X.Y.Z
python scripts/generate_version.py check
# Rewrite the generated files from library.json without changing the version:
python scripts/generate_version.py sync
```

The generator synchronizes `Version.h`, `idf_component.yml`, and Doxygen's
project version from `library.json`. `check` verifies they agree and fails
otherwise; PlatformIO also runs it as a pre-build script.

Version 3 is a breaking release because configuration and cooperative-owner
operation contracts were added and the default health policy changed to passive.

## License

MIT. See [LICENSE](LICENSE).
