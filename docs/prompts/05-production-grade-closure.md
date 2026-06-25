# AI Coder Prompt: INA228 Production-Grade Closure Pass

You are working inside the current INA228 repository. The library is a strong
pre-production hardening candidate, but do not call it production-grade until
the concrete closure items below are implemented, verified, and documented with
evidence.

Goal:
Close the remaining engineering and evidence gaps that prevent an honest
production-grade claim. Prefer simple, functional, robust changes. Reuse
existing code, examples, HIL runner helpers, native fake-bus tests, and docs
where feasible. Do not introduce broad frameworks, fake production devices, or
speculative abstractions.

## Initial Rules

1. Read `AGENTS.md` first.
2. Inspect `README.md`, `CHANGELOG.md`, `docs/validation/`,
   `docs/reports/`, `tools/run_i2c_hil.py`, examples, tests,
   `.github/workflows/ci.yml`, and `platformio.ini` before editing.
3. Preserve dirty user changes. Do not revert unrelated work.
4. Do not commit unless explicitly asked.
5. Do not claim production readiness, industry readiness, hardware validation,
   ESP-IDF validation, or field readiness without matching dated evidence.
6. Keep core transport callback-only and framework-neutral.
7. Keep fixture/fault logic out of production core paths.
8. Use bounded waits, bounded retries, finite loops, and visible failure
   statuses only.

You may spawn read-only subagents for docs/evidence, HIL/tooling, API/status,
CI, and native-test audit. Keep final judgment, edits, and verification in the
main agent.

## Current Evidence To Account For

- HIL reports exist under `docs/reports/`, including:
  - an 8-hour Arduino ESP32-S3 run with `UNKNOWN` serial framing rows,
  - a targeted feature sweep with no FAIL/UNKNOWN rows,
  - a targeted stress run that stopped on one `UNKNOWN` serial framing row.
- Current validation docs still say hardware validation is `NOT RUN`.
- Existing HIL proves CLI-visible staged behavior, but not exact backend
  transfer counts per poll/chunk.
- Current HIL fixture did not cover disconnected target, bus fault injection,
  alert-pin capture, controlled MCU reset, or power-cycle behavior.
- Current checked-in reports were produced from dirty worktrees, so they are
  not clean release-candidate evidence.

## Required Workstreams

### 1. Evidence And Claim Alignment

Update docs so claims match evidence exactly.

Required behavior:
- `README.md`, `CHANGELOG.md`, and `docs/validation/validation-status.md` must
  distinguish:
  - implemented behavior,
  - local native tests,
  - local build/package checks,
  - CI configured,
  - CI verified,
  - partial low-voltage HIL evidence,
  - release-grade hardware validation.
- Existing `docs/reports/` runs may be described only as partial HIL evidence.
  Do not treat them as full production hardware validation because they include
  dirty worktrees, fixture gaps, and at least one `UNKNOWN` framing event in
  stress/soak evidence.
- Replace or qualify wording such as `industry-readiness`, `production-ready`,
  `field-proven`, or `hardware validated` unless a dated clean-commit evidence
  directory supports it.

Suggested wording:
- `pre-production hardening candidate with partial low-voltage Arduino ESP32-S3 HIL evidence`
- `release-grade hardware validation is blocked pending clean-commit framed HIL, transfer-count evidence, fault-injection fixture coverage, alert-pin capture, and reviewed CI logs`

If preserving existing reports, add an index such as:

```text
docs/validation/hardware-evidence.md
```

The index should list each report, commit, dirty status, framework, board, port,
PASS/FAIL/UNKNOWN/NOT RUN counts, and whether it is release-grade evidence.

### 2. Stronger HIL Framing

The current serial-text marker approach can produce `UNKNOWN` rows under
sustained traffic. Add a stronger example-only framed HIL path.

Concrete names and output contract:
- Add CLI command:

```text
hilrun <token> <seq> <inner command...>
```

- `hilrun` must call the existing command dispatcher for `<inner command...>`.
  Do not duplicate command implementations.
