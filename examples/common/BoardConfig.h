/**
 * @file BoardConfig.h
 * @brief Example board configuration for ESP32-S2 / ESP32-S3 reference hardware.
 *
 * These are convenience defaults for reference designs only.
 * NOT part of the library API. Override for your hardware.
 *
 * @warning The library itself is board-agnostic. Pins are configured only by
 *          the application-owned example transport and are never passed to or
 *          configured by the core driver.
 */

#pragma once

#include <stdint.h>

#include "examples/common/I2cTransport.h"

namespace board {

// ====================================================================
// EXAMPLE DEFAULTS - ESP32-S2 / ESP32-S3 REFERENCE HARDWARE
// ====================================================================

/// @brief I2C SDA pin (data line). Example default for ESP32-S2/S3.
static constexpr int I2C_SDA = 8;

/// @brief I2C SCL pin (clock line). Example default for ESP32-S2/S3.
static constexpr int I2C_SCL = 9;

/// @brief I2C clock frequency in Hz.
static constexpr uint32_t I2C_FREQ_HZ = 400000;

/// @brief I2C timeout in milliseconds for example transactions.
static constexpr uint16_t I2C_TIMEOUT_MS = 50;

/// @brief Default INA228 7-bit I2C address used by the reference example.
static constexpr uint8_t INA228_I2C_ADDR = 0x40;

/// @brief Initialize I2C for examples using the default config.
inline bool initI2c() {
  return transport::initWire(I2C_SDA, I2C_SCL, I2C_FREQ_HZ, I2C_TIMEOUT_MS);
}

}  // namespace board
