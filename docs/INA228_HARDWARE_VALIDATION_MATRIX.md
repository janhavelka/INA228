# INA228 Hardware Validation Matrix

Status: DRAFT - all hardware rows are `NOT RUN`.
Last updated: 2026-06-08
Reference commit at template update: `dd553f2a30c6c9be55b03690ceaa54a3d1b1134b`
Branch: `hardening/ina228-industry-readiness`

This document is a validation plan and evidence index. It is not a hardware
validation claim. A row may move from `NOT RUN` only when dated logs and
equipment/setup details are checked in under the row's log path.

Allowed row status values: `NOT RUN`, `PASS`, `FAIL`, `BLOCKED`, `N/A`.

Evidence path pattern:

```text
docs/validation/hardware/YYYY-MM-DD/<short-commit>-<framework>-<target>-addr-0xNN/
```

Example path:

```text
docs/validation/hardware/2026-06-08/dd553f2-arduino-esp32s3dev-addr-0x40/
```

## Safety Gate

Do not run hardware validation on unsafe rails. INA228 supports high
common-mode/bus voltage, but this library and its examples do not make a
system safe. Before applying power, document isolation, creepage/clearance,
fusing/current limiting, shunt dissipation, transient protection, grounding,
USB ground relationship, enclosure/access control, and qualified operator
approval.

Never connect non-isolated, mains-derived, or high-energy rails to a
development board, debugger, or USB-connected PC without proper isolation and
protection. ALERT output and measurement readings are monitoring aids, not
certified safety functions.

## Required Evidence For Every PASS/FAIL Row

- Exact git commit under test.
- Date/time and operator.
- Framework/target and build command.
- ESP32 board and revision.
- INA228 module/PCB, INA228 package/variant if known, and address straps.
- Shunt value, tolerance, power rating, temperature coefficient, and Kelvin
  connection description.
- Supply/load type, current limit, bus voltage range, and load profile.
- Test equipment model/serial/calibration state where available.
- Command transcript/log path and any scope/DMM/load screenshots or captures.
- Pass/fail result, measured values, limits, and notes.

## Build And Capture Presets

Arduino ESP32-S3 example:

```powershell
$Port = "COMx"
$Run = "docs\validation\hardware\YYYY-MM-DD\<short-commit>-arduino-esp32s3dev-addr-0x40"
New-Item -ItemType Directory -Force $Run
git rev-parse HEAD | Tee-Object "$Run\commit.txt"
python -m platformio run -e esp32s3dev 2>&1 | Tee-Object "$Run\build.log"
python -m platformio run -e esp32s3dev -t upload --upload-port $Port 2>&1 | Tee-Object "$Run\upload.log"
Start-Transcript -Path "$Run\monitor-transcript.log"
python -m platformio device monitor -e esp32s3dev -p $Port
Stop-Transcript
```

Use `esp32s2dev` for Arduino ESP32-S2.

Pure ESP-IDF ESP32-S3 example:

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

## Matrix

