# TunnelMonitor-node suitability audit

## INA228 power monitor library

Date: 2026-07-18

Audit result: **strong protocol base, focused refactor required before integration**

INA228 v2.0.0 contains most of the chip work that TunnelMonitor should not keep
inside `I2cTask`. It has injected transport callbacks, fixed memory, correct
register widths and signedness, calibration handling, precise transport errors,
fixed-unit integer output, destructive-diagnostic tracking, and a bounded
fixed-step instantaneous sample path.

It is not suitable unchanged. Initialization and recovery still execute many
I2C callbacks in one call. Active jobs cannot be cancelled. The continuous
fixed-step sample omits `DIAG_ALRT`, so it can publish current and power without
checking `MATHOF` or `MEMSTAT`. Its five reads can also cross continuous ADC
updates. The calibration API cannot directly express TunnelMonitor's deliberate
`0x0666` / nominal `5 uA/LSB` profile. Finally, the library's mandatory
`OFFLINE` latch competes with `I2cTask`, which already owns optional-device
health and bus recovery.

The recommended path is to refactor the existing v2 staged core, publish and
exact-pin a new immutable revision, then replace the direct INA228 protocol in
`I2cTask` with one small owner-private adapter. Do not keep the current direct
path as a fallback after the adapter is qualified.

## Audit basis

The audit used these revisions:

| Repository | Revision | Notes |
| --- | --- | --- |
| TunnelMonitor-node | `fff99fe17e60b9287ec4d8d3eca5b3230ae44223` | Branch `prompt-44b-sequence`; current direct INA228 implementation and architecture authority |
| INA228 | `f03f4c8b97aa4c1ee7d2be612904df483dda4288` | Final audited source; local `main`, `origin/main`, and annotated tag `v2.0.0` |

The first audit pass used
`c2b32facd793e4be2a90eb7a9d3ed994cfaf5ef9` on
`audit/ina228-industry-readiness-exploration`. The final checkout is two commits
ahead. Full diff inspection found no changes in `include/`, `src/`, `test/`,
`platformio.ini`, library metadata, or CI configuration. The delta updates
release documentation, Doxygen configuration, and validation-document retention.
Findings about library code therefore apply directly to v2.0.0.

### Latest-branch revalidation

Revalidated on 2026-07-18 after `git fetch origin --prune --tags`. The remote
default branch is `origin/main`, and it is also the newest remote branch by
commit date. The local checkout was safely switched from
`audit/ina228-industry-readiness-exploration` at
`c2b32facd793e4be2a90eb7a9d3ed994cfaf5ef9` to `main` at
`f03f4c8b97aa4c1ee7d2be612904df483dda4288`. Local `main`, `origin/main`, and
the dereferenced annotated tag `v2.0.0` now resolve to that commit, with zero
commits ahead and zero behind. The two intervening commits change release and
validation documentation only; they do not change library code, tests,
PlatformIO behavior, or CI behavior.

A fresh full-source re-audit re-read the final public API, implementation,
native tests, build configuration, and retained validation evidence. It
reconfirmed all eleven hard findings at their stated priority. The code delta
does not fix lifecycle work bounds, cancellation, sample diagnostics/coherence,
calibration selection, health ownership, job exclusivity, hardware-state
verification, diagnostic event ownership, accumulator scale epochs, or
deterministic alert startup. Secondary finding S-04 was narrowed because the
final release commit corrected the stale README and changelog release labels.

TunnelMonitor's dependency authority still records historical INA228 v1.3.0 as
deferred (`docs/guidelines/dependency_policy.md:115`). That version does not
contain the v2 fixed-step work and is not the recommended refactor base.

Primary chip facts were checked against the bundled Texas Instruments
[INA228 datasheet](reference/vendor/INA228_datasheet.pdf), revision A. Relevant
PDF pages 14, 17, 23, and 27 through 31 were rendered and visually reviewed.
Those pages cover sequential conversion updates, conversion-ready behavior,
`MATHOF`, `MEMSTAT`, identity fields, measurement resolution, and calibration.
Poppler was not installed, so PyMuPDF was used for rendering. Temporary page
images were removed after review.

This audit changed no library or firmware source, selected no production
dependency, and ran no new physical hardware test. The only intended repository
change is this report.

## Decision summary

### Use after a focused refactor

These are the release gates for TunnelMonitor integration:

1. Split zero-I2C binding from device initialization. Add cooperative
   initialization and reinitialization jobs that use at most the caller's
   transfer budget per poll.
2. Replace the implicit staged read with an explicit start/poll/cancel sample
   job. Cancellation must perform no I2C and must discard partial sample data.
3. Make a TunnelMonitor sample one triggered conversion job: trigger, wait,
   read `DIAG_ALRT`, validate `CNVRF`/`MEMSTAT`/`MATHOF`, then read all five
   instantaneous values while the ADC is back in shutdown.
4. Give an active library job exclusive access to hardware-mutating and
   hardware-reading APIs. Cached getters may remain available.
5. Add an explicit-current-LSB calibration option using fixed engineering
   units. Do not misuse the maximum-current field to force `0x0666`.
6. Remove mandatory library health admission control, or add an external-health
   mode that never suppresses owner-requested I2C.
7. Add staged configuration readback and an explicit hardware-state
   invalidation path for disappearance, reappearance, bus recovery, and sensor
   power loss.
8. Correct diagnostic event ownership: distinguish the latest raw register from
   sticky events, preserve the original event time, and allow acknowledgement.
9. Fix accumulator scale-generation handling before calling the complete
   energy/charge API platform-ready. TunnelMonitor should not use accumulators
   in the first integration.
10. Add exact TunnelMonitor native tests and connected-board HIL for address
    `0x41`, 20 ms callbacks, the selected 25 milliohm calibration, hotplug,
    recovery, and electrical units.

### Do not solve this with adapter band-aids

Avoid these long-term workarounds:

