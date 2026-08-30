# Hardware Validation Procedure

This is a procedure and evidence template. It is not a hardware-validation
claim. Record completed runs under:

```text
docs/validation/hardware/YYYY-MM-DD/<short-commit>-<framework>-<target>-addr-0xNN/
```

## Safety Gate

Do not run validation on unsafe rails. INA228 supports high common-mode voltage,
but this library and its examples do not make a system safe. Before applying
power, document isolation, creepage/clearance, fusing/current limiting, shunt
dissipation, transient protection, grounding, USB ground relationship,
enclosure/access control, and qualified operator approval.

Never connect non-isolated, mains-derived, or high-energy rails to a
development board, debugger, or USB-connected PC without proper isolation and
protection.

## Evidence Required For Every PASS/FAIL

- Exact git commit.
- Date/time and operator.
- Framework and target.
- Build, upload/flash, and monitor commands.
- ESP32 board/revision.
- INA228 module/PCB, package/variant if known, and address straps.
- Shunt value, tolerance, power rating, temperature coefficient, and Kelvin
  connection notes.
- Supply/load type, current limit, bus voltage range, and load profile.
- Reference equipment model/serial/calibration state where available.
- Raw command transcript and any DMM/scope/load captures.
- Pass/fail result, measured values, limits, and notes.

## Capture Presets

Install `pyserial` in the runner environment before automated HIL capture. A
release run starts from a clean checkout; the runner validates the firmware's
library version, exact 12-character Git commit identity, source status, and exact
profile framework tokens before accepting the `version` step.

Arduino ESP32-S3:

```powershell
$Port = "COMx"
$Run = "docs\validation\hardware\YYYY-MM-DD\<short-commit>-arduino-esp32s3dev-addr-0x40"
$LibraryVersion = (Get-Content library.json -Raw | ConvertFrom-Json).version
$Commit = git rev-parse --short=12 HEAD
New-Item -ItemType Directory -Force $Run
git rev-parse HEAD | Tee-Object "$Run\commit.txt"
git status --porcelain --untracked-files=normal | Tee-Object "$Run\source-status.txt"
.\scripts\pio.cmd run -e esp32s3dev 2>&1 | Tee-Object "$Run\build.txt"
.\scripts\pio.cmd run -e esp32s3dev -t upload --upload-port $Port 2>&1 | Tee-Object "$Run\upload.txt"
python tools\run_i2c_hil.py --port $Port --baud 115200 --profile arduino --expected-library-version $LibraryVersion --expected-commit $Commit --expected-git-status clean --suite exhaustive --require-framed --fail-on-unknown --include-not-run --benchmark-count 100 --operator "<name>" --board "<board/revision>" --environment "esp32s3dev" --fixture "<fixture/load>" --safety "<approved limits>" --report "$Run\hil-exhaustive.md" --transcript "$Run\hil-exhaustive-transcript.txt" 2>&1 | Tee-Object "$Run\hil-exhaustive-runner.txt"
Start-Transcript -Path "$Run\monitor-transcript.txt"
.\scripts\pio.cmd device monitor -e esp32s3dev -p $Port
Stop-Transcript
```

Use `esp32s2dev` for Arduino ESP32-S2.

Pure ESP-IDF ESP32-S3:

