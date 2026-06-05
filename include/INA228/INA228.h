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

/// @brief Driver state for health monitoring.
enum class DriverState : uint8_t {
  UNINIT,    ///< begin() not called or end() called
  READY,     ///< Operational, consecutiveFailures == 0
  DEGRADED,  ///< 1 <= consecutiveFailures < offlineThreshold
  OFFLINE    ///< consecutiveFailures >= offlineThreshold
};

/// @brief Converted measurement result.
struct Measurement {
  float shuntVoltageV = 0.0f;  ///< Shunt voltage in volts
  float busVoltageV = 0.0f;    ///< Bus voltage in volts
  float temperatureC = 0.0f;   ///< Die temperature in Celsius
  float currentA = 0.0f;       ///< Current in amperes
  float powerW = 0.0f;         ///< Power in watts
  double energyJ = 0.0;        ///< Accumulated energy in joules
  double chargeC = 0.0;        ///< Accumulated charge in coulombs
  bool energyValid = false;    ///< True when energyJ is valid for this read
  bool chargeValid = false;    ///< True when chargeC is valid for this read
  bool energyOverflow = false; ///< ENERGYOF was observed before accumulator read
  bool chargeOverflow = false; ///< CHARGEOF was observed before accumulator read
  bool mathOverflow = false;   ///< MATHOF was observed before accumulator read
  bool diagAlertValid = false; ///< True when diagAlertRaw came from DIAG_ALRT
  uint16_t diagAlertRaw = 0;   ///< DIAG_ALRT captured before accumulator reads
};

/// @brief Raw measurement register values.
struct RawSample {
  int32_t vshunt = 0;   ///< Raw shunt voltage (20-bit, signed, bits 23:4)
  uint32_t vbus = 0;    ///< Raw bus voltage (20-bit, unsigned, bits 23:4)
  int16_t dietemp = 0;  ///< Raw die temperature (16-bit, signed)
  int32_t current = 0;  ///< Raw current (20-bit, signed, bits 23:4)
  uint32_t power = 0;   ///< Raw power (24-bit, unsigned)
  uint64_t energy = 0;  ///< Raw energy (40-bit, unsigned)
  int64_t charge = 0;   ///< Raw charge (40-bit, signed)
  bool energyValid = false;    ///< True when raw energy is valid for this read
  bool chargeValid = false;    ///< True when raw charge is valid for this read
  bool energyOverflow = false; ///< ENERGYOF was observed before accumulator read
  bool chargeOverflow = false; ///< CHARGEOF was observed before accumulator read
  bool mathOverflow = false;   ///< MATHOF was observed before accumulator read
  bool diagAlertValid = false; ///< True when diagAlertRaw came from DIAG_ALRT
  uint16_t diagAlertRaw = 0;   ///< DIAG_ALRT captured before accumulator reads
};

/// @brief Snapshot of cached settings and runtime state without I2C access.
struct SettingsSnapshot {
  bool initialized = false;
  DriverState state = DriverState::UNINIT;
  uint8_t i2cAddress = 0x40;
  uint32_t i2cTimeoutMs = 0;
  uint8_t offlineThreshold = 0;
  bool hasNowMsHook = false;
  Mode mode = Mode::CONT_ALL;
  ConvTime vbusConvTime = ConvTime::US_1052;
  ConvTime vshuntConvTime = ConvTime::US_1052;
  ConvTime vtempConvTime = ConvTime::US_1052;
  Averaging averaging = Averaging::AVG_1;
  AdcRange adcRange = AdcRange::MV_163_84;
  float shuntResistanceOhm = 0.0f;
  float maxExpectedCurrentA = 0.0f;
  bool tempCompEnabled = false;
  uint16_t shuntTempCoeffPpmC = 0;
  uint8_t convDelayMs2 = 0;
  float currentLsb = 0.0f;             ///< Actual amps/LSB used for converted current-derived values
  uint16_t shuntCal = 0;               ///< Actual SHUNT_CAL value programmed by the driver
  bool calibrated = false;             ///< True when SHUNT_CAL/currentLsb are usable and hardware is clean
  bool calibrationClamped = false;     ///< True when requested calibration was clamped to SHUNT_CAL range
  bool maxCurrentExceedsShuntRange = false; ///< True when max A * shunt ohms exceeds ADC full-scale
  bool hardwareDirty = false;          ///< True after unresolved partial config/calibration hardware state
  uint64_t dirtyRegisterMask = 0;      ///< Bit mask of possibly dirty registers, indexed by register address
  bool thresholdsDirty = false;        ///< True when scaling changed after engineering-unit thresholds were set
  bool triggeredConversionPending = false;
  uint32_t triggeredConversionStartMs = 0;
};

