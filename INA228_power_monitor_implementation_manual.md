# INA228 — Extraction Document

## 1. Source Documents

| # | Document Title | Filename | TI Literature # | Pages | Role |
|---|---------------|----------|-----------------|-------|------|
| 1 | INA228 85-V, 20-Bit, Ultra-Precise Power/Energy/Charge Monitor With I2C Interface | `INA228_datasheet.pdf` | SLYS021A (Jan 2021, Rev May 2022) | 48 | Primary datasheet |
| 2 | Digital Interfaces for Current Sensing Devices | `digital_interfaces.pdf` | SBOA203A (Jun 2017, Rev Sep 2021) | 3 | Application brief — digital interface overview |
| 3 | Integrated, Current Sensing Analog-to-Digital Converter | `integrated_current_sensing_adc.pdf` | SBOA179A (Dec 2016, Rev Sep 2021) | 4 | Application brief — integrated ADC concepts |
| 4 | Simplifying High-Voltage Sensing with Hall-Effect Current Sensors | `simplifying_high_voltage_sensing.pdf` | SSZTCY8 (Aug 2023) | 4 | Technical article — Hall-effect comparison (no INA228-specific content) |
| 5 | Using a PCB Copper Trace as a Current-Sense Shunt Resistor | `current_sense_trace_design.pdf` | SBOA533 (Jan 2022) | 20 | Application note — PCB trace shunt design (no INA228-specific content) |
| 6 | Closed Loop Constant Power Drive to Simplify Heater Element Control | `closed_loop_power_drive.pdf` | SLVAFK1 (Jan 2025) | 10 | Application note — uses INA234, not INA228 |
| 7 | Improving Power Amplifier Efficiency With Current Monitors | `improving_power_amplifier.pdf` | SBOA369B (Dec 2019, Rev Jul 2023) | 3 | Application brief — mentions INA228 for periodic calibration |
| 8 | Integrating the Current Sensing Signal Path | `integrating_current_sensing.pdf` | SBOA167B (Dec 2016, Rev Sep 2021) | 3 | Application brief — signal path integration concepts |

**Implementation-relevant caveats from secondary documents:**
- The INA228 supports I2C high-speed mode (up to 2.94 MHz) and also features an Alert pin and SMBus error resolution protocols (digital_interfaces.pdf, p2).
- The INA228 is cited as "highest accuracy" among I2C/SMBus-compatible current-sense devices with 85-V common mode, 20-bit ADC, power/energy/charge/temperature monitoring (digital_interfaces.pdf, p2).
- The INA229 is the SPI variant with the same current-sensing specifications as INA228 but with SPI interface up to 10 MHz (digital_interfaces.pdf, p2).
- For periodic calibration PA feedback applications, the INA228 (or INA226) provides integrated current monitoring and voltage monitoring in one package (improving_power_amplifier.pdf, p2).
- No INA228-specific implementation caveats were found in the remaining secondary documents; they cover general shunt design, Hall-effect sensors, and other device families.

---

## 2. Device Identity and Variants

| Field | Value | Source |
|-------|-------|--------|
| Device name | INA228 | INA228_datasheet.pdf, p1 |
| Full title | 85-V, 20-Bit, Ultra-Precise Power/Energy/Charge Monitor With I2C Interface | INA228_datasheet.pdf, p1 |
| Automotive variant | INA228-Q1 | INA228_datasheet.pdf, p1 (title mentions INA228; Q1 implied by revision note referencing "commercial data sheet") |
| SPI variant | INA229 (same current-sensing specifications, SPI interface up to 10 MHz) | digital_interfaces.pdf, p2 |
| 16-bit sibling | INA238 (85-V, 16-bit ADC, same pinout concept) | digital_interfaces.pdf, p2 |
| Package | VSSOP-10 (DGS) | INA228_datasheet.pdf, p1 |
| Body size | 3.00 mm × 3.00 mm | INA228_datasheet.pdf, p1 |
| Pin count | 10 | INA228_datasheet.pdf, p3 |
| Manufacturer ID register value | 0x5449 ("TI" in ASCII) | INA228_datasheet.pdf, p29 |
| Device ID register value | 0x2281 (DIEID = 0x228, REV_ID = 0x1) | INA228_datasheet.pdf, p29 |
| TI literature number | SLYS021A | INA228_datasheet.pdf, p1 |

---

## 3. High-Level Functional Summary

The INA228 is a high-precision digital power monitor with a 20-bit delta-sigma ADC. It measures:
- **Shunt voltage** (differential across IN+/IN−): ±163.84 mV or ±40.96 mV full-scale range
- **Bus voltage** (single-ended at VBUS pin): 0 V to 85 V
- **Die temperature**: −40 °C to +125 °C (package-limited)
- **Current** (calculated from shunt voltage and programmed calibration constant)
- **Power** (calculated from current × bus voltage)
- **Energy** (accumulated power × time, 40-bit register)
- **Charge** (accumulated current × time, 40-bit register)

The device communicates over I2C (up to 2.94 MHz high-speed mode) with 16 pin-selectable slave addresses. It features a multi-purpose open-drain ALERT pin for fault monitoring and conversion-ready notification. An integrated ±0.5% precision oscillator provides the ADC timebase and the time-count for energy/charge accumulation.

(INA228_datasheet.pdf, p1, p12)

---

## 4. Interface Summary

### I2C Interface

| Parameter | Value | Source |
|-----------|-------|--------|
| Interface type | I2C (also SMBus compatible) | INA228_datasheet.pdf, p18 |
| Bus role | Secondary (slave) only | INA228_datasheet.pdf, p18 |
| Fast mode clock range | 1 kHz to 400 kHz | INA228_datasheet.pdf, p7, p19 |
| High-speed mode clock range | 1 kHz to 2.94 MHz | INA228_datasheet.pdf, p7, p19 |
| Address pins | A0 (pin 2), A1 (pin 1) | INA228_datasheet.pdf, p3 |
| Number of selectable addresses | 16 | INA228_datasheet.pdf, p19 |
| Address pin connection options | GND, VS, SDA, SCL (4 states per pin) | INA228_datasheet.pdf, p19 |
| SDA type | Open-drain bidirectional | INA228_datasheet.pdf, p3 |
| SCL type | Digital input | INA228_datasheet.pdf, p3 |
| Packet Error Checking (PEC) | Not supported | INA228_datasheet.pdf, p20 |
| Clock stretching | Not supported | INA228_datasheet.pdf, p20 |
| SMBus Alert Response | Supported (address 0001 100 with R/W high) | INA228_datasheet.pdf, p21 |
| Byte order | MSB first | INA228_datasheet.pdf, p19 |
| Integrated spike suppression | Yes (on SDA and SCL) | INA228_datasheet.pdf, p18 |

### I2C Slave Address Table (7-bit addresses)

| A1 | A0 | Address (binary) | Address (hex) |
|----|----|-------------------|---------------|
| GND | GND | 1000000 | 0x40 |
| GND | VS | 1000001 | 0x41 |
| GND | SDA | 1000010 | 0x42 |
| GND | SCL | 1000011 | 0x43 |
| VS | GND | 1000100 | 0x44 |
| VS | VS | 1000101 | 0x45 |
| VS | SDA | 1000110 | 0x46 |
| VS | SCL | 1000111 | 0x47 |
| SDA | GND | 1001000 | 0x48 |
| SDA | VS | 1001001 | 0x49 |
| SDA | SDA | 1001010 | 0x4A |
| SDA | SCL | 1001011 | 0x4B |
| SCL | GND | 1001100 | 0x4C |
| SCL | VS | 1001101 | 0x4D |
| SCL | SDA | 1001110 | 0x4E |
| SCL | SCL | 1001111 | 0x4F |

(INA228_datasheet.pdf, p19, Table 7-2)

**Note:** The device samples A0 and A1 on every bus communication. When SDA is connected to A0 or A1, an additional hold time of 100 ns is needed on the MSB of the I2C address to ensure correct device addressing. (INA228_datasheet.pdf, p19)

### High-Speed I2C Mode Entry Sequence

1. Bus idle (both SDA and SCL high)
2. Main device generates START condition
3. Main device sends HS master code byte `00001XXX` at fast mode (≤400 kHz)
4. Device does NOT acknowledge the HS master code but recognizes it and switches internal filters to 2.94 MHz
5. Main device generates a repeated START condition
6. All subsequent communication at up to 2.94 MHz
7. A STOP condition ends HS mode and switches filters back to F/S mode
8. Use repeated STARTs (not STOPs) to maintain HS mode

(INA228_datasheet.pdf, p20–21)

### I2C Timing — Fast Mode (INA228_datasheet.pdf, p7)

| Parameter | Symbol | Min | Max | Unit |
|-----------|--------|-----|-----|------|
| Clock frequency | F(SCL) | 1 | 400 | kHz |
| Bus free time (STOP to START) | t(BUF) | 600 | — | ns |
| Hold time after repeated START | t(HDSTA) | 100 | — | ns |
| Repeated START setup time | t(SUSTA) | 100 | — | ns |
| STOP setup time | t(SUSTO) | 100 | — | ns |
| Data hold time | t(HDDAT) | 10 | 900 | ns |
| Data setup time | t(SUDAT) | 100 | — | ns |
| SCL low period | t(LOW) | 1300 | — | ns |
| SCL high period | t(HIGH) | 600 | — | ns |
| Data fall time | tF | — | 300 | ns |
| Clock fall time | tF | — | 300 | ns |
| Clock rise time | tR | — | 300 | ns |

### I2C Timing — High-Speed Mode (INA228_datasheet.pdf, p7)

| Parameter | Symbol | Min | Max | Unit |
|-----------|--------|-----|-----|------|
| Clock frequency | F(SCL) | 10 | 2940 | kHz |
| Bus free time (STOP to START) | t(BUF) | 160 | — | ns |
| Hold time after repeated START | t(HDSTA) | 100 | — | ns |
| Repeated START setup time | t(SUSTA) | 100 | — | ns |
| STOP setup time | t(SUSTO) | 100 | — | ns |
| Data hold time | t(HDDAT) | 10 | 125 | ns |
| Data setup time | t(SUDAT) | 20 | — | ns |
| SCL low period | t(LOW) | 200 | — | ns |
| SCL high period | t(HIGH) | 60 | — | ns |
| Data fall time | tF | — | 80 | ns |
| Clock fall time | tF | — | 40 | ns |
| Clock rise time | tR | — | 40 | ns |

---

## 5. Electrical and Timing Constraints Relevant to Software

### Absolute Maximum Ratings (INA228_datasheet.pdf, p3)