- calling the current synchronous `begin()` or `recover()` from one owner poll;
- setting `offlineThreshold` to a large value;
- passing about `2.62144 A` as the maximum only to obtain a nominal `5 uA` LSB;
- adding a separate owner-side `DIAG_ALRT` phase around the library sample;
- reconstructing the object or calling `end()` to cancel an expired job;
- copying identity, register, sign-extension, or calibration logic into the
  adapter;
- trusting cached calibration after device removal or power loss;
- keeping direct and library INA228 implementations in parallel;
- adding a second I2C task, device registry, plugin system, or generic sensor
  framework; or
- exposing INA228 library types through TunnelMonitor public contracts.

The missing behavior belongs in the reusable chip library. The firmware adapter
should contain transport mapping, owner scheduling, and application policy only.

## TunnelMonitor requirements

The library must fit the existing owner model. The firmware should not weaken
that model to fit a driver.

| Requirement | Current authority or evidence | Consequence for INA228 |
| --- | --- | --- |
| One I2C owner | `docs/guidelines/i2c_peripherals.md:28-35`; `docs/guidelines/target_architecture.md:306` | Only `I2cTask` may call transport. The library owns no bus, task, queue, lock, recovery, or retry. |
| Cooperative work | `docs/guidelines/i2c_peripherals.md:100-133` | A normal `ReadPower` owner poll advances at most one library transport callback. |
| Fixed timing | `include/TunnelMonitor/contracts/EnvPowerDisplay.h:50-54`; `docs/guidelines/interfaces.md:528-535` | Each callback receives a 20 ms timeout. The whole operation retains its original 1000 ms owner deadline. |
| Fixed board facts | `include/TunnelMonitor/BoardPins.h:26-30,80-87`; `include/TunnelMonitor/i2c/I2cConfig.h:9` | ESP32-S3 hardware 2.0.0, GPIO8/GPIO9, 400 kHz, INA228 address `0x41`. |
| Fixed first-firmware profile | `include/TunnelMonitor/contracts/EnvPowerDisplay.h:60-63`; `docs/guidelines/i2c_peripherals.md:446-453` | Shunt 25,000 micro-ohms, warning 1500 mA, maximum 2500 mA, maximum bus 15000 mV. No runtime calibration setting. |
| Fixed result | `include/TunnelMonitor/contracts/EnvPowerDisplay.h:135-145` | Bus mV is unsigned; shunt uV, current mA, power mW, and die temperature milli-C are stored in project result fields. INA228 types remain private. |
| Required fields | `include/TunnelMonitor/contracts/EnvPowerDisplay.h:107-109`; `src/measurement/MeasurementAssembler.cpp:231-251` | Bus voltage and current are required by measurement assembly. Shunt, power, and die temperature are diagnostic/status data. |
| Application flags | `include/TunnelMonitor/contracts/EnvPowerDisplay.h:86-92`; `docs/guidelines/states.md:329-333` | Warning-current and over-range remain firmware policy and do not degrade health after a valid read. |
| Optional device | `docs/guidelines/time_health_watchdog.md:39-53,243-262` | Absence is disabled/optional, not an aggregate-health or watchdog failure. Hotplug must work without reboot. |
| Polling and stale policy | `docs/guidelines/i2c_peripherals.md:416-423` | Firmware owns the 5000 ms cadence, 15000 ms stale rule, cached last-good status, and result publication. |
| Production backend | `docs/guidelines/decisions.md:85-87` | The adapter targets the existing ESP-IDF backend, not Arduino `Wire`. |
| Fixed memory | `include/TunnelMonitor/i2c/I2cTask.h:121-149` | No heap growth, dynamic containers, unbounded wait, or unbounded retry in steady operation. |

The current firmware performs nine I2C transfers per logical power read: two
identity reads, one calibration write, one diagnostic read, and five
measurement reads (`src/i2c/I2cTask.cpp:2622-2835`). The manufacturer read is
already both a presence and identity operation. A library integration should
not add a mandatory ACK-only probe before it.

## Fit matrix

| Area | v2.0.0 state | TunnelMonitor decision |
| --- | --- | --- |
| Framework-neutral injected transport | Good | Keep |
| Fixed memory and stable object ownership | Good | Keep |
| Register widths, endian handling, signedness | Good | Keep |
| Fixed uV/mV/milli-C/mA/mW output | Good | Keep with checked result mapping |
| Precise transport status values | Good | Keep and map in the adapter |
| One-transfer fixed-step sample | Partial | Keep the engine, refactor the operation |
| Atomic sample output commit | Good | Keep |
| Zero-I2C bind | Missing | Required |
| Staged identity/config initialization | Missing | Required |
| Staged recovery/reappearance | Missing | Required |
| Public job cancellation | Missing | Required |
| Active-job hardware exclusivity | Missing | Required |
| Sample-local `DIAG_ALRT` evidence | Missing in normal fixed-step path | Required |
| Coherent one-conversion sample | Not guaranteed in continuous mode | Required for TunnelMonitor path |
| Exact 25 milliohm / nominal 5 uA profile | Not expressible directly | Required |
| External health ownership | Conflicts with mandatory library `OFFLINE` | Required |
| Brownout/config-loss detection | Missing from sample path | Required |
| Exact TunnelMonitor native/HIL evidence | Missing | Required before dependency selection |

## What already fits

These v2.0.0 properties should be retained:

- Core headers and source do not include Arduino, ESP-IDF, FreeRTOS, or `Wire`.
- Transport and optional time callbacks are non-owning and receive an explicit
  timeout (`include/INA228/Config.h:11-49`).
- The core creates no bus, configures no pins, performs no bus recovery, and
  contains no steady-path heap allocation.
- Driver instances are noncopyable and nonmovable
  (`include/INA228/INA228.h:159-166`).
- `Mode`, `ConvTime`, `Averaging`, and `AdcRange` encode the documented fields.
- `RawSample` preserves the device's real widths and signedness. VSHUNT,
  CURRENT, DIETEMP, and CHARGE are signed; VBUS, POWER, and ENERGY are unsigned
  (`include/INA228/INA228.h:46-62`).
