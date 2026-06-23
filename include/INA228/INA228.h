/// @file INA228.h
/// @brief Main driver class for INA228 85-V, 20-bit power/energy/charge monitor
/// @warning The driver and examples do not provide isolation, fusing,
/// creepage/clearance, or electrical safety protection. High-voltage systems
/// require qualified hardware design, validation, and operating procedures.
/// The 85-V input capability is an IC rating, not a system safety rating.
/// Do not connect non-isolated or high-energy rails to development boards,
/// debug probes, or USB-connected PCs without appropriate protection.
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

/// @brief Fixed-unit integer measurement sample for bounded poll consumers.
///
/// Values are rounded to the nearest integer engineering unit. Current and
/// power require a valid SHUNT_CAL/current-LSB contract.
struct IntegerSample {
  int32_t shuntMicrovolts = 0;     ///< Shunt voltage in microvolts
  uint32_t busMillivolts = 0;      ///< Bus voltage in millivolts
  int32_t dieTemperatureMilliC = 0; ///< Die temperature in milli-degrees C
  int32_t currentMilliamps = 0;    ///< Current in milliamps
  uint32_t powerMilliwatts = 0;    ///< Power in milliwatts
  bool diagAlertValid = false;     ///< True when diagAlertRaw came from DIAG_ALRT
  uint16_t diagAlertRaw = 0;       ///< DIAG_ALRT captured before current-derived reads
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
  Status hardwareDirtyCause{};         ///< First failure/status that made hardwareDirty true
  bool thresholdsDirty = false;        ///< Sticky advisory that thresholds may not match active scale
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
/// evidence when a DIAG_ALRT read clears hardware status bits. Clearable/event
/// evidence is sticky until the next begin()/recover()/reset path replaces the
/// driver state; alert config bits and MEMSTAT reflect the latest captured read.
struct DiagAlertSnapshot {
  bool valid = false;       ///< True after the driver captured a DIAG_ALRT value
  uint16_t raw = 0;         ///< Preserved DIAG_ALRT evidence plus latest config/MEMSTAT bits
  DiagAlert diag{};         ///< Parsed DIAG_ALRT fields
  uint32_t capturedMs = 0;  ///< Timestamp from Config::nowMs, or 0 if unavailable
};

/// @brief Managed synchronous INA228 driver.
///
/// Instances are not thread-safe or ISR-safe. Applications must serialize API
/// calls, provide any shared-bus locking outside the driver, and ensure
/// transport/time callbacks do not re-enter the same INA228 instance.
/// The driver does not own or configure the I2C bus; bus creation, locking,
/// recovery, pins, pull-ups, and platform handles belong to the application
/// or an injected transport adapter.
/// Measurements and ALERT output are monitoring signals only, not certified
/// safety functions or substitutes for independent hardware protection.
/// Unless a method explicitly documents different behavior, output parameters
/// are committed only when Status::Ok() is returned and remain unchanged on
/// non-OK status. Readiness APIs are the exception: they clear their @p ready
/// output before polling, including on error.
class INA228 {
public:
  INA228() = default;

  INA228(const INA228&) = delete;
  INA228& operator=(const INA228&) = delete;
  INA228(INA228&&) = delete;
  INA228& operator=(INA228&&) = delete;

  // =========================================================================
  // Lifecycle
  // =========================================================================

  /// Initialize the driver with configuration
  /// @param config Configuration including transport callbacks and calibration
  /// @return Status::Ok() on success, error otherwise. Definite address NACK
  /// during identity/MEMSTAT reads maps to DEVICE_NOT_FOUND; timeout, data
  /// NACK, bus, and generic I2C errors are returned with their transport code.
  /// @note Startup verifies MEMSTAT by reading DIAG_ALRT. The observed evidence
  /// is preserved in getDiagAlertSnapshot(), but the hardware read can still
  /// clear CNVRF and latched diagnostic evidence.
  Status begin(const Config& config);

  /// Process pending operations (call regularly from loop)
  /// @param nowMs Current monotonic timestamp in milliseconds. This timestamp
  /// is used directly for triggered-conversion deadline checks.
  /// @note When driver-owned triggered or accumulator-readiness state can
  /// advance, this can perform a status-clearing DIAG_ALRT read. Observed
  /// evidence is preserved in getDiagAlertSnapshot().
  void tick(uint32_t nowMs);

