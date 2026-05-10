# INA228 chip overview

The INA228 is an I2C/SMBus current, voltage, power, energy, charge, and die-temperature monitor for high-side or low-side current sensing. It measures differential shunt voltage between `IN+` and `IN-`, bus voltage at `VBUS`, and internal die temperature through a multiplexed ADC, then reports both raw and calculated values through registers.

Source: INA228 datasheet, pp. 1, 12, 30.

## Driver-facing capabilities

| Capability | INA228 facts to model | Source |
| --- | --- | --- |
| 85 V common-mode / bus voltage monitoring | `VBUS` reports 0 V to 85 V with 195.3125 uV/LSB. | Datasheet, pp. 1, 4, 30 |
| 20-bit measurement path for shunt and bus results | Sign extension and right-shift handling are required for 24-bit result registers with data in bits 23:4. | Datasheet, pp. 24, 30 |
| Programmable shunt full-scale range | `ADCRANGE=0` is +/-163.84 mV, `ADCRANGE=1` is +/-40.96 mV. | Datasheet, p. 30 |
| Calculated current/power/energy/charge | Driver must program `SHUNT_CAL` before `CURRENT`, `POWER`, `ENERGY`, and `CHARGE` are meaningful. | Datasheet, pp. 24, 30-31 |
| Programmable conversion timing and averaging | `ADC_CONFIG` controls mode, per-input conversion time, and averaging. | Datasheet, pp. 14-15, 22-23 |
| Multi-source ALERT output | `DIAG_ALRT` configures alert behavior and reports diagnostic flags. | Datasheet, pp. 16-18, 26-27 |
| 16 I2C addresses | Two strap pins, `A1` and `A0`, each connect to GND, VS, SDA, or SCL. | Datasheet, p. 19 |

## Implementation facts tied to the datasheet

- INA228 register transactions use an 8-bit pointer and MSB-first register bytes.
- Defined register widths are 16, 24, and 40 bits; width comes from the register address.
- `SHUNT_CAL` encodes the selected shunt resistance and `CURRENT_LSB`; calculated `CURRENT`, `POWER`, `ENERGY`, and `CHARGE` depend on it.
- Supplemental application notes in this repo contain no INA228 register addresses, reset values, or I2C command sequences.
