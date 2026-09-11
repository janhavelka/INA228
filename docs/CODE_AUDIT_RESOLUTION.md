# Code Audit Resolution — updated 2026-09-09

The original review below is retained with its historical context. Tasks 1–9
of the follow-up are committed in `2cc5874`; the final sections record the
2026-09-09 fresh-sweep review and its local validation separately from CI/HIL.

This report records the review and resolution of every finding in
[`CODE_AUDIT.md`](CODE_AUDIT.md) against the current `main` codebase. The
review started from `e3e21aa`; `origin/main` and the local branch were already
at that commit after `git fetch --prune`. The pre-existing move of the audit
from the repository root into `docs/` was preserved.

## Outcome

The audit found real defects, useful documentation gaps, already-correct code,
and a few proposed fixes that were not the simplest correct solution. Changes
were made only where the current implementation or its checked contract needed
them. No retry was added to the core, no transfer bound was widened, and no new
production abstraction was introduced.

### Open findings O1–O8

| Finding | Review result | Resolution |
| --- | --- | --- |
| O1, triggered MODE cache | Valid. The device powers down after a one-shot but retains the triggered `ADC_CONFIG.MODE` field. Rewriting only the cache to shutdown caused a real cache/readback mismatch. | Removed the cache rewrite. `mode` remains `TRIG_*`; `triggeredConversionPending` becomes false on completion. API/device documentation and four regressions were updated, including successful verification after completion. |
| O2, sample timing | Valid, but the proposed second full wait was not appropriate: it would either require another DIAG transfer or make the result latency unnecessarily large. | Added an integer-only `ceil(nominal whole-device interval / 64)` margin over conversion, CONVDLY, and wake-up time. The job still makes one readiness read, has zero retries, and retains its exact 11-transfer bound. Fast-conversion/max-delay, maximum-profile, and deadline-boundary cases are tested. |
| O3, MATHOF recovery | Valid documentation/actionability gap; the existing latch behavior was correct. | Documented that a DIAG read does not clear MATHOF and that another triggered conversion or `resetAccumulators()` is required. All MATHOF status paths now give the same actionable message. |
| O4, verification invalidation | Valid, but setting `HardwareState::UNKNOWN` while leaving initialization valid would contradict the public preconditions, and definite address absence also disproves the verified state. | Inconclusive transport errors during the read-only verify job preserve synchronization, identity, and accumulator epoch. `CONFIG_MISMATCH`, identity/revision/MEMSTAT failures, and definite address NACK/absence invalidate them. Tests cover timeout preservation, continued energy validity, readback mismatch, address disappearance, and trigger-timing invalidation. |
| O5, raw DIAG probes | Valid. | For an initialized bound address, the examples route the DIAG read through `readDiagAlertRaw()` so the driver captures destructive evidence; other addresses stay raw. A cooperative active job blocks probing. Help and probe output describe the behavior. |
| O6, example parity | Valid. | Both CLIs now use the same seven read-only `stress_mix` operations, accept only `Status::ok()`, and run the same self-test checks. Native IDF raw-address reads now increment the read counter. The static contract compares operation lists, predicates, self-test labels, help/dispatch, and demo profile constants. |
| O7, IDF NACK mapping | The main premise was incorrect. ESP-IDF 6.0.1 documents `ESP_ERR_INVALID_RESPONSE` as the unexpected-NACK result for the master transmit APIs, so the existing phase-unknown NACK branch was reachable and correct. The fallback mismatch was valid. | Kept `ESP_ERR_INVALID_RESPONSE -> I2C_NACK_UNKNOWN_PHASE` for transfers and address-NACK mapping for the explicit probe. Changed unclassified transfer/probe failures from `I2C_BUS` to `I2C_ERROR` with the original `esp_err_t` detail, and documented the distinction. See the version-pinned official [ESP-IDF 6.0.1 I2C API](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32/api-reference/peripherals/i2c.html) and [ESP-IDF 6.0 peripheral migration guide](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32/migration-guides/release-6.x/6.0/peripherals.html). |
| O8, limit decoding | Valid for power and for missing advisories. A successful range change does make the cached range equal to hardware, so treating every `thresholdsDirty` state as an unknown range would have hidden a valid raw decode. | Shunt limits are decoded only with synchronized hardware state; power limits only with `SettingsSnapshot::calibrated`. Raw codes are always shown, and `thresholdsDirty` produces an explicit reapplication warning. The same rules apply to `limits`, individual shunt queries, and `pwrlim`. |

