# Shared Rules for INA228 Industry-Readiness Hardening

You are working in the INA228 I2C library repository.

The exploration report classified the repo as **engineering-grade with major gaps**. The core architecture is good, but several INA228-specific semantics must be fixed before the library can be honestly called industry-standard.

Use the ADS1115 workflow only as a quality/style reference. Do not blindly copy ADS1115 names, APIs, health model, or implementation details.

## Always follow these rules

- Work only in the INA228 repository.
- Keep the core framework-neutral: no Arduino, Wire, ESP-IDF, FreeRTOS, logging framework, heap-heavy framework types, or platform delays in `include/` or `src/`.
- The core must not own the I2C bus. Bus creation, locking, recovery, and platform handles belong to injected transports or examples.
- Preserve precise transport errors where possible: address NACK, data NACK, timeout, bus error, generic I2C error.
- No hardware-validation claims unless hardware was actually run and logs are captured.
- No pure ESP-IDF readiness claim unless `idf.py` builds or CI prove it.
- Do not hide uncertainty. If something is not tested, say so.
- Use subagents where available and have them report factual findings before implementation.
- Keep each chunk focused. Do not start later chunks early unless a small dependency is unavoidable.
- At the end of each chunk, run relevant checks, update the progress report, commit, and push/sync if available.

## Standard end-of-chunk checks

Run what is available and relevant. Do not invent results.

```bash
git status --short
python tools/check_core_timing_guard.py
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python scripts/generate_version.py check
python -m platformio test -e native
python -m platformio run -e esp32s3dev
python -m platformio run -e esp32s2dev
python -m platformio pkg pack
```

If ESP-IDF is available, also run:

```bash
idf.py --version
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

If a command does not exist or fails due to environment/toolchain, report the exact failure and continue only when safe.

## Standard progress report

Maintain or create:

```text
docs/INA228_INDUSTRY_HARDENING_PROGRESS.md
```

For every chunk append: chunk number/title, commit hash if committed, what changed, API changes, tests added/updated, commands run/results, commands not run and why, remaining risks.


# Chunk 10 — Documentation, High-Voltage Safety, and Hardware Validation Matrix

## Goal

Make documentation match the hardened behavior and prevent dangerous overclaims. INA228 is an 85 V monitor; the library must not imply that code alone makes high-voltage systems safe.

This chunk covers H7, README honesty, Doxygen contracts, and validation matrix creation.

## Subagents

Spawn:

1. `safety-doc-agent`: draft high-voltage, shunt, layout, and measurement-safety docs.
2. `api-doc-agent`: audit README/Doxygen against actual APIs after chunks 02-09.
3. `validation-agent`: create hardware validation matrix with exact commands/procedures.
4. `release-honesty-agent`: remove overclaims such as "production-grade" unless fully supported.

## Documentation requirements

Update:

- `README.md`;
- public header Doxygen comments;
- `docs/IDF_PORT.md` / equivalent;
- `docs/INA228_HARDWARE_VALIDATION_MATRIX.md`;
- progress report;
- changelog or release notes if repository style uses it.

## Required content

### High-voltage safety

Add prominent warnings:

- INA228 supports high common-mode/bus voltage, but the library and example board do not make a system safe.
- Users must understand isolation, creepage/clearance, fusing, grounding, USB-ground hazards, transients, shunt dissipation, and qualified handling.
- Never connect unsafe voltages to development boards or USB-connected PCs without proper isolation/protection.

### Shunt and calibration guidance

Document shunt selection, shunt power dissipation, Kelvin connections, current LSB, max expected current, SHUNT_CAL, ADCRANGE, actual vs requested calibration, and uncalibrated behavior.

### Measurement validity

Document continuous vs triggered modes, latest vs fresh, ENERGY/CHARGE valid modes, overflow flags, accumulator reset, `DIAG_ALRT` destructive reads, alert config vs live status, threshold units and invalidation when calibration/range changes.

### API contracts

Document framework-neutral core, injected transport, no bus ownership, timing behavior, `nowMs`/tick contract, thread/ISR safety, copy/move deletion, raw register API warnings, and status/error mapping.

### Hardware validation matrix

Create `docs/INA228_HARDWARE_VALIDATION_MATRIX.md` with commit, board, INA228 module, shunt value/tolerance, test equipment, supply/load, bus voltage range, address straps, framework, command/log path, pass/fail, notes.

Required validation rows: address scan 0x40-0x4F, identity read, MEMSTAT, bus voltage known-source, shunt voltage known-source, current with known shunt/load, negative/bidirectional current if possible, ADCRANGE 0/1, conversion timing/averaging, continuous mode, triggered mode, ENERGY/CHARGE accumulation, alerts, NACK/unplug/replug, brownout/reset, 24-72h soak, Arduino S2/S3, pure ESP-IDF S2/S3.

If not run, mark `NOT RUN`.

### Release honesty

Allowed wording: "industry-readiness hardened", "pre-production candidate pending hardware validation", "not fully field-proven".

Forbidden without proof: "fully industry-grade", "field-proven", "85 V safe", "validated on ESP-IDF".

## Checks and commit

Run docs/guard/native checks, update progress report, then commit:

```bash
git status --short
git add .
git commit -m "docs: document INA228 safety and validation matrix"
git push
```
