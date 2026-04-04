# Simplifying High-Voltage Sensing with Hall-Effect Current Sensors

**Source:** simplifying_high_voltage_sensing.pdf | **TI Document #:** SSZTCY8 | **Pages:** 4

## Key Takeaways
- Hall-effect sensors (TMCS1123) are simpler to design in than shunt-based solutions: no precision resistors, no high/low-side power supplies needed
- Shunt-based sensors (INA228 family) remain more accurate (<1% drift over lifetime) than Hall-effect sensors
- TMCS1123 achieves ±1.75% total sensitivity error over lifetime and temperature — a significant improvement for Hall-effect technology
- Hall-effect sensors have faster propagation delay; shunt-based devices have slower response due to architecture
- Target applications: EV chargers, solar inverters, traction inverters — high-voltage systems where design simplicity matters

## Summary
This technical article compares shunt-based and Hall-effect current sensing technologies for high-voltage applications like EV charging and solar inverters. Shunt-based solutions (amplifiers + precision ADCs + precision resistors) provide the best accuracy (<1% drift) but add design complexity (high/low-side power supplies, careful shunt selection, power dissipation management). Hall-effect in-package sensors eliminate the precision resistor entirely, using the IC leadframe as the conductor, and require only a single low-side power supply.

Historically, Hall-effect sensors suffered from high drift over temperature and lifetime, making them unsuitable for precision high-voltage systems. The TMCS1123 addresses this with innovations that reduce lifetime sensitivity drift to ±0.5% and total maximum sensitivity error to ±1.75% over lifetime and temperature. It supports 75 ARMS at 25°C and includes features like overcurrent detection, precision voltage reference, sensor alert, and differential Hall-effect sensing for magnetic interference rejection.

## Technical Details

### Shunt-Based vs. Hall-Effect Comparison
| Parameter | Shunt-Based (INA228 family) | Hall-Effect (TMCS1123) |
|-----------|---------------------------|----------------------|
| Accuracy | <1% drift over lifetime | ±1.75% total error (lifetime + temp) |
| Design complexity | Higher (precision R, dual supplies) | Lower (no external R, single supply) |
| Propagation delay | Slower | Faster |
| Power dissipation | Shunt R dissipates I²R | Minimal (0.67 mΩ leadframe) |
| Cost | Higher (precision components) | Lower (fewer components) |
| Additional components | Shunt R, bypass caps, pull-ups | Bypass caps only |

### TMCS1123 Key Specifications
| Parameter | Value |
|-----------|-------|
| Lifetime sensitivity drift | ±0.5% |
| Total sensitivity error (lifetime + temp) | ±1.75% |
| Max continuous current @ 25°C | 75 ARMS |
| Leadframe resistance | 0.67 mΩ |
| Sensing method | Differential Hall-effect |
| Features | Overcurrent detection, precision VREF, alert |

### Design Selection Criteria
1. **Accuracy requirements** — determines feasible technology
2. **Power ratings** — system voltage/current must be within device limits
3. **Bandwidth/speed** — critical for controlling switching systems (DC/DC, inverters)
4. **Design complexity** — Hall-effect is simpler for high-voltage levels

## Relevance to INA228 Implementation
This note is **indirectly relevant** — it positions the INA228 family as the high-accuracy alternative to Hall-effect sensors. For INA228 implementations:

- The INA228's shunt-based approach provides superior accuracy (<1% drift) compared to the ±1.75% of the best Hall-effect devices
- If your system already uses an INA228, this note confirms it's the right choice when accuracy is the priority
- For systems where the INA228's accuracy is overkill and design simplicity/cost is prioritized, the TMCS1123 is the Hall-effect alternative — useful context if evaluating sensor trade-offs
- The INA228's 85V common-mode range covers many of the high-voltage applications discussed (EV chargers, solar inverters)
- The INA228 requires more careful PCB design (precision shunt, high/low-side power) compared to a drop-in Hall-effect sensor — a trade-off to be aware of at the system level
