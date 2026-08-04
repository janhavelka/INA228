# INA228 Documentation

The root `README.md` remains the main user-facing guide; public API details come
from Doxygen comments in `include/INA228/`. This directory contains integration
guidance, vendor reference material, and dated validation/release evidence.

## Integration

- [ESP-IDF integration](integration/esp-idf.md) - native ESP-IDF component,
  example, build commands, and adapter boundary.

## Reference

- [CLI and HIL reference](CLI.md) - shared Arduino/ESP-IDF commands, parsing
  rules, destructive operations, framing, verdicts, and provenance gates.
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
- `validation/hardware/` - generated reports and serial transcripts for the
  exact dated fixtures described by each run.

Generated Doxygen output belongs in `docs/doxygen/` and should not be committed.
