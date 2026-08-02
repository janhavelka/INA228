# INA228 v3.0.2 Post-Write Wait Timing And TunnelMonitor Pin

## Objective

Correct one proven cooperative-job timing defect in the INA228 library,
publish the correction as a new exact annotated patch release `v3.0.2`, and
then update the sibling TunnelMonitor firmware to exact-pin that released tag.

The correction must stay small, framework-neutral, fixed-memory, bounded, and
backward-compatible. Do not redesign the cooperative job engine.

## Repositories And Authority

- Library repository:
  `C:\Users\Honza\Documents\Projects\INA228`
- Firmware consumer:
  `C:\Users\Honza\Documents\Projects\TunnelMonitor-node`
- Read and obey both repositories' `AGENTS.md` files before changing either
  repository.
- Preserve all unrelated dirty changes. The INA228 production tree is expected
  to start clean at `v3.0.1`; this instruction file is the sole allowed prompt
  addition and is not production dirt. Stop and report any tracked production
  change, staged change, other untracked source/test/release file, or HEAD/tag
  mismatch.
- Inspect the TunnelMonitor repository before editing. Preserve any unrelated
  worktree changes and keep the dependency pin in the focused consumer commit
  required below; do not mix other firmware work into it.

## Definitive Culprit

Both `v3.0.0` and `v3.0.1` contain this sequence in the cooperative job engine:

```cpp
case JobPhase::SAMPLE_TRIGGER:
  st = writeReg16(cmd::REG_ADC_CONFIG, _triggeredAllAdcConfig());
  // ...
  _jobWaitStartMs = nowMs;

case JobPhase::RESET_WRITE:
  st = writeReg16(cmd::REG_CONFIG, cmd::CONFIG_RST);
  // ...
  _jobWaitStartMs = nowMs;

case JobPhase::WRITE_ADC_CONFIG:
  st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
  // ...
  if (isTriggeredMode(_config.mode)) {
    _markTriggeredConversionStarted(nowMs);
  }
```

`nowMs` is the caller timestamp passed into `pollJob()` before the synchronous,
blocking I2C write callback runs. If the callback takes 2-4 ms, those
milliseconds are counted as ADC-conversion or reset-startup wait even though
the INA228 cannot act on the completed register command until it is delivered
during that callback. The exact hardware start instant is inside the callback,
near completion of the successful transfer; a timestamp sampled immediately
after callback return is the first portable, conservative time origin available
to the library. The next poll can currently read `DIAG_ALRT.CNVRF` or verify
reset too early.

This is not, by itself, a mismatch between two timestamp domains. It is a stale
pre-transfer value being used as the epoch for a post-transfer hardware event.
When both `Config::nowMs` and caller-supplied `pollJob()` timestamps are used,
they must nevertheless be documented to come from the same wrap-safe monotonic
millisecond domain.

TunnelMonitor retained two real `MEASUREMENT_NOT_READY` terminals and one
durable `Partial` sample with invalid power fields. There was no matching
INA228 transport failure, and the next acquisition succeeded. That evidence is
consistent with this defect, but without a retained pre-write/post-write/read
trace it does not prove that this defect was the sole cause of every observed
terminal. The source defect itself is deterministic and independently proven.
Increasing the firmware owner deadline cannot correct its time origin.

## Required Library Change

### 1. Represent one unresolved post-transport time origin explicitly

Use one small private fixed-state field in the existing `INA228` state machine;
do not add a timing service, allocation, queue, task, or public API. A private
enum is clearer than overloading timestamp value `0`, because every `uint32_t`
value is valid across wraparound. Semantics equivalent to this are required:

```cpp
enum class DeferredTimeOrigin : uint8_t {
  NONE,
  JOB_WAIT,
  TRIGGERED_CONVERSION,
};

DeferredTimeOrigin _deferredTimeOrigin = DeferredTimeOrigin::NONE;
```

Only one such origin can be unresolved because the existing cooperative owner
allows one active job and the relevant transitions are mutually exclusive.
Prove that invariant in code review and tests. If an equally small existing
phase representation can express the same states without ambiguity, reuse it
instead; do not add parallel state.

Do not use `0`, `UINT32_MAX`, or another timestamp sentinel.

### 2. Use one private post-transport arming helper

Add one small helper near `_nowMs()` and the existing timing helpers. It is
called only after a successful write callback returns and arms either
`_jobWaitStartMs` or `_trigStartMs`:

