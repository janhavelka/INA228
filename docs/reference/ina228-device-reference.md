# INA228 Device Reference

This is a compact implementation reference for the driver. The vendor PDF in
`vendor/INA228_datasheet.pdf` remains the source of truth. Scope is INA228
Rev. A, DGS 10-pin VSSOP. The checked-in source material does not document
alternate INA228 package pinouts or fixed device IDs for other INA22x family
members.

## Device And Bus

- I2C addresses are `0x40` through `0x4F`, selected by A0/A1 strap pins.
- A0 and A1 can each strap to GND, VS, SDA, or SCL. The device samples the
  address pins on every bus communication; when an address pin is tied to SDA,
  the datasheet requires an additional 100 ns hold time on the address MSB.
- Register transactions use an 8-bit pointer and MSB-first register bytes.
- Defined register widths are 16, 24, and 40 bits.
- Writes always include the register pointer byte. Reads use the retained
  pointer; a specific-register read is a pointer write followed by repeated
  START and address+read. Repeated reads can omit the pointer write until the
  pointer changes.
- The device supports standard/fast I2C and high-speed mode. High-speed mode
  requires the controller-code entry sequence; this library does not hide that
  policy in the core.
- Datasheet timing figures state no PEC and no clock stretching for the normal
  register transactions.
- Measurement polarity is `IN+ - IN-`; current sign follows that polarity.
  `VBUS` is ground-referenced and nonnegative. High-side or low-side orientation
  depends on where the system places the shunt.

### DGS 10-Pin VSSOP Signals

| Pin | Name | Driver relevance |
| ---: | --- | --- |
| 1 | A1 | I2C address strap to GND, VS, SDA, or SCL. |
| 2 | A0 | I2C address strap to GND, VS, SDA, or SCL. |
| 3 | ALERT | Open-drain alert output, active low by default. |
| 4 | SDA | Open-drain I2C/SMBus data. |
| 5 | SCL | I2C/SMBus clock input. |
| 6 | VS | Device supply. |
| 7 | GND | Device ground. |
| 8 | VBUS | Ground-referenced bus-voltage input. |
| 9 | IN- | Negative shunt input; high-side load side or low-side ground side. |
| 10 | IN+ | Positive shunt input; high-side supply side or low-side load side. |

| A1 | A0 | Address |
| --- | --- | --- |
| GND | GND | `0x40` |
| GND | VS | `0x41` |
| GND | SDA | `0x42` |
| GND | SCL | `0x43` |
| VS | GND | `0x44` |
| VS | VS | `0x45` |
| VS | SDA | `0x46` |
| VS | SCL | `0x47` |
| SDA | GND | `0x48` |
| SDA | VS | `0x49` |
| SDA | SDA | `0x4A` |
| SDA | SCL | `0x4B` |
| SCL | GND | `0x4C` |
| SCL | VS | `0x4D` |
| SCL | SDA | `0x4E` |
| SCL | SCL | `0x4F` |

## Operating Limits

- Recommended `VS`: 2.7 V to 5.5 V.
- Common-mode input range: -0.3 V to +85 V.
- `VBUS` measurement range: 0 V to 85 V.
- Ambient operating range: -40 degC to +125 degC.
- Shutdown current: 5 uA maximum.
- I2C fast mode: 1 kHz to 400 kHz.
- I2C high-speed mode: 10 kHz to 2.94 MHz after the high-speed
  controller-code entry sequence.
- High-speed entry sends controller code `00001XXXb` at no more than 400 kHz.
  INA228 does not acknowledge that code; repeated START keeps high-speed mode
  active and STOP exits it.

## Identity And Reset

- `MANUFACTURER_ID` is expected to read `0x5449`.
- `DEVICE_ID[15:4]` is DIEID and must equal `0x228`; `DEVICE_ID[3:0]` is
  revision. The commonly documented reset value `0x2281` therefore describes
  DIEID `0x228`, revision 1, rather than one indivisible product constant.
- The driver parses revision separately and applies `Config::supportedRevisionMask`.
- `DIAG_ALRT.MEMSTAT` reports nonvolatile trim memory health.
- Power-on reset loads documented register defaults.
- Software reset uses `CONFIG.RST`; the bit self-clears after reset.
- Accumulator reset uses `CONFIG.RSTACC` and clears only `ENERGY` and `CHARGE`.
- `DIAG_ALRT` is status-sensitive and can clear evidence when read.
- `DIAG_ALRT.MEMSTAT=1` means nonvolatile memory status is healthy.