| Parameter | Min | Max | Unit |
|-----------|-----|-----|------|
| VS supply voltage | — | 6 | V |
| VIN+ − VIN− differential | −40 | 40 | V |
| VIN+, VIN− common-mode | −0.3 | 85 | V |
| VVBUS | −0.3 | 85 | V |
| VALERT | −0.3 | VS + 0.3 | V |
| VIO (SDA, SCL) | −0.3 | 6 | V |
| Input current into any pin | — | 5 | mA |
| Digital output current | — | 10 | mA |

### Addendum: Absolute Maximum Ratings — Missing Entries (INA228_datasheet.pdf, p3)

| Parameter | Min | Max | Unit |
|-----------|-----|-----|------|
| TJ (Junction temperature) | — | 150 | °C |
| Tstg (Storage temperature) | −65 | 150 | °C |

**Note:** Stresses beyond those listed under Absolute Maximum Ratings may cause permanent damage to the device. These are stress ratings only, which do not imply functional operation of the device at these or any other conditions beyond those indicated under Recommended Operating Conditions. Exposure to absolute-maximum-rated conditions for extended periods may affect device reliability. (INA228_datasheet.pdf, p3)

### Addendum: ESD Ratings (INA228_datasheet.pdf, p4)

| Parameter | Test Method | Value | Unit |
|-----------|-------------|-------|------|
| V(ESD) — Human Body Model (HBM) | ANSI/ESDA/JEDEC JS-001, all pins | ±2000 | V |
| V(ESD) — Charged Device Model (CDM) | JEDEC specification JESD22-C101, all pins | ±1000 | V |

- JEDEC document JEP155 states that 500-V HBM allows safe manufacturing with a standard ESD control process. (INA228_datasheet.pdf, p4)
- JEDEC document JEP157 states that 250-V CDM allows safe manufacturing with a standard ESD control process. (INA228_datasheet.pdf, p4)

### Addendum: Thermal Information (INA228_datasheet.pdf, p4)

| Thermal Metric | INA228 DGS (VSSOP) 10-Pin | Unit |
|----------------|---------------------------|------|
| RθJA (Junction-to-ambient) | 177.6 | °C/W |
| RθJC(top) (Junction-to-case, top) | 66.4 | °C/W |
| RθJB (Junction-to-board) | 99.5 | °C/W |
| ΨJT (Junction-to-top characterization) | 9.7 | °C/W |
| ΨJB (Junction-to-board characterization) | 97.6 | °C/W |
| RθJC(bot) (Junction-to-case, bottom) | N/A | °C/W |

### Recommended Operating Conditions (INA228_datasheet.pdf, p4)

| Parameter | Min | Nom | Max | Unit |
|-----------|-----|-----|-----|------|
| Common-mode input range (VCM) | −0.3 | — | 85 | V |
| Operating supply range (VS) | 2.7 | — | 5.5 | V |
| Ambient temperature (TA) | −40 | — | 125 | °C |

### Key Electrical Characteristics (INA228_datasheet.pdf, p5–6)

| Parameter | Conditions | Min | Typ | Max | Unit |
|-----------|-----------|-----|-----|-----|------|
| Shunt voltage input range (ADCRANGE=0) | — | −163.84 | — | 163.84 | mV |
| Shunt voltage input range (ADCRANGE=1) | — | −40.96 | — | 40.96 | mV |
| Bus voltage input range | — | 0 | — | 85 | V |
| CMRR | −0.3V < VCM < 85V | 154 | 170 | — | dB |
| Shunt offset voltage (VCM=48V or 0V, TCT>280µs) | — | — | ±0.3 | ±1 | µV |
| Shunt offset voltage drift | −40°C to +125°C | — | ±2 | ±10 | nV/°C |
| Shunt offset vs power supply | VS=2.7–5.5V | — | ±0.05 | ±0.5 | µV/V |
| Bus offset voltage (VBUS=20mV) | — | — | ±1 | ±2.5 | mV |
| Bus offset voltage drift | −40°C to +125°C | — | ±4 | ±20 | µV/°C |
| VBUS offset voltage vs power supply (PSRR) | VS=2.7–5.5V | — | ±0.25 | — | mV/V |
| Input bias current (IB) | VCM=85V | — | 0.1 | 2.5 | nA |
| VBUS pin input impedance (active) | — | 0.8 | 1 | 1.2 | MΩ |
| Input differential impedance | VIN+−VIN− < 164mV | — | 92 | — | kΩ |
| Shunt voltage gain error (VCM=24V) | — | — | ±0.01 | ±0.05 | % |
| Shunt voltage gain error drift | — | — | — | ±20 | ppm/°C |
| Bus voltage gain error | — | — | ±0.01 | ±0.05 | % |
| Power TME (full scale, −40–125°C) | — | — | — | ±0.5 | % |
| Energy and charge TME (full scale) | — | — | — | ±1 | % |
| ADC resolution | — | — | 20 | — | bits |
| INL | — | — | — | ±2 | m% |
| DNL | — | — | 0.2 | — | LSB |
| Internal oscillator frequency | — | — | 1 | — | MHz |
| Oscillator tolerance (25°C) | — | — | — | ±0.5 | % |
| Oscillator tolerance (−40–125°C) | — | — | — | ±1 | % |
| Temperature measurement range | −40 | — | +125 | °C |
| Temperature accuracy (25°C) | — | — | ±0.15 | ±1 | °C |
| Temperature accuracy (−40–125°C) | — | — | ±0.2 | ±2 | °C |

### Digital I/O Levels (INA228_datasheet.pdf, p6)

| Parameter | Min | Max | Unit |
|-----------|-----|-----|------|
| VIH (SDA, SCL) | 1.2 | 5.5 | V |
| VIL (SDA, SCL) | GND | 0.4 | V |
| VOL (IOL = 3 mA) | GND | 0.4 | V |
| Digital leakage input current (0 ≤ VIN ≤ VS) | −1 | 1 | µA |

### ADC LSB Step Sizes (INA228_datasheet.pdf, p5)

| Measurement | LSB Value | Condition |
|-------------|-----------|-----------|
| Shunt voltage | 312.5 nV/LSB | ADCRANGE = 0 |
| Shunt voltage | 78.125 nV/LSB | ADCRANGE = 1 |
| Bus voltage | 195.3125 µV/LSB | — |
| Temperature | 7.8125 m°C/LSB | — |

---

## 6. Power, Reset, Enable, and Startup Behavior

### Power Supply (INA228_datasheet.pdf, p5–6, p39)

| Parameter | Min | Typ | Max | Unit |
|-----------|-----|-----|-----|------|
| Supply voltage (VS) | 2.7 | — | 5.5 | V |
| Quiescent current (active, VSENSE=0) | — | 640 | 750 | µA |
| Quiescent current (active, −40–125°C) | — | — | 1.1 | mA |
| Shutdown quiescent current | — | 2.8 | 5 | µA |
| Device start-up time (POR) | — | — | 300 | µs |
| Start-up from shutdown mode | — | — | 60 | µs |

**Power-on reset (POR):** POR is asserted when VS drops below 1.26 V (typical), at which point all registers are reset to their default values. (INA228_datasheet.pdf, p18)

**Software reset:** Setting the RST bit (bit 15) in CONFIG register (0x00) to 1 generates a system reset equivalent to POR. This bit self-clears. (INA228_datasheet.pdf, p22)

**Accumulator reset:** Setting the RSTACC bit (bit 14) in CONFIG register (0x00) to 1 clears the ENERGY and CHARGE registers to 0. (INA228_datasheet.pdf, p22)

**Supply sequencing:** No special power-supply sequencing required. The common-mode input range and device supply voltage are independent; bus voltage can be present with supply off and vice-versa without damage. (INA228_datasheet.pdf, p12)

**Supply bypass:** Place 0.1 µF bypass capacitor as close as possible to VS and GND pins. Noisy or high-impedance supplies may need additional decoupling. (INA228_datasheet.pdf, p39)

**Oscillator stabilization:** On power up, the internal oscillator and ADC take ~300 µs to reach <1% error stability. Once stabilized, ADC data output is accurate to spec. (INA228_datasheet.pdf, p16)

**GND caution:** Avoid applications where the GND pin is disconnected while device is actively powered. (INA228_datasheet.pdf, p39)

---

## 7. Pin Behavior Relevant to Firmware

### Pin Map (INA228_datasheet.pdf, p3, Table 5-1)

| Pin # | Name | Type | Description |
|-------|------|------|-------------|
| 1 | A1 | Digital input | I2C address pin. Connect to GND, SCL, SDA, or VS. |
| 2 | A0 | Digital input | I2C address pin. Connect to GND, SCL, SDA, or VS. |
| 3 | ALERT | Digital output | Open-drain alert output, default active-low. |
| 4 | SDA | Digital I/O | Open-drain bidirectional I2C data. |
| 5 | SCL | Digital input | I2C clock input. |
| 6 | VS | Power | Supply, 2.7 V to 5.5 V. |
| 7 | GND | Ground | Ground reference. |
| 8 | VBUS | Analog input | Bus voltage input (0–85 V). |
| 9 | IN− | Analog input | Negative differential input (shunt resistor connection). |
| 10 | IN+ | Analog input | Positive differential input (shunt resistor connection). |

### ALERT Pin Behavior (INA228_datasheet.pdf, p16–18)

- **Type:** Open-drain output
- **Default polarity:** Active-low (APOL = 0)
- **Configurable polarity:** Active-high open-drain (APOL = 1)
- **Requires external pull-up resistor**
- **Functions reported via ALERT pin:**
  - Shunt over-voltage (overcurrent)
  - Shunt under-voltage (undercurrent)
  - Bus over-voltage
  - Bus under-voltage
  - Temperature over-limit
  - Power over-limit
  - Conversion ready (when CNVR bit enabled)
- **Fast alert response:** 75 µs (INA228_datasheet.pdf, p1)
- **Alert response timing:** For fault conditions near threshold, response is 0.5 to 1.5 conversion cycles. For faults significantly above threshold, response can be as fast as ¼ conversion time. Applications where alert timing is critical should assume 1.5 × ADC conversion time. (INA228_datasheet.pdf, p37)
- **Latch mode (ALATCH=1):** ALERT pin and flag remain active after fault clears until DIAG_ALRT register is read.
- **Transparent mode (ALATCH=0):** ALERT pin and flag reset when fault clears.
- **SMBus Alert Response:** Device responds to Alert Response address (0001100 with R/W=1) by acknowledging and sending its own address on the bus. (INA228_datasheet.pdf, p21)

### VBUS Pin (INA228_datasheet.pdf, p5)

- Input impedance (active mode): 0.8–1.2 MΩ (typ 1 MΩ)
- Leakage current (shutdown, VBUS=85V): 10 nA max
- Can be left floating if unused (layout example note, INA228_datasheet.pdf, p39)

### IN+/IN− Pins