```text
if Config::nowMs exists:
    sample it now, after callback return
    store that value in the requested destination
    leave no deferred origin
else:
    do not store the caller's pre-write poll timestamp
    mark the appropriate origin as deferred
```

The exact private function signature and name may follow the surrounding code,
but keep one helper and one state field. Do not pass a pre-write timestamp as a
fallback. `Config::nowMs` remains optional.

### 3. Resolve a deferred origin from the next explicit caller timestamp

When no hook exists, the library cannot observe callback-completion time during
the write call. Use the next caller-supplied monotonic timestamp, which is
necessarily obtained after the callback returned, as a conservative origin:

- For `JOB_WAIT`, the next `pollJob(nowMs, ...)` must store
  `_jobWaitStartMs = nowMs`, clear the deferred state, return `IN_PROGRESS`, and
  perform zero transport callbacks on that poll. The full existing sample or
  reset wait begins at this newly established origin.
- "Next" means a later public API invocation entered after the successful
  write callback returned. `pollJob()` can consume multiple transfers in one
  invocation; its internal loop must not resolve a deferral created by an
  earlier transfer in that same invocation, because the invocation still owns
  the stale pre-write `nowMs`. Resolve `JOB_WAIT` only at public `pollJob()`
  entry when the deferral was already present on entry. After a no-hook sample
  or reset write creates the deferral, return `IN_PROGRESS` from that invocation
  even if transfer budget remains.
- Resolving the origin must be bus-silent even if `maxTransfers > 0`; with
  `maxTransfers == 0` it has the same result and remains bus-silent.
- For `TRIGGERED_CONVERSION`, the next API call that supplies an explicit
  timestamp (`pollJob`, `pollConversionReady`, `pollMeasurementReady`, or
  `tick` through the existing readiness path) must store `_trigStartMs = nowMs`
  and clear the deferred state. A readiness API must report `ready = false`
  and perform no I2C on that anchoring call; only a later call after the full
  calculated interval may inspect `CNVRF`.
- The same invocation rule applies to `TRIGGERED_CONVERSION`: an internal
  continuation of the `pollJob()` call that performed `WRITE_ADC_CONFIG` is not
  a new timestamp source. Initialization/configuration verification may finish
  with its remaining transfer budget, but it must leave the deferred trigger
  origin unresolved for a later public readiness/tick/poll invocation.
- `isConversionReady()` has no caller timestamp. With no configured hook and a
  deferred triggered origin, it must remain bus-silent and report not ready;
  documentation must direct the caller to `pollConversionReady(nowMs, ...)` or
  `tick(nowMs)` to establish and advance the origin.

This deliberately adds at most one bus-silent owner activation in no-hook mode.
It does not add to the sensor's specified wait. Do not change the conversion or
reset duration, transfer budgets, or `JobLimits`; application wall-clock
deadlines already include owner scheduling cadence.

### 4. Correct all three proven cooperative write-started epochs

Apply the arming behavior immediately after successful writes in all three
reachable paths:

1. `SAMPLE_TRIGGER -> SAMPLE_WAIT`: arm `_jobWaitStartMs`.
2. `RESET_WRITE -> RESET_WAIT`: arm `_jobWaitStartMs`.
3. Cooperative `WRITE_ADC_CONFIG` during initialize/reinitialize/config replay
   when the configured mode is triggered: arm `_trigStartMs` instead of calling
   `_markTriggeredConversionStarted(nowMs)` with the stale poll timestamp.

The third path is proven reachable because a valid bound `Config` can select a
triggered mode; `startApplyCalibration()` also uses reinitialization. It is not
speculative and must have its own boundary test.

Audit every assignment to `_jobWaitStartMs` and `_trigStartMs` and every call to
`_markTriggeredConversionStarted()`. Direct synchronous trigger/setter paths
currently sample `_nowMs()` after their successful write when a hook exists;
preserve that correct behavior. Route those paths through the same arming
semantics so a no-hook consumer defers `_trigStartMs` to its next explicit
`pollConversionReady(nowMs, ...)`, `pollMeasurementReady(nowMs, ...)`, or
`tick(nowMs)` timestamp instead of storing the unavailable-hook value `0`.
This is necessary for the stated universal no-hook guarantee and should reuse
the same helper/state, not create another mechanism. Do not change unrelated
diagnostic-capture or job-start/finish timestamps.