  /// End the driver session and clear local runtime state without I2C.
  ///
  /// This is local teardown only. Use setMode(Mode::SHUTDOWN) first when the
  /// application wants an observable, status-returning hardware shutdown.
  void end();

  /// Check if begin() completed successfully and end() has not been called
  bool isInitialized() const { return _initialized; }

  /// Get the cached configuration snapshot currently owned by the driver
  const Config& getConfig() const { return _config; }

  // =========================================================================
  // Diagnostics
  // =========================================================================

  /// Check if device is present on the bus (no health tracking)
  /// @return Status::Ok() if device responds with correct IDs. Definite
  /// address NACK maps to DEVICE_NOT_FOUND; other transport errors are
  /// returned unchanged.
  Status probe();

  /// Attempt to recover from DEGRADED/OFFLINE state by re-validating IDs, MEMSTAT, and cached config
  /// @return Status::Ok() if device now responsive and configuration is re-applied, error otherwise
  /// @note Recovery verifies MEMSTAT by reading DIAG_ALRT. The observed
  /// evidence is preserved, but the hardware read is status-clearing.
  Status recover();

  /// Populate a cache-only settings snapshot without touching I2C.
  /// @param out Destination snapshot.
  /// @return Status::Ok(); inspect SettingsSnapshot::initialized for state.
  Status getSettings(SettingsSnapshot& out) const;

  /// Populate the last preserved DIAG_ALRT snapshot without touching I2C.
  /// @param out Destination snapshot.
  /// @return Status::Ok(); valid is false until DIAG_ALRT evidence exists.
  Status getDiagAlertSnapshot(DiagAlertSnapshot& out) const;

  // =========================================================================
  // Driver State
  // =========================================================================

  /// Get current driver state
  DriverState state() const { return _driverState; }

  /// Get current driver state; compatibility alias for shared I2C library APIs
  DriverState driverState() const { return state(); }

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
  /// Requires valid calibration because the aggregate includes current and
  /// power fields. Use scalar voltage/temperature reads or readRawSample() for
  /// uncalibrated bring-up.
  ///
  /// Continuous modes return the latest hardware register contents available at
  /// read time; they do not guarantee freshness since the previous call. While
  /// a driver-tracked triggered conversion is pending, this returns
  /// MEASUREMENT_NOT_READY and leaves @p out unchanged. ENERGY and CHARGE are
  /// reported only when their validity flags are true; otherwise the fields are
  /// not valid accumulator data. Converted current, power, energy, and charge
  /// fields require valid calibration. This call preserves DIAG_ALRT evidence
  /// before accumulator reads, but those status-sensitive reads may still clear
  /// hardware flags according to the datasheet.
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
  /// accumulator reads can clear overflow evidence. This diagnostic/raw call
  /// preserves the DIAG_ALRT snapshot it consumes, then reads raw accumulators;
  /// raw ENERGY/CHARGE fields may be invalid when their validity flags are
  /// false.
  /// @param out RawSample structure
  /// @return Status::Ok() on success
  Status readRawSample(RawSample& out);

  /// Read fixed-unit integer values for voltage, temperature, current, and power.
  ///
  /// This convenience call does not read ENERGY or CHARGE, so it does not
  /// consume accumulator overflow evidence. It still performs multiple backend
  /// transfers and a destructive DIAG_ALRT read before current-derived fields;
  /// bounded poll owners that need one register transfer per poll should stage
  /// raw register reads and use convertRawSample().
  /// @param out IntegerSample result
  /// @return Status::Ok() on success
  Status readIntegerSample(IntegerSample& out);

  /// Convert raw register values to fixed-unit integer values without I2C.
  ///
  /// This is intended for callers that stage raw register reads themselves.
  /// It requires an initialized, calibrated, clean driver because current and
  /// power scaling depend on the cached SHUNT_CAL/current-LSB contract.
  /// ENERGY and CHARGE fields in @p raw are ignored.
  /// @param raw Raw register sample or staged raw values
  /// @param out IntegerSample result
  /// @return Status::Ok() on success
  Status convertRawSample(const RawSample& raw, IntegerSample& out) const;

  /// Start a triggered one-shot measurement.
  ///
  /// This is a named wrapper around triggerConversion() for fixed-step users.
  /// It performs one ADC_CONFIG write and returns Err::IN_PROGRESS when the
  /// conversion was started.
  Status startTriggeredMeasurement(Mode mode = Mode::TRIG_ALL);