```powershell
$Port = "COMx"
$Run = "docs\validation\hardware\YYYY-MM-DD\<short-commit>-idf-esp32s3-addr-0x40"
$LibraryVersion = (Get-Content library.json -Raw | ConvertFrom-Json).version
$Commit = git rev-parse --short=12 HEAD
New-Item -ItemType Directory -Force $Run
git rev-parse HEAD | Tee-Object "$Run\commit.txt"
git status --porcelain --untracked-files=normal | Tee-Object "$Run\source-status.txt"
idf.py -C examples/esp_idf/basic set-target esp32s3 build 2>&1 | Tee-Object "$Run\build.txt"
idf.py -C examples/esp_idf/basic -p $Port flash 2>&1 | Tee-Object "$Run\flash.txt"
python tools\run_i2c_hil.py --port $Port --baud 115200 --profile idf --expected-library-version $LibraryVersion --expected-commit $Commit --expected-git-status clean --suite exhaustive --require-framed --fail-on-unknown --include-not-run --benchmark-count 100 --operator "<name>" --board "<board/revision>" --environment "esp-idf-v6.0.1-esp32s3" --fixture "<fixture/load>" --safety "<approved limits>" --report "$Run\hil-exhaustive.md" --transcript "$Run\hil-exhaustive-transcript.txt" 2>&1 | Tee-Object "$Run\hil-exhaustive-runner.txt"
Start-Transcript -Path "$Run\flash-monitor-transcript.txt"
idf.py -C examples/esp_idf/basic -p $Port monitor
Stop-Transcript
```

Use `esp32s2` for pure ESP-IDF ESP32-S2.

## Minimum Test Set

Run each item for Arduino and ESP-IDF targets where hardware is available:

| Area | Commands/procedure | Expected evidence |
| --- | --- | --- |
| Address and identity | `scan`, `scanina`, `addr`, `init`, `probe`, `mfgid`, `devid` | Correct address handling, manufacturer `0x5449`, DIEID `0x228`, separately recorded revision, precise transport errors. |
| MEMSTAT and DIAG_ALRT | `diag`, `diagraw`, `selftest` | MEMSTAT healthy and destructive/status-clearing behavior visible. |
| Voltage/current scaling | `cal`, `cfg`, `vbus`, `vshunt`, `current`, `power`, `read`, `raw` | Values agree with reference equipment within recorded tolerances. |
| ADCRANGE | `adcrange 0`, `adcrange 1`, repeated calibration/readback | The 15 mOhm/10 A demonstration profile must reject range 1 as unsafe. Physical range-1 coverage requires a separate electrically compatible calibration/load profile; scaling then remains coherent and no dirty state remains after success. |
| Triggered freshness | `trigger`, repeated `ready`, `read` | Not-ready before CNVRF and fresh data after completion. |
| Cooperative transfer budgets | `sample_step`, `apply_step`, `reset_step`, `xfer_reset`, `xfer_assert`, `xfer_stats`, or the `transfer` diagnostic subset of the exhaustive runner | A zero budget is accepted and bus-silent; positive budgets never exceed the requested callback count; wait gates consume zero callbacks; terminal result is delivered once. |
| Continuous accumulation | `mode 15`, `rstacc`, `ready`, `energy`, `charge`, `read` | Accumulation valid only after continuous CNVRF and no overflow is hidden. |
| Alerts | threshold commands, `limits`, `diag`, `diagraw`, ALERT pin capture | Alert polarity/latch and threshold flags match controlled crossings. |
| Reset/recovery | `rstacc`, `reset_start`/`reset_step`, `probe`, `recover`, fault injection if safe | Reset startup wait is bus-silent; invalidation/reinitialization is bounded; partial/indeterminate effects require reconciliation. |
| Stress/soak | `stress`, `stress_mix`, repeated reads over time | No unbounded waits; all faults and recoveries are logged. |

Mark unsupported fixture tests as `NOT RUN` only when the limitation is
documented. Do not count them as passes.

## Command Sequences

The examples below use the 15 mOhm / 10 A demonstration profile unless the
operator changes it. Confirm it is safe for the connected hardware first. Send
every command through framed `hilrun` mode when collecting release evidence; see
the <a href="../CLI.md">CLI and HIL reference</a> for framing, suites, verdicts,
and the canonical runner invocation.

### Identity and configuration

```text
version
scan
init 0xNN
probe
mfgid
devid
cfg
timing
drv
```

Replace `0xNN` with the healthy address reported by `scan`. `scan` already
includes the INA228 identity/MEMSTAT probe; use `scanina` only for an
INA228-only repeat scan, and note that it reads status-sensitive `DIAG_ALRT`
and can consume CNVRF or latched evidence.

