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
python -m platformio run -e esp32s3dev 2>&1 | Tee-Object "$Run\build.log"
python -m platformio run -e esp32s3dev -t upload --upload-port $Port 2>&1 | Tee-Object "$Run\upload.log"
python tools\run_i2c_hil.py --port $Port --baud 115200 --suite targeted --require-framed --fail-on-unknown --report "$Run\hil-targeted.md" --transcript "$Run\hil-targeted.log" 2>&1 | Tee-Object "$Run\hil-targeted-runner.log"
python tools\run_i2c_hil.py --port $Port --baud 115200 --suite transfer --require-framed --fail-on-unknown --report "$Run\hil-transfer.md" --transcript "$Run\hil-transfer.log" 2>&1 | Tee-Object "$Run\hil-transfer-runner.log"
Start-Transcript -Path "$Run\monitor-transcript.log"
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
idf.py -C examples/esp_idf/basic set-target esp32s3 build 2>&1 | Tee-Object "$Run\build.log"
Start-Transcript -Path "$Run\flash-monitor-transcript.log"
idf.py -C examples/esp_idf/basic -p $Port flash monitor
Stop-Transcript
```

Use `esp32s2` for pure ESP-IDF ESP32-S2.

## Minimum Test Set

Run each item for Arduino and ESP-IDF targets where hardware is available:

| Area | Commands/procedure | Expected evidence |
| --- | --- | --- |
| Address and identity | `scan`, `scanina`, `addr`, `init`, `probe`, `mfgid`, `devid` | Correct address handling, `0x5449`, `0x2281`, precise transport errors. |
| MEMSTAT and DIAG_ALRT | `diag`, `diagraw`, `selftest` | MEMSTAT healthy and destructive/status-clearing behavior visible. |
| Voltage/current scaling | `cal`, `cfg`, `vbus`, `vshunt`, `current`, `power`, `read`, `raw` | Values agree with reference equipment within recorded tolerances. |
| ADCRANGE | `adcrange 0`, `adcrange 1`, repeated calibration/readback | Scaling remains coherent and no dirty state remains after success. |
| Triggered freshness | `trigger`, repeated `ready`, `read` | Not-ready before CNVRF and fresh data after completion. |
| Fixed-step transfer budgets | `ready_step`, `sample_step`, `apply_step`, `reset_step`, `xfer_reset`, `xfer_assert`, `xfer_stats`, or `tools/run_i2c_hil.py --suite transfer --require-framed` | Deterministic zero-budget and sample-step paths touch only the documented number of example transport callbacks; paths where the example loop can call `tick()` between serial commands must record `xfer_stats` and explain any background readiness reads. |
| Continuous accumulation | `mode 15`, `rstacc`, `ready`, `energy`, `charge`, `read` | Accumulation valid only after continuous CNVRF and no overflow is hidden. |
| Alerts | threshold commands, `limits`, `diag`, `diagraw`, ALERT pin capture | Alert polarity/latch and threshold flags match controlled crossings. |
| Reset/recovery | `rstacc`, `reset`, `probe`, `recover`, fault injection if safe | Dirty/recovery behavior is explicit and bounded. |
| Stress/soak | `stress`, `stress_mix`, repeated reads over time | No unbounded waits; all faults and recoveries are logged. |

Mark unsupported fault-injection tests as `N/A` only when the fixture limitation
is documented.
