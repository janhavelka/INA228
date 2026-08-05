# INA228 diagnostic CLI

The repository provides the same diagnostic command contract in two example
firmwares:

- Arduino/PlatformIO bring-up CLI: `examples/01_basic_bringup_cli/README.md`
- native ESP-IDF bring-up CLI: `examples/esp_idf/basic/README.md`

These CLIs are development, validation, and hardware-in-the-loop (HIL) tools.
They are not part of the INA228 library API and are not intended to be copied
unchanged into a production command interface.

> **Electrical safety:** neither the INA228, this library, nor these examples
> make an 85 V system safe. Use a qualified high-voltage design, suitable
> isolation, fusing, creepage and clearance, shunt power/thermal analysis, and
> deliberate USB-ground handling. The documented HIL flow is not evidence of
> high-voltage validation.

## Arduino and native ESP-IDF parity

The Arduino and native ESP-IDF examples have identical help rows, command
names, aliases, argument grammar, strict parsing rules, library-status names,
and `hilrun` framing. Repository contract checks compare the help rows in order
and verify that every documented alias has dispatch code in both examples.

Parity does not mean byte-for-byte console output. The startup banner,
framework provenance, logging decoration, and I2C transport implementation are
platform-specific:

- Arduino reports its Arduino-ESP32 and underlying ESP-IDF versions and uses
  the application-owned `Wire` adapter in `examples/common/`.
- The native example reports `Runtime: native ESP-IDF ...` and uses the ESP-IDF
  `i2c_master` driver directly. It does not use Arduino compatibility types.

Use a 115200 baud serial terminal with LF line endings. Enter `help` or `?` to
print the command list implemented by the flashed firmware.

## Argument parsing

Arguments are parsed strictly and must consume the complete token:

- Integer arguments accept decimal and `0x` hexadecimal forms, subject to the
  command's documented range. Unsigned arguments reject a leading minus sign.
- Signed and unsigned overflow is rejected.
- Floating-point arguments must be finite and in range; trailing characters,
  `nan`, and `inf` are rejected.
- Boolean arguments accept only numeric `0` or `1`. Values such as `true`,
  `yes`, `2`, and `1junk` are invalid.
- Missing, extra, malformed, or out-of-range arguments produce a usage/error
  message. Inside `hilrun`, these failures are reported as
  `status=INVALID_PARAM`.

For example, `mode 3`, `addr 0x41`, and `tempcomp 1` are valid; `mode 3x`,
`addr -1`, and `tempcomp true` are not.

## Command reference

Square brackets mean that the argument is optional. Angle brackets mean that
it is required.

### Common and measurement

