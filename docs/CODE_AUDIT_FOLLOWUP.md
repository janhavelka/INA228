# Code audit follow-up — verification of the 2026-08-30 resolution

Reviewed tree: `a41646c` (`main`, in sync with `origin/main`).
Reviewed against: [`CODE_AUDIT.md`](CODE_AUDIT.md) and its disposition record
[`CODE_AUDIT_RESOLUTION.md`](CODE_AUDIT_RESOLUTION.md).

This file is a task list for an AI coder. Delete it once every item is
resolved or explicitly declined.

---

## What was verified as genuinely done

The resolution work is substantially real. Independently re-proved by running
code against the committed tree, not by reading the resolution document:

| Item | Evidence |
| --- | --- |
| **O1** triggered MODE cache | Repro prints `cached mode=0x7, hw ADC_CONFIG=0x7B68 (nibble 0x7)`; `startVerifyConfiguration()` + `pollJob()` now return `OK` (previously `CONFIG_MISMATCH detail=0x17B68`). |
| **O2** sample timing margin | `ceil(nominal/64)` = 1.5625 %, above the datasheet's ±1 % oscillator spec; declared `maxWaitMicroseconds` matches the actual wait exactly; still 11 transfers, one DIAG read, zero retries; no uint32 overflow at the maximum profile. |
| **O3** MATHOF | All four latched-MATHOF paths now return the identical actionable message. |
| **O4** verify-job invalidation | Inconclusive transport errors preserve `SYNCHRONIZED` + identity + accumulator epoch; `CONFIG_MISMATCH`, identity/revision/MEMSTAT and definite address NACK still invalidate. |
| **O5** raw DIAG probes | Bound-address probes route through `readDiagAlertRaw()` in both CLIs, guarded on `isInitialized()`; `trigger` → `probe` → `vbus` no longer wedges. |
| **O6** example parity | `STRESS_MIX_OPERATIONS`, the accept predicate and the 17 self-test checks are now identical; the IDF read counter is fixed. |
| **O7** ESP-IDF NACK mapping | The original finding's premise was wrong; `ESP_ERR_INVALID_RESPONSE` really is the transmit NACK result in IDF v6.0.1. The correction is right. |
| **O8** limit decoding | Gate uses `SettingsSnapshot::calibrated`, which is *stronger* than what the audit proposed. |
| **D1–D9** | All present and effective. |
| **T1, T3–T6, T8–T12** | Guard mutations were executed: each fails when the fix is reverted. |
| **O9a–O9f** | Verified, including eight executed mutations of the demo-profile comparison. |

**Mutation testing on the driver** (each mutation applied, suite run, then
reverted): removing the O2 margin fails 1 test, restoring the O1 mode rewrite
fails 4, deleting the D2 threshold advisory fails 1, and reverting the O4 policy
fails 2 *in both directions*. The fixes are pinned by regressions, not merely
present.

Full gate status on `a41646c`: 126/126 native tests, both Arduino builds,
all four static contracts, HIL parser self-test + 9 regression groups,
version check, Doxygen warnings-as-errors, and a clean `pio pkg pack`.

---

## Task 1 — [HIGH] A failed DIAG read in a verify job permanently strands a pending conversion

**This is a new regression introduced by the O4 fix.**

`_failJob()` classifies the `READ_DIAG` transfer as side-effecting because the
read destroys CNVRF (`src/INA228.cpp:1021`, `:1120`). For a verification job that
flag now only sets `JobEffect`; it no longer influences invalidation
(`src/INA228.cpp:778-784`). The clearing of trigger timing at
`src/INA228.cpp:788-790` is still gated on `identityJob || resetMayHaveInvalidatedTrigger`,
neither of which covers a verify job. So the device consumes CNVRF, the read
fails, and the driver keeps believing the conversion is pending.

Reproduced, and confirmed to be a behaviour change:

```
=== BEFORE (e3e21aa, pre-O4) ===
verify terminal: I2C_TIMEOUT  init=0 hwState=3 trigPending=0 effect=3
readBusVoltage after evidence loss: NOT_INITIALIZED        <- recoverable via recover()

=== AFTER (a41646c, post-O4) ===
verify terminal: I2C_TIMEOUT  init=1 hwState=2 trigPending=1 effect=3
readBusVoltage after 3s more polling: MEASUREMENT_NOT_READY <- stuck forever
```

