# TunnelMonitor-node suitability re-audit

Date: 2026-07-19
Status: implementation complete in the INA228 v3 working tree; native/build/HIL
evidence remains classified separately below.

## Audit basis

INA228 was re-audited from baseline commit
`c5691e935a2f4b3184a938e36cfca56c80df0e6e` on branch
`hardening/tunnelmonitor-suitability-reaudit`. At the start, that commit matched
local `main` and `origin/main`; tag `v2.0.0` was one commit behind. The only
post-tag commit added the earlier suitability audit.

TunnelMonitor-node was inspected read-only at local commit
`602114ea6c723e31c41f0eb7cd8ac2b56a46d40e` on branch
`prompt-44b-sequence`, one commit behind its remote. Its worktree already had an
unrelated modified `.vscode/extensions.json` and an untracked report under
`docs/reports/`; neither was modified or included. The authoritative integration
contracts were revalidated against commit range based on `fff99...`; subsequent
local commits did not change the relevant INA228/I2C ownership contract.

This document supersedes the 2026-07-18 assessment and its proposed API names.
Current source, tests, public headers, examples, CI, repository `AGENTS.md`, and
the following TunnelMonitor areas were inspected:

- the I2C peripheral ownership guideline and target architecture;
- I2C task request/result identity, deadline, slot-capacity, and retry rules;
- INA228 dependency authority and existing direct-register adapter behavior;
- power-monitor product constants and health/result publication paths.

No TunnelMonitor source was changed. Its dependency authority still requires an
independently hardened, exact reviewed pin plus a private adapter and HIL before
integration. Product-specific calibration/profile selection remains an explicit
application decision.

## Required TunnelMonitor boundary

The current authoritative application contract is:

| Concern | TunnelMonitor owner contract |
|---|---|
| Bus ownership | Only `I2cTask` owns or calls the bus; the library owns no task, queue, lock, retry, or recovery. |
| Normal scheduling | A normal ReadPower owner poll permits at most one library transport callback. |
| Transfer bound | Each callback is bounded by 20 ms. |
| Operation deadline | The owner uses the earlier incoming deadline or its fixed 1000 ms ReadPower bound; progress cannot renew it. |
| Retry | Read callbacks may retain owner bus recovery plus one retry. Mutating callbacks are one physical attempt; ambiguous effects require reconciliation, not blind retry. |
| Outer identity | Results use `(requestId, submissionToken, device, operation)` with eight protected slots, 1250 ms cutoff, and exactly-once delivery. |
| Inner identity | INA228 `requestToken`/`operationId` protects library-job correlation and must not duplicate or replace the outer I2C queue identity. |
| Product facts | Address `0x41`, 400 kHz bus, 25,000 uOhm shunt, 2,500 mA maximum, 1,500 mA warning, 15,000 mV bus maximum. |
| Publication | Bus mV and current mA are required; shunt, power, and temperature are diagnostic. Optional hot-plug, stale/cache, health, and recovery policy remains application-owned. |

The current firmware's direct-register implementation uses `SHUNT_CAL=0x0666`,
nominal 5000 nA/bit, and 16 uW/bit. Those values are evidence of existing
behavior, not an approved frozen product choice. The v3 integration must select
and approve either a maximum-current-derived or explicit-current-LSB profile,
then validate it on hardware.

## Architecture selected

Version 3 adds one fixed-memory cooperative job slot per instance:

1. `bind(config)` validates and retains desired state with zero I2C.
2. A typed `start*()` returns a nonzero operation ID and records the request
   token with zero I2C.
3. `pollJob(nowMs, maxTransfers)` performs at most the caller's callback budget;
   zero is valid and bus-silent, including elapsed wait-gate advancement.
4. The owner can inspect cache-only progress, cancel, or time out without I2C.
5. A terminal result exposes success/failure/cancel/timeout plus
   `NONE`/`CONFIRMED`/`PARTIAL`/`INDETERMINATE` hardware effect.