### Smaller findings O9

| Item | Result |
| --- | --- |
| O9a | Added and documented `verify_start` / `verify_step`; added the path to the HIL feature sweep. |
| O9b | Added a distinct `BUSY` message for an unconsumed terminal result and regression coverage for both BUSY causes. |
| O9c | Removed both `hilmark` handlers and the runner's legacy marker flag, retries, parser, and documentation. |
| O9d | Valid. On this Windows host `time.monotonic_ns()` uses a 15.625 ms-resolution `GetTickCount64` clock, so adjacent calls frequently repeat. The runner now increments a per-run frame sequence independently of the clock; a frozen-clock regression proves consecutive identities remain distinct. |
| O9e | Parser self-tests now exercise production `classify_step()`; dead `classify_output()` was removed. |
| O9f | Kept native IDF independent of Arduino glue. The contract now parses and compares SDA, SCL, frequency, timeout, address, mode, calibration mode, shunt, and maximum current. |
| O9g | No change. The alleged fallback is unreachable under the exact two-transfer synchronous budget, and adding dead defensive control flow would not improve behavior. |
| O9h | Kept the public width constants and used them in register buffers. `maxRetries` remains a documented, default-zero contract with tests. |
| O9i | No change. `docs/CLI.md` remains outside Doxygen and the existing raw HTML link strategy passes warnings-as-errors generation. |
| O9j | Scoped cppcheck explicitly to the same dated local validation run; it is not presented as a recurring CI gate. |
| O9k | No change. The datasheet-compatible legacy `2^19` divisor and conservative fixed-unit positive-limit planner intentionally serve different contracts. |
| O9l | No behavior change. The source comment now states the actual boundary: initial bring-up and reinitialization after explicit invalidation are excluded, while recovery from a still-initialized session is tracked. |

## Fresh independent re-audit — 2026-08-30

The completed work at `1713f29` was audited again from the original
`CODE_AUDIT.md`, not from this report's earlier conclusions. Three parallel
reviews covered the core driver, both example/transport implementations, and
tooling/documentation. Their candidates were then reproduced against the live
source and diff before any follow-up fix was accepted.