The driver reports `initialized=1`, `SYNCHRONIZED`, `hardwareDirty=0` — it looks
healthy — while every measurement read returns `MEASUREMENT_NOT_READY` until
`setMode()`, `triggerConversion()` or `invalidateHardwareState()`. This is worse
than the pre-O4 behaviour, which at least surfaced an error the documented
recovery path clears.

**Fix.** In `_failJob()`, alongside the existing trigger-timing clear at
`src/INA228.cpp:788-790`:

```cpp
if (verificationJob && failedSideEffectingTransfer && !invalidatesHardwareState) {
  // The DIAG read may have consumed CNVRF before failing. Keep the verified
  // configuration, but stop claiming the conversion is still pending.
  _invalidateTriggeredConversionTiming();
}
```

**Acceptance.** Add a regression next to
`test_verify_configuration_distinguishes_inconclusive_transport_from_disproof`
(`test/test_basic.cpp:3618`): trigger a conversion, run a verify job whose
`READ_DIAG` read fails, then assert `triggeredConversionPending == false`,
`hardwareState() == SYNCHRONIZED`, `initialized == true`, and that
`readBusVoltage()` succeeds. Confirm it fails without the fix.

---

## Task 2 — [MEDIUM] `recover` is classified `FAIL` forever after any historical transient

T6 strips the historical health block only when `step.command == "drv"`
(`tools/run_i2c_hil.py:582`), but `printDriverHealth()` is also called by
`recover` (`examples/01_basic_bringup_cli/main.cpp:2637`,
`examples/esp_idf/basic/main/main.cpp:2081`). `recover` is in both
`FUNCTIONAL_STEPS` and `V3_TARGETED_STEPS`, so it is in the `exhaustive` release
suite — one recovered glitch spuriously fails the release gate.

Reproduced by feeding identical text through the shipped parser:

```
drv     -> PASS
recover -> FAIL
```

**Fix.** Key the strip on the block itself, not the command name. `=== Driver
Health ===` is emitted from exactly one function, and `settings` does not print
those fields, so the exemption stays narrow:

```python
scanned = HISTORICAL_HEALTH_RE.sub("", text) if "=== Driver Health ===" in text else text
```

**Acceptance.** Extend the `classify_step` self-test cases in
`tools/run_i2c_hil.py` with a `recover` step whose output carries the health
block with `Total failures: 3` / `Last error:` / `Error code:` and
`Consecutive failures: 0` → `PASS`, and the same with
`Consecutive failures: 2` → `FAIL`.

---

## Task 3 — [MEDIUM] The native-IDF forbidden-token guard still inspects only two hardcoded files

`tools/check_idf_example_contract.py:286-295` enumerates every C/C++ source under
`examples/esp_idf/basic/main/` into `idf_sources`, but only two checks run over
it. `FORBIDDEN_IDF_TOKENS` is still applied at `:224` (main.cpp) and `:282`
(the transport) only. `CODE_AUDIT_RESOLUTION.md` claims "Scan every native-IDF
source", which is not what the code does.

Reproduced against a `git archive` export of `a41646c`: adding
`examples/esp_idf/basic/main/extra/Helper.cpp` containing `#include <Arduino.h>`,
`#include <Wire.h>`, `static String gBuffer;`, `TwoWire* gWire = &Wire;` and
`Serial.println(...)` leaves both guards green:

```
--- with rogue Arduino file under native-IDF main/ ---
IDF example contract PASSED
Core timing guard PASSED
```

The identical tokens inside `main.cpp` fail immediately, so the guard is exactly
one new file away from being bypassed.

**Fix.** After `tools/check_idf_example_contract.py:295`:

```python
for token in FORBIDDEN_IDF_TOKENS:
    if token in idf_sources:
        fail(f"native ESP-IDF sources contain forbidden Arduino/facade token '{token}'")
```

**Acceptance.** The rogue file above must fail the check; the unmodified tree
must still pass.

---

## Task 4 — [MEDIUM] `--require-framed` has no regression covering its actual wiring

