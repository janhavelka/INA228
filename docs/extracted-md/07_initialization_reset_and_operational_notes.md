# INA228 initialization, reset, and operational notes

## Reset behavior

Power-on reset loads all registers with default values. A software reset is initiated by setting `CONFIG.RST`; the bit self-clears after reset. `CONFIG.RSTACC` clears only the `ENERGY` and `CHARGE` accumulators.

Source: INA228 datasheet, pp. 18, 21.

## Practical initialization sequence

1. Probe the selected 7-bit address.
2. Optionally read `MANUFACTURER_ID` (`0x5449`) and `DEVICE_ID` (`0x2281`).
3. Program `CONFIG` for shunt range, conversion delay, and optional shunt temperature compensation.
4. Choose `CURRENT_LSB` from maximum expected current and desired resolution.
5. Write `SHUNT_CAL`; current, power, energy, and charge results depend on this value.
6. Program alert thresholds and `DIAG_ALRT` if ALERT support is used.
7. Program `ADC_CONFIG` last for conversion mode, averaging, and conversion times.
8. In triggered modes, wait for or poll `CNVRF` before reading results.

Source: INA228 datasheet, pp. 21-31, 35-37.

## Calibration and scaling

The datasheet defines:

- `SHUNT_CAL = 13107.2 * 10^6 * CURRENT_LSB * RSHUNT` for `ADCRANGE=0`.
- Multiply `SHUNT_CAL` by 4 when `ADCRANGE=1`.
- `Current [A] = CURRENT_LSB * CURRENT`.
- `Power [W] = 3.2 * CURRENT_LSB * POWER`.
- `Energy [J] = 16 * 3.2 * CURRENT_LSB * ENERGY`.
- `Charge [C] = CURRENT_LSB * CHARGE`.

Source: INA228 datasheet, pp. 30-31.

## Operational cautions for driver code

- If `SHUNT_CAL` is zero, calculated current through `CURRENT` is zero and dependent calculated quantities are not useful.
- Use signed conversion for `VSHUNT`, `CURRENT`, and `CHARGE`.
- `ENERGY` and `CHARGE` roll over on overflow; overflow flags are available in `DIAG_ALRT`.
- Avoid implicit floating-point requirements in low-level register APIs; provide raw reads plus optional scaling helpers.