/// @brief Diagnostic, alert configuration, and alert flag bits from DIAG_ALRT.
/// @note Values produced by readDiagAlert() come from a destructive hardware
/// read. That read can clear CNVRF and latched alert status bits according to
/// the INA228 datasheet.
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

/// @brief Cache-only preserved DIAG_ALRT evidence from prior driver reads.
/// @note This snapshot does not touch I2C and may be older than the current
/// hardware state. It exists so internal CNVRF polling does not discard alert
/// evidence when a DIAG_ALRT read clears hardware status bits.
struct DiagAlertSnapshot {
  bool valid = false;       ///< True after the driver captured a DIAG_ALRT value
  uint16_t raw = 0;         ///< Raw DIAG_ALRT value captured by the driver
  DiagAlert diag{};         ///< Parsed DIAG_ALRT fields
  uint32_t capturedMs = 0;  ///< Timestamp from Config::nowMs, or 0 if unavailable
};

/// @brief Managed synchronous INA228 driver.
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
  /// @param nowMs Current monotonic timestamp in milliseconds. This timestamp
  /// is used directly for triggered-conversion deadline checks.
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

  /// Populate a cache-only settings snapshot without touching I2C.
  Status getSettings(SettingsSnapshot& out) const;

  /// Populate the last preserved DIAG_ALRT snapshot without touching I2C.
  Status getDiagAlertSnapshot(DiagAlertSnapshot& out) const;

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

  /// Read all available measurements into a float structure.
  ///
  /// Continuous modes return the latest hardware register contents available at
  /// read time; they do not guarantee freshness since the previous call. While
  /// a driver-tracked triggered conversion is pending, this returns
  /// MEASUREMENT_NOT_READY and leaves @p out unchanged. ENERGY and CHARGE are
  /// reported only when their validity flags are true; otherwise the fields are
  /// not valid accumulator data.
  /// @param out Measurement result
  /// @return Status::Ok() on success
  Status readMeasurement(Measurement& out);

  /// Read raw register values for all channels.
  ///
  /// Continuous modes return the latest hardware register contents available at
  /// read time; they do not guarantee freshness since the previous call. While
  /// a driver-tracked triggered conversion is pending, this returns
  /// MEASUREMENT_NOT_READY and leaves @p out unchanged. ENERGY and CHARGE
  /// register values are accompanied by validity and overflow flags because
  /// accumulator reads can clear overflow evidence.
  /// @param out RawSample structure
  /// @return Status::Ok() on success
  Status readRawSample(RawSample& out);

  /// Read shunt voltage in volts
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @param out Shunt voltage (V)
  /// @return Status::Ok() on success
  Status readShuntVoltage(float& out);

  /// Read bus voltage in volts
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @param out Bus voltage (V)
  /// @return Status::Ok() on success
  Status readBusVoltage(float& out);

  /// Read die temperature in Celsius
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @param out Temperature (°C)
  /// @return Status::Ok() on success
  Status readTemperature(float& out);

  /// Read calculated current in amperes (requires calibration)
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @param out Current (A)
  /// @return Status::Ok() on success
  Status readCurrent(float& out);

  /// Read calculated power in watts (requires calibration)
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @param out Power (W)
  /// @return Status::Ok() on success
  Status readPower(float& out);

  /// Read accumulated energy in joules (requires calibration and valid continuous accumulation)
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @note Returns ACCUMULATION_INVALID outside valid continuous accumulation
  /// conditions and ACCUMULATION_OVERFLOW when ENERGYOF is observed.
  /// @param out Energy (J)
  /// @return Status::Ok() on success
  Status readEnergy(double& out);

  /// Read accumulated charge in coulombs (requires calibration and valid continuous accumulation)
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @note Returns ACCUMULATION_INVALID outside valid continuous accumulation
  /// conditions and ACCUMULATION_OVERFLOW when CHARGEOF is observed.
  /// @param out Charge (C)
  /// @return Status::Ok() on success
  Status readCharge(double& out);

  /// Check if conversion is ready using Config::nowMs when a trigger is pending.
  /// @param ready Set to true if conversion complete
  /// @return Status::Ok() on success
  Status isConversionReady(bool& ready);

  /// Poll conversion readiness using the caller-provided timestamp.
  ///
  /// This is the status-returning variant used by tick(). It allows triggered
  /// conversions to advance deterministically even when Config::nowMs is unset.
  /// CNVRF remains authoritative after the software deadline has elapsed.
  /// @param nowMs Current monotonic timestamp in milliseconds
  /// @param ready Set to true if CNVRF was observed on this poll
  /// @return Status::Ok() on success
  Status pollConversionReady(uint32_t nowMs, bool& ready);

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

  /// Set shunt ADC range.
  ///
  /// The driver precomputes the matching SHUNT_CAL/currentLsb contract before
  /// changing CONFIG. If the follow-up SHUNT_CAL write fails, CONFIG is rolled
  /// back. If rollback also fails, converted current/power/energy/charge APIs
  /// return Err::HARDWARE_DIRTY until recover() successfully replays config.
  ///
  /// @note Existing alert thresholds are not re-encoded. SettingsSnapshot
  /// thresholdsDirty is set after a scale change so applications can reapply
  /// engineering-unit limits deliberately.
  Status setAdcRange(AdcRange range);

  /// Update shunt calibration for the installed shunt resistor.
  ///
  /// This programs SHUNT_CAL and updates the cached shunt resistance,
  /// maximum expected current, and CURRENT_LSB used for current, power, energy,
  /// and charge conversion.
  ///
  /// Cache and scaling update only after the register write succeeds. If the
  /// computed SHUNT_CAL value clamps, currentLsb() reflects the clamped value
  /// and SettingsSnapshot::calibrationClamped is set.
  ///
  /// A successful calibration change invalidates accumulation readiness and
  /// marks SettingsSnapshot::thresholdsDirty so engineering-unit thresholds can
  /// be reapplied for the new scale.
  /// @param shuntOhm Shunt resistance in ohms
  /// @param maxCurrentA Maximum expected current in amps
  /// @return Status::Ok() on success
  Status setCalibration(float shuntOhm, float maxCurrentA);

  /// Set shunt temperature coefficient register value.
  ///
  /// The coefficient is programmed even when temperature compensation is
  /// disabled so register readback stays deterministic.
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

  /// Read diagnostic, alert configuration, and alert flag bits.
  ///
  /// This performs a destructive hardware read of DIAG_ALRT. It can clear
  /// CNVRF and latched alert flags; the exact raw value is preserved in
  /// getDiagAlertSnapshot().
  Status readDiagAlert(DiagAlert& out);

  /// Read raw DIAG_ALRT register value.
  ///
  /// This performs a destructive hardware read and preserves the exact raw
  /// value in getDiagAlertSnapshot().
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
  /// Uses the signed shunt threshold register scale for the active ADC range
  /// (5 uV/LSB at +/-163.84 mV, 1.25 uV/LSB at +/-40.96 mV).
  /// @param voltageV Threshold voltage in volts
  Status setShuntOvervoltageThreshold(float voltageV);

  /// Set shunt undervoltage threshold
  /// Uses the signed shunt threshold register scale for the active ADC range
  /// (5 uV/LSB at +/-163.84 mV, 1.25 uV/LSB at +/-40.96 mV).
  /// @param voltageV Threshold voltage in volts
  Status setShuntUndervoltageThreshold(float voltageV);

  /// Set bus overvoltage threshold
  /// Uses the unsigned bus threshold register scale (3.125 mV/LSB).
  /// @param voltageV Threshold voltage in volts (0–85V)
  Status setBusOvervoltageThreshold(float voltageV);

  /// Set bus undervoltage threshold
  /// Uses the unsigned bus threshold register scale (3.125 mV/LSB).
  /// @param voltageV Threshold voltage in volts (0–85V)
  Status setBusUndervoltageThreshold(float voltageV);

  /// Set temperature over-limit threshold
  /// @param tempC Threshold temperature in Celsius
  Status setTemperatureOverlimitThreshold(float tempC);

  /// Set power over-limit threshold
  /// Requires calibration because the threshold register scale is derived from
  /// CURRENT_LSB.
  /// @param powerW Threshold power in watts (requires calibration)
  Status setPowerOverlimitThreshold(float powerW);

  // =========================================================================
  // Device Control
  // =========================================================================

  /// Software reset (equivalent to POR)
  Status softReset();

  /// Reset energy and charge accumulators.
  ///
  /// This clears the device's accumulated ENERGY and CHARGE registers and
  /// invalidates driver-side accumulator readiness until a continuous CNVRF is
  /// observed again.
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

  /// Read DIAG_ALRT through tracked transport and preserve the raw evidence.
  Status _readDiagAlertTracked(uint16_t& raw);

  /// Read DIAG_ALRT through raw transport and preserve the raw evidence.
  Status _readDiagAlertRaw(uint16_t& raw);

  /// Parse and store a DIAG_ALRT value without touching I2C.
  void _captureDiagAlert(uint16_t raw);

  /// Write cached DIAG_ALRT alert configuration bits without reading live flags.
  Status _writeDiagAlertConfig(uint16_t configBits);

  // =========================================================================
  // Health Management
  // =========================================================================

  /// Update health counters and state based on operation result
  /// Called ONLY from tracked transport wrappers
  Status _updateHealth(const Status& st);

  /// Record non-transport semantic failures that make recovery unsuccessful.
  Status _recordFailure(const Status& st);
  void _reassertOfflineLatch();

  /// Reject normal public I2C while the driver is latched OFFLINE.
  Status _ensureNormalI2cAllowed() const;

  // =========================================================================
  // Internal Helpers
  // =========================================================================

  Status _applyConfig();
  Status _applyCalibration();
  Status _ensureHardwareClean() const;
  Status _ensureCalibrated() const;
  Status _ensureMeasurementReadyForRead();
  uint32_t _nowMs() const;
  bool _modeSupportsEnergyAccumulation() const;
  bool _modeSupportsChargeAccumulation() const;
  bool _modeSupportsAnyAccumulation() const;
  void _markAccumulationInvalid();
  Status _ensureEnergyAccumulatorReadable() const;
  Status _ensureChargeAccumulatorReadable() const;
  Status _readAccumulatorDiag(uint16_t& raw);
  Status _validateAccumulatorDiag(uint16_t raw, uint16_t overflowBit,
                                  const char* overflowMsg) const;
  bool _triggerDeadlineElapsed(uint32_t nowMs) const;
  void _markTriggeredConversionStarted(uint32_t nowMs);
  void _completeTriggeredConversion();
  void _clearCapturedConversionReadyFlag();
  void _markHardwareDirty(uint8_t reg);
  void _clearHardwareDirty();
  void _markThresholdsDirty();

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
  bool _allowOfflineI2c = false;

  // Calibration state
  float _currentLsb = 0.0f;  ///< Amps per LSB (0 = uncalibrated)
  uint16_t _shuntCal = 0;    ///< Computed SHUNT_CAL register value
  bool _calibrationClamped = false;
  bool _maxCurrentExceedsShuntRange = false;
  bool _hardwareDirty = false;
  uint64_t _dirtyRegisterMask = 0;
  bool _thresholdsDirty = false;

  // Triggered conversion tracking
  bool _trigPending = false;
  uint32_t _trigStartMs = 0;

  // Accumulator validity tracking
  bool _accumulationReady = false;

  // DIAG_ALRT cache and preserved evidence
  uint16_t _diagAlertConfigBits = 0;
  DiagAlertSnapshot _diagAlertSnapshot{};
};

} // namespace INA228