The only coverage is `tools/run_i2c_hil.py:766`, a unit test of the pure helper
`missing_frame_verdict()`. Disconnecting `run_step` from the flag at
`tools/run_i2c_hil.py:983` — replacing `missing_frame_verdict(args.require_framed)`
with `missing_frame_verdict(False)` — leaves the parser self-test and all nine
standalone regression groups green while the documented release gate
(`docs/CLI.md:291`, `:299`) silently stops failing.

**Fix.** Add a group to `tools/test_run_i2c_hil_parser.py` that drives `run_step`
with a fake serial returning plausible output but no `HIL_BEGIN`/`HIL_END`,
asserting `UNKNOWN` with `require_framed=False` and `FAIL` with
`require_framed=True`. Verify it fails when line 983 is disconnected.

---

## Task 5 — [LOW-MEDIUM] Undocumented API behaviour change from O1

Because the cached mode now stays `TRIG_*`, `startInstantaneousSample()` and
`readPowerSampleRawStep()` return
`INVALID_CONFIG "Instantaneous job requires shutdown or continuous base mode"`
after *any* completed triggered conversion (`src/INA228.cpp:709-712`). The
rejection is correct — `SAMPLE_RESTORE_ADC` would otherwise write a `TRIG_*` mode
without arming the trigger, recreating a cache/hardware divergence — but owners
must now call `setMode()` back to a continuous or shutdown mode first, and
nothing says so.

**Fix.** One sentence in the `startInstantaneousSample()` Doxygen block and one
in the `CHANGELOG.md` O1 bullet.

---

## Task 6 — [LOW] Audit documents ship to library consumers, unguarded

`library.json` excludes `docs/CODE_AUDIT.md` but not `docs/CODE_AUDIT_RESOLUTION.md`.
Confirmed against a real `pio pkg pack`:

```
docs/CODE_AUDIT_RESOLUTION.md      <-- ships
```

`ci.yml:196-207`'s forbidden-prefix list covers neither, so the existing
exclusion is unguarded too.

**Fix.** Add `docs/CODE_AUDIT_RESOLUTION.md` and `docs/CODE_AUDIT_FOLLOWUP.md`
(this file) to `library.json` `export.exclude`, and add `docs/CODE_AUDIT` to the
negative-grep prefix list in `.github/workflows/ci.yml`. Also add all three to
`docs/README.md` or leave them out deliberately — they are currently unindexed
orphans in the docs tree.

---

## Task 7 — [LOW] `validation-status.md` is stale and no longer the claim authority

`docs/validation/validation-status.md` still reads `Last reviewed: 2026-08-27`,
still claims `passed 117/117 ... on 2026-08-04`, and describes only the v3.0.3
release candidate. None of the post-3.0.3 work in `[Unreleased]` is covered, and
the suite is now 126 tests. This is the repository's own claim-authority
document, so staleness here is the failure mode it exists to prevent.

**Fix.** Refresh the date, the native-test count and the gate rows to describe
the `a41646c` validation actually performed, or add a row stating explicitly
that the post-3.0.3 work has local-only evidence.

---

## Task 8 — [LOW] `CHANGELOG.md` `[Unreleased]` is duplicated and written in two voices

The section is 84 lines. Four topics appear twice, once in the follow-up's
summary style and once in the earlier audit's style:

| Topic | Duplicate bullets |
| --- | --- |
| Reset marks thresholds dirty | "A reset marks alert thresholds dirty before the reset write…" and "A successful cooperative software reset now sets `SettingsSnapshot::thresholdsDirty`…" |
| Full reset dirty-register set | same first bullet and "A reset job now records every register the reset restores…" |
| First dirty cause preserved | same first bullet and "`invalidateHardwareState()` no longer clears `dirtyRegisterMask`…" |
| Example / tooling fixes | "Arduino and native ESP-IDF `probe`, `stress_mix`…" and "Static guards now parse multiline…" vs the trailing "Examples: …" and "Tooling: …" bullets |

**Fix.** Merge into one voice, one bullet per user-visible behaviour change,
before the next release.

---

## Task 9 — [LOW] Remaining smaller items

