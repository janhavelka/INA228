# INA228 audit — 2026-08-27

> Review disposition: every finding in this audit was rechecked against the
> current tree on 2026-08-30. Implemented changes, rejected proposals, factual
> corrections, and validation results are recorded in
> [CODE_AUDIT_RESOLUTION.md](CODE_AUDIT_RESOLUTION.md).

Scope: the driver (`include/INA228/`, `src/INA228.cpp`) checked against the TI
INA228 datasheet (SLYS021A, rev. A May 2022) in `docs/reference/vendor/`, plus
the examples, the HIL/contract tooling, and the documentation tree.

Base commit: `6137ee7` (tag `v3.0.3`). Everything below was verified against the
real code; findings reproduced by a compiled program are marked **[proven]**.

This file is retained as historical review input. Current dispositions and
follow-up verification belong in `CODE_AUDIT_RESOLUTION.md` and `CHANGELOG.md`.

---

## 1. Scaling and register handling: verified correct

The driver was checked end to end against the datasheet's own worked example
(§8.2.2.5, R_SHUNT = 16.2 mΩ, I_MAX = 10 A). Feeding the datasheet's returned
register values through the driver reproduces every published result exactly:

| Quantity | Register value | Driver output | Datasheet |
| --- | ---: | ---: | ---: |
| SHUNT_CAL | — | 4050 | 4050 |
| Shunt voltage | 311040 | 0.097200 V | 0.0972 V |
| Shunt voltage (negative) | 0xB4100 | −0.097200 V | −0.0972 V |
| Bus voltage | 245760 | 48.0000 V | 48 V |
| Temperature | 3200 | 25.0000 °C | 25 °C |
| Current | 314572 | 5.99998 A | 6 A |
| Power | 4718604 | 288.0007 W | 288 W |
| Energy | 1061683200 | 1036800.0 J | 1036800 J |
| Charge | 1132462080 | 21600.0 C | 21600 C |

Threshold encoders match the datasheet's own examples too: `SOVL` 162 mV →
32400 (`0x7E90`), `BOVL` 52 V → 16640 (`0x4100`), `TEMP_LIMIT` 100 °C → 12800,
`PWR_LIMIT` 288 W → 18432. `setAdcRange(MV_40_96)` correctly multiplies
SHUNT_CAL by 4 and leaves CURRENT_LSB unchanged.

Also confirmed correct: DIETEMP is read as a full 16-bit signed value with no
shift (the single most common INA228 porting mistake); VSHUNT/VBUS/CURRENT are
sign-extended from bit 19 of the shifted 20-bit value, not bit 23; POWER is
24-bit unsigned; ENERGY is 40-bit unsigned and CHARGE 40-bit signed;
`DIAG_CLEAR_ON_READ_MASK` correctly excludes ENERGYOF/CHARGEOF/MATHOF; energy
accumulation requires continuous shunt **and** bus conversion while charge
requires shunt only, matching §7.3.2; triggered modes correctly refuse
accumulation per §7.3.4.

No scaling, signedness, bit-position, or register-width defect was found.

---

## 2. Fixed in this pass

All changes below are in the working tree. `pio test -e native` passes
121/121 (117 pre-existing + 4 new regression tests), both Arduino targets build,
and all six static contract checks plus Doxygen pass.

### 2.1 Driver core

**D1 — Conversion and reset waits were skipped whenever the clock advanced
during the write. [proven]** *(critical)*

`_armPostWriteTimeOrigin()` samples `Config::nowMs` **after** the blocking write
returns, while the wait gates compared against the `nowMs` the caller sampled
**before** calling `pollJob()`. If the write crossed a millisecond boundary —
routine for a 3-byte write — `_jobWaitStartMs > nowMs`, `nowMs - _jobWaitStartMs`
underflowed to ~0xFFFFFFFF, and the gate read as "elapsed". The entire
conversion wait was consumed instantly:

```
adv=0 ms/write  declared wait=4000 us  pollJob(1000,20) -> IN_PROGRESS transfers=3   (correct)
adv=1 ms/write  declared wait=4000 us  pollJob(1000,20) -> OK          transfers=11  (whole job)
```

Consequence: `SAMPLE_DIAG` ran ~0 ms after the trigger write. In continuous base
mode the stale CNVRF latched by the *previous* conversion was still set, so all
five channels were read before the new conversion completed and committed as a
`SUCCEEDED` / `CONFIRMED` sample. Wrong data, presented as verified.

Fix: a shared wrap-safe helper that treats a delta in the upper half of the
uint32 range as "origin is in the future, not elapsed", applied to both job wait
gates and to `_triggerDeadlineElapsed()`.