- `IntegerSample` already supplies the engineering units needed by
  TunnelMonitor (`include/INA228/INA228.h:64-76`).
- The five-register fixed-step path honors an instruction budget and commits
  outputs only on success (`src/INA228.cpp:721-847`).
- Failed fixed-step register reads clear that job and leave caller outputs
  unchanged. Native tests cover failure at every current sample phase.
- Triggered conversion timing is cooperative and uses wrap-safe unsigned
  elapsed-time comparison (`src/INA228.cpp:1102-1152,2671-2673`).
- Calibration, ADCRANGE, SHUNT_CAL, dirty-state, and rollback handling are
  materially stronger than the direct firmware path.
- `DIAG_ALRT` writes use cached configuration bits instead of destructive
  read-modify-write (`src/INA228.cpp:2356-2364`).
- Identity, MEMSTAT, signed edge vectors, overflow handling, threshold encoding,
  and partial configuration failures have broad native fake-bus coverage.
- The staged reset job is bounded and can run with one instruction per poll.

The existing staged engine is the correct foundation. This audit does not
recommend a new generic driver framework.

## Hard findings

### H-01: initialization and recovery violate the owner work budget

Priority: integration blocker

`begin()` performs three reads and five writes in one call:

- manufacturer ID, device ID, and `DIAG_ALRT`/MEMSTAT reads at
  `src/INA228.cpp:369-404`;
- CONFIG, DIAG_ALRT configuration, SHUNT_TEMPCO, and ADC_CONFIG writes at
  `src/INA228.cpp:406-415,2838-2860`; and
- one SHUNT_CAL write at `src/INA228.cpp:2863-2900`.

At TunnelMonitor's 20 ms per-transfer limit, that one call can occupy about
160 ms if callbacks consume their full bounds. `recover()` is also synchronous:
it performs identity/MEMSTAT reads and six replay writes
(`src/INA228.cpp:489-523,2524-2592`).

The staged replay API is not a staged initialization API.
`startApplyCalibration()` requires an initialized driver
(`src/INA228.cpp:1495-1506`), and `probe()` also rejects use before successful
`begin()` (`src/INA228.cpp:461-486`).

There is another lifecycle problem. If begin fails after a partial hardware
write, it clears the desired `Config` and calibration cache before marking
hardware dirty (`src/INA228.cpp:343-365`). The driver is then uninitialized and
cannot perform its own recovery. The caller must retain and submit the whole
configuration again.

Required refactor:

- Add `bind()` or equivalent that validates and stores callbacks, address,
  timeout, desired configuration, and calibration with zero device I/O.
- Add `startInitialize()` and `pollInitialize(nowMs, maxInstructions)`.
- Include identity, revision, MEMSTAT, deterministic configuration, calibration,
  and readback in that job.
- Add a reinitialization/reappearance entry point using the same engine.
- Retain desired configuration after a failed job. Keep actual hardware state
  `Unknown` or `ResyncRequired` until verification completes.
- Do not add a synchronous INA228 exception to `I2cTask`.

### H-02: active fixed-step jobs cannot be cancelled

Priority: integration blocker

The job kind, phase, and scratch outputs are private
(`include/INA228/INA228.h:691-735,891-901`). The only general clear routine is
private (`src/INA228.cpp:2658-2669`). `end()` clears jobs, but it also tears down
the whole driver (`src/INA228.cpp:438-455`).

`readPowerSampleRawStep()` starts or resumes implicitly
(`src/INA228.cpp:721-748`). If the 1000 ms owner deadline expires after two
registers and the owner stops polling, the old scratch state remains active.
The next command can resume data from the expired command lifetime.

Required refactor:

- Split the operation into `startInstantaneousSample()` and
  `pollInstantaneousSample()`.
- Add bus-silent `cancelJob()` and cache-only `getJobState()`.
- Cancelling a read-only job discards scratch data and leaves the last completed
  output unchanged.
- Cancelling a configuration or reset job after a possible write marks the
  affected register set `ResyncRequired`.
- Make cancellation idempotent and allow a new job immediately afterward.

### H-03: the fixed-step sample can skip invalid current and power evidence

Priority: data-validity blocker

In continuous mode the sample job starts at VSHUNT. It reads `DIAG_ALRT` only
when `_trigPending` is true (`src/INA228.cpp:743-774`). The native test explicitly
asserts that the continuous path does not read DIAG_ALRT
(`test/test_basic.cpp:3237-3274`).

`convertRawSample()` rejects `MATHOF` only when diagnostic data is marked valid
(`src/INA228.cpp:894-904`). The continuous staged path leaves it invalid, so it
can return current and power even though the INA228 says those fields may be
invalid. It also does not see a low `MEMSTAT` during the sample.

The separate triggered helper composition is fragile. A successful
`pollMeasurementReady()` consumes CNVRF and clears `_trigPending`; a following
`readPowerSampleRawStep()` then starts at VSHUNT and does not attach the
diagnostic read to the sample.

The datasheet states that `MATHOF=1` means current and power data may be invalid
(PDF page 27), and `MEMSTAT=0` means trim-memory checksum failure (PDF page 28).

Required refactor:

- Make trigger, readiness, diagnostic capture, and value reads one library job.
- Always return raw and parsed diagnostic evidence with the sample.
- Require CNVRF for triggered data and reject or invalidate current-derived
  fields on MATHOF.
- Return a typed memory/protocol failure when MEMSTAT is low.
- Commit no new complete TunnelMonitor result if required bus voltage or current
  is invalid.
- Preserve the destructive read once; do not add a second adapter-side DIAG
  read.

### H-04: continuous stepped reads are not one coherent sample

Priority: data-coherency blocker

The fixed-step helper reads VSHUNT, VBUS, DIETEMP, CURRENT, and POWER in five
separate transport calls, potentially across five owner polls
(`src/INA228.cpp:777-838`).

