# Digital Interfaces for Current Sensing Devices

**Source:** digital_interfaces.pdf | **TI Document #:** SBOA203A | **Pages:** 3

## Key Takeaways
- I2C, SMBus, and PMBus can coexist on the same physical bus (all share 0.4V logic-low threshold)
- The INA228 supports I2C high-speed mode (up to 2.94 MHz) and SMBus features (Alert pin, error resolution)
- SPI alternative (INA229) offers the same sensing specs as INA228 but with 10 MHz clock rate
- PMBus standardizes command/register addresses, enabling one driver for multiple devices
- SMBus adds bus timeout, CRC error checking, and alert capability over base I2C

## Summary
This application brief compares the four digital interfaces used in TI current sensing devices: I2C, SMBus, PMBus, and SPI. I2C is the base protocol; SMBus extends it with bus timeouts, alert notification, CRC checking, and standardized transaction protocols. PMBus further adds standardized command registers for power systems. All three share compatible electrical levels and can coexist on one bus. SPI is a separate 4-wire interface offering much higher clock speeds (10 MHz for INA229) without pull-up resistors, but requires dedicated chip-select lines.

The document catalogs TI's digital current-sense device portfolio, positioning the INA228 as the highest-accuracy I2C/SMBus option (85V, 20-bit ADC, 0.05% gain error, 1 µV offset) with power/energy/charge/temperature monitoring.

## Technical Details

### Interface Comparison
| Parameter | I2C | SMBus | PMBus | SPI |
|-----------|-----|-------|-------|-----|
| Wires | 2 (SDA, SCL) | 3 (+SMBALERT) | 3–4 (+CONTROL opt.) | 4 (MOSI, MISO, SCK, CS) |
| Max Speed | 5 MHz | 400 kHz / 1 MHz | Same as SMBus | 10 MHz (INA229) |
| Min Speed | None | 100 kHz | 100 kHz | None |
| Pull-ups Required | Yes | Yes | Yes | No |
| Bus Timeout | No | Yes | Yes | N/A |
| Alert Capability | No | Yes | Yes | N/A |
| CRC Error Checking | No | Optional | Optional | N/A |
| Standardized Commands | No | No | Yes | N/A |
| Group Protocol | No | Optional | Required | N/A |

### INA228 vs INA229 (I2C vs SPI)
| Parameter | INA228 (I2C/SMBus) | INA229 (SPI) |
|-----------|-------------------|--------------|
| ADC Resolution | 20-bit delta-sigma | 20-bit delta-sigma |
| Common-mode Voltage | 85V | 85V |
| Gain Error | 0.05% | 0.05% |
| Max Offset | 1 µV | 1 µV |
| Max Data Clock | 2.94 MHz | 10 MHz |
| Package | VSSOP-10 | VSSOP-10 |
| Monitors | V, I, P, Energy, Charge, Temp | V, I, P, Energy, Charge, Temp |

### Key I2C/SMBus Device Selection
| Device | Resolution | V_CM Max | Key Feature |
|--------|-----------|----------|-------------|
| INA228 | 20-bit | 85V | Highest accuracy, energy/charge/temp |
| INA238 | 16-bit | 85V | Lower cost version of INA228 |
| INA226 | 16-bit | 36V | High accuracy, current/power |
| INA234 | 12-bit | 28V | Small DSBGA-8, low cost |
| INA3221 | 12-bit | 26V | 3-channel monitor |
| INA260 | 16-bit | 36V | Internal 2mΩ shunt, up to 15A |

## Relevance to INA228 Implementation
**Directly relevant.** The INA228 uses I2C/SMBus interface. Key implementation notes:
- The device supports I2C high-speed mode up to **2.94 MHz**, but this library
  does not enter high-speed mode or own the bus; the application-supplied
  transport owns bus speed, pins, pull-ups, timeout, locking, and recovery
- Has SMBus-compatible **Alert pin** for fault notification (overcurrent, overvoltage, overpower)
- Can coexist with SMBus/PMBus devices on the same bus
- For higher data rates or simpler bus topology, consider INA229 (SPI, same sensing performance, 10 MHz)
- SMBus timeout behavior is a bus-manager policy, not a core-driver feature;
  configure it in the application-owned transport if the platform supports it
- Pull-up resistor value affects max bus speed via RC time constant with bus capacitance
