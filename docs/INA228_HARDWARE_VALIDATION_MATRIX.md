# INA228 Hardware Validation Matrix

Status: DRAFT - all required hardware rows are `NOT RUN`.
Last updated: 2026-06-05
Reference commit at template creation: `1da6bd0a91d85c846703dd7275aea74fd8d71426`
Branch: `hardening/ina228-industry-readiness`

This file records required hardware validation evidence. It is not a hardware
validation claim until rows are updated from `NOT RUN` with dated logs,
equipment, board/module details, operator, and pass/fail notes.

Allowed row status values: `NOT RUN`, `PASS`, `FAIL`, `BLOCKED`, `N/A`.

Evidence path pattern:

```text
docs/validation/hardware/YYYY-MM-DD/<short-commit>-<framework>-<target>-addr-0xNN/
```

Example path:

```text
docs/validation/hardware/2026-06-05/1da6bd0-arduino-esp32s3dev-addr-0x40/
```

## Safety Gate

Do not run hardware validation on unsafe rails. INA228 supports high
common-mode/bus voltage, but this library and the example board do not make a
system safe. Before applying power, document isolation, creepage/clearance,
fusing/current limiting, shunt dissipation, transient protection, grounding, USB
ground relationship, enclosure/access control, and qualified operator approval.

Never connect non-isolated, mains-derived, or high-energy rails to a
development board, debugger, or USB-connected PC without proper isolation and
protection.

## Required Metadata For Every PASS/FAIL Row

- Date and operator.
- Exact git commit under test.
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

