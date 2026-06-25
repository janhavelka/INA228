# AI Coder Prompt: Exhaustive HIL Validation And Repository Audit

You are working inside one repository in the current workspace. Determine the
target device, library type, supported boards, examples, and public API from the
repository itself. Perform extensive hardware-in-loop validation on the
connected hardware, extend the automated HIL tooling as needed, create a
detailed Markdown report, then audit the library for concrete functional weak
spots found during testing.

Serial port: `COMXX`

Replace `COMXX` with the actual port before running hardware commands.

## Goals

- Exercise every safe feature exposed by the target device, the core library,
  and the repository examples.
- Push timing, sampling, conversion, reset, recovery, diagnostics, staged jobs,
  and bus behavior within safe electrical and firmware bounds.
- Run an intensive 8-hour soak test that pushes safe functional paths to the
  practical limits of the available fixture.
- Extend the existing automated HIL script instead of creating a parallel test
  framework unless the existing script cannot reasonably support the need.
- Produce a new Markdown report with exact commands, timing, expected result,
  observed result, pass/fail, limitations, and proposed fixes.
- Finish with a library audit that identifies functional weak spots and proposes
  simple, safe, robust fixes with tests.

## Hard Rules

1. Read repository instructions first: `AGENTS.md`, `README.md`, docs,
   examples, build config, test config, and existing HIL tooling.
2. Preserve dirty user changes. Do not revert unrelated work.
3. Do not commit unless explicitly asked.
4. Do not claim hardware validation for any test that did not run on hardware.
5. Do not perform electrically dangerous tests unless the hardware fixture is
   explicitly safe for them.
6. Use bounded waits, deadlines, and finite iteration counts only.
7. Preserve the repository architecture. Do not add framework dependencies or
   bus ownership to core code unless the repository already requires that.
8. Do not add fake devices, simulated buses, or test doubles to production
   paths.
9. Prefer simple local extensions to existing examples and HIL tools over new
   abstractions.
10. If a test needs hardware that is not present, mark it `NOT RUN` with the
    exact missing fixture.

## Initial Setup

Record:

- repository path
- branch name
- commit hash
- dirty git status
- date/time and timezone
- operating system
- Python version
- build tool versions
- target board/environment
- serial port and baud rate
- detected device identity, address, or endpoint, if applicable
- relevant fixture details and electrical safety assumptions

Then:

1. Inspect and summarize the current HIL script, expected CLI commands, example
   command surface, and public API.
2. Build the firmware or application that will be flashed for HIL.
3. Flash the target using the provided serial port.
4. Open serial at the documented baud rate and capture the boot transcript.
5. Confirm the example prompt or control endpoint is responsive before running
   the full suite.

Use repository-appropriate commands. For PlatformIO-style firmware repositories,
commands may look like:

```powershell
pio run -e <environment>
pio run -e <environment> -t upload --upload-port COMXX
python tools\<hil-runner>.py --port COMXX --baud 115200 --timeout-s 5 --verbose
```

If the repository uses another build system, infer and document the equivalent
build, flash, and HIL commands.

## Required Tooling Work

Extend the current HIL runner, or create the smallest reasonable runner if none
exists, to support:

- `--port COMXX`
- configurable baud rate
- bounded per-command timeout
- bounded idle timeout
- optional boot/reset settle time
- verbose transcript capture
- Markdown report table output or data suitable for the report
- pass/fail/unknown/not-run classification
- per-step elapsed time
- expected tokens and failure-token classification
- optional reset/reconnect handling
- optional sample-rate benchmark mode
- optional stress duration/count limits
- iterative adjustment of serial read timing, boot settle, prompt detection,
  command pacing, retry-on-no-prompt behavior, and transcript capture when the
  real hardware session proves the defaults are too brittle
- dry-run and parser self-test modes