Never arm or defer a time origin after a failed or ambiguous write. Preserve
the current transport error, hardware-dirty, job-effect, terminal-delivery,
restore, and reinitialization behavior.

### 5. Make deferred-state lifetime explicit

- Clear `JOB_WAIT` deferral on cancellation, timeout, failure, teardown, bind
  reset, and cooperative-state reset so it cannot leak into a later job.
- Clear `TRIGGERED_CONVERSION` deferral when the associated trigger is
  completed, invalidated, cancelled, timed out, or fails.
- Preserve `TRIGGERED_CONVERSION` deferral across a successful initialize or
  reinitialize terminal result when that successful job actually left a
  triggered conversion pending; the next explicit readiness timestamp must
  resolve it.
- Taking a terminal result must not resurrect or accidentally discard valid
  pending triggered-conversion timing.
- Cancellation/timeout after a successful sample-trigger or reset write must
  retain the existing `PARTIAL`/resynchronization semantics while also clearing
  the deferred time-origin state.
- Starting a later unrelated operation must never inherit a deferred origin.

## Explicit Non-Solutions

Do not:

- add a fixed sleep, delay, grace margin, or `i2cTimeoutMs` to the sensor wait;
- increase TunnelMonitor's INA228 owner deadline;
- retry a triggered sample or reset automatically;
- accept `CNVRF == 0` as valid data;
- move INA228 register or readiness logic into TunnelMonitor;
- change conversion-time formulas, reset wait duration, `JobLimits`, transfer
  counts, or `pollJob(..., maxTransfers)` budgeting;
- allocate memory, create a task, own the bus, or add framework time APIs;
- reuse the pre-write `pollJob()` timestamp when `Config::nowMs` is absent;
- make `Config::nowMs` mandatory;
- reuse, delete, or move `v3.0.0` or `v3.0.1` tags.

## Deterministic Native Regression Tests

Extend the existing `FakeBus` in `test/test_basic.cpp`; do not add a second
fake framework. Add a narrowly configurable one-shot successful-write
duration, keyed by register and write-match plus `advanceNowMsOnWrite`, so the
fake monotonic clock advances inside `fakeWrite()` immediately before the
callback returns. Forced failures and apply-then-fail cases must not consume a
successful-write timing injection or arm an origin.

### Triggered sample test

Prove all of the following:

1. Start from an initialized cooperative device.
2. Record the caller timestamp supplied to the poll that executes
   `SAMPLE_TRIGGER`.
3. Make the successful `ADC_CONFIG` trigger write advance `FakeBus::nowMs` by
   3 ms.
4. Prove the next `DIAG_ALRT` read is not eligible at
   `postWriteMs + calculatedWaitMs - 1`; the poll remains `IN_PROGRESS` and is
   bus-silent.
5. Prove `DIAG_ALRT` becomes eligible at
   `postWriteMs + calculatedWaitMs`, not at
   `preWriteMs + calculatedWaitMs`.
6. Set `MEMSTAT | CNVRF`, complete the existing atomic sample path, and prove
   the sample succeeds with unchanged transfer order/count and valid fields.

Repeat the boundary with `Config::nowMs == nullptr`:

1. Execute the trigger write using a caller timestamp that is clearly nonzero.
2. Prove that timestamp was not reused as the wait origin.
3. On the next `pollJob(anchorMs, 0)`, prove the origin is established,
   `IN_PROGRESS` is returned, and no transfer occurs.
4. Prove `anchorMs + waitMs - 1` is bus-silent and the diagnostic read is first
   eligible at `anchorMs + waitMs`.
5. Repeat the trigger-write poll with transfer budget greater than one and
   prove the just-created deferral is not resolved inside that same invocation;
   the later public poll is still the only anchoring call.

The test must fail against unmodified `v3.0.1` for the intended reason.

### Reset test

Use the same fake write-duration mechanism for `CONFIG.RST` and prove:

1. reset verification is bus-silent before
   `postWriteMs + resetWaitMs`;
2. verification begins only at/after that boundary;
3. the reset job still completes with its existing transfer count, effect,
   and synchronized final state.

Repeat the boundary without `Config::nowMs`: the first poll after the reset
write only anchors the origin and is bus-silent, then the full reset wait must
elapse before the verification read. Use a multi-transfer budget on the write
poll and prove remaining budget cannot resolve the new deferral in-place.

### Cooperative configured-trigger test

