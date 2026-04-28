/// @file INA228.h
/// @brief Main driver class for INA228 85-V, 20-bit power/energy/charge monitor
#pragma once

#include <cstddef>
#include <cstdint>
#include "INA228/Status.h"
#include "INA228/Config.h"
#include "INA228/CommandTable.h"
#include "INA228/Version.h"

namespace INA228 {

/// Driver state for health monitoring
enum class DriverState : uint8_t {
  UNINIT,    ///< begin() not called or end() called
  READY,     ///< Operational, consecutiveFailures == 0
  DEGRADED,  ///< 1 <= consecutiveFailures < offlineThreshold
  OFFLINE    ///< consecutiveFailures >= offlineThreshold
};

/// Measurement result (float, all channels)
struct Measurement {
  float shuntVoltageV = 0.0f;  ///< Shunt voltage in volts
  float busVoltageV = 0.0f;    ///< Bus voltage in volts
  float temperatureC = 0.0f;   ///< Die temperature in Celsius
  float currentA = 0.0f;       ///< Current in amperes
  float powerW = 0.0f;         ///< Power in watts
  double energyJ = 0.0;        ///< Accumulated energy in joules
  double chargeC = 0.0;        ///< Accumulated charge in coulombs
};

/// Raw register values
struct RawSample {
  int32_t vshunt = 0;   ///< Raw shunt voltage (20-bit, signed, bits 23:4)
  uint32_t vbus = 0;    ///< Raw bus voltage (20-bit, unsigned, bits 23:4)
  int16_t dietemp = 0;  ///< Raw die temperature (16-bit, signed)
  int32_t current = 0;  ///< Raw current (20-bit, signed, bits 23:4)
  uint32_t power = 0;   ///< Raw power (24-bit, unsigned)
  uint64_t energy = 0;  ///< Raw energy (40-bit, unsigned)
  int64_t charge = 0;   ///< Raw charge (40-bit, signed)
};

/// Diagnostic and alert flags from DIAG_ALRT register
struct DiagAlert {
  bool alatch = false;     ///< Alert latch enabled
  bool cnvr = false;       ///< Conversion ready on ALERT pin
  bool slowAlert = false;  ///< Alert on averaged value
  bool apol = false;       ///< Alert polarity (true = active-high)
  bool energyOF = false;   ///< Energy register overflowed
  bool chargeOF = false;   ///< Charge register overflowed
  bool mathOF = false;     ///< Arithmetic overflow
  bool tmpOL = false;      ///< Temperature over-limit
  bool shntOL = false;     ///< Shunt over-voltage
  bool shntUL = false;     ///< Shunt under-voltage
  bool busOL = false;      ///< Bus over-voltage
  bool busUL = false;      ///< Bus under-voltage
  bool pOL = false;        ///< Power over-limit
  bool cnvrf = false;      ///< Conversion ready flag
  bool memstat = false;    ///< Memory status (true = OK)
};

/// INA228 driver class
class INA228 {
public:
  // =========================================================================
  // Lifecycle
  // =========================================================================

  /// Initialize the driver with configuration
  /// @param config Configuration including transport callbacks and calibration
  /// @return Status::Ok() on success, error otherwise
  Status begin(const Config& config);

  /// Process pending operations (call regularly from loop)
  /// @param nowMs Current timestamp in milliseconds
  void tick(uint32_t nowMs);

  /// Shutdown the driver and release resources
  void end();

  /// Check if begin() completed successfully and end() has not been called
  bool isInitialized() const { return _initialized; }

  /// Get the cached configuration snapshot currently owned by the driver
  const Config& getConfig() const { return _config; }

  // =========================================================================
  // Diagnostics
  // =========================================================================

  /// Check if device is present on the bus (no health tracking)
  /// @return Status::Ok() if device responds with correct IDs
  Status probe();

  /// Attempt to recover from DEGRADED/OFFLINE state by re-validating IDs, MEMSTAT, and cached config
  /// @return Status::Ok() if device now responsive and configuration is re-applied, error otherwise
  Status recover();

  // =========================================================================
  // Driver State
  // =========================================================================

  /// Get current driver state
  DriverState state() const { return _driverState; }

  /// Check if driver is ready for operations
  bool isOnline() const {
    return _driverState == DriverState::READY ||
           _driverState == DriverState::DEGRADED;
  }

  // =========================================================================
  // Health Tracking
  // =========================================================================

  /// Timestamp of last successful I2C operation
  uint32_t lastOkMs() const { return _lastOkMs; }

  /// Timestamp of last failed I2C operation
  uint32_t lastErrorMs() const { return _lastErrorMs; }

  /// Most recent error status
  Status lastError() const { return _lastError; }

  /// Consecutive failures since last success
  uint8_t consecutiveFailures() const { return _consecutiveFailures; }

  /// Total failure count (lifetime)
  uint32_t totalFailures() const { return _totalFailures; }