## Register Summary

| Addr | Register | Width | Reset | Notes |
| --- | --- | ---: | --- | --- |
| `0x00` | `CONFIG` | 16 | `0x0000` | Reset bits, conversion delay, temperature compensation, ADCRANGE. |
| `0x01` | `ADC_CONFIG` | 16 | `0xFB68` | Mode, bus/shunt/temp conversion time, averaging. |
| `0x02` | `SHUNT_CAL` | 16 | `0x1000` | Calibration contract for current-derived values. |
| `0x03` | `SHUNT_TEMPCO` | 16 | `0x0000` | Shunt temperature coefficient. |
| `0x04` | `VSHUNT` | 24 | `0x000000` | Signed 20-bit value in bits 23:4. |
| `0x05` | `VBUS` | 24 | `0x000000` | Unsigned 20-bit value in bits 23:4. |
| `0x06` | `DIETEMP` | 16 | `0x0000` | Signed die temperature. |
| `0x07` | `CURRENT` | 24 | `0x000000` | Signed 20-bit value in bits 23:4; requires SHUNT_CAL. |
| `0x08` | `POWER` | 24 | `0x000000` | Unsigned power value; requires SHUNT_CAL. |
| `0x09` | `ENERGY` | 40 | `0x0000000000` | Unsigned accumulator. |
| `0x0A` | `CHARGE` | 40 | `0x0000000000` | Signed accumulator. |
| `0x0B` | `DIAG_ALRT` | 16 | `0x0001` | Alert configuration plus live/clear-on-read diagnostics. |
| `0x0C` | `SOVL` | 16 | `0x7FFF` | Signed shunt overvoltage threshold. |
| `0x0D` | `SUVL` | 16 | `0x8000` | Signed shunt undervoltage threshold. |
| `0x0E` | `BOVL` | 16 | `0x7FFF` | Unsigned bus overvoltage threshold. |
| `0x0F` | `BUVL` | 16 | `0x0000` | Unsigned bus undervoltage threshold. |
| `0x10` | `TEMP_LIMIT` | 16 | `0x7FFF` | Signed temperature threshold. |
| `0x11` | `PWR_LIMIT` | 16 | `0xFFFF` | Unsigned power threshold. |
| `0x3E` | `MANUFACTURER_ID` | 16 | `0x5449` | Identity check. |
| `0x3F` | `DEVICE_ID` | 16 | `0x2281` | DIEID in bits 15:4, revision in bits 3:0. |

Reserved bits documented as read-zero include `CONFIG[3:0]`, `SHUNT_CAL[15]`,
`SHUNT_TEMPCO[15:14]`, result-register low nibbles for 20-bit values,
`DIAG_ALRT[8]`, and `BOVL`/`BUVL[15]`. Do not write unlisted register
addresses.

## Scaling

- Shunt voltage LSB is `312.5 nV` at `ADCRANGE=0` and `78.125 nV` at
  `ADCRANGE=1`.
- Bus voltage LSB is `195.3125 uV`.
- Die temperature LSB is `7.8125 mdegC`.
- `SHUNT_CAL = 13107.2 * 10^6 * CURRENT_LSB * RSHUNT` for `ADCRANGE=0`.
- Multiply `SHUNT_CAL` by 4 when `ADCRANGE=1`.
- Maximum-current planning divides by the signed 20-bit positive limit
  (`524287`), not the total 20-bit code count. Its derived `SHUNT_CAL` is rounded
  upward so register quantization cannot reduce representable positive current
  below the caller's requested maximum. Explicit-current-LSB planning retains
  nearest-value rounding because its caller selected the LSB directly.
- `Current [A] = CURRENT_LSB * CURRENT`.
- `Power [W] = 3.2 * CURRENT_LSB * POWER`.
- `Energy [J] = 16 * 3.2 * CURRENT_LSB * ENERGY`.
- `Charge [C] = CURRENT_LSB * CHARGE`.
- `SOVL`/`SUVL` use `5 uV/LSB` at `ADCRANGE=0` and `1.25 uV/LSB` at
  `ADCRANGE=1`.