- Input bias current: 0.1 nA typ, 2.5 nA max at VCM=85V (INA228_datasheet.pdf, p5)
- Differential input impedance (active): 92 kΩ (INA228_datasheet.pdf, p5)
- Tolerates full 0–85V common-mode even with VS off (INA228_datasheet.pdf, p39)
- For high-side: IN+ to supply side of shunt, IN− to load side
- For low-side: IN+ to load side, IN− to ground side

---

## 8. Register Map Overview

All registers are accessed via I2C. Register pointer is set by the first byte after the slave address byte (R/W low). Multi-byte registers are transmitted MSB first.

| Address | Acronym | Register Name | Width (bits) | Access | Reset Value |
|---------|---------|--------------|--------------|--------|-------------|
| 0x00 | CONFIG | Configuration | 16 | R/W | 0x0000 |
| 0x01 | ADC_CONFIG | ADC Configuration | 16 | R/W | 0xFB68 |
| 0x02 | SHUNT_CAL | Shunt Calibration | 16 | R/W | 0x1000 |
| 0x03 | SHUNT_TEMPCO | Shunt Temperature Coefficient | 16 | R/W | 0x0000 |
| 0x04 | VSHUNT | Shunt Voltage Measurement | 24 | R | 0x000000 |
| 0x05 | VBUS | Bus Voltage Measurement | 24 | R | 0x000000 |
| 0x06 | DIETEMP | Temperature Measurement | 16 | R | 0x0000 |
| 0x07 | CURRENT | Current Result | 24 | R | 0x000000 |
| 0x08 | POWER | Power Result | 24 | R | 0x000000 |
| 0x09 | ENERGY | Energy Result | 40 | R | 0x0000000000 |
| 0x0A | CHARGE | Charge Result | 40 | R | 0x0000000000 |
| 0x0B | DIAG_ALRT | Diagnostic Flags and Alert | 16 | R/W | 0x0001 |
| 0x0C | SOVL | Shunt Overvoltage Threshold | 16 | R/W | 0x7FFF |
| 0x0D | SUVL | Shunt Undervoltage Threshold | 16 | R/W | 0x8000 |
| 0x0E | BOVL | Bus Overvoltage Threshold | 16 | R/W | 0x7FFF |
| 0x0F | BUVL | Bus Undervoltage Threshold | 16 | R/W | 0x0000 |
| 0x10 | TEMP_LIMIT | Temperature Over-Limit Threshold | 16 | R/W | 0x7FFF |
| 0x11 | PWR_LIMIT | Power Over-Limit Threshold | 16 | R/W | 0xFFFF |
| 0x3E | MANUFACTURER_ID | Manufacturer ID | 16 | R | 0x5449 |
| 0x3F | DEVICE_ID | Device ID | 16 | R | 0x2281 |

(INA228_datasheet.pdf, p21, Table 7-3)

**Note:** All register locations not listed should be considered reserved; contents should not be modified. (INA228_datasheet.pdf, p21)

**Note on register widths:** 16-bit registers require 2 data bytes on I2C. 24-bit registers require 3 data bytes. 40-bit registers (ENERGY, CHARGE) require 5 data bytes. All transferred MSB first. (INA228_datasheet.pdf, p19–20)

---

## 9. Detailed Register and Bitfield Breakdown

### 9.1 CONFIG Register (0x00) — Configuration [Reset = 0x0000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15 | RST | R/W | 0 | Reset bit. Writing 1 generates system reset (same as POR). Resets all registers to defaults. Self-clears. |
| 14 | RSTACC | R/W | 0 | Reset accumulation. Writing 1 clears ENERGY and CHARGE registers to 0. |
| 13:6 | CONVDLY | R/W | 0x00 | Conversion delay for initial ADC conversion in steps of 2 ms. 0x00 = 0 s; 0x01 = 2 ms; 0xFF = 510 ms. |
| 5 | TEMPCOMP | R/W | 0 | Temperature compensation enable. 0 = disabled; 1 = enabled. |
| 4 | ADCRANGE | R/W | 0 | Shunt full-scale range. 0 = ±163.84 mV; 1 = ±40.96 mV. |
| 3:0 | RESERVED | R | 0x0 | Reserved. Always reads 0. |

(INA228_datasheet.pdf, p22, Table 7-5)

### 9.2 ADC_CONFIG Register (0x01) — ADC Configuration [Reset = 0xFB68]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15:12 | MODE | R/W | 0xF | Operating mode selection (see MODE enumeration below) |
| 11:9 | VBUSCT | R/W | 0x5 | Bus voltage conversion time (see conversion time enumeration below) |
| 8:6 | VSHCT | R/W | 0x5 | Shunt voltage conversion time (see conversion time enumeration below) |
| 5:3 | VTCT | R/W | 0x5 | Temperature conversion time (see conversion time enumeration below) |
| 2:0 | AVG | R/W | 0x0 | Averaging count (see averaging enumeration below) |

(INA228_datasheet.pdf, p23–24, Table 7-6)

**Reset value 0xFB68 decoded:** MODE=0xF (continuous all 3), VBUSCT=0x5 (1052µs), VSHCT=0x5 (1052µs), VTCT=0x5 (1052µs), AVG=0x0 (1 sample).

#### MODE Field Enumeration (bits 15:12)

| Value | Mode | Description |
|-------|------|-------------|
| 0x0 | Shutdown | ADC off |
| 0x1 | Triggered bus voltage | Single shot bus voltage only |
| 0x2 | Triggered shunt voltage | Single shot shunt voltage only |
| 0x3 | Triggered shunt + bus voltage | Single shot |
| 0x4 | Triggered temperature | Single shot temperature only |
| 0x5 | Triggered temperature + bus voltage | Single shot |
| 0x6 | Triggered temperature + shunt voltage | Single shot |
| 0x7 | Triggered all three | Single shot bus + shunt + temperature |
| 0x8 | Shutdown | ADC off (same as 0x0) |
| 0x9 | Continuous bus voltage only | |
| 0xA | Continuous shunt voltage only | |
| 0xB | Continuous shunt + bus voltage | |
| 0xC | Continuous temperature only | |
| 0xD | Continuous bus voltage + temperature | |
| 0xE | Continuous temperature + shunt voltage | |
| 0xF | Continuous all three | Bus + shunt + temperature (default) |

#### Conversion Time Enumeration (for VBUSCT, VSHCT, VTCT fields)

| Value | Conversion Time |
|-------|----------------|
| 0x0 | 50 µs |
| 0x1 | 84 µs |
| 0x2 | 150 µs |
| 0x3 | 280 µs |
| 0x4 | 540 µs |
| 0x5 | 1052 µs (default) |
| 0x6 | 2074 µs |
| 0x7 | 4120 µs |

#### Averaging Count Enumeration (AVG bits 2:0)

| Value | Number of Averages |
|-------|--------------------|
| 0x0 | 1 (default) |
| 0x1 | 4 |
| 0x2 | 16 |
| 0x3 | 64 |
| 0x4 | 128 |
| 0x5 | 256 |
| 0x6 | 512 |
| 0x7 | 1024 |

### 9.3 SHUNT_CAL Register (0x02) — Shunt Calibration [Reset = 0x1000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15 | RESERVED | R | 0 | Reserved. Always reads 0. |
| 14:0 | SHUNT_CAL | R/W | 0x1000 | Shunt calibration value. Conversion constant representing shunt resistance used to calculate current. Also sets CURRENT register resolution. Formula in Section 8.1.2. If zero, current/power/energy/charge all report zero. |

(INA228_datasheet.pdf, p24, Table 7-7)

**Calibration formula:**
```
SHUNT_CAL = 13107.2 × 10^6 × CURRENT_LSB × RSHUNT
```
For ADCRANGE = 1, multiply the result by 4.

```
CURRENT_LSB = Maximum_Expected_Current / 2^19
```

(INA228_datasheet.pdf, p30–31, Equations 2 and 3)

### 9.4 SHUNT_TEMPCO Register (0x03) — Shunt Temperature Coefficient [Reset = 0x0000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15:14 | RESERVED | R | 0 | Reserved. Always reads 0. |
| 13:0 | TEMPCO | R/W | 0x0000 | Temperature coefficient of shunt in ppm/°C. Reference: +25°C. Full scale = 16383 ppm/°C. Resolution: 1 ppm/°C/LSB. |

(INA228_datasheet.pdf, p24, Table 7-8)

**Temperature compensation formula (Equation 1, INA228_datasheet.pdf, p16):**
```
RADJ = RNOM + (RNOM × (DIETEMP − 25) × SHUNT_TEMPCO) / 10^6
```
Where:
- RNOM = nominal shunt resistance at 25°C (Ohms)
- DIETEMP = temperature from DIETEMP register (°C)
- SHUNT_TEMPCO = temperature coefficient (ppm/°C)
- Shunt is always assumed to have a **positive** temperature coefficient

**Warning:** If temperature compensation is enabled with high shunt voltage (>70% of full range), high tempco (>2000 ppm/°C), and high temperature (>100°C), the calculated current may be lower than actual because the effective resistance increases while detected shunt voltage is clamped at ADC range limit. (INA228_datasheet.pdf, p16)

### 9.5 VSHUNT Register (0x04) — Shunt Voltage Measurement [Reset = 0x000000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 23:4 | VSHUNT | R | 0x00000 | 20-bit differential voltage across shunt. Two's complement. 312.5 nV/LSB (ADCRANGE=0), 78.125 nV/LSB (ADCRANGE=1). |
| 3:0 | RESERVED | R | 0x0 | Reserved. Always reads 0. |

(INA228_datasheet.pdf, p25, Table 7-9)

**Voltage calculation:** `Shunt_Voltage [V] = VSHUNT[23:4] × LSB_value`

### 9.6 VBUS Register (0x05) — Bus Voltage Measurement [Reset = 0x000000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 23:4 | VBUS | R | 0x00000 | 20-bit bus voltage. Two's complement format but always positive. 195.3125 µV/LSB. |
| 3:0 | RESERVED | R | 0x0 | Reserved. Always reads 0. |

(INA228_datasheet.pdf, p25, Table 7-10)

**Voltage calculation:** `Bus_Voltage [V] = VBUS[23:4] × 195.3125 × 10^-6`

### 9.7 DIETEMP Register (0x06) — Temperature Measurement [Reset = 0x0000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15:0 | DIETEMP | R | 0x0000 | 16-bit die temperature. Two's complement. 7.8125 m°C/LSB. |

(INA228_datasheet.pdf, p25, Table 7-11)

**Temperature calculation:** `Temperature [°C] = DIETEMP × 7.8125 × 10^-3`

### 9.8 CURRENT Register (0x07) — Current Result [Reset = 0x000000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 23:4 | CURRENT | R | 0x00000 | 20-bit calculated current in Amperes. Two's complement. |
| 3:0 | RESERVED | R | 0x0 | Reserved. Always reads 0. |

(INA228_datasheet.pdf, p25, Table 7-12)