The INA228 datasheet states that, without averaging, converted input values are
updated independently in their corresponding registers at each conversion end.
Enabled inputs are converted sequentially, and continuous mode repeats the
sequence indefinitely (PDF page 14). Therefore an ADC update can occur between
the five library reads. The result can mix different continuous conversion
cycles. The current positive fake-bus test cannot detect this.

For TunnelMonitor's five-second cadence, the simplest safe path is a triggered
all-channel operation:

1. Write triggered-all ADC_CONFIG.
2. Wait the computed conversion deadline without I2C.
3. Read DIAG_ALRT and require CNVRF.
4. Read the five values after the chip has returned to shutdown.

The inputs are still converted sequentially inside one defined conversion
sequence, not simultaneously, but the output registers remain stable while the
owner reads them. TunnelMonitor does not need continuous ENERGY/CHARGE
accumulation, so there is no benefit in accepting the continuous tearing risk.

If the library retains a continuous convenience read, document it as a
sequential latest-register view rather than a coherent sample.

### H-05: calibration cannot express the frozen TunnelMonitor profile

Priority: integration blocker

The library accepts only floating-point shunt resistance and maximum expected
current (`include/INA228/Config.h:129-131`). It always requests:

`CURRENT_LSB = maxExpectedCurrentA / 524288`

and derives SHUNT_CAL from that value (`src/INA228.cpp:169-223`).

For TunnelMonitor's `0.025 ohm`, `2.5 A`, wide-range configuration, v2 computes:

| Value | Library result |
| --- | ---: |
| Requested current LSB | `4.76837158203125 uA` |
| SHUNT_CAL | `0x061B` |
| Effective LSB derived from rounded SHUNT_CAL | about `4.7698974609375 uA` |

The current firmware instead programs `0x0666` and converts CURRENT with a
nominal `5 uA/LSB`; it converts POWER with a nominal `16 uW/LSB`
(`src/i2c/I2cTask.cpp:707,2764-2777`). The datasheet explicitly allows choosing
a larger round-number CURRENT_LSB, up to eight times the minimum, to simplify
conversion (PDF pages 30-31).

Both profiles are within the INA228's numeric capability, but they are not
identical. Adopting the current library config would change raw scaling and
rounding. Passing a false maximum-current value only to force `0x0666` would
hide the policy.

Required refactor:

- Add a calibration mode for maximum-current-derived LSB and one for explicit
  LSB.
- Prefer fixed input units: `shuntMicroOhms`, `maxCurrentMilliAmps`, and
  `currentLsbNanoAmps`.
- Return a `CalibrationPlan` containing the written SHUNT_CAL, selected and
  effective LSB, representable signed current range, shunt full-scale range,
  and quantization warning.
- Reject impossible or clamped calibration by default. Permit a degraded plan
  only through explicit opt-in.
- Add exact `25000 uOhm` / `5000 nA` and max-current-derived vectors.
- Make the TunnelMonitor migration choose one profile explicitly and verify it
  against an independent current reference.

### H-06: library health conflicts with owner health and counts the wrong unit

Priority: ownership blocker

Each successful tracked register transfer resets the library's failure streak
and returns it to READY (`src/INA228.cpp:2371-2409`). This counts transfers, not
complete operations. If VSHUNT succeeds and a later register fails every time,
the next VSHUNT success clears the preceding failure. The library may never
reach OFFLINE even though every complete sample fails.

If the threshold is reached, the library latches OFFLINE and rejects normal I/O
without calling transport (`src/INA228.cpp:2452-2456`). Its synchronous
`recover()` becomes the expected route out.

TunnelMonitor already owns complete-command outcomes, optional absence, stale
data, retry, bus recovery, device health, and aggregate-health policy. The
library latch can prevent the owner from executing its chosen hotplug or
recovery sequence.

Required refactor:

- Prefer removing DEGRADED/OFFLINE admission policy from the chip core.
- If standalone health is retained, add a passive external-owner mode that
  never blocks transport.
- Count complete logical operations if operation health is reported.
- Keep raw transport counters observational only.
- Do not treat optional absence as a permanent library latch.

### H-07: staged jobs do not have exclusive hardware access

Priority: state-integrity blocker

The three fixed-step job starters check `_asyncJob`, but most synchronous reads,
configuration setters, raw register access, `triggerConversion()`, and recovery
do not use one central active-job guard. Searches of `_asyncJob` show checks only
around selected staged APIs (`src/INA228.cpp:729,1132,1222,1499,1839`).

Even with externally serialized calls, one owner can accidentally call a
setter between two sample polls. That can change ADCRANGE, SHUNT_CAL, mode, or
the cached current LSB while old raw scratch values are pending. Final
conversion can then use a different scale than the scale active during earlier
register reads.

Required refactor:

- Centralize hardware-call admission.
- While a job is active, permit only its matching poll, cancellation, and
  cache-only status access.
- Return `BUSY` before any transport or cache mutation for other hardware APIs.
- Keep the documented external serialization rule; job exclusivity does not
  make the class thread-safe.

### H-08: cached clean state does not prove hardware configuration

Priority: hotplug and brownout blocker

Initialization and staged config replay write settings but do not read back and
verify the final CONFIG, ADC_CONFIG, SHUNT_CAL, and DIAG configuration. The
staged sample checks only cached `_hardwareDirty` and calibration state before
starting (`src/INA228.cpp:733-747`).

A sensor brownout or same-address replacement restores register defaults while
the driver still considers its cache clean. Current and power can then be zero
or scaled differently while the library converts them using the old
`_currentLsb`.

The current direct TunnelMonitor path rewrites SHUNT_CAL on every read. That is
wasteful and unverified, but it incidentally reduces this specific calibration
loss risk. A library integration that initializes once must not regress it.

Required refactor:

- Verify initialization writes through staged readback.
- Expose bus-silent `invalidateHardwareState(cause)` for owner-observed absence,
  bus recovery, or suspected device reset.
