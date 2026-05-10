# INA228 variants and source caveats

## Variant scope

The checked-in datasheet source is for INA228, 85-V, 20-bit, ultra-precise power/energy/charge monitor with I2C interface, Rev. A. The compact notes assume the DGS 10-pin VSSOP package and the register map from that datasheet.

Source: INA228 datasheet, pp. 1, 3, 21.

## Datasheet facts used by this repo

| Topic | Datasheet fact |
| --- | --- |
| Register byte order | Big-endian, MSB first. |
| Register widths | Per map: 16, 24, and 40 bits. |
| Supported addresses | `0x40` through `0x4F` selected by `A1`/`A0`. |
| Identity checks | `MANUFACTURER_ID=0x5449`, `DEVICE_ID=0x2281`. |
| Result representation | 20-bit values are stored left-aligned in 24-bit registers where documented. |

## Not documented in PDFs

| Missing or ambiguous fact | Source status |
| --- | --- |
| Alternate INA228 package pinouts | The checked-in INA228 datasheet extract documents the DGS 10-pin VSSOP pin table only. |
| A separate fixed device ID for other INA22x family members | The checked-in INA228 datasheet gives only INA228 `DEVICE_ID=0x2281`. |
| PEC enable/disable register | The INA228 timing figures state that PEC is not supported; no PEC control bit is documented. |
| Clock-stretching timing | The INA228 timing figures state that the device does not perform clock stretching; no stretch timing is documented. |
