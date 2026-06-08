# INA228 Documentation Index

This directory contains public support documentation, hardware validation
procedures, ESP-IDF notes, and source-reference extracts. It is not itself a
hardware-validation claim.

## Current Documents

| Document | Purpose |
| --- | --- |
| `INA228_RELEASE_CHECKLIST.md` | Required gates before merge, release, or tag wording can be used. |
| `INA228_HARDWARE_VALIDATION_MATRIX.md` | Hardware validation plan and evidence index. All rows remain `NOT RUN` until dated logs are checked in. |
| `ESP_IDF_BUILD.md` | Reproducible pure ESP-IDF build commands and claim policy. |
| `IDF_PORT_IMPLEMENTATION.md` | Implemented ESP-IDF example/component structure and validation notes. |
| `../tools/INA228_HIL_COMMAND_SEQUENCE.md` | CLI transcript template for repeatable hardware-in-loop validation runs. |

## Reference Extracts

| Path | Purpose |
| --- | --- |
| `extracted-md/` | Curated INA228 datasheet notes used by the driver work. These notes summarize facts from the checked-in datasheet. |
| `application_notes/` | Curated external TI application-note summaries and PDFs. They are design references, not validation evidence for this repository. |
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