- Revalidate identity, revision, MEMSTAT, CONFIG, ADC_CONFIG, and SHUNT_CAL on
  reappearance.
- Add a small sample-time configuration guard. For the triggered Tunnel profile,
  checking CONFIG and SHUNT_CAL before the trigger is simple and bounded.
- Represent uncertain state as `ResyncRequired`; converted reads must fail
  closed until verification completes.
- Do not write calibration on every healthy sample after this exists.

### H-09: diagnostic sticky evidence has no reliable event lifecycle

Priority: diagnostics correctness

`_captureDiagAlert()` ORs old clear-on-read evidence into every later snapshot,
then replaces `capturedMs` on every read
(`src/INA228.cpp:2337-2347`). An old threshold event can therefore remain set
for the whole session, while a later harmless readiness read makes it appear
newly captured.

There is no public consume or acknowledgement API. `readAndClearDiagAlert()`
clears hardware evidence through a read but does not acknowledge the cached
sticky evidence (`src/INA228.cpp:1594-1596`). A caller-supplied timestamp to
`pollConversionReady()` is also not passed into capture; with no `nowMs`
callback, the diagnostic timestamp remains zero.

Required refactor:

- Separate `latestRaw`, `newlyObservedEvents`, and `stickyEvents`.
- Preserve the first or most recent actual observation time for each retained
  event; do not refresh it on a read that did not contain the bit.
- Add `takeDiagnosticEvents()` or `acknowledgeDiagnosticEvents(mask)`.
- Pass the poll caller's time into diagnostic capture.
- Keep writable alert configuration separate from live and sticky status.

TunnelMonitor's first polling integration needs sample-local MEMSTAT/MATHOF.
The richer event helper is important for later platform diagnostics but need not
be exposed through public firmware contracts immediately.

### H-10: accumulator counts can be revalidated under a new scale

Priority: general library correctness; not a first TunnelMonitor blocker

Calibration or ADC-range changes mark accumulation invalid only temporarily
(`src/INA228.cpp:1382-1390,1430-1433`). The next continuous CNVRF sets
accumulation ready again (`src/INA228.cpp:2348-2349`). Existing ENERGY and
CHARGE counts remain in hardware, but later reads apply the new current LSB to
the entire accumulated value (`src/INA228.cpp:636-654,1036-1099`).

Required refactor:

- Treat a scale change as a new accumulator epoch.
- Keep accumulators invalid until a verified RSTACC job completes.
- Tag accumulator state with calibration generation and reset time.
- Add a fixed-step accumulator reset. The current public reset is a synchronous
  three-transfer sequence (`src/INA228.cpp:2061-2102`).

TunnelMonitor does not currently consume ENERGY or CHARGE. Leave those APIs out
of the first adapter rather than expanding integration scope.

### H-11: startup alert configuration is inherited, not declared

Priority: deterministic configuration

`Config` has no alert configuration. `begin()` clears the local diagnostic
configuration cache, reads DIAG_ALRT, captures any existing control bits, then
writes those bits back during apply (`src/INA228.cpp:270-283,394-404,2337-2347,
2847-2859`). The new session can therefore inherit latch, polarity, conversion-
ready routing, or slow-alert settings left by earlier firmware.

Required refactor:

- Add a small explicit `AlertConfig` to desired configuration.
- Default to documented disabled/normal settings.
- Preserve existing hardware control bits only through an explicit diagnostic
  option.

TunnelMonitor does not currently use the physical ALERT pin, so alert-pin HIL
is not a gate for the first polling-only adapter. Deterministic register state
is still required.

## Important secondary findings

### S-01: exact device identity should expose revision separately

The datasheet defines DEVICE_ID bits 15:4 as DIEID `0x228` and bits 3:0 as
REV_ID. The library requires exact full value `0x2281`
(`include/INA228/CommandTable.h:39-40`; `src/INA228.cpp:382-392`). That is correct
for the verified revision-1 hardware but classifies another revision of the
same die as a different device.

TunnelMonitor has the opposite defect: its alternate mask can accept invalid
values such as `0xF228` (`src/i2c/I2cTask.cpp:701-705`). Its fake backend also
returns `0x2280`, while retained library HIL reports real hardware `0x2281`.

Use `DeviceIdentity { manufacturerId, dieId, revision }`. Validate DIEID
exactly, then apply an explicit supported-revision policy. Fix the TunnelMonitor
fake instead of weakening validation.

### S-02: unsafe calibration should fail by default

The current calibration code can clamp SHUNT_CAL and return success, and it
reports expected shunt-range overflow only through snapshot booleans
(`src/INA228.cpp:186-223`; `include/INA228/INA228.h:98-101`). This makes callers
inspect advisory state after an apparently successful request.

Strict calibration should reject clamping and an incompatible shunt/current
range by default. If permissive planning remains, require explicit opt-in and
return the effective limits directly.

TunnelMonitor's selected wide range is electrically numerically compatible:
`2.5 A * 0.025 ohm = 62.5 mV`, below the `163.84 mV` shunt full scale. Shunt
power, layout, fusing, transients, and safe rail limits remain board concerns.

### S-03: synchronous soft reset ignores the documented startup wait

`softReset()` writes reset and immediately starts finite reset-bit reads
(`src/INA228.cpp:1778-1813,2508-2522`). The fixed-step reset job correctly waits
the datasheet startup interval (`src/INA228.cpp:1835-1907`). Deprecate the
synchronous reset for production use and keep the staged implementation as the
one maintained path.

### S-04: public product-specific naming needs cleanup

The public header describes `readPowerSampleRawStep()` as the "TunnelMonitor
power sample" (`include/INA228/INA228.h:344`). A reusable library should call
this an instantaneous sample and keep product names out of its API docs.

The final `main` release commit fixed the stale metadata present in the original
audit checkout: `README.md` now identifies v2.0.0 as the current release and
pins its installation example to the tag, while `CHANGELOG.md` dates 2.0.0 and
links its comparison to `v2.0.0`. No release-label defect remains on the final
checkout. Keep only the product-specific API naming cleanup for the next
breaking refactor.