```cpp
static bool waitElapsed(uint32_t nowMs, uint32_t startMs, uint32_t waitMs) {
  const uint32_t elapsed = nowMs - startMs;
  return elapsed < 0x80000000U && elapsed >= waitMs;
}
```

Normal wraparound still works; the inversion no longer does. Covered by
`test_wait_origin_newer_than_caller_timestamp_does_not_skip_wait`, which fails
if the fix is reverted.

**D2 — A successful software reset silently reverted every alert threshold.
[proven]**

Datasheet §8.2.2.4: *"Values stored in the alert limit registers are set to the
default values after VS power cycle events and need to be reprogrammed each time
power is applied."* `CONFIG.RST` is the same event. The reset job replays only
CONFIG, ADC_CONFIG, DIAG_ALRT, SHUNT_TEMPCO and SHUNT_CAL — SOVL, SUVL, BOVL,
BUVL, TEMP_LIMIT and PWR_LIMIT went back to full scale with
`SettingsSnapshot::thresholdsDirty` still `false`. The raw
`writeRegister16(REG_CONFIG, …|CONFIG_RST)` path already did the right thing;
the cooperative job did not. For an over-voltage limit, silently reverting to
full scale with no advisory is the worst failure mode available.

Fix: `RESET_WRITE` now calls `_markThresholdsDirty()`.

**D3 — The reset job under-reported which registers were dirty.**

`RESET_WRITE` recorded only `REG_CONFIG` in `_jobTouchedRegisterMask`, so a
reset that failed afterwards told recovery tooling that only CONFIG needed
attention. Fix: a named `RESET_TOUCHED_REGISTER_MASK` covering all eleven
registers the reset restores.

**D4 — `invalidateHardwareState()` destroyed the evidence it was recording.**

It set `_hardwareDirty = true` and then `_dirtyRegisterMask = 0` — including the
mask a just-failed job had contributed through `_cancelJob`. It also overwrote
`_hardwareDirtyCause` unconditionally, contradicting that field's documented
"first failure that made hardwareDirty true". Fix: keep the mask, and only
record the cause when the driver was not already dirty.

**D5 — A bad NV-trim checksum left `hardwareDirty` false.**

The `MEMORY_ERROR` branch in `SAMPLE_VERIFY_ADC` set three of the six fields
`_invalidateJobHardwareState()` maintains, so `getSettings()` reported
`hardwareDirty == false` and `hardwareDirtyCause == OK` after a MEMSTAT failure.
Fix: call the shared helper. `JobEffect::CONFIRMED` is retained — the ADC
restore genuinely was verified by readback; the MEMSTAT failure is a device
health problem, not an unresolved mutation.

**D6 — `bind()` did not clear `thresholdsDirty`.**

`INA228.h` states the flag is "sticky until begin()/end() state reset"; `bind()`
reset every other advisory but not this one, so a fresh binding — potentially to
a different device — inherited the previous binding's advisory.

**D7 — `setAdcRange()` left a stale full-scale in the calibration plan.**

On an uncalibrated binding neither calibration branch ran, so `newPlan` was
committed unchanged while `CONFIG.ADCRANGE` did change. `getCalibrationPlan()`
then reported 163840 µV full scale for hardware programmed to ±40.96 mV — a
cache/hardware divergence with no dirty flag. Fix: set
`shuntFullScaleMicrovolts` from the requested range unconditionally.

**D8 — Internal invalidation left triggered-conversion timing armed.**

`_invalidateJobHardwareState()` did not clear `_trigPending`/`_trigStartMs`,
while the public `invalidateHardwareState()` did. Harmless today (the readiness
APIs are gated on `_initialized`) but an asymmetry one refactor away from
mattering. Fix: call `_invalidateTriggeredConversionTiming()`.

**D9 — Failed threshold writes raised no advisory.**

A failed `setShuntOvervoltageThreshold()` (etc.) may still have landed on the
wire, but nothing told the owner to reapply the limit. Fix: the six typed
setters and the raw `writeRegister16()` threshold path now share
`_writeThresholdRegister()`, which marks thresholds dirty on failure.

### 2.2 Examples

**E1 — The CLI wedged permanently after any operation deadline. [both examples]**

```cpp
st = device.timeoutJob();
if (!st.ok()) { return st; }          // always taken
st = device.takeJobResult(timedOutOperationId, result);
activeOperationId = 0U;               // unreachable
```

