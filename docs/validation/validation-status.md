# Validation status

Last reviewed: 2026-09-09

This file separates implemented behavior, current local evidence, CI
configuration, historical HIL, and physical validation. Do not infer a stronger
claim from a weaker one.

The reviewed audit-fix commit is
`2cc5874a4991ff300deb88120a7c796fe46286b1`. It passed
[CI run 33728387995](https://github.com/janhavelka/INA228/actions/runs/33728387995)
on 2026-09-03: native tests, both Arduino target builds, static and parser
contracts, documentation, package validation, and both ESP-IDF ESP32-S2/S3 jobs.
The previously recorded 127-test/11-parser-group follow-up work is committed
and CI-verified on that exact tree.

The fresh-sweep corrections reviewed on 2026-09-09 are a new local worktree
based on `2cc5874`. Their local validation is listed below; the September 3 CI
run does not cover these new edits. No new HIL or hardware validation was run.

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
example, or packaging file differs. The tagged commit also passed
[CI run 31001617981](https://github.com/janhavelka/INA228/actions/runs/31001617981).

| Evidence level | Current status |
|---|---|
| Implemented | Cooperative owner API, post-write sample/reset/configured-trigger timing, fixed calibration, hardware synchronization, job identity/effects, diagnostics, accumulator epochs, Arduino/native ESP-IDF examples, docs, metadata, and CI guards are present. |
| Native-tested | 135/135 registered native tests passed locally on the fresh-sweep worktree on 2026-09-09. Eight new regressions first failed against `2cc5874`; its existing 127 tests passed. The committed base separately passed CI run 33728387995. |
| Static guards | Core timing, owner, CLI/IDF contracts, HIL parser self-test, all 11 standalone parser groups, and eight new CLI regression groups passed locally on 2026-09-09. The new CLI tool compiles actual extracted reporting/parser code on the host and checks scheduling/prompt/usage source contracts; it is wired into CI. Python byte-compilation, exhaustive HIL dry-run, version consistency, and `git diff --check` also passed. This is not ESP32 execution. |
| Arduino stack | PlatformIO Core 6.1.19 with PIOArduino 55.03.311: Arduino-ESP32 3.3.11, ESP-IDF 5.5.5, GCC 14.2.0, and esptool 5.3.0. Runtime `version` output confirmed Arduino-ESP32 3.3.11 and ESP-IDF v5.5.5 on the tested S3. |
| Arduino ESP32-S3 built | PASS locally on the fresh-sweep worktree on 2026-09-09: 24,872 B RAM and 399,440 B flash. Committed `2cc5874` separately passed CI run 33728387995. |
| Arduino ESP32-S2 built | PASS locally on the fresh-sweep worktree on 2026-09-09: 51,844 B RAM and 409,649 B flash. Committed `2cc5874` separately passed CI run 33728387995. |
| C++17 configuration | PlatformIO removes framework GNU++11 and applies GNU++17; both target builds passed without the earlier inline-variable language-version warning. |
| Package validation | PASS locally on 2026-09-09: 37 entries, required public files and README links present, audit/tool/test/script exclusions verified, exported driver compiled under standalone C++17. Committed `2cc5874` separately passed the CI package gate. |
| API documentation | Doxygen warnings-as-errors generation passed locally on the fresh-sweep worktree on 2026-09-09. The committed base passed the same gate in CI run 33728387995. |
| ESP-IDF CI | CI run 33728387995 used ESP-IDF v6.0.1 and passed native example builds for ESP32-S2 and ESP32-S3 on `2cc5874` on 2026-09-03. This verifies the committed prior follow-up, not the new local fresh-sweep edits. |
| ESP-IDF locally built | NOT RUN on 2026-09-09: `idf.py` is unavailable and `IDF_PATH` is unset in this shell. Host CLI regressions and Arduino builds are not native ESP-IDF build evidence. |
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
- diagnostic new/sticky/acknowledgement timestamps and matching snapshot acknowledgements;
- sample preflight failures preserving verified state and the energy epoch;
- runtime cooperative jobs respecting the offline latch;
- transport-returned validation errors updating health;
- legacy sample convenience preserving external job/result ownership;
- unknown job-kind rejection without output mutation;
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

Acceptable current wording is: committed audit-fix tree `2cc5874` passed all
CI software gates on 2026-09-03, including native ESP-IDF builds. The additional
2026-09-09 fresh-sweep work has the local evidence listed above and has not yet
run in CI. Historical low-voltage S3 HIL applies only to the revisions and
fixtures recorded above. Release-grade physical validation remains external.

## Remaining release gates

- repeat the framed exhaustive HIL, including its transfer-budget checks, from
  the final clean commit with no FAIL/UNKNOWN results;
- removal/reappearance, NACK phase, timeout, bus-fault, cancellation, reset, and
  application-owned recovery fault injection;
- alert-pin capture and controlled reset/power-cycle evidence;
- clean 8-hour framed soak;
- approved product calibration and end-to-end external-owner integration HIL;
- high-voltage/electrical safety review and independent protection validation.
