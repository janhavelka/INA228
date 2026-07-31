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

Arduino ESP32-S3:

```powershell
$Port = "COMx"
$Run = "docs\validation\hardware\YYYY-MM-DD\<short-commit>-arduino-esp32s3dev-addr-0x40"
New-Item -ItemType Directory -Force $Run
git rev-parse HEAD | Tee-Object "$Run\commit.txt"
python -m platformio run -e esp32s3dev 2>&1 | Tee-Object "$Run\build.txt"
python -m platformio run -e esp32s3dev -t upload --upload-port $Port 2>&1 | Tee-Object "$Run\upload.txt"
python tools\run_i2c_hil.py --port $Port --baud 115200 --suite exhaustive --require-framed --fail-on-unknown --include-not-run --benchmark-count 100 --report "$Run\hil-exhaustive.md" --transcript "$Run\hil-exhaustive-transcript.txt" 2>&1 | Tee-Object "$Run\hil-exhaustive-runner.txt"
Start-Transcript -Path "$Run\monitor-transcript.txt"
python -m platformio device monitor -e esp32s3dev -p $Port
Stop-Transcript
```

Use `esp32s2dev` for Arduino ESP32-S2.

Pure ESP-IDF ESP32-S3:

```powershell
$Port = "COMx"
$Run = "docs\validation\hardware\YYYY-MM-DD\<short-commit>-idf-esp32s3-addr-0x40"
New-Item -ItemType Directory -Force $Run
git rev-parse HEAD | Tee-Object "$Run\commit.txt"
idf.py -C examples/esp_idf/basic set-target esp32s3 build 2>&1 | Tee-Object "$Run\build.txt"
Start-Transcript -Path "$Run\flash-monitor-transcript.txt"
idf.py -C examples/esp_idf/basic -p $Port flash monitor
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
