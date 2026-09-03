# Validation status

Last reviewed: 2026-09-01

This file separates implemented behavior, current local evidence, CI
configuration, historical HIL, and physical validation. Do not infer a stronger
claim from a weaker one.

The post-v3.0.3 audit-fix worktree based on
`a41646cff67ef49dab000fd24142b2bae63938eb` has local-only software evidence from
2026-09-01: 127/127 native tests, both Arduino target builds, all static
contracts, HIL parser self-test plus 11 regression groups, exhaustive HIL dry
run, version consistency, Python byte-compilation, Doxygen warnings-as-errors,
and package export/smoke compilation passed. These uncommitted changes have not
run in CI and add no hardware-validation claim. The exact clean base commit
`a41646c` passed
[CI run 33321351272](https://github.com/janhavelka/INA228/actions/runs/33321351272)
on 2026-08-30, including the repository's native, Arduino, package,
documentation, static-contract, and ESP-IDF ESP32-S2/S3 jobs.

The clean base commit `cb3eb2bc6d5cc63380683cc603c46cc5dfe915c5` passed
[CI run 30902376574](https://github.com/janhavelka/INA228/actions/runs/30902376574)
on 2026-08-04, including native tests, both Arduino builds,
package/documentation validation, and native ESP-IDF builds for ESP32-S2/S3.
The exact v3.0.2 release commit also passed
[CI run 30815246708](https://github.com/janhavelka/INA228/actions/runs/30815246708).
The v3.0.3 release-content commit
`dc05bd7257929b7d2d7ad2b47b00e7adbbab9b82` passed
[CI run 31000462035](https://github.com/janhavelka/INA228/actions/runs/31000462035)
on 2026-08-05, including all of the same software gates. It adds no new
hardware-validation claim.

Tag `v3.0.3` points at `6137ee7984f191ceedd8cdb65886112b9da1ea15`, one commit
after `dc05bd7`. That commit changes `.github/workflows/ci.yml` (GitHub action
major versions), `CHANGELOG.md`, and validation documentation; no library source,
example, or packaging file differs. No separate CI run is recorded for the
tagged commit itself.

| Evidence level | Current status |
|---|---|
| Implemented | Cooperative owner API, post-write sample/reset/configured-trigger timing, fixed calibration, hardware synchronization, job identity/effects, diagnostics, accumulator epochs, Arduino/native ESP-IDF examples, docs, metadata, and CI guards are present. |
| Native-tested | PlatformIO Core 6.1.19 `pio test -e native` passed 127/127 registered tests locally on the post-v3.0.3 audit-fix worktree on 2026-09-01. This is dirty-worktree evidence, not CI evidence. The prior v3.0.3 release-candidate worktree passed 117/117 on 2026-08-04; the exact v3.0.2 release CI native job passed 109/109. |
| Static guards | Core timing, owner contract, checked Arduino/native CLI contract parity, native ESP-IDF contract, Python byte-compilation, HIL parser self-test, all 11 standalone parser regression groups, exhaustive dry-run with benchmark/NOT RUN rows, version consistency, Doxygen warnings-as-errors, and `git diff --check` passed locally on the audit-fix worktree on 2026-09-01. CI run 33321351272 separately passed the pre-fix clean base. |
| Arduino stack | PlatformIO Core 6.1.19 with PIOArduino 55.03.311: Arduino-ESP32 3.3.11, ESP-IDF 5.5.5, GCC 14.2.0, and esptool 5.3.0. Runtime `version` output confirmed Arduino-ESP32 3.3.11 and ESP-IDF v5.5.5 on the tested S3. |
| Arduino ESP32-S3 built | PlatformIO `esp32s3dev` passed locally on the audit-fix worktree on 2026-09-01. Firmware used 24,872 bytes RAM and 398,680 bytes application flash. |
| Arduino ESP32-S2 built | PlatformIO `esp32s2dev` passed locally on the audit-fix worktree on 2026-09-01. Firmware used 51,844 bytes RAM and 408,741 bytes application flash. |
| C++17 configuration | PlatformIO removes framework GNU++11 and applies GNU++17; both target builds passed without the earlier inline-variable language-version warning. |
| Package validation | PlatformIO Core 6.1.19 exported 37 package entries from the audit-fix worktree on 2026-09-01. Required public/CMake/ESP-IDF files were present, all `docs/CODE_AUDIT*` records were absent, and exported `src/INA228.cpp` compiled standalone under C++17. The broader v3.0.3 release-candidate package/link gate passed on 2026-08-04. |
| API documentation | Doxygen generated the audit-fix worktree on 2026-09-01 with warnings treated as errors. Doxygen 1.13.2 performed the prior v3.0.3 release-candidate validation on 2026-08-04. |
| ESP-IDF CI | CI run 33321351272 used ESP-IDF v6.0.1 and passed native example builds for ESP32-S2 and ESP32-S3 on clean base `a41646c`. Runs 31000462035, 30902376574, and 30815246708 passed the same targets on the v3.0.3 release-content, clean-base, and exact v3.0.2 release commits, respectively. The current uncommitted fixes have static-contract and Arduino-build evidence only, not a local native-IDF build. |
| ESP-IDF locally built | NOT RUN on 2026-09-01: `idf.py` is unavailable and `IDF_PATH` is unset in the current shell. Arduino builds are not treated as local native ESP-IDF evidence. |
| Historical low-voltage HIL | v2 Arduino ESP32-S3 evidence is preserved in `hardware-evidence.md`; it does not validate v3. |
| v3 low-voltage HIL | Dirty-worktree S3 evidence on 2026-07-31 passed 851 exhaustive/benchmark commands and a separate 5,940-command, 60-second shakedown (eight smoke plus 5,932 soak commands) with zero FAIL/UNKNOWN results. Summarized in `hardware-evidence.md`; the generated reports and raw transcripts are not retained. |
| v3.0.2-era HIL | On 2026-08-04 the Arduino ESP32-S3 firmware from clean commit `cb3eb2bc6d5c` passed 189/189 executable checks in the framed targeted suite (zero FAIL/UNKNOWN, five explicit NOT RUN fixture/soak rows), and a follow-up smoke suite on the CLI-output fix passed 8/8. Both runs predate the final v3.0.3 tree and neither report was retained. |
| Release-grade hardware validated | Not claimed. The clean targeted HIL predates the v3.0.3 worktree, and the subsequent CLI-fix run was dirty and smoke-only. Neither covers controlled fault injection, ALERT-pin capture, reference-instrument accuracy measurements, controlled power cycling, the alternate low-range calibration profile, S2/ESP-IDF physical runs, or an 8-hour clean soak. |

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
- destructive verification-DIAG failures clearing uncertain trigger timing
  without revoking synchronized configuration;
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

Acceptable current wording is: clean base `a41646c` passed its CI software
gates, including native ESP-IDF builds; the post-v3.0.3 audit-fix worktree passes
local native tests, static guards, package validation, Doxygen, and Arduino
target builds but has not run in CI. A clean `cb3eb2bc6d5c` targeted S3 HIL run
is local, partial evidence only; a subsequent dirty-worktree smoke run verifies
the CLI-output correction on COM21 but predates the final cleanup. Release-grade
physical validation remains an external gate. Historical
dirty-worktree low-voltage S3 HIL applies only to the revisions and fixtures
recorded above.

## Remaining release gates

- repeat the framed exhaustive HIL, including its transfer-budget checks, from
  the final clean commit with no FAIL/UNKNOWN results;
- removal/reappearance, NACK phase, timeout, bus-fault, cancellation, reset, and
  application-owned recovery fault injection;
- alert-pin capture and controlled reset/power-cycle evidence;
- clean 8-hour framed soak;
- approved product calibration and end-to-end external-owner integration HIL;
- high-voltage/electrical safety review and independent protection validation.