**Current calculation (Equation 4, INA228_datasheet.pdf, p31):**
```
Current [A] = CURRENT_LSB × CURRENT[23:4]
```

### 9.9 POWER Register (0x08) — Power Result [Reset = 0x000000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 23:0 | POWER | R | 0x000000 | 24-bit calculated power. Unsigned, positive only. |

(INA228_datasheet.pdf, p25–26, Table 7-13)

**Power calculation (Equation 5, INA228_datasheet.pdf, p31):**
```
Power [W] = 3.2 × CURRENT_LSB × POWER
```

### 9.10 ENERGY Register (0x09) — Energy Result [Reset = 0x0000000000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 39:0 | ENERGY | R | 0 | 40-bit accumulated energy in Joules. Unsigned, positive only. Rolls over to zero on overflow. |

(INA228_datasheet.pdf, p26, Table 7-14)

**Energy calculation (Equation 6, INA228_datasheet.pdf, p31):**
```
Energy [J] = 16 × 3.2 × CURRENT_LSB × ENERGY
```

**Overflow:** The ENERGYOF bit (bit 11) in DIAG_ALRT indicates overflow. Clears when ENERGY register is read. Register can be reset via RSTACC bit. (INA228_datasheet.pdf, p27)

### 9.11 CHARGE Register (0x0A) — Charge Result [Reset = 0x0000000000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 39:0 | CHARGE | R | 0 | 40-bit accumulated charge in Coulombs. Two's complement (signed). Rolls over to zero on overflow. |

(INA228_datasheet.pdf, p26, Table 7-15)

**Charge calculation (Equation 7, INA228_datasheet.pdf, p32):**
```
Charge [C] = CURRENT_LSB × CHARGE
```

**Overflow:** The CHARGEOF bit (bit 10) in DIAG_ALRT indicates overflow. Clears when CHARGE register is read. Register can be reset via RSTACC bit. (INA228_datasheet.pdf, p27)

### 9.12 DIAG_ALRT Register (0x0B) — Diagnostic Flags and Alert [Reset = 0x0001]

| Bit | Field | Type | Reset | Description |
|-----|-------|------|-------|-------------|
| 15 | ALATCH | R/W | 0 | Alert latch enable. 0 = Transparent (auto-clear when fault clears). 1 = Latched (held until DIAG_ALRT is read). |
| 14 | CNVR | R/W | 0 | Conversion ready on ALERT pin. 0 = disabled. 1 = ALERT asserts when conversion complete. |
| 13 | SLOWALERT | R/W | 0 | Alert on averaged value. 0 = comparison on non-averaged (raw ADC) value. 1 = comparison on averaged value. |
| 12 | APOL | R/W | 0 | Alert polarity. 0 = active-low (default, open-drain). 1 = inverted, active-high (open-drain). |
| 11 | ENERGYOF | R | 0 | Energy register overflow. 0 = normal. 1 = 40-bit ENERGY register overflowed. Clears when ENERGY register is read. |
| 10 | CHARGEOF | R | 0 | Charge register overflow. 0 = normal. 1 = 40-bit CHARGE register overflowed. Clears when CHARGE register is read. |
| 9 | MATHOF | R | 0 | Math overflow. 0 = normal. 1 = arithmetic overflow (current/power may be invalid). Must be manually cleared by triggering another conversion or clearing accumulators with RSTACC. |
| 8 | RESERVED | R | 0 | Reserved. Always reads 0. |
| 7 | TMPOL | R/W | 0 | Temperature over-limit flag. 0 = normal. 1 = over-temp event. When ALATCH=1, cleared by reading this register. |
| 6 | SHNTOL | R/W | 0 | Shunt over-voltage flag. 0 = normal. 1 = over-shunt-voltage event. When ALATCH=1, cleared by reading this register. |
| 5 | SHNTUL | R/W | 0 | Shunt under-voltage flag. 0 = normal. 1 = under-shunt-voltage event. When ALATCH=1, cleared by reading this register. |
| 4 | BUSOL | R/W | 0 | Bus over-voltage flag. 0 = normal. 1 = bus over-limit event. When ALATCH=1, cleared by reading this register. |
| 3 | BUSUL | R/W | 0 | Bus under-voltage flag. 0 = normal. 1 = bus under-limit event. When ALATCH=1, cleared by reading this register. |
| 2 | POL | R/W | 0 | Power over-limit flag. 0 = normal. 1 = power over-limit event. When ALATCH=1, cleared by reading this register. |
| 1 | CNVRF | R/W | 0 | Conversion ready flag. 0 = normal. 1 = conversion complete. When ALATCH=1, cleared by reading this register or starting a new triggered conversion. |
| 0 | MEMSTAT | R/W | 1 | Memory status. 0 = checksum error in trim memory. 1 = normal operation. **Should always read 1 when device is operating properly.** |

(INA228_datasheet.pdf, p26–28, Table 7-16)

**Conversion Ready Flag (CNVRF) clears under these conditions:**
1. Writing to ADC_CONFIG register (except when selecting shutdown mode), OR
2. Reading the DIAG_ALRT register

(INA228_datasheet.pdf, p14)

### 9.13 SOVL Register (0x0C) — Shunt Overvoltage Threshold [Reset = 0x7FFF]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15:0 | SOVL | R/W | 0x7FFF | Shunt overvoltage threshold. Two's complement. 5 µV/LSB (ADCRANGE=0), 1.25 µV/LSB (ADCRANGE=1). |

(INA228_datasheet.pdf, p28, Table 7-17)

**Caveat:** If negative values are entered, a shunt voltage measurement of 0 V will trip this alarm. When using negative values for shunt thresholds, the overvoltage threshold must be the larger (less negative) of the two. (INA228_datasheet.pdf, p28)

### 9.14 SUVL Register (0x0D) — Shunt Undervoltage Threshold [Reset = 0x8000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15:0 | SUVL | R/W | 0x8000 | Shunt undervoltage threshold. Two's complement. 5 µV/LSB (ADCRANGE=0), 1.25 µV/LSB (ADCRANGE=1). |

(INA228_datasheet.pdf, p28, Table 7-18)

### 9.15 BOVL Register (0x0E) — Bus Overvoltage Threshold [Reset = 0x7FFF]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15 | Reserved | R | 0 | Reserved. Always reads 0. |
| 14:0 | BOVL | R/W | 0x7FFF | Bus overvoltage threshold. Unsigned, positive only. 3.125 mV/LSB. |

(INA228_datasheet.pdf, p28, Table 7-19)

### 9.16 BUVL Register (0x0F) — Bus Undervoltage Threshold [Reset = 0x0000]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15 | Reserved | R | 0 | Reserved. Always reads 0. |
| 14:0 | BUVL | R/W | 0x0000 | Bus undervoltage threshold. Unsigned, positive only. 3.125 mV/LSB. |

(INA228_datasheet.pdf, p28–29, Table 7-20)

### 9.17 TEMP_LIMIT Register (0x10) — Temperature Over-Limit Threshold [Reset = 0x7FFF]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15:0 | TOL | R/W | 0x7FFF | Temperature over-limit threshold. Two's complement. Compared directly against DIETEMP register. 7.8125 m°C/LSB. |

(INA228_datasheet.pdf, p29, Table 7-21)

### 9.18 PWR_LIMIT Register (0x11) — Power Over-Limit Threshold [Reset = 0xFFFF]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15:0 | POL | R/W | 0xFFFF | Power over-limit threshold. Unsigned, positive only. Compared directly against POWER register MSBs. Conversion factor: 256 × Power LSB. |

(INA228_datasheet.pdf, p29, Table 7-22)

**Note on power limit LSB:** The power limit register is 16 bits while the POWER register is 24 bits, so the effective LSB for the limit register is 256× the power LSB. (INA228_datasheet.pdf, p37)

### 9.19 MANUFACTURER_ID Register (0x3E) [Reset = 0x5449]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15:0 | MANFID | R | 0x5449 | Manufacturer ID. Reads "TI" in ASCII (0x54='T', 0x49='I'). |

(INA228_datasheet.pdf, p29, Table 7-23)

### 9.20 DEVICE_ID Register (0x3F) [Reset = 0x2281]

| Bits | Field | Type | Reset | Description |
|------|-------|------|-------|-------------|
| 15:4 | DIEID | R | 0x228 | Device identification: 0x228. |
| 3:0 | REV_ID | R | 0x1 | Device revision identification: 0x1. |

(INA228_datasheet.pdf, p29, Table 7-24)

---

## 10. Commands and Transaction-Level Behaviors

### Write Transaction (16-bit register example)
1. START
2. Slave address byte (7-bit address + W bit = 0) → ACK from device
3. Register pointer byte (8 bits) → ACK from device
4. Data MSByte → ACK from device
5. Data LSByte → ACK from device
6. STOP (or repeated START)

(INA228_datasheet.pdf, p20, Figure 7-7)

### Read Transaction (16-bit register example)
If the register pointer already points to the desired register, the pointer-set phase can be skipped.

**Setting register pointer:**
1. START
2. Slave address byte (7-bit address + W bit = 0) → ACK from device
3. Register pointer byte → ACK from device
4. STOP (or proceed to read phase with repeated START)

**Reading data:**
1. START (or repeated START)
2. Slave address byte (7-bit address + R bit = 1) → ACK from device
3. Data MSByte (from device) → ACK from master
4. Data LSByte (from device) → NACK from master (or ACK)
5. STOP (or repeated START)

(INA228_datasheet.pdf, p19–20, Figures 7-8 and 7-9)

### Multi-Byte Register Access
- 24-bit registers (VSHUNT, VBUS, CURRENT, POWER): 3 data bytes, MSB first
- 40-bit registers (ENERGY, CHARGE): 5 data bytes, MSB first
- 16-bit registers: 2 data bytes, MSB first

### Register Pointer Persistence
The device retains the register pointer value until changed by the next write operation. Repeated reads from the same register do not require resending the register pointer. (INA228_datasheet.pdf, p20)

### Transaction Notes
- The device does **not** support Packet Error Checking (PEC). (INA228_datasheet.pdf, p20)
- The device does **not** perform clock stretching. (INA228_datasheet.pdf, p20)
- All data bytes are transmitted MSB first following SMBus 3.0 transfer protocol. (INA228_datasheet.pdf, p19)

---

## 11. Initialization and Configuration Sequences

### Recommended Power-On Initialization Sequence

1. **Wait for POR completion:** Wait at least 300 µs after VS is applied for oscillator and ADC stabilization. (INA228_datasheet.pdf, p6, p16)

2. **Verify device identity (optional but recommended):**
   - Read MANUFACTURER_ID (0x3E) — expect 0x5449
   - Read DEVICE_ID (0x3F) — expect 0x2281

3. **Check memory status:** Read DIAG_ALRT (0x0B), verify MEMSTAT (bit 0) = 1 for normal operation. (INA228_datasheet.pdf, p28)