| Command | Purpose |
| --- | --- |
| `help` / `?` | Show help. |
| `version` / `ver` | Print firmware, framework, library version, commit, and source-tree status. |
| `scan` | Scan the I2C bus and probe addresses `0x40`-`0x4F` for INA228 identities. The INA228 identity pass reads `DIAG_ALRT`/`MEMSTAT`. |
| `scanina` | Probe INA228 identities and read `DIAG_ALRT`/`MEMSTAT`. This consumes status-sensitive evidence. |
| `read` | Read converted measurements, accumulator-validity flags, overflow flags, and the preserved diagnostic snapshot. |
| `raw` | Read the raw measurement registers with accumulator-validity and overflow flags. See [status-sensitive reads](#status-sensitive-and-raw-access). |
| `integer` / `int` | Run one bounded cooperative instantaneous sample and print fixed-unit integer results. |
| `diagsnap` | Print the cached `DIAG_ALRT` snapshot without performing I2C. |
| `timing` | Print conversion-ready, estimated conversion time, and calibration information. |
| `vbus` | Read bus voltage. |
| `vshunt` | Read shunt voltage. |
| `temp` | Read die temperature. |
| `current` | Read calibrated current. |
| `power` | Read calibrated power. |
| `energy` | Read accumulated energy; valid only under continuous-accumulation conditions. |
| `charge` | Read accumulated charge; valid only under continuous-accumulation conditions. |
| `ready` | Check whether a conversion is ready. |
| `trigger [mode]` | Trigger a single conversion; optional mode is `1`-`7`. |
| `ready_step <budget>` | Poll readiness with a bounded instruction budget. |
| `sample_step <budget>` | Start or advance an instantaneous sample by at most the specified I2C-transfer budget. |

### Configuration and cooperative jobs

| Command | Purpose |
| --- | --- |
| `mode [0..15]` | Show or set the INA228 operating mode. |
| `convtime [vbus|vsh|temp <0..7>]` | Show conversion times or set one channel's conversion-time index. |
| `averaging [0..7]` | Show or set the averaging index. |
| `adcrange [0|1]` | Show or set the shunt ADC range. The active calibration contract can reject an incompatible range. |
| `cal [shunt_ohm max_current_a]` | Show calibration. This example's fixed-unit profile rejects runtime changes; see [fixed calibration](#fixed-demo-calibration-contract). |
| `tempco [ppm]` | Show or set the shunt temperature coefficient. |
| `tempcomp [0|1]` | Show or enable/disable temperature compensation. |
| `delay [0..255]` | Show or set conversion delay in 2 ms steps. |
| `cfg` / `settings` | Print active settings. |
| `addr [0x40..0x4F]` | Show or select the target address. Selecting an address does not itself initialize the device. |
| `init [0x40..0x4F]` | Reinitialize at the selected or supplied INA228 address. |
| `end` | End the driver session. |
| `reset` | Perform the example's bounded software-reset flow. |
| `reset_start` / `reset_step <budget>` | Start or advance the cooperative reset job. |
| `rstacc` | Reset energy and charge accumulators and establish a new accumulation epoch. |
| `apply_start` / `apply_step <budget>` | Start or advance verified cooperative reinitialization. |
| `replay_start` / `replay_step <budget>` | Alias for the verified reinitialization job. |

A cooperative `*_start` command creates a job. Repeated `*_step <budget>` calls
advance it by no more than the supplied instruction/I2C-transfer budget.
`IN_PROGRESS` is a normal nonterminal result; continue polling until `OK` or a
terminal error is returned. A zero budget is meaningful for the job poll APIs,
but `ready_step` requires a positive budget.

### Alerts and register diagnostics

| Command | Purpose |
| --- | --- |
| `diag` | Decode `DIAG_ALRT`; the hardware read is destructive/status-clearing. |
| `diagraw` | Print raw `DIAG_ALRT`; the hardware read is destructive/status-clearing. |
| `limits` | Read and decode all alert-limit registers. |
| `alatch [0|1]` | Show or set alert latch mode. The query reads `DIAG_ALRT`. |
| `cnvralert [0|1]` | Show or set conversion-ready alert output. The query reads `DIAG_ALRT`. |
| `alslow [0|1]` | Show or set slow-alert mode. The query reads `DIAG_ALRT`. |
| `apol [0|1]` | Show or set alert polarity. The query reads `DIAG_ALRT`. |
| `sovl [volts]` | Show or set shunt-overvoltage threshold. |
| `suvl [volts]` | Show or set shunt-undervoltage threshold. |
| `bovl [volts]` | Show or set bus-overvoltage threshold. |
| `buvl [volts]` | Show or set bus-undervoltage threshold. |
| `tmplim [degC]` | Show or set temperature-over-limit threshold. |
| `pwrlim [watts]` | Show or set power-over-limit threshold. |
| `mfgid` | Read manufacturer ID; expected value is `0x5449`. |
| `devid` | Read `DEVICE_ID`; expected die ID is `0x228` plus a revision nibble. |
| `reg16 <addr>` | Read an arbitrary 16-bit register. This can clear flags for a status-sensitive address. |
| `reg24 <addr>` | Read an arbitrary 24-bit register. Intended for diagnosis only. |
| `reg40 <addr>` | Read an arbitrary 40-bit register. Accumulator reads can consume overflow evidence. |
| `wreg16 <addr> <val> confirm` | Write an arbitrary 16-bit register after an explicit confirmation token. This can desynchronize cached configuration. |

### Driver, stress, and HIL diagnostics

| Command | Purpose |
| --- | --- |
| `drv` | Print driver state and health counters. |
| `probe` | Probe the selected device without health tracking. It reads `DIAG_ALRT`. |
| `recover` | Invalidate cached hardware state and run bounded reinitialization. |
| `verbose [0|1]` | Show or set verbose example output. |
| `stress [N]` | Run `N` measurement cycles; default is 10. |
| `stress_mix [N]` | Run `N` mixed-operation cycles; default is 50. |
| `hilrun <token> <seq> <cmd>` | Execute one inner command inside a machine-readable HIL frame. |
| `hilmark <token>` | Emit a legacy `HILMARK` token. New automation should use `hilrun`. |
| `xfer_reset` | Reset example-transport read/write counters. |
| `xfer_stats` | Print example-transport counters. |
| `xfer_assert <r> <w> <t>` | Assert exact read, write, and total transport counts. |
| `selftest` | Run the diagnostic self-test. It reads `DIAG_ALRT`. |

Stress counts must be positive and are bounded by the example. Stress commands
are diagnostics, not qualification or lifetime tests.

## Fixed demo calibration contract

Both examples bind the same calibration during initialization:

| Setting | Value |
| --- | --- |
| Shunt resistance | `15000` micro-ohms (`0.015` ohm, 15 mΩ) |
| Calibration mode | `FROM_MAXIMUM_CURRENT` |
| Maximum expected current | `10000` mA (10 A) |
| Initial operating mode | `CONT_ALL` |

This is a fixture-specific demonstration profile, not a universal sensor-board
default. Change the source configuration and rebuild for hardware with a
different shunt or current range. The command `cal` reports the active values,
but `cal <shunt_ohm> <max_current_a>` returns `INVALID_CONFIG` because changing
a fixed-unit calibration requires rebinding/reinitialization. Do not interpret
plausible current, power, energy, or charge output as valid if the firmware
profile does not match the physical shunt.

## Status-sensitive and raw access

`DIAG_ALRT` combines configuration with live clear-on-read diagnostic evidence.
The following operations therefore need deliberate use:

- `diag`, `diagraw`, `scan`, `scanina`, `probe`, `selftest`, and the alert-mode
  query forms read `DIAG_ALRT` and can clear live hardware flags.
- `reg16 0x0B` is the raw form of the same destructive read. Other arbitrary
  register reads must still use the width defined by the datasheet.
- `read` and `raw` intentionally capture `DIAG_ALRT` before reading the
  accumulators. The consumed value is returned with the sample and retained in
  the driver's cache; `diagsnap` retrieves that cache without another I2C read.
- `current`, `power`, `energy`, and `charge` consume a diagnostic snapshot to
  validate math/overflow state. `ready`, `ready_step`, `timing`, `integer`, and
  `sample_step` can also read `DIAG_ALRT` while checking readiness or producing
  a current-derived sample. The driver preserves every successful tracked
  diagnostic read in the same cache.
- `raw` values are not automatically valid engineering results. In particular,
  ENERGY and CHARGE must be interpreted only when their printed validity flags
  are true. Accumulator mode, epoch, and overflow state still apply.

`wreg16` is more dangerous than the read commands. It requires the literal,
lowercase final token `confirm`, for example:

```text
wreg16 0x00 0x0000 confirm
```

This confirmation prevents an accidental write; it does not make the value
safe. A direct write can reset the chip, change modes or scaling, alter reserved
bits, clear evidence, or leave the driver's cached configuration different from
hardware. After a deliberate diagnostic write, use `recover` and complete the
cooperative reinitialization before trusting normal measurements. Prefer the
typed configuration commands whenever they cover the intended operation.

## Framed HIL protocol

The preferred automation command is:

```text
hilrun <token> <sequence> <inner command>
```

For a valid frame request, the firmware emits exactly one begin boundary,
command payload, and end boundary:

```text
HIL_BEGIN token=T123 seq=7
  Status: OK (code=0, detail=0)
HIL_END token=T123 seq=7 status=OK elapsed_ms=3
```

The token and sequence in both boundaries echo the request. `elapsed_ms` is the
firmware-side elapsed time for the inner command. An empty inner command or a
nested `hilrun` is rejected with `status=INVALID_PARAM`. The runner also rejects
missing, mismatched, truncated, or oversized frames instead of attributing
unrelated serial output to a command.

The `status` field is an uppercase `INA228::Err` name. Important classes are:

Interactive status output uses the same rule in both firmware profiles: a
successful operation prints one green `Status: OK` line. The yellow `Message:`
line is emitted only for non-OK diagnostic context, so success is not repeated
or shown with a warning color.

| Status | Meaning in a frame |
| --- | --- |
| `OK` | Inner command completed successfully. |
| `IN_PROGRESS` | A cooperative operation remains active; this is nonterminal, not a failure. |
| `INVALID_PARAM` | Command grammar, numeric conversion, confirmation, or range validation failed. |
| `INVALID_CONFIG`, `NOT_INITIALIZED`, `NOT_BOUND`, `BUSY` | A library precondition or active configuration prevented the operation. |
| `MEASUREMENT_NOT_READY`, `RESULT_NOT_AVAILABLE`, `STALE_RESULT` | Measurement/job result is not currently consumable. |
| `I2C_NACK_ADDR`, `I2C_NACK_DATA`, `I2C_NACK_UNKNOWN_PHASE`, `I2C_TIMEOUT`, `I2C_BUS`, `I2C_ERROR` | Transport failure, with as much phase detail as the platform provides. |
| `DEVICE_NOT_FOUND`, `DEVICE_ID_MISMATCH`, `UNSUPPORTED_REVISION`, `MEMORY_ERROR` | Device identity, revision, or trim-memory validation failed. |
| `MATH_OVERFLOW`, `ACCUMULATION_INVALID`, `ACCUMULATION_OVERFLOW` | Measurement or accumulator validity failed. |
| `HARDWARE_DIRTY`, `HARDWARE_STATE_UNKNOWN`, `CONFIG_MISMATCH` | Cached/desired configuration is not verified against hardware. |
| `TIMEOUT`, `OPERATION_TIMEOUT`, `CANCELLED` | A bounded operation did not complete or was cancelled. |

The exhaustive list and numeric values are defined by `INA228::Err` in
`include/INA228/Status.h`. Automation should compare the symbolic name, not its
numeric value.

`hilmark <token>` supports the runner's older command-plus-marker mode. It does
not carry a command status and is less robust than `hilrun`. Raw unframed serial
commands are intended for interactive diagnosis only.

## HIL runner

`tools/run_i2c_hil.py` drives either flashed CLI through pyserial. Its suite
selection is cumulative:

| Suite | Coverage |
| --- | --- |
| `smoke` | Provenance, discovery, initialization, health, diagnostic, and raw-read basics. |
| `functional` | Smoke plus normal help and measurement paths. |
| `targeted` | Smoke plus feature sweep, strict rejection cases, and cooperative-job behavior. |
| `transfer` | Smoke plus exact I2C-transfer budget assertions. |
| `exhaustive` | Smoke, functional, feature sweep, targeted, and transfer coverage. |

Run parser checks and inspect a plan without hardware:

```powershell
python tools\run_i2c_hil.py --parser-self-test
python tools\test_run_i2c_hil_parser.py
python tools\run_i2c_hil.py --suite exhaustive --dry-run --include-not-run --benchmark-count 100
```

For a provenance-bound Arduino run from a clean tree:

```powershell
$Version = (Get-Content library.json | ConvertFrom-Json).version
$Commit = git rev-parse --short=12 HEAD
python tools\run_i2c_hil.py --port COMx --baud 115200 --profile arduino --expected-library-version $Version --expected-commit $Commit --expected-git-status clean --suite exhaustive --require-framed --fail-on-unknown --include-not-run --benchmark-count 100 --report hil-arduino.md --transcript hil-arduino.txt
```

For the native ESP-IDF firmware:

```powershell
$Version = (Get-Content library.json | ConvertFrom-Json).version
$Commit = git rev-parse --short=12 HEAD
python tools\run_i2c_hil.py --port COMx --baud 115200 --profile idf --expected-library-version $Version --expected-commit $Commit --expected-git-status clean --suite exhaustive --require-framed --fail-on-unknown --include-not-run --benchmark-count 100 --report hil-idf.md --transcript hil-idf.txt
```

Replace `COMx` and add the report metadata options (`--operator`, `--board`,
`--environment`, `--fixture`, `--safety`, and `--notes`) for retained evidence.
The runner defaults the expected library version from `library.json`, the
expected commit from the current 12-character `HEAD`, and expected status to
`clean`; the explicit arguments above make the evidence contract visible.

The current profile defaults require these `version` output tokens:

- `arduino`: `Arduino-ESP32: 3.3.11` and `ESP-IDF: v5.5.5`
- `idf`: `Runtime: native ESP-IDF v6.0.1`

Use repeatable `--framework-token` arguments only when intentionally validating
a different framework build. `--expected-library-version any`,
`--expected-commit any`, and `--expected-git-status any` relax provenance and
should not be used for release evidence. If a test firmware was intentionally
built from a dirty worktree, specify `--expected-git-status dirty` and record
why; do not claim it as clean release evidence.

### Runner verdicts

The frame `status` and runner verdict are different layers:

| Verdict | Meaning |
| --- | --- |
| `PASS` | Required output tokens appeared and no unexpected failure signature was present. Expected negative tests can pass by producing their specified error. |
| `FAIL` | A failure signature, unexpected non-OK status, failed assertion, self-test failure, or provenance mismatch was observed. |
| `UNKNOWN` | Output was incomplete or ambiguous, required tokens were missing, or framing was missing/mismatched/truncated. |
| `NOT RUN` | A known fixture-dependent check or unrequested soak was recorded as not executed. It is not a pass. |

Any `FAIL` makes the runner return nonzero. `UNKNOWN` returns nonzero when
`--fail-on-unknown` is supplied; use that option for validation gates.
`--include-not-run` records known gaps such as disconnected-target, bus-fault,
alert-pin, reset/power-control, or an unrequested eight-hour soak. Use
`--soak-hours 8` for the documented long soak and `--stop-on-non-pass` when an
`UNKNOWN` should stop that soak immediately (`FAIL` always stops it).
In generated reports, `Commands executed` excludes these explicit `NOT RUN`
rows; `Commands recorded in detail` includes them so fixture gaps remain
visible.