| Confirmed gap | Final resolution |
| --- | --- |
| Raw `writeRegister16(CONFIG, CONFIG_RST)` did not apply the cooperative reset path's full state policy. A reset that applied despite a transport error could leave only CONFIG marked dirty, thresholds apparently trustworthy, and old trigger timing pending. | Mark the complete 11-register reset set and threshold advisory before the raw write, and invalidate trigger timing immediately. Success and apply-then-fail cases now share the same conservative policy and have a direct regression. |
| `scan` issued raw bus probes before its per-device active-job check. `selftest` and `recover` could proceed after BUSY and invalidate an outstanding job while the CLI still owned its ID. | Both examples use one cache-only active-owner guard for probe, both scans, self-test, and recovery. Rejection is reported as `BUSY` before I2C or invalidation. Self-test now checks `isInitialized()` directly instead of waiting for a `probe()` status that could not occur. The transfer HIL suite asserts these rejected commands use zero transfers and leave the verify job completable. |
| Native-IDF address selection and teardown discarded SDK failures, and successful temporary-address transfers discarded handle-removal failure. | Address selection and teardown return `Status`; initialization and binding stop on failure. Failed removal keeps the still-owned handle available for a cleanup retry, while a cleanup failure after an otherwise successful temporary transfer is returned with the original `esp_err_t` detail. |
| E9's platform buffer limit was compiled but its inclusive boundary was not executed. | Native tests now exercise write, TX, and RX sizes exactly at `WIRE_BUFFER_LIMIT` and one byte beyond it. |
| O9d relied on `monotonic_ns()` uniqueness while keeping `seq=0`. The assumption is false on coarse host clocks. | Added an incrementing per-run sequence and a regression with a frozen clock. |
| T3's CI target check accepted PlatformIO `esp32s2dev`/`esp32s3dev` prefixes after the real ESP-IDF rows were removed. Its CMake check also searched outside the actual component call. | Require exact YAML list items for both IDF targets and parse multiline `INCLUDE_DIRS` only inside `idf_component_register()`. |
| T2/T5 checked only the current hardcoded IDF files and did not recognize C++ raw strings, allowing a future source or raw literal to bypass framework/timing guards. | Scan every C/C++ source/header under the native-IDF main component. Both source lexers recognize delimited raw strings and run internal executable-versus-literal fixtures before scanning the repository. |
| T8 created `SoakSummary` inside `run_soak()`, so an exception before return lost completed, compressed-away PASS counts and timing. | The serial-run owner creates the summary and the soak mutates it in place. A forced mid-soak exception regression verifies unstored completed iterations survive for the final partial report. |
| Several earlier statements described behavior as tested when the setup or regression was incomplete. | Added stale-end-before-begin framing coverage; made D6 first set the threshold advisory; asserted D9's advisory after every typed threshold failure; tested repeated `readCurrent()` MATHOF recovery; and added mid-verification definite-address versus phase-unknown NACK cases. The public verify/reset documentation now states those policies. |

The fresh review reconfirmed the remaining original dispositions. In
particular, the integer-only sample margin is simpler than adding another
transfer/retry; phase-unknown IDF transfer NACK remains the correct SDK mapping;
the two-transfer accumulator-reset convenience has no reachable fallback path;
and the distinct legacy/fixed-unit calibration divisors serve documented
contracts rather than representing an implementation mismatch.

## Re-review of findings reported as already fixed

All D1–D9, E1–E10, and T1–T12 changes were traced in the current code. The
reported core behavior was generally correct, but the re-review found several
edge gaps and guard weaknesses:

- D2 now marks thresholds dirty before cooperative and raw reset writes,
  covering apply-then-fail transport results; raw reset also reports the full
  reset-owned dirty set and clears stale trigger timing. D4's internal job
  invalidation now preserves the first dirty cause as well as the public path.
  D5 has direct MEMSTAT dirty state/cause assertions, D8 is exercised through
  semantic verification invalidation, and D9's advisory behavior remains
  tested.
- E2 now drains the actual terminal operation ID before reporting a stale CLI
  owner ID. E6's successful re-probe fallback is a generic `I2C_ERROR`, because
  Arduino supplies no phase or cause at that point. E7 restarts the local TX
  buffer before closing, preventing transmission of a truncated payload. The
  Wire stub now injects begin, partial-write, queued end, and short-read outcomes
  so E6–E9 execute at and across their boundaries rather than merely compile.
- T2 scans all native-IDF C/C++ sources and forbids implementation includes and
  Arduino paths. T3 parses multiline `INCLUDE_DIRS` in the real component call
  and requires exact CI targets; T4 checks parsed aliases; T5's source-order
  lexer handles comments, ordinary literals, and raw strings without hiding
  later code. T6 strips historical health fields only for `drv`, T7 has
  framed/unframed verdict coverage, and T8 preserves both an explicit abort row
  and in-place compressed-soak totals. T10 now directly covers a stale end
  marker before its matching begin marker.