Use a valid triggered-mode `Config` and exercise both initialize and
reinitialize/config replay as applicable to the shared path. Prove:

1. the successful cooperative `ADC_CONFIG` write is the event that arms
   triggered-conversion timing;
2. with a hook, readiness remains bus-silent through
   `postWriteMs + conversionWaitMs - 1` and first becomes eligible at the exact
   wrapped-safe boundary;
3. without a hook, even if a large transfer budget lets initialization finish
   in the same `pollJob()` call, the first explicit readiness timestamp only
   anchors `_trigStartMs`, reports not ready, and performs no I2C;
4. a later explicit readiness timestamp enforces the full conversion interval;
5. hookless `isConversionReady()` cannot accidentally anchor at zero or read
   `DIAG_ALRT`; explicit `pollConversionReady(nowMs, ...)` or `tick(nowMs)` can
   establish the origin.

Also update the existing hookless direct-trigger test. Start a conversion at a
nonzero application uptime, prove the first explicit `tick(nowMs)` or
`pollConversionReady(nowMs, ...)` only anchors the deferred origin and remains
bus-silent, then prove `CNVRF` is not read until the full interval after that
anchor. Exercise at least one common setter-trigger route or otherwise prove by
focused shared-helper coverage that every direct post-write
`_markTriggeredConversionStarted()` call uses the same behavior.

### Retained contracts

Retain or extend tests proving:

- `uint32_t` wraparound remains correct for hooked post-write origins and
  hookless next-poll origins for sample, reset, and configured-trigger timing;
- `maxTransfers == 0` remains bus-silent;
- a failed trigger/reset write never enters the wait phase;
- an ambiguous/failed write never arms or defers a timing origin;
- cancelling and timing out before a hookless origin is resolved clears the
  deferred state, produces the existing terminal result/effect, and cannot
  influence the next operation;
- a successful triggered-mode initialization retains exactly the one valid
  deferred trigger origin until an explicit timestamp resolves it;
- a direct triggered conversion without a hook never uses timestamp `0` as a
  fabricated origin at nonzero application uptime;
- cancellation, timeout, terminal-result delivery, request identity, and job
  effects otherwise remain unchanged;
- the no-hook anchoring poll is always transfer-silent and does not consume the
  caller's transfer budget;
- `sizeof(INA228)` and fixed result-size guards still pass.

Register new tests in the existing `main()` list. Do not claim coverage from an
unregistered Unity function.

## Documentation And Version `3.0.2`

This is a backward-compatible patch release.

After the code and tests pass:

1. Set `library.json` to `3.0.2` using the repository generator.
2. Run `python scripts/generate_version.py sync`; commit the generated
   `include/INA228/Version.h`, `idf_component.yml`, and Doxygen version update.
3. Add a `CHANGELOG.md` `3.0.2` entry that states:
   - all cooperative sample/reset/configured-trigger time origins are now
     established only after the successful blocking write completes;
   - with `Config::nowMs`, the library samples the post-write time immediately;
   - without the optional hook, the next explicit caller timestamp establishes
     the origin and the full device wait follows, adding one bus-silent owner
     activation rather than reusing a pre-write timestamp;
   - no waits, transfer budgets, retries, or public APIs changed.
4. Update the `NowMsFn`/cooperative timing documentation in `Config.h`, README,
   and the relevant integration/reference page to state:
   - hook and explicit polling timestamps use the same wrap-safe monotonic
     millisecond domain;
   - the hook is sampled after a successful write;
   - no-hook timed transitions require the next explicit timestamp to anchor
     the full wait and therefore add one bus-silent poll;
   - hookless `isConversionReady()` cannot advance an unresolved origin.
5. Update validation status with software evidence only. Do not claim new HIL
   until it actually runs and is retained.

## INA228 Release Gates

Run the complete repository checklist, not only the new tests:

```text
python -m py_compile tools/run_i2c_hil.py
python tools/run_i2c_hil.py --parser-self-test
python tools/run_i2c_hil.py --dry-run --suite exhaustive --include-not-run --benchmark-count 100
python tools/check_core_timing_guard.py
python tools/check_owner_contract.py
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python scripts/generate_version.py check
python -m platformio test -e native
python -m platformio run -e esp32s3dev
python -m platformio run -e esp32s2dev
python -m platformio pkg pack
doxygen Doxyfile
git diff --check
```