### S-05: Arduino C++17 flags are not applied cleanly

Both current Arduino builds pass, but they warn that an inline variable in
`examples/common/CliStyle.h:17` requires C++17. `platformio.ini` requests
`-std=c++17`; the effective Arduino compiler flags still produce the warning.
This does not block the framework-neutral core or TunnelMonitor adapter, but it
should be corrected so examples test the intended language mode without
extension warnings.

## Recommended narrow design

### Cooperative operations

A small explicit operation surface is enough:

```cpp
Status bind(const DriverConfig& config);                 // zero I2C
Status startInitialize();
PollResult pollJob(uint32_t nowMs, uint8_t maxTransfers);

Status startInstantaneousSample(SampleMode mode);
Status startVerifyConfiguration();
Status startReset();
Status startAccumulatorReset();

Status cancelJob();                                      // zero I2C
JobState getJobState() const;                            // cache only
Status invalidateHardwareState(Status cause);           // zero I2C
```

`pollJob(nowMs, 1)` should perform zero or one transport callback. A delay gate
uses zero callbacks. The library should not own a whole-operation deadline;
TunnelMonitor already has a wrap-safe 64-bit absolute deadline. The library
must provide cancellation when that deadline expires.

For a read-only sample, all candidate raw values, parsed diagnostics, fixed
units, configuration generation, and completion state stay in job scratch and
commit together. No partial output becomes public.

### Useful types

Keep the set concrete and small:

```cpp
enum class CalibrationMode : uint8_t {
  FromMaximumCurrent,
  ExplicitCurrentLsb,
};

struct CalibrationConfig {
  uint32_t shuntMicroOhms;
  CalibrationMode mode;
  uint32_t maxCurrentMilliAmps;
  uint32_t currentLsbNanoAmps;
};

struct CalibrationPlan {
  uint16_t shuntCal;
  uint32_t selectedCurrentLsbNanoAmps;
  uint32_t effectiveCurrentLsbNanoAmps;
  uint32_t representableCurrentMilliAmps;
  uint32_t shuntFullScaleMicroVolts;
  bool quantized;
};

struct DeviceIdentity {
  uint16_t manufacturerId;
  uint16_t dieId;
  uint8_t revision;
};

enum class JobKind : uint8_t {
  None,
  Initialize,
  Reinitialize,
  InstantaneousSample,
  VerifyConfiguration,
  Reset,
  AccumulatorReset,
};

enum class HardwareState : uint8_t {
  Unbound,
  Unknown,
  Synchronized,
  ResyncRequired,
};

enum class ChannelFlag : uint16_t {
  BusVoltageValid = 1U << 0,
  ShuntVoltageValid = 1U << 1,
  TemperatureValid = 1U << 2,
  CurrentValid = 1U << 3,
  PowerValid = 1U << 4,
};

struct DiagnosticEvents {
  uint16_t latestRaw;
  uint16_t newlyObservedEvents;
  uint16_t stickyEvents;
  uint32_t observedAtMs;
};

struct AlertConfig {
  bool latched;
  bool conversionReady;
  bool slowAlert;
  bool activeHigh;
};
```

Existing `Mode`, `ConvTime`, `Averaging`, `AdcRange`, raw values, and integer
measurement types are useful. They can be renamed to normal permanent names
such as `OperatingMode`, `ConversionTime`, `AveragingCount`,
`RawInstantaneousSample`, and `InstantaneousSample` during the breaking
refactor. Avoid adding an inheritance hierarchy or generic device interface.

### Useful pure helpers

The following helpers improve testability without owning policy:

- `isValidAddress(uint8_t)` for `0x40..0x4F`;
- `parseDeviceIdentity(uint16_t manufacturer, uint16_t device)`;
- `calculateCalibration(const CalibrationConfig&, AdcRange)`;
- `estimateConversionTimeUs(const AdcConfig&)`;
- checked 20-bit sign extension and 16/24/40-bit big-endian codecs;
- fixed-unit raw conversion with checked integer intermediates;
- `parseDiagnosticFlags(uint16_t)`;
- `configurationMatches(expected, readback)` with reserved-bit masks; and
- cache-only job/hardware/diagnostic snapshots.

Threshold encoding helpers already exist in public setters. Do not add broad
units, variant, callback, registry, or plugin frameworks for this integration.

## Recommended TunnelMonitor integration flow

After the library refactor:

1. Add one exact-pinned INA228 dependency and one owner-private adapter.
2. The adapter supplies the existing `I2cBackend::transfer` through the
   library's callback shape. It does not initialize or recover the bus.
3. Bind address `0x41`, timeout 20 ms, explicit wide ADCRANGE, explicit alert
   defaults, and the chosen 25 milliohm calibration profile with zero I2C.
4. On first use, reappearance, or post-recovery invalidation, start the staged
   initialization job. Advance it with one callback per owner poll.
5. Verify manufacturer, die ID/revision, MEMSTAT, CONFIG, ADC_CONFIG, and
   SHUNT_CAL before publishing synchronized state.
6. For `ReadPower`, start one triggered instantaneous sample job. The library
   owns trigger, conversion wait, DIAG read, and five value phases.
7. On every owner poll, call `pollJob(nowMs, 1)`. Do not renew the original
   command deadline.
8. On deadline expiry, call the bus-silent cancel API. Preserve the prior
   completed TunnelMonitor status values.
9. Map the complete integer sample into `PowerReadResult`. Perform a checked
   conversion from the library's unsigned power value to TunnelMonitor's signed
   field.
10. Apply warning-current, maximum-current, maximum-bus, presence, stale, health,
    and result-publication policy in `I2cTask`.
11. On address absence or suspected sensor reset, invalidate the library's
    hardware state. Reappearance goes through staged initialization, not a
    direct calibration write.