4. **Configure CONFIG register (0x00):**
   - Set ADCRANGE (bit 4): 0 for ±163.84 mV, 1 for ±40.96 mV
   - Set CONVDLY (bits 13:6) if synchronization delay needed (0–510 ms in 2 ms steps)
   - Set TEMPCOMP (bit 5) if using temperature compensation
   - Do NOT set RST or RSTACC unless intentional

5. **Configure ADC_CONFIG register (0x01):**
   - Set MODE (bits 15:12) to desired operating mode
   - Set VBUSCT, VSHCT, VTCT (bits 11:3) to desired conversion times
   - Set AVG (bits 2:0) to desired averaging count
   - **Note:** Writing ADC_CONFIG clears the CNVRF flag and restarts conversions (except shutdown mode). (INA228_datasheet.pdf, p14)

6. **Program SHUNT_CAL register (0x02):**
   - Calculate: `SHUNT_CAL = 13107.2 × 10^6 × CURRENT_LSB × RSHUNT`
   - For ADCRANGE = 1, multiply by 4
   - **Must be programmed at every VS power cycle.** Failure to set results in zero for current/power/energy/charge. (INA228_datasheet.pdf, p36)

7. **Program SHUNT_TEMPCO register (0x03) if TEMPCOMP is enabled:**
   - Write shunt temperature coefficient in ppm/°C (1 LSB = 1 ppm/°C)

8. **Set alert thresholds (if using alerts):**
   - Write SOVL (0x0C), SUVL (0x0D), BOVL (0x0E), BUVL (0x0F), TEMP_LIMIT (0x10), PWR_LIMIT (0x11)
   - **All threshold registers reset to defaults on VS power cycle and must be reprogrammed.** (INA228_datasheet.pdf, p37)

9. **Configure DIAG_ALRT register (0x0B) alert control bits:**
   - ALATCH (bit 15): latch mode
   - CNVR (bit 14): conversion ready on ALERT pin
   - SLOWALERT (bit 13): compare on averaged value
   - APOL (bit 12): alert polarity

### Re-Initialization After Software Reset (RST bit)
Same sequence as power-on, but POR wait may be shorter (device is already powered). All registers are reset to defaults. (INA228_datasheet.pdf, p18, p22)

---

## 12. Operating Modes and State Machine Behavior

### Three Operating Modes (selected by MODE bits in ADC_CONFIG)

#### 1. Continuous Conversion Mode (MODE = 0x9 through 0xF)
- The ADC continuously converts the selected measurement inputs in a loop.
- When averaging is used, the sequence repeats until the averaging count is reached before outputting final values.
- Output registers are updated only after all averaging is complete (if averaging > 1).
- ENERGY and CHARGE accumulation is valid and continuously updated.
- **Default mode on power-up:** MODE = 0xF (continuous bus + shunt + temperature).

#### 2. Triggered (One-Shot) Conversion Mode (MODE = 0x1 through 0x7)
- A single conversion cycle of the selected inputs is performed.
- After conversion completes, device enters shutdown mode.
- Each new triggered conversion requires writing the MODE bits again.
- **ENERGY and CHARGE registers are invalid in triggered mode** because the device does not track elapsed time. (INA228_datasheet.pdf, p14)
- Writing the MODE bits will interrupt and restart any triggered or continuous conversion in progress. (INA228_datasheet.pdf, p14)

#### 3. Shutdown Mode (MODE = 0x0 or 0x8)
- Quiescent current drops to < 5 µA (max). (INA228_datasheet.pdf, p5)
- Current into device inputs is turned off. (INA228_datasheet.pdf, p18)
- Registers can still be read and written via I2C. (INA228_datasheet.pdf, p18)
- Device remains in shutdown until triggered or continuous conversion command is received.
- Active I2C clock/data activity increases shutdown current as a function of bus frequency. (INA228_datasheet.pdf, p18, Figure 6-23)
- Wake-up from shutdown: ~60 µs. (INA228_datasheet.pdf, p6)

### Conversion Sequencing

Enabled measurement inputs are converted **sequentially** through the internal multiplexer: shunt voltage → bus voltage → temperature (order determined by which are enabled). (INA228_datasheet.pdf, p12, p14)

**Total conversion time** (single conversion, no averaging):
```
T_total = T_VSHCT (if shunt enabled) + T_VBUSCT (if bus enabled) + T_VTCT (if temp enabled)
```

**Total conversion time** (with averaging):
```
T_total = AVG × [T_VSHCT (if enabled) + T_VBUSCT (if enabled) + T_VTCT (if enabled)]
```

### Conversion Delay (CONVDLY)
- Programmable delay before the first ADC conversion in a cycle.
- Range: 0 to 510 ms in 2 ms steps.
- Default: 0 (no delay).
- Useful for synchronizing multiple devices. Note: synchronization will drift over time due to oscillator mismatch. (INA228_datasheet.pdf, p14)

### Data Validity
- Without averaging: registers are updated immediately after each conversion. (INA228_datasheet.pdf, p13)
- With averaging: intermediate values are stored in accumulator; output registers are only updated after all averages are collected. Values in output registers remain valid until replaced by next completed conversion. (INA228_datasheet.pdf, p14)
- Reading data output registers does not affect a conversion in progress. (INA228_datasheet.pdf, p14)

---

## 13. Measurement / Data Path Behavior

### Internal Measurement Architecture

The INA228 has a single 20-bit delta-sigma ADC with an internal MUX that selects between:
1. Shunt voltage (differential: IN+ − IN−)
2. Bus voltage (single-ended: VBUS to GND)
3. Die temperature (internal PTAT sensor)

(INA228_datasheet.pdf, p12, Figure 7-1)

### Calculation Engine (INA228_datasheet.pdf, p13)

- **Current** is calculated after a shunt voltage measurement.
- **Charge** is accumulated after current is calculated.
- **Power** is calculated after a bus voltage measurement, using the previously-calculated current and the latest bus voltage.
- **Energy** is accumulated after power is calculated.
- If SHUNT_CAL = 0, power, energy, and charge all report zero. (INA228_datasheet.pdf, p13)
- All calculations are performed in the background and do not add to overall conversion time. (INA228_datasheet.pdf, p13)

### Data Formats

| Register | Width | Format | Sign |
|----------|-------|--------|------|
| VSHUNT | 20-bit (in 24-bit register, bits 23:4) | Two's complement | Signed |
| VBUS | 20-bit (in 24-bit register, bits 23:4) | Two's complement (always positive) | Unsigned effectively |
| DIETEMP | 16-bit | Two's complement | Signed |
| CURRENT | 20-bit (in 24-bit register, bits 23:4) | Two's complement | Signed |
| POWER | 24-bit | Unsigned | Positive only |
| ENERGY | 40-bit | Unsigned | Positive only |
| CHARGE | 40-bit | Two's complement | Signed |

### Conversion Formulas Summary

| Measurement | Formula | Source |
|-------------|---------|--------|
| Shunt voltage | `V_shunt = VSHUNT[23:4] × 312.5 nV` (ADCRANGE=0) or `× 78.125 nV` (ADCRANGE=1) | datasheet p5, p25 |
| Bus voltage | `V_bus = VBUS[23:4] × 195.3125 µV` | datasheet p5, p25 |
| Temperature | `T = DIETEMP × 7.8125 m°C` | datasheet p5, p25 |
| CURRENT_LSB | `CURRENT_LSB = Max_Expected_Current / 2^19` | datasheet p31, Eq3 |
| SHUNT_CAL | `SHUNT_CAL = 13107.2 × 10^6 × CURRENT_LSB × RSHUNT` (×4 if ADCRANGE=1) | datasheet p31, Eq2 |
| Current | `I [A] = CURRENT_LSB × CURRENT[23:4]` | datasheet p31, Eq4 |
| Power | `P [W] = 3.2 × CURRENT_LSB × POWER` | datasheet p31, Eq5 |
| Energy | `E [J] = 16 × 3.2 × CURRENT_LSB × ENERGY` | datasheet p31, Eq6 |
| Charge | `Q [C] = CURRENT_LSB × CHARGE` | datasheet p32, Eq7 |

### Alert Threshold Conversion Formulas

| Threshold Register | LSB Size | Format |
|-------------------|----------|--------|
| SOVL (shunt OV) | 5 µV/LSB (ADCRANGE=0), 1.25 µV/LSB (ADCRANGE=1) | Two's complement |
| SUVL (shunt UV) | 5 µV/LSB (ADCRANGE=0), 1.25 µV/LSB (ADCRANGE=1) | Two's complement |
| BOVL (bus OV) | 3.125 mV/LSB | Unsigned (15-bit, bit 15 reserved) |
| BUVL (bus UV) | 3.125 mV/LSB | Unsigned (15-bit, bit 15 reserved) |
| TEMP_LIMIT | 7.8125 m°C/LSB | Two's complement |
| PWR_LIMIT | 256 × Power_LSB | Unsigned |

**Relationship between measurement and threshold LSBs:**
- Shunt threshold LSB = measurement LSB × 16 (20-bit data → 16-bit threshold: `312.5 nV × 16 = 5 µV`)
- Bus threshold LSB = measurement LSB × 16 (`195.3125 µV × 16 = 3.125 mV`)
- Power threshold LSB = Power_LSB × 256 (24-bit data → 16-bit threshold)

### Energy and Charge Accumulation Behavior

- Energy and charge are accumulated for **each conversion cycle** (not per average). (INA228_datasheet.pdf, p13)
- The INA228 averaging function is **not applied** to energy and charge. (INA228_datasheet.pdf, p13)
- On overflow, ENERGY and CHARGE registers roll over to zero. (INA228_datasheet.pdf, p32)
- ENERGYOF / CHARGEOF bits in DIAG_ALRT indicate overflow; they clear when the respective register is read. (INA228_datasheet.pdf, p27)
- RSTACC bit in CONFIG register resets both ENERGY and CHARGE to 0. (INA228_datasheet.pdf, p22)
- **Energy and charge are invalid in triggered mode** because elapsed time is not tracked. Use continuous mode for accumulation. (INA228_datasheet.pdf, p14)

### Digital Filter Behavior

- Integrated low-pass filter performs decimation and filtering on ADC output.
- Filter settles within one conversion cycle (no multi-cycle settling required).
- First amplitude notch at Nyquist: `f_NOTCH = 1 / (2 × TCT)`.
- Filter cutoff scales proportionally with output data rate.
- Transients at or near the 1 MHz sampling rate harmonics can cause problems; use external input filtering if needed. (INA228_datasheet.pdf, p14–15, p34)

### Input Filtering Recommendations (INA228_datasheet.pdf, p34–35)