| # | Item | Fix |
| --- | --- | --- |
| 9a | Dry-run vs real-run NOT RUN parity holds only with `--include-not-run`: `run_serial` emits the rows when `args.include_not_run or report_path is not None` (`tools/run_i2c_hil.py:1361`) but `print_plan` is gated on `args.include_not_run` alone (`:1492`). Same bug class T9 fixed, surviving in another flag combination. | Pass `args.include_not_run or bool(args.report)` into `print_plan`. |
| 9b | `check_owner_contract.py`'s literal stripper consumes `#include "Arduino.h"`, so the quoted form passes while `<Arduino.h>` fails. Defense in depth holds — `check_core_timing_guard.py` catches it on the raw text. | Run the include regex over unstripped text in `check_owner_contract.py` too. |
| 9c | `--parser-self-test` and `--dry-run` abort outside a git worktree, because provenance validation (`tools/run_i2c_hil.py:1478`) runs before the dispatch (`:1489`). Harmless in CI, a trap from a source export. | Move the dispatch above the provenance validation, or skip it when either flag is set. |
| 9d | `bovl`, `buvl` and `tmplim` print no `thresholdsDirty` advisory, although `limits` does. Their decode uses fixed LSBs so the numbers are right, but after a reset the "needs reapplication" signal is silently absent. | Hoist the advisory into a shared helper used by `printBusAlertLimit` and `printTemperatureAlertLimit` in both CLIs. |
| 9e | `Ina228IdfI2cTransport.cpp:302-309`: when both the transfer and the follow-up `i2c_master_bus_rm_device()` fail, `cleanupError` is discarded and `tempDev` goes out of scope while the SDK still owns it — an unrecoverable handle leak, up to 15 per `scanina` on a faulty bus. | Retain the handle in the context for a retry, or return the cleanup status. |
| 9f | The Arduino short-read re-probe and the empty discard transaction put real traffic on the wire without incrementing `transferStatsStorage()`, so `xfer_assert` under-reports during fault injection. | Count them, or document the exemption. |
| 9g | `convertRawSample()`'s Doxygen has no MATHOF note and does not document its `MATH_OVERFLOW` return; `DiagAlert`, `readDiagAlert()` and `readDiagAlertRaw()` still describe the read as clearing "latched alert flags" without excluding MATHOF/ENERGYOF/CHARGEOF. | Copy the note already present on `readCurrent()`. |
| 9h | `check_idf_example_contract.py:232` matches `idf_component_register\s*\((.*?)\)` and stops at the first `)`; any nested paren truncates the argument list the `INCLUDE_DIRS` check inspects. `:313`'s `count(...) < 5` heuristic cannot tell which command lost its guard. | Balance the parens; assert per-command rather than by count. |
| 9i | `scripts/generate_version.py:380` runs `main([])` on import, *writing* `Version.h`, `idf_component.yml` and `Doxyfile`. `py_compile` does not import, so CI is safe, but any tool that imports the module mutates the tree. | Guard the side effect behind an explicit entry point. |
| 9j | Residual example divergence not covered by O6: `stress`, `scan`, `scanina` and `recover` still print different text on the two platforms, and `verbose` inverts meaning inside `stress` (Arduino logs failures, IDF logs successes). | Align, or extend the contract checker the way it now compares `STRESS_MIX_OPERATIONS`. |

---

## Release decision to make (not a defect)

The public API surface is unchanged since `v3.0.3` — no declarations removed,
two new private methods. But `getMode()` and `SettingsSnapshot::mode` now report
`TRIG_*` where they previously reported `SHUTDOWN` after a completed triggered
conversion, and `startInstantaneousSample()` now rejects in that state (Task 5).
`library.json` is still at `3.0.3` with everything in `[Unreleased]`.

Decide whether that observable change is a MINOR with a prominent migration note
or warrants a MAJOR, and bump before tagging.

---

## How to verify the whole set

```
.\scripts\pio.cmd test -e native
.\scripts\pio.cmd run -e esp32s3dev
.\scripts\pio.cmd run -e esp32s2dev
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python tools/check_core_timing_guard.py
python tools/check_owner_contract.py
python tools/run_i2c_hil.py --parser-self-test
python tools/test_run_i2c_hil_parser.py
python tools/run_i2c_hil.py --dry-run --suite exhaustive --include-not-run --benchmark-count 100
python scripts/generate_version.py check
doxygen Doxyfile
```

For each task, first write the regression that fails against `a41646c`, then
apply the fix, then confirm the regression passes and the full gate list above
stays green. Do not widen any declared transfer bound, add a retry to the core,
or introduce a new production abstraction.
