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
| Hardware validated | Not claimed; no dated hardware logs are checked in under `docs/validation/hardware/`. |

## Claim Rules

- "Implemented" means source and examples exist in the tree.
- "Native-tested" requires a current native test log.
- "CI verified" requires reviewed logs for the exact branch, PR, or release
  commit.
- "Hardware validated" requires dated logs with setup details under
  `docs/validation/hardware/...`.

Do not use these phrases without matching evidence:

- `production-ready`
- `field-proven`
- `hardware validated`
- `85 V safe`
- `ESP-IDF build verified`

Acceptable wording before hardware logs exist:

- `pre-production hardening candidate`
- `CI configured for ESP-IDF builds, pending reviewed logs`
- `hardware validation procedure exists; hardware validation is not yet claimed`
- `native fake-bus tests pass locally on the reported checkout`
