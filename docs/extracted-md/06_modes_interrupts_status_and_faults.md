# INA228 modes, interrupts, status, and faults

## Conversion modes

`ADC_CONFIG.MODE` selects shutdown, triggered, or continuous conversions. Modes can convert bus voltage, shunt voltage, die temperature, or combinations of those inputs. The default reset value `0xFB68` configures continuous conversion of bus voltage, shunt voltage, and temperature.

Source: INA228 datasheet, pp. 14, 18, 22-23, 35.

## Shutdown

Shutdown mode reduces quiescent current to less than the 5 uA maximum while preserving register access over I2C. Enter shutdown through `ADC_CONFIG.MODE`.

Source: INA228 datasheet, pp. 6, 18.

## ALERT behavior

`ALERT` is open-drain and active low by default. `DIAG_ALRT` controls whether alerts are latched or transparent, polarity, conversion-ready routing, and whether threshold comparisons use averaged values.

Source: INA228 datasheet, pp. 16-17, 26-27.

## Diagnostic and fault sources

| Source | Register/flag area | Notes |
| --- | --- | --- |
| Shunt overvoltage | `SOVL`, `DIAG_ALRT` | Compares shunt result against programmed threshold. |
| Shunt undervoltage | `SUVL`, `DIAG_ALRT` | Signed threshold path; preserve sign. |
| Bus overvoltage | `BOVL`, `DIAG_ALRT` | Uses bus-voltage scale. |
| Bus undervoltage | `BUVL`, `DIAG_ALRT` | Uses bus-voltage scale. |
| Temperature over-limit | `TEMP_LIMIT`, `DIAG_ALRT` | Uses die-temperature result. |
| Power over-limit | `PWR_LIMIT`, `DIAG_ALRT` | Compares against `POWER`. |
| Energy overflow | `DIAG_ALRT.ENERGYOF` | Clears when `ENERGY` is read. |
| Charge overflow | `DIAG_ALRT.CHARGEOF` | Clears when `CHARGE` is read. |
| Conversion ready | `DIAG_ALRT.CNVRF` | Useful for triggered mode polling. |

Source: INA228 datasheet, pp. 16-17, 26-29.

## Clear behavior

- Latched alert mode holds the alert/flag until `DIAG_ALRT` is read.
- `CNVRF` clears when `DIAG_ALRT` is read or when a new triggered conversion starts.
- Energy and charge overflow flags clear when their corresponding accumulator register is read.