  /// Total success count (lifetime)
  uint32_t totalSuccess() const { return _totalSuccess; }

  // =========================================================================
  // Measurement API
  // =========================================================================

  /// Read all available measurements into a float structure
  /// Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @param out Measurement result
  /// @return Status::Ok() on success
  Status readMeasurement(Measurement& out);

  /// Read raw register values for all channels
  /// Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @param out RawSample structure
  /// @return Status::Ok() on success
  Status readRawSample(RawSample& out);

  /// Read shunt voltage in volts
  /// @param out Shunt voltage (V)
  /// @return Status::Ok() on success
  Status readShuntVoltage(float& out);

  /// Read bus voltage in volts
  /// @param out Bus voltage (V)
  /// @return Status::Ok() on success
  Status readBusVoltage(float& out);

  /// Read die temperature in Celsius
  /// @param out Temperature (°C)
  /// @return Status::Ok() on success
  Status readTemperature(float& out);

  /// Read calculated current in amperes (requires calibration)
  /// @param out Current (A)
  /// @return Status::Ok() on success
  Status readCurrent(float& out);

  /// Read calculated power in watts (requires calibration)
  /// @param out Power (W)
  /// @return Status::Ok() on success
  Status readPower(float& out);

  /// Read accumulated energy in joules (requires calibration, continuous mode)
  /// @param out Energy (J)
  /// @return Status::Ok() on success
  Status readEnergy(double& out);

  /// Read accumulated charge in coulombs (requires calibration, continuous mode)
  /// @param out Charge (C)
  /// @return Status::Ok() on success
  Status readCharge(double& out);

  /// Check if conversion is ready
  /// @param ready Set to true if conversion complete
  /// @return Status::Ok() on success
  Status isConversionReady(bool& ready);

  // =========================================================================
  // Configuration
  // =========================================================================

  /// Set operating mode
  /// @param mode New operating mode
  /// @return Status::Ok() on success
  Status setMode(Mode mode);

  /// Get current operating mode
  Status getMode(Mode& out) const;

  /// Trigger a one-shot conversion with specified inputs
  /// Writing the MODE bits restarts any conversion in progress.
  /// @param mode One of TRIG_* modes
  /// @return Err::IN_PROGRESS when the conversion was started
  Status triggerConversion(Mode mode);

  /// Set bus voltage conversion time
  Status setVbusConvTime(ConvTime ct);

  /// Set shunt voltage conversion time
  Status setVshuntConvTime(ConvTime ct);

  /// Set temperature conversion time
  Status setTempConvTime(ConvTime ct);

  /// Set averaging count
  Status setAveraging(Averaging avg);

  /// Set shunt ADC range
  /// @note Changing range requires recalibration (SHUNT_CAL is recomputed)
  Status setAdcRange(AdcRange range);

  /// Update shunt calibration (SHUNT_CAL register)
  /// Cache and scaling update only after the register write succeeds. If the
  /// computed SHUNT_CAL value clamps, currentLsb() reflects the clamped value.
  /// @param shuntOhm Shunt resistance in ohms
  /// @param maxCurrentA Maximum expected current in amps
  /// @return Status::Ok() on success
  Status setCalibration(float shuntOhm, float maxCurrentA);

  /// Set shunt temperature coefficient
  /// @param ppmPerC Temperature coefficient in ppm/°C (0–16383)
  /// @return Status::Ok() on success
  Status setShuntTempCoeff(uint16_t ppmPerC);

  /// Enable or disable temperature compensation
  Status setTempCompensation(bool enable);

  /// Set conversion delay
  /// @param steps2ms Delay in 2-ms steps (0–255 = 0–510 ms)
  Status setConversionDelay(uint8_t steps2ms);

  // =========================================================================
  // Alert Configuration
  // =========================================================================

  /// Read diagnostic and alert register
  Status readDiagAlert(DiagAlert& out);

  /// Read raw DIAG_ALRT register value
  Status readDiagAlertRaw(uint16_t& raw);

  /// Configure alert latch mode
  /// @param latch true = latched (hold until read), false = transparent
  Status setAlertLatch(bool latch);

  /// Configure conversion ready on ALERT pin
  Status setConversionReadyAlert(bool enable);

  /// Configure slow alert (compare averaged values)
  Status setSlowAlert(bool enable);

  /// Configure alert pin polarity
  /// @param activeHigh true = active-high, false = active-low (default)
  Status setAlertPolarity(bool activeHigh);

  /// Set shunt overvoltage threshold
  /// @param voltageV Threshold voltage in volts
  Status setShuntOvervoltageThreshold(float voltageV);

  /// Set shunt undervoltage threshold
  /// @param voltageV Threshold voltage in volts
  Status setShuntUndervoltageThreshold(float voltageV);

  /// Set bus overvoltage threshold
  /// @param voltageV Threshold voltage in volts (0–85V)
  Status setBusOvervoltageThreshold(float voltageV);