The audit's Section 1 scaling calculations were independently confirmed. The
permanent native vector now feeds the datasheet register encodings—including
wire value `0xB41000`, decoded signed 20-bit value `0xB4100`—through the public
driver and verifies voltage, temperature, current, power, energy, and charge.
The existing threshold vectors cover signed shunt, unsigned bus, temperature,
and calibrated power encodings.

Two wording limitations in the historical audit were also identified. TI's
worked example explicitly supplies the SOVL and BOVL threshold values; the
TEMP_LIMIT and PWR_LIMIT vectors are correct derivations rather than TI-provided
example values. Also, DIAG_ALRT bits 11–8 are read-only while event/status bits
7–0 are R/W; the audit's broad statement that all diagnostic flags are writable
was not exact. These corrections do not change the implemented policies.

## Documentation and evidence corrections

- The removed generated reports totaled 227,410 bytes. The retained PASS counts
  were not the same thing as stored detail-row counts: the soak report stored
  249 detailed rows, while 5,940 was the summarized PASS count. The exhaustive
  benchmark ran 100 iterations over six paths (600 executions), not 100 total
  identical rows.
- Commit `4c32312` is reachable from `main`; what cannot be reconstructed is the
  dirty worktree based on it. The changelog now says that precisely.
- The repository layout now lists `.github/`, long cooperative work is assigned
  to `pollJob()` rather than `tick()`, and the `v3.0.3` tag-delta description
  includes `CHANGELOG.md`.
- Package exclusion follows the audit's current path,
  `docs/CODE_AUDIT.md`. The original audit is retained as review input and this
  file is its disposition record.

## Historical validation — 2026-08-30

Performed on Windows with the repository-prescribed PlatformIO wrapper. The
shell had an unrelated `PLATFORMIO_CORE_DIR=C:\pio` override; clearing it for
the final native and Arduino validation processes allowed the wrapper's VS
Code-managed Core and installed platforms to be used.

| Gate | Result |
| --- | --- |
| Git synchronization before work | `main` and `origin/main` both at `e3e21aa`; fetch/prune found no newer remote commit or branch. |
| Fresh re-audit synchronization | The independent follow-up started with `main` and `origin/main` both at `1713f29`. |
| Native Unity suite | PASS, 126/126. |
| Arduino ESP32-S3 build | PASS; 24,872 B RAM, 398,476 B flash. |
| Arduino ESP32-S2 build | PASS; 51,844 B RAM, 408,497 B flash. |
| CLI, IDF, core-timing, owner static contracts | PASS. |
| HIL parser self-test and standalone parser regressions | PASS; nine standalone groups, including coarse-clock frame identity, stale framing, and interrupted compressed-soak preservation. |
| Exhaustive HIL dry-run with 100 × 6 benchmark executions | PASS. The transfer plan now includes zero-I2C scan/self-test/recover rejection during a live verify job. No hardware run was claimed. |
| Python byte compilation and version synchronization | PASS. |
| Doxygen warnings-as-errors generation | PASS. |
| `git diff --check` | PASS. |
| Native ESP-IDF local build | NOT RUN: `idf.py` and `IDF_PATH` are unavailable in this shell. The source was covered by the IDF static contract; Arduino builds are not counted as native IDF evidence. |

No physical INA228, high-voltage, fault-injection, ALERT-pin, or soak validation
was performed in this review. Existing hardware-evidence limitations remain in
force.

## Follow-up Tasks 1–9 — resolved in `2cc5874`

