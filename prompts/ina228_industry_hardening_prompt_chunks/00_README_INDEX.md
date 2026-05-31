# INA228 Industry-Readiness Hardening Prompt Pack

Use these prompts one by one, in order. Paste exactly one chunk into the AI coding agent, let it finish, review the report/commit, then send the next chunk.

The goal is an industry-standard INA228 I2C library. The exploration report found a strong framework-neutral/injected-transport foundation, but several production blockers remain:

- destructive `DIAG_ALRT` clear-on-read side effects;
- alert configuration read-modify-write on live status bits;
- triggered conversion freshness and `tick(nowMs)` holes;
- invalid ENERGY/CHARGE exposure in triggered/shutdown modes;
- `setAdcRange()` hardware/cache divergence after partial failure;
- reset/RSTACC semantics;
- transport-error preservation and copy/move safety;
- missing exact calibration/scaling/signedness vectors;
- missing pure ESP-IDF build proof and dated hardware validation matrix;
- insufficient high-voltage safety documentation.

## Prompt order

1. `01_BRANCH_BASELINE_AND_AGENTS.md`
2. `02_DIAG_ALRT_AND_ALERT_CONFIG_SAFETY.md`
3. `03_TRIGGERED_FRESHNESS_AND_TIMING.md`
4. `04_ENERGY_CHARGE_AND_OVERFLOW_VALIDITY.md`
5. `05_CALIBRATION_ADCRANGE_ATOMICITY_SCALING.md`
6. `06_RESET_RSTACC_RECOVERY_PARTIAL_STATE.md`
7. `07_STATUS_ERRORS_COPYMOVE_OUTPUT_ATOMICITY.md`
8. `08_NATIVE_TESTS_FAKE_BUS_AND_DATASHEET_VECTORS.md`
9. `09_ESPIDF_ARDUINO_EXAMPLES_AND_CI.md`
10. `10_DOCUMENTATION_SAFETY_AND_VALIDATION_MATRIX.md`
11. `11_FINAL_INTEGRATION_REVIEW_RELEASE_VERDICT.md`

## Expected branch

```text
hardening/ina228-industry-readiness
```

## Expected final reports

```text
docs/INA228_INDUSTRY_HARDENING_PROGRESS.md
docs/INA228_INDUSTRY_HARDENING_FINAL_REPORT.md
docs/INA228_HARDWARE_VALIDATION_MATRIX.md
```

Do not claim industry-grade until the final report proves P0 issues are fixed, tests pass, CI/build status is clear, and hardware validation status is honestly documented.