12. After native and HIL qualification, delete INA228 register addresses,
    identity masks, endian parsing, sign extension, calibration constants, and
    conversion math from `I2cTask`.

Do not use library ENERGY/CHARGE, hardware thresholds, raw register commands, or
physical ALERT support in the first adapter. Their existence does not make them
TunnelMonitor features.

## What must stay in TunnelMonitor

The chip library must not own:

- the ESP-IDF I2C handle, pins, 400 kHz bus setup, or device handles;
- queue admission, priority, original operation deadline, retry, or bus
  recovery;
- the fixed board address and board-level 25 milliohm choice;
- warning current, maximum current, maximum bus voltage, or health meaning;
- optional presence, stale state, cached last-good values, or aggregate health;
- the 5000 ms background schedule;
- `PowerReadCommand`, `PowerReadResult`, validity bits, measurement masks, CLI,
  web, or display formatting;
- shunt heating, fusing, isolation, transient protection, layout, or operator
  safety; or
- any dynamic registry or second resource owner.

The library should return protocol facts. The firmware decides what those facts
mean for this product.

## Firmware findings separate from library suitability

These TunnelMonitor issues should be fixed with or around integration. They are
not reasons to move application policy into INA228.

### F-01: the public power status does not apply the declared stale rule

`kOptionalDeviceStaleMs = 15000` is declared, but no implementation use was
found outside its declaration. `SystemRuntime` copies cached `i2c.power`
directly (`src/system/SystemRuntime.cpp:2364-2368`). The public power status can
therefore remain `Ok` indefinitely if no later attempt updates it.

Apply freshness in firmware status projection. Do not ask the chip library to
own this policy.

### F-02: current device-ID validation accepts an invalid alternate layout

`validIna228Identity()` accepts either the correct DIEID mask or
`(deviceId & 0x0FFF) == 0x0228` (`src/i2c/I2cTask.cpp:701-705`). The second test
can accept values such as `0xF228`, even though DEVICE_ID bits 15:4 are not
`0x228`.

Remove this direct logic when the library identity parser is integrated. Until
then, validate the documented DIEID field and revision nibble explicitly.

### F-03: current scaling depends on reset-default ADC configuration

The firmware writes SHUNT_CAL but does not explicitly apply or verify CONFIG or
ADC_CONFIG. Its shunt and calibration math assumes ADCRANGE 0. This is probably
true on the current sole-owner reset-default path, but it is too implicit for a
platformized driver.

The staged library initialization should apply and verify one explicit profile.

### F-04: current MATHOF handling still marks derived data valid

The direct path reads MATHOF and sets the over-range flag, but it also sets all
measurement validity bits and returns success (`src/i2c/I2cTask.cpp:2745-2835`).
The datasheet says current and power may be invalid when MATHOF is set.

After integration, retain over-range evidence but do not publish current and
power as a new valid sample when the library reports math overflow.

### F-05: repeated calibration write is unverified

Every direct read writes `0x0666` but does not read SHUNT_CAL back
(`src/i2c/I2cTask.cpp:2680-2707`). Replace this with verified initialization and
explicit reinitialization. Do not preserve the write-on-every-read behavior in
the adapter.

## Validation evidence and limits

### Checks run on the audited checkout

| Check | Result |
| --- | --- |
| `python -m platformio test -e native` | PASS, 138/138 tests |
| `platformio run -e esp32s3dev -e esp32s2dev` | PASS for both targets; C++17 warning noted above |
| `python tools/check_core_timing_guard.py` | PASS |
| `python tools/check_cli_contract.py` | PASS |
| `python tools/check_idf_example_contract.py` | PASS |
| `python scripts/generate_version.py check` | PASS |
| `python tools/run_i2c_hil.py --parser-self-test` | PASS |
| `platformio pkg pack` | PASS |