`timeoutJob()` reports the cancellation *as* `Err::OPERATION_TIMEOUT`; it returns
`Ok()` only when no job was active, which this branch can never be. So the
terminal result was never consumed and `activeOperationId` never cleared — every
later `reset`, `rstacc`, `integer`, `recover`, `selftest`, `sample_step` and
`apply_start` returned `BUSY` forever, recoverable only by `end`/`init`. Fix:
accept `OPERATION_TIMEOUT` as the success path.

**E2 — `activeOperationId` leaked when `takeJobResult()` failed.** Same
permanent-`BUSY` state via `RESULT_NOT_AVAILABLE`/`STALE_RESULT`. The job has
already left `ACTIVE` at that point, so the slot is now released either way.

**E3 — `mode 1`..`mode 7` were reported as failures.** `setMode()` returns
`Err::IN_PROGRESS` for triggered modes. Both CLIs printed red/FAIL and latched
`hilCommandStatus = IN_PROGRESS`, which the HIL frame then reported as a
non-OK terminal status. The `trigger` handler already did this correctly.

**E4 — `sleepMs(1)` never slept.** `pdMS_TO_TICKS(1)` is 0 at the default 100 Hz
FreeRTOS tick, so `runOwnerJob()` busy-spun at full task priority for up to the
2 s operation deadline, starving IDLE0. Fix: clamp to at least one tick.

**E5 — `select()` failure spun without backoff.** `ready < 0` (e.g. a console VFS
without `select` support) fell into the same `continue` as the normal 10 ms
timeout. Fix: distinguish the two and sleep on error.

**E6 — Every Arduino read failure collapsed into a generic `I2C_ERROR`.** On
arduino-esp32 `endTransmission(false)` only latches the repeated-start flag and
returns 0; the whole write+read is issued by `requestFrom()`. So the
`mapWireResult` branch was dead and address NACK, data NACK, timeout and bus
error were indistinguishable — `init` against an absent device reported
`I2C_ERROR` instead of `DEVICE_NOT_FOUND`. Fix: on a short read, re-probe the
address and map that result, falling back to `I2C_NACK_UNKNOWN_PHASE` (which
exists for exactly this case) when the device does acknowledge.

**E7 — An aborted `wire->write()` left the transmission open.** Harmless on
arduino-esp32, wrong on cores that keep buffer state. Fix: close it.

**E8 — `initWire()` discarded `Wire.begin()`'s result** and always returned
`true`, so the `setup()` failure check could never fire.

**E9 — Hardcoded 128-byte Wire buffer limit** replaced with
`I2C_BUFFER_LENGTH`/`BUFFER_LENGTH` with a conservative fallback.

**E10 — Raw `wreg16` never resynchronized.** `writeRegister16()`'s contract says
to follow a raw write with `invalidateHardwareState()`; neither CLI did, so the
driver kept reporting `SYNCHRONIZED` with stale cached settings. Both CLIs now
invalidate and tell the operator to run `recover`.

### 2.3 Tooling and CI guards

**T1 — The help-item guard could be satisfied by a different command. [proven]**
`check_idf_example_contract.py` tested `f'printHelpItem("{command}'` — no closing
quote — so `printHelpItem("tempco"` satisfied the requirement for `temp`. The
same hole existed for `read`→`ready`, `reset`→`reset_start`, `diag`→`diagraw`,
`scan`→`scanina`. Deleting `printHelpItem("temp", …)` from **both** CLIs left
every check green. Fix: match against the parsed alias set, as
`check_cli_contract.py` already did. Verified: the guard now fails on that
deletion.

**T2 — Dead guards removed.** `IDF_EXAMPLE_MACRO` (`INA228_EXAMPLE_PLATFORM_IDF`
appears nowhere in the tree) and `CLI_SOURCE_INCLUDE` (a `.cpp`-includes-`.cpp`
trick the split example can no longer express).

**T3 — Guards that anything could satisfy.** The CMake `INCLUDE_DIRS` check was
line-bound (`[^\r\n]*`) although CMake argument lists span lines; the CI tokens
`esp32s3`/`esp32s2` were already satisfied by the PlatformIO matrix rows
`esp32s3dev`/`esp32s2dev`, so deleting the entire ESP-IDF job would have passed.
Both tightened.

**T4 — `cfg`/`settings` guard matched a local variable.** `re.search(r"\bcfg\b")`
hit `INA228::Config cfg;` 27 times, so deleting the CLI command entirely would
still pass. Fix: check the alias set.

**T5 — `check_owner_contract.py` scanned prose.** Its forbidden-token sweep ran
over 62 KB of Doxygen comments, so a doc comment saying "this header has no
dependency on `Arduino.h`" would fail the build.
`check_core_timing_guard.py` already strips comments and string literals; the
owner guard now does the same.