The AI coder may edit the Python HIL runner during the hardware session when
communication, delay, prompt detection, reset/reconnect, or timing behavior
shows that the current script is too strict or too loose. Each adjustment must
remain bounded, deterministic, and recorded in the report with the reason for
the change. Keep the script small and readable. Do not add a broad test
framework.

If the example CLI or control surface lacks commands needed for a real device
feature, add narrowly scoped commands that expose existing library behavior
instead of bypassing the example with ad hoc protocol code in the runner.

## Required Report

Create a new report file:

```text
docs/reports/hil-validation-COMXX-YYYYMMDD.md
```

Replace `COMXX` and `YYYYMMDD` with the actual port and date.

The report must include:

- title, date/time, timezone
- repository branch and commit
- dirty status summary
- hardware setup and wiring
- board type and firmware/application environment
- detected device identity/address/endpoint
- electrical limits and safety assumptions
- exact build, flash, and HIL commands
- summary table of all tests
- detailed table with:
  - test id
  - feature area
  - command or script step
  - expected result
  - observed result
  - elapsed time
  - pass/fail/unknown/not-run
  - notes
- raw serial transcript location or embedded excerpts
- sampling/timing results
- failures and anomalies
- limitations and tests not run
- concrete library/example/tooling fixes proposed
- fixes implemented during the run, if any
- final verification commands and results
- 8-hour soak summary with start/end time, total duration, command mix,
  sample counts, error counts, reset/recovery counts, worst observed latency,
  health-state changes, and any script adjustments made during the run

Do not claim production readiness. Use evidence-based wording only.

## Required Test Coverage

Adapt this section to the actual repository and target device. If a feature is
not applicable, mark it `NOT APPLICABLE` in the report with a short reason.

### Basic Connectivity And Identity

- serial boot and prompt responsiveness
- version/build information
- bus scan or endpoint discovery, if applicable
- target-address or target-endpoint scan, if applicable
- explicit probe/detect command
- device identity values documented by the repository or datasheet
- device health/status self-checks
- wrong-address or wrong-endpoint behavior if safe and supported
- disconnect or absent-device behavior only if a safe fixture permits it

### Lifecycle, State, And Health

- initial initialization result shown by the example
- settings/config snapshot
- driver/library health output
- probe no-health-side-effect behavior, if observable
- forced or induced read/write failure classification if safely possible
- recovery command or API
- shutdown/end/deinit behavior if exposed
- repeated initialization
- invalid address/endpoint/config rejection
- health counters before and after successful operations
- degraded/offline behavior if safely triggerable without damaging hardware

### Data Reads And Conversions

Test every available public/example read command:

- scalar reads
- aggregate reads
- raw reads
- fixed-unit or integer reads
- floating-point convenience reads
- raw register or raw field helpers
- staged/chunked reads, if present

For each read, record:

- status
- raw value where available
- converted value
- units
- elapsed time
- whether calibration/configuration was required
- whether output is plausible for the fixture
- whether the read has side effects or consumes diagnostic evidence

### Modes, Timing, And State Machines

Exercise every safe operating mode exposed by the repository:

- shutdown/idle/low-power modes
- one-shot/triggered modes
- continuous modes
- mode combinations
- mode transitions
- state-machine jobs

For each mode:

- set mode
- record expected timing
- trigger if applicable
- poll readiness with bounded polling
- verify no stale read before ready
- verify read succeeds after ready
- record actual ready latency and compare to estimate
- verify documented post-completion state transitions

### Configuration Matrix And Boundary Values

Test all documented configuration categories where practical:

- timing/conversion/sample-period settings
- averaging/filtering/oversampling settings
- delay/synchronization settings
- gain/range/scale settings
- threshold/limit settings
- calibration settings
- enable/disable flags

For each selected combination:

- set the config
- read back settings
- record expected behavior
- perform at least one relevant operation
- measure actual elapsed time or output change
- classify pass/fail by bounded tolerance

Do not run a full Cartesian product if it would be too long. Use a small smoke
matrix plus boundary values, and document skipped combinations.