Expected identity is manufacturer `0x5449`, DIEID `0x228`, and an explicitly
reported/accepted revision. Do not validate identity by masking arbitrary high
bits or by assuming the whole DEVICE_ID equals `0x2281`.

### Cooperative sample budget (max 11 callbacks)

```text
xfer_reset
sample_step 0
xfer_assert 0 0 0

xfer_reset
sample_step 1
xfer_assert 1 0 1

xfer_reset
sample_step 2
xfer_assert 1 1 2
```

Wait longer than the configured conversion time, then finish the active sample:

```text
xfer_reset
sample_step 8
xfer_assert 7 1 8
```

The terminal output must be exactly one `Cooperative Sample Result` carrying
operation ID, request token, configuration generation, fixed-unit values, raw
values, channel validity, and diagnostic evidence. No intermediate command may
expose a partial sample as successful.

Repeat with a fresh job and a large budget:

```text
sample_step 255
sample_step 255
```

The first command may stop at the zero-I2C conversion wait even with remaining
budget; the second completes once the wait has elapsed. Total callbacks for the
job must not exceed 11.

### Cooperative reinitialization budget (max 14 callbacks)

```text
apply_start
xfer_reset
apply_step 0
xfer_assert 0 0 0

xfer_reset
apply_step 1
xfer_assert 1 0 1

xfer_reset
apply_step 13
xfer_assert 7 6 13
```

The job performs no driver retry and must end only after identity, MEMSTAT,
desired configuration, calibration, alert defaults, temperature coefficient, and
ADC profile have all been read back successfully.

### Cooperative reset budget and wait (max 16 callbacks)

```text
reset_start
xfer_reset
reset_step 0
xfer_assert 0 0 0

xfer_reset
reset_step 1
xfer_assert 0 1 1
```

The first callback writes reset. Before the datasheet startup delay expires,
repeat a zero-budget step and verify it remains bus-silent:

```text
xfer_reset
reset_step 0
xfer_assert 0 0 0
```

After the wait, finish with:

```text
reset_step 15
xfer_assert 9 6 15
```

The whole maintenance job includes full verified initialization. No blind retry
is permitted after an ambiguous reset write.

### Accumulator and diagnostics

```text
rstacc
diagsnap
diag
diagsnap
integer
```

Record that `diag` is destructive/status-sensitive while `diagsnap` is
cache-only. Verify accumulation is never reported valid across calibration,
ADCRANGE, mode/timing, triggered-operation, temperature-compensation, or reset
generation changes until a verified accumulator reset establishes a coherent
epoch.

### Fault injection

At each meaningful cooperative stage inject, where the fixture safely permits:
definite address NACK; data NACK or NACK with unknown phase; transfer timeout;
arbitration/bus error; removal and reappearance; failed read after a successful
write; ambiguous failed write callback; and owner cancellation or deadline
timeout before a write, after a confirmed write, and after an ambiguous write.

For every case record the request token and operation ID, the callback count and
outer deadline, the terminal state/status/effect, whether a result was delivered
exactly once, the resulting hardware state (`UNKNOWN`, `SYNCHRONIZED`, or
`RESYNC_REQUIRED`), the application bus recovery/retry action, and the
successful verified reconciliation before any subsequent publication.

A write callback is one physical attempt. Do not let the fixture or adapter
blindly retry an ambiguous mutation.

### Alert capture

```text
alatch 0
apol 0
cnvralert 0
alslow 0
limits
```

Then exercise each threshold with safe controlled crossings while capturing
ALERT with an independent instrument. Record polarity, latch/transparent
clearing, conversion-ready routing, and diagnostic evidence. ALERT is monitoring
only and is not a safety interlock.

### Stress and soak

After the exhaustive suite passes with no FAIL/UNKNOWN rows:

```text
stress_mix 1000
stress 1000
```

Then run the framed 8-hour soak from a clean commit. Serial framing loss is
`UNKNOWN`, never `PASS`. Record device removal/reappearance and owner recovery
separately from steady-state measurement reliability.