| ID | Status | Date | Commit | Framework / Target | Board | INA228 Module | Address / Straps | Shunt | Supply / Load | Bus Voltage Range | Equipment | Procedure | Evidence Path | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| HW-ADDR-001 | NOT RUN | TBD | TBD | Arduino S2/S3 and IDF S2/S3 | TBD | TBD | 0x40-0x4F / A0-A1 straps TBD | TBD | Safe current-limited I2C setup | 0 V or safe low voltage | Logic analyzer optional | Run `scan`, `scanina`, then for each available strap/address run `addr 0xNN`, `init 0xNN`, `probe`. Verify empty slots NACK cleanly and healthy INA228 devices report valid ID/MEMSTAT. | `$RUN/address-scan-0x40-0x4f.log` | Required for all address strap combinations available to the fixture. |
| HW-ID-001 | NOT RUN | TBD | TBD | Arduino S2/S3 and IDF S2/S3 | TBD | TBD | Active address TBD | TBD | Powered INA228 | Safe low voltage | Serial log | Run `mfgid`, `devid`, `probe`; expect `MANUFACTURER_ID=0x5449`, `DEVICE_ID=0x2281`. | `$RUN/identity.log` | Include raw status on mismatch. |
| HW-MEMSTAT-001 | NOT RUN | TBD | TBD | Arduino S2/S3 and IDF S2/S3 | TBD | TBD | Active address TBD | TBD | Powered INA228 | Safe low voltage | Serial log | Run `scanina`, `diag`, `selftest`; expect MEMSTAT healthy. Note that DIAG_ALRT reads are destructive/status-clearing. | `$RUN/memstat.log` | Preserve first diagnostic snapshot in log. |
| HW-VBUS-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | TBD | Calibrated voltage source / no unsafe rail | 0 V, mid-range, high-range below fixture safety limit | Calibrated DMM/source | Apply documented safe known source points. Run `vbus`, `read`, `raw`; compare against DMM/source. | `$RUN/vbus-known-source.log` | Do not use mains-derived sources without isolation. |
| HW-VSHUNT-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Known precision shunt or source | Known differential shunt voltage | Common-mode within fixture limit | DMM/source | Apply known differential shunt voltage. Run `vshunt`, `raw`; compare against DMM/source. | `$RUN/vshunt-known-source.log` | Verify polarity. |
| HW-CURRENT-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Value/tolerance/power/TCR TBD | Known load/current-limited supply | Safe range TBD | DMM/e-load/source | Run `cal <shunt_ohm> <max_current_a>`, `timing`, `current`, `power`, `read`; compare with shunt value and load current. | `$RUN/current-power-known-load.log` | Record shunt dissipation and temperature. |
| HW-BIDIR-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Bidirectional-capable fixture TBD | Reversible current path | Safe range TBD | DMM/e-load/source | Reverse current path or use bidirectional supply/load. Run `vshunt`, `current`, `charge`, `read`; verify sign handling. | `$RUN/bidirectional-current.log` | Mark `N/A` only if fixture cannot safely reverse current. |
| HW-ADCRANGE-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Value/tolerance/power/TCR TBD | Known load/current-limited supply | Safe range TBD | DMM/e-load/source | Run `adcrange 0`, `cal ...`, measurements; repeat `adcrange 1`, `cal ...`; verify scaling and no dirty hardware state. | `$RUN/adcrange-0-1.log` | Reapply thresholds after scale changes. |
| HW-TIMING-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | TBD | Stable source/load | Safe range TBD | Timestamped serial log, scope or logic analyzer | For indices `0..7`, run `convtime vbus N`, `convtime vsh N`, `convtime temp N`, `averaging N`, `timing`, `trigger`, `ready`; compare CNVRF timing. | `$RUN/conversion-timing-averaging.log` | Include conversion delay if used. |
| HW-MODE-CONT-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Calibrated shunt | Stable source/load | Safe range TBD | Serial log | Run `mode 15`, `read`, repeated `ready`, `read`, `raw`; verify latest-register behavior and valid accumulation after CNVRF. | `$RUN/continuous-mode.log` | Record interval and health counters. |
| HW-MODE-TRIG-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Calibrated shunt | Stable source/load | Safe range TBD | Timestamped serial log | Run `trigger` or `trigger 7`, poll `ready`, then `read`; verify not-ready before CNVRF and fresh data after completion. | `$RUN/triggered-mode.log` | Include behavior when `Config::nowMs` is used. |
| HW-ACCUM-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Calibrated shunt | Known load over measured time | Safe range TBD | DMM/e-load/source/timer | In continuous mode run `rstacc`, wait for CNVRF, then `energy`, `charge`, `read` over known time/load; verify joules/coulombs and overflow flags. | `$RUN/energy-charge.log` | ENERGY/CHARGE invalid in triggered/shutdown modes. |
| HW-ALERT-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Calibrated shunt | Source/load capable of crossing limits safely | Safe range TBD | DMM/source/scope on ALERT | Run `limits`, `alatch`, `cnvralert`, `alslow`, `apol`, `sovl`, `suvl`, `bovl`, `buvl`, `tmplim`, `pwrlim`; verify `diag`/`diagraw` and ALERT pin. | `$RUN/alerts.log` | ALERT is not a safety interlock. |
| HW-NACK-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | TBD | Safe switched I2C fixture | Safe low voltage | Serial log, optional logic analyzer | Use a safe switched fixture to disconnect INA228/I2C during `stress 100` or `stress_mix 100`; run `drv`, `probe`, `recover` before/after reconnect. | `$RUN/nack-unplug-replug.log` | Verify precise NACK/timeout/bus status. |
| HW-RESET-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Calibrated shunt | Safe power-cycle/brownout fixture | Safe range TBD | Serial log, power supply log | Brownout/power-cycle INA228 with MCU alive, then MCU with INA228 alive; run `drv`, `probe`, `recover`, `reset`, `read`; verify resync. | `$RUN/brownout-reset.log` | Record dirty state and recovery status. |
| HW-SOAK-001 | NOT RUN | TBD | TBD | Arduino and IDF | TBD | TBD | Active address TBD | Calibrated shunt | Stable source/load | Safe range TBD | Serial logger, source/load logs | Run enough scripted cycles for 24-72h, for example `stress 1000000`; record wall-clock start/end, health counters, min/avg/max readings, and errors. | `$RUN/soak-24-72h.log` | Stop on unsafe temperature or fault. |
| HW-FW-ARD-S3-001 | NOT RUN | TBD | TBD | Arduino / `esp32s3dev` | ESP32-S3 board TBD | TBD | Active address TBD | TBD | Safe source/load | Safe range TBD | PlatformIO build/upload/monitor logs | Build/upload/monitor `esp32s3dev`; run `version`, `scanina`, `selftest`, `read`, `stress_mix 100`. | `$RUN/arduino-esp32s3.log` | Framework smoke row. |
| HW-FW-ARD-S2-001 | NOT RUN | TBD | TBD | Arduino / `esp32s2dev` | ESP32-S2 board TBD | TBD | Active address TBD | TBD | Safe source/load | Safe range TBD | PlatformIO build/upload/monitor logs | Build/upload/monitor `esp32s2dev`; run `version`, `scanina`, `selftest`, `read`, `stress_mix 100`. | `$RUN/arduino-esp32s2.log` | Framework smoke row. |
| HW-FW-IDF-S3-001 | NOT RUN | TBD | TBD | Pure ESP-IDF / `esp32s3` | ESP32-S3 board TBD | TBD | Active address TBD | TBD | Safe source/load | Safe range TBD | `idf.py` build/flash/monitor logs | Build/flash/monitor `idf.py -C examples/esp_idf/basic set-target esp32s3 build`; run `version`, `scanina`, `selftest`, `read`, `stress_mix 100`. | `$RUN/idf-esp32s3.log` | Requires local or CI IDF logs plus hardware transcript. |
| HW-FW-IDF-S2-001 | NOT RUN | TBD | TBD | Pure ESP-IDF / `esp32s2` | ESP32-S2 board TBD | TBD | Active address TBD | TBD | Safe source/load | Safe range TBD | `idf.py` build/flash/monitor logs | Build/flash/monitor `idf.py -C examples/esp_idf/basic set-target esp32s2 build`; run `version`, `scanina`, `selftest`, `read`, `stress_mix 100`. | `$RUN/idf-esp32s2.log` | Requires local or CI IDF logs plus hardware transcript. |

## Current Evidence Summary

- No hardware validation rows are currently marked `PASS`.
- No dated `docs/validation/hardware/...` logs are checked in.
- Local `idf.py` was not available during the hardening chunks, so local pure
  ESP-IDF builds were not run here.
- GitHub Actions is configured to build the pure ESP-IDF example, but CI build
  configuration is not a hardware validation result.