6. `takeJobResult(operationId, result)` performs exactly-once correlated delivery.

Initialization, reinitialization, configuration verification, instantaneous
sampling, software reset, and accumulator reset share this model. There are no
driver retries. The application still owns any retry or bus recovery. Legacy
synchronous typed APIs remain for compatibility and diagnostics, share the
validated state, and are documented as multi-callback conveniences rather than
the external-owner production path.

## Finding dispositions

All earlier hard and secondary findings were confirmed against v2 and resolved
in v3. Test names below are in `test/test_basic.cpp`.

| Finding | Severity / affected contract | Revalidated v2 evidence | v3 resolution and evidence | Status |
|---|---|---|---|---|
| H-01 initialization/recovery exceed owner budget | Critical: scheduling/deadline | `begin()` and `recover()` executed multi-register procedures synchronously. | Zero-I2C bind/start plus caller-budgeted initialize/reinitialize. Exact limits are reported by `getJobLimits()`. Covered by `test_cooperative_initialize_budget_order_forwarding_and_alert_determinism` and `test_cooperative_job_limits_are_exact_and_retry_free`. | Resolved |
| H-02 staged jobs cannot be cancelled | High: cancellation/lifecycle | Legacy staged helpers had no coherent cancellation/result contract. | Bus-silent, idempotent `cancelJob()` and `timeoutJob()` create terminal results with explicit effect. Covered by `test_cancel_and_timeout_are_bus_silent_with_precise_effects` and reset/accumulator cancellation tests. | Resolved |
| H-03 sample can publish invalid current/power | Critical: measurement integrity | The old stepped sample did not make DIAG/MATHOF evidence part of one atomic outcome. | Cooperative sample captures DIAG before five-channel reads, enforces MEMSTAT/CNVRF/MATHOF, and commits only a complete sample. Failed terminal results retain correlated DIAG scratch if captured. Covered by diagnostic and every-stage failure-injection tests. | Resolved |
| H-04 stepped continuous reads are not coherent | High: sample coherence | Registers were read from an ongoing continuous conversion stream. | One triggered-all sequence waits, reads while shutdown, restores and verifies the configured mode, and publishes one generation-tagged sample. Covered by `test_triggered_sample_sequence_wait_budget_and_atomic_result` and wrap-boundary test. | Resolved |
| H-05 calibration cannot express product profile | High: calibration/scaling | Float-only maximum-current planning could not preserve an explicit current LSB or expose quantization/range decisions. | Fixed-unit `CalibrationConfig` supports disabled, maximum-current-derived, and explicit-LSB modes. Pure planning reports selected/effective LSB, clamping, and both range limits. Covered by the TunnelMonitor profile/strict-limit vectors. | Resolved; product profile choice external |
| H-06 health conflicts with owner policy | High: I2C ownership/recovery | Latched OFFLINE could suppress a transaction and counted per callback rather than outer request. | `PASSIVE` is the default: library health is diagnostic and never suppresses owner-requested transport. Legacy latch remains opt-in only. Covered by `test_passive_health_never_suppresses_owner_requested_transport`. | Resolved |
| H-07 staged jobs lack exclusive access | Critical: state/exclusivity | Other hardware APIs could interleave with staged state. | One active job or unconsumed terminal result owns the instance hardware path; hardware APIs return `BUSY` until matching result consumption, while cache-only inspection/take/end/invalidation remains available. Covered by `test_active_job_excludes_other_hardware_apis_but_allows_cache_access`. | Resolved |
| H-08 clean cache does not prove hardware | Critical: cache/hardware integrity | Cached configuration could be presented as clean without full readback after external reset/recovery. | `HardwareState` separates unbound, unknown, synchronized, and resync-required states. Identity is staged in job-local scratch and committed only with the fully verified configuration. Failed/cancelled reconciliation and active-job invalidation revoke initialized/identity state. Covered by `test_invalidate_and_configuration_guard_require_verified_reappearance`, `test_reinitialize_identity_commit_cancel_and_invalidation_are_atomic`, and initialization readback/failure tests. | Resolved |
| H-09 sticky diagnostic evidence lacks lifecycle | High: diagnostic evidence | One preserved snapshot did not represent new/sticky/acknowledged events. | Fixed `DiagnosticEvents` records latest, new, sticky, observation time, and first-observed time per bit; cache-only acknowledgement is explicit. Covered by deterministic acknowledgement/timestamp tests. | Resolved |
| H-10 accumulator counts can cross scale generations | Critical: energy/charge scaling | Configuration changes could revalidate accumulated counts under a new scaling contract. | Configuration and accumulator generations/epochs are tracked. Range, calibration, mode/timing, triggered operation, temperature compensation, and reset transitions invalidate accumulation until verified reset establishes a coherent epoch. Covered by `test_accumulator_epoch_requires_verified_reset_after_each_generation`. | Resolved |
| H-11 startup alert config inherited hardware state | High: deterministic alert configuration | DIAG_ALRT writable bits were read-modify-written from live/destructive state. | `AlertConfig` declares latch, conversion-ready, slow-alert, and polarity defaults; initialization writes deterministic writable bits and verifies them. No live-register RMW is used. Covered by initialization order/failure tests and `test_revision_policy_and_declared_alert_defaults_are_verified`. | Resolved |
| S-01 identity assumes exact `0x2281` | Medium: identity/revision policy | Revision nibble was folded into exact ID comparison. | `parseDeviceIdentity()` validates DIEID `0x228`, separates revision, and applies `supportedRevisionMask` during initialization. Covered by strict parser and revision-policy tests. | Resolved |
| S-02 unsafe calibration silently clamps | High: configuration safety | Legacy planning could accept a range-exceeding/clamped plan. | Unsafe planning fails by default; `allowUnsafePlan` is an explicit caller opt-in and the plan still reports unsafe conditions. Covered by `test_fixed_calibration_plans_cover_tunnelmonitor_profile_and_strict_limits`. | Resolved |
| S-03 synchronous reset ignores startup wait | High: reset timing/effects | Legacy reset procedure did not model the data-sheet startup gate as owner-visible work. | Cooperative reset has a declared maintenance bound, a zero-I2C startup wait, wrap-safe deadline math, verification, and full reinitialization. Covered by reset wait/zero-budget tests. | Resolved |
| S-04 product-specific public naming | Low: API clarity/compatibility | `readPowerSampleRawStep` mixed one application-shaped bundle with public mechanism. | Production path is the device-neutral `INSTANTANEOUS_SAMPLE` job and generic `pollJob`; legacy names remain compatibility conveniences and are not the recommended integration API. | Resolved without silent removal |
| S-05 Arduino C++17 flag ineffective | Medium: portability/build contract | Arduino injected `-std=gnu++11` after the project flag, producing inline-variable warnings. | PlatformIO removes `-std=gnu++11` and applies `-std=gnu++17`; CI builds both supported Arduino targets and guards the effective config. | Resolved |