- Prevent recursion: `hilrun ... hilrun ...` must return `INVALID_PARAM`.
- Output exactly one begin line and one end line around the inner command:

```text
HIL_BEGIN token=<token> seq=<seq>
... existing command output ...
HIL_END token=<token> seq=<seq> status=<ERR_NAME> elapsed_ms=<n>
```

- `status=<ERR_NAME>` must be `OK` when the command was parsed and completed
  without a visible non-OK `Status`. It must be a precise error name when the
  command reports one. Unknown commands must report `INVALID_PARAM`.
- Keep the legacy `hilmark` command available, but make the HIL runner prefer
  `hilrun` by default when the CLI supports it.
- Add runner options:
  - `--legacy-marker` to force old marker mode,
  - `--require-framed` to fail if `hilrun` is unavailable,
  - `--max-frame-bytes <n>` with a conservative default, for example `8192`.
- Add parser self-tests for:
  - complete frame,
  - truncated frame,
  - wrong token,
  - wrong sequence,
  - nested `hilrun`,
  - command output containing text that looks like an old marker.

Acceptance gate:
- Framed HIL stress must complete with `0 FAIL` and `0 UNKNOWN` before it can
  support a production-grade claim.

### 3. Example-Only Transfer Counters

Add optional example-only counters around injected transport calls so HIL can
prove chunking/polling transfer budgets.

Concrete command names:

```text
xfer_reset
xfer_stats
xfer_assert <read> <write> <total>
```

Definitions:
- `read` means calls to the `i2cWriteRead` callback.
- `write` means calls to the `i2cWrite` callback.
- `total = read + write`.
- Count callback invocations, not bytes and not low-level ESP-IDF transactions.
- Counters must live in example glue only, not in `include/` or `src/`.

Output contract:

```text
XFER_STATS read=<n> write=<n> total=<n>
XFER_ASSERT PASS read=<actual> write=<actual> total=<actual>
XFER_ASSERT FAIL expected_read=<n> expected_write=<n> expected_total=<n> read=<actual> write=<actual> total=<actual>
```

Required HIL checks, after verifying exact counts against implementation:
- `ready_step 0`: `read=0 write=0 total=0`.
- `ready_step 1` when the readiness deadline has elapsed and a poll is needed:
  `read=1 write=0 total=1`.
- `sample_step 1` from a clean sample job: `read=1 write=0 total=1`.
- `sample_step 2` from a clean sample job: `read=2 write=0 total=2`.
- `sample_step 5` in continuous mode from a clean sample job:
  `read=5 write=0 total=5`.
- `sample_step 5` after a completed triggered conversion may need one
  readiness read plus five sample reads: candidate bound
  `read<=6 write=0 total<=6`.
- `apply_step 0`: `read=0 write=0 total=0`.
- `apply_step 6` for a full config replay job: candidate bound
  `read=0 write=6 total=6`.
- `reset_step 0`: `read=0 write=0 total=0`.
- Full reset job after `startResetJob()`: candidate bound
  `write<=7 read<=6 total<=13`.

If any candidate count differs, do not loosen it casually. First inspect the
implementation, document the exact sequence in the HIL report, and set the
smallest defensible bound.

### 4. Hardware Fault-Injection And Alert Fixture Contract

Do not simulate faults in production paths. Add fixture integration only when a
safe external fixture exists. If no fixture is attached, tests must stay
`NOT RUN` and production-grade claim remains blocked.

Suggested fixture commands for the HIL runner or fixture controller:

```text
fixture addr_nack on|off
fixture sda_low <ms>
fixture scl_low <ms>
fixture power_cycle <ms>
fixture mcu_reset <ms>
fixture alert_capture arm
fixture alert_capture read
fixture safe_state
```

Required HIL assertions when fixture support exists:
- Address NACK maps to `DEVICE_NOT_FOUND` only when absence is definite.
- Data/phase NACK maps to `I2C_NACK_DATA` or
  `I2C_NACK_UNKNOWN_PHASE`, not `DEVICE_NOT_FOUND`.
