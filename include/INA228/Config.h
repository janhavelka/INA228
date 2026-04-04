/// @file Config.h
/// @brief Configuration structure for INA228 driver
#pragma once

#include <cstddef>
#include <cstdint>
#include "INA228/Status.h"

namespace INA228 {

/// I2C write callback signature
/// @param addr     I2C device address (7-bit)
/// @param data     Pointer to data to write
/// @param len      Number of bytes to write
/// @param timeoutMs Maximum time to wait for completion
/// @param user     User context pointer passed through from Config
/// @return Status indicating success or failure
using I2cWriteFn = Status (*)(uint8_t addr, const uint8_t* data, size_t len,
                              uint32_t timeoutMs, void* user);

/// I2C write-then-read callback signature
/// @param addr     I2C device address (7-bit)
/// @param txData   Pointer to data to write
/// @param txLen    Number of bytes to write
/// @param rxData   Pointer to buffer for read data
/// @param rxLen    Number of bytes to read
/// @param timeoutMs Maximum time to wait for completion
/// @param user     User context pointer passed through from Config
/// @return Status indicating success or failure
using I2cWriteReadFn = Status (*)(uint8_t addr, const uint8_t* txData, size_t txLen,
                                  uint8_t* rxData, size_t rxLen, uint32_t timeoutMs,
                                  void* user);

/// Millisecond timestamp callback.
/// @param user User context pointer passed through from Config
/// @return Current monotonic milliseconds
using NowMsFn = uint32_t (*)(void* user);

/// ADC operating mode (MODE field in ADC_CONFIG register, bits 15:12)
enum class Mode : uint8_t {
  SHUTDOWN         = 0x0,  ///< ADC off
  TRIG_BUS         = 0x1,  ///< Triggered bus voltage only
  TRIG_SHUNT       = 0x2,  ///< Triggered shunt voltage only
  TRIG_SHUNT_BUS   = 0x3,  ///< Triggered shunt + bus voltage
  TRIG_TEMP        = 0x4,  ///< Triggered temperature only
  TRIG_TEMP_BUS    = 0x5,  ///< Triggered temperature + bus voltage
  TRIG_TEMP_SHUNT  = 0x6,  ///< Triggered temperature + shunt voltage
  TRIG_ALL         = 0x7,  ///< Triggered all three
  SHUTDOWN2        = 0x8,  ///< ADC off (alternate)
  CONT_BUS         = 0x9,  ///< Continuous bus voltage only
  CONT_SHUNT       = 0xA,  ///< Continuous shunt voltage only
  CONT_SHUNT_BUS   = 0xB,  ///< Continuous shunt + bus voltage
  CONT_TEMP        = 0xC,  ///< Continuous temperature only
  CONT_TEMP_BUS    = 0xD,  ///< Continuous temperature + bus voltage
  CONT_TEMP_SHUNT  = 0xE,  ///< Continuous temperature + shunt voltage
  CONT_ALL         = 0xF   ///< Continuous all three (default)
};

/// ADC conversion time (for VBUSCT, VSHCT, VTCT fields)
enum class ConvTime : uint8_t {
  US_50   = 0x0,  ///< 50 µs
  US_84   = 0x1,  ///< 84 µs
  US_150  = 0x2,  ///< 150 µs
  US_280  = 0x3,  ///< 280 µs
  US_540  = 0x4,  ///< 540 µs
  US_1052 = 0x5,  ///< 1052 µs (default)
  US_2074 = 0x6,  ///< 2074 µs
  US_4120 = 0x7   ///< 4120 µs
};

/// Averaging count (AVG field in ADC_CONFIG register)
enum class Averaging : uint8_t {
  AVG_1    = 0x0,  ///< 1 sample (default)
  AVG_4    = 0x1,  ///< 4 samples
  AVG_16   = 0x2,  ///< 16 samples
  AVG_64   = 0x3,  ///< 64 samples
  AVG_128  = 0x4,  ///< 128 samples
  AVG_256  = 0x5,  ///< 256 samples
  AVG_512  = 0x6,  ///< 512 samples
  AVG_1024 = 0x7   ///< 1024 samples
};

/// Shunt full-scale range (ADCRANGE bit in CONFIG register)
enum class AdcRange : uint8_t {
  MV_163_84 = 0,  ///< ±163.84 mV (default)
  MV_40_96  = 1   ///< ±40.96 mV (4x higher resolution)
};

/// Configuration for INA228 driver
struct Config {
  // === I2C Transport (required) ===
  I2cWriteFn i2cWrite = nullptr;         ///< I2C write function pointer
  I2cWriteReadFn i2cWriteRead = nullptr; ///< I2C write-read function pointer
  void* i2cUser = nullptr;               ///< User context for callbacks

  // === Timing Hooks (optional) ===
  NowMsFn nowMs = nullptr;               ///< Monotonic millisecond source
  void* timeUser = nullptr;              ///< User context for timing hook

  // === Device Settings ===
  uint8_t i2cAddress = 0x40;             ///< 0x40–0x4F (A0/A1 pin config)
  uint32_t i2cTimeoutMs = 50;            ///< I2C transaction timeout in ms

  // === ADC Settings ===
  Mode mode = Mode::CONT_ALL;            ///< Operating mode
  ConvTime vbusConvTime = ConvTime::US_1052;  ///< Bus voltage conversion time
  ConvTime vshuntConvTime = ConvTime::US_1052; ///< Shunt voltage conversion time
  ConvTime vtempConvTime = ConvTime::US_1052;  ///< Temperature conversion time
  Averaging averaging = Averaging::AVG_1;      ///< Number of averages
  AdcRange adcRange = AdcRange::MV_163_84;     ///< Shunt full-scale range

  // === Calibration ===
  float shuntResistanceOhm = 0.0f;      ///< Shunt resistor value in ohms (0 = uncalibrated)
  float maxExpectedCurrentA = 0.0f;      ///< Maximum expected current in amps (for CURRENT_LSB)

  // === Temperature Compensation (optional) ===
  bool tempCompEnabled = false;          ///< Enable shunt temperature compensation
  uint16_t shuntTempCoeffPpmC = 0;       ///< Shunt temp coefficient in ppm/°C (max 16383)

  // === Conversion Delay (optional) ===
  uint8_t convDelayMs2 = 0;             ///< Conversion delay in 2-ms steps (0–255 = 0–510 ms)

  // === Health Tracking ===
  uint8_t offlineThreshold = 5;          ///< Consecutive failures before OFFLINE state
};

} // namespace INA228
