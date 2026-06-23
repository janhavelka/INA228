/// @file Status.h
/// @brief Error codes and status handling for INA228 driver
#pragma once

#include <cstdint>

namespace INA228 {

/// @brief Error codes for all INA228 operations.
enum class Err : uint8_t {
  OK = 0,                    ///< Operation successful
  NOT_INITIALIZED,           ///< begin() not called
  INVALID_CONFIG,            ///< Invalid configuration parameter
  I2C_ERROR,                 ///< I2C communication failure
  TIMEOUT,                   ///< Operation timed out
  INVALID_PARAM,             ///< Invalid parameter value
  DEVICE_NOT_FOUND,          ///< Device not responding on I2C bus
  DEVICE_ID_MISMATCH,        ///< Device/Manufacturer ID mismatch
  MEMORY_ERROR,              ///< MEMSTAT bit indicates NV trim checksum error
  MEASUREMENT_NOT_READY,     ///< Conversion not yet complete
  CONVERSION_NOT_READY = MEASUREMENT_NOT_READY, ///< Alias for cross-library uniformity
  MATH_OVERFLOW,             ///< Host arithmetic overflow or INA228 DIAG_ALRT.MATHOF observed
  BUSY,                      ///< Device or driver is busy
  IN_PROGRESS,               ///< Operation scheduled; call tick() to complete

  // I2C transport details (append-only to preserve existing values)
  I2C_NACK_ADDR,             ///< I2C address not acknowledged
  I2C_NACK_DATA,             ///< I2C data byte not acknowledged
  I2C_TIMEOUT,               ///< I2C transaction timeout
  I2C_BUS,                   ///< I2C bus error (arbitration lost, etc.)

  // Accumulator validity details (append-only to preserve existing values)
  ACCUMULATION_INVALID,      ///< ENERGY/CHARGE is not valid in the current state
  ACCUMULATION_OVERFLOW,     ///< ENERGY/CHARGE overflow evidence was observed
  HARDWARE_DIRTY,            ///< Cached config may not match device registers
  I2C_NACK_UNKNOWN_PHASE     ///< I2C NACK with platform-unreported phase
};

/// @brief Static string name for an error code.
constexpr const char* errName(Err err) {
  return err == Err::OK ? "OK" :
         err == Err::NOT_INITIALIZED ? "NOT_INITIALIZED" :
         err == Err::INVALID_CONFIG ? "INVALID_CONFIG" :
         err == Err::I2C_ERROR ? "I2C_ERROR" :
         err == Err::TIMEOUT ? "TIMEOUT" :
         err == Err::INVALID_PARAM ? "INVALID_PARAM" :
         err == Err::DEVICE_NOT_FOUND ? "DEVICE_NOT_FOUND" :
         err == Err::DEVICE_ID_MISMATCH ? "DEVICE_ID_MISMATCH" :
         err == Err::MEMORY_ERROR ? "MEMORY_ERROR" :
         err == Err::MEASUREMENT_NOT_READY ? "MEASUREMENT_NOT_READY" :
         err == Err::MATH_OVERFLOW ? "MATH_OVERFLOW" :
         err == Err::BUSY ? "BUSY" :
         err == Err::IN_PROGRESS ? "IN_PROGRESS" :
         err == Err::I2C_NACK_ADDR ? "I2C_NACK_ADDR" :
         err == Err::I2C_NACK_DATA ? "I2C_NACK_DATA" :
         err == Err::I2C_TIMEOUT ? "I2C_TIMEOUT" :
         err == Err::I2C_BUS ? "I2C_BUS" :
         err == Err::ACCUMULATION_INVALID ? "ACCUMULATION_INVALID" :
         err == Err::ACCUMULATION_OVERFLOW ? "ACCUMULATION_OVERFLOW" :
         err == Err::HARDWARE_DIRTY ? "HARDWARE_DIRTY" :
         err == Err::I2C_NACK_UNKNOWN_PHASE ? "I2C_NACK_UNKNOWN_PHASE" :
         "UNKNOWN";
}

/// @brief Status structure returned by all fallible operations.
struct Status {
  Err code = Err::OK;
  int32_t detail = 0;        ///< Implementation-specific detail (e.g., I2C error code)
  const char* msg = "";      ///< Static string describing the error

  constexpr Status() = default;
  constexpr Status(Err c, int32_t d, const char* m) : code(c), detail(d), msg(m) {}

  /// @return true if operation succeeded
  constexpr bool ok() const { return code == Err::OK; }

  /// @return true if the status code matches @p err
  constexpr bool is(Err err) const { return code == err; }

  /// @return true if operation succeeded
  constexpr explicit operator bool() const { return ok(); }

  /// @return true if operation in progress (not a failure)
  constexpr bool inProgress() const { return code == Err::IN_PROGRESS; }

  /// Create a success status
  static constexpr Status Ok() { return Status{Err::OK, 0, "OK"}; }

  /// Create an error status
  static constexpr Status Error(Err err, const char* message, int32_t detailCode = 0) {
    return Status{err, detailCode, message};
  }
};

} // namespace INA228
