# INA228 Documentation

This directory is intentionally small. The root `README.md` remains the main
user-facing guide; public API details come from Doxygen comments in
`include/INA228/`. The files here cover integration, vendor reference material,
and validation/release evidence.

## Integration

- [ESP-IDF integration](integration/esp-idf.md) - native ESP-IDF component,
  example, build commands, and adapter boundary.

## Reference

- [INA228 device reference](reference/ina228-device-reference.md) - compact
  register, scaling, timing, alert, and reset notes used by the driver.
- [Application notes](reference/application-notes.md) - concise relevance notes
  for retained TI application-note PDFs.
- `reference/vendor/` - vendor PDFs retained as source material.

## Validation

- [Validation status](validation/validation-status.md) - current evidence level
  and claim policy.
- [Hardware evidence summary](validation/hardware-evidence.md) - compact HIL
  evidence retained from cleanup and why it is partial evidence only.
- [Hardware validation procedure](validation/hardware-validation-procedure.md)
  - repeatable hardware/HIL procedure and required evidence.
- [Release checklist](validation/release-checklist.md) - version, CI, package,
  documentation, and tag gates.

Generated Doxygen output belongs in `docs/doxygen/` and should not be committed.