GitHub Actions run
[#24 for v2.0.0](https://github.com/janhavelka/INA228/actions/runs/28242514415)
reported success for native tests, package/static validation, Arduino ESP32-S2
and S3, and ESP-IDF ESP32-S2 and S3. This is useful release evidence; it does
not test the proposed TunnelMonitor adapter.

### Existing native coverage

The library already tests:

- callback injection, addresses, identity, MEMSTAT, and transport errors;
- calibration vectors, clamping, ADCRANGE, rollback, and dirty state;
- signed 20-bit VSHUNT/CURRENT edges and low-nibble masking;
- unsigned VBUS/POWER/ENERGY and signed CHARGE handling;
- DIAG clear-on-read preservation, MATHOF, and accumulator overflow;
- triggered conversion timing and wraparound;
- one- and two-instruction fixed-step sample budgets;
- output atomicity on each fixed-step sample failure;
- staged configuration replay and reset failure phases; and
- threshold encoding and invalid values.

That is a strong protocol base. Passing tests do not cover the unsafe API
compositions identified above; one test intentionally asserts that DIAG is
absent from the continuous staged sample.

### Current evidence gaps

No native test currently proves the complete TunnelMonitor profile:

- address `0x41` together with timeout `20` on every callback;
- 25,000 micro-ohm calibration with the explicit nominal `5 uA` decision;
- integrated trigger/wait/DIAG/sample sequencing;
- active-job cancellation at every phase;
- job exclusivity against synchronous setters and raw access;
- full absent-start, insertion, removal, reappearance, and bus-recovery flow;
- configuration loss after sensor power cycle;
- exact warning and over-range boundaries in the new adapter; or
- checked mapping of unsigned library power into the signed project field.

The fake callbacks currently discard timeout values, and the general address
forwarding test uses `0x4F`, not the product profile.

### Limits of retained HIL

The library's own evidence index classifies retained HIL as partial low-voltage
evidence. The runs used dirty worktrees and did not include fault injection,
ALERT-pin capture, controlled MCU reset, or INA228 power-cycle control
(`docs/validation/hardware-evidence.md`).

Useful retained evidence includes address `0x41`, a 400 kHz Arduino example,
framed targeted and transfer-count passes, and a long low-voltage soak. It used
a 50 ms timeout and `0.015 ohm / 10 A` calibration, not TunnelMonitor's 20 ms
and 25 milliohm profile. The file named as the longest 20-hour attempt ended
after about 3.9 hours and retained one UNKNOWN row; it is not a completed
20-hour clean soak.

TunnelMonitor's architecture authority still lists INA228 electrical-unit
acceptance as open (`docs/guidelines/open_questions.md:235`;
`docs/guidelines/i2c_peripherals.md:535-541`). Native tests and compilation do
not close that physical requirement.

## Required tests after refactor

### Library-native tests

Add deterministic tests for:

- zero-I2C bind and invalid bind parameters;
- initialize/reinitialize one-callback budgets and exact phase order;
- initialization failure at every read/write/readback phase;
- cancellation at every job phase, including idempotent repeat cancellation;
- dirty-state behavior when cancelling after each side-effecting write;
- hardware exclusivity for every public hardware API while a job is active;
- integrated triggered sample timing and stable post-CNVRF reads;
- DIAG failure, CNVRF absent, MEMSTAT low, and MATHOF set;
- diagnostic latest/new/sticky event acknowledgement and timestamps;
- explicit and max-derived calibration plans, including 25 milliohm vectors;
- strict rejection of clamped and shunt-range-incompatible plans;
- zero, `+1`, `-1`, signed 20-bit min/max, ignored low nibble, VBUS max,
  POWER max, and fixed-unit rounding through the staged sample API;
- configuration mismatch and reinitialization after simulated sensor reset;
- accumulator scale change remaining invalid until verified accumulator reset;
- exact DIEID and supported/unsupported revision behavior; and
- deterministic alert configuration from non-default starting registers.

### TunnelMonitor integration tests

Add owner/adapter tests for:

- address `0x41`, 20 ms timeout, context pointer, and one callback per poll;
- no library retry or bus recovery; owner read-only recovery plus one retry
  remains the only retry policy;
- no retry for ambiguous configuration/calibration writes;
- owner deadline cancellation without old scratch leaking into the next command;
- absent startup, hot insertion, mid-sample removal, cached-value retention,
  reappearance, staged reconfiguration, and next-sample success;
- sensor reset or calibration/config mismatch between samples;
- exact current warning boundaries: magnitude 1500 is clear under current
  strict-`>` policy; 1501 and -1501 warn;
- exact current over-range boundaries: magnitude 2500 is clear; 2501 and -2501
  set over-range;
- exact bus boundary: 15000 mV is clear; 15001 mV sets over-range;
- MATHOF evidence, invalid derived channels, and over-range projection;
- warning and over-range not degrading optional device health after a valid
  result;
- stale public power status after 15000 ms;
- signed current and shunt mapping plus checked power type conversion; and
- removal of all INA228 register and conversion logic from `I2cTask`.

### Physical TunnelMonitor HIL

After the exact refactored revision and adapter are ready, run a reasonable
connected-board acceptance set:

- clean immutable library and firmware revisions;
- actual ESP32-S3, address `0x41`, shared 400 kHz bus, and 20 ms callbacks;
- independent voltage and current reference checks at zero/near-zero and normal
  load;
- the selected 25 milliohm calibration and both current directions if the
  hardware supports reverse current safely;
- safe points around the 1.5 A warning threshold;
- safe limit checks only within the fixture, shunt, supply, wiring, and board
  ratings;
- optional absence, insertion, removal during a sample, and return;
- controlled sensor reset or power cycle;
- address NACK, data/unknown-phase NACK, timeout, and stuck-bus recovery;
- fixed-step transfer counts and no owner deadline renewal; and
- a clean framed shared-bus soak with no unexplained UNKNOWN rows.

Physical ALERT-pin testing is required only if TunnelMonitor later uses that
pin. It is not a blocker for the polling-only first integration.

## Physical suitability boundary

The INA228's 85 V rating is an IC input capability, not a TunnelMonitor system
safety rating. This software audit does not validate:

- shunt wattage and temperature rise;
- Kelvin routing and common-mode behavior on the board;
- input filtering, transients, fusing, creepage, clearance, or isolation;
- absolute bus voltage accuracy;
- enclosure wiring or service procedures; or
- safe operation of a high-energy HIL fixture.

The first-firmware software maximum of 15 V is application policy. It is not a
hardware interlock. Keep independent electrical protection and qualified board
validation outside the driver.

## Recommended implementation order

1. Create the zero-I2C bind, unified job state, cache-only job snapshot, and
   bus-silent cancellation.
2. Add staged initialization/reinitialization with deterministic config and
   readback.
3. Add explicit fixed-unit calibration planning and freeze the TunnelMonitor
   profile through tests.
4. Refactor instantaneous sampling into one triggered job with DIAG and stable
   post-conversion reads.
5. Enforce active-job exclusivity and external-owner health mode.
6. Add configuration invalidation/verification and hotplug tests.
7. Correct diagnostic event lifecycle and accumulator scale epochs.
8. Clean identity/revision handling, alert defaults, reset API, naming, and
   release metadata.
9. Publish a new immutable INA228 release and exact-pin it in a private
   TunnelMonitor adapter.
10. Replace the direct firmware protocol, fix firmware-only findings, run native
    tests/builds, then complete connected-board HIL.

## Final assessment

INA228 v2.0.0 is worth refactoring. Its register protocol, conversion code,
fixed memory model, typed errors, integer result, and staged sample foundation
are stronger than maintaining a second chip implementation in TunnelMonitor.

It is not a safe drop-in dependency today. The blockers are concentrated at
the lifecycle, owner, calibration, diagnostic, coherence, and hotplug
boundaries. They do not justify rewriting the chip protocol in firmware, and
they do not require a broad platform framework.

Refactor the existing library around passive binding and explicit cooperative
jobs, publish one reviewed immutable revision, then integrate it behind
`I2cTask`. Until that work and TunnelMonitor-specific HIL are complete, the
current direct path remains the production baseline.