**T6 — `drv` poisoned every later step after one transient error. [proven]**
`FAILURE_PATTERNS` matched `Total failures: 3`, `Last error: 400 ms ago` and
`Error code: I2C_TIMEOUT` — all *lifetime/historical* fields. One recovered
glitch therefore classified every subsequent `drv` step as `FAIL`, forever,
including all 5900+ iterations of a documented 8-hour soak. Fix: strip the
historical lines before failure scanning. The live signal
`Consecutive failures: N` is deliberately kept.

**T7 — `--require-framed` did nothing.** It appears in every documented release
command but had no effect on any verdict; a lost frame was only ever downgraded
to `UNKNOWN`. It now promotes a missing or mismatched frame to `FAIL`, and
`docs/CLI.md` says so.

**T8 — A serial exception discarded the whole run.** `open()` was outside any
`try`, and the report/transcript writes were after the `with` block, so a
mid-soak `SerialException` lost 8 hours of evidence. Fix: `try/finally`, with
the report and transcript written in the `finally`, plus a clean exit code and
message on an unopenable port.

**T9 — Dry-run undercounted NOT RUN rows** (4 printed, 5 produced), including in
the exact CI invocation. **T10 — `HIL_END` was searched from offset 0** rather
than from the frame start. **T11 — `generate_version.py`** had an unreachable
error branch; the surviving `main([])` PlatformIO hook is now commented.
**T12 —** `3.0.2` fixture strings in the parser self-tests replaced with `9.9.9`
so they cannot be misread as a live version pin.

### 2.4 Tests

The Wire stub declared `begin()` as `void`; arduino-esp32 returns `bool`, which
is why E8 could not be fixed without touching the stub. Four regression tests
were added, covering D1, D2+D3, D4+D6 and D7.

---

## 3. Documentation cleanup

- **Deleted 227 KB of generated hardware reports** under
  `docs/validation/hardware/2026-07-31/…` — 856- and 5940-row machine-generated
  tables (100 identical benchmark rows, 5932 identical soak rows) for a commit
  (`4c32312` + dirty migration changes) that is not reachable from `main` and is
  unreproducible by construction. Both listed `Operator: Codex` and referenced
  transcripts that had already been removed. Every durable fact — date, board
  serial, fixture, PASS/FAIL/UNKNOWN/NOT RUN counts, duration, scope caveats —
  is preserved in `docs/validation/hardware-evidence.md`, which is now the sole
  record. `library.json` and `ci.yml` already excluded these from the package.
- **Deleted `tools/INA228_HIL_COMMAND_SEQUENCE.md`** — nothing linked to it, and
  ~70% duplicated `docs/CLI.md` and the validation procedure. Its unique content
  (the concrete `xfer_assert` transfer-budget sequences) moved into
  `docs/validation/hardware-validation-procedure.md`, where the "Cooperative
  transfer budgets" row already pointed. Removed from the Doxygen input set,
  which had been publishing an internal procedure template as API reference.
