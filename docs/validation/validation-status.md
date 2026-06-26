# Validation Status

This file separates implemented behavior from evidence. Do not infer a stronger
claim from a weaker one.

| Evidence level | Current status |
| --- | --- |
| Implemented | Core library, Arduino CLI example, pure ESP-IDF example, CI configuration, and docs are present. |
| Native-tested | Native fake-bus tests pass locally when `pio test -e native` is run on this checkout. |
| PlatformIO built | Arduino ESP32-S2/S3 build and package checks require current local logs or CI logs. |
| ESP-IDF configured | CI is configured to build the native ESP-IDF example for ESP32-S2/S3. |
| ESP-IDF verified | Not claimed unless local `idf.py` output or reviewed CI logs are captured. |
| Partial low-voltage HIL evidence | Compact evidence in `hardware-evidence.md` summarizes Arduino ESP32-S3 CLI-visible behavior and fixed-step transfer budgets from dirty worktrees. |
| Release-grade hardware validated | Not claimed; clean-commit framed HIL, 8-hour clean framed soak, fault-injection fixture coverage, alert-pin capture, controlled reset/power-cycle evidence, and reviewed CI logs are still required. |

## Claim Rules

- "Implemented" means source and examples exist in the tree.
- "Native-tested" requires a current native test log.
- "CI verified" requires reviewed logs for the exact branch, PR, or release
  commit.
- "Partial low-voltage HIL evidence" means useful test evidence exists, but
  fixture gaps, dirty worktrees, or framing anomalies block release-grade
  claims.
- "Hardware validated" requires dated logs with setup details under
  `docs/validation/hardware/...`.

Do not use these phrases without matching evidence:

- `production-ready`
- `field-proven`
- `hardware validated`
- `release-grade hardware validated`
- `85 V safe`
- `ESP-IDF build verified`

Acceptable conservative wording:

- `partial low-voltage Arduino ESP32-S3 HIL evidence exists`
- `CI configured for ESP-IDF builds, pending reviewed logs`
- `hardware validation procedure exists; release-grade hardware validation is not claimed`
- `native fake-bus tests pass locally on the reported checkout`
