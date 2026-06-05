# Integrated, Current Sensing Analog-to-Digital Converter

**Source:** integrated_current_sensing_adc.pdf | **TI Document #:** SBOA179A | **Pages:** 4

## Key Takeaways
- Dedicated current-sensing ADCs (e.g., INA226) replace the entire AFE + ADC + reference signal chain with a single device
- The INA226's ±80 mV full-scale input measures directly across the shunt — no external amplifier needed
- On-chip shunt resistor register enables automatic current, power calculation — offloads the host processor
- Programmable alert register can report overcurrent/overvoltage/overpower
  threshold events; independent protection hardware is still required for
  safety functions
- Common-mode voltage independence (up to 36V for INA226, 85V for INA228) decouples measurement from supply voltage

## Summary
This application brief explains the traditional current-sensing signal chain (shunt resistor → analog front-end amplifier → ADC → processor) and shows how dedicated current-sensing ADCs like the INA226 collapse multiple functional blocks into a single device. Traditional ADCs have full-scale input ranges of 2–5V, wasting resolution when measuring small shunt voltages (millivolts). The INA226's ±80 mV full-scale range with 2.5 µV LSB provides 15× more resolution than a standard 16-bit ADC with 2.5V range.

The device also integrates bus voltage measurement via internal multiplexer, shunt-to-current conversion using a programmable resistor value, power computation, signal averaging, and alert monitoring — all reducing processor burden.

## Technical Details

### Traditional Signal Chain vs. Integrated Solution
```
Traditional:  Shunt → CSA (INA210) → ADC → REF → OSC → Controller
Integrated:   Shunt → INA226 (I2C) → Controller
```

### INA226 Key Specifications (reference device in this app note)
| Parameter | Value |
|-----------|-------|
| ADC Resolution | 16-bit delta-sigma |
| Full-scale shunt input | ±81.92 mV |
| LSB size | 2.5 µV |
| Max input offset | 10 µV |
| Offset drift | 0.1 µV/°C |
| Max gain error | 0.1% |
| Common-mode range | 0V to 36V |
| Supply voltage | 2.7V to 5.5V |
| Interface | I2C |

### On-Chip Features
- **Shunt resistor register**: Programmable with PCB shunt value; enables automatic V_shunt → I_load conversion
- **Internal MUX**: Switches ADC between differential shunt measurement and single-ended bus voltage
- **Power calculation**: Combines measured current and voltage to compute and store power
- **Programmable alert**: Compare each conversion against user-defined limits
  and report threshold events; not a certified protection function
- **Signal averaging**: Programmable averaging to reduce noise

### Shunt Resistor Selection Trade-off
- **Larger R_shunt** → larger differential signal → lower offset error impact → **higher power dissipation** (P = I²R)
- **Smaller R_shunt** → lower power dissipation → offset error becomes larger fraction of signal → **reduced accuracy**
- Dedicated CSA-ADCs resolve this by having very small full-scale input ranges (80 mV), allowing small R_shunt without sacrificing resolution

### Device Alternatives
| Device | Optimized For | Trade-off |
|--------|--------------|-----------|
| INA234 | Lower cost | Higher offset & gain error, lower V_CM |
| INA260 | Lower system gain error (integrated 2mΩ shunt) | Larger TSSOP-16 package |
| AMC1305 | Isolated measurement (1.5kV DC, 7kV peak) | Higher cost and errors |
| INA210 | Lower cost analog amplifier | Higher offset & gain error |

## Relevance to INA228 Implementation
**Highly relevant — the INA228 is the next-generation evolution of the INA226 described here.** The INA228 improves on every specification:
- 20-bit ADC (vs. 16-bit) → finer resolution
- 85V common-mode IC input capability (vs. 36V) → wider measurement range when
  the system-level isolation, transient, and safety design is valid
- 0.05% gain error (vs. 0.1%) → better accuracy
- 1 µV max offset (vs. 10 µV) → much better low-current accuracy
- Adds energy accumulation, charge accumulation, and die temperature registers

The signal chain simplification, shunt resistor programming, and on-chip calculation concepts described here apply directly to the INA228. The INA228 provides even greater processor offloading with its energy and charge accumulators.