### Calibration And Scale

Exercise:

- uncalibrated behavior
- configured calibration used by the example
- calibration readback
- scale/range switching
- boundary/clamp behavior if exposed
- temperature or compensation settings if applicable
- output plausibility under the available fixture

Do not apply unsafe stimulus to the hardware. If no calibrated fixture is
present, validate status, register/config readback, and plausible benign
behavior only.

### Accumulators, Counters, Or Integrators

If the device/library has accumulators, counters, history, integrators, FIFOs,
or latched data:

- readiness in supported modes
- reset/clear behavior
- read before ready
- read after ready
- raw reads
- behavior after reset
- overflow flags only if the fixture can produce them safely; otherwise mark
  `NOT RUN`

Record whether reads are destructive or preserve evidence.

### Diagnostics, Alerts, And Limits

Exercise:

- raw diagnostic/status read
- parsed diagnostic/status output
- clear-on-read or destructive status behavior where observable
- alert/interrupt enable and disable
- latch/transparent behavior if applicable
- polarity or output mode if applicable
- threshold and limit configuration
- safe threshold-trigger tests using benign fixture values

Do not create real overvoltage, overtemperature, overcurrent, mechanical,
thermal, RF, or other unsafe physical conditions unless a safe fixture is
explicitly present.

### Reset, Reboot, And Persistence

Exercise:

- software reset
- staged/fixed-step reset job, if exposed
- reset with minimum budget
- reset with intermediate budget
- reset with full budget
- post-reset identity/health recheck
- config/calibration replay after reset
- data/accumulator reset
- MCU/application reboot and fresh initialization
- power-cycle behavior only if available and safe

Do not assume settings are persistent unless the repository or datasheet says
so. Verify and report what is restored by initialization after reboot/reset.

### Staged Jobs And Bus Transaction Budgets

For repositories with staged APIs, chunked operations, or bus transaction
budgets:

- single-instruction progression
- two-instruction progression
- full-budget completion
- zero-budget rejection
- active-job busy behavior
- delay gates
- transaction count per poll if observable
- output unchanged on non-OK status
- recovery from failed or interrupted staged jobs where safely possible

For I2C, SPI, UART, CAN, or similar bus libraries, also record:

- transfer count per operation, if observable
- timeout behavior
- NACK/timeout/bus/data error classification, if applicable
- whether the library owns or merely uses the bus
- whether bus recovery is explicit and bounded

### Sampling Frequency And Stress

Measure achievable and stable sampling rates for all relevant read paths:

- fastest scalar read
- slowest scalar read
- raw sample read
- aggregate read
- integer/fixed-unit read if present
- staged read with minimum budget
- staged read with intermediate budget
- staged read with full budget

For each benchmark:

- run a short warmup
- run a bounded duration or count
- record success count, failures, min/mean/max interval, effective Hz, and
  worst-case latency
- verify health state after the run
- check whether diagnostic evidence was consumed

Do not use unbounded stress. Suggested limits:

- quick: 50 samples
- normal: 500 samples
- extended: 5000 samples or 60 seconds, whichever comes first

### 8-Hour Intensive Soak

Run one bounded 8-hour soak test after the shorter functional tests pass. The
soak must keep the hardware and firmware inside safe electrical, thermal, and
mechanical limits for the actual fixture.

The soak should cycle through a practical command mix:

- continuous reads
- raw reads
- integer/fixed-unit reads if exposed
- staged reads with minimum budget
- staged reads with intermediate budget
- staged reads with full budget
- periodic settings/config snapshots
- periodic health/status reads
- periodic diagnostic reads
- periodic probe/detect
- periodic recover
- safe mode changes
- safe timing and averaging/filter boundary checks
- safe threshold/limit toggles that do not create dangerous physical states
- periodic data/accumulator reset and post-reset reads, if applicable
- periodic software reset or fixed-step reset job if the session remains stable
  and the script can re-synchronize cleanly

