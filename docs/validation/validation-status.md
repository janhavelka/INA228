# Validation status

Last reviewed: 2026-09-12

This page records the strongest current evidence for the source tree and keeps
software verification separate from physical validation. It is not a claim of
production readiness or electrical safety.

## Current software baseline

Release-content commit `c96602fb64e083e7aa7870d8b5cb9cfff1e7db46` passed
[CI run 34690901676](https://github.com/janhavelka/INA228/actions/runs/34690901676)
on 2026-09-12. The exact commit passed all configured jobs:

- 138 native tests;
- Arduino ESP32-S2 and ESP32-S3 builds;
- native ESP-IDF v6.0.1 ESP32-S2 and ESP32-S3 builds;
- CLI, owner, core-timing, and ESP-IDF source contracts;
- HIL parser/self-test and exhaustive dry-run checks;
- version synchronization, warnings-as-errors Doxygen generation, and package
  validation, including a standalone C++17 compile of the exported driver.

The current local source tree based on that commit was also checked on
2026-09-12:

| Check | Result |
| --- | --- |
| Native suite | PASS: 138/138 tests. |
| Static and parser checks | PASS: four contract guards, eight CLI regression tests, 17 HIL parser groups, parser self-test, exhaustive dry run, version check, and `git diff --check`. |
| Arduino ESP32-S3 | PASS with PIOArduino 55.03.311: 24,872 B RAM and 399,488 B flash. |
| Arduino ESP32-S2 | PASS with PIOArduino 55.03.311: 51,844 B RAM and 409,693 B flash. |
| API documentation | PASS: Doxygen completed with warnings treated as errors. |
| Package | PASS: 37 entries, required files and exclusions verified, and exported `INA228.cpp` compiled standalone as C++17. |
| Native ESP-IDF locally | NOT RUN: `idf.py` is unavailable in this shell. The exact release-content commit passed both native ESP-IDF targets in CI. |
| Physical HIL | NOT RUN for this worktree. |

Native fake-bus tests prove driver logic, transaction order, bounded transfer
budgets, failure handling, output atomicity, calibration/diagnostic contracts,
and wrap-safe timing. They do not prove electrical timing, signal integrity,
silicon behavior, scheduler integration, or measurement accuracy on hardware.

## Physical evidence

[Hardware evidence](hardware-evidence.md) preserves the dated summary of the
available low-voltage ESP32-S3 runs: the 2026-07-31 dirty-worktree migration
sweep and short shakedown, plus a 2026-08-04 clean-commit targeted suite and
dirty-worktree CLI smoke run. They had no FAIL or UNKNOWN verdicts, but all
predate the current source and omit the release gates listed below. Historical
v2 results do not validate v3.

Release-grade hardware validation is not claimed. No retained evidence covers
the current clean commit across controlled transport faults, ALERT-pin capture,
reference-instrument accuracy, controlled power cycling, both shunt ranges,
ESP32-S2 and native ESP-IDF hardware, and a clean eight-hour soak.

## Claim rules

- **Implemented** means the behavior exists in the source tree.
- **Native-tested** means host tests passed; it does not mean hardware ran.
- **Built** means the named framework/target compiled and linked.
- **CI verified** requires a successful run for the exact cited commit.
- **Historical HIL** applies only to its recorded commit, firmware, and fixture.
- **Hardware validated** requires dated, commit-linked logs with complete setup,
  procedure, expected/actual results, and equipment details.

Do not describe the library as production-ready, field-proven, 85 V safe, or
release-grade hardware validated without matching evidence. The INA228 and its
ALERT output are monitoring components, not certified safety functions.

## Remaining hardware release gates

- Run the framed exhaustive HIL suite from the final clean commit with no FAIL
  or UNKNOWN verdicts.
- Exercise removal/reappearance, NACK phases, timeouts, bus faults,
  cancellation, reset, and application-owned recovery.
- Capture ALERT behavior and controlled reset/power-cycle results.
- Complete a clean eight-hour framed soak.
- Validate the approved product calibration and external-owner integration.
- Complete high-voltage/electrical safety review and independent protection
  validation.

Use the [hardware validation procedure](hardware-validation-procedure.md) for
the required setup and evidence format.