- `BOVL`/`BUVL` use `3.125 mV/LSB`.
- `TEMP_LIMIT` uses `7.8125 mdegC/LSB`.
- `PWR_LIMIT` uses `256 * power LSB`.

## Timing And Modes

- Conversion times are 50, 84, 150, 280, 540, 1052, 2074, and 4120 us.
- Averaging choices are 1, 4, 16, 64, 128, 256, 512, and 1024 samples.
- `CONFIG.CONVDLY` is an 8-bit field in 2 ms steps, for 0 to 510 ms of
  conversion delay.
- The reset `ADC_CONFIG` value configures continuous bus, shunt, and die
  temperature conversion.
- Shutdown disables ADC conversion while preserving I2C register access.
- Triggered modes perform one conversion sequence and return to shutdown.
- Continuous modes update measurement registers repeatedly.
- In triggered mode, wait for the computed deadline and confirm `CNVRF` before
  treating a sample as fresh.
- Timed trigger/reset origins are established after the successful blocking
  register write returns. `Config::nowMs` is sampled then; without that hook,
  the next explicit wrap-safe timestamp anchors the full wait in a bus-silent
  activation. Hooked and explicit timestamps must share one monotonic domain.
- Hookless `isConversionReady()` cannot advance an unresolved trigger; use
  `pollConversionReady(nowMs, ...)` or `tick(nowMs)`.
- The cooperative instantaneous-sample job owns its triggered transition. Its
  active configured base mode must be continuous or shutdown, even though
  initialization itself supports valid triggered `Config::mode` values.

## Alert And Diagnostic Notes

- `ALERT` and `SDA` are open-drain bus-style signals. ALERT is active low by
  default.
- `DIAG_ALRT` configuration bits control latch mode, conversion-ready routing,
  averaged-value alerting, and polarity.
- `DIAG_ALRT.CNVRF` clears when `DIAG_ALRT` is read or when a new triggered
  conversion starts.
- Latched alert flags clear when `DIAG_ALRT` is read.
- `ENERGYOF` clears when `ENERGY` is read; `CHARGEOF` clears when `CHARGE` is
  read. Accumulators roll over on overflow. `MATHOF` is a separate diagnostic
  flag for current-derived calculation overflow.
- Alert sources include shunt OV/UV, bus OV/UV, temperature over-limit, power
  over-limit, energy overflow, charge overflow, math overflow, conversion ready,
  and MEMSTAT.
- SMBus Alert Response address `0001100b` with read can return the responding
  device address when ALERT is asserted; if multiple devices respond,
  arbitration selects one responder and the others keep ALERT asserted until
  cleared or serviced.
- ALERT is a monitoring aid, not a safety interlock.

## Driver Implications

- The production cooperative initialization order is: verify
  manufacturer/DIEID/revision/MEMSTAT, enter deterministic shutdown, program
  `CONFIG`, deterministic writable `DIAG_ALRT`, `SHUNT_TEMPCO`, `SHUNT_CAL`, and
  `ADC_CONFIG`, then read back all critical writable state before declaring
  hardware synchronized.
- Preserve signedness exactly: `VSHUNT`, `CURRENT`, `CHARGE`, and `DIETEMP` are
  signed; `VBUS`, `POWER`, and `ENERGY` are unsigned.
- Treat `SHUNT_CAL`, `ADCRANGE`, shunt resistance, maximum expected current, and
  `CURRENT_LSB` as one calibration contract.
- Prefer fixed-unit calibration input and inspect the planner's selected versus
  effective LSB, quantization, clamping, current-register range, and
  shunt-voltage range. Unsafe plans fail unless explicitly authorized.
- If `SHUNT_CAL` is zero, `CURRENT` reads zero and current-derived `POWER`,
  `ENERGY`, and `CHARGE` are not useful for converted values.
- Avoid read-modify-write on `DIAG_ALRT`; write cached alert configuration bits
  deliberately and read live status only through documented APIs.
- A triggered conversion or a change to range, calibration, conversion
  mode/timing, or temperature compensation creates a new accumulation epoch.
  Energy/charge conversion remains invalid until a verified accumulator-reset
  sequence establishes coherent scale and mode assumptions.
- High-voltage capability is an IC input rating. Board design, isolation,
  fusing, shunt dissipation, grounding, and operator safety remain system
  responsibilities.