| Test ID | Purpose | Required setup | Command/procedure | Expected result | Actual result | Status | Commit | Date/time | Board | INA228 module | Shunt value/tolerance | Equipment | Log path | Operator notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| HW-ADDR-001 | Address scan 0x40-0x4F. | Safe powered I2C bus; address straps available or fixture documents unavailable straps. | Run `scan`, `scanina`; for each available address run `addr 0xNN`, `init 0xNN`, `probe`. | Empty addresses show precise NACK/no device; populated INA228 addresses identify cleanly. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/address-scan-0x40-0x4f.log` | Record A0/A1 strap state for each address. |
| HW-ID-MFG-001 | MANUFACTURER_ID identity. | Powered INA228 at active address. | Run `mfgid` and capture raw register value. | `MANUFACTURER_ID=0x5449`; errors preserve transport status. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/manufacturer-id.log` | Include first failed raw value on mismatch. |
| HW-ID-DEV-001 | DEVICE_ID identity. | Powered INA228 at active address. | Run `devid` and capture raw register value. | `DEVICE_ID=0x2281`; errors preserve transport status. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/device-id.log` | Include revision bits if printed. |
| HW-MEMSTAT-001 | MEMSTAT health from DIAG_ALRT. | Powered INA228 at active address. | Run `diag`, `selftest`; preserve the first diagnostic read. | MEMSTAT healthy; destructive read behavior is visible in transcript. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/memstat.log` | Note that `diag`/`diagraw` can clear status evidence. |
| HW-VBUS-001 | Bus voltage known-source accuracy. | Calibrated voltage source within fixture safety limit; DMM reference. | Apply documented safe voltage points; run `vbus`, `read`, `raw`. | Reported VBUS agrees with source/DMM within stated tolerance. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/vbus-known-source.log` | Do not use unsafe or non-isolated high-energy rails. |
| HW-VSHUNT-001 | Shunt voltage known-source accuracy and polarity. | Known differential shunt source or precision shunt/load; DMM reference. | Apply positive and, if safe, negative shunt voltage; run `vshunt`, `raw`. | Reported VSHUNT magnitude and sign match reference. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/vshunt-known-source.log` | Record common-mode voltage and polarity wiring. |
| HW-CURRENT-CAL-001 | Current calibration with known shunt/load. | Precision shunt with known tolerance/TCR/power rating; current-limited source/load; DMM or e-load. | Run `cal <shunt_ohm> <max_current_a>`, `cfg`, `current`, `power`, `read`. | Current and power agree with shunt/load references; calibration values are logged. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/current-calibration.log` | Record shunt temperature rise and dissipation. |
| HW-ADCRANGE0-001 | ADCRANGE 0 scaling. | Same calibrated fixture as current test; shunt voltage kept inside +/-163.84 mV range. | Run `adcrange 0`, `cal <shunt_ohm> <max_current_a>`, `cfg`, `vshunt`, `current`, `read`. | Scaling and current LSB are coherent; no hardware-dirty state remains. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/adcrange-0.log` | Reapply thresholds after range changes. |
| HW-ADCRANGE1-001 | ADCRANGE 1 scaling. | Same calibrated fixture; shunt voltage kept inside +/-40.96 mV range. | Run `adcrange 1`, `cal <shunt_ohm> <max_current_a>`, `cfg`, `vshunt`, `current`, `read`. | Scaling and current LSB are coherent; no hardware-dirty state remains. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/adcrange-1.log` | Verify expected finer shunt-voltage resolution. |
| HW-BIDIR-001 | Negative/bidirectional current handling if fixture supports it. | Bidirectional supply/load or reversible current fixture with safe isolation. | Reverse current path safely; run `vshunt`, `current`, `charge`, `read`, `raw`. | Signed VSHUNT/CURRENT/CHARGE behavior matches current direction. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/bidirectional-current.log` | Mark `N/A` only with documented fixture limitation. |
| HW-TIMING-MIN-001 | Minimum conversion-time and averaging timing. | Stable source/load; timestamped log and optional logic analyzer/scope. | Run `convtime vbus 0`, `convtime vsh 0`, `convtime temp 0`, `averaging 0`, `timing`, `trigger`, repeated `ready`. | CNVRF timing is bounded by documented estimate/tolerance. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/timing-min.log` | Include conversion delay if configured. |
| HW-TIMING-DEFAULT-001 | Default conversion-time and averaging timing. | Stable source/load; timestamped log. | Run `reset` or re-init default config, then `timing`, `trigger`, repeated `ready`. | Default timing matches driver estimate and device behavior. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/timing-default.log` | Preserve config snapshot. |
| HW-TIMING-MAX-001 | Maximum conversion-time and averaging timing. | Stable source/load; long timeout monitor. | Run `convtime vbus 7`, `convtime vsh 7`, `convtime temp 7`, `averaging 7`, `timing`, `trigger`, repeated `ready`. | Long conversion is bounded; no stale triggered read is reported as fresh. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/timing-max.log` | Ensure monitor timeout exceeds expected conversion time. |
| HW-MODE-CONT-001 | Continuous mode latest-register behavior. | Calibrated fixture with stable source/load. | Run `mode 15`, `ready`, `read`, `raw`, wait, repeat `ready`, `read`. | Continuous reads return latest registers; accumulation becomes valid after CNVRF. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/continuous-mode.log` | Record interval between reads. |
| HW-MODE-TRIG-001 | Triggered mode freshness and not-ready behavior. | Calibrated fixture with stable source/load. | Run `trigger 7`, immediately run `ready` and `read`, then poll `ready` until complete and run `read`. | Before completion, fresh read is not-ready; after CNVRF, fresh data is reported. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/triggered-mode.log` | Preserve timestamps for each poll. |
| HW-ACCUM-CONT-001 | ENERGY/CHARGE accumulation in continuous mode. | Calibrated shunt/load and independent timer/reference. | Run `mode 15`, `rstacc`, wait for CNVRF, then run `energy`, `charge`, `read` over a known load interval. | ENERGY/CHARGE agree with expected joules/coulombs within tolerance and no overflow is hidden. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/energy-charge-continuous.log` | Record load stability and elapsed time. |
| HW-ACCUM-INVALID-001 | ENERGY/CHARGE invalidity in triggered/shutdown modes. | Calibrated fixture; safe stable source/load. | Run `trigger 7`, then `energy`, `charge`; run `mode 0`, then `energy`, `charge`. | ENERGY/CHARGE return invalid/not-ready status rather than valid values. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/energy-charge-invalid.log` | `mode 0` is shutdown. |
| HW-ALERT-001 | Thresholds and ALERT pin behavior. | Source/load that can cross limits safely; scope/logic analyzer on ALERT. | Configure `alatch`, `cnvralert`, `alslow`, `apol`, `sovl`, `suvl`, `bovl`, `buvl`, `tmplim`, `pwrlim`; run `limits`, `diag`, `diagraw`. | ALERT polarity/latch and threshold flags match configured crossings. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/alerts.log` | ALERT is not a safety interlock. |
| HW-DIAG-001 | DIAG_ALRT destructive/status-sensitive behavior. | Fixture that can create or preserve diagnostic flags; serial transcript. | Capture `diag`, `diagraw`, repeated `diag`, and relevant triggering action such as `ready` or alert threshold crossing. | First read preserves/logs evidence; subsequent read shows documented clear-on-read behavior where applicable. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/diag-destructive.log` | Preserve ordering exactly. |
| HW-RSTACC-001 | Reset and RSTACC behavior. | Calibrated continuous fixture. | Run `rstacc`, `cfg`, `ready`, `energy`, `charge`; run `reset`, `probe`, `cfg`, `read`. | Accumulation invalidates after RSTACC until continuous CNVRF; reset recovers identity/MEMSTAT/config or reports dirty state. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/reset-rstacc.log` | Include settings before/after. |
| HW-MCU-RESET-001 | MCU reset while INA228 remains powered. | Fixture where MCU can reset without INA228 power loss. | Keep INA228 powered, reset ESP32, run `version`, `probe`, `cfg`, `diag`, `read`, `recover` if needed. | Driver reinitializes deterministically without false hardware validation claims. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/mcu-reset-ina-powered.log` | Record reset method and power rails. |
| HW-INA-RESET-001 | INA228 reset/brownout while MCU remains powered. | Current-limited switchable INA228 supply or safe brownout fixture. | Keep MCU alive, reset/brownout INA228, run `drv`, `probe`, `recover`, `reset`, `read`. | Precise errors during outage; recovery/resync succeeds or reports clear failure/dirty state. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/ina-reset-mcu-powered.log` | Do not hot-plug unsafe rails. |
| HW-NACK-001 | Unplug/replug or address NACK behavior. | Safe switched I2C fixture or address mismatch method. | Use safe disconnect or wrong `addr`; run `probe`, `read`, `stress 100`, reconnect and run `recover`, `probe`. | Address NACK and recovery behavior are precise and logged. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/nack-unplug-replug.log` | Optional logic analyzer strongly preferred. |
| HW-TIMEOUT-001 | Stuck/timeout bus if safely simulatable. | Bus fault injection fixture that cannot damage ESP32/INA228. | Hold SCL/SDA in a safe scripted fault if fixture supports it; run `read`, `drv`, `recover`. | Timeout/bus status remains precise and no unbounded wait occurs. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/stuck-timeout-bus.log` | Mark `N/A` if no safe fault fixture exists. |
| HW-SOAK-001 | 24-72 h soak. | Stable safe source/load, logging host, thermal monitoring. | Run repeated `read`, `diag`, `drv`, `stress_mix 200` cycles for 24-72 h with timestamps. | No unbounded failures; all errors and recovery attempts are logged; temperatures remain safe. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/soak-24-72h.log` | Stop on unsafe temperature or supply/load fault. |
| HW-FW-ARD-S2-001 | Arduino ESP32-S2 hardware smoke. | ESP32-S2 board and INA228 fixture. | Build/upload/monitor `esp32s2dev`; run `version`, `scanina`, `selftest`, `read`, `stress_mix 100`. | Arduino S2 example builds, boots, and runs CLI validation transcript. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/arduino-esp32s2.log` | Include build and upload logs. |
| HW-FW-ARD-S3-001 | Arduino ESP32-S3 hardware smoke. | ESP32-S3 board and INA228 fixture. | Build/upload/monitor `esp32s3dev`; run `version`, `scanina`, `selftest`, `read`, `stress_mix 100`. | Arduino S3 example builds, boots, and runs CLI validation transcript. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/arduino-esp32s3.log` | Include build and upload logs. |
| HW-FW-IDF-S2-001 | Pure ESP-IDF ESP32-S2 hardware smoke. | ESP32-S2 board, INA228 fixture, ESP-IDF available. | Build/flash/monitor `idf.py -C examples/esp_idf/basic set-target esp32s2 build`; run `version`, `scanina`, `selftest`, `read`, `stress_mix 100`. | Pure ESP-IDF S2 example builds, boots, and runs CLI validation transcript. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/idf-esp32s2.log` | Requires `idf.py` build and hardware transcript. |
| HW-FW-IDF-S3-001 | Pure ESP-IDF ESP32-S3 hardware smoke. | ESP32-S3 board, INA228 fixture, ESP-IDF available. | Build/flash/monitor `idf.py -C examples/esp_idf/basic set-target esp32s3 build`; run `version`, `scanina`, `selftest`, `read`, `stress_mix 100`. | Pure ESP-IDF S3 example builds, boots, and runs CLI validation transcript. | NOT RUN | NOT RUN | TBD | TBD | TBD | TBD | TBD | TBD | `$RUN/idf-esp32s3.log` | Requires `idf.py` build and hardware transcript. |

## Current Evidence Summary

- No hardware validation rows are currently marked `PASS`.
- No dated `docs/validation/hardware/...` logs are checked in.
- Local `idf.py` was not available during the no-hardware checks, so local
  pure ESP-IDF builds were not run here.
- GitHub Actions is configured to build the pure ESP-IDF example, but CI build
  configuration is not a hardware validation result.
- Native fake-bus tests and local no-hardware builds are useful evidence, but
  they do not replace this matrix.
