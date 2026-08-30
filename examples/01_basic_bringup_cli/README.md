# Arduino INA228 bring-up CLI

This PlatformIO example provides the Arduino implementation of the repository's
diagnostic CLI. Its command grammar and HIL framing match the
[native ESP-IDF example](../esp_idf/basic/README.md); use the central
[CLI and HIL reference](../../docs/CLI.md) for the complete command list,
strict parsing rules, destructive-read warnings, and runner verdicts.

The CLI is example and validation code, not part of the library API.

## Reference wiring and calibration

The checked-in example defaults are:

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

These are application-owned reference settings in `examples/common/BoardConfig.h`
and `main.cpp`; the INA228 library does not configure pins or own `Wire`.
Change them and rebuild when your board, address straps, shunt, or current range
differs. On startup the example scans the bus and all valid INA228 addresses.
If the device is not at `0x40`, use `addr 0x4x` followed by `init`, or
`init 0x4x`.

The runtime `cal <shunt_ohm> <max_current_a>` form is intentionally rejected by
this fixed-unit profile. `cal` without arguments displays the active contract.

> **Safety:** this example does not make an 85 V system safe. Use suitable
> isolation, fusing, creepage/clearance, shunt power and thermal analysis, and
> careful USB-ground handling. Start bring-up on a current-limited low-voltage
> fixture.

## Build, flash, and open the CLI

On Windows, use the repository PlatformIO wrapper:

```powershell
.\scripts\pio.cmd run -e esp32s3dev
.\scripts\pio.cmd run -e esp32s3dev -t upload --upload-port COMx
.\scripts\pio.cmd device monitor -e esp32s3dev -p COMx
```

Use `esp32s2dev` instead for the supplied ESP32-S2 environment. The serial
monitor is configured for 115200 baud, LF line endings, and no DTR/RTS reset.
Enter `help` for the live command list.

A conservative first session is:

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

Read the [status-sensitive and raw-access notes](../../docs/CLI.md#status-sensitive-and-raw-access)
before using `diag`, `diagraw`, arbitrary `reg*` commands, or `wreg16`.
`wreg16` requires the literal trailing word `confirm` and can desynchronize the
driver's cached configuration.

## Hardware-in-the-loop runs

The runner uses framed `hilrun` commands by default. After flashing firmware
built from the tree you intend to validate, run the canonical `--profile arduino`
invocation from the [HIL runner reference](../../docs/CLI.md#hil-runner); it is
not duplicated here so the two cannot drift.

The `arduino` profile currently verifies `Arduino-ESP32: 3.3.11` and
`ESP-IDF: v5.5.5` in the `version` response in addition to library version,
commit, and clean/dirty status. See the [HIL runner reference](../../docs/CLI.md#hil-runner)
for parser tests, other suites, framework-token overrides, soak options, and
the distinction between frame statuses and runner verdicts.