- Use lowest possible series resistance (≤100 Ω) with ceramic capacitor (0.1 µF to 1 µF).
- Do NOT use >100 Ω for filter resistors — this degrades gain error and increases non-linearity.
- 10 Ω series resistors on each input protect against dV/dt failure up to 40 V differential.

### Two's Complement Conversion Example (INA228_datasheet.pdf, p37)

For negative shunt voltage reading `1011 0100 0001 0000 0000`:
1. MSB = 1 → negative value
2. Invert all bits: `0100 1011 1111 0000 0000`
3. Add 1: `0100 1100 0000 0000 0000` → Actually the datasheet says result is 311040d = 0x4BF00
4. Multiply by LSB (312.5 nV) = 97.2 mV
5. Apply negative sign → −97.2 mV

### Addendum: Explicit LSB Relationships (INA228_datasheet.pdf, p31–32, p37)

These named LSB relationships are used throughout the datasheet but are not always stated in one place:

| LSB Name | Formula | Source |
|----------|---------|--------|
| CURRENT_LSB | Max_Expected_Current / 2^19 | Equation 3, p31 |
| Power_LSB | 3.2 × CURRENT_LSB | Equation 5, p31 |
| Energy_LSB | 16 × 3.2 × CURRENT_LSB = 16 × Power_LSB | Equation 6, p31 |
| Charge_LSB | CURRENT_LSB | Equation 7, p32 |
| PWR_LIMIT LSB | 256 × Power_LSB | p29, p37 |
| Shunt threshold LSB | Measurement LSB × 16 (5 µV for ADCRANGE=0, 1.25 µV for ADCRANGE=1) | p28, p37 |
| Bus threshold LSB | Measurement LSB × 16 = 3.125 mV | p28–29, p37 |

### Addendum: Maximum Shunt Resistor Formula (INA228_datasheet.pdf, p36, Equation 8)

```
RSHUNT < VSENSE_MAX / IMAX
```

Where:
- VSENSE_MAX = ±163.84 mV (ADCRANGE=0) or ±40.96 mV (ADCRANGE=1)
- IMAX = maximum expected current

The closest standard resistor value **smaller** than the calculated maximum should be selected. Also ensure RSHUNT can handle the power dissipated at maximum load current. (INA228_datasheet.pdf, p36)

### Addendum: Temperature Sensor Internal Range (INA228_datasheet.pdf, p30)

The internal die temperature sensor range extends from **−256 °C to +256 °C** in the digital domain, but is **limited by the package** to −40 °C to +125 °C. The 16-bit two's complement format of the DIETEMP register can represent this full raw range.

### Addendum: ADC Noise Performance / ENOB Table (INA228_datasheet.pdf, p32–34, Table 8-2)

The INA228 noise-free effective resolution (ENOB) depends on both ADC conversion time and number of averages. Typical noise-free resolution is based on measured peak-to-peak noise data.

| TCT [µs] | AVG | Output Period [ms] | ENOB (±163.84 mV, ADCRANGE=0) | ENOB (±40.96 mV, ADCRANGE=1) |
|-----------|-----|--------------------|-------------------------------|------------------------------|
| 50 | 1 | 0.05 | 12.4 | 10.4 |
| 84 | 1 | 0.084 | 12.6 | 10.4 |
| 150 | 1 | 0.15 | 13.3 | 11.4 |
| 280 | 1 | 0.28 | 13.8 | 11.8 |
| 540 | 1 | 0.54 | 14.2 | 12.4 |
| 1052 | 1 | 1.052 | 14.5 | 12.6 |
| 2074 | 1 | 2.074 | 15.3 | 13.3 |
| 4120 | 1 | 4.12 | 16.0 | 13.8 |
| 50 | 4 | 0.2 | 13.1 | 11.4 |
| 84 | 4 | 0.336 | 13.9 | 11.8 |
| 150 | 4 | 0.6 | 14.3 | 12.2 |
| 280 | 4 | 1.12 | 14.9 | 12.8 |
| 540 | 4 | 2.16 | 15.1 | 13.0 |
| 1052 | 4 | 4.208 | 15.8 | 13.8 |
| 2074 | 4 | 8.296 | 16.1 | 14.3 |
| 4120 | 4 | 16.48 | 16.5 | 14.4 |
| 50 | 16 | 0.8 | 13.9 | 12.3 |
| 84 | 16 | 1.344 | 14.7 | 12.9 |
| 150 | 16 | 2.4 | 15.1 | 13.0 |
| 280 | 16 | 4.48 | 15.8 | 13.7 |
| 540 | 16 | 8.64 | 16.3 | 14.3 |
| 1052 | 16 | 16.832 | 16.5 | 14.6 |
| 2074 | 16 | 33.184 | 17.1 | 15.3 |
| 4120 | 16 | 65.92 | 17.7 | 15.9 |
| 50 | 64 | 3.2 | 15.0 | 13.3 |
| 84 | 64 | 5.376 | 15.9 | 13.8 |
| 150 | 64 | 9.6 | 16.4 | 14.4 |
| 280 | 64 | 17.92 | 16.9 | 14.5 |
| 540 | 64 | 34.56 | 17.7 | 15.3 |
| 1052 | 64 | 67.328 | 17.7 | 15.9 |
| 2074 | 64 | 132.736 | 18.1 | 16.3 |
| 4120 | 64 | 263.68 | 18.7 | 16.5 |
| 50 | 128 | 6.4 | 15.5 | 13.4 |
| 84 | 128 | 10.752 | 16.3 | 14.3 |
| 150 | 128 | 19.2 | 16.9 | 14.7 |
| 280 | 128 | 35.84 | 17.1 | 15.2 |
| 540 | 128 | 69.12 | 18.1 | 15.9 |
| 1052 | 128 | 134.656 | 18.1 | 16.4 |
| 2074 | 128 | 265.472 | 18.7 | 16.9 |
| 4120 | 128 | 527.36 | 19.7 | 17.1 |
| 50 | 256 | 12.8 | 15.5 | 14.4 |
| 84 | 256 | 21.504 | 16.7 | 14.7 |
| 150 | 256 | 38.4 | 17.4 | 15.3 |
| 280 | 256 | 71.68 | 17.7 | 15.7 |
| 540 | 256 | 138.24 | 18.7 | 16.1 |
| 1052 | 256 | 269.312 | 18.7 | 16.7 |
| 2074 | 256 | 530.944 | 19.7 | 17.4 |
| 4120 | 256 | 1054.72 | 19.7 | 17.7 |
| 50 | 512 | 25.6 | 16.7 | 14.3 |
| 84 | 512 | 43.0 | 17.4 | 15.4 |
| 150 | 512 | 76.8 | 17.7 | 15.5 |
| 280 | 512 | 143.36 | 18.7 | 16.3 |
| 540 | 512 | 276.48 | 18.7 | 16.5 |
| 1052 | 512 | 538.624 | 19.7 | 17.4 |
| 2074 | 512 | 1061.888 | 19.7 | 17.7 |
| 4120 | 512 | 2109.44 | 19.7 | 18.7 |
| 50 | 1024 | 51.2 | 17.1 | 15.0 |
| 84 | 1024 | 86.016 | 17.7 | 15.9 |
| 150 | 1024 | 153.6 | 18.1 | 16.0 |
| 280 | 1024 | 286.72 | 18.7 | 16.9 |
| 540 | 1024 | 552.96 | 19.7 | 17.1 |
| 1052 | 1024 | 1077.248 | 19.7 | 17.7 |
| 2074 | 1024 | 2123.776 | 19.7 | 18.1 |
| 4120 | 1024 | 4218.88 | 20.0 | 18.7 |

**Key observations:** Maximum achievable ENOB is 20.0 bits at 4120 µs / 1024 averages (ADCRANGE=0) or 18.7 bits (ADCRANGE=1). The fastest reasonable 16-bit precision is ~540 µs / 16 averages for ADCRANGE=0.

---

## 14. Interrupts, Alerts, Status, and Faults

### Alert Sources and Threshold Registers

| Alert | Status Bit | Bit # | Threshold Register | Default Threshold |
|-------|-----------|-------|-------------------|-------------------|
| Shunt Over-Voltage | SHNTOL | 6 | SOVL (0x0C) | 0x7FFF (max positive) |
| Shunt Under-Voltage | SHNTUL | 5 | SUVL (0x0D) | 0x8000 (max negative) |
| Bus Over-Voltage | BUSOL | 4 | BOVL (0x0E) | 0x7FFF (max positive) |
| Bus Under-Voltage | BUSUL | 3 | BUVL (0x0F) | 0x0000 (0 V) |
| Temperature Over-Limit | TMPOL | 7 | TEMP_LIMIT (0x10) | 0x7FFF |
| Power Over-Limit | POL | 2 | PWR_LIMIT (0x11) | 0xFFFF |

(INA228_datasheet.pdf, p17, Table 7-1)

### Alert Configuration Bits (in DIAG_ALRT register)

| Bit | Field | Purpose |
|-----|-------|---------|
| 15 | ALATCH | 0 = transparent (auto-clear). 1 = latched (hold until register read). |
| 14 | CNVR | 0 = conversion ready not on ALERT pin. 1 = ALERT asserts on conversion complete. |
| 13 | SLOWALERT | 0 = compare raw ADC value. 1 = compare averaged value (delayed alert). |
| 12 | APOL | 0 = active-low open-drain. 1 = active-high open-drain. |

### Status-Only Diagnostics (not routed to ALERT pin, read via DIAG_ALRT)

| Bit | Field | Description |
|-----|-------|-------------|
| 11 | ENERGYOF | Energy register overflow. Clears when ENERGY is read. |
| 10 | CHARGEOF | Charge register overflow. Clears when CHARGE is read. |
| 9 | MATHOF | Arithmetic overflow. Current/power data may be invalid. Clear by triggering conversion or RSTACC. |
| 0 | MEMSTAT | NV trim memory health. Should always be 1. If 0, checksum error detected. |

### CNVRF (Conversion Ready Flag) — Bit 1 of DIAG_ALRT

- Set to 1 when all conversions and averaging are completed.
- **Clears on:**
  1. Writing to ADC_CONFIG register (except shutdown mode selection)
  2. Reading DIAG_ALRT register
- When CNVR=1 (bit 14), ALERT pin also asserts on conversion ready.
- The CNVRF status bit can be read regardless of CNVR setting. (INA228_datasheet.pdf, p14, p17, p28)

### Multi-Alert Behavior

When CNVR=1 (conversion ready on ALERT), the ALERT pin serves as a multipurpose output. It pulses for conversion-complete events and also asserts for fault conditions. Fault events and conversion-complete events can overlap on the same pin. (INA228_datasheet.pdf, p17–18, Figure 7-6)

### Alert Timing

