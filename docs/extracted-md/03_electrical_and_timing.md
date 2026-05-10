# INA228 electrical and timing notes

## Operating limits relevant to software

| Parameter | Value | Source |
| --- | --- | --- |
| Recommended `VS` | 2.7 V to 5.5 V | Datasheet, p. 6 |
| Common-mode input range | -0.3 V to +85 V | Datasheet, pp. 1, 5 |
| `VBUS` range | 0 V to 85 V | Datasheet, p. 30 |
| Operating ambient temperature | -40 degC to +125 degC | Datasheet, p. 5 |
| Shutdown current | 5 uA max | Datasheet, p. 6 |
| I2C fast mode | Up to 400 kHz | Datasheet, p. 7 |
| I2C high-speed mode | Up to 2.94 MHz | Datasheet, p. 7 |

## ADC scales

Source: INA228 datasheet, p. 30.

| Measurement | Full scale | LSB |
| --- | --- | --- |
| Shunt, `ADCRANGE=0` | +/-163.84 mV | 312.5 nV/LSB |
| Shunt, `ADCRANGE=1` | +/-40.96 mV | 78.125 nV/LSB |
| Bus voltage | 0 V to 85 V | 195.3125 uV/LSB |
| Die temperature | -40 degC to +125 degC package-limited | 7.8125 mdegC/LSB |

`VSHUNT` and `VBUS` are 24-bit registers carrying 20-bit measurement values in bits 23:4. `VSHUNT` is signed two's-complement; `VBUS` is described as two's-complement but always positive. `DIETEMP` is a signed 16-bit value.

## Conversion timing

Each shunt, bus, and temperature conversion time is selected independently in `ADC_CONFIG`. Supported conversion-time field values are 50 us, 84 us, 150 us, 280 us, 540 us, 1052 us, 2074 us, and 4120 us. Averaging is also selected in `ADC_CONFIG` and ranges from 1x to 1024x.

Source: INA228 datasheet, pp. 6, 14-15, 22-23.

## Driver timing implications

- In triggered modes, poll `CNVRF` in `DIAG_ALRT` or wait long enough for every enabled conversion plus averaging.
- The optional conversion delay in `CONFIG.CONVDLY` can add 0 to 510 measurement-delay units before a conversion cycle.
- Do not use I2C high-speed mode unless the bus layer explicitly implements the high-speed master-code entry sequence.