  /// Poll triggered measurement readiness with an explicit instruction budget.
  ///
  /// Delay gates consume zero instructions. Once the software conversion
  /// deadline has elapsed, this performs at most one DIAG_ALRT read when
  /// maxInstructions is greater than zero.
  /// @param nowMs Current monotonic timestamp in milliseconds
  /// @param maxInstructions Maximum transport callbacks allowed, from 1 upward
  /// @param ready Cleared before polling, then set true when the sample is ready
  Status pollMeasurementReady(uint32_t nowMs, uint8_t maxInstructions,
                              bool& ready);

  /// Read the TunnelMonitor power sample as a fixed-step job.
  ///
  /// Each call consumes at most maxInstructions backend transfers. A 16-bit or
  /// 24-bit register read is one instruction. The steady continuous-mode path
  /// reads VSHUNT, VBUS, DIETEMP, CURRENT, and POWER only; ENERGY and CHARGE
  /// are intentionally excluded. Outputs are committed only when OK is
  /// returned and remain unchanged while IN_PROGRESS is returned.
  Status readPowerSampleRawStep(RawSample& rawOut, IntegerSample& integerOut,
                                uint8_t maxInstructions);

  /// Start replaying cached static configuration and calibration as a job.
  Status startApplyCalibration();

  /// Poll cached configuration/calibration replay.
  ///
  /// Each side-effecting register write consumes one instruction and the job
  /// stops on first failure, leaving dirty-state evidence for recovery.
  Status pollApplyCalibration(uint32_t nowMs, uint8_t maxInstructions);

  /// Start replaying cached static configuration and calibration as a job.
  ///
  /// Preferred clearer alias for startApplyCalibration().
  Status startConfigReplayJob() { return startApplyCalibration(); }

  /// Poll cached static configuration and calibration replay.
  ///
  /// Preferred clearer alias for pollApplyCalibration().
  Status pollConfigReplayJob(uint32_t nowMs, uint8_t maxInstructions) {
    return pollApplyCalibration(nowMs, maxInstructions);
  }

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
  /// @note Returns MATH_OVERFLOW when DIAG_ALRT.MATHOF indicates current or
  /// power data may be invalid; this check performs a destructive DIAG_ALRT
  /// read and preserves the evidence in getDiagAlertSnapshot().
  /// @param out Current (A)
  /// @return Status::Ok() on success
  Status readCurrent(float& out);

  /// Read calculated power in watts (requires calibration)
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @note Returns MATH_OVERFLOW when DIAG_ALRT.MATHOF indicates current or
  /// power data may be invalid; this check performs a destructive DIAG_ALRT
  /// read and preserves the evidence in getDiagAlertSnapshot().
  /// @param out Power (W)
  /// @return Status::Ok() on success
  Status readPower(float& out);

  /// Read accumulated energy in joules (requires calibration and valid continuous accumulation)
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @note Returns ACCUMULATION_INVALID outside valid continuous accumulation
  /// conditions, ACCUMULATION_OVERFLOW when ENERGYOF is observed, and
  /// MATH_OVERFLOW when MATHOF is observed.
  /// @param out Energy (J)
  /// @return Status::Ok() on success
  Status readEnergy(double& out);

  /// Read accumulated charge in coulombs (requires calibration and valid continuous accumulation)
  /// @note Returns MEASUREMENT_NOT_READY while a triggered conversion is pending.
  /// @note Returns ACCUMULATION_INVALID outside valid continuous accumulation
  /// conditions, ACCUMULATION_OVERFLOW when CHARGEOF is observed, and
  /// MATH_OVERFLOW when MATHOF is observed.
  /// @param out Charge (C)
  /// @return Status::Ok() on success
  Status readCharge(double& out);

  /// Check if conversion is ready using Config::nowMs when a trigger is pending.
  /// @note After the software deadline elapses this performs a destructive
  /// DIAG_ALRT read to observe CNVRF. Evidence from that read is preserved in
  /// getDiagAlertSnapshot(), but hardware status bits can still be cleared.
  /// @param ready Cleared before polling, then set true if conversion complete
  /// @return Status::Ok() on success
  Status isConversionReady(bool& ready);

