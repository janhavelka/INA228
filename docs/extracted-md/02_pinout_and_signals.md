# INA228 pinout and signals

Source: INA228 datasheet, p. 3.

## DGS 10-pin VSSOP pins

| Pin | Name | Type | Driver relevance |
| --- | --- | --- | --- |
| 1 | A1 | Digital input | I2C address strap. Connect to GND, VS, SDA, or SCL. |
| 2 | A0 | Digital input | I2C address strap. Connect to GND, VS, SDA, or SCL. |
| 3 | ALERT | Digital output | Open-drain alert output, active low by default. |
| 4 | SDA | Digital I/O | Open-drain I2C/SMBus data. |
| 5 | SCL | Digital input | I2C/SMBus clock. |
| 6 | VS | Power | Device supply, 2.7 V to 5.5 V recommended. |
| 7 | GND | Ground | Device ground. |
| 8 | VBUS | Analog input | Bus-voltage input. |
| 9 | IN- | Analog input | Negative shunt input. High-side: load side of shunt. Low-side: ground side of shunt. |
| 10 | IN+ | Analog input | Positive shunt input. High-side: supply side of shunt. Low-side: load side of shunt. |

## Address straps

The 7-bit I2C address is selected by `A1` and `A0`.

Source: INA228 datasheet, p. 19.

| A1 | A0 | 7-bit address |
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

## Signal notes

- `ALERT`, `SDA`, and bus-level pullups are open-drain style signals; push-pull output behavior is not documented for these pins.
- Measurement polarity follows `IN+ - IN-`; current calculations inherit this sign.
- `VBUS` is measured relative to ground and is always reported as a nonnegative bus result.