Also inspect the produced package and compile its exported core source under
C++17 as required by `docs/validation/release-checklist.md`. Remove the
generated package archive after validation. Run pure ESP-IDF S3/S2 builds when
the configured IDF environment is available; otherwise report them as
`NOT RUN`, never inferred from Arduino builds.

Do not access or upload to COM21; another test owns it. If hardware validation
is authorized and COM19 is available, use COM19 only and retain a condensed
report plus raw evidence under INA228's documented validation locations.

## Publish Exact INA228 `v3.0.2`

Only after every mandatory software gate passes:

1. Reconfirm the INA228 worktree contains only intended release changes.
2. Commit with subject `Release v3.0.2`.
3. Create annotated tag `v3.0.2` with message `Release v3.0.2`.
4. Verify and record:
   - release commit SHA;
   - annotated tag object SHA;
   - peeled commit from `v3.0.2^{}`;
   - `library.json`, `Version.h`, `idf_component.yml`, and Doxygen versions all
     equal `3.0.2`.
5. Push `main` and the annotated tag only after the verified local release is
   complete. If authentication/network is unavailable, stop with the clean
   commit/tag intact and report the exact remaining push commands.

Never force or move an existing tag.

## Pin TunnelMonitor To The Released Tag

Only after the remote annotated `v3.0.2` tag resolves to the verified release
commit:

1. In `TunnelMonitor-node/platformio.ini`, change only the INA228 dependency
   from `#v3.0.0` to `#v3.0.2`.
2. Update generated BuildInfo expectations from `INA228@v3.0.0` to
   `INA228@v3.0.2`.
3. Update active current dependency facts and exact commit/tag-object facts in:
   - `docs/guidelines/dependency_policy.md`;
   - `docs/guidelines/i2c_peripherals.md`;
   - `docs/guidelines/decisions.md`;
   - `docs/guidelines/ownership.md`;
   - `docs/guidelines/reference/dependencies.md`;
   - `docs/guidelines/reference/hardware_and_build_facts.md`;
   - `docs/guidelines/target_architecture.md` where it states the selected
     current INA228 version;
   - the current Prompt 46 implementation-plan/open-question status.
4. Do not rewrite archived prompts, handoffs, or historical reports that
   correctly describe v3.0.0 evidence.
5. Do not change `Ina228Module`, the 1000 ms owner deadline, I2C ownership,
   readiness policy, measurement schema, or application retry behavior unless
   a failing integration test proves a separate incompatibility. Stop and
   report such an incompatibility instead of masking it.

Run the TunnelMonitor consumer gates:

```text
python scripts/check_build_profile_matrix.py
python scripts/check_native_test_registration.py
python -m unittest discover -s test/python -p "test_*.py"
node --test test/web/web_pages.test.mjs
pio test -e native
pio test -e native_hil_fram
pio run -e tunnelmonitor_wifi
pio run -e tunnelmonitor_wifi_hil
git diff --check
```

Commit the exact TunnelMonitor dependency pin and its directly required
BuildInfo/documentation updates as one focused consumer commit after all
consumer gates pass. Do not freeze or qualify a new firmware release candidate
in this prompt. The RS485 cancellation and replay-cache corrections are already
implemented in the firmware branch; preserve them and report any conflict
instead of reimplementing or weakening them.

If COM19-only firmware HIL is authorized, prove repeated scheduled power
acquisitions under representative I2C/measurement/storage/Cloud load with:

- no `MEASUREMENT_NOT_READY` terminal;
- no durable `Partial` sample caused by invalid INA228 voltage/current fields;
- exact request/result conservation;
- no panic, unexpected reset, queue overflow, or progressive heap loss.

Do not use COM21.

## Completion Report

Report separately:

1. exact INA228 source/test/document files changed;
2. why the timestamp is now post-write;
3. regression test boundaries for all three paths, with and without the hook,
   and why they fail on v3.0.1;
4. every software/HIL gate as `PASS`, `FAIL`, or `NOT RUN`;
5. release commit, tag object, and peeled commit;
6. exact TunnelMonitor pin and BuildInfo string;
7. TunnelMonitor files changed and consumer gate results;
8. intentional omissions and remaining release work.

Do not call the overall TunnelMonitor firmware release-ready from this prompt.

The timing fix is complete only if no valid configuration can use a pre-write
timestamp as the start of a conversion/reset interval: hook-equipped paths use
the immediate post-callback sample, and no-hook paths use a later explicit
caller timestamp followed by the full required interval.
