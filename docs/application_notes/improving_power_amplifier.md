# Improving Power Amplifier Efficiency With Current Monitors

**Source:** improving_power_amplifier.pdf | **TI Document #:** SBOA369B | **Pages:** 3

## Key Takeaways
- Three PA bias control strategies with increasing precision: factory calibration, periodic calibration, real-time sensing
- INA226/INA228 recommended for periodic calibration — integrated voltage + current monitoring in one package
- Real-time sensing provides tightest control but requires CSM on every PA stage (multichannel devices like INA2290 save board space)
- Fixed-bias PA control neglects supply variation, device aging, and transconductance drift — feedback is essential for efficiency
- EZShunt™ devices (INA700, INA780A) integrate shunt resistor into leadframe, reducing BOM and board area

## Summary
This application brief addresses current-shunt monitor (CSM) implementations for power amplifier stages in wireless infrastructure (active antenna systems, remote radio units). PA efficiency depends on controlling transistor bias points, which shift due to supply variation, aging, and temperature. Three feedback approaches are compared: factory calibration (look-up table from one-time measurement), periodic calibration (re-measure during downtime), and real-time continuous sensing.

Each approach trades off BOM count, control precision, and shunt power loss. The INA228 is specifically mentioned as suitable for periodic calibration due to its integrated current + voltage monitoring capability.

## Technical Details

### PA Bias Control Approaches
| Approach | BOM Count | Control | Bus Monitor | OC Protection | Shunt Loss |
|----------|-----------|---------|-------------|---------------|------------|
| Factory Calibration | Low | Loose | No | No | Never (shunt removable) |
| Periodic Calibration | Medium | Tighter | Yes | No | Always |
| Real-Time Sensing | High | Tightest | Yes | Yes | Always |

### Factory Calibration
- CSM measures PA under controlled conditions; results stored in look-up table
- DAC controls bias from table during operation
- CSM can be off-board or shorted post-calibration (zero shunt loss)
- Recommended devices: INA290, INA186 (high common-mode, high-side)
- Add temperature sensors for drift compensation

### Periodic Calibration
- System powers off periodically to recalibrate and update look-up table
- Accounts for supply variation and device aging
- Recommended devices: **INA226, INA228** (integrated V + I monitoring)
- EZShunt™ alternatives: INA700 (2 mΩ), INA780A (400 µΩ) — integrated shunt resistor
- Minimize shunt loss: use low-offset parts (INA190, INA290) to permit smaller shunt values
- Hall-sensor alternative: TMCS1123 (0.67 mΩ leadframe, isolated)

### Real-Time Sensing
- Continuous monitoring of all PA stages
- Multichannel devices: INA2290 (dual), INA4181 (quad) — save board space
- Integrated comparator devices: INA381, INA310A — on-chip overcurrent detection

## Relevance to INA228 Implementation
The INA228 is explicitly recommended for **periodic calibration** of PA stages. Its integrated bus voltage measurement, current sensing, and power calculation reduce the number of discrete components needed. In an INA228-based implementation:
- Use the power register to directly read PA power consumption
- Use the bus voltage register to monitor supply rail
- The 85V common-mode rating is an IC input capability; verify the actual PA
  rail voltage, transients, isolation, grounding, and protection design before
  applying INA228 hardware
- Energy accumulation can track cumulative PA energy consumption over calibration intervals
- This is a **system-level application note** — no INA228-specific register settings or firmware patterns are provided
