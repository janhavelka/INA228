# INA228 I2C Uniformization Prompt

Repository: `INA228`

Absolute path: `C:\Users\Honza\Documents\Projects\INA228`

## Execution Rules

You are working inside this single repository. Implement this prompt directly;
do not repeat the cross-repository audit.

You may spawn subagents for read-only inspection of APIs, tests, I2C
transactions, docs, and diagnostics. Keep final judgment, edits, and
verification in the main agent.

Prefer simple, robust, readable code. Before adding code, inspect whether
existing code can be simplified, reused, tightened, or deleted.

Preserve dirty user changes. Do not commit unless explicitly asked.

## Common Uniformization Target

Apply this shared I2C library contract: injected non-owning transport, `Status` returns, cache-only `getSettings(SettingsSnapshot&) const`, active `probe()`/diagnostics named explicitly, `DriverState` with `state()` and `driverState()`, `isOnline()`, `lastOkMs()`, `lastErrorMs()`, `lastError()`, `consecutiveFailures()`, `totalFailures()`, and `totalSuccess()`.

Keep the common `Err` vocabulary append-only where missing: `OK`, `NOT_INITIALIZED`, `INVALID_CONFIG`, `INVALID_PARAM`, `I2C_ERROR`, `I2C_NACK_ADDR`, `I2C_NACK_DATA`, `I2C_TIMEOUT`, `I2C_BUS`, `DEVICE_NOT_FOUND`, `TIMEOUT`, `BUSY`, and `IN_PROGRESS`. Preserve INA228-specific device-ID, accumulation, math, DIAG_ALRT, and hardware-dirty codes.

Uniformization is not a new base class or framework. Make only local, source-compatible additions and tests.

## Current State

- Public lifecycle and health are in `include\INA228\INA228.h`: `DriverState` at line 21, `begin()` near line 179, `probe()` at line 207, `recover()` at line 213, `getSettings(SettingsSnapshot&)` at line 218, `state()` at line 230, `lastOkMs()` through `totalSuccess()` at lines 243-258.
- The driver already has diagnostic evidence and dirty hardware state concepts: `SettingsSnapshot` around `include\INA228\INA228.h:79`, DIAG_ALRT snapshot handling at `src\INA228.cpp:560`, and dirty/diagnostic comments at `include\INA228\INA228.h:594-596`.
- Public register helpers exist for 16/24/40-bit register widths at `include\INA228\INA228.h:635-653`.
- Raw/tracked I2C and `_updateHealth()` are implemented in `src\INA228.cpp:2186-2202` and related helpers.
- No explicit HIL runner was found.

## Best Sources To Adapt

- For the missing `driverState()` alias, copy the simple SHT3x/BME280 alias shape: `SHT3x-main\include\SHT3x\SHT3x.h:227-230`.
- For register-dirty wording, compare BME280 `include\BME280\BME280.h:549-585`; adapt the concept, not the exact 8-bit register assumptions.
- For HIL runner design, use SHT3x/BME280 only if INA228 examples expose a real diagnostic CLI.

## Implementation Tasks

1. Add `DriverState driverState() const { return state(); }` next to `state()` in `include\INA228\INA228.h`.
   Preserve existing compatibility aliases; do not remove or rename public APIs to achieve uniform naming.
2. Confirm `getSettings(SettingsSnapshot&) const` remains cache-only and does not read DIAG_ALRT. DIAG_ALRT reads can consume evidence and must stay under explicit active names.
3. Review public raw register helper docs. Ensure `readRegister40()` and accumulator/DIAG_ALRT reads warn about side effects and evidence preservation.
4. Audit every wait/poll path for finite timeout bounds and visible status returns. Normal register/measurement APIs must not hide retries; recovery remains explicit and application-scheduled.
5. If the example CLI has enough commands, add a bounded HIL runner covering the common minimum contract: `version`, `scan`, `probe`, `settings`, `health`, failure-token classification, dry-run/parser test support, DIAG_ALRT, and a conversion read. If no CLI exists, document that HIL is not automated.
6. Add tests for the `driverState()` alias and for DIAG_ALRT/evidence reads not being hidden inside read-only status paths.

## API Changes Required

- Add only the non-breaking `driverState()` alias.

## Simplifications Before Adding Code

- Do not add a second health snapshot type. Extend `SettingsSnapshot` only if it is missing fields needed by current diagnostics.

## Tests To Add Or Update

- Native alias test for `driverState()`.
- Native test proving `getSettings()` is bus-silent with a fake transport.
- Native test for raw DIAG_ALRT read side effects if not already covered.
- HIL parser test only if a real runner is added.

## Commands To Run

- `pio test -e native`
- `pio run -e esp32s3dev`
- Optional if added: `python tools\run_i2c_hil.py --dry-run`

## Constraints And Non-Goals

- Do not collapse INA228-specific `DEVICE_ID_MISMATCH`, accumulation, or DIAG_ALRT errors into generic I2C failures.
- Preserve distinct timeout, address NACK/device-not-found, data NACK, bus, device-ID, accumulation, math, DIAG_ALRT, and dirty-state statuses. Do not use `DEVICE_NOT_FOUND` for timeout/data/bus failures.
- Do not reset or reconfigure a shared bus from the driver.

## Risks And Open Questions

- Open: whether INA228 needs standardized HIL now or whether native/fault-injection tests are enough until a measurement fixture is available.
