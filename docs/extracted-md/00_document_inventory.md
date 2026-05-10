# INA228 compact documentation inventory

This directory summarizes INA228 Rev. A datasheet facts needed by the driver: I2C address straps, SMBus transactions, register widths, reset values, conversion scaling, alert flags, and calibration formulas. The raw extraction archive remains in `docs/pdf-extracted-md/`.

| File | Purpose |
| --- | --- |
| `00_document_inventory.md` | Map of compact notes and source documents. |
| `01_chip_overview.md` | Device role, measurement capabilities, and driver scope. |
| `02_pinout_and_signals.md` | Package pins, bus pins, measurement inputs, ALERT, and address straps. |
| `03_electrical_and_timing.md` | Electrical limits, ADC scales, conversion timing, and I2C timing. |
| `04_protocol_commands_and_transactions.md` | I2C register pointer, reads/writes, high-speed mode, and SMBus alert response. |
| `05_register_map.md` | Driver-facing register map and conversion notes. |
| `06_modes_interrupts_status_and_faults.md` | Conversion modes, alert/fault sources, latch behavior, and overflow flags. |
| `07_initialization_reset_and_operational_notes.md` | Reset, startup, calibration, result scaling, and implementation sequence. |
| `08_variant_differences_and_open_questions.md` | INA228 variant scope plus facts not documented or ambiguous in the checked-in PDFs. |

## Source documents

| Source PDF | Raw extract | Pages used | Notes |
| --- | --- | --- | --- |
| `docs/INA228_datasheet.pdf` | `docs/pdf-extracted-md/INA228_datasheet.md` | 1, 3-7, 12-21, 24-30, 35-37 | Primary source for all compact notes. |
| Supplemental current-sense application notes | `docs/pdf-extracted-md/*.md` except the datasheet | Not used for register facts | Board-layout and sensing context only; no INA228 register defaults or I2C command facts were taken from these notes. |
