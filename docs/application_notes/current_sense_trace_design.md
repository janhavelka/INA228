# Using a PCB Copper Trace as a Current-Sense Shunt Resistor

**Source:** current_sense_trace_design.pdf | **TI Document #:** SBOA533 | **Pages:** 20

## Key Takeaways
- PCB copper traces can replace SMT shunt resistors for high-current sensing, but with significant accuracy trade-offs
- PCB manufacturing tolerances cause trace thickness to vary up to ~1.8× nominal (1 oz ordered → 1.82 oz measured), yielding 40–55% resistance errors for narrow traces
- Copper's temperature coefficient of resistance (TCR) makes trace resistance highly sensitive to both ambient temperature and self-heating
- A two-point calibration can reduce errors to ~2–5% for mid-range currents on the same board, but accuracy degrades across boards and revisions
- Only very wide traces (≥1750 mil) approach acceptable uncalibrated accuracy (~1% error at room temperature)

## Summary
This application note experimentally evaluates using PCB copper traces as current-sense shunt resistors across multiple trace widths (8, 100, 200, 1750 mil), lengths (1–3 inches), and temperatures (0–85°C). Traces were measured using INA190 current-sense amplifiers with TMP235 temperature sensors. The key finding is that PCB manufacturing processes deposit significantly more copper than the ordered 1 oz specification on narrow traces — up to 1.82 oz/ft² — because isolated narrow traces receive disproportionate copper plating. This produces large negative resistance errors (traces are thicker, thus lower resistance than calculated).

A second board revision tested mitigation strategies: extended ground planes around traces (reduced error from ~45% to ~20%), and two-point calibration (reduced error to ~1–5% within same board revision). However, calibration accuracy degrades significantly across different board revisions and manufacturers. The conclusion is that copper trace shunts are only suitable for cost-optimized, low-accuracy applications, and require per-board calibration.

## Technical Details

### Trace Resistance Errors (Uncalibrated, Room Temperature)
| Trace Width | Avg. Error (vs. 1oz theoretical) | Notes |
|-------------|----------------------------------|-------|
| 8 mil | -52.93% | Measured 1.82 oz actual thickness |
| 100 mil (bottom) | -45.34% | Up to -54.78% on board 2 |
| 100 mil (center) | -38.98% | Center vs bottom tap: ~6% difference |
| 200 mil (bottom) | -31.85% | |
| 1750 mil (bottom) | -1.07% | Best performer — large enough to reduce plating effect |
| 1750 mil (center) | 0.00% | |

### SEM-Measured Trace Thicknesses
| Nominal Width | Measured Thickness (µm) | Measured Thickness (oz/ft²) | Expected: 34.8 µm (1 oz) |
|---------------|------------------------|----------------------------|--------------------------|
| 100 mil | 63.5 | 1.82 | 82% thicker |
| 200 mil | 62.1 | 1.78 | 78% thicker |
| 1750 mil | 41.7 | 1.20 | 20% thicker |

### Temperature Effects (Controlled Environment, 3-inch traces)
| Trace | 0°C Error | 25°C Error | 55°C Error | 85°C Error |
|-------|-----------|------------|------------|------------|
| 100 mil bottom | -49.51% | -44.19% | -37.75% | -31.31% |
| 200 mil bottom | -39.70% | -33.17% | -25.74% | -20.07% |
| 1750 mil bottom | -13.72% | -4.75% | +6.39% | +16.92% |

Resistance increases with temperature; over-thick narrow traces approach nominal at higher temperatures.

### Two-Point Calibration Results (100 mil trace)
- **Same board, same revision:** 1–5% error for currents >0.5A
- **Same revision, different board:** similar accuracy
- **Different revision:** errors up to 5–80%, calibration breaks down
- **Cross-width (100 mil cal → 200 mil trace):** errors 14–50%, not viable
- Very low currents (<0.1A) always show large errors due to amplifier offset

### Key Design Rules
- Copper TCR ≈ +0.393%/°C — must account for self-heating and ambient
- IPC standards specify **minimum** trace thickness only, not maximum
- Narrow/isolated traces receive more plating copper → lower resistance
- Trace resistance becomes approximately linear with current after initial nonlinear region (offset-dominated)
- Resistance stabilization after current change can take >5 minutes

## Relevance to INA228 Implementation
This app note does not use the INA228 directly (uses INA190 analog amplifier), but the findings are critical for INA228 PCB design. If using a copper trace instead of a discrete shunt resistor with the INA228, the same manufacturing tolerance and TCR issues apply. The INA228's 20-bit resolution and 1 µV offset would better resolve small voltage drops across trace shunts, but the fundamental limitation is the unpredictable trace resistance, not the monitor's precision. **For INA228 designs, use a precision SMT shunt resistor unless cost constraints are severe and ±5% accuracy (with per-board calibration) is acceptable.**