- **`AGENTS.md`** — removed the LLM role prompt ("You are a professional embedded
  software engineer…", "These rules are binding"), the agent-directed git and
  "stop and report" instructions, and two generic assistant-behaviour rules.
  Replaced the "Driver Architecture: Managed Synchronous Driver" section, which
  described the pre-v3 model and contradicted the actual API, with the
  cooperative model plus `HardwareState` and the `HealthPolicy::PASSIVE`
  default. Corrected the identity rule (DIEID `0x228` + revision mask, not
  `DEVICE_ID == 0x2281`), the lifecycle line, and the repository tree, which was
  missing `docs/`, `tools/`, `scripts/`, `test/`, `.github/` and the entire
  ESP-IDF example.
- **`docs/validation/validation-status.md`** — recorded that tag `v3.0.3` points
  at `6137ee7`, one commit after the CI-verified `dc05bd7`, and what differs;
  collapsed three transient-worktree HIL rows into one, dropping the
  "temporary report was removed after review" narrative.
- **`docs/CLI.md`** — repaired a severed lead-in before the status table,
  documented the real argument ranges (`ready_step <1..255>`,
  `tempco [0..16383]`, `stress [1..100000]`, the `vshunt` alias), escaped the
  pipes in eight table cells that were breaking the rendered tables, corrected
  the `cal` rejection attribution, and documented `--require-framed`.
- **Deduplication** — the long HIL runner command existed in four places with
  drifting `-Raw` usage; the two example READMEs now link the canonical form in
  `docs/CLI.md`. `hardware-evidence.md`'s duplicate release-gate list was
  dropped in favour of `validation-status.md`.
- **Build/config** — removed `docs/reports/` (deleted in v2.0.0) from
  `.gitignore` and `library.json`; removed the never-tracked
  `docs/_pdf_extracts` Doxygen exclude and the `[Makefile]` `.editorconfig`
  section; added `.gitignore` entries for the artifacts the release checklist
  tells you to produce locally (`INA228-*.tar.gz`, `package-smoke/`,
  `static-contract-logs/`, `hil-*.md`, `hil-*.txt`); reworded the agent-directed
  error message in `scripts/pio.cmd`; documented the undocumented ESP-IDF
  `>= 6.0.0` floor in `README.md` and `docs/integration/esp-idf.md`; added the
  `sync` subcommand to the README's version-tool list.

---

## 4. Open issues, with proposed fixes

Ranked by severity. Each was traced through the code; the first two were
reproduced.

### O1 — Cached mode diverges from ADC_CONFIG after a triggered conversion **[proven]**

When CNVRF is observed, `_captureDiagAlert()` calls
`_completeTriggeredConversion()`, which forces `_config.mode = Mode::SHUTDOWN`.
The datasheet does not clear the MODE field: §7.3.4 describes MODE as a plain
R/W field whose write "will interrupt and restart triggered or continuous
conversions", and the device powers down after the conversion without altering
the register. So the cache now says SHUTDOWN while ADC_CONFIG still holds
`TRIG_*`:

```
trigger: IN_PROGRESS
ready:   OK ready=1
cached mode=0x0, hw ADC_CONFIG=0x7B68 (mode nibble 0x7)
startVerify: OK
pollJob:  CONFIG_MISMATCH   detail=0x17B68
```

`startVerifyConfiguration()` fails and drops the driver to `RESYNC_REQUIRED`
after a perfectly normal triggered read. Worse, any later ADC_CONFIG-touching
setter (`setAveraging`, `setVbusConvTime`, …) writes MODE=0, silently shutting
the ADC down.

This is deliberate — four existing tests assert `snapshot.mode == SHUTDOWN` —
so it needs a decision, not a unilateral change.

**Proposal.** Keep the cached mode equal to what is actually in ADC_CONFIG, and
track "the triggered conversion has completed" in the existing `_trigPending`
flag, which already carries exactly that meaning:

1. Delete the `_config.mode = Mode::SHUTDOWN` assignment (and the now-unused
   `wasTriggeredMode` local) from `_completeTriggeredConversion()`.
2. `getMode()`/`SettingsSnapshot::mode` then report `TRIG_*` with
   `triggeredConversionPending == false`, which is both truthful about the
   register and unambiguous about the device being idle. Document that pairing
   in `INA228.h` and in `docs/reference/ina228-device-reference.md`, whose
   "Triggered modes perform one conversion sequence and return to shutdown" line
   should say the *device* powers down while the MODE field retains its value.
3. Update the four assertions (`test_basic.cpp:1285, 1360, 1389, 1758`) to expect
   the triggered mode plus `triggeredConversionPending == false`.
4. Add a regression test that a completed triggered conversion is followed by a
   successful `startVerifyConfiguration()`.

`estimateConversionTimeUs()` will then return the triggered-mode time instead of
0 after completion; that is only consumed by `_triggerDeadlineElapsed()` behind
`_trigPending`, so no behaviour depends on the current zero.

### O2 — The instantaneous-sample job hard-fails when CNVRF is late **[proven]**

`SAMPLE_WAIT` waits the nominal computed time, then `SAMPLE_DIAG` fails the whole
job with `MEASUREMENT_NOT_READY` if CNVRF is not yet set. There is no re-wait.
The datasheet specifies conversion times as *typical*, footnoted "subject to
oscillator accuracy and drift", with the internal oscillator at ±1 % over
−40 °C to +125 °C. At `AVG_1024` with `4120 µs` conversions the nominal wait is
12657 ms and the +1 % worst case is 12783 ms — the deadline expires ~126 ms
early and the job fails on correct hardware.

The polling path self-corrects (the caller just polls again); only the job path
is affected.

**Proposal.** Add a bounded oscillator margin and one re-wait:

1. In `_instantaneousSampleWaitUs()`, scale the conversion component by 1/64
   (≈1.6 %, comfortably above the datasheet's ±1 %) before adding CONVDLY and
   the shutdown wake-up. Keep it integer-only.
2. In `SAMPLE_DIAG`, when MEMSTAT is healthy but CNVRF is clear, allow one
   return to `SAMPLE_WAIT` (guarded by a single `bool _sampleReWaited`) before
   failing. `JobLimits::maxWaitMicroseconds` becomes twice the computed wait, so
   the declared bound stays exact and the transfer count is unchanged.

Alternative, if a fixed bound is preferred over a re-wait: apply only step 1 and
document that owners of very slow profiles should widen their deadline.

### O3 — MATHOF never clears, so current/power reads stick at `MATH_OVERFLOW` **[proven]**

Datasheet Table 7-16: MATHOF *"must be manually cleared by triggering another
conversion or by clearing the accumulators with the RSTACC bit."* It is **not**
cleared by reading DIAG_ALRT. `_readAndValidateMathDiag()` returns
`MATH_OVERFLOW` whenever MATHOF is set, so once it latches, every
`readCurrent()`, `readPower()`, `readMeasurement()` and `readIntegerSample()`
fails permanently:

```
readCurrent #0 -> MATH_OVERFLOW
readCurrent #1 -> MATH_OVERFLOW
readCurrent #2 -> MATH_OVERFLOW
resetAccumulators -> OK
readCurrent after RSTACC -> OK
```

The recovery path exists but is undocumented, and several Doxygen comments call
the DIAG_ALRT read "destructive", which implies a clear that does not happen for
this bit.

**Proposal.** Documentation plus one message, no behaviour change:

1. In `INA228.h`, on `readCurrent`, `readPower`, `readMeasurement` and
   `readIntegerSample`, state that MATHOF is latched until a new triggered
   conversion or `resetAccumulators()` (CONFIG.RSTACC), and that reading
   DIAG_ALRT does not clear it.
2. Change the status text in `_readAndValidateMathDiag()` from
   `"INA228 math overflow"` to
   `"INA228 MATHOF latched; clear with resetAccumulators()"`.
3. Add the same note to the "Alert And Diagnostic Notes" section of
   `docs/reference/ina228-device-reference.md`, which currently says only that
   MATHOF "is a separate diagnostic flag".

### O4 — A transient read failure during read-only verification destroys the accumulator epoch

`_failJob()` invalidates hardware state for `JobKind::VERIFY_CONFIGURATION`
unconditionally. A single NACK on the first MANUFACTURER_ID read — a job that
writes nothing — sets `RESYNC_REQUIRED`, clears `_initialized`, and calls
`_invalidateAccumulatorEpoch()`. Recovery then costs a 14-transfer reinit plus a
2-transfer accumulator reset, and the energy/charge baseline is gone. On a noisy
bus this is self-amplifying.

**Proposal.** Separate "verification was disproved" from "verification could not
be completed":

```cpp
const bool verificationDisproved =
    _jobSnapshot.kind == JobKind::VERIFY_CONFIGURATION &&
    (status.code == Err::CONFIG_MISMATCH || status.code == Err::DEVICE_ID_MISMATCH ||
     status.code == Err::MEMORY_ERROR || status.code == Err::UNSUPPORTED_REVISION);
```

Use `verificationDisproved` in place of the bare kind check. For a transport
failure with no successful write, set `_hardwareState = HardwareState::UNKNOWN`
and leave `_initialized`, `_accumulationReady` and the accumulator generation
alone — the owner can simply re-run the verification. Add a test asserting that
a single injected `I2C_TIMEOUT` on the first verify read preserves a valid
energy epoch.

### O5 — Raw DIAG_ALRT probes steal CNVRF from a pending triggered conversion

Both CLIs' `probeAddressRaw()` (used by `probe`, `scan`, `scanina` and the IDF
`selftest`) reads DIAG_ALRT through the raw transport, bypassing
`_captureDiagAlert()`. Since CNVRF is clear-on-read and the driver's readiness
path is CNVRF-authoritative with no time-only fallback, the sequence
`trigger 7` → `probe` → `vbus` leaves `_trigPending` set forever; every read
returns `MEASUREMENT_NOT_READY` until a new trigger or `setMode`.

The driver documents this hazard for external consumers, so the fix belongs in
the examples. It is left open because every candidate changes probe semantics
that the HIL contract asserts.

**Proposal (preferred).** In `probeAddressRaw()`, when the probed address is the
bound address and the driver is initialized, take the MEMSTAT reading from
`device.readDiagAlertRaw()` instead of the raw helper. The driver then captures
the evidence and completes the pending trigger, and the transfer count is
unchanged (one callback either way). The one behavioural difference is that the
read becomes health-tracked; if the Arduino `selftest`'s "probe has no health
side effects" assertion must hold, scope the change to `scan`/`scanina`/`probe`
and leave `selftest` on the raw path.

**Proposal (minimal).** Keep the raw read, but have the CLI check
`getSettings().triggeredConversionPending` first and print an explicit warning
that the probe will consume CNVRF and the trigger must be reissued.

### O6 — Two example commands with the same name do different things

`stress_mix` exercises 7 operations and counts only `st.ok()` as success in the
Arduino CLI, but 8 different operations accepting `ok || inProgress ||
MEASUREMENT_NOT_READY` in the ESP-IDF CLI. `stress_mix` after `trigger`
therefore records failures on one firmware and successes on the other.
`selftest` differs similarly: the Arduino version exercises `device.probe()` and
a plausible-range check, the IDF version does neither and instead checks
`setAveraging`/`setMode`. `docs/CLI.md` claims the two CLIs have an identical
command contract, and the contract checker compares help rows but not
behaviour.

Related: `wireWriteReadAt` increments the Arduino read counter while
`ina228IdfI2cWriteReadAt` does not, so `xfer_assert <r> <w> <t>` needs different
expected numbers per platform after any `probe`/`scanina`/`selftest`.

**Proposal.** Pick one operation list and one accept predicate for `stress_mix`,
and one check list for `selftest`; make the IDF address-probe helper increment
`gTransferStats.read` like its Arduino counterpart. Then extend
`check_idf_example_contract.py` to compare the operation-count constants and the
accept predicate tokens, so the two cannot drift again.

### O7 — ESP-IDF transport never maps an unexpected NACK

`Ina228IdfI2cTransport.cpp` maps `ESP_ERR_INVALID_RESPONSE` to
`I2C_NACK_UNKNOWN_PHASE`, but that code belongs to `i2c_master_probe()` (already
handled correctly in `mapEspProbeErr`). `i2c_master_transmit()` /
`i2c_master_transmit_receive()` surface an unexpected NACK differently, so a
NACKing device falls through to the `I2C_BUS` fallback — which also contradicts
`docs/integration/esp-idf.md:135` ("other transport failure → `I2C_ERROR` with
`esp_err_t` in `detail`").

Left open because the exact enum must be confirmed against the pinned ESP-IDF
v6.0.1 headers, which are not available in this checkout.

**Proposal.** Confirm the return code for an unexpected NACK in IDF v6.0.1, add
an explicit branch mapping it to `I2C_NACK_UNKNOWN_PHASE`, and change the
fallback from `I2C_BUS` to `I2C_ERROR` so code and documentation agree.

### O8 — `limits` and `pwrlim` decode with an unusable CURRENT_LSB

Both CLIs gate the PWR_LIMIT decode on `device.currentLsb() > 0.0f`, but
`currentLsb()` documents that `SettingsSnapshot::calibrated` and
`hardwareDirty` must be checked first. After a failed SHUNT_CAL write (the
`setAdcRange` rollback path) the value is non-zero but stale, and `limits`
prints a wrong wattage with no warning. Separately, `limits` decodes shunt
thresholds with the *current* `adcRange` while ignoring the sticky
`thresholdsDirty` advisory, so after `adcrange 1` the printed mV misrepresents
what is programmed. `cfg` prints the flag; `limits` does not.

**Proposal.** In both `printAlertLimits()` implementations, fetch
`getSettings()` once, decode power only when `calibrated && !hardwareDirty`
(printing "requires valid calibration" otherwise), and prefix the block with an
explicit warning when `thresholdsDirty` is set.

### O9 — Smaller items

| # | Item | Proposal |
| --- | --- | --- |
| a | `JobKind::VERIFY_CONFIGURATION` is handled in both CLIs' job switch but no command reaches it. | Add `verify_start`/`verify_step` (and the matching help rows and doc entries), or drop the case. |
| b | An unconsumed terminal result makes every hardware API return `BUSY "Cooperative job owns hardware access"` even though no job is active. | Keep the behaviour; use a distinct message such as "Unconsumed terminal result; call takeJobResult()". |
| c | `--legacy-marker`, `--marker-retries`, `strip_hil_marker()` and the two `hilmark` CLI handlers implement a retired framing protocol that no doc, CI step or stored report uses. | Remove the flag, the retry loop, the helper and both CLI handlers, or add a self-test that exercises the path. |
| d | The `hilrun` sequence number is hardcoded to `"0"`, so the frame's sequence check can never distinguish this reply from a previous one; identity rests entirely on the timestamp token. | Thread an incrementing counter through `run_step`. |
| e | `classify_output()` is only used by the parser self-test; production classification goes through `classify_step()`. The CI parser gate therefore exercises a function no hardware run uses. | Rewrite the ten self-test cases against `classify_step()` and delete `classify_output()`. |
| f | Board constants (SDA 8, SCL 9, 400 kHz, 50 ms, 0x40) and the demo calibration profile are duplicated by hand between `examples/common/BoardConfig.h` and the IDF example. | Deliberate (the IDF example must not include Arduino glue) — have `check_idf_example_contract.py` assert the values match. |
| g | `resetAccumulators()` has no `staged.inProgress()` fallback, unlike `begin()` and `recover()`. Unreachable today with a 2-transfer budget. | Add the same cancel-and-drain fallback for symmetry. |
| h | `cmd::REG_WIDTH_16/24/40` are never referenced. `JobLimits::maxRetries` is never assigned (it relies on its default and is documented as always zero). | Removing the width constants is a (minor) public-header break — schedule for the next major, or keep them as documentation. |
| i | `docs/CLI.md` cannot join the Doxygen input set: its `#status-sensitive-and-raw-access` anchor resolves on GitHub but not in Doxygen, and `WARN_AS_ERROR = YES` then fails the build. This is why `README.md:286` links it with raw HTML. | Either drop the intra-page anchor link at `docs/CLI.md:70`, or keep the file out of `INPUT` and standardise on the HTML link form (what the tree does today). |
| j | `validation-status.md` lists "exhaustive cppcheck with zero findings" as a passed static guard, but no CI job runs cppcheck and the release checklist does not list it. | Add cppcheck to `ci.yml` and the checklist, or scope the claim to the dated local run it describes. |
| k | The legacy calibration path divides max current by 524288 (`2^19`, the datasheet's Equation 3) while the fixed-unit planner divides by 524287 and rounds up. | Deliberate and documented in the device reference; no change, but keep the two in step if either is touched. |
| l | `_updateHealth()` ignores every transfer issued while `_initialized == false`, so initialization and recovery traffic is invisible to the health counters. | Deliberate — `test_begin_success_sets_ready_without_health_counts` encodes it, and it stops recovery attempts from inflating the counters that gate recovery. A comment was added; consider exposing a separate recovery-attempt counter if owners need it. |

---

## 5. Datasheet contradictions worth knowing

Found while cross-checking; the driver already follows the correct
interpretation in each case.

- **TEMP_LIMIT / PWR_LIMIT reset values.** Table 7-1 lists TEMP_LIMIT default
  `0xFFFF` and PWR_LIMIT `0x7FFF`; §7.6.1.17/§7.6.1.18 state the opposite
  (`0x7FFF` and `0xFFFF`). The register sections are self-consistent
  (`0x7FFF` = max positive signed temperature, `0xFFFF` = max unsigned power) and
  are what `CommandTable.h` encodes. Table 7-1's last two rows appear transposed.
- **SHUNT_TEMPCO full scale.** §7.3.5 says 16384 ppm/°C; Table 7-8 says 16383
  (`0x3FFF`), which is arithmetically correct for a 14-bit field at 1 ppm/°C/LSB.
  `TEMPCO_MAX = 16383` is right.
- **CNVRF clear-on-read.** §7.3.4 states it unconditionally; Table 7-16 qualifies
  it with "When ALATCH = 1". The driver treats it as always clear-on-read, which
  is the safe reading.
- **DIAG_ALRT status-bit writability.** Table 7-1 marks the status bits RO;
  Table 7-16 marks them R/W, and the datasheet never says what writing them
  does. The driver writes only the four config bits, which unavoidably writes 0
  to MEMSTAT; hardware runs show this is harmless.
- **No documented atomicity** for multi-byte 24-bit/40-bit reads, and no
  specified settling time for the software `RST` bit (only `T_POR` = 300 µs for
  power-up, which is what `POR_STARTUP_US` uses).

---

## 6. Verification performed

| Gate | Result |
| --- | --- |
| `pio test -e native` | 121/121 passed (117 existing + 4 new) |
| `pio run -e esp32s3dev` | SUCCESS |
| `pio run -e esp32s2dev` | SUCCESS |
| `check_cli_contract.py` | PASSED |
| `check_idf_example_contract.py` | PASSED (and now fails on a deleted help row) |
| `check_core_timing_guard.py` | PASSED |
| `check_owner_contract.py` | PASSED |
| `run_i2c_hil.py --parser-self-test` | PASSED |
| `test_run_i2c_hil_parser.py` | PASSED (7 groups) |
| `run_i2c_hil.py --dry-run --suite exhaustive` | PASSED, NOT RUN count now matches a real run |
| `generate_version.py check` | Up to date |
| `doxygen Doxyfile` (`WARN_AS_ERROR = YES`) | Clean |

Not run: ESP-IDF `idf.py` builds (`IDF_PATH` unset in this environment) and any
hardware-in-the-loop run.
