# INA228 Release Checklist

This checklist blocks release/tag claims until evidence exists. It is a
no-hardware release-control document; it does not claim validation was run.

## Required Before Merge

| Item | Required evidence | Current status |
| --- | --- | --- |
| Branch review | PR review and final maintainer approval | Pending |
| CI green | Current GitHub Actions run for the branch or PR | Pending |
| Native tests | `python -m platformio test -e native` log | Available locally in reports |
| Arduino S2/S3 builds | `python -m platformio run -e esp32s2dev` and `esp32s3dev` logs | Available locally in reports |
| Package validation | `python -m platformio pkg pack` log and generated tarball removed | Available locally in reports |
| Release wording review | README/final report avoid overclaims | Done, keep reviewing |

## Required Before Release Or Tag

| Item | Required evidence | Current status |
| --- | --- | --- |
| SemVer/version/changelog | `library.json`, generated `Version.h`, `idf_component.yml`, README, and CHANGELOG agree | `2.0.0` candidate |
| Current CI green | GitHub Actions logs for native, Arduino, package, guards, and ESP-IDF jobs | Pending current run review |
| ESP-IDF build proof | Local `idf.py` logs or reviewed remote CI logs for `esp32s2` and `esp32s3` | Pending |
| Hardware validation matrix | PASS/FAIL rows with dated logs under `docs/validation/hardware/...` | `NOT RUN` |
| High-voltage safety review | README and hardware matrix reviewed for isolation, fusing, grounding, shunt dissipation, and USB-ground hazards | Pending release-owner review |
| Package artifact decision | Release owner confirms packed artifact and tag contents | Pending |

## Forbidden Release Claims Without Evidence

Do not use these phrases unless corresponding dated logs exist:

- `production-ready`
- `fully industry-grade`
- `field-proven`
- `hardware validated`
- `85 V safe`
- `ESP-IDF build verified`

Allowed wording before hardware logs exist:

- `industry-readiness hardened pre-production candidate`
- `CI configured for ESP-IDF builds, pending reviewed logs`
- `hardware validation matrix exists; all hardware rows remain NOT RUN`
- `native fake-bus tests pass locally on the reported checkout`

## Final Release Gate

A release/tag is blocked until all of these are true:

- SemVer, version metadata, and changelog are final.
- Current CI is green.
- ESP-IDF build proof is captured for ESP32-S2 and ESP32-S3.
- Hardware validation matrix has dated logs for required rows.
- Package validation is complete.
- High-voltage safety documentation is reviewed.