## Operation bounds and scheduling fit

| Operation | Class | Maximum callbacks | Driver retries | Wait/effect policy |
|---|---|---:|---:|---|
| Initialize/reinitialize | Multi-step runtime | 14 | 0 | No internal delay; verified terminal state. |
| Verify configuration | Multi-step runtime | 8 | 0 | No configuration writes; destructive DIAG evidence is captured and failures remain observable. |
| Instantaneous sample | Steady-state | 11 | 0 | Conversion wait is a zero-I2C gate; atomic publication. |
| Software reset | Maintenance | 16 | 0 | Explicit startup wait; reset and later config effects remain observable. |
| Accumulator reset | Multi-step runtime | 2 | 0 | One write plus readback; ambiguous write forces resync. |

These maxima fit the application's one-callback-per-owner-poll rule because
`maxTransfers=1` is enforced per call. They do not by themselves prove the
application's 1000 ms outer deadline for every configured averaging/conversion
profile. The adapter must compare `getJobLimits().maxWaitMicroseconds` plus its
callback and scheduling allowance against the earlier outer deadline before
starting work.

The driver operation ID is nested inside the application's protected outer
result identity. The adapter must retain both, reject stale inner terminal
results, and publish through the existing eight-slot outer mechanism exactly
once. The driver does not add a queue or change that capacity.