These dispositions close [`CODE_AUDIT_FOLLOWUP.md`](CODE_AUDIT_FOLLOWUP.md).
The historical 126-test/nine-group table above describes the earlier tree.
Commit `2cc5874a4991ff300deb88120a7c796fe46286b1` passed 127 native tests,
11 standalone HIL parser groups, and all six CI jobs in
[run 33728387995](https://github.com/janhavelka/INA228/actions/runs/33728387995)
on 2026-09-03, including both native ESP-IDF targets.

| Task | Committed disposition |
| --- | --- |
| 1 | Destructive verify-DIAG failure clears uncertain trigger timing without revoking synchronized configuration. |
| 2 | HIL classification removes historical counters from health blocks, including `recover`, while retaining live failure evidence. |
| 3 | The native-IDF guard scans all component C/C++ sources and headers. |
| 4 | Standalone regressions exercise the actual `run_step` wiring for `--require-framed`. |
| 5 | Public API and changelog document retained `TRIG_*` mode and instantaneous-sample base-mode preconditions. |
| 6 | Package rules and CI exclude all `docs/CODE_AUDIT*` records. |
| 7 | Local evidence was updated for the follow-up tree; the current validation page now also records its committed CI result. |
| 8 | Unreleased changes were consolidated into one changelog section. |
| 9 | Report-induced NOT RUN parity, quoted core includes, source-export dry runs, threshold advisories, retained IDF cleanup handles, Arduino transfer accounting, diagnostic Doxygen, balanced/per-command guards, read-only version-module import, and verbose stress reporting are resolved. |

## Fresh sweep against `2cc5874` — 2026-09-09

Reviewed the new report against a clean `2cc5874`, without reopening prior audit
rounds. Seven new core regressions failed before production edits, with all 127
existing tests still passing. Eight CLI regression groups then failed before
example edits (28 platform/command subcase failures). The CLI harness compiles
the actual extracted query, parser, calibration-print, and self-test reporting
code; scheduling, prompt, usage, and initialized-local checks are source guards.
Neither harness executes an ESP32 scheduler or INA228 silicon.

| Finding | Review and correction |
| --- | --- |
| A1 | Valid. Preserve synchronization and the energy epoch after inconclusive CONFIG/SHUNT_CAL preflight errors. Semantic mismatch or definite absence still invalidates, as do ambiguous or partial later writes. Tests cover both preflight registers, timeout, unknown-phase NACK, address NACK, and mismatch. |
| A2 | Valid. Reject runtime jobs at start while latched, before arming or touching the bus. Scope the poll exemption to initialize/reinitialize/reset, which verify identity and MEMSTAT. Recovery still uses its existing 14-transfer bound. |
| A3 | Valid. Compute the legacy CURRENT exceedance flag and reject it in bind, `setCalibration`, and `setAdcRange`. This adopts the report's rounded-request versus integer-plan milliamp comparison without changing the legacy calibration formula. It is conservative near full scale: conversion fixtures now request 9.999 A instead of 10 A while retaining SHUNT_CAL=4050 and all measurement expectations. Range-boundary fixtures use representable nearby values. The reproduced 500 microohm/100 mA input is rejected without I2C. |
| A4 | Valid cache/API inconsistency. Chose the report's acknowledgement alternative: clear the same event bits in both public caches, retaining unrelated flags and allowing later re-observation. Keep unacknowledged evidence through recovery/invalidation instead of silently erasing it; correct the snapshot documentation. `latestRaw` remains a historical last-read value. |
| A5 | Valid. Unknown enum values return `INVALID_PARAM` and leave the caller's limits unchanged. |
| A6 | Valid. Driver-side buffer/callback/admission validation remains outside health tracking; callback-returned validation codes are tracked for both reads and writes with original detail. |
| A7 | Valid. Track the legacy convenience's operation ID privately. It returns BUSY for externally started jobs/results, including token zero, and resumes or consumes only its own operation. Tests also cover mixed polling and cancellation. |
| B1 | Scheduling risk supported by source; the claimed watchdog reset is not hardware-confirmed. Replace bare yields with Arduino `delay(1)` and native-IDF `sleepMs(1)` (at least one RTOS tick). See Espressif's [watchdog guidance](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html). No HIL performed. |
| B2 | Valid. Both `cal` and `cfg` derive displayed ohms/amps from the fixed-unit contract when selected, retaining legacy display support. |
| B3 | Valid. All seven configuration queries reject uninitialized state before accessing cached defaults and set HIL status through `printStatus`. |
| B4 | Valid. Failed self-test checks set non-OK HIL status while preserving an already reported error. |
| B5 | Valid. Arduino bus-init failure prints help and prompt before returning. |
| B6 | Valid parser defect; current caller bounds limited its impact. Skip leading whitespace before rejecting a minus sign, retaining full-token/range checks and unchanged output on failure. |
| B7 | Valid usage mismatch. Advertise and enforce readiness budgets 1–255. |
| B8 | Valid defensive initialization issue, without a demonstrated read of the old indeterminate values. Initialize both Arduino mode locals to SHUTDOWN. |
| C1 | Valid. Record the reviewed commit and verified September 3 CI run; distinguish this new local worktree from that committed evidence. |
| C2 | Valid. Retain the historical follow-up with a completion banner and the Tasks 1–9 dispositions above. Historical counts stay dated; current counts are below. |
| C3 | Maintainer release preference requested. No version was inferred or bumped; changes remain under Unreleased at 3.0.3. |

No transfer bound was widened, no core retry was added, and no new production
abstraction was introduced. The new CLI regression tool has a concrete CI caller.

### Fresh-sweep local validation

| Gate | Result on 2026-09-09 |
| --- | --- |
| Native Unity | PASS, 134/134; seven new regression groups failed first. |
| CLI regressions | PASS, eight groups across both examples; failed first (28 subcases). |
| Arduino ESP32-S3 | PASS; 24,872 B RAM, 399,440 B flash. |
| Arduino ESP32-S2 | PASS; 51,844 B RAM, 409,649 B flash. |
| Four static contracts | PASS: CLI, IDF example, core timing, owner. |
| HIL parser and plan | PASS: parser self-test, 11 standalone parser groups, exhaustive dry-run including NOT RUN and 100-iteration benchmarks. No device execution. |
| Version, Python, Doxygen | PASS: version consistency, byte-compilation, Doxygen warnings-as-errors. |
| Package | PASS: 37 entries, required public files/README links, all exclusions, standalone C++17 compile. |
| Diff whitespace | PASS. |
| Remote CI | Existing `2cc5874` run 33728387995 is green; no remote run covers these uncommitted edits. CI now includes the new CLI regression command. |
| Native ESP-IDF locally | NOT RUN: no `idf.py` or `IDF_PATH`. |
| HIL | NOT RUN: hardware unavailable; B1 watchdog reproduction remains unconfirmed. |

Windows build validation used `scripts/pio.cmd` and VS Code-managed Core
6.1.19. An inherited `PLATFORMIO_CORE_DIR=C:\pio` referenced mismatched local
packages. The default package installation also encountered disabled Windows
long-path support and an incomplete compiler install. Validation succeeded
using a temporary `P:` mapping to the existing user `.platformio` directory,
reinstalling the pinned compiler bootstrap, and adding that pinned compiler's
`tools/toolchain-xtensa-esp-elf/bin` directory to the build process PATH.
The drive mapping was removed afterward; no repository dependency pin or
machine-wide Windows setting was changed.

Local logs are in `.pio/fresh-sweep-*.log` (ignored build artifacts), including
before/after regression results and the final Arduino build. These local logs
and this disposition record do not substitute for a clean-commit CI/HIL run.


### Later HIL adapter correction (2026-09-11)

The former E6 short-read address re-probe used another complete timeout and
could attribute a later probe failure to the original read. The Arduino adapter
now returns generic `I2C_ERROR` with the received byte count when `requestFrom`
does not expose the original phase, without re-probing. Direct Wire statuses
retain their precise mapping. Explicit address scans/probes remain available as
separate diagnostics; missing-device read failures are no longer forced into
`DEVICE_NOT_FOUND` by this adapter. Native regressions verify one read callback,
zero extra probe transfers, and that a hypothetical later NACK stays unconsumed.
