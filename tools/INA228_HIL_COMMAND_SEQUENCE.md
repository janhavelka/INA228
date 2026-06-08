# INA228 HIL Command Sequence Template

This is a no-hardware transcript template for future hardware-in-loop runs. It
documents the command sequence to paste into either bring-up CLI:

- Arduino: `examples/01_basic_bringup_cli`
- Pure ESP-IDF: `examples/esp_idf/basic`

Do not mark any hardware validation row `PASS` unless the full transcript,
setup metadata, commit hash, and equipment details are checked in under
`docs/validation/hardware/...`.

## Transcript Header

Record this before opening the serial monitor:

```text
Commit:
Date/time:
Operator:
Framework/target:
Build command:
Board:
INA228 module:
Address straps:
Shunt value/tolerance/power/TCR:
Supply/load:
Bus voltage range:
Equipment:
Log directory:
Safety review complete: yes/no
```

## Baseline CLI Smoke

These commands exist in both Arduino and ESP-IDF examples:

```text
version
help
scan
scanina
addr 0x40
init 0x40
probe
mfgid
devid
cfg
drv
diag
diagraw
timing
read
raw
selftest
stress 500
stress_mix 200
```

If the active address is not `0x40`, replace `0x40` with the documented strap
address from `0x40` through `0x4F`.

## Measurement And Calibration Sequence

Use safe low-energy sources first. Do not connect high-energy rails or
USB-grounded development boards to unsafe systems.

```text
adcrange 0
cal <shunt_ohm> <max_current_a>
cfg
vbus
vshunt
current
power
read
raw
adcrange 1
cal <shunt_ohm> <max_current_a>
cfg
vbus
vshunt
current
power
read
raw
```

Replace `<shunt_ohm>` and `<max_current_a>` with the actual fixture values.
Record the expected current from the reference DMM/e-load/source.

## Mode And Accumulator Sequence

```text
mode 15
ready
rstacc
ready
energy
charge
read
trigger 7
ready
read
energy
charge
mode 0
energy
charge
mode 15
```

Expected behavior:

- Continuous mode can make ENERGY/CHARGE valid after a continuous CNVRF.
- Triggered and shutdown modes must not report ENERGY/CHARGE as valid.
- Triggered reads must not report stale data as a fresh completed conversion.

## Alert And DIAG Sequence

Only cross thresholds with safe sources and current limits.

```text
limits
alatch 1
cnvralert 1
alslow 0
apol 1
sovl <volts>
suvl <volts>
bovl <volts>
buvl <volts>
tmplim <degC>
pwrlim <watts>
limits
diag
diagraw
diag
```

`DIAG_ALRT` reads are destructive/status-clearing. Preserve command order in
the transcript.

## Reset And Recovery Sequence

```text
cfg
drv
rstacc
cfg
ready
reset
probe
cfg
read
drv
recover
probe
read
```

Use the hardware matrix for MCU-reset, INA228-reset/brownout, address NACK,
and timeout/stuck-bus fault injection steps. Do not simulate bus faults without
a safe fixture.

## Missing Convenience Aliases

The prompt examples sometimes use these names, but the current CLI does not
implement them:

- `id`: use `mfgid` and `devid`.
- `read raw`: use `raw`.

Recommended future additions, if desired: add harmless aliases for `id` and
`read raw` to reduce operator mistakes during validation transcripts.