  /// Set bus undervoltage threshold
  /// @param voltageV Threshold voltage in volts (0–85V)
  Status setBusUndervoltageThreshold(float voltageV);

  /// Set temperature over-limit threshold
  /// @param tempC Threshold temperature in Celsius
  Status setTemperatureOverlimitThreshold(float tempC);

  /// Set power over-limit threshold
  /// @param powerW Threshold power in watts (requires calibration)
  Status setPowerOverlimitThreshold(float powerW);

  // =========================================================================
  // Device Control
  // =========================================================================

  /// Software reset (equivalent to POR)
  Status softReset();

  /// Reset energy and charge accumulators
  Status resetAccumulators();

  /// Read manufacturer ID (expect 0x5449)
  Status readManufacturerId(uint16_t& id);

  /// Read device ID (expect 0x2281)
  Status readDeviceId(uint16_t& id);

  // =========================================================================
  // Raw Register Access
  // =========================================================================

  /// Read a 16-bit register using tracked transport
  Status readRegister16(uint8_t reg, uint16_t& value);

  /// Read a 24-bit register using tracked transport
  Status readRegister24(uint8_t reg, uint32_t& value);

  /// Read a 40-bit register using tracked transport
  Status readRegister40(uint8_t reg, uint64_t& value);

  /// Write a 16-bit register using tracked transport
  Status writeRegister16(uint8_t reg, uint16_t value);

  // =========================================================================
  // Timing
  // =========================================================================

  /// Estimate total conversion time in microseconds based on current config
  /// @return Conversion time in microseconds
  uint32_t estimateConversionTimeUs() const;

  /// Estimate total conversion time in milliseconds (rounded up)
  uint32_t estimateConversionTimeMs() const;

  /// Get current CURRENT_LSB value (amps per LSB)
  /// @return CURRENT_LSB, or 0 if not calibrated
  float currentLsb() const { return _currentLsb; }

private:
  // =========================================================================
  // Transport Wrappers
  // =========================================================================

  /// Raw I2C write-read (no health tracking)
  Status _i2cWriteReadRaw(const uint8_t* txBuf, size_t txLen,
                          uint8_t* rxBuf, size_t rxLen);

  /// Raw I2C write (no health tracking)
  Status _i2cWriteRaw(const uint8_t* buf, size_t len);

  /// Tracked I2C write-read (updates health)
  Status _i2cWriteReadTracked(const uint8_t* txBuf, size_t txLen,
                              uint8_t* rxBuf, size_t rxLen);

  /// Tracked I2C write (updates health)
  Status _i2cWriteTracked(const uint8_t* buf, size_t len);

  // =========================================================================
  // Register Access
  // =========================================================================

  /// Read 16-bit register (tracked)
  Status readReg16(uint8_t reg, uint16_t& value);

  /// Read 24-bit register (tracked)
  Status readReg24(uint8_t reg, uint32_t& value);

  /// Read 40-bit register (tracked)
  Status readReg40(uint8_t reg, uint64_t& value);

  /// Write 16-bit register (tracked)
  Status writeReg16(uint8_t reg, uint16_t value);

  /// Read 16-bit register (raw, no health tracking)
  Status _readReg16Raw(uint8_t reg, uint16_t& value);

  // =========================================================================
  // Health Management
  // =========================================================================

  /// Update health counters and state based on operation result
  /// Called ONLY from tracked transport wrappers
  Status _updateHealth(const Status& st);

  /// Record non-transport semantic failures that make recovery unsuccessful.
  Status _recordFailure(const Status& st);

  // =========================================================================
  // Internal Helpers
  // =========================================================================

  Status _applyConfig();
  Status _applyCalibration();
  Status _ensureMeasurementReadyForRead();
  uint32_t _nowMs() const;

  /// Build ADC_CONFIG register value from current config
  uint16_t _buildAdcConfig() const;

  /// Build CONFIG register value from current config
  uint16_t _buildConfig() const;

  /// Sign-extend 20-bit value stored in bits 23:4 of a 24-bit register
  static int32_t _signExtend20(uint32_t raw24);

  /// Sign-extend 40-bit value from uint64_t
  static int64_t _signExtend40(uint64_t raw40);

  // =========================================================================
  // State
  // =========================================================================

  Config _config;
  bool _initialized = false;
  DriverState _driverState = DriverState::UNINIT;

  // Health counters
  uint32_t _lastOkMs = 0;
  uint32_t _lastErrorMs = 0;
  Status _lastError = Status::Ok();
  uint8_t _consecutiveFailures = 0;
  uint32_t _totalFailures = 0;
  uint32_t _totalSuccess = 0;

  // Calibration state
  float _currentLsb = 0.0f;  ///< Amps per LSB (0 = uncalibrated)
  uint16_t _shuntCal = 0;    ///< Computed SHUNT_CAL register value

  // Triggered conversion tracking
  bool _trigPending = false;
  uint32_t _trigStartMs = 0;
};

} // namespace INA228