- **Fast alert response:** 75 µs (from feature list). (INA228_datasheet.pdf, p1)
- For fault significantly above threshold: as fast as ¼ conversion time.
- For fault just exceeding threshold: 0.5 to 1.5 conversion cycles.
- **Critical timing applications should assume 1.5 × conversion time.** (INA228_datasheet.pdf, p37)
- Variation exists because external fault is not synchronized to internal ADC conversion start. (INA228_datasheet.pdf, p37)

### Addendum: Alert Response Time Measured Values (INA228_datasheet.pdf, p37–38, Figures 8-3 and 8-4)

Measured with TCT = 50 µs, AVG = 1, SLOWALERT = 0, bus voltage only conversions:

| Fault Condition | Fault Threshold | Min Delay | Max Delay |
|-----------------|-----------------|-----------|-----------|
| Sampled values significantly above threshold | 0.2 V | 12.4 µs | 37.9 µs |
| Sampled values slightly above threshold | 1.9 V | 24 µs | 73.2 µs |

The ADC is constantly sampling, so response time for fault events starting from zero will be slower than fault events starting from values near the set fault threshold. (INA228_datasheet.pdf, p37)

### SMBus Alert Response Protocol

- Main device broadcasts Alert Response address `0001100` with R/W=1.
- Any device with active alert acknowledges and sends its own address.
- If multiple devices alert simultaneously, standard I2C bus arbitration applies.
- Losing device does not ACK and continues holding ALERT low until it wins arbitration.
(INA228_datasheet.pdf, p21)

---

## 15. Nonvolatile Memory / OTP / EEPROM Behavior

The INA228 has internal nonvolatile trim memory used for factory calibration and trimming. This memory is **not user-accessible** for writing.

**MEMSTAT bit (DIAG_ALRT bit 0):** Monitors health of device NV trim memory via checksum. Should always read 1 during normal operation. A value of 0 indicates a checksum error in the trim memory space. (INA228_datasheet.pdf, p28)

There is no user-programmable NVM, EEPROM, or OTP on this device. All configuration registers are volatile and reset to defaults on power-on or software reset.

---

## 16. Special Behaviors, Caveats, and Footnotes

1. **SHUNT_CAL = 0 disables all current-based outputs.** If SHUNT_CAL is zero, current, power, energy, and charge all report zero. Must be programmed at every power cycle. (INA228_datasheet.pdf, p13, p36)

2. **Temperature compensation warning.** With TEMPCOMP enabled, if shunt voltage exceeds ~70% of full range AND shunt tempco > 2000 ppm/°C AND temperature > 100°C, reported current will be lower than actual because effective resistance calculated keeps increasing while shunt voltage is clamped at ADC range. (INA228_datasheet.pdf, p16)

3. **Energy and charge invalid in triggered mode.** The device does not track elapsed time in triggered mode. Use continuous mode for accumulation. (INA228_datasheet.pdf, p14)

4. **Writing MODE bits restarts conversions.** Writing the MODE bits interrupts and restarts any triggered or continuous conversion in progress. (INA228_datasheet.pdf, p14)

5. **Conversion time subject to oscillator accuracy.** The ADC conversion times are subject to oscillator accuracy and drift (±0.5% at 25°C, ±1% over temperature). (INA228_datasheet.pdf, p5–6)

6. **SHUNT_CAL bit 15 is reserved.** Only bits 14:0 are writable. (INA228_datasheet.pdf, p24)

7. **VBUS register is two's complement but always positive.** The bus voltage measurement format uses two's complement but the value is always positive (0 V to 85 V). (INA228_datasheet.pdf, p25)

8. **Synchronization drift.** Even with precise internal time base, synchronization between multiple devices will be lost over time due to internal/external time base mismatch. (INA228_datasheet.pdf, p14)

9. **SDA connected to A0/A1 requires extra hold time.** When SDA is connected to A0 or A1 for address selection, add 100 ns hold time on the MSB of the I2C address. (INA228_datasheet.pdf, p19)

10. **Threshold registers reset on power cycle.** All alert limit register values reset to defaults on VS power cycle and must be reprogrammed. (INA228_datasheet.pdf, p37)

11. **Negative SOVL values trigger on 0 V.** If negative values are used in SOVL, a 0 V shunt measurement will trip the alarm. The overvoltage threshold must be less negative than the undervoltage threshold. (INA228_datasheet.pdf, p28)

12. **MATHOF must be manually cleared.** Unlike other flags, MATHOF is not auto-cleared by reading DIAG_ALRT. Clear by triggering another conversion or using RSTACC. (INA228_datasheet.pdf, p27)

13. **ENERGYOF and CHARGEOF clear on register read.** Reading the ENERGY register clears ENERGYOF; reading CHARGE clears CHARGEOF. (INA228_datasheet.pdf, p27)

14. **SLOWALERT delays alert.** When enabled, alert comparison uses the averaged value, which delays the alert by the full averaging period. (INA228_datasheet.pdf, p17)

15. **Input filter series resistors must be ≤100 Ω.** Values greater than 100 Ω degrade gain error and increase non-linearity. (INA228_datasheet.pdf, p35)

16. **No PEC, no clock stretching.** The device does not support I2C Packet Error Checking or clock stretching. (INA228_datasheet.pdf, p20)

17. **Active I2C bus increases shutdown current.** Clock and data activity during shutdown increase current consumption proportional to bus frequency. (INA228_datasheet.pdf, p18)

18. **Shunt offset spec requires TCT > 280 µs.** The ±1 µV max shunt offset voltage specification is valid only when conversion time > 280 µs. (INA228_datasheet.pdf, p5)

### Addendum: Layout Guidelines — Kelvin Connection (INA228_datasheet.pdf, p39)

Connect the input pins (IN+ and IN−) to the sensing resistor using a **Kelvin connection** (4-wire connection). This connection technique ensures that only the current-sensing resistor impedance is sensed between the input pins. Poor routing of the current-sensing resistor commonly results in additional resistance present between the input pins. Given the very low ohmic value of the current-sensing resistor, any additional high-current carrying impedance causes significant measurement errors. (INA228_datasheet.pdf, p39)

### Addendum: Overload and dV/dt Input Protection (INA228_datasheet.pdf, p34–35)

- Device inputs tolerate ±40 V differential across IN+/IN−.
- A short-to-ground on the load side of the shunt can apply full supply voltage across the shunt.
- Removing a short can cause inductive kickbacks exceeding the 40-V differential or 85-V common-mode absolute maximum ratings.
- Inductive kickback voltages are best controlled by Zener-type transient-absorbing devices (transzorbs) combined with sufficient energy storage capacitance.
- In systems without large energy storage/electrolytic capacitors, excessive dV/dt can activate the ESD protection. Adding **10 Ω series resistors** on each input protects against dV/dt failure up to the 40-V maximum differential rating. (INA228_datasheet.pdf, p34–35)

### Addendum: Typical Application Worked Design Example (INA228_datasheet.pdf, p35–37, Tables 8-3/8-4)

**Design Parameters (Table 8-3):**

| Parameter | Example Value |
|-----------|---------------|
| Power-supply voltage (VS) | 5 V |
| Bus supply rail (VCM) | 48 V |
| Bus supply rail over-voltage fault threshold | 52 V |
| Average current | 6 A |
| Overcurrent fault threshold (IMAX) | 10 A |
| ADC Range Selection (VSENSE_MAX) | ±163.84 mV |
| Temperature | 25 °C |
| Charge accumulation period | 1 hour |

**Worked calculations:**
- RSHUNT(max) = 163.84 mV / 10 A = 16.38 mΩ → select 16.2 mΩ standard value (INA228_datasheet.pdf, p36, Eq8)
- CURRENT_LSB = 10 A / 2^19 = 19.0735 µA (INA228_datasheet.pdf, p36)
- SHUNT_CAL = 13107.2 × 10^6 × 19.0735 × 10^-6 × 16.2 × 10^-3 = 4050d = 0x0FD2 (INA228_datasheet.pdf, p36)
- SOVL register for 10-A overcurrent: (10 A × 16.2 mΩ) / (312.5 nV × 16) = 162 mV / 5 µV = 32400d = 0x7E90 (INA228_datasheet.pdf, p36)
- BOVL register for 52-V bus OV: 52 V / (195.3125 µV × 16) = 52 V / 3.125 mV = 16640d = 0x4100 (INA228_datasheet.pdf, p37)

**Calculated Return Values (Table 8-4):**

| Parameter | Returned Value | LSB Value | Calculated Value |
|-----------|---------------|-----------|-----------------|
| Shunt voltage | 311040d | 312.5 nV/LSB | 0.0972 V |
| Current | 314572d | 19.073486 µA/LSB | 6 A |
| Bus voltage | 245760d | 195.3125 µV/LSB | 48 V |
| Power | 4718604d | 61.035156 µW/LSB (= CURRENT_LSB × 3.2) | 288 W |
| Energy | 1061683200d | 976.5625 µJ/LSB (= Power_LSB × 16) | 1036800 J |
| Charge | 1132462080d | 19.073486 µC/LSB (= CURRENT_LSB) | 21600 C |
| Temperature | 3200d | 7.8125 m°C/LSB | 25 °C |

(INA228_datasheet.pdf, p37)

### Addendum: Orderable Part Information (INA228_datasheet.pdf, p41)

| Orderable Part Number | Status | Package | Pins | Pkg Qty | RoHS | Lead Finish | MSL Rating | Op Temp (°C) | Part Marking |
|-----------------------|--------|---------|------|---------|------|-------------|------------|-------------|-------------|
| INA228AIDGSR | Active | VSSOP (DGS) | 10 | 2500 (Large T&R) | Yes | SN | Level-2-260C-1 YEAR | −40 to 125 | 228I |

Automotive qualified version: **INA228-Q1** (Q100 qualified for high-reliability automotive applications targeting zero defects). (INA228_datasheet.pdf, p41–42)

---

## 17. Recommended Polling and Control Strategy Hints from the Docs

1. **Use CNVRF to coordinate reads.** The Conversion Ready flag (CNVRF, DIAG_ALRT bit 1) indicates when all conversions and averaging are complete. This is particularly useful for triggered mode. (INA228_datasheet.pdf, p14)

2. **ALERT pin for interrupt-driven operation.** Configure CNVR=1 in DIAG_ALRT to assert ALERT on conversion complete, enabling interrupt-driven reads instead of polling. (INA228_datasheet.pdf, p17)

3. **Reading data registers at any time is safe.** The device can be read at any time; data from the last completed conversion remains available. Reading does not affect conversions in progress. (INA228_datasheet.pdf, p14)

4. **For energy/charge applications, use continuous mode.** Triggered mode does not track time; only continuous mode provides valid energy and charge accumulation. (INA228_datasheet.pdf, p14)

5. **RSTACC to reset accumulators.** Use CONFIG bit 14 (RSTACC) to clear ENERGY and CHARGE registers at known intervals for periodic energy accounting. (INA228_datasheet.pdf, p22)

