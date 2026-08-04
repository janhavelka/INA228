/// @file CommandTable.h
/// @brief Register addresses, bit definitions, and constants for INA228
#pragma once

#include <cstdint>

namespace INA228 {
/// @brief Register addresses, fields, masks, reset values, and scaling constants.
namespace cmd {

// ============================================================================
// Register Addresses
// ============================================================================

static constexpr uint8_t REG_CONFIG          = 0x00; ///< Configuration (16-bit, R/W, reset=0x0000)
static constexpr uint8_t REG_ADC_CONFIG      = 0x01; ///< ADC Configuration (16-bit, R/W, reset=0xFB68)
static constexpr uint8_t REG_SHUNT_CAL       = 0x02; ///< Shunt Calibration (16-bit, R/W, reset=0x1000)
static constexpr uint8_t REG_SHUNT_TEMPCO    = 0x03; ///< Shunt Temperature Coefficient (16-bit, R/W, reset=0x0000)
static constexpr uint8_t REG_VSHUNT          = 0x04; ///< Shunt Voltage (24-bit, R, reset=0x000000)
static constexpr uint8_t REG_VBUS            = 0x05; ///< Bus Voltage (24-bit, R, reset=0x000000)
static constexpr uint8_t REG_DIETEMP         = 0x06; ///< Die Temperature (16-bit, R, reset=0x0000)
static constexpr uint8_t REG_CURRENT         = 0x07; ///< Current Result (24-bit, R, reset=0x000000)
static constexpr uint8_t REG_POWER           = 0x08; ///< Power Result (24-bit, R, reset=0x000000)
static constexpr uint8_t REG_ENERGY          = 0x09; ///< Energy Result (40-bit, R, reset=0x0000000000)
static constexpr uint8_t REG_CHARGE          = 0x0A; ///< Charge Result (40-bit, R, reset=0x0000000000)
static constexpr uint8_t REG_DIAG_ALRT       = 0x0B; ///< Diagnostic Flags and Alert (16-bit, R/W, reset=0x0001)
static constexpr uint8_t REG_SOVL            = 0x0C; ///< Shunt Overvoltage Threshold (16-bit, R/W, reset=0x7FFF)
static constexpr uint8_t REG_SUVL            = 0x0D; ///< Shunt Undervoltage Threshold (16-bit, R/W, reset=0x8000)
static constexpr uint8_t REG_BOVL            = 0x0E; ///< Bus Overvoltage Threshold (16-bit, R/W, reset=0x7FFF)
static constexpr uint8_t REG_BUVL            = 0x0F; ///< Bus Undervoltage Threshold (16-bit, R/W, reset=0x0000)
static constexpr uint8_t REG_TEMP_LIMIT      = 0x10; ///< Temperature Over-Limit Threshold (16-bit, R/W, reset=0x7FFF)
static constexpr uint8_t REG_PWR_LIMIT       = 0x11; ///< Power Over-Limit Threshold (16-bit, R/W, reset=0xFFFF)
static constexpr uint8_t REG_MANUFACTURER_ID = 0x3E; ///< Manufacturer ID (16-bit, R, reset=0x5449)
static constexpr uint8_t REG_DEVICE_ID       = 0x3F; ///< Device ID (16-bit, R, reset=0x2281)

// ============================================================================
// Device Identity Constants
// ============================================================================

static constexpr uint16_t MANUFACTURER_ID    = 0x5449; ///< "TI" in ASCII
static constexpr uint16_t DEVICE_ID          = 0x2281; ///< DIEID=0x228, REV_ID=0x1
static constexpr uint16_t DEVICE_DIE_ID      = 0x0228; ///< DEVICE_ID bits 15:4
static constexpr uint16_t DEVICE_DIE_ID_MASK = 0xFFF0; ///< DEVICE_ID DIEID field
static constexpr uint16_t DEVICE_REV_ID_MASK = 0x000F; ///< DEVICE_ID REV_ID field

// ============================================================================
// CONFIG Register (0x00) Bit Definitions
// ============================================================================

static constexpr uint16_t CONFIG_RST         = 0x8000; ///< Bit 15: Reset (self-clearing)
static constexpr uint16_t CONFIG_RSTACC      = 0x4000; ///< Bit 14: Reset accumulators
static constexpr uint16_t CONFIG_TEMPCOMP    = 0x0020; ///< Bit 5: Temperature compensation enable
static constexpr uint16_t CONFIG_ADCRANGE    = 0x0010; ///< Bit 4: ADC range (0=±163.84mV, 1=±40.96mV)

static constexpr uint8_t  BIT_CONFIG_RST       = 15; ///< CONFIG.RST bit position
static constexpr uint8_t  BIT_CONFIG_RSTACC    = 14; ///< CONFIG.RSTACC bit position
static constexpr uint8_t  BIT_CONFIG_CONVDLY   = 6;  ///< CONVDLY field starts at bit 6
static constexpr uint8_t  BIT_CONFIG_TEMPCOMP  = 5;  ///< CONFIG.TEMPCOMP bit position
static constexpr uint8_t  BIT_CONFIG_ADCRANGE  = 4;  ///< CONFIG.ADCRANGE bit position

static constexpr uint16_t MASK_CONFIG_CONVDLY  = 0x3FC0; ///< Bits 13:6 (8-bit field)

// ============================================================================
// ADC_CONFIG Register (0x01) Bit Definitions
// ============================================================================

static constexpr uint8_t  BIT_ADC_MODE    = 12; ///< MODE field starts at bit 12 (4 bits)
static constexpr uint8_t  BIT_ADC_VBUSCT  = 9;  ///< VBUSCT field starts at bit 9 (3 bits)
static constexpr uint8_t  BIT_ADC_VSHCT   = 6;  ///< VSHCT field starts at bit 6 (3 bits)
static constexpr uint8_t  BIT_ADC_VTCT    = 3;  ///< VTCT field starts at bit 3 (3 bits)
static constexpr uint8_t  BIT_ADC_AVG     = 0;  ///< AVG field starts at bit 0 (3 bits)

static constexpr uint16_t MASK_ADC_MODE   = 0xF000; ///< Bits 15:12
static constexpr uint16_t MASK_ADC_VBUSCT = 0x0E00; ///< Bits 11:9
static constexpr uint16_t MASK_ADC_VSHCT  = 0x01C0; ///< Bits 8:6
static constexpr uint16_t MASK_ADC_VTCT   = 0x0038; ///< Bits 5:3
static constexpr uint16_t MASK_ADC_AVG    = 0x0007; ///< Bits 2:0

static constexpr uint16_t ADC_CONFIG_RESET = 0xFB68; ///< Default ADC_CONFIG value

// ============================================================================
// SHUNT_CAL Register (0x02) Bit Definitions
// ============================================================================

static constexpr uint16_t MASK_SHUNT_CAL  = 0x7FFF; ///< Bits 14:0 (bit 15 reserved)
static constexpr uint16_t SHUNT_CAL_RESET = 0x1000; ///< Default value

// ============================================================================
// SHUNT_TEMPCO Register (0x03) Bit Definitions
// ============================================================================

static constexpr uint16_t MASK_SHUNT_TEMPCO = 0x3FFF; ///< Bits 13:0 (bits 15:14 reserved)
static constexpr uint16_t TEMPCO_MAX        = 16383;  ///< Maximum ppm/°C value

// ============================================================================
// DIAG_ALRT Register (0x0B) Bit Definitions
// ============================================================================

static constexpr uint16_t DIAG_ALATCH    = 0x8000; ///< Bit 15: Alert latch enable
static constexpr uint16_t DIAG_CNVR      = 0x4000; ///< Bit 14: Conversion ready on ALERT pin
static constexpr uint16_t DIAG_SLOWALERT = 0x2000; ///< Bit 13: Alert on averaged value
static constexpr uint16_t DIAG_APOL      = 0x1000; ///< Bit 12: Alert polarity

static constexpr uint16_t DIAG_ENERGYOF  = 0x0800; ///< Bit 11: Energy overflow (R)
static constexpr uint16_t DIAG_CHARGEOF  = 0x0400; ///< Bit 10: Charge overflow (R)
static constexpr uint16_t DIAG_MATHOF    = 0x0200; ///< Bit 9: Math overflow (R)

static constexpr uint16_t DIAG_TMPOL     = 0x0080; ///< Bit 7: Temperature over-limit flag
static constexpr uint16_t DIAG_SHNTOL    = 0x0040; ///< Bit 6: Shunt over-voltage flag
static constexpr uint16_t DIAG_SHNTUL    = 0x0020; ///< Bit 5: Shunt under-voltage flag
static constexpr uint16_t DIAG_BUSOL     = 0x0010; ///< Bit 4: Bus over-voltage flag
static constexpr uint16_t DIAG_BUSUL     = 0x0008; ///< Bit 3: Bus under-voltage flag
static constexpr uint16_t DIAG_POL       = 0x0004; ///< Bit 2: Power over-limit flag
static constexpr uint16_t DIAG_CNVRF     = 0x0002; ///< Bit 1: Conversion ready flag
static constexpr uint16_t DIAG_MEMSTAT   = 0x0001; ///< Bit 0: Memory status (1=OK)

static constexpr uint16_t DIAG_ALRT_RESET = 0x0001; ///< Default: MEMSTAT=1
static constexpr uint16_t DIAG_CONFIG_MASK =
    DIAG_ALATCH | DIAG_CNVR | DIAG_SLOWALERT | DIAG_APOL; ///< Writable alert config bits
static constexpr uint16_t DIAG_CLEAR_ON_READ_MASK =
    DIAG_TMPOL | DIAG_SHNTOL | DIAG_SHNTUL | DIAG_BUSOL |
    DIAG_BUSUL | DIAG_POL | DIAG_CNVRF; ///< Status bits consumed by DIAG_ALRT reads

// ============================================================================
// Threshold Register Defaults
// ============================================================================

static constexpr uint16_t SOVL_RESET       = 0x7FFF; ///< SOVL reset value
static constexpr uint16_t SUVL_RESET       = 0x8000; ///< SUVL reset value
static constexpr uint16_t BOVL_RESET       = 0x7FFF; ///< BOVL reset value
static constexpr uint16_t BUVL_RESET       = 0x0000; ///< BUVL reset value
static constexpr uint16_t TEMP_LIMIT_RESET = 0x7FFF; ///< TEMP_LIMIT reset value
static constexpr uint16_t PWR_LIMIT_RESET  = 0xFFFF; ///< PWR_LIMIT reset value

// ============================================================================
// ADC LSB Constants
// ============================================================================

/// Shunt voltage LSB for ADCRANGE=0: 312.5 nV = 312.5e-9 V
static constexpr double VSHUNT_LSB_RANGE0 = 312.5e-9;
/// Shunt voltage LSB for ADCRANGE=1: 78.125 nV = 78.125e-9 V
static constexpr double VSHUNT_LSB_RANGE1 = 78.125e-9;
/// Bus voltage LSB: 195.3125 µV = 195.3125e-6 V
static constexpr double VBUS_LSB = 195.3125e-6;
/// Temperature LSB: 7.8125 m°C = 7.8125e-3 °C
static constexpr double TEMP_LSB = 7.8125e-3;
/// Power multiplier relative to CURRENT_LSB
static constexpr double POWER_COEFF = 3.2;
/// Energy multiplier relative to Power_LSB
static constexpr double ENERGY_COEFF = 16.0;

/// Shunt threshold LSB for ADCRANGE=0: 5 µV = 5e-6 V
static constexpr double SHUNT_THRESHOLD_LSB_RANGE0 = 5.0e-6;
/// Shunt threshold LSB for ADCRANGE=1: 1.25 µV = 1.25e-6 V
static constexpr double SHUNT_THRESHOLD_LSB_RANGE1 = 1.25e-6;
/// Bus threshold LSB: 3.125 mV = 3.125e-3 V
static constexpr double BUS_THRESHOLD_LSB = 3.125e-3;

/// Calibration formula constant: 13107.2 × 10^6
static constexpr double SHUNT_CAL_FACTOR = 13107.2e6;

// ============================================================================
// Conversion Time Lookup (microseconds)
// ============================================================================

/// @brief Conversion-time lookup in microseconds, indexed by ConvTime code.
static constexpr uint16_t CONV_TIME_US[] = {
  50, 84, 150, 280, 540, 1052, 2074, 4120
};

// ============================================================================
// Averaging Count Lookup
// ============================================================================

/// @brief Averaging sample-count lookup, indexed by Averaging code.
static constexpr uint16_t AVG_COUNT[] = {
  1, 4, 16, 64, 128, 256, 512, 1024
};

// ============================================================================
// Register Width Constants
// ============================================================================

static constexpr uint8_t REG_WIDTH_16 = 2;  ///< 16-bit register: 2 bytes
static constexpr uint8_t REG_WIDTH_24 = 3;  ///< 24-bit register: 3 bytes
static constexpr uint8_t REG_WIDTH_40 = 5;  ///< 40-bit register: 5 bytes

// ============================================================================
// Timing Constants
// ============================================================================

static constexpr uint32_t POR_STARTUP_US = 300;   ///< Power-on reset startup time
static constexpr uint32_t SHUTDOWN_WAKEUP_US = 60; ///< Wake-up from shutdown

} // namespace cmd
} // namespace INA228