  /// Poll conversion readiness using the caller-provided timestamp.
  ///
  /// This is the status-returning variant used by tick(). It allows triggered
  /// conversions to advance deterministically even when Config::nowMs is unset.
  /// CNVRF remains authoritative after the software deadline has elapsed.
  /// @note After the software deadline elapses this performs a destructive
  /// DIAG_ALRT read to observe CNVRF. Evidence from that read is preserved in
  /// getDiagAlertSnapshot(), but hardware status bits can still be cleared.
  /// @param nowMs Current monotonic timestamp in milliseconds
  /// @param ready Cleared before polling, then set true if CNVRF was observed on this poll
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
  /// back. The failed write still leaves conservative dirty evidence, so
  /// converted current/power/energy/charge APIs return Err::HARDWARE_DIRTY
  /// until recover() successfully replays config. If rollback also fails,
  /// CONFIG is marked dirty too.
  ///
  /// @note Existing alert thresholds are not re-encoded. SettingsSnapshot
  /// thresholdsDirty is set after a scale change so applications can reapply
  /// engineering-unit limits deliberately. It is sticky and is not cleared by
  /// successful threshold writes.
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
  /// be reapplied for the new scale. The flag is sticky until begin()/end()
  /// state reset or a full reinitialization path clears it.
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

  /// Read and clear status-sensitive DIAG_ALRT evidence explicitly.
  ///
  /// This is equivalent to readDiagAlertRaw() and is named for callers that
  /// want to make the clear-on-read behavior visible in their poll jobs.
  Status readAndClearDiagAlert(uint16_t& raw);

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

  /// @warning Alert thresholds and ALERT output are monitoring aids, not a
  /// safety interlock. Use independent hardware protection for hazardous rails.

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

  /// Software reset (equivalent to POR).
  ///
  /// This is bounded and contains no platform delay. The driver writes RST,
  /// verifies finite readback of CONFIG.RST/RSTACC clear plus identity and
  /// MEMSTAT, then replays cached configuration/calibration. MEMSTAT
  /// verification reads DIAG_ALRT and can consume live status even though the
  /// raw value is preserved. If reset or replay
  /// cannot be verified, SettingsSnapshot::hardwareDirty remains true and typed
  /// converted reads fail until recover() or another successful softReset().
  Status softReset();

  /// Start software reset as a fixed-step job.
  ///
  /// The job writes reset, waits the datasheet startup time through
  /// pollResetJob(), verifies CONFIG, IDs, and MEMSTAT one register read at a
  /// time, then replays cached configuration/calibration.
  Status startResetJob();

  /// Poll a software reset job.
  ///
  /// Delay gates consume zero instructions. Every register read or write
  /// consumes one instruction, and maxInstructions must be at least one.
  Status pollResetJob(uint32_t nowMs, uint8_t maxInstructions);

  /// Reset energy and charge accumulators.
  ///
  /// This clears the device's accumulated ENERGY and CHARGE registers and
  /// explicitly writes CONFIG again with RSTACC clear because self-clear is not
  /// assumed. The driver verifies CONFIG.RSTACC is clear and
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

  /// Read a 16-bit register using tracked transport.
  ///
  /// Diagnostic/service access only. Status-sensitive registers can have read
  /// side effects; REG_DIAG_ALRT reads consume live diagnostic evidence and
  /// accumulator register reads can clear overflow evidence.
  Status readRegister16(uint8_t reg, uint16_t& value);

  /// Read a 24-bit register using tracked transport.
  ///
  /// Diagnostic/service access only. Prefer typed APIs for normal operation.
  Status readRegister24(uint8_t reg, uint32_t& value);

  /// Read a 40-bit register using tracked transport.
  ///
  /// Diagnostic/service access only. ENERGY/CHARGE reads can affect overflow
  /// evidence. This raw helper does not pre-read or preserve DIAG_ALRT; use
  /// typed APIs or explicitly read DIAG_ALRT first when evidence matters.
  Status readRegister40(uint8_t reg, uint64_t& value);

  /// Write a 16-bit register using tracked transport.
  ///
  /// Diagnostic/service access only. Raw writes bypass typed cache/calibration
  /// helpers and can make cached driver state differ from hardware; use
  /// recover(), softReset(), or begin() to resynchronize after manual writes.
  Status writeRegister16(uint8_t reg, uint16_t value);

