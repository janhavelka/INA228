# Native ESP-IDF INA228 bring-up CLI

This project is a native ESP-IDF implementation of the repository's diagnostic
CLI. It uses `app_main`, `driver/i2c_master.h`, `esp_timer`, FreeRTOS delays, and
fixed C buffers. It does not include Arduino sources or use `Arduino.h`,
`Wire.h`, `String`, `Serial`, or a compatibility facade.

Its command grammar and HIL frames match the
[Arduino example](../../01_basic_bringup_cli/README.md). The complete contract
is in the [CLI and HIL reference](../../../docs/CLI.md).

## Reference wiring and calibration

The native example owns these defaults:

| Setting | Value |
| --- | --- |
| SDA | GPIO 8 |
| SCL | GPIO 9 |
| I2C clock | 400 kHz |
| I2C transaction timeout | 50 ms |
| Initial INA228 address | `0x40` |
| Shunt | 15 mΩ |
| Maximum expected current | 10 A |
| Calibration mode | `FROM_MAXIMUM_CURRENT` |

Change the application source and rebuild for different hardware. The driver
itself remains framework-neutral and never owns or configures the I2C bus. On
startup the example scans the bus and all INA228 addresses from `0x40` through
`0x4F`; use `init 0x4x` if the strapped address differs from the default.

The example has a fixed-unit calibration contract. `cal` displays it, while
`cal <shunt_ohm> <max_current_a>` returns `INVALID_CONFIG`; changing the shunt
or maximum current requires a source change and reinitialization.

> **Safety:** this example does not make an 85 V system safe. Use suitable
> isolation, fusing, creepage/clearance, shunt power and thermal analysis, and
> careful serial/USB grounding. Start with a current-limited low-voltage
> fixture.

## Build, flash, and open the CLI

CI builds this example with ESP-IDF v6.0.1 for ESP32-S3 and ESP32-S2. In an
activated ESP-IDF environment, verify the selected toolchain and build one
target:

```powershell
idf.py --version
idf.py -C examples/esp_idf/basic set-target esp32s3 build
```

Use `esp32s2` instead for ESP32-S2. Flash and monitor the selected target:

```powershell
idf.py -C examples/esp_idf/basic -p COMx flash
idf.py -C examples/esp_idf/basic -p COMx monitor
```

The console runs at 115200 baud. Enter `help` for the live command list. A
conservative first session is:

```text
version
scan
scanina
init
settings
drv
timing
vbus
vshunt
temp
diagsnap
```

Review the [status-sensitive and raw-access notes](../../../docs/CLI.md#status-sensitive-and-raw-access)
before using `diag`, `diagraw`, arbitrary register access, or `wreg16`.
`wreg16` requires a lowercase trailing `confirm`, can alter reserved or
status-sensitive bits, and can leave cached configuration out of sync.

## Hardware-in-the-loop runs

After flashing firmware built from the source tree being validated, run the same
framed suite with the native profile using the canonical `--profile idf`
invocation from the [HIL runner reference](../../../docs/CLI.md#hil-runner); it
is not duplicated here so the two cannot drift.

The `idf` profile currently requires `Runtime: native ESP-IDF v6.0.1` in the
`version` output, plus the expected library version, commit, and clean/dirty
source status. The example CMake build refreshes and embeds the commit and
worktree status in the firmware before every build. See the
[HIL runner reference](../../../docs/CLI.md#hil-runner)
for dry runs, parser checks, suite composition, report metadata, soak options,
and verdict definitions.
