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
  IN_PROGRESS,               ///< Operation started or remains active; continue its documented poll/tick API

  // I2C transport details (append-only to preserve existing values)
  I2C_NACK_ADDR,             ///< I2C address not acknowledged
  I2C_NACK_DATA,             ///< I2C data byte not acknowledged
  I2C_TIMEOUT,               ///< I2C transaction timeout
  I2C_BUS,                   ///< I2C bus error (arbitration lost, etc.)

  // Accumulator validity details (append-only to preserve existing values)
  ACCUMULATION_INVALID,      ///< ENERGY/CHARGE is not valid in the current state
  ACCUMULATION_OVERFLOW,     ///< ENERGY/CHARGE overflow evidence was observed
  HARDWARE_DIRTY,            ///< Cached config may not match device registers
  I2C_NACK_UNKNOWN_PHASE,    ///< I2C NACK with platform-unreported phase

  // Cooperative operation and verification details (append-only)
  NOT_BOUND,                 ///< No zero-I2C configuration binding exists
  CANCELLED,                 ///< Operation was cancelled by its owner
  HARDWARE_STATE_UNKNOWN,    ///< Hardware is not verified against desired state
  CONFIG_MISMATCH,           ///< Register readback did not match desired state
  RESULT_NOT_AVAILABLE,      ///< No unconsumed terminal result is available
  STALE_RESULT,              ///< Requested operation ID does not match the result
  UNSUPPORTED_REVISION,      ///< INA228 die matched, but revision is unsupported
  OPERATION_TIMEOUT          ///< External owner cancelled at its operation deadline
};

/// @brief Static string name for an error code.
/// @param err Error code to name.
/// @return Static uppercase name, or `"UNKNOWN"` for an unrecognized value.
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
         err == Err::NOT_BOUND ? "NOT_BOUND" :
         err == Err::CANCELLED ? "CANCELLED" :
         err == Err::HARDWARE_STATE_UNKNOWN ? "HARDWARE_STATE_UNKNOWN" :
         err == Err::CONFIG_MISMATCH ? "CONFIG_MISMATCH" :
         err == Err::RESULT_NOT_AVAILABLE ? "RESULT_NOT_AVAILABLE" :
         err == Err::STALE_RESULT ? "STALE_RESULT" :
         err == Err::UNSUPPORTED_REVISION ? "UNSUPPORTED_REVISION" :
         err == Err::OPERATION_TIMEOUT ? "OPERATION_TIMEOUT" :
         "UNKNOWN";
}

/// @brief Status structure returned by all fallible operations.
struct Status {
  Err code = Err::OK;          ///< Stable library status code
  int32_t detail = 0;        ///< Implementation-specific detail (e.g., I2C error code)
  const char* msg = "";      ///< Static string describing the error

  /// @brief Construct an OK status with empty detail and message fields.
  constexpr Status() = default;

  /// @brief Construct a status from explicit fields.
  /// @param c Status code.
  /// @param d Implementation-specific detail value.
  /// @param m Static-lifetime descriptive message.
  constexpr Status(Err c, int32_t d, const char* m) : code(c), detail(d), msg(m) {}

  /// @return true if operation succeeded
  constexpr bool ok() const { return code == Err::OK; }

  /// @param err Status code to compare.
  /// @return true if the status code matches @p err
  constexpr bool is(Err err) const { return code == err; }

  /// @return true if operation succeeded
  constexpr explicit operator bool() const { return ok(); }

  /// @return true if operation in progress (not a failure)
  constexpr bool inProgress() const { return code == Err::IN_PROGRESS; }

  /// @brief Create a success status.
  /// @return Status with Err::OK.
  static constexpr Status Ok() { return Status{Err::OK, 0, "OK"}; }

  /// @brief Create a non-OK status.
  /// @param err Status code.
  /// @param message Static-lifetime descriptive message.
  /// @param detailCode Optional implementation-specific detail value.
  /// @return Status populated from the supplied fields.
  static constexpr Status Error(Err err, const char* message, int32_t detailCode = 0) {
    return Status{err, detailCode, message};
  }
};

} // namespace INA228