  // =========================================================================
  // Timing
  // =========================================================================

  /// Estimate total conversion time in microseconds based on current config
  /// @return Conversion time in microseconds
  uint32_t estimateConversionTimeUs() const;

  /// Estimate total conversion time in milliseconds (rounded up)
  uint32_t estimateConversionTimeMs() const;

  /// Get cached CURRENT_LSB value (amps per LSB)
  /// @return CURRENT_LSB, or 0 if calibration was never configured. Check
  /// SettingsSnapshot::calibrated and SettingsSnapshot::hardwareDirty before
  /// treating the value as usable for converted readings.
  float currentLsb() const { return _currentLsb; }

private:
  enum class AsyncJob : uint8_t {
    NONE,
    POWER_SAMPLE,
    APPLY_CALIBRATION,
    RESET
  };

  enum class PowerSampleStep : uint8_t {
    IDLE,
    DIAG_READY,
    VSHUNT,
    VBUS,
    DIETEMP,
    CURRENT,
    POWER
  };

  enum class ApplyStep : uint8_t {
    IDLE,
    ADC_SHUTDOWN,
    CONFIG,
    DIAG_ALRT,
    TEMPCO,
    SHUNT_CAL,
    ADC_CONFIG,
    DONE
  };

  enum class ResetStep : uint8_t {
    IDLE,
    WRITE_RESET,
    WAIT_STARTUP,
    VERIFY_CONFIG,
    READ_MANUFACTURER,
    READ_DEVICE,
    READ_DIAG,
    APPLY_ADC_SHUTDOWN,
    APPLY_CONFIG,
    APPLY_DIAG_ALRT,
    APPLY_TEMPCO,
    APPLY_SHUNT_CAL,
    APPLY_ADC_CONFIG,
    DONE
  };

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

  /// Read DIAG_ALRT through tracked transport and preserve diagnostic evidence.
  Status _readDiagAlertTracked(uint16_t& raw);

  /// Read DIAG_ALRT through raw transport and preserve diagnostic evidence.
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
  Status _applyStaticConfig();
  Status _applyCalibration();
  Status _resyncCachedHardware();
  Status _verifyResetComplete();
  Status _verifyIdentityAndMemstat(bool preserveAlertConfig);
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
  Status _readAndValidateMathDiag(uint16_t& raw);
  Status _validateAccumulatorDiag(uint16_t raw, uint16_t overflowBit,
                                  const char* overflowMsg) const;
  bool _triggerDeadlineElapsed(uint32_t nowMs) const;
  void _markTriggeredConversionStarted(uint32_t nowMs);
  void _completeTriggeredConversion();
  void _clearCapturedConversionReadyFlag();
  void _clearCapturedAccumulatorEvidence();
  void _markHardwareDirty(uint8_t reg);
  void _markHardwareDirty(uint8_t reg, const Status& cause);
  void _markConfigReplayDirty(const Status& cause);
  void _markCalibrationDirty(const Status& cause);
  void _clearHardwareDirty();
  void _markThresholdsDirty();
  bool _isThresholdRegister(uint8_t reg) const;
  Status _writeApplyStep(ApplyStep step);
  Status _fillPowerSampleUnits(const RawSample& raw, IntegerSample& out) const;
  void _clearAsyncJob();

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
  Status _hardwareDirtyCause = Status::Ok();
  bool _thresholdsDirty = false;

  // Triggered conversion tracking
  bool _trigPending = false;
  uint32_t _trigStartMs = 0;

  // Accumulator validity tracking
  bool _accumulationReady = false;

  // DIAG_ALRT cache and preserved evidence
  uint16_t _diagAlertConfigBits = 0;
  DiagAlertSnapshot _diagAlertSnapshot{};

  // Fixed-step job state
  AsyncJob _asyncJob = AsyncJob::NONE;
  PowerSampleStep _powerSampleStep = PowerSampleStep::IDLE;
  RawSample _powerSampleRawScratch{};
  IntegerSample _powerSampleIntegerScratch{};
  ApplyStep _applyStep = ApplyStep::IDLE;
  ResetStep _resetStep = ResetStep::IDLE;
  uint32_t _resetStartMs = 0;
  uint8_t _resetVerifyAttempts = 0;
  bool _resetStartedOffline = false;
  uint16_t _resetDesiredDiagConfig = 0;
};

} // namespace INA228
