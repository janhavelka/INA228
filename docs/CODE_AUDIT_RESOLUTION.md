# Code Audit Resolution — 2026-08-30

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
| O7, IDF NACK mapping | The main premise was incorrect. ESP-IDF 6.x documents `ESP_ERR_INVALID_RESPONSE` as the unexpected-NACK result for the master transmit APIs, so the existing phase-unknown NACK branch was reachable and correct. The fallback mismatch was valid. | Kept `ESP_ERR_INVALID_RESPONSE -> I2C_NACK_UNKNOWN_PHASE` for transfers and address-NACK mapping for the explicit probe. Changed unclassified transfer/probe failures from `I2C_BUS` to `I2C_ERROR` with the original `esp_err_t` detail, and documented the distinction. See the official [ESP-IDF I2C API](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html) and [ESP-IDF 6.0 peripheral migration guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/migration-guides/release-6.x/6.0/peripherals.html). |
| O8, limit decoding | Valid for power and for missing advisories. A successful range change does make the cached range equal to hardware, so treating every `thresholdsDirty` state as an unknown range would have hidden a valid raw decode. | Shunt limits are decoded only with synchronized hardware state; power limits only with `SettingsSnapshot::calibrated`. Raw codes are always shown, and `thresholdsDirty` produces an explicit reapplication warning. The same rules apply to `limits`, individual shunt queries, and `pwrlim`. |

### Smaller findings O9

| Item | Result |
| --- | --- |
| O9a | Added and documented `verify_start` / `verify_step`; added the path to the HIL feature sweep. |
| O9b | Added a distinct `BUSY` message for an unconsumed terminal result and regression coverage for both BUSY causes. |
| O9c | Removed both `hilmark` handlers and the runner's legacy marker flag, retries, parser, and documentation. |
| O9d | No change. The frame token contains a new monotonic-nanosecond value per command; a constant sequence field is redundant but does not permit an old token to match a new frame. An incrementing counter would add state without improving identity. |
| O9e | Parser self-tests now exercise production `classify_step()`; dead `classify_output()` was removed. |
| O9f | Kept native IDF independent of Arduino glue. The contract now parses and compares SDA, SCL, frequency, timeout, address, mode, calibration mode, shunt, and maximum current. |
| O9g | No change. The alleged fallback is unreachable under the exact two-transfer synchronous budget, and adding dead defensive control flow would not improve behavior. |
| O9h | Kept the public width constants and used them in register buffers. `maxRetries` remains a documented, default-zero contract with tests. |
| O9i | No change. `docs/CLI.md` remains outside Doxygen and the existing raw HTML link strategy passes warnings-as-errors generation. |
| O9j | Scoped cppcheck explicitly to the same dated local validation run; it is not presented as a recurring CI gate. |
| O9k | No change. The datasheet-compatible legacy `2^19` divisor and conservative fixed-unit positive-limit planner intentionally serve different contracts. |
| O9l | No behavior change. The source comment now states the actual boundary: initial bring-up and reinitialization after explicit invalidation are excluded, while recovery from a still-initialized session is tracked. |

## Re-review of findings reported as already fixed

All D1–D9, E1–E10, and T1–T12 changes were traced in the current code. The
reported core behavior was generally correct, but the re-review found several
edge gaps and guard weaknesses:

- D2 now marks thresholds dirty before the reset write, covering an
  apply-then-fail transport result. D4's internal job invalidation now preserves
  the first dirty cause as well as the public path. D5 has direct MEMSTAT dirty
  state/cause assertions, D8 is exercised through semantic verification
  invalidation, and D9's advisory behavior remains tested.
- E2 now drains the actual terminal operation ID before reporting a stale CLI
  owner ID. E6's successful re-probe fallback is a generic `I2C_ERROR`, because
  Arduino supplies no phase or cause at that point. E7 restarts the local TX
  buffer before closing, preventing transmission of a truncated payload. The
  Wire stub now injects begin, partial-write, queued end, and short-read outcomes
  so E6–E9 execute rather than merely compile.
- T2 now forbids any `.cpp` include and Arduino example path in native IDF
  sources. T3 parses multiline `INCLUDE_DIRS`; T4 checks parsed aliases; T5 uses
  one source-order lexer expression so comment markers inside strings cannot
  hide later code. T6 strips historical health fields only for `drv`, T7 has
  framed/unframed verdict coverage, and T8 appends an explicit failing runner
  abort row before writing partial evidence. T10's stale-end-before-begin case
  remains covered.

The audit's Section 1 scaling calculations were independently confirmed. The
permanent native vector now feeds the datasheet register encodings—including
wire value `0xB41000`, decoded signed 20-bit value `0xB4100`—through the public
driver and verifies voltage, temperature, current, power, energy, and charge.
The existing threshold vectors cover signed shunt, unsigned bus, temperature,
and calibrated power encodings.

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

## Validation

Performed on Windows with the repository-prescribed PlatformIO wrapper. The
shell had an unrelated `PLATFORMIO_CORE_DIR=C:\pio` override; clearing it for
the final native and Arduino validation processes allowed the wrapper's VS Code-managed Core and
installed platforms to be used.

| Gate | Result |
| --- | --- |
| Git synchronization before work | `main` and `origin/main` both at `e3e21aa`; fetch/prune found no newer remote commit or branch. |
| Native Unity suite | PASS, 124/124. |
| Arduino ESP32-S3 build | PASS; 24,872 B RAM, 398,172 B flash. |
| Arduino ESP32-S2 build | PASS; 51,844 B RAM, 408,169 B flash. |
| CLI, IDF, core-timing, owner static contracts | PASS. |
| HIL parser self-test and standalone parser regressions | PASS. |
| Exhaustive HIL dry-run with 100 × 6 benchmark executions | PASS. No hardware run was claimed. |
| Python byte compilation and version synchronization | PASS. |
| Doxygen warnings-as-errors generation | PASS. |
| `git diff --check` | PASS. |
| Native ESP-IDF local build | NOT RUN: `idf.py` and `IDF_PATH` are unavailable in this shell. The source was covered by the IDF static contract; Arduino builds are not counted as native IDF evidence. |

No physical INA228, high-voltage, fault-injection, ALERT-pin, or soak validation
was performed in this review. Existing hardware-evidence limitations remain in
force.
