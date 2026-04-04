# Integrating the Current Sensing Signal Path

**Source:** integrating_current_sensing.pdf | **TI Document #:** SBOA167B | **Pages:** 3

## Key Takeaways
- The current-sensing signal path has three main blocks: shunt resistor, analog front-end (AFE/CSA), and ADC — each can be optimized
- Dedicated current-sense amplifiers (e.g., INA210) integrate gain resistors internally for better matching and temperature stability vs. external resistor networks
- The INA226 eliminates the AFE entirely by measuring directly across the shunt with ±80 mV full-scale input
- On-chip current/power computation and alert monitoring reduce processor resource requirements
- Shunt resistor selection is a fundamental trade-off: accuracy (larger R) vs. power dissipation (smaller R)

## Summary
This application brief walks through the standard current-sensing signal path — from shunt resistor through amplification and digitization to the controller — and shows progressive integration levels. At the first level, dedicated current-sense amplifiers like the INA210 replace op-amp + discrete gain resistors, providing better gain matching and temperature stability in a smaller package. At the next level, specialized ADCs like the INA226 eliminate the amplifier entirely, measuring the small shunt voltage directly with a reduced full-scale input range optimized for current sensing.

The INA226 additionally integrates bus voltage measurement (via internal MUX), current calculation (from programmed shunt value), power computation, and programmable alert monitoring — consolidating the entire sensing signal path into one device communicating over I2C.

## Technical Details

### Signal Path Integration Levels
```
Level 1: Shunt → Op-Amp + External Resistors → ADC → REF → Controller
Level 2: Shunt → INA210 (integrated gain) → ADC → Controller  
Level 3: Shunt → INA226 (direct shunt ADC + compute) → Controller
```

### Shunt Resistor Selection
| Factor | Large R_shunt | Small R_shunt |
|--------|--------------|---------------|
| Differential voltage | Larger (better SNR) | Smaller |
| Offset error impact | Lower (% of signal) | Higher |
| Power dissipation | Higher (P = I²R) | Lower |
| Amplifier gain needed | Lower | Higher |

- Select R_shunt so that max current × R_shunt ≈ ADC full-scale input (or CSA full-scale)
- Amplifier gain selected to map CSA output to ADC full-scale range

### INA226 Advantages Over Discrete Chain
| Feature | Discrete (AFE + ADC) | INA226 |
|---------|---------------------|--------|
| Full-scale input | 2–5V (typical ADC) | ±80 mV |
| LSB resolution | ~38 µV (16-bit, 2.5V range) | 2.5 µV |
| Resolution advantage | 1× | 15× for shunt signals |
| Offset voltage | Depends on AFE | 10 µV max |
| Common-mode range | Limited by supply | 0–36V (independent of supply) |
| On-chip calculations | None | Current, power, alert |
| Component count | Shunt + CSA + ADC + REF | Shunt + INA226 |

### INA210 Current-Sense Amplifier
- Integrated gain-setting resistors → better matching and temperature drift vs. external
- QFN package reduces board space vs. op-amp + 4 external resistors
- Available in multiple fixed gains to optimize R_shunt ↔ ADC pairing
- Common-mode: 0V to 26V, Supply: 2.7V to 26V

### Device Alternatives
| Device | Optimized For | Trade-off |
|--------|--------------|-----------|
| INA199 | Lower cost CSA | Higher offset and gain error |
| INA301 | Overcurrent detection (1 µs comparator) | Larger MSOP-8 package |
| INA219 | Lower cost digital monitor | Higher offset and gain error |
| INA190 | Higher accuracy CSA | — |

## Relevance to INA228 Implementation
**Directly relevant as architectural guidance.** The INA228 represents the ultimate integration of the signal path described here — "Level 3" taken further with 20-bit resolution, 85V common-mode, and additional energy/charge/temperature registers. Key takeaways for INA228 implementation:
- Shunt resistor selection trade-offs apply identically to the INA228
- The INA228's ±82.mV full-scale differential input means very small R_shunt values work well
- The 20-bit ADC provides even more headroom to use smaller shunt resistors with lower power dissipation
- On-chip current and power registers eliminate most host-side math
- Programmable alert replaces external comparator circuits for overcurrent protection
