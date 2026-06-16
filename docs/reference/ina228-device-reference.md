# INA228 Device Reference

This is a compact implementation reference for the driver. The vendor PDF in
`vendor/INA228_datasheet.pdf` remains the source of truth.

## Device And Bus

- I2C addresses are `0x40` through `0x4F`, selected by A0/A1 strap pins.
- Register transactions use an 8-bit pointer and MSB-first register bytes.
- Defined register widths are 16, 24, and 40 bits.
- The device supports standard/fast I2C and high-speed mode. High-speed mode
  requires the controller-code entry sequence; this library does not hide that
  policy in the core.
- Datasheet timing figures state no PEC and no clock stretching for the normal
  register transactions.

## Identity And Reset

- `MANUFACTURER_ID` is expected to read `0x5449`.
- `DEVICE_ID` is expected to read `0x2281`.
- `DIAG_ALRT.MEMSTAT` reports nonvolatile trim memory health.
- Software reset uses `CONFIG.RST`; accumulator reset uses `CONFIG.RSTACC`.
- `DIAG_ALRT` is status-sensitive and can clear evidence when read.

## Register Summary

| Register | Width | Notes |
| --- | --- | --- |
| `CONFIG` | 16 | Reset bits, conversion delay, temperature compensation, ADCRANGE. |
| `ADC_CONFIG` | 16 | Mode, bus/shunt/temp conversion time, averaging. |
| `SHUNT_CAL` | 16 | Calibration contract for current-derived values. |
| `SHUNT_TEMPCO` | 16 | Shunt temperature coefficient. |
| `VSHUNT` | 24 | Signed 20-bit value in bits 23:4. |
| `VBUS` | 24 | Unsigned 20-bit value in bits 23:4. |
| `DIETEMP` | 16 | Signed die temperature. |
| `CURRENT` | 24 | Signed 20-bit value in bits 23:4; requires SHUNT_CAL. |
| `POWER` | 24 | Unsigned power value; requires SHUNT_CAL. |
| `ENERGY` | 40 | Unsigned accumulator. |
| `CHARGE` | 40 | Signed accumulator. |
| `DIAG_ALRT` | 16 | Alert configuration plus live/clear-on-read diagnostics. |
| `SOVL` / `SUVL` | 16 | Signed shunt-voltage thresholds. |
| `BOVL` / `BUVL` | 16 | Unsigned bus-voltage thresholds. |
| `TEMP_LIMIT` | 16 | Signed temperature threshold. |
| `PWR_LIMIT` | 16 | Unsigned power threshold. |
| `MANUFACTURER_ID` | 16 | Identity check. |
| `DEVICE_ID` | 16 | Identity check. |

## Scaling

- Shunt voltage LSB is `312.5 nV` at `ADCRANGE=0` and `78.125 nV` at
  `ADCRANGE=1`.
- Bus voltage LSB is `195.3125 uV`.
- Die temperature LSB is `7.8125 mdegC`.
- `SHUNT_CAL = 13107.2 * 10^6 * CURRENT_LSB * RSHUNT` for `ADCRANGE=0`.
- Multiply `SHUNT_CAL` by 4 when `ADCRANGE=1`.
- `Current [A] = CURRENT_LSB * CURRENT`.
- `Power [W] = 3.2 * CURRENT_LSB * POWER`.
- `Energy [J] = 16 * 3.2 * CURRENT_LSB * ENERGY`.
- `Charge [C] = CURRENT_LSB * CHARGE`.
- `SOVL`/`SUVL` use `5 uV/LSB` at `ADCRANGE=0` and `1.25 uV/LSB` at
  `ADCRANGE=1`.
- `BOVL`/`BUVL` use `3.125 mV/LSB`.
- `PWR_LIMIT` uses `256 * power LSB`.

## Timing And Modes

- Conversion times are 50, 84, 150, 280, 540, 1052, 2074, and 4120 us.
- Averaging choices are 1, 4, 16, 64, 128, 256, 512, and 1024 samples.
- Shutdown disables ADC conversion.
- Triggered modes perform one conversion sequence and return to shutdown.
- Continuous modes update measurement registers repeatedly.
- In triggered mode, wait for the computed deadline and confirm `CNVRF` before
  treating a sample as fresh.

## Alert And Diagnostic Notes

- `ALERT` and `SDA` are open-drain bus-style signals.
- `DIAG_ALRT.CNVRF` clears when `DIAG_ALRT` is read or when a new triggered
  conversion starts.
- Latched alert flags clear when `DIAG_ALRT` is read.
- Energy and charge overflow evidence can be affected by accumulator reads.
- ALERT is a monitoring aid, not a safety interlock.

## Driver Implications

- Preserve signedness exactly: `VSHUNT`, `CURRENT`, `CHARGE`, and `DIETEMP` are
  signed; `VBUS`, `POWER`, and `ENERGY` are unsigned.
- Treat `SHUNT_CAL`, `ADCRANGE`, shunt resistance, maximum expected current, and
  `CURRENT_LSB` as one calibration contract.
- Avoid read-modify-write on `DIAG_ALRT`; write cached alert configuration bits
  deliberately and read live status only through documented APIs.
- High-voltage capability is an IC input rating. Board design, isolation,
  fusing, shunt dissipation, grounding, and operator safety remain system
  responsibilities.