6. **Use SLOWALERT for noisy environments.** When measurements are noisy, enable SLOWALERT so that alert comparison is performed on the averaged result, preventing false alerts from noise spikes. (INA228_datasheet.pdf, p17)

7. **Use latch mode (ALATCH=1) to prevent missing transient faults.** In transparent mode, the alert auto-clears when the fault condition clears, which could be missed between polls. Latch mode holds the alert until the DIAG_ALRT register is explicitly read. (INA228_datasheet.pdf, p17)

8. **Monitor MEMSTAT at startup.** Verify MEMSTAT=1 after power-up to ensure factory trim data integrity. (INA228_datasheet.pdf, p28)

9. **Monitor MATHOF.** After reading current or power, check MATHOF to verify no arithmetic overflow invalidated the data. (INA228_datasheet.pdf, p27)

10. **Synchronization with CONVDLY.** Use the conversion delay feature to synchronize multiple INA228 devices, but be aware of drift over time. (INA228_datasheet.pdf, p14)

11. **Round CURRENT_LSB for simpler conversion.** While the smallest CURRENT_LSB yields highest resolution, it is common to select a higher round number (up to 8× larger) to simplify software conversion. (INA228_datasheet.pdf, p30)

---

## 18. Ambiguities, Conflicts, and Missing Information

1. **RSTACC self-clear behavior not stated.** The RST bit (bit 15) is documented as self-clearing, but it is not explicitly stated whether the RSTACC bit (bit 14) also self-clears. Likely self-clears, but not confirmed. **Addendum:** On closer review, the datasheet explicitly states "This bit self-clears" ONLY for the RST bit (p22). The RSTACC description does NOT include this statement. Firmware should therefore treat RSTACC as NOT self-clearing and explicitly write it back to 0 after use, or re-read CONFIG to confirm state.

2. **DIAG_ALRT alert flag access type inconsistency.** The alert flag bits (TMPOL, SHNTOL, SHNTUL, BUSOL, BUSUL, POL, CNVRF, MEMSTAT) are listed as "R/W" in the register table but their behavior is primarily read-only status flags that are cleared by reading the register (in latch mode). The write functionality for these bits is not explained — it is unclear whether writing 0 clears them or writing 1 has any effect.

3. **Exact register pointer behavior for 24-bit and 40-bit registers not fully specified.** The I2C transaction diagrams show 16-bit examples. For 24-bit registers (3 bytes) and 40-bit registers (5 bytes), the write/read protocol is described as "behav[ing] similarly" but exact timing diagrams are not provided. (INA228_datasheet.pdf, p20)

4. **TEMP_LIMIT register reset value discrepancy.** Table 7-1 states the TEMP_LIMIT default is 0xFFFF while Table 7-21 and the register heading state 0x7FFF. The register heading value (0x7FFF) is most likely correct.

5. **PWR_LIMIT register reset value.** Table 7-1 states default 0x7FFF while Table 7-22 states 0xFFFF. The register heading (0xFFFF) is used in this document since it is the more detailed specification.

6. **INA228-Q1 automotive variant not detailed.** The datasheet references the Q1 variant through revision notes ("align with the commercial data sheet") but the Q1 variant does not appear in the reviewed text with separate specifications.

7. **Power calculation ordering dependency.** The datasheet states "Power and energy are calculated based on the previous current calculation and the latest bus voltage measurement." It is unclear exactly what happens if only bus voltage conversions are enabled (without shunt voltage) — is power calculated using a stale/zero current value?

8. **CONVDLY behavior with triggered mode.** It is not explicitly stated whether CONVDLY applies to each trigger or only the first trigger after power-up.

9. **Two's complement interpretation for VBUS.** The datasheet says VBUS is "Two's complement value, however always positive." This is potentially confusing — the actual format is simply unsigned 20-bit but the field description specifies two's complement. Firmware should treat bits 23:4 as unsigned.

10. **HS mode code bits.** The HS master code is "00001XXX" — the three X bits are not specified for INA228. Standard I2C practice allows any value for these bits (they distinguish multiple HS masters).

---

## 19. Raw Implementation Checklist

- [ ] Configure I2C master for 7-bit addressing, supporting up to 2.94 MHz (or 400 kHz fast mode)
- [ ] Determine slave address from A0/A1 pin connections (0x40–0x4F range)
- [ ] Wait ≥300 µs after power-on before first communication
- [ ] Read MANUFACTURER_ID (0x3E) → verify 0x5449
- [ ] Read DEVICE_ID (0x3F) → verify 0x2281
- [ ] Read DIAG_ALRT (0x0B) → verify MEMSTAT (bit 0) = 1
- [ ] Write CONFIG (0x00): set ADCRANGE, CONVDLY, TEMPCOMP as needed
- [ ] Write ADC_CONFIG (0x01): set MODE, VBUSCT, VSHCT, VTCT, AVG
- [ ] Calculate CURRENT_LSB = max_current / 2^19
- [ ] Calculate SHUNT_CAL = 13107.2e6 × CURRENT_LSB × R_shunt (×4 if ADCRANGE=1)
- [ ] Write SHUNT_CAL (0x02) with calculated value (must be non-zero)
- [ ] If TEMPCOMP enabled: write SHUNT_TEMPCO (0x03) with shunt ppm/°C value
- [ ] Write alert threshold registers as needed: SOVL, SUVL, BOVL, BUVL, TEMP_LIMIT, PWR_LIMIT
- [ ] Configure DIAG_ALRT (0x0B) control bits: ALATCH, CNVR, SLOWALERT, APOL
- [ ] Implement read routine for 16-bit, 24-bit, and 40-bit registers (MSB first)
- [ ] Handle two's complement sign extension for VSHUNT (20-bit signed in 24-bit), CURRENT (20-bit signed in 24-bit), DIETEMP (16-bit signed), CHARGE (40-bit signed)
- [ ] Handle unsigned format for VBUS (20-bit unsigned in 24-bit), POWER (24-bit unsigned), ENERGY (40-bit unsigned)
- [ ] Apply conversion formulas with CURRENT_LSB for current, power, energy, charge
- [ ] Monitor CNVRF flag (poll DIAG_ALRT or use ALERT pin interrupt) before reading measurement registers
- [ ] Be aware that reading DIAG_ALRT clears CNVRF and latched alert flags
- [ ] Check MATHOF after reading current/power to detect overflow
- [ ] Check ENERGYOF / CHARGEOF periodically; note these clear on reading ENERGY/CHARGE
- [ ] Implement RSTACC to periodically reset accumulators if doing periodic energy accounting
- [ ] After any VS power cycle: re-program CONFIG, ADC_CONFIG, SHUNT_CAL, SHUNT_TEMPCO, all threshold registers, and DIAG_ALRT control bits
- [ ] For HS I2C mode: send HS master code `00001XXX` at ≤400 kHz, then repeated START, then communicate at up to 2.94 MHz; STOP exits HS mode
- [ ] If using SMBus Alert Response: implement handler for Alert Response address (0x0C with R/W=1)
- [ ] If VBUS pin unused: can be left floating (per layout example)
- [ ] Keep input filter resistors ≤100 Ω to avoid gain error degradation

---

## 20. Source Citation Appendix

All facts in this document are extracted from the following primary source:

| Citation Key | Document | TI Lit # |
|-------------|----------|----------|
| INA228_datasheet.pdf | INA228 85-V, 20-Bit, Ultra-Precise Power/Energy/Charge Monitor With I2C Interface | SLYS021A (Rev A, May 2022) |
| digital_interfaces.pdf | Digital Interfaces for Current Sensing Devices | SBOA203A (Rev Sep 2021) |
| integrated_current_sensing_adc.pdf | Integrated, Current Sensing Analog-to-Digital Converter | SBOA179A (Rev Sep 2021) |
| simplifying_high_voltage_sensing.pdf | Simplifying High-Voltage Sensing with Hall-Effect Current Sensors | SSZTCY8 (Aug 2023) |
| current_sense_trace_design.pdf | Using a PCB Copper Trace as a Current-Sense Shunt Resistor | SBOA533 (Jan 2022) |
| closed_loop_power_drive.pdf | Closed Loop Constant Power Drive | SLVAFK1 (Jan 2025) |
| improving_power_amplifier.pdf | Improving Power Amplifier Efficiency With Current Monitors | SBOA369B (Rev Jul 2023) |
| integrating_current_sensing.pdf | Integrating the Current Sensing Signal Path | SBOA167B (Rev Sep 2021) |

### Page Citation Index

| Datasheet Page(s) | Content |
|-------------------|---------|
| p1 | Features, applications, description, device info, block diagram |
| p2 | Table of contents, revision history |
| p3 | Pin configuration, pin functions, absolute maximum ratings |
| p4 | ESD ratings, recommended operating conditions, thermal information |
| p5–6 | Electrical characteristics (all parameters) |
| p7 | I2C timing requirements (fast mode and high-speed mode), timing diagram |
| p8–11 | Typical characteristics (graphs) |
| p12 | Overview, functional block diagram, versatile high-voltage measurement |
| p13 | Internal measurement and calculation engine, low bias current |
| p13–14 | High-precision delta-sigma ADC, conversion modes, CNVRF behavior |
| p14–15 | Low-latency digital filter, flexible conversion times |
| p16 | Shunt resistor drift compensation (TEMPCOMP, Equation 1), precision oscillator |
| p16–18 | Multi-alert monitoring, fault detection, alert pin behavior, Figure 7-6 |
| p18 | Shutdown mode, power-on reset |
| p18–20 | I2C serial interface, address table, write/read protocols |
| p20–21 | High-speed I2C mode, SMBus Alert Response |
| p21 | Register map overview table |
| p22 | CONFIG register |
| p23–24 | ADC_CONFIG register (MODE, conversion time, averaging enumerations) |
| p24 | SHUNT_CAL register, SHUNT_TEMPCO register |
| p25 | VSHUNT, VBUS, DIETEMP, CURRENT registers |
| p25–26 | POWER register |
| p26 | ENERGY register, CHARGE register |
| p26–28 | DIAG_ALRT register (all bitfields) |
| p28 | SOVL, SUVL registers |
| p28–29 | BOVL, BUVL registers |
| p29 | TEMP_LIMIT, PWR_LIMIT, MANUFACTURER_ID, DEVICE_ID registers |
| p30–31 | Measurement range/resolution, SHUNT_CAL formula (Equations 2–5) |
| p31–32 | Energy formula (Eq 6), charge formula (Eq 7), overflow behavior |
| p32–34 | Noise performance table (ENOB vs conversion time and averaging) |
| p34–35 | Input filtering considerations, RFILTER limits |
| p35–37 | Typical application, design procedure, calculating returned values |
| p37–38 | Alert response time measurements and application curves |
| p39 | Power supply recommendations, layout guidelines, layout example |