The runner must record:

- start and end timestamps
- elapsed duration
- command counts by type
- pass/fail/unknown/not-run counts
- consecutive failure bursts
- min/mean/max command latency
- worst read latency
- effective sample rate by read path
- health state before, during, and after
- last error and total failure counters if exposed
- reset/recovery counts
- serial reconnects or target reboots
- memory/heap information if exposed
- diagnostic evidence observed and whether it was expected

The soak must stop early and mark `FAIL` if:

- the communication link cannot be recovered within bounded reconnect attempts
- the target stops responding
- the driver remains offline/unusable after bounded recovery attempts
- unexpected identity mismatch, persistent timeout, persistent bus error, or
  repeated data-corruption symptoms are observed
- measured timing exceeds the documented bound by a large margin and repeats
- any unsafe electrical, thermal, mechanical, or other physical condition is
  observed

If the 8-hour run cannot be completed, record the actual runtime, why it
stopped, what evidence was captured, and whether the partial run is useful.

### Invalid Input And Error Classification

Exercise CLI and API-facing validation where exposed:

- invalid command
- invalid address/endpoint
- invalid mode
- invalid timing setting
- invalid averaging/filter setting
- invalid delay
- invalid threshold/limit values
- invalid raw register/field/command
- read before initialization only if a command can safely reach it
- timeout/NACK/bus/data error classification if the fixture supports safe fault
  injection

Verify that errors are precise and visible.

## Implementation Rules During HIL Work

- Add only narrowly scoped commands needed to expose existing library behavior.
- Do not add production simulation paths.
- Do not add broad registries, plugin systems, generic hardware managers, or
  placeholder abstractions.
- Keep each new command bounded and documented in help or equivalent docs.
- All new HIL script actions must have finite timeouts.
- If a command can alter hardware state, the runner must restore the previous
  safe state or mark the hardware dirty and require recovery/reinitialization.
- The runner must stop or downgrade to `NOT RUN` when the fixture cannot safely
  support a test.

## Audit After Testing

After HIL execution, audit the library and examples using the report evidence.

For each finding, include:

- severity: critical, high, medium, low
- file and function/line reference
- observed HIL evidence or reason from code inspection
- current behavior
- why it is risky
- simplest safe fix
- required native tests
- required HIL regression test
- whether you implemented the fix in this pass

Prioritize:

1. incorrect success after hardware failure
2. stale or unsafe data
3. hangs, unbounded waits, or unbounded allocation
4. cache/hardware or state desynchronization
5. destructive status reads that lose evidence
6. reset/recovery failures
7. unclear public API behavior
8. documentation or example drift

Implement only small, concrete fixes that are clearly supported by the HIL
evidence and native tests. Stop and report if a fix requires an architectural
decision.

## Required Verification After Any Fix

Run the smallest relevant check after each fix. Before final response, run as
much of the repository's validation set as the machine supports.

For PlatformIO-style repositories, this may include:

```powershell
python tools\<hil-runner>.py --parser-self-test
python tools\<hil-runner>.py --dry-run
python tools\<hil-runner>.py --port COMXX --baud 115200 --timeout-s 5 --verbose
python tools\<contract-check>.py
pio test -e native
pio run -e <environment>
pio pkg pack
git diff --check
```

For other repositories, infer and document equivalent unit tests, static
checks, builds, package checks, and HIL commands.

If a platform-native build tool is available, run the example/application build
for the actual target or document why it was not run.

Remove generated package archives after package checks unless the user asks to
keep them.

## Final Response Requirements

Keep the final response concise and factual. Include:

- report path
- serial port used
- firmware/application environment used
- files changed
- HIL summary: pass/fail/unknown/not-run counts
- timing/sampling highlights
- soak-test result and duration
- failures found
- fixes implemented
- tests/builds run and results
- hardware or fixture limitations
- CI status if checked

Do not say HIL passed unless the hardware run completed and the report contains
the evidence.
