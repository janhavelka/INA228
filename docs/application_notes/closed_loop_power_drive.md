# Closed Loop Constant Power Drive to Simplify Heater Element Control and Extend Battery Life

**Source:** closed_loop_power_drive.pdf | **TI Document #:** SLVAFK1 | **Pages:** 10

## Key Takeaways
- Heater temperature is approximately linear with applied power, enabling control without a temperature sensor
- A closed-loop constant power topology using INA234 + I2C-controlled DC/DC (TPS62868) maintains power within ±3.0% of setpoint
- DC/DC converter draws average current from battery (vs. PWM pulses), extending battery life and lifetime
- The approach eliminates the need for thermocouple/thermistor mounting, simplifying mechanical design
- ±3.0% power variation corresponds to ~±0.15°C (at 1W) to ~±0.75°C (at 9W) temperature tolerance

## Summary
Traditional heater control uses a temperature sensor in a feedback loop with PWM drive. This is mechanically complex (sensor mounting) and electrically noisy (PWM current pulses reduce battery life). Since heater temperature is nearly linear with applied power (unlike voltage or current alone, due to resistance changes with temperature), measuring electrical power can replace temperature measurement entirely.

This reference design implements a closed-loop constant power drive using an INA234 power monitor, TPS62868 I2C-controlled buck converter, and MSPM0L1306 MCU. The MCU reads power from the INA234, computes a voltage correction, and adjusts the DC/DC output to maintain constant power delivery to a resistive heater element. The system was validated from 1W–9W with a 1.5Ω nominal load across -18°C to +23°C ambient.

## Technical Details

### Hardware Architecture
| Component | Role | Key Specs |
|-----------|------|-----------|
| INA234 | Power monitor (V, I, P via I2C) | 28V, 12-bit, I2C output |
| TPS62868 | I2C-controlled buck converter | 2.4–5.5V input, 4A output |
| MSPM0L1306 | Control MCU | Low-cost, I2C master |
| R_sense | Current sense resistor | 10 mΩ, ±1%, 1W |

### Design Parameters
- Heater: 1Ω nominal, ±20% variation across temperature and batch
- Input voltage: 3.3V to 5.0V
- Power range: 4W to 9W tested
- Operating voltage to heater: 1.79V to 3.29V depending on resistance and power level

### Control Algorithm
1. Read power from INA234 via I2C (`INA234_getPOWER_W()`)
2. Calculate power error = target − measured
3. Compute voltage step = error × gain factor (gain = 2)
4. Clamp voltage step to ±5 range
5. If power too high → decrease output voltage; if too low → increase
6. Write new voltage to TPS62868 output register via I2C

### Measured Performance
| Power (W) | Avg Measured (W) | Neg. Variation | Pos. Variation |
|-----------|-----------------|----------------|----------------|
| 1.0 | 1.0008 | -2.90% | +2.90% |
| 5.0 | 5.0108 | -2.72% | +2.90% |
| 9.0 | 8.9982 | -2.76% | +2.79% |

- Temperature range test (4W, -18°C to +23°C): power variation +1.5% / -2.1%
- Load resistance varied from 1.33Ω (-18°C) to 1.75Ω (+23°C); algorithm compensated successfully

## Relevance to INA228 Implementation
This app note uses the INA234 (28V, 12-bit), not the INA228 directly. Treat the
constant-power control-loop pattern as conceptual guidance for INA228 designs:
read power, compute a correction, then actuate. INA228-specific use still needs
control-loop stability analysis, timing review, calibration, shunt thermal
design, and hardware validation. Do not infer tighter power-control performance
or high-voltage system suitability from the INA234 validation results.