- Timeout maps to `I2C_TIMEOUT`.
- Stuck or invalid bus state maps to `I2C_BUS`.
- Repeated failures reach `OFFLINE` after `offlineThreshold=5`.
- Normal public I2C operations while `OFFLINE` return `BUSY` without touching
  the bus.
- `recover()` returns to `READY` after the fixture restores safe bus/device
  state.
- ALERT pin capture verifies polarity, latch/transparent behavior,
  conversion-ready alert routing, and safe threshold crossings.
- Controlled MCU reset verifies fresh `begin()` replays desired configuration.
- Controlled INA228 power cycle verifies identity/MEMSTAT/config replay or
  visible failure.

Every fault test must restore `fixture safe_state` or explicitly stop with a
dirty/hazard note.

### 5. ESP-IDF NACK Precision

The native ESP-IDF example currently cannot distinguish address phase from data
phase for some `ESP_ERR_INVALID_RESPONSE` transfers. Avoid collapsing this into
generic `I2C_ERROR`.

Concrete public status addition:

```cpp
Err::I2C_NACK_UNKNOWN_PHASE
```

Rules:
- Append the new enum value after existing `Err` values. Do not renumber old
  codes.
- Map ESP-IDF `ESP_ERR_INVALID_RESPONSE` during normal transfer to
  `I2C_NACK_UNKNOWN_PHASE` unless the code can prove address or data phase.
- Keep explicit address probe failures mapped to `I2C_NACK_ADDR`.
- `begin()` and `probe()` may map only definite address NACK to
  `DEVICE_NOT_FOUND`.
- Timeout and bus errors must remain distinct.

Tests:
- Native tests for new `Err` name and status behavior.
- ESP-IDF transport contract check updated to require the new mapping.
- HIL fixture tests should prove absence/fault classifications when hardware
  allows it.

### 6. Central Error-Name Mapping

Status names are duplicated in examples and can become stale.

Concrete API:

```cpp
constexpr const char* errName(Err err);
```

Placement:
- Add it to `include/INA228/Status.h`.
- Keep it framework-neutral, constexpr-capable, and static-string only.
- Examples should use `INA228::errName(err)` instead of local duplicate
  switches where feasible.

Required names:
- `OK`
- `NOT_INITIALIZED`
- `INVALID_CONFIG`
- `I2C_ERROR`
- `TIMEOUT`
- `INVALID_PARAM`
- `DEVICE_NOT_FOUND`
- `DEVICE_ID_MISMATCH`
- `MEMORY_ERROR`
- `MEASUREMENT_NOT_READY`
- `MATH_OVERFLOW`
- `BUSY`
- `IN_PROGRESS`
- `I2C_NACK_ADDR`
- `I2C_NACK_DATA`
- `I2C_TIMEOUT`
- `I2C_BUS`
- `ACCUMULATION_INVALID`
- `ACCUMULATION_OVERFLOW`
- `HARDWARE_DIRTY`
- `I2C_NACK_UNKNOWN_PHASE` if added

Tests:
- Add a native test that every current `Err` value returns the exact string
  above and no current value returns `UNKNOWN`.
- Add/update a static contract script if useful, but do not create a broad code
  generator for this.

### 7. Clearer Config Replay Job Names

The existing fixed-step names `startApplyCalibration()` and
`pollApplyCalibration()` understate that the job replays cached static
configuration and calibration.

Concrete compatible aliases:

```cpp
Status startConfigReplayJob();
Status pollConfigReplayJob(uint32_t nowMs, uint8_t maxInstructions);
```

Rules:
- Keep old `startApplyCalibration()` and `pollApplyCalibration()` as
  source-compatible aliases.
- Do not remove or rename existing public APIs in this pass.
- Update docs and examples to prefer the new names where clarity helps.
- Keep CLI compatibility for `apply_start` / `apply_step`, but add optional
  aliases:

```text
replay_start
replay_step <maxInstructions>
```

Tests:
- Native alias tests proving old and new names use the same job.
- HIL targeted checks for both CLI names if aliases are added.

### 8. Staged Failure Coverage

Add host tests that exhaustively validate staged failure cleanup, dirty state,
and output preservation. Use existing fake-bus helpers; do not add production
simulation paths.

