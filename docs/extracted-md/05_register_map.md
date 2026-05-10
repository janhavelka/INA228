# INA228 register map

Source: INA228 datasheet, pp. 21-29.

## Register summary

| Address | Name | Bits | Access intent | Reset/default |
| --- | --- | ---: | --- | --- |
| `0x00` | `CONFIG` | 16 | Reset, accumulation reset, ADCRANGE, temperature compensation, conversion delay | `0x0000` |
| `0x01` | `ADC_CONFIG` | 16 | Mode, conversion times, averaging | `0xFB68` |
| `0x02` | `SHUNT_CAL` | 16 | Current/power calibration constant | `0x1000` |
| `0x03` | `SHUNT_TEMPCO` | 16 | External shunt temperature coefficient | `0x0000` |
| `0x04` | `VSHUNT` | 24 | Shunt voltage result | `0x000000` |
| `0x05` | `VBUS` | 24 | Bus voltage result | `0x000000` |
| `0x06` | `DIETEMP` | 16 | Die temperature result | `0x0000` |
| `0x07` | `CURRENT` | 24 | Calculated current result | `0x000000` |
| `0x08` | `POWER` | 24 | Calculated power result | `0x000000` |
| `0x09` | `ENERGY` | 40 | Calculated energy accumulator | `0x0000000000` |
| `0x0A` | `CHARGE` | 40 | Calculated charge accumulator | `0x0000000000` |
| `0x0B` | `DIAG_ALRT` | 16 | Alert configuration and diagnostic flags | `0x0001` |
| `0x0C` | `SOVL` | 16 | Shunt overvoltage threshold | `0x7FFF` |
| `0x0D` | `SUVL` | 16 | Shunt undervoltage threshold | `0x8000` |
| `0x0E` | `BOVL` | 16 | Bus overvoltage threshold | `0x7FFF` |
| `0x0F` | `BUVL` | 16 | Bus undervoltage threshold | `0x0000` |
| `0x10` | `TEMP_LIMIT` | 16 | Temperature over-limit threshold | `0x7FFF` |
| `0x11` | `PWR_LIMIT` | 16 | Power over-limit threshold | `0xFFFF` |
| `0x3E` | `MANUFACTURER_ID` | 16 | Manufacturer ID | `0x5449` |
| `0x3F` | `DEVICE_ID` | 16 | Device ID | `0x2281` |

## Key bit fields

| Register | Fields to model |
| --- | --- |
| `CONFIG` | `RST`, `RSTACC`, `CONVDLY`, `TEMPCOMP`, `ADCRANGE` |
| `ADC_CONFIG` | `MODE`, `VBUSCT`, `VSHCT`, `VTCT`, `AVG` |
| `DIAG_ALRT` | Alert latch/polarity, conversion-ready routing, averaged-alert comparison, overflow flags, conversion-ready flag, threshold flags |

## Documented reserved-bit behavior

| Register | Reserved bits | Datasheet behavior |
| --- | --- | --- |
| `CONFIG` | Bits 3:0 | Read 0. |
| `SHUNT_CAL` | Bit 15 | Read 0. |
| `SHUNT_TEMPCO` | Bits 15:14 | Read 0. |
| `VSHUNT`, `VBUS`, `CURRENT` | Bits 3:0 | Read 0; measurement value is in bits 23:4. |
| `DIAG_ALRT` | Bit 8 | Read 0. |
| `BOVL`, `BUVL` | Bit 15 | Read 0; threshold value is bits 14:0. |
| Unlisted register addresses | Whole location | Reserved locations; the datasheet says the contents are not to be modified. |

Source: INA228 datasheet, pp. 21-29.

## Result scaling reminders

- `VSHUNT`: signed 20-bit value in bits 23:4; use shunt LSB from `ADCRANGE`.
- `VBUS`: 20-bit value in bits 23:4; multiply by 195.3125 uV/LSB.
- `DIETEMP`: signed 16-bit value; multiply by 7.8125 mdegC/LSB.
- `CURRENT`: signed 20-bit value in bits 23:4; multiply by application-selected `CURRENT_LSB`.
- `POWER`: unsigned 24-bit value; watts = `3.2 * CURRENT_LSB * POWER`.
- `ENERGY`: unsigned 40-bit accumulator; joules = `16 * 3.2 * CURRENT_LSB * ENERGY`.
- `CHARGE`: signed 40-bit accumulator; coulombs = `CURRENT_LSB * CHARGE`.
- `SOVL`/`SUVL`: signed 16-bit shunt thresholds, 5 uV/LSB at `ADCRANGE=0`, 1.25 uV/LSB at `ADCRANGE=1`.
- `BOVL`/`BUVL`: unsigned 15-bit bus thresholds, 3.125 mV/LSB.
- `TEMP_LIMIT`: signed 16-bit threshold, 7.8125 mdegC/LSB.
- `PWR_LIMIT`: unsigned 16-bit threshold, 256 x Power LSB.

Source: INA228 datasheet, pp. 24-31.
