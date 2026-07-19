# INA228 v3 HIL command sequence

This is a procedure template, not evidence. Run on a clean, exact commit and
store the framed transcript under `docs/validation/hardware/` with fixture,
shunt, address, equipment, operator, and safety information.

The examples use a 15 mOhm/10 A demonstration profile unless changed by the
operator. Confirm it is safe for the connected hardware before running.

## Basic identity and configuration

```text
version
scan
scanina
probe
mfgid
devid
cfg
timing
drv
```

Expected identity is manufacturer `0x5449`, DIEID `0x228`, and an explicitly
reported/accepted revision. Do not validate identity by masking arbitrary high
bits or by assuming the whole DEVICE_ID must always equal `0x2281`.

## Cooperative sample transfer budget

Run commands with framed `hilrun` mode when collecting release evidence. The
following counts refer to example transport callbacks, not logic-analyzer byte
counts.

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

The terminal output must be one `Cooperative Sample Result` with operation ID,
request token, configuration generation, fixed-unit values, raw values, channel
validity, and diagnostic evidence. No intermediate command may expose a partial
sample as successful.

Repeat with a fresh job and a large budget:

```text
sample_step 255
sample_step 255
```

The first command can stop at the zero-I2C conversion wait even with remaining
budget. The second command, after elapsed time, completes. Total callbacks for
the job must not exceed 11.

## Cooperative reinitialization budget

```text
apply_start
xfer_reset
apply_step 0
xfer_assert 0 0 0

xfer_reset
apply_step 1
xfer_assert 1 0 1

apply_step 13
```

The job maximum is 14 callbacks, performs no driver retry, and must end only
after identity, MEMSTAT, desired configuration, calibration, alert defaults,
temperature coefficient, and ADC profile have been read back successfully.

## Cooperative reset budget and wait

```text
reset_start
xfer_reset
reset_step 0
xfer_assert 0 0 0

xfer_reset
reset_step 1
xfer_assert 0 1 1
```

The first callback writes reset. Before the data-sheet startup delay expires,
repeat a zero-budget step and verify it remains bus-silent:

```text
xfer_reset
reset_step 0
xfer_assert 0 0 0
```

After the wait, finish with:

```text
reset_step 15
```

The whole maintenance job maximum is 16 callbacks, including full verified
initialization. No blind retry is permitted after an ambiguous reset write.

## Accumulator and diagnostics

```text
rstacc
diagsnap
diag
diagsnap
integer
```

Record that `diag` is destructive/status-sensitive while `diagsnap` is
cache-only. Verify accumulation is not reported valid across calibration,
ADCRANGE, mode/timing, triggered-operation, temperature-compensation, or reset
generation changes until a verified accumulator reset establishes a coherent
epoch.

## Fault injection

At each meaningful cooperative stage inject, where the fixture safely permits:

- definite address NACK;
- data NACK or NACK with unknown phase;
- transfer timeout;
- arbitration/bus error;
- removal and reappearance;
- failed read after a successful write;
- ambiguous failed write callback;
- owner cancellation and deadline timeout before a write, after a confirmed
  write, and after an ambiguous write.

For every case record:

- exact request token and operation ID;
- callback count and outer deadline;
- terminal state/status/effect;
- whether a result was delivered exactly once;
- hardware state (`UNKNOWN`, `SYNCHRONIZED`, or `RESYNC_REQUIRED`);
- application bus recovery/retry action;
- successful verified reconciliation before subsequent publication.

A write callback is one physical attempt. Do not make the fixture or adapter
blindly retry an ambiguous mutation.

## Alert and electrical validation

Use safe controlled crossings and independent instruments for:

```text
alatch 0
apol 0
cnvralert 0
alslow 0
limits
```

Then exercise each threshold while capturing ALERT. Record polarity,
latch/transparent clearing, conversion-ready routing, and diagnostic evidence.
ALERT is monitoring only and is not a safety interlock.

## Stress and soak

After the targeted and transfer suites pass with no FAIL/UNKNOWN rows:

```text
stress_mix 1000
stress 1000
```

Run the framed 8-hour soak from a clean commit. Serial framing loss is UNKNOWN,
not PASS. Record device removal/reappearance and owner recovery separately from
steady-state measurement reliability.
