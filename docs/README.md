# INA228 Documentation Index

This directory contains implementation notes, release evidence, hardware
validation procedures, and source-reference extracts. It is not itself a
hardware-validation claim.

## Current Merge And Release Documents

| Document | Purpose |
| --- | --- |
| `INA228_INDUSTRY_HARDENING_FINAL_REPORT.md` | Final hardening summary, API changes, evidence level, merge verdict, and release blockers. |
| `INA228_RELEASE_CHECKLIST.md` | Required gates before merge, release, or tag wording can be used. |
| `INA228_HARDWARE_VALIDATION_MATRIX.md` | Hardware validation plan and evidence index. All rows remain `NOT RUN` until dated logs are checked in. |
| `INA228_ESPIDF_BUILD_PROOF_REPORT.md` | ESP-IDF build-path review and local/CI proof status. |
| `INA228_RELEASE_PREP_REPORT.md` | Version, changelog, PlatformIO pinning, CI coverage, and package-prep notes. |
| `INA228_NO_HARDWARE_RELEASE_BLOCKERS_REPORT.md` | No-hardware release blockers and validation-tooling cleanup report. |
| `ESP_IDF_BUILD.md` | Reproducible pure ESP-IDF build commands and claim policy. |
| `IDF_PORT.md` | ESP-IDF port audit and remaining build/hardware limitations. |
| `IDF_PORT_IMPLEMENTATION.md` | Implemented ESP-IDF example/component structure and validation notes. |

## Historical Audit Trail

| Path | Purpose |
| --- | --- |
| `INA228_INDUSTRY_HARDENING_PROGRESS.md` | Chunk-by-chunk progress log. Historical entries may describe then-current findings before later fixes. |
| `INA228_INDUSTRY_READINESS_EXPLORATION.md` | Initial exploration report that drove the hardening work. Treat findings as historical unless confirmed by the final report. |
| `../prompts/` | Historical AI-coder prompt pack used to execute the hardening chunks. These prompts are not current instructions or public API documentation. |

## Reference Extracts

| Path | Purpose |
| --- | --- |
| `extracted-md/` | Curated INA228 datasheet notes used by the driver work. |
| `pdf-extracted-md/` | Rawer PDF extraction output kept for traceability. It can contain extraction artifacts and should not override the public API docs. |
| `application_notes/` | External TI application-note summaries and PDFs. They are design references, not validation evidence for this repository. |
| `INA228_datasheet.pdf` | Checked-in INA228 datasheet reference. |

## Evidence Levels

- Implemented: source, examples, docs, and CI configuration exist in the tree.
- Native-tested: fake-bus/native tests passed on a dated checkout.
- Locally built: local PlatformIO or `idf.py` build logs exist.
- CI-configured: workflow files contain the job.
- CI-verified: current workflow logs were reviewed and passed.
- Hardware-validated: dated hardware logs are checked in and matrix rows are
  moved out of `NOT RUN`.

Do not infer a stronger evidence level from a weaker one.
