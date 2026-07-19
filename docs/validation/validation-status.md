# Validation status

Date: 2026-07-19

This file separates implemented behavior, current local evidence, CI
configuration, historical HIL, and physical validation. Do not infer a stronger
claim from a weaker one.

The v3 hardening branch was developed from INA228 commit
`c5691e935a2f4b3184a938e36cfca56c80df0e6e`. Results below apply to this branch
change set. Exact pushed commit IDs are recorded in the task handoff; this is
not yet a reviewed release tag.

| Evidence level | Current status |
|---|---|
| Implemented | Cooperative owner API, fixed calibration, hardware synchronization, job identity/effects, diagnostics, accumulator epochs, Arduino/native ESP-IDF examples, docs, metadata, and CI guards are present. |
| Native-tested | `python -m platformio test -e native` passed 101/101 test cases locally in 2.407 s on 2026-07-19 after independent-review fixes. |
| Static guards | Core timing, owner contract, CLI contract, native ESP-IDF contract, HIL parser self-test, and generated-version check passed locally on 2026-07-19. |
| Arduino ESP32-S3 built | Clean PlatformIO `esp32s3dev` build passed locally on 2026-07-19. |
| Arduino ESP32-S2 built | Clean PlatformIO `esp32s2dev` build passed locally on 2026-07-19. |
| C++17 configuration | PlatformIO removes framework GNU++11 and applies GNU++17; the clean builds passed without the earlier inline-variable language-version warning. |
| Package validation | `python -m platformio pkg pack` produced `INA228-3.0.0.tar.gz` successfully on 2026-07-19; the generated artifact was then removed. |
| API documentation | Doxygen 1.15.0 generated the configured documentation locally without warnings on 2026-07-19; generated output was then removed. |
| ESP-IDF configured | CI uses ESP-IDF v6.0.1 to build the native example for ESP32-S2/S3. |
| ESP-IDF locally built | Not run: `idf.py` is not installed in the current shell. Reviewed CI logs for a final commit are still required. |
| Historical low-voltage HIL | v2 Arduino ESP32-S3 evidence is preserved in `hardware-evidence.md`; it does not validate v3. |
| v3 hardware validated | Not claimed. No v3 HIL or electrical validation was performed in this task. |

## Current native coverage

The cooperative tests include:

- zero-I2C bind/start/wait/cancel/timeout/cache access;
- exact per-poll and whole-job transfer limits with zero retries;
- failure injection at every distinct cooperative transfer phase, including
  initialization, verification, sampling, reset, and accumulator reset;
- partial versus ambiguous write effects and resynchronization;
- clock-wrap conversion/reset wait boundaries;
- stale-result rejection and exactly-once terminal delivery;
- exclusive hardware access during an active job;
- atomic identity commit plus failed/cancelled reinitialization invalidation;
- fixed-unit calibration vectors and unsafe-plan rejection;
- strict DIEID/revision policy and deterministic alert defaults;
- atomic instantaneous samples and correlated failure diagnostics;
- diagnostic new/sticky/acknowledgement timestamps;
- accumulator scale generations and reset epochs;
- passive health that never suppresses owner transport.
- retained range/timing/averaging, reset/replay, accumulator, and explicit
  legacy offline-policy contracts.

Native fake-bus tests validate logic and transaction ordering, not electrical
timing, signal integrity, silicon behavior, or an application scheduler.

## Claim rules

- "Implemented" means source and examples exist in the tree.
- "Native-tested" requires a current native test log.
- "Built" means compilation/linking for the named target; it is not hardware
  execution.
- "CI verified" requires reviewed logs for the exact branch, PR, or release
  commit.
- "Historical HIL" describes only its exact historical commit/fixture.
- "Hardware validated" requires dated, commit-linked logs with setup details
  under `docs/validation/hardware/`.

Do not use these phrases without matching evidence:

- `production-ready`
- `field-proven`
- `hardware validated`
- `release-grade hardware validated`
- `85 V safe`
- `ESP-IDF build verified`

Acceptable current wording is: the v3 owner contract is implemented and passes
native tests plus Arduino target builds; native ESP-IDF CI and physical v3
validation remain external gates.

## Remaining release gates

- final clean commit and reviewed CI logs for that exact revision;
- native ESP-IDF ESP32-S2/S3 build evidence from CI or a configured local SDK;
- v3 framed targeted and transfer-budget HIL with no FAIL/UNKNOWN results;
- removal/reappearance, NACK phase, timeout, bus-fault, cancellation, reset, and
  application-owned recovery fault injection;
- alert-pin capture and controlled reset/power-cycle evidence;
- clean 8-hour framed soak;
- approved product calibration and end-to-end external-owner integration HIL;
- high-voltage/electrical safety review and independent protection validation.
