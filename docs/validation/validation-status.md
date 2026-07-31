# Validation status

Date: 2026-07-31

This file separates implemented behavior, current local evidence, CI
configuration, historical HIL, and physical validation. Do not infer a stronger
claim from a weaker one.

The current PIOArduino migration was tested from base commit
`4c32312b32a59bf38192080c9043170d2a001d33` with the migration changes present
in a dirty worktree. It is not yet a reviewed release tag or clean-commit
release qualification.

| Evidence level | Current status |
|---|---|
| Implemented | Cooperative owner API, fixed calibration, hardware synchronization, job identity/effects, diagnostics, accumulator epochs, Arduino/native ESP-IDF examples, docs, metadata, and CI guards are present. |
| Native-tested | `pio test -e native` passed 101/101 test cases locally in 2.396 s on 2026-07-31, including the 15 mOhm/10 A calibration-quantization regression. |
| Static guards | Core timing, owner contract, CLI contract, native ESP-IDF contract, and HIL parser self-test passed locally on 2026-07-31. |
| Arduino stack | Pinned to PIOArduino 55.03.311: Arduino-ESP32 3.3.11, ESP-IDF 5.5.5, GCC 14.2.0, and esptool 5.3.0. Runtime `version` output confirmed Arduino-ESP32 3.3.11 and ESP-IDF v5.5.5 on the tested S3. |
| Arduino ESP32-S3 built | PlatformIO `esp32s3dev` build passed locally in 8.077 s on 2026-07-31. Firmware used 24,872 bytes RAM and 396,292 bytes application flash. |
| Arduino ESP32-S2 built | PlatformIO `esp32s2dev` build passed locally in 8.950 s on 2026-07-31. Firmware used 51,844 bytes RAM and 406,341 bytes application flash. |
| C++17 configuration | PlatformIO removes framework GNU++11 and applies GNU++17; the clean builds passed without the earlier inline-variable language-version warning. |
| Package validation | `python -m platformio pkg pack` produced `INA228-3.0.0.tar.gz` successfully on 2026-07-19; the generated artifact was then removed. |
| API documentation | Doxygen 1.15.0 generated the configured documentation locally without warnings on 2026-07-19; generated output was then removed. |
| ESP-IDF configured | CI uses ESP-IDF v6.0.1 to build the native example for ESP32-S2/S3. |
| ESP-IDF locally built | Not run: `idf.py` is not installed in the current shell. Reviewed CI logs for a final commit are still required. |
| Historical low-voltage HIL | v2 Arduino ESP32-S3 evidence is preserved in `hardware-evidence.md`; it does not validate v3. |
| v3 low-voltage HIL | Dirty-worktree S3 evidence on 2026-07-31 passed 851 exhaustive/benchmark commands and a separate 5,932-command, 60-second shakedown with zero FAIL/UNKNOWN results. Exact reports and transcripts are under `hardware/2026-07-31/`. |
| Release-grade hardware validated | Not claimed. The current HIL used a dirty worktree and lacked controlled fault injection, ALERT-pin capture, reference-instrument accuracy measurements, controlled power cycling, the alternate low-range calibration profile, S2/ESP-IDF physical runs, and an 8-hour clean soak. |

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

Acceptable current wording is: the v3 owner contract passes current native
tests, Arduino target builds, and dirty-worktree low-voltage S3 functional HIL;
native ESP-IDF review and clean-commit release-grade physical validation remain
external gates.

## Remaining release gates

- final clean commit and reviewed CI logs for that exact revision;
- native ESP-IDF ESP32-S2/S3 build evidence from CI or a configured local SDK;
- repeat the framed exhaustive and transfer-budget HIL from the final clean
  commit with no FAIL/UNKNOWN results;
- removal/reappearance, NACK phase, timeout, bus-fault, cancellation, reset, and
  application-owned recovery fault injection;
- alert-pin capture and controlled reset/power-cycle evidence;
- clean 8-hour framed soak;
- approved product calibration and end-to-end external-owner integration HIL;
- high-voltage/electrical safety review and independent protection validation.