## TunnelMonitor integration flow

1. Keep the existing I2C task as the sole bus owner and inject its bounded
   transport adapter; do not let INA228 configure or recover the bus.
2. Bind address `0x41`, 20 ms callback bound, passive health, deterministic
   alert defaults, and the approved fixed-unit product calibration profile.
3. On first use, reappearance, or after owner bus recovery, invalidate hardware
   state and start initialization/reinitialization.
4. On each owner poll, call `pollJob(nowMs, 1)` only when the earlier outer
   deadline still permits work. Use `pollJob(nowMs, 0)` only to advance/check a
   wait gate without consuming I2C budget.
5. On outer cancellation/deadline, call bus-silent cancel/timeout, consume the
   matching inner result, and reconcile before further measurement if the
   effect is partial or indeterminate.
6. For ReadPower, start one instantaneous sample, publish only a matching
   successful terminal sample, map bus mV/current mA to required fields, and
   treat other channels/diagnostics per application policy.
7. Keep the existing owner read-only recovery-plus-one-retry rule outside the
   library. Never automatically retry an ambiguous write.

## Separate current firmware findings

The read-only re-audit also observed application issues that are not repaired by
changing this general-purpose library:

- an alternate identity mask can accept values whose DIEID bits do not equal
  `0x228`;
- direct-register sampling relies on ADC defaults rather than a verified desired
  profile;
- MATHOF handling can leave derived data marked valid;
- calibration is repeatedly written without verified configuration generation;
- an existing stale-data rule is not applied to this publication path.

These belong in the future private TunnelMonitor adapter/integration change,
after dependency approval. No application-side state machine was added here to
compensate for library defects.

## Evidence and remaining gates

Implemented behavior is supported by native fault-injection tests for every
distinct cooperative transfer phase, exact work budgets, zero-I2C waits, cancellation,
timeouts, wrap boundaries, ambiguous writes, stale/exactly-once results,
hardware synchronization, diagnostic lifecycle, calibration boundaries,
accumulator epochs, and passive health. Static guards check framework neutrality,
owner API/example use, ESP-IDF purity, version synchronization, and effective
C++17 configuration.

Build commands and actual results are recorded in
`docs/validation/validation-status.md`.
Generic dirty-worktree ESP32-S3/INA228 v3 HIL from 2026-07-31 is summarized in
`docs/validation/hardware-evidence.md`. It exercises this library and example,
but it does not validate the private TunnelMonitor adapter, exact product pins,
outer owner, product calibration, or end-to-end publication path. The following
external gates remain:

- ESP32-S2 hardware coverage and ESP32-S3 controlled fault/electrical runs of
  cancellation, removal, reappearance, NACK/timeout/bus fault, reset, and
  diagnostic behavior;
- approval of the TunnelMonitor calibration/current-LSB profile;
- an exact reviewed dependency commit pin and private adapter review;
- end-to-end HIL of outer deadline, eight-slot identity, one-transfer owner
  budget, exactly-once publication, removal/reappearance, and owner recovery;
- electrical validation of shunt tolerance/heating, ADC range, temperature
  behavior, alert wiring, high-voltage layout, and independent protection.

## Final assessment

The v3 library contract is suitable as the device-level component for a future
TunnelMonitor private adapter: it is general-purpose, externally scheduled,
fixed-memory, retry-free, budgeted, cancellable, result-correlated, and explicit
about verified versus unknown hardware state. TunnelMonitor itself is not yet
integrated or hardware-validated. Suitability therefore means the library-side
blockers are resolved; it is not a claim of product readiness or field proof.
