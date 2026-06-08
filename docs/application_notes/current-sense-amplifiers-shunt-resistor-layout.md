# Shunt Resistor Layout Considerations

**Source:** current-sense-amplifiers-shunt-resistor-layout.pdf

**Document type:** TI Precision Labs current-sense amplifier layout guidance

## Key Points

- Place the shunt resistor close to the current monitor inputs. Long sense
  traces add noise pickup, parasitic resistance, and parasitic capacitance.
- Use Kelvin connections from the shunt sense pads to the monitor inputs.
  Load current should flow through the shunt body and load-current copper, not
  through the sense traces.
- Keep the IN+ and IN- sense traces short, balanced, and routed as a pair where
  practical. Mismatched trace resistance can create small offsets.
- Follow the shunt manufacturer footprint, pad, and placement guidance,
  especially for four-terminal or metal-element shunts.

## Relevance To INA228

The INA228 measures microvolt-level shunt voltage, so shunt layout directly
affects accuracy. Software calibration cannot fix poor Kelvin routing, load
current flowing through sense traces, undersized shunts, thermal gradients, or
unsafe high-energy PCB layout.
