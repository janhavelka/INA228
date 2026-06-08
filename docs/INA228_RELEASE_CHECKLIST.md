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

## Explicit Release Gates

These gates must be checked against the exact release/tag commit. Hardware
items remain unchecked until real logs exist.

- [ ] CI green on current branch or PR.
- [ ] Native tests pass.
- [ ] Arduino ESP32-S2 build pass.
- [ ] Arduino ESP32-S3 build pass.
- [ ] ESP-IDF ESP32-S2 `idf.py` build pass.
- [ ] ESP-IDF ESP32-S3 `idf.py` build pass.
- [ ] Version metadata consistent.
- [ ] CHANGELOG updated.
- [ ] Package validation done.
- [ ] Hardware validation matrix run and logs checked in.
- [ ] Safety documentation reviewed.
- [ ] Release wording reviewed.

## Manual CI Review Steps

GitHub CLI was not available in the local cleanup environment. Before merge or
release, review the current GitHub Actions run manually. The workflow runs on
PRs targeting `main`, pushes to `main`, or explicit `workflow_dispatch`; a
plain push to `hardening/ina228-industry-readiness` does not by itself prove CI
unless a PR or manual run exists.

1. Open GitHub Actions for branch `hardening/ina228-industry-readiness` or the
   PR that targets `main`.
2. Confirm native tests passed.
3. Confirm Arduino ESP32-S2 build passed.
4. Confirm Arduino ESP32-S3 build passed.
5. Confirm package validation and guard checks passed.
6. Confirm ESP-IDF ESP32-S2 `idf.py` build passed.
7. Confirm ESP-IDF ESP32-S3 `idf.py` build passed.
8. Attach or link logs before release/tag wording is approved.

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