Required test names:

```text
test_power_sample_step_failure_each_register_clears_job_preserves_outputs
test_apply_replay_failure_each_step_marks_exact_dirty_register
test_reset_job_failure_each_step_reasserts_offline_when_started_offline
test_zero_budget_fixed_step_calls_are_bus_silent
```

Expected behavior:
- Output structs remain unchanged on non-OK status.
- Active jobs do not remain wedged after failed staged reads.
- Dirty register masks are as narrow as implementation can prove.
- Zero-budget calls return `INVALID_PARAM` and touch no transport.
- Reset/replay failures preserve visible status and recovery path.

Mirror the first two cases in HIL after transfer counters and fixture fault
support exist.

### 9. CI Evidence Improvements

Update CI so release reviewers have durable evidence.

Required CI additions:
- `python -m py_compile tools/run_i2c_hil.py`
- `python tools/run_i2c_hil.py --parser-self-test`
- Upload native test logs/artifacts as `native-test-results`.
- Upload guard-script output as `static-contract-logs`.
- Upload PlatformIO package archive as `platformio-package`, or explicitly
  remove it after a separate artifact copy.
- Upload ESP-IDF build logs as `esp-idf-build-logs`.
- Keep existing native, PlatformIO S2/S3, ESP-IDF S2/S3, and package checks.

Do not rely on CI presence alone. Update docs only after a completed CI run is
reviewed for the exact branch, PR, or release commit.

### 10. Clean Release-Candidate Evidence Run

After code and docs changes are complete, run validation from a clean
release-candidate commit.

Because this prompt must not commit unless explicitly asked:
- If the worktree is dirty, run local checks but do not claim clean release
  evidence.
- Ask the user for commit/tag permission if clean-commit HIL evidence is needed
  in the same session.

Required release-grade HIL evidence:
- Clean git status before build/flash.
- Exact commit hash.
- Build and upload logs.
- Full serial transcript.
- Framed targeted feature suite: `0 FAIL`, `0 UNKNOWN`.
- Framed transfer-count suite: `0 FAIL`, `0 UNKNOWN`.
- Framed 8-hour soak: `0 FAIL`, `0 UNKNOWN`.
- Fault-injection suite: `0 FAIL`, `0 UNKNOWN`, or explicitly `NOT RUN` with
  production-grade claim blocked.
- Alert-pin suite: `0 FAIL`, `0 UNKNOWN`, or explicitly `NOT RUN` with
  production-grade claim blocked.
- ESP-IDF build evidence or reviewed CI URL.
- ESP32-S2 and ESP32-S3 evidence where hardware is available.

Suggested evidence path:

```text
docs/validation/hardware/YYYY-MM-DD/<short-commit>-<framework>-<target>-addr-0xNN/
```

## Commands To Run

Run the smallest relevant check after each fix. Before final response, run as
much of this set as the machine supports:

```powershell
python -m py_compile tools\run_i2c_hil.py
python tools\run_i2c_hil.py --parser-self-test
python tools\run_i2c_hil.py --dry-run --suite targeted
python tools\check_cli_contract.py
python tools\check_idf_example_contract.py
python tools\check_core_timing_guard.py
python scripts\generate_version.py check
pio test -e native
pio run -e esp32s3dev
pio run -e esp32s2dev
pio pkg pack
git diff --check
```

If ESP-IDF is installed:

```powershell
idf.py -C examples\esp_idf\basic set-target esp32s3 build
idf.py -C examples\esp_idf\basic set-target esp32s2 build
```

Remove generated `INA228-*.tar.gz` archives after package validation unless
the user asks to keep them.

## Final Response Requirements

Keep the final response concise and factual. Include:

- files changed,
- production-closure items implemented,
- items still blocking production-grade claims,
- tests/builds/HIL commands run and results,
- CI status or reason CI was not checked,
- exact HIL report/evidence paths,
- explicit statement whether production-grade can now be claimed.

Do not say production-grade is achieved unless every release-grade evidence
gate above is satisfied.
