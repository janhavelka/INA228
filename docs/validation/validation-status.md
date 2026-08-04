# Validation status

Date: 2026-08-04

This file separates implemented behavior, current local evidence, CI
configuration, historical HIL, and physical validation. Do not infer a stronger
claim from a weaker one.

The exact v3.0.2 release commit `7e280de37927ceff824e6ea30a8fd2f37ac77016`
passed [CI run 30815246708](https://github.com/janhavelka/INA228/actions/runs/30815246708),
including native tests, both Arduino builds, package/documentation validation,
and native ESP-IDF builds for ESP32-S2/S3. The current Unreleased cleanup
worktree has additional local software evidence but has not run in CI and adds
no new hardware-validation claim.

| Evidence level | Current status |
|---|---|
| Implemented | Cooperative owner API, post-write sample/reset/configured-trigger timing, fixed calibration, hardware synchronization, job identity/effects, diagnostics, accumulator epochs, Arduino/native ESP-IDF examples, docs, metadata, and CI guards are present. |
| Native-tested | PlatformIO Core 6.1.19 `pio test -e native` passed 117/117 registered test cases locally on the current Unreleased cleanup worktree on 2026-08-04. The exact v3.0.2 release CI native job also passed its 109/109 tests. |
| Static guards | Core timing, owner contract, exact Arduino/native CLI parity, native ESP-IDF contract, HIL runner byte-compilation/parser-and-report self-test, standalone parser regressions, and the exhaustive dry-run with benchmark rows passed locally on the current cleanup worktree on 2026-08-04. Version-generation consistency, strict host compiler warnings, cppcheck with zero high/medium findings, and `git diff --check` also passed. |
| Arduino stack | PlatformIO Core 6.1.19 with PIOArduino 55.03.311: Arduino-ESP32 3.3.11, ESP-IDF 5.5.5, GCC 14.2.0, and esptool 5.3.0. Runtime `version` output confirmed Arduino-ESP32 3.3.11 and ESP-IDF v5.5.5 on the tested S3. |
| Arduino ESP32-S3 built | PlatformIO `esp32s3dev` passed locally on the current cleanup worktree on 2026-08-04. Firmware used 24,872 bytes RAM and 396,764 bytes application flash. |
| Arduino ESP32-S2 built | PlatformIO `esp32s2dev` passed locally on the current cleanup worktree on 2026-08-04. Firmware used 51,844 bytes RAM and 406,721 bytes application flash. |
| C++17 configuration | PlatformIO removes framework GNU++11 and applies GNU++17; both target builds passed without the earlier inline-variable language-version warning. |
| Package validation | PlatformIO Core 6.1.19 exported the current cleanup worktree on 2026-08-04. Required public/CMake/ESP-IDF/example files and linked Markdown guides were present, repo-only/heavy paths were absent, every packaged Markdown link resolved, and the exported source compiled standalone under C++17; generated validation artifacts were then removed. |
| API documentation | Doxygen 1.13.2 generated the current cleanup worktree on 2026-08-04 with extraction restricted to documented APIs and undocumented entities, missing parameter documentation, and documentation warnings treated as errors; generated output was then removed. |
| ESP-IDF release CI | CI run 30815246708 used ESP-IDF v6.0.1 and passed native example builds for ESP32-S2 and ESP32-S3 on the exact v3.0.2 release commit. |
| ESP-IDF locally built | NOT RUN on 2026-08-04: `idf.py` is unavailable and `IDF_PATH` is unset in the current shell. Arduino builds are not treated as local native ESP-IDF evidence. |
| Historical low-voltage HIL | v2 Arduino ESP32-S3 evidence is preserved in `hardware-evidence.md`; it does not validate v3. |
| v3 low-voltage HIL | Dirty-worktree S3 evidence on 2026-07-31 passed 851 exhaustive/benchmark commands and a separate 5,940-command, 60-second shakedown (eight smoke plus 5,932 soak commands) with zero FAIL/UNKNOWN results. Exact reports and transcripts are under `hardware/2026-07-31/`. |
| Earlier cleanup-worktree S3 HIL | After rebuilding and hash-verified flashing, a local framed exhaustive/benchmark run on the same serial-numbered COM4 fixture passed 851 checks with 0 FAIL, 0 UNKNOWN, and 5 explicit NOT RUN fixture/soak rows. Its temporary report was not retained and it does not cover v3.0.2 or the current Unreleased worktree. |
| v3.0.2 HIL | NOT RUN. No serial port was accessed for this timing correction, and no new hardware evidence is claimed. |
| Release-grade hardware validated | Not claimed. The current HIL used a dirty worktree and lacked controlled fault injection, ALERT-pin capture, reference-instrument accuracy measurements, controlled power cycling, the alternate low-range calibration profile, S2/ESP-IDF physical runs, and an 8-hour clean soak. |

## Current native coverage

The cooperative tests include:

- zero-I2C bind/start/wait/cancel/timeout/cache access;
- exact per-poll and whole-job transfer limits with zero retries;
- failure injection at every distinct cooperative transfer phase, including
  initialization, verification, sampling, reset, and accumulator reset;
- partial versus ambiguous write effects and resynchronization;
- clock-wrap conversion/reset wait boundaries;
- exact post-write sample, reset, and configured-trigger wait boundaries with
  and without `Config::nowMs`, including deferred bus-silent anchoring;
- failed/ambiguous write non-arming, retrigger timing invalidation, raw
  conversion-invalidating writes, and deferred-origin cancellation/timeout
  lifetime;
- stale-result rejection and exactly-once terminal delivery;
- exclusive hardware access during an active job;
- atomic identity commit plus failed/cancelled reinitialization invalidation;
- fixed-unit and legacy range-boundary calibration plans, unsafe-plan
  rejection, and checked public-unit narrowing;
- strict DIEID/revision policy and deterministic alert defaults;
- atomic instantaneous samples and correlated failure diagnostics;
- diagnostic new/sticky/acknowledgement timestamps;
- accumulator scale generations and reset epochs;
- passive health that never suppresses owner transport;
- calibration and alert-config write failures preserving committed cache state
  while marking the affected hardware registers dirty;
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

Acceptable current wording is: the exact v3.0.2 release commit passed its CI
software gates, including native ESP-IDF builds; the current Unreleased cleanup
worktree passes local native tests, static guards, package validation, Doxygen,
and Arduino target builds. CI for the cleanup revision and v3.0.2 physical
validation remain external gates. Historical dirty-worktree low-voltage S3 HIL
applies only to the revisions and fixtures recorded above.

## Remaining release gates

- final clean commit and reviewed CI logs for the current Unreleased cleanup
  revision, including its native ESP-IDF ESP32-S2/S3 jobs;
- repeat the framed exhaustive HIL, including its transfer-budget checks, from
  the final clean commit with no FAIL/UNKNOWN results;
- removal/reappearance, NACK phase, timeout, bus-fault, cancellation, reset, and
  application-owned recovery fault injection;
- alert-pin capture and controlled reset/power-cycle evidence;
- clean 8-hour framed soak;
- approved product calibration and end-to-end external-owner integration HIL;
- high-voltage/electrical safety review and independent protection validation.
