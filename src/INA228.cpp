/**
 * @file INA228.cpp
 * @brief INA228 driver implementation.
 */

#include "INA228/INA228.h"

#include <cmath>
#include <limits>

namespace INA228 {
namespace {

static constexpr uint32_t RESET_STARTUP_MS =
    (cmd::POR_STARTUP_US + 999U) / 1000U;
/// Registers a CONFIG.RST software reset restores to their defaults.
static constexpr uint64_t RESET_TOUCHED_REGISTER_MASK =
    (1ULL << cmd::REG_CONFIG) | (1ULL << cmd::REG_ADC_CONFIG) |
    (1ULL << cmd::REG_SHUNT_CAL) | (1ULL << cmd::REG_SHUNT_TEMPCO) |
    (1ULL << cmd::REG_DIAG_ALRT) | (1ULL << cmd::REG_SOVL) |
    (1ULL << cmd::REG_SUVL) | (1ULL << cmd::REG_BOVL) |
    (1ULL << cmd::REG_BUVL) | (1ULL << cmd::REG_TEMP_LIMIT) |
    (1ULL << cmd::REG_PWR_LIMIT);
static constexpr uint16_t DIAG_EVIDENCE_MASK =
    cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF | cmd::DIAG_MATHOF |
    cmd::DIAG_TMPOL | cmd::DIAG_SHNTOL | cmd::DIAG_SHNTUL |
    cmd::DIAG_BUSOL | cmd::DIAG_BUSUL | cmd::DIAG_POL |
    cmd::DIAG_CNVRF;

/// @brief Wrap-safe "has @p waitMs elapsed since @p startMs" test.
///
/// Wait origins are sampled after a blocking write returns, so they can be
/// slightly newer than a timestamp the caller sampled before the call. A plain
/// `now - start >= wait` underflows in that case and reports the wait as
/// already elapsed. Treating a delta in the upper half of the uint32 range as
/// "origin is in the future, not elapsed" keeps normal wraparound working while
/// rejecting that inversion.
static bool waitElapsed(uint32_t nowMs, uint32_t startMs, uint32_t waitMs) {
  const uint32_t elapsed = nowMs - startMs;
  return elapsed < 0x80000000U && elapsed >= waitMs;
}

static bool isValidAddress(uint8_t addr) {
  return addr >= 0x40 && addr <= 0x4F;
}

static bool isValidConvTime(ConvTime ct) {
  return static_cast<uint8_t>(ct) <= 0x07;
}

static bool isValidAveraging(Averaging avg) {
  return static_cast<uint8_t>(avg) <= 0x07;
}

static bool isValidAdcRange(AdcRange range) {
  return range == AdcRange::MV_163_84 || range == AdcRange::MV_40_96;
}

static bool isValidMode(Mode mode) {
  return static_cast<uint8_t>(mode) <= 0x0F;
}

static bool isTriggeredMode(Mode mode) {
  const uint8_t m = static_cast<uint8_t>(mode);
  return m >= 1 && m <= 7;
}

static bool isContinuousMode(Mode mode) {
  const uint8_t m = static_cast<uint8_t>(mode);
  return m >= 9 && m <= 15;
}

static uint16_t convTimeUs(ConvTime ct) {
  const uint8_t idx = static_cast<uint8_t>(ct);
  if (idx > 7) return cmd::CONV_TIME_US[5]; // safe fallback
  return cmd::CONV_TIME_US[idx];
}

static uint16_t avgCount(Averaging avg) {
  const uint8_t idx = static_cast<uint8_t>(avg);
  if (idx > 7) return 1;
  return cmd::AVG_COUNT[idx];
}

/// Check if mode includes shunt voltage measurement
static bool modeHasShunt(Mode mode) {
  const uint8_t m = static_cast<uint8_t>(mode) & 0x07;
  return m == 2 || m == 3 || m == 6 || m == 7;
}

/// Check if mode includes bus voltage measurement
static bool modeHasBus(Mode mode) {
  const uint8_t m = static_cast<uint8_t>(mode) & 0x07;
  return m == 1 || m == 3 || m == 5 || m == 7;
}

/// Check if mode includes temperature measurement
static bool modeHasTemp(Mode mode) {
  const uint8_t m = static_cast<uint8_t>(mode) & 0x07;
  return m == 4 || m == 5 || m == 6 || m == 7;
}

static Status validatePositiveFinite(float value, const char* message) {
  if (!std::isfinite(value) || value <= 0.0f) {
    return Status::Error(Err::INVALID_PARAM, message);
  }
  return Status::Ok();
}

static Status assignFiniteFloat(double value, float& out, const char* message) {
  const double maxFloat = static_cast<double>(std::numeric_limits<float>::max());
  if (!std::isfinite(value) || value > maxFloat || value < -maxFloat) {
    return Status::Error(Err::MATH_OVERFLOW, message);
  }
  out = static_cast<float>(value);
  return Status::Ok();
}

static Status assignRoundedInt32(double value, int32_t& out, const char* message) {
  const double rounded = std::round(value);
  if (!std::isfinite(rounded) ||
      rounded > static_cast<double>(std::numeric_limits<int32_t>::max()) ||
      rounded < static_cast<double>(std::numeric_limits<int32_t>::min())) {
    return Status::Error(Err::MATH_OVERFLOW, message);
  }
  out = static_cast<int32_t>(rounded);
  return Status::Ok();
}

static Status assignRoundedUint32(double value, uint32_t& out, const char* message) {
  const double rounded = std::round(value);
  if (!std::isfinite(rounded) || rounded < 0.0 ||
      rounded > static_cast<double>(std::numeric_limits<uint32_t>::max())) {
    return Status::Error(Err::MATH_OVERFLOW, message);
  }
  out = static_cast<uint32_t>(rounded);
  return Status::Ok();
}

static int32_t roundDivNearestSigned(int64_t numerator, int64_t denominator) {
  if (numerator >= 0) {
    return static_cast<int32_t>((numerator + (denominator / 2)) / denominator);
  }
  return -static_cast<int32_t>((-numerator + (denominator / 2)) / denominator);
}

static uint32_t roundDivNearestUnsigned(uint64_t numerator, uint64_t denominator) {
  return static_cast<uint32_t>((numerator + (denominator / 2U)) / denominator);
}

static int32_t shuntRawToMicrovolts(int32_t raw, AdcRange range) {
  const int64_t numerator = static_cast<int64_t>(raw) * 5;
  const int64_t denominator = (range == AdcRange::MV_40_96) ? 64 : 16;
  return roundDivNearestSigned(numerator, denominator);
}

static uint32_t busRawToMillivolts(uint32_t raw) {
  return roundDivNearestUnsigned(static_cast<uint64_t>(raw) * 25U, 128U);
}

static int32_t tempRawToMilliC(int16_t raw) {
  return roundDivNearestSigned(static_cast<int64_t>(raw) * 125, 16);
}

static double shuntFullScaleV(AdcRange range) {
  return (range == AdcRange::MV_40_96) ? 0.04096 : 0.16384;
}

static Status encodeShuntThreshold(float voltageV, AdcRange range,
                                   uint16_t& out) {
  if (!std::isfinite(voltageV)) {
    return Status::Error(Err::INVALID_PARAM, "Threshold must be finite");
  }
  const double lsb = (range == AdcRange::MV_40_96)
                         ? cmd::SHUNT_THRESHOLD_LSB_RANGE1
                         : cmd::SHUNT_THRESHOLD_LSB_RANGE0;
  const double scaled = std::round(static_cast<double>(voltageV) / lsb);
  if (scaled < -32768.0 || scaled > 32767.0) {
    return Status::Error(Err::INVALID_PARAM, "Shunt threshold out of range");
  }
  out = static_cast<uint16_t>(static_cast<int16_t>(scaled));
  return Status::Ok();
}

static Status encodeBusThreshold(float voltageV, uint16_t& out) {
  if (!std::isfinite(voltageV) || voltageV < 0.0f || voltageV > 85.0f) {
    return Status::Error(Err::INVALID_PARAM, "Bus threshold out of range");
  }
  out = static_cast<uint16_t>(std::round(
            static_cast<double>(voltageV) / cmd::BUS_THRESHOLD_LSB)) &
        0x7FFFU;
  return Status::Ok();
}

static Status computeCalibration(float shuntOhm, float maxCurrentA, AdcRange range,
                                 uint16_t& shuntCal, float& currentLsb,
                                 bool& clamped, bool& maxCurrentExceedsRange) {
  clamped = false;
  maxCurrentExceedsRange = false;
  Status st = validatePositiveFinite(shuntOhm, "Shunt must be finite and > 0");
  if (!st.ok()) {
    return st;
  }
  st = validatePositiveFinite(maxCurrentA, "Max current must be finite and > 0");
  if (!st.ok()) {
    return st;
  }
  if (!isValidAdcRange(range)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid ADC range");
  }

  const double requestedMaxShuntV =
      static_cast<double>(maxCurrentA) * static_cast<double>(shuntOhm);
  if (!std::isfinite(requestedMaxShuntV)) {
    return Status::Error(Err::INVALID_PARAM, "Calibration out of range");
  }
  maxCurrentExceedsRange = requestedMaxShuntV > shuntFullScaleV(range);

  const double rangeMultiplier = (range == AdcRange::MV_40_96) ? 4.0 : 1.0;
  const double requestedLsb = static_cast<double>(maxCurrentA) / 524288.0;
  double calValue = cmd::SHUNT_CAL_FACTOR * requestedLsb *
                    static_cast<double>(shuntOhm) * rangeMultiplier;
  if (!std::isfinite(calValue) || calValue <= 0.0) {
    return Status::Error(Err::INVALID_PARAM, "Calibration out of range");
  }
  if (calValue > static_cast<double>(cmd::MASK_SHUNT_CAL)) {
    calValue = static_cast<double>(cmd::MASK_SHUNT_CAL);
    clamped = true;
  }

  const uint16_t roundedCal =
      static_cast<uint16_t>(std::round(calValue)) & cmd::MASK_SHUNT_CAL;
  if (roundedCal == 0) {
    return Status::Error(Err::INVALID_PARAM, "Calibration underflows SHUNT_CAL");
  }

  const double actualLsb = static_cast<double>(roundedCal) /
      (cmd::SHUNT_CAL_FACTOR * static_cast<double>(shuntOhm) * rangeMultiplier);
  if (!std::isfinite(actualLsb) || actualLsb <= 0.0) {
    return Status::Error(Err::INVALID_PARAM, "Calibration out of range");
  }
  const float actualLsbFloat = static_cast<float>(actualLsb);
  if (!std::isfinite(actualLsbFloat) || actualLsbFloat <= 0.0f) {
    return Status::Error(Err::INVALID_PARAM, "Calibration out of range");
  }

  shuntCal = roundedCal;
  currentLsb = actualLsbFloat;
  return Status::Ok();
}

static Status buildLegacyCalibrationPlan(float shuntOhm, float maxCurrentA,
                                         AdcRange range, CalibrationPlan& plan,
                                         float& currentLsb) {
  uint16_t shuntCal = 0;
  bool clamped = false;
  bool exceedsShuntRange = false;
  Status st = computeCalibration(shuntOhm, maxCurrentA, range, shuntCal,
                                 currentLsb, clamped, exceedsShuntRange);
  if (!st.ok()) {
    return st;
  }

  CalibrationPlan candidate{};
  candidate.shuntCal = shuntCal;
  candidate.shuntFullScaleMicrovolts =
      range == AdcRange::MV_40_96 ? 40960U : 163840U;
  candidate.clamped = clamped;
  candidate.maxCurrentExceedsShuntRange = exceedsShuntRange;
  if (clamped || exceedsShuntRange) {
    plan = candidate;
    return Status::Ok();
  }

  const double selectedNanoAmps = std::round(
      static_cast<double>(maxCurrentA) * 1.0e9 / 524288.0);
  const double effectiveNanoAmps = std::round(
      static_cast<double>(currentLsb) * 1.0e9);
  const double uint32Max =
      static_cast<double>(std::numeric_limits<uint32_t>::max());
  if (!std::isfinite(selectedNanoAmps) || selectedNanoAmps <= 0.0 ||
      selectedNanoAmps > uint32Max || !std::isfinite(effectiveNanoAmps) ||
      effectiveNanoAmps <= 0.0 || effectiveNanoAmps > uint32Max) {
    return Status::Error(Err::INVALID_PARAM,
                         "Current LSB is not representable in nanoamps");
  }
  candidate.selectedCurrentLsbNanoAmps =
      static_cast<uint32_t>(selectedNanoAmps);
  candidate.effectiveCurrentLsbNanoAmps =
      static_cast<uint32_t>(effectiveNanoAmps);
  candidate.representableCurrentMilliAmps = static_cast<uint32_t>(
      (static_cast<uint64_t>(candidate.effectiveCurrentLsbNanoAmps) *
       524287ULL) / 1000000ULL);
  candidate.quantized = candidate.selectedCurrentLsbNanoAmps !=
                        candidate.effectiveCurrentLsbNanoAmps;
  plan = candidate;
  return Status::Ok();
}

static void parseDiagAlert(uint16_t raw, DiagAlert& out) {
  out.alatch    = (raw & cmd::DIAG_ALATCH) != 0;
  out.cnvr      = (raw & cmd::DIAG_CNVR) != 0;
  out.slowAlert = (raw & cmd::DIAG_SLOWALERT) != 0;
  out.apol      = (raw & cmd::DIAG_APOL) != 0;
  out.energyOF  = (raw & cmd::DIAG_ENERGYOF) != 0;
  out.chargeOF  = (raw & cmd::DIAG_CHARGEOF) != 0;
  out.mathOF    = (raw & cmd::DIAG_MATHOF) != 0;
  out.tmpOL     = (raw & cmd::DIAG_TMPOL) != 0;
  out.shntOL    = (raw & cmd::DIAG_SHNTOL) != 0;
  out.shntUL    = (raw & cmd::DIAG_SHNTUL) != 0;
  out.busOL     = (raw & cmd::DIAG_BUSOL) != 0;
  out.busUL     = (raw & cmd::DIAG_BUSUL) != 0;
  out.pOL       = (raw & cmd::DIAG_POL) != 0;
  out.cnvrf     = (raw & cmd::DIAG_CNVRF) != 0;
  out.memstat   = (raw & cmd::DIAG_MEMSTAT) != 0;
}

static Status mapPresenceReadFailure(const Status& st, const char* message) {
  if (st.code == Err::I2C_NACK_ADDR) {
    return Status::Error(Err::DEVICE_NOT_FOUND, message, st.detail);
  }
  return st;
}

}  // namespace

// ===========================================================================
// Lifecycle
// ===========================================================================

Status INA228::calculateCalibration(const CalibrationConfig& config,
                                    AdcRange range, CalibrationPlan& out) {
  CalibrationPlan plan{};
  if (!isValidAdcRange(range)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid ADC range");
  }
  plan.shuntFullScaleMicrovolts =
      range == AdcRange::MV_40_96 ? 40960U : 163840U;
  if (config.mode == CalibrationMode::NONE) {
    if (config.shuntMicroOhms != 0 || config.maxCurrentMilliAmps != 0 ||
        config.currentLsbNanoAmps != 0) {
      return Status::Error(Err::INVALID_PARAM,
                           "Uncalibrated mode requires zero calibration fields");
    }
    out = plan;
    return Status::Ok();
  }
  if (config.mode != CalibrationMode::FROM_MAXIMUM_CURRENT &&
      config.mode != CalibrationMode::EXPLICIT_CURRENT_LSB) {
    return Status::Error(Err::INVALID_PARAM, "Invalid calibration mode");
  }
  if (config.shuntMicroOhms == 0 || config.maxCurrentMilliAmps == 0) {
    return Status::Error(Err::INVALID_PARAM,
                         "Shunt and maximum current must be nonzero");
  }

  uint64_t selectedNanoAmps = 0;
  if (config.mode == CalibrationMode::FROM_MAXIMUM_CURRENT) {
    const uint64_t numerator =
        static_cast<uint64_t>(config.maxCurrentMilliAmps) * 1000000ULL;
    selectedNanoAmps = (numerator + 524286ULL) / 524287ULL;
  } else {
    selectedNanoAmps = config.currentLsbNanoAmps;
  }
  if (selectedNanoAmps == 0 ||
      selectedNanoAmps > std::numeric_limits<uint32_t>::max()) {
    return Status::Error(Err::INVALID_PARAM, "CURRENT_LSB is out of range");
  }
  plan.selectedCurrentLsbNanoAmps =
      static_cast<uint32_t>(selectedNanoAmps);

  const uint64_t requestedShuntMicrovolts =
      (static_cast<uint64_t>(config.maxCurrentMilliAmps) *
       config.shuntMicroOhms + 999ULL) / 1000ULL;
  plan.maxCurrentExceedsShuntRange =
      requestedShuntMicrovolts > plan.shuntFullScaleMicrovolts;

  const double rangeMultiplier =
      range == AdcRange::MV_40_96 ? 4.0 : 1.0;
  double requestedCal = cmd::SHUNT_CAL_FACTOR *
      (static_cast<double>(selectedNanoAmps) * 1.0e-9) *
      (static_cast<double>(config.shuntMicroOhms) * 1.0e-6) *
      rangeMultiplier;
  if (!std::isfinite(requestedCal) || requestedCal <= 0.0) {
    return Status::Error(Err::INVALID_PARAM, "Calibration is out of range");
  }
  if (requestedCal > cmd::MASK_SHUNT_CAL) {
    plan.clamped = true;
    requestedCal = cmd::MASK_SHUNT_CAL;
  }
  const uint32_t rounded = static_cast<uint32_t>(
      config.mode == CalibrationMode::FROM_MAXIMUM_CURRENT
          ? std::ceil(requestedCal)
          : std::round(requestedCal));
  if (rounded == 0) {
    return Status::Error(Err::INVALID_PARAM, "Calibration underflows SHUNT_CAL");
  }
  plan.shuntCal = static_cast<uint16_t>(rounded) & cmd::MASK_SHUNT_CAL;

  const double effectiveNanoAmps =
      (static_cast<double>(plan.shuntCal) * 1.0e9) /
      (cmd::SHUNT_CAL_FACTOR *
       (static_cast<double>(config.shuntMicroOhms) * 1.0e-6) *
       rangeMultiplier);
  if (!std::isfinite(effectiveNanoAmps) || effectiveNanoAmps <= 0.0 ||
      effectiveNanoAmps > std::numeric_limits<uint32_t>::max()) {
    return Status::Error(Err::INVALID_PARAM, "Effective CURRENT_LSB is out of range");
  }
  plan.effectiveCurrentLsbNanoAmps =
      static_cast<uint32_t>(std::round(effectiveNanoAmps));
  plan.representableCurrentMilliAmps = static_cast<uint32_t>(
      (static_cast<uint64_t>(plan.effectiveCurrentLsbNanoAmps) * 524287ULL) /
      1000000ULL);
  plan.maxCurrentExceedsCurrentRegister =
      config.maxCurrentMilliAmps > plan.representableCurrentMilliAmps;
  plan.quantized = plan.effectiveCurrentLsbNanoAmps !=
                   plan.selectedCurrentLsbNanoAmps;

  if (!config.allowUnsafePlan &&
      (plan.clamped || plan.maxCurrentExceedsShuntRange ||
       plan.maxCurrentExceedsCurrentRegister)) {
    return Status::Error(Err::INVALID_CONFIG,
                         plan.clamped ? "SHUNT_CAL would clamp" :
                         plan.maxCurrentExceedsShuntRange ?
                           "Maximum current exceeds shunt range" :
                           "CURRENT_LSB cannot represent maximum current");
  }
  out = plan;
  return Status::Ok();
}

Status INA228::parseDeviceIdentity(uint16_t manufacturerId, uint16_t deviceId,
                                   DeviceIdentity& out) {
  if (manufacturerId != cmd::MANUFACTURER_ID) {
    return Status::Error(Err::DEVICE_ID_MISMATCH,
                         "Manufacturer ID mismatch", manufacturerId);
  }
  const uint16_t dieId = static_cast<uint16_t>(
      (deviceId & cmd::DEVICE_DIE_ID_MASK) >> 4);
  if (dieId != cmd::DEVICE_DIE_ID) {
    return Status::Error(Err::DEVICE_ID_MISMATCH,
                         "INA228 die ID mismatch", deviceId);
  }
  DeviceIdentity parsed{};
  parsed.manufacturerId = manufacturerId;
  parsed.dieId = dieId;
  parsed.revision = static_cast<uint8_t>(deviceId & cmd::DEVICE_REV_ID_MASK);
  out = parsed;
  return Status::Ok();
}

Status INA228::_validateBinding(const Config& config, CalibrationPlan& plan,
                                bool& usesFixedCalibration) const {
  if (config.i2cWrite == nullptr || config.i2cWriteRead == nullptr) {
    return Status::Error(Err::INVALID_CONFIG, "I2C callbacks not set");
  }
  if (config.i2cTimeoutMs == 0 || !isValidAddress(config.i2cAddress)) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid I2C address or timeout");
  }
  if (!isValidMode(config.mode) || !isValidConvTime(config.vbusConvTime) ||
      !isValidConvTime(config.vshuntConvTime) ||
      !isValidConvTime(config.vtempConvTime) ||
      !isValidAveraging(config.averaging) || !isValidAdcRange(config.adcRange)) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid ADC configuration");
  }
  if (config.shuntTempCoeffPpmC > cmd::TEMPCO_MAX ||
      config.supportedRevisionMask == 0) {
    return Status::Error(Err::INVALID_CONFIG,
                         "Invalid temperature coefficient or revision policy");
  }
  usesFixedCalibration = config.calibration.mode != CalibrationMode::NONE ||
      config.calibration.shuntMicroOhms != 0 ||
      config.calibration.maxCurrentMilliAmps != 0 ||
      config.calibration.currentLsbNanoAmps != 0;
  if (usesFixedCalibration) {
    if (config.shuntResistanceOhm != 0.0f || config.maxExpectedCurrentA != 0.0f) {
      return Status::Error(Err::INVALID_CONFIG,
                           "Choose fixed-unit or legacy calibration, not both");
    }
    Status st = calculateCalibration(config.calibration, config.adcRange, plan);
    return st.ok() ? st : Status::Error(Err::INVALID_CONFIG, st.msg, st.detail);
  }
  if (!std::isfinite(config.shuntResistanceOhm) ||
      !std::isfinite(config.maxExpectedCurrentA) ||
      config.shuntResistanceOhm < 0.0f || config.maxExpectedCurrentA < 0.0f ||
      ((config.shuntResistanceOhm > 0.0f) !=
       (config.maxExpectedCurrentA > 0.0f))) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid legacy calibration");
  }
  if (config.shuntResistanceOhm == 0.0f) {
    plan = CalibrationPlan{};
    plan.shuntFullScaleMicrovolts =
        config.adcRange == AdcRange::MV_40_96 ? 40960U : 163840U;
    return Status::Ok();
  }
  float legacyCurrentLsb = 0.0f;
  Status st = buildLegacyCalibrationPlan(
      config.shuntResistanceOhm, config.maxExpectedCurrentA, config.adcRange,
      plan, legacyCurrentLsb);
  if (!st.ok()) {
    return Status::Error(Err::INVALID_CONFIG, st.msg, st.detail);
  }
  if (plan.clamped || plan.maxCurrentExceedsShuntRange) {
    return Status::Error(Err::INVALID_CONFIG,
                         plan.clamped ? "Legacy SHUNT_CAL would clamp" :
                                        "Legacy maximum current exceeds shunt range");
  }
  return Status::Ok();
}

Status INA228::bind(const Config& config) {
  if (_cooperativeJobActive() || _terminalResultAvailable) {
    return Status::Error(Err::BUSY, "Consume or cancel current job before bind");
  }
  CalibrationPlan plan{};
  bool usesFixed = false;
  Status st = _validateBinding(config, plan, usesFixed);
  if (!st.ok()) return st;

  _config = config;
  if (_config.offlineThreshold == 0) _config.offlineThreshold = 1;
  _bound = true;
  _initialized = false;
  _driverState = DriverState::UNINIT;
  _hardwareState = HardwareState::UNKNOWN;
  _deviceIdentity = DeviceIdentity{};
  _deviceIdentityValid = false;
  _calibrationPlan = plan;
  _usesFixedCalibration = usesFixed;
  _shuntCal = plan.shuntCal;
  _currentLsb = _plannedCurrentLsbAmps();
  _calibrationClamped = plan.clamped;
  _maxCurrentExceedsShuntRange = plan.maxCurrentExceedsShuntRange;
  _diagAlertConfigBits = _desiredDiagConfigBits();
  _diagAlertSnapshot = DiagAlertSnapshot{};
  _diagnosticEvents = DiagnosticEvents{};
  _clearHardwareDirty();
  _thresholdsDirty = false;
  _hardwareDirty = true;
  _hardwareDirtyCause = Status::Error(
      Err::HARDWARE_STATE_UNKNOWN, "Bound hardware has not been verified");
  _trigPending = false;
  _trigStartMs = 0;
  _accumulationReady = false;
  _configurationGeneration = 0;
  _accumulatorGeneration = 0;
  _lastOkMs = 0;
  _lastErrorMs = 0;
  _lastError = Status::Ok();
  _consecutiveFailures = 0;
  _totalFailures = 0;
  _totalSuccess = 0;
  _clearCooperativeState();
  return Status::Ok();
}

uint16_t INA228::_desiredDiagConfigBits() const {
  uint16_t bits = 0;
  if (_config.alerts.latched) bits |= cmd::DIAG_ALATCH;
  if (_config.alerts.conversionReady) bits |= cmd::DIAG_CNVR;
  if (_config.alerts.slowAlert) bits |= cmd::DIAG_SLOWALERT;
  if (_config.alerts.activeHigh) bits |= cmd::DIAG_APOL;
  return bits;
}

uint32_t INA228::_instantaneousSampleWaitUs() const {
  return (static_cast<uint32_t>(convTimeUs(_config.vbusConvTime)) +
          convTimeUs(_config.vshuntConvTime) +
          convTimeUs(_config.vtempConvTime)) * avgCount(_config.averaging) +
         static_cast<uint32_t>(_config.convDelayMs2) * 2000U +
         cmd::SHUTDOWN_WAKEUP_US;
}

uint32_t INA228::_instantaneousSampleWaitMs() const {
  return (_instantaneousSampleWaitUs() + 999U) / 1000U;
}

float INA228::_plannedCurrentLsbAmps() const {
  if (_calibrationPlan.shuntCal == 0) return 0.0f;
  const double shuntOhms = _usesFixedCalibration
      ? static_cast<double>(_config.calibration.shuntMicroOhms) * 1.0e-6
      : static_cast<double>(_config.shuntResistanceOhm);
  if (shuntOhms <= 0.0) return 0.0f;
  const double rangeMultiplier =
      _config.adcRange == AdcRange::MV_40_96 ? 4.0 : 1.0;
  return static_cast<float>(static_cast<double>(_calibrationPlan.shuntCal) /
      (cmd::SHUNT_CAL_FACTOR * shuntOhms * rangeMultiplier));
}

uint32_t INA228::_nextOperationIdValue() {
  const uint32_t value = _nextOperationId == 0 ? 1 : _nextOperationId;
  _nextOperationId = value == std::numeric_limits<uint32_t>::max()
      ? 1
      : value + 1U;
  return value;
}

bool INA228::_cooperativeJobActive() const {
  return _jobSnapshot.state == JobState::ACTIVE;
}

bool INA228::_hardwareAccessAllowed() const {
  return _jobPollActive ||
         (!_cooperativeJobActive() && !_terminalResultAvailable);
}

void INA228::_clearCooperativeState() {
  _jobSnapshot = JobSnapshot{};
  _jobPhase = JobPhase::IDLE;
  _terminalResult = JobResult{};
  _terminalResultAvailable = false;
  _jobPollActive = false;
  _jobHadSuccessfulWrite = false;
  _jobTouchedRegisterMask = 0;
  _jobIdentityScratch = DeviceIdentity{};
  _sampleScratch = InstantaneousSample{};
  _jobWaitStartMs = 0;
  _deferredTimeOrigin = DeferredTimeOrigin::NONE;
  _jobDeferredStatus = Status::Ok();
}

void INA228::_setJobPhase(JobPhase phase) {
  _jobPhase = phase;
  _jobSnapshot.phase = static_cast<uint16_t>(phase);
}

Status INA228::_startJob(JobKind kind, JobPhase firstPhase,
                         uint32_t requestToken, uint32_t& operationId) {
  if (!_bound) {
    return Status::Error(Err::NOT_BOUND, "bind() has not completed");
  }
  if (_cooperativeJobActive() || _terminalResultAvailable) {
    return Status::Error(Err::BUSY, "A job or unconsumed result already exists");
  }

  const uint32_t id = _nextOperationIdValue();
  _jobSnapshot = JobSnapshot{};
  _jobSnapshot.kind = kind;
  _jobSnapshot.state = JobState::ACTIVE;
  _jobSnapshot.operationId = id;
  _jobSnapshot.requestToken = requestToken;
  _jobSnapshot.startConfigurationGeneration = _configurationGeneration;
  _jobSnapshot.configurationGeneration = _configurationGeneration;
  _jobSnapshot.startedAtMs = _nowMs();
  _jobSnapshot.status = Status{Err::IN_PROGRESS, 0, "Job in progress"};
  _setJobPhase(firstPhase);
  _jobHadSuccessfulWrite = false;
  _jobTouchedRegisterMask = 0;
  _jobIdentityScratch = DeviceIdentity{};
  _sampleScratch = InstantaneousSample{};
  _sampleScratch.operationId = id;
  _sampleScratch.requestToken = requestToken;
  _sampleScratch.configurationGeneration = _configurationGeneration;
  _jobWaitStartMs = 0;
  if (_deferredTimeOrigin == DeferredTimeOrigin::JOB_WAIT) {
    _deferredTimeOrigin = DeferredTimeOrigin::NONE;
  }
  _jobDeferredStatus = Status::Ok();
  operationId = id;
  return Status::Ok();
}

Status INA228::startInitialize(uint32_t requestToken, uint32_t& operationId) {
  return _startJob(JobKind::INITIALIZE, JobPhase::READ_MANUFACTURER,
                   requestToken, operationId);
}

Status INA228::startReinitialize(uint32_t requestToken, uint32_t& operationId) {
  return _startJob(JobKind::REINITIALIZE, JobPhase::READ_MANUFACTURER,
                   requestToken, operationId);
}

Status INA228::startVerifyConfiguration(uint32_t requestToken,
                                        uint32_t& operationId) {
  if (!_initialized || _hardwareState != HardwareState::SYNCHRONIZED) {
    return Status::Error(Err::HARDWARE_STATE_UNKNOWN,
                         "Initialize hardware before verification");
  }
  return _startJob(JobKind::VERIFY_CONFIGURATION,
                   JobPhase::READ_MANUFACTURER, requestToken, operationId);
}

Status INA228::startInstantaneousSample(uint32_t requestToken,
                                        uint32_t& operationId) {
  if (!_initialized || _hardwareState != HardwareState::SYNCHRONIZED) {
    return Status::Error(Err::HARDWARE_STATE_UNKNOWN,
                         "Initialize hardware before sampling");
  }
  if (_calibrationPlan.shuntCal == 0) {
    return Status::Error(Err::INVALID_CONFIG,
                         "Instantaneous current/power sample requires calibration");
  }
  if (isTriggeredMode(_config.mode)) {
    return Status::Error(Err::INVALID_CONFIG,
                         "Instantaneous job requires shutdown or continuous base mode");
  }
  return _startJob(JobKind::INSTANTANEOUS_SAMPLE,
                   JobPhase::SAMPLE_VERIFY_CONFIG, requestToken, operationId);
}

Status INA228::startReset(uint32_t requestToken, uint32_t& operationId) {
  if (!_initialized || _hardwareState != HardwareState::SYNCHRONIZED) {
    return Status::Error(Err::HARDWARE_STATE_UNKNOWN,
                         "Initialize hardware before reset");
  }
  return _startJob(JobKind::RESET, JobPhase::RESET_WRITE,
                   requestToken, operationId);
}

Status INA228::startAccumulatorReset(uint32_t requestToken,
                                     uint32_t& operationId) {
  if (!_initialized || _hardwareState != HardwareState::SYNCHRONIZED) {
    return Status::Error(Err::HARDWARE_STATE_UNKNOWN,
                         "Initialize hardware before accumulator reset");
  }
  return _startJob(JobKind::ACCUMULATOR_RESET,
                   JobPhase::ACCUMULATOR_WRITE, requestToken, operationId);
}

Status INA228::_finishJob(const Status& status, JobState state,
                          JobEffect effect, uint32_t nowMs) {
  _jobSnapshot.state = state;
  _jobSnapshot.effect = effect;
  _jobSnapshot.status = status;
  _jobSnapshot.finishedAtMs = nowMs;
  _jobSnapshot.configurationGeneration = _configurationGeneration;
  _jobSnapshot.resultAvailable = true;
  _terminalResult = JobResult{};
  _terminalResult.job = _jobSnapshot;
  if (_jobSnapshot.kind == JobKind::INSTANTANEOUS_SAMPLE &&
      _sampleScratch.raw.diagAlertValid) {
    _terminalResult.hasInstantaneousSample = true;
    _terminalResult.instantaneousSample = _sampleScratch;
  }
  _terminalResultAvailable = true;
  _jobPollActive = false;
  return status;
}

Status INA228::_failJob(const Status& status, bool failedSideEffectingTransfer,
                        uint32_t nowMs) {
  const bool identityJob = _jobSnapshot.kind == JobKind::INITIALIZE ||
                           _jobSnapshot.kind == JobKind::REINITIALIZE;
  const bool resetMayHaveInvalidatedTrigger =
      _jobSnapshot.kind == JobKind::RESET &&
      (failedSideEffectingTransfer || _jobHadSuccessfulWrite);
  JobEffect effect = JobEffect::NONE;
  if (failedSideEffectingTransfer) {
    effect = JobEffect::INDETERMINATE;
  } else if (_jobHadSuccessfulWrite) {
    effect = JobEffect::PARTIAL;
  }
  if (failedSideEffectingTransfer || _jobHadSuccessfulWrite || identityJob ||
      _jobSnapshot.kind == JobKind::VERIFY_CONFIGURATION ||
      _jobSnapshot.kind == JobKind::INSTANTANEOUS_SAMPLE) {
    _invalidateJobHardwareState(status);
  }
  if (_deferredTimeOrigin == DeferredTimeOrigin::JOB_WAIT) {
    _deferredTimeOrigin = DeferredTimeOrigin::NONE;
  }
  if (identityJob || resetMayHaveInvalidatedTrigger) {
    _invalidateTriggeredConversionTiming();
  }
  return _finishJob(status, JobState::FAILED, effect, nowMs);
}

Status INA228::_cancelJob(const Status& status, JobState state) {
  if (!_cooperativeJobActive()) {
    return Status::Ok();
  }
  JobEffect effect = JobEffect::NONE;
  const bool identityJob = _jobSnapshot.kind == JobKind::INITIALIZE ||
                           _jobSnapshot.kind == JobKind::REINITIALIZE;
  const bool resetInvalidatedTrigger =
      _jobSnapshot.kind == JobKind::RESET && _jobHadSuccessfulWrite;
  if (_jobHadSuccessfulWrite || identityJob) {
    if (_jobHadSuccessfulWrite) effect = JobEffect::PARTIAL;
    _invalidateJobHardwareState(status);
  }
  if (_deferredTimeOrigin == DeferredTimeOrigin::JOB_WAIT) {
    _deferredTimeOrigin = DeferredTimeOrigin::NONE;
  }
  if (identityJob || resetInvalidatedTrigger) {
    _invalidateTriggeredConversionTiming();
  }
  _sampleScratch = InstantaneousSample{};
  return _finishJob(status, state, effect, _nowMs());
}

void INA228::_invalidateJobHardwareState(const Status& cause) {
  _hardwareState = HardwareState::RESYNC_REQUIRED;
  _initialized = false;
  _driverState = DriverState::UNINIT;
  _hardwareDirty = true;
  _dirtyRegisterMask |= _jobTouchedRegisterMask;
  _hardwareDirtyCause = cause;
  _deviceIdentityValid = false;
  _invalidateTriggeredConversionTiming();
  _invalidateAccumulatorEpoch();
}

Status INA228::cancelJob() {
  return _cancelJob(Status::Error(Err::CANCELLED, "Job cancelled by owner"),
                    JobState::CANCELLED);
}

Status INA228::timeoutJob() {
  return _cancelJob(Status::Error(Err::OPERATION_TIMEOUT,
                                  "Owner operation deadline expired"),
                    JobState::TIMED_OUT);
}

Status INA228::getJobState(JobSnapshot& out) const {
  out = _jobSnapshot;
  out.resultAvailable = _terminalResultAvailable;
  return Status::Ok();
}

Status INA228::getJobLimits(JobKind kind, JobLimits& out) const {
  if (!_bound) {
    return Status::Error(Err::NOT_BOUND, "bind() has not completed");
  }
  JobLimits limits{};
  switch (kind) {
    case JobKind::INITIALIZE:
    case JobKind::REINITIALIZE:
      limits.maxTransfers = 14;
      break;
    case JobKind::VERIFY_CONFIGURATION:
      limits.maxTransfers = 8;
      break;
    case JobKind::INSTANTANEOUS_SAMPLE:
      limits.operationClass = OperationClass::STEADY_STATE;
      limits.maxTransfers = 11;
      limits.maxWaitMicroseconds = _instantaneousSampleWaitMs() * 1000U;
      break;
    case JobKind::RESET:
      limits.operationClass = OperationClass::MAINTENANCE;
      limits.maxTransfers = 16;
      limits.maxWaitMicroseconds = RESET_STARTUP_MS * 1000U;
      break;
    case JobKind::ACCUMULATOR_RESET:
      limits.operationClass = OperationClass::MULTI_STEP_RUNTIME;
      limits.maxTransfers = 2;
      break;
    case JobKind::NONE:
      return Status::Error(Err::INVALID_PARAM, "NONE has no job limits");
  }
  out = limits;
  return Status::Ok();
}

Status INA228::takeJobResult(uint32_t expectedOperationId, JobResult& out) {
  if (expectedOperationId == 0) {
    return Status::Error(Err::INVALID_PARAM, "Operation ID must be nonzero");
  }
  if (_cooperativeJobActive()) {
    if (_jobSnapshot.operationId != expectedOperationId) {
      return Status::Error(Err::STALE_RESULT, "Operation ID does not match active job");
    }
    return Status{Err::IN_PROGRESS, 0, "Job has not reached a terminal state"};
  }
  if (!_terminalResultAvailable) {
    return Status::Error(Err::RESULT_NOT_AVAILABLE,
                         "Terminal result already consumed or unavailable");
  }
  if (_terminalResult.job.operationId != expectedOperationId) {
    return Status::Error(Err::STALE_RESULT,
                         "Operation ID does not match terminal result");
  }
  out = _terminalResult;
  _terminalResultAvailable = false;
  _jobSnapshot.resultAvailable = false;
  _terminalResult = JobResult{};
  return Status::Ok();
}

Status INA228::invalidateHardwareState(const Status& cause) {
  if (!_bound) {
    return Status::Error(Err::NOT_BOUND, "bind() has not completed");
  }
  Status reason = cause.ok() ?
      Status::Error(Err::HARDWARE_STATE_UNKNOWN,
                    "Hardware state invalidated by owner") : cause;
  Status result = Status::Ok();
  if (_cooperativeJobActive()) {
    result = _cancelJob(reason, JobState::FAILED);
  }
  // Keep any register evidence already accumulated: invalidation makes the
  // hardware less trusted, never more, and hardwareDirtyCause documents the
  // FIRST cause. Clearing them here would discard what a failed job just
  // recorded.
  if (!_hardwareDirty) {
    _hardwareDirtyCause = reason;
  }
  _hardwareState = HardwareState::RESYNC_REQUIRED;
  _initialized = false;
  _driverState = DriverState::UNINIT;
  _hardwareDirty = true;
  _deviceIdentityValid = false;
  _invalidateAccumulatorEpoch();
  _trigPending = false;
  _trigStartMs = 0;
  _deferredTimeOrigin = DeferredTimeOrigin::NONE;
  return result;
}

Status INA228::getDeviceIdentity(DeviceIdentity& out) const {
  if (!_deviceIdentityValid || !_initialized ||
      _hardwareState != HardwareState::SYNCHRONIZED) {
    return Status::Error(Err::HARDWARE_STATE_UNKNOWN,
                         "Verified identity is not available");
  }
  out = _deviceIdentity;
  return Status::Ok();
}

Status INA228::getCalibrationPlan(CalibrationPlan& out) const {
  if (!_bound) {
    return Status::Error(Err::NOT_BOUND, "bind() has not completed");
  }
  out = _calibrationPlan;
  return Status::Ok();
}

Status INA228::getDiagnosticEvents(DiagnosticEvents& out) const {
  out = _diagnosticEvents;
  return Status::Ok();
}

Status INA228::acknowledgeDiagnosticEvents(uint16_t mask) {
  const uint16_t acknowledged = mask & DIAG_EVIDENCE_MASK;
  _diagnosticEvents.stickyEvents &= static_cast<uint16_t>(~acknowledged);
  _diagnosticEvents.newlyObservedEvents &= static_cast<uint16_t>(~acknowledged);
  for (uint8_t bit = 0; bit < 16; ++bit) {
    if ((acknowledged & (static_cast<uint16_t>(1U) << bit)) != 0) {
      _diagnosticEvents.firstObservedAtMs[bit] = 0;
    }
  }
  return Status::Ok();
}

Status INA228::_verifyRegister(uint8_t reg, uint16_t actual,
                               uint16_t expected, uint16_t mask) const {
  if ((actual & mask) != (expected & mask)) {
    return Status::Error(Err::CONFIG_MISMATCH,
                         "Register readback mismatch",
                         (static_cast<int32_t>(reg) << 16) | actual);
  }
  return Status::Ok();
}

Status INA228::_pollJobTransfer(uint32_t nowMs) {
  ++_jobSnapshot.transfersCompleted;
  Status st = Status::Ok();
  uint16_t raw16 = 0;
  uint32_t raw24 = 0;

  switch (_jobPhase) {
    case JobPhase::READ_MANUFACTURER:
      st = readReg16(cmd::REG_MANUFACTURER_ID, raw16);
      if (!st.ok()) {
        return _failJob(mapPresenceReadFailure(st, "Manufacturer ID read failed"),
                        false, nowMs);
      }
      _jobIdentityScratch = DeviceIdentity{};
      _jobIdentityScratch.manufacturerId = raw16;
      _setJobPhase(JobPhase::READ_DEVICE);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::READ_DEVICE: {
      st = readReg16(cmd::REG_DEVICE_ID, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      DeviceIdentity parsed{};
      st = parseDeviceIdentity(_jobIdentityScratch.manufacturerId, raw16,
                               parsed);
      if (!st.ok()) return _failJob(st, false, nowMs);
      if ((_config.supportedRevisionMask &
           (static_cast<uint16_t>(1U) << parsed.revision)) == 0) {
        return _failJob(Status::Error(Err::UNSUPPORTED_REVISION,
                                      "Unsupported INA228 revision",
                                      parsed.revision), false, nowMs);
      }
      _jobIdentityScratch = parsed;
      _setJobPhase(JobPhase::READ_DIAG);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};
    }

    case JobPhase::READ_DIAG:
      st = readReg16(cmd::REG_DIAG_ALRT, raw16);
      if (!st.ok()) return _failJob(st, true, nowMs);
      _captureDiagAlert(raw16, nowMs);
      if ((raw16 & cmd::DIAG_MEMSTAT) == 0) {
        return _failJob(Status::Error(Err::MEMORY_ERROR,
                                      "NV trim memory checksum error"),
                        false, nowMs);
      }
      if (_jobSnapshot.kind == JobKind::VERIFY_CONFIGURATION) {
        _setJobPhase(JobPhase::VERIFY_CONFIG);
      } else {
        _setJobPhase(JobPhase::WRITE_ADC_SHUTDOWN);
      }
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::WRITE_ADC_SHUTDOWN: {
      _jobTouchedRegisterMask |= (1ULL << cmd::REG_ADC_CONFIG);
      const uint16_t shutdown = static_cast<uint16_t>(
          _buildAdcConfig() & static_cast<uint16_t>(~cmd::MASK_ADC_MODE));
      st = writeReg16(cmd::REG_ADC_CONFIG, shutdown);
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      _setJobPhase(JobPhase::WRITE_CONFIG);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};
    }

    case JobPhase::WRITE_CONFIG:
      _jobTouchedRegisterMask |= (1ULL << cmd::REG_CONFIG);
      st = writeReg16(cmd::REG_CONFIG, _buildConfig());
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      _setJobPhase(JobPhase::WRITE_DIAG);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::WRITE_DIAG:
      _jobTouchedRegisterMask |= (1ULL << cmd::REG_DIAG_ALRT);
      st = writeReg16(cmd::REG_DIAG_ALRT, _desiredDiagConfigBits());
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      _diagAlertConfigBits = _desiredDiagConfigBits();
      _setJobPhase(JobPhase::WRITE_TEMPCO);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::WRITE_TEMPCO:
      _jobTouchedRegisterMask |= (1ULL << cmd::REG_SHUNT_TEMPCO);
      st = writeReg16(cmd::REG_SHUNT_TEMPCO,
                      _config.shuntTempCoeffPpmC & cmd::MASK_SHUNT_TEMPCO);
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      _setJobPhase(JobPhase::WRITE_SHUNT_CAL);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::WRITE_SHUNT_CAL:
      _jobTouchedRegisterMask |= (1ULL << cmd::REG_SHUNT_CAL);
      st = writeReg16(cmd::REG_SHUNT_CAL, _calibrationPlan.shuntCal);
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      _setJobPhase(JobPhase::WRITE_ADC_CONFIG);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::WRITE_ADC_CONFIG:
      _jobTouchedRegisterMask |= (1ULL << cmd::REG_ADC_CONFIG);
      st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      if (isTriggeredMode(_config.mode)) {
        _markTriggeredConversionStarted();
      }
      _setJobPhase(JobPhase::VERIFY_CONFIG);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::VERIFY_CONFIG:
      st = readReg16(cmd::REG_CONFIG, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      st = _verifyRegister(cmd::REG_CONFIG, raw16, _buildConfig(),
          cmd::MASK_CONFIG_CONVDLY | cmd::CONFIG_TEMPCOMP |
          cmd::CONFIG_ADCRANGE | cmd::CONFIG_RST | cmd::CONFIG_RSTACC);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _setJobPhase(JobPhase::VERIFY_ADC_CONFIG);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::VERIFY_ADC_CONFIG:
      st = readReg16(cmd::REG_ADC_CONFIG, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      st = _verifyRegister(cmd::REG_ADC_CONFIG, raw16,
                           _buildAdcConfig(), 0xFFFFU);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _setJobPhase(JobPhase::VERIFY_SHUNT_CAL);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::VERIFY_SHUNT_CAL:
      st = readReg16(cmd::REG_SHUNT_CAL, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      st = _verifyRegister(cmd::REG_SHUNT_CAL, raw16,
                           _calibrationPlan.shuntCal, cmd::MASK_SHUNT_CAL);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _setJobPhase(JobPhase::VERIFY_DIAG);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::VERIFY_DIAG:
      st = readReg16(cmd::REG_DIAG_ALRT, raw16);
      if (!st.ok()) return _failJob(st, true, nowMs);
      _captureDiagAlert(raw16, nowMs);
      if ((raw16 & cmd::DIAG_MEMSTAT) == 0) {
        return _failJob(Status::Error(Err::MEMORY_ERROR,
                                      "NV trim memory checksum error"),
                        false, nowMs);
      }
      st = _verifyRegister(cmd::REG_DIAG_ALRT, raw16,
                           _desiredDiagConfigBits() | cmd::DIAG_MEMSTAT,
                           cmd::DIAG_CONFIG_MASK | cmd::DIAG_MEMSTAT);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _setJobPhase(JobPhase::VERIFY_TEMPCO);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::VERIFY_TEMPCO:
      st = readReg16(cmd::REG_SHUNT_TEMPCO, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      st = _verifyRegister(cmd::REG_SHUNT_TEMPCO, raw16,
                           _config.shuntTempCoeffPpmC,
                           cmd::MASK_SHUNT_TEMPCO);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _hardwareState = HardwareState::SYNCHRONIZED;
      _initialized = true;
      _driverState = DriverState::READY;
      _deviceIdentity = _jobIdentityScratch;
      _deviceIdentityValid = true;
      _clearHardwareDirty();
      if (_jobSnapshot.kind != JobKind::VERIFY_CONFIGURATION) {
        ++_configurationGeneration;
        if (_configurationGeneration == 0) ++_configurationGeneration;
        _invalidateAccumulatorEpoch();
      }
      _shuntCal = _calibrationPlan.shuntCal;
      _currentLsb = _plannedCurrentLsbAmps();
      return _finishJob(Status::Ok(), JobState::SUCCEEDED,
                        JobEffect::CONFIRMED, nowMs);

    case JobPhase::SAMPLE_VERIFY_CONFIG:
      st = readReg16(cmd::REG_CONFIG, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      st = _verifyRegister(cmd::REG_CONFIG, raw16, _buildConfig(),
          cmd::MASK_CONFIG_CONVDLY | cmd::CONFIG_TEMPCOMP |
          cmd::CONFIG_ADCRANGE | cmd::CONFIG_RST | cmd::CONFIG_RSTACC);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _setJobPhase(JobPhase::SAMPLE_VERIFY_SHUNT_CAL);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::SAMPLE_VERIFY_SHUNT_CAL:
      st = readReg16(cmd::REG_SHUNT_CAL, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      st = _verifyRegister(cmd::REG_SHUNT_CAL, raw16,
                           _calibrationPlan.shuntCal, cmd::MASK_SHUNT_CAL);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _setJobPhase(JobPhase::SAMPLE_TRIGGER);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::SAMPLE_TRIGGER:
      _jobTouchedRegisterMask |= (1ULL << cmd::REG_ADC_CONFIG);
      st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig(Mode::TRIG_ALL));
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      _armPostWriteTimeOrigin(DeferredTimeOrigin::JOB_WAIT);
      _invalidateAccumulatorEpoch();
      _clearCapturedConversionReadyFlag();
      _setJobPhase(JobPhase::SAMPLE_WAIT);
      return Status{Err::IN_PROGRESS, 0, "Conversion wait in progress"};

    case JobPhase::SAMPLE_DIAG:
      st = readReg16(cmd::REG_DIAG_ALRT, raw16);
      if (!st.ok()) return _failJob(st, true, nowMs);
      _captureDiagAlert(raw16, nowMs);
      _sampleScratch.raw.diagAlertValid = true;
      _sampleScratch.raw.diagAlertRaw = raw16;
      _sampleScratch.raw.mathOverflow = (raw16 & cmd::DIAG_MATHOF) != 0;
      _sampleScratch.diagnostics = _diagnosticEvents;
      _sampleScratch.diagnostics.latestRaw = raw16;
      _sampleScratch.capturedAtMs = nowMs;
      if ((raw16 & cmd::DIAG_MEMSTAT) == 0) {
        _jobDeferredStatus = Status::Error(
            Err::MEMORY_ERROR, "NV trim memory checksum error");
        _setJobPhase(JobPhase::SAMPLE_RESTORE_ADC);
      } else if ((raw16 & cmd::DIAG_CNVRF) == 0) {
        _jobDeferredStatus = Status::Error(
            Err::MEASUREMENT_NOT_READY, "CNVRF not set after conversion deadline");
        _setJobPhase(JobPhase::SAMPLE_RESTORE_ADC);
      } else if ((raw16 & cmd::DIAG_MATHOF) != 0) {
        _jobDeferredStatus = Status::Error(
            Err::MATH_OVERFLOW, "INA228 MATHOF invalidates current and power",
            raw16);
        _setJobPhase(JobPhase::SAMPLE_RESTORE_ADC);
      } else {
        _setJobPhase(JobPhase::SAMPLE_VSHUNT);
      }
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::SAMPLE_VSHUNT:
      st = readReg24(cmd::REG_VSHUNT, raw24);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _sampleScratch.raw.vshunt = _signExtend20(raw24);
      _setJobPhase(JobPhase::SAMPLE_VBUS);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::SAMPLE_VBUS:
      st = readReg24(cmd::REG_VBUS, raw24);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _sampleScratch.raw.vbus = raw24 >> 4;
      _setJobPhase(JobPhase::SAMPLE_DIETEMP);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::SAMPLE_DIETEMP:
      st = readReg16(cmd::REG_DIETEMP, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _sampleScratch.raw.dietemp = static_cast<int16_t>(raw16);
      _setJobPhase(JobPhase::SAMPLE_CURRENT);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::SAMPLE_CURRENT:
      st = readReg24(cmd::REG_CURRENT, raw24);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _sampleScratch.raw.current = _signExtend20(raw24);
      _setJobPhase(JobPhase::SAMPLE_POWER);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::SAMPLE_POWER:
      st = readReg24(cmd::REG_POWER, raw24);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _sampleScratch.raw.power = raw24;
      _setJobPhase(JobPhase::SAMPLE_RESTORE_ADC);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::SAMPLE_RESTORE_ADC:
      _jobTouchedRegisterMask |= (1ULL << cmd::REG_ADC_CONFIG);
      st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      _setJobPhase(JobPhase::SAMPLE_VERIFY_ADC);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::SAMPLE_VERIFY_ADC:
      st = readReg16(cmd::REG_ADC_CONFIG, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      st = _verifyRegister(cmd::REG_ADC_CONFIG, raw16,
                           _buildAdcConfig(), 0xFFFFU);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _hardwareState = HardwareState::SYNCHRONIZED;
      _initialized = true;
      if (!_jobDeferredStatus.ok()) {
        if (_jobDeferredStatus.code == Err::MEMORY_ERROR) {
          // A bad NV trim checksum invalidates the whole cached contract, not
          // just the three lifecycle fields this branch used to touch.
          _invalidateJobHardwareState(_jobDeferredStatus);
        }
        return _finishJob(_jobDeferredStatus, JobState::FAILED,
                          JobEffect::CONFIRMED, nowMs);
      }
      st = convertRawSample(_sampleScratch.raw, _sampleScratch.values);
      if (!st.ok()) {
        return _finishJob(st, JobState::FAILED, JobEffect::CONFIRMED, nowMs);
      }
      _sampleScratch.operationId = _jobSnapshot.operationId;
      _sampleScratch.requestToken = _jobSnapshot.requestToken;
      _sampleScratch.configurationGeneration = _configurationGeneration;
      _sampleScratch.validChannels =
          CHANNEL_SHUNT_VOLTAGE_VALID | CHANNEL_BUS_VOLTAGE_VALID |
          CHANNEL_TEMPERATURE_VALID | CHANNEL_CURRENT_VALID |
          CHANNEL_POWER_VALID;
      return _finishJob(Status::Ok(), JobState::SUCCEEDED,
                        JobEffect::CONFIRMED, nowMs);

    case JobPhase::RESET_WRITE:
      // CONFIG.RST restores every register, not just CONFIG, so the whole
      // writable set is potentially dirty from this point on.
      _jobTouchedRegisterMask |= RESET_TOUCHED_REGISTER_MASK;
      st = writeReg16(cmd::REG_CONFIG, cmd::CONFIG_RST);
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      _hardwareState = HardwareState::RESYNC_REQUIRED;
      // Alert thresholds are not part of the replayed configuration, so a
      // successful reset silently reverts them to datasheet defaults. Raise the
      // sticky advisory so the owner reapplies engineering-unit limits.
      _markThresholdsDirty();
      // Reset invalidates any earlier conversion. Discard its timing before
      // arming the reset wait so the single deferred origin stays unambiguous.
      _trigPending = false;
      _trigStartMs = 0;
      if (_deferredTimeOrigin == DeferredTimeOrigin::TRIGGERED_CONVERSION) {
        _deferredTimeOrigin = DeferredTimeOrigin::NONE;
      }
      _armPostWriteTimeOrigin(DeferredTimeOrigin::JOB_WAIT);
      _setJobPhase(JobPhase::RESET_WAIT);
      return Status{Err::IN_PROGRESS, 0, "Reset startup wait in progress"};

    case JobPhase::RESET_VERIFY_CLEAR:
      st = readReg16(cmd::REG_CONFIG, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      if ((raw16 & (cmd::CONFIG_RST | cmd::CONFIG_RSTACC)) != 0) {
        return _failJob(Status::Error(Err::CONFIG_MISMATCH,
                                      "Reset bits did not self-clear", raw16),
                        false, nowMs);
      }
      _setJobPhase(JobPhase::READ_MANUFACTURER);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::ACCUMULATOR_WRITE:
      _jobTouchedRegisterMask |= (1ULL << cmd::REG_CONFIG);
      st = writeReg16(cmd::REG_CONFIG,
                      static_cast<uint16_t>(_buildConfig() | cmd::CONFIG_RSTACC));
      if (!st.ok()) return _failJob(st, true, nowMs);
      _jobHadSuccessfulWrite = true;
      _accumulationReady = false;
      _setJobPhase(JobPhase::ACCUMULATOR_VERIFY);
      return Status{Err::IN_PROGRESS, 0, "Job in progress"};

    case JobPhase::ACCUMULATOR_VERIFY:
      st = readReg16(cmd::REG_CONFIG, raw16);
      if (!st.ok()) return _failJob(st, false, nowMs);
      st = _verifyRegister(cmd::REG_CONFIG, raw16, _buildConfig(),
          cmd::MASK_CONFIG_CONVDLY | cmd::CONFIG_TEMPCOMP |
          cmd::CONFIG_ADCRANGE | cmd::CONFIG_RST | cmd::CONFIG_RSTACC);
      if (!st.ok()) return _failJob(st, false, nowMs);
      _accumulatorGeneration = _configurationGeneration;
      _accumulationReady = true;
      _clearCapturedAccumulatorEvidence();
      return _finishJob(Status::Ok(), JobState::SUCCEEDED,
                        JobEffect::CONFIRMED, nowMs);

    case JobPhase::SAMPLE_WAIT:
    case JobPhase::RESET_WAIT:
    case JobPhase::IDLE:
      break;
  }
  return _failJob(Status::Error(Err::INVALID_CONFIG, "Invalid job phase"),
                  false, nowMs);
}

Status INA228::pollJob(uint32_t nowMs, uint8_t maxTransfers) {
  return _pollJobImpl(nowMs, maxTransfers, true);
}

Status INA228::_pollJobImpl(uint32_t nowMs, uint8_t maxTransfers,
                            bool explicitTimestamp) {
  if (!_cooperativeJobActive()) {
    return Status::Error(Err::INVALID_PARAM, "No active job to poll");
  }
  if (_jobPollActive) {
    return Status::Error(Err::BUSY, "Job poll re-entry is not allowed");
  }

  if (_deferredTimeOrigin == DeferredTimeOrigin::JOB_WAIT) {
    _jobWaitStartMs = nowMs;
    _deferredTimeOrigin = DeferredTimeOrigin::NONE;
    return Status{Err::IN_PROGRESS, 0, "Post-write wait origin established"};
  }
  if (explicitTimestamp &&
      _deferredTimeOrigin == DeferredTimeOrigin::TRIGGERED_CONVERSION) {
    _trigStartMs = nowMs;
    _deferredTimeOrigin = DeferredTimeOrigin::NONE;
    return Status{Err::IN_PROGRESS, 0,
                  "Triggered conversion time origin established"};
  }
  _jobPollActive = true;

  if (_jobPhase == JobPhase::SAMPLE_WAIT) {
    const uint32_t waitMs = _instantaneousSampleWaitMs();
    if (waitElapsed(nowMs, _jobWaitStartMs, waitMs)) {
      _setJobPhase(JobPhase::SAMPLE_DIAG);
    }
  } else if (_jobPhase == JobPhase::RESET_WAIT &&
             waitElapsed(nowMs, _jobWaitStartMs, RESET_STARTUP_MS)) {
    _setJobPhase(JobPhase::RESET_VERIFY_CLEAR);
  }

  if (maxTransfers == 0) {
    _jobPollActive = false;
    return Status{Err::IN_PROGRESS, 0, "Job in progress; no transfer budget"};
  }

  uint8_t used = 0;
  while (used < maxTransfers && _cooperativeJobActive()) {
    if (_jobPhase == JobPhase::SAMPLE_WAIT) {
      const uint32_t waitMs = _instantaneousSampleWaitMs();
      if (!waitElapsed(nowMs, _jobWaitStartMs, waitMs)) {
        _jobPollActive = false;
        return Status{Err::IN_PROGRESS, 0, "Conversion wait in progress"};
      }
      _setJobPhase(JobPhase::SAMPLE_DIAG);
      continue;
    }
    if (_jobPhase == JobPhase::RESET_WAIT) {
      if (!waitElapsed(nowMs, _jobWaitStartMs, RESET_STARTUP_MS)) {
        _jobPollActive = false;
        return Status{Err::IN_PROGRESS, 0, "Reset startup wait in progress"};
      }
      _setJobPhase(JobPhase::RESET_VERIFY_CLEAR);
      continue;
    }

    Status st = _pollJobTransfer(nowMs);
    ++used;
    if (!_cooperativeJobActive()) {
      _jobPollActive = false;
      return st;
    }
    if (_deferredTimeOrigin == DeferredTimeOrigin::JOB_WAIT) {
      _jobPollActive = false;
      return Status{Err::IN_PROGRESS, 0,
                    "Post-write wait origin pending caller timestamp"};
    }
  }
  _jobPollActive = false;
  return Status{Err::IN_PROGRESS, 0, "Job in progress"};
}

void INA228::_invalidateAccumulatorEpoch() {
  _accumulationReady = false;
  _accumulatorGeneration = 0;
}

Status INA228::begin(const Config& config) {
  // Legacy synchronous convenience: drive the same verified initialization
  // engine with its finite 14-transfer maximum. External bus owners should use
  // bind()/startInitialize()/pollJob() to retain scheduling authority.
  Status staged = bind(config);
  if (!staged.ok()) return staged;
  uint32_t operationId = 0;
  staged = startInitialize(0, operationId);
  if (!staged.ok()) return staged;
  staged = _pollJobImpl(_nowMs(), 14, _config.nowMs != nullptr);
  if (staged.inProgress()) {
    (void)cancelJob();
    JobResult cancelled{};
    (void)takeJobResult(operationId, cancelled);
    return Status::Error(Err::INVALID_CONFIG,
                         "Initialization exceeded declared transfer bound");
  }
  JobResult result{};
  Status taken = takeJobResult(operationId, result);
  return taken.ok() ? staged : taken;
}

void INA228::tick(uint32_t nowMs) {
  if (!_initialized || !_trigPending) {
    return;
  }
  bool ready = false;
  (void)pollConversionReady(nowMs, ready);
}

void INA228::end() {
  _bound = false;
  _initialized = false;
  _driverState = DriverState::UNINIT;
  _hardwareState = HardwareState::UNBOUND;
  _deviceIdentity = DeviceIdentity{};
  _deviceIdentityValid = false;
  _trigPending = false;
  _trigStartMs = 0;
  _accumulationReady = false;
  _currentLsb = 0.0f;
  _shuntCal = 0;
  _calibrationClamped = false;
  _maxCurrentExceedsShuntRange = false;
  _calibrationPlan = CalibrationPlan{};
  _usesFixedCalibration = false;
  _hardwareDirty = false;
  _dirtyRegisterMask = 0;
  _hardwareDirtyCause = Status::Ok();
  _thresholdsDirty = false;
  _diagAlertConfigBits = 0;
  _diagAlertSnapshot = DiagAlertSnapshot{};
  _diagnosticEvents = DiagnosticEvents{};
  _configurationGeneration = 0;
  _accumulatorGeneration = 0;
  _clearCooperativeState();
  _config = Config{};
}

// ===========================================================================
// Diagnostics
// ===========================================================================

Status INA228::probe() {
  if (!_bound) {
    return Status::Error(Err::NOT_BOUND, "bind() has not completed");
  }
  if (!_hardwareAccessAllowed()) {
    return Status::Error(Err::BUSY, "Cooperative job owns hardware access");
  }

  uint16_t mfgId = 0;
  Status st = _readReg16Raw(cmd::REG_MANUFACTURER_ID, mfgId);
  if (!st.ok()) {
    return mapPresenceReadFailure(st, "Device not responding");
  }
  uint16_t devId = 0;
  st = _readReg16Raw(cmd::REG_DEVICE_ID, devId);
  if (!st.ok()) {
    return mapPresenceReadFailure(st, "Device ID read failed");
  }
  DeviceIdentity identity{};
  st = parseDeviceIdentity(mfgId, devId, identity);
  if (!st.ok()) return st;
  if ((_config.supportedRevisionMask &
       (static_cast<uint16_t>(1U) << identity.revision)) == 0) {
    return Status::Error(Err::UNSUPPORTED_REVISION,
                         "Unsupported INA228 revision", identity.revision);
  }
  return Status::Ok();
}

Status INA228::recover() {
  // Legacy bounded convenience over the verified reinitialization job.
  if (!_bound) {
    return Status::Error(Err::NOT_BOUND, "bind() has not completed");
  }
  uint32_t operationId = 0;
  Status staged = startReinitialize(0, operationId);
  if (!staged.ok()) return staged;
  staged = _pollJobImpl(_nowMs(), 14, _config.nowMs != nullptr);
  if (staged.inProgress()) {
    (void)cancelJob();
    JobResult cancelled{};
    (void)takeJobResult(operationId, cancelled);
    return Status::Error(Err::INVALID_CONFIG,
                         "Reinitialization exceeded declared transfer bound");
  }
  JobResult result{};
  Status taken = takeJobResult(operationId, result);
  return taken.ok() ? staged : taken;
}

Status INA228::getSettings(SettingsSnapshot& out) const {
  out.initialized = _initialized;
  out.state = _driverState;
  out.i2cAddress = _config.i2cAddress;
  out.i2cTimeoutMs = _config.i2cTimeoutMs;
  out.offlineThreshold = _config.offlineThreshold;
  out.hasNowMsHook = _config.nowMs != nullptr;
  out.mode = _config.mode;
  out.vbusConvTime = _config.vbusConvTime;
  out.vshuntConvTime = _config.vshuntConvTime;
  out.vtempConvTime = _config.vtempConvTime;
  out.averaging = _config.averaging;
  out.adcRange = _config.adcRange;
  out.shuntResistanceOhm = _config.shuntResistanceOhm;
  out.maxExpectedCurrentA = _config.maxExpectedCurrentA;
  out.tempCompEnabled = _config.tempCompEnabled;
  out.shuntTempCoeffPpmC = _config.shuntTempCoeffPpmC;
  out.convDelayMs2 = _config.convDelayMs2;
  out.currentLsb = _currentLsb;
  out.shuntCal = _shuntCal;
  out.calibrated = _currentLsb > 0.0f && _shuntCal > 0 && !_hardwareDirty &&
                   _hardwareState == HardwareState::SYNCHRONIZED;
  out.calibrationClamped = _calibrationClamped;
  out.maxCurrentExceedsShuntRange = _maxCurrentExceedsShuntRange;
  out.hardwareDirty = _hardwareDirty;
  out.dirtyRegisterMask = _dirtyRegisterMask;
  out.hardwareDirtyCause = _hardwareDirtyCause;
  out.thresholdsDirty = _thresholdsDirty;
  out.triggeredConversionPending = _trigPending;
  out.triggeredConversionStartMs = _trigStartMs;
  return Status::Ok();
}

Status INA228::getDiagAlertSnapshot(DiagAlertSnapshot& out) const {
  out = _diagAlertSnapshot;
  return Status::Ok();
}

// ===========================================================================
// Measurement API
// ===========================================================================

Status INA228::readMeasurement(Measurement& out) {
  uint16_t diag = 0;
  Status st = _prepareCalibratedMeasurementRead(diag);
  if (!st.ok()) return st;

  Measurement result{};

  // Read shunt voltage (24-bit)
  uint32_t raw24 = 0;
  st = readReg24(cmd::REG_VSHUNT, raw24);
  if (!st.ok()) return st;
  int32_t vshRaw = _signExtend20(raw24);
  const double vshLsb = (_config.adcRange == AdcRange::MV_40_96)
                          ? cmd::VSHUNT_LSB_RANGE1
                          : cmd::VSHUNT_LSB_RANGE0;
  result.shuntVoltageV = static_cast<float>(vshRaw * vshLsb);

  // Read bus voltage (24-bit)
  st = readReg24(cmd::REG_VBUS, raw24);
  if (!st.ok()) return st;
  uint32_t vbusRaw = raw24 >> 4;
  result.busVoltageV = static_cast<float>(vbusRaw * cmd::VBUS_LSB);

  // Read temperature (16-bit)
  uint16_t raw16 = 0;
  st = readReg16(cmd::REG_DIETEMP, raw16);
  if (!st.ok()) return st;
  result.temperatureC = static_cast<float>(static_cast<int16_t>(raw16) * cmd::TEMP_LSB);

  // Read current (24-bit, requires calibration)
  st = readReg24(cmd::REG_CURRENT, raw24);
  if (!st.ok()) return st;
  int32_t curRaw = _signExtend20(raw24);
  st = assignFiniteFloat(static_cast<double>(_currentLsb) *
                         static_cast<double>(curRaw),
                         result.currentA,
                         "Current conversion overflow");
  if (!st.ok()) return st;

  // Read power (24-bit, unsigned)
  st = readReg24(cmd::REG_POWER, raw24);
  if (!st.ok()) return st;
  st = assignFiniteFloat(cmd::POWER_COEFF * static_cast<double>(_currentLsb) *
                         static_cast<double>(raw24),
                         result.powerW,
                         "Power conversion overflow");
  if (!st.ok()) return st;

  const bool energyCandidate = _ensureEnergyAccumulatorReadable().ok();
  const bool chargeCandidate = _ensureChargeAccumulatorReadable().ok();
  if (energyCandidate || chargeCandidate) {
    result.diagAlertValid = true;
    result.diagAlertRaw = diag;
    result.energyOverflow = (diag & cmd::DIAG_ENERGYOF) != 0;
    result.chargeOverflow = (diag & cmd::DIAG_CHARGEOF) != 0;
    result.mathOverflow = (diag & cmd::DIAG_MATHOF) != 0;

    const bool mathOk = !result.mathOverflow;

    if (energyCandidate && mathOk && !result.energyOverflow) {
      // Read energy (40-bit, unsigned)
      uint64_t raw40 = 0;
      st = readReg40(cmd::REG_ENERGY, raw40);
      if (!st.ok()) return st;
      result.energyJ = cmd::ENERGY_COEFF * cmd::POWER_COEFF *
                       static_cast<double>(_currentLsb) * static_cast<double>(raw40);
      result.energyValid = true;
    }

    if (chargeCandidate && mathOk && !result.chargeOverflow) {
      // Read charge (40-bit, signed)
      uint64_t raw40 = 0;
      st = readReg40(cmd::REG_CHARGE, raw40);
      if (!st.ok()) return st;
      int64_t chargeSigned = _signExtend40(raw40);
      result.chargeC = static_cast<double>(_currentLsb) *
                       static_cast<double>(chargeSigned);
      result.chargeValid = true;
    }
  }

  out = result;
  return Status::Ok();
}

Status INA228::readRawSample(RawSample& out) {
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }

  RawSample result{};

  uint32_t raw24 = 0;
  Status st = readReg24(cmd::REG_VSHUNT, raw24);
  if (!st.ok()) return st;
  result.vshunt = _signExtend20(raw24);

  st = readReg24(cmd::REG_VBUS, raw24);
  if (!st.ok()) return st;
  result.vbus = raw24 >> 4;

  uint16_t raw16 = 0;
  st = readReg16(cmd::REG_DIETEMP, raw16);
  if (!st.ok()) return st;
  result.dietemp = static_cast<int16_t>(raw16);

  st = readReg24(cmd::REG_CURRENT, raw24);
  if (!st.ok()) return st;
  result.current = _signExtend20(raw24);

  st = readReg24(cmd::REG_POWER, raw24);
  if (!st.ok()) return st;
  result.power = raw24;

  uint16_t diag = 0;
  st = _readDiagAlertTracked(diag);
  if (!st.ok()) return st;
  result.diagAlertValid = true;
  result.diagAlertRaw = diag;
  result.energyOverflow = (diag & cmd::DIAG_ENERGYOF) != 0;
  result.chargeOverflow = (diag & cmd::DIAG_CHARGEOF) != 0;
  result.mathOverflow = (diag & cmd::DIAG_MATHOF) != 0;

  uint64_t raw40 = 0;
  st = readReg40(cmd::REG_ENERGY, raw40);
  if (!st.ok()) return st;
  result.energy = raw40;
  result.energyValid = _ensureEnergyAccumulatorReadable().ok() &&
                       !result.mathOverflow && !result.energyOverflow;

  st = readReg40(cmd::REG_CHARGE, raw40);
  if (!st.ok()) return st;
  result.charge = _signExtend40(raw40);
  result.chargeValid = _ensureChargeAccumulatorReadable().ok() &&
                       !result.mathOverflow && !result.chargeOverflow;

  out = result;
  return Status::Ok();
}

Status INA228::readPowerSampleRawStep(RawSample& rawOut, IntegerSample& integerOut,
                                      uint8_t maxInstructions) {
  // Deprecated compatibility surface over the unified triggered sample job.
  // It needs a time hook because this signature cannot accept caller time.
  if (maxInstructions == 0) {
    return Status::Error(Err::INVALID_PARAM, "Instruction budget must be > 0");
  }
  if (_config.nowMs == nullptr) {
    return Status::Error(Err::INVALID_CONFIG,
                         "Use startInstantaneousSample/pollJob without a time hook");
  }
  uint32_t operationId = 0;
  if (!_cooperativeJobActive()) {
    Status started = startInstantaneousSample(0, operationId);
    if (!started.ok()) return started;
  } else {
    if (_jobSnapshot.kind != JobKind::INSTANTANEOUS_SAMPLE) {
      return Status::Error(Err::BUSY, "Another cooperative job is active");
    }
    operationId = _jobSnapshot.operationId;
  }
  Status polled = _pollJobImpl(_nowMs(), maxInstructions, true);
  if (polled.inProgress()) return polled;
  JobResult result{};
  Status taken = takeJobResult(operationId, result);
  if (!taken.ok()) return taken;
  if (!polled.ok()) return polled;
  if (!result.hasInstantaneousSample) {
    return Status::Error(Err::RESULT_NOT_AVAILABLE,
                         "Sample job completed without a sample");
  }
  rawOut = result.instantaneousSample.raw;
  integerOut = result.instantaneousSample.values;
  return Status::Ok();
}

Status INA228::readIntegerSample(IntegerSample& out) {
  uint16_t diag = 0;
  Status st = _prepareCalibratedMeasurementRead(diag);
  if (!st.ok()) return st;

  RawSample raw{};
  raw.diagAlertValid = true;
  raw.diagAlertRaw = diag;

  uint32_t raw24 = 0;
  st = readReg24(cmd::REG_VSHUNT, raw24);
  if (!st.ok()) return st;
  raw.vshunt = _signExtend20(raw24);

  st = readReg24(cmd::REG_VBUS, raw24);
  if (!st.ok()) return st;
  raw.vbus = raw24 >> 4;

  uint16_t raw16 = 0;
  st = readReg16(cmd::REG_DIETEMP, raw16);
  if (!st.ok()) return st;
  raw.dietemp = static_cast<int16_t>(raw16);

  st = readReg24(cmd::REG_CURRENT, raw24);
  if (!st.ok()) return st;
  raw.current = _signExtend20(raw24);

  st = readReg24(cmd::REG_POWER, raw24);
  if (!st.ok()) return st;
  raw.power = raw24;

  return convertRawSample(raw, out);
}

Status INA228::convertRawSample(const RawSample& raw, IntegerSample& out) const {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status calStatus = _ensureCalibrated();
  if (!calStatus.ok()) return calStatus;
  if (raw.diagAlertValid &&
      (raw.mathOverflow || ((raw.diagAlertRaw & cmd::DIAG_MATHOF) != 0))) {
    return Status::Error(Err::MATH_OVERFLOW, "INA228 math overflow",
                         static_cast<int32_t>(raw.diagAlertRaw));
  }

  IntegerSample result{};
  result.diagAlertValid = raw.diagAlertValid;
  result.diagAlertRaw = raw.diagAlertRaw;
  result.shuntMicrovolts = shuntRawToMicrovolts(raw.vshunt, _config.adcRange);
  result.busMillivolts = busRawToMillivolts(raw.vbus);
  result.dieTemperatureMilliC = tempRawToMilliC(raw.dietemp);

  Status st = assignRoundedInt32(static_cast<double>(_currentLsb) *
                                 static_cast<double>(raw.current) * 1000.0,
                                 result.currentMilliamps,
                                 "Current integer conversion overflow");
  if (!st.ok()) return st;

  st = assignRoundedUint32(cmd::POWER_COEFF * static_cast<double>(_currentLsb) *
                           static_cast<double>(raw.power) * 1000.0,
                           result.powerMilliwatts,
                           "Power integer conversion overflow");
  if (!st.ok()) return st;

  out = result;
  return Status::Ok();
}

Status INA228::readShuntVoltage(float& out) {
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }

  uint32_t raw24 = 0;
  Status st = readReg24(cmd::REG_VSHUNT, raw24);
  if (!st.ok()) return st;

  int32_t raw = _signExtend20(raw24);
  const double lsb = (_config.adcRange == AdcRange::MV_40_96)
                       ? cmd::VSHUNT_LSB_RANGE1
                       : cmd::VSHUNT_LSB_RANGE0;
  out = static_cast<float>(raw * lsb);
  return Status::Ok();
}

Status INA228::readBusVoltage(float& out) {
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }

  uint32_t raw24 = 0;
  Status st = readReg24(cmd::REG_VBUS, raw24);
  if (!st.ok()) return st;

  uint32_t raw = raw24 >> 4;
  out = static_cast<float>(raw * cmd::VBUS_LSB);
  return Status::Ok();
}

Status INA228::readTemperature(float& out) {
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }

  uint16_t raw16 = 0;
  Status st = readReg16(cmd::REG_DIETEMP, raw16);
  if (!st.ok()) return st;

  out = static_cast<float>(static_cast<int16_t>(raw16) * cmd::TEMP_LSB);
  return Status::Ok();
}

Status INA228::readCurrent(float& out) {
  uint16_t diag = 0;
  Status st = _prepareCalibratedMeasurementRead(diag);
  if (!st.ok()) return st;

  uint32_t raw24 = 0;
  st = readReg24(cmd::REG_CURRENT, raw24);
  if (!st.ok()) return st;

  int32_t raw = _signExtend20(raw24);
  return assignFiniteFloat(static_cast<double>(_currentLsb) *
                           static_cast<double>(raw),
                           out,
                           "Current conversion overflow");
}

Status INA228::readPower(float& out) {
  uint16_t diag = 0;
  Status st = _prepareCalibratedMeasurementRead(diag);
  if (!st.ok()) return st;

  uint32_t raw24 = 0;
  st = readReg24(cmd::REG_POWER, raw24);
  if (!st.ok()) return st;

  return assignFiniteFloat(cmd::POWER_COEFF * static_cast<double>(_currentLsb) *
                           static_cast<double>(raw24),
                           out,
                           "Power conversion overflow");
}

Status INA228::readEnergy(double& out) {
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }
  Status calStatus = _ensureCalibrated();
  if (!calStatus.ok()) return calStatus;
  Status accStatus = _ensureEnergyAccumulatorReadable();
  if (!accStatus.ok()) {
    return accStatus;
  }

  uint16_t diag = 0;
  Status st = _readDiagAlertTracked(diag);
  if (!st.ok()) return st;

  st = _validateAccumulatorDiag(diag, cmd::DIAG_ENERGYOF, "Energy accumulator overflow");
  if (!st.ok()) {
    return st;
  }

  uint64_t raw40 = 0;
  st = readReg40(cmd::REG_ENERGY, raw40);
  if (!st.ok()) return st;

  out = cmd::ENERGY_COEFF * cmd::POWER_COEFF *
        static_cast<double>(_currentLsb) * static_cast<double>(raw40);
  return Status::Ok();
}

Status INA228::readCharge(double& out) {
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }
  Status calStatus = _ensureCalibrated();
  if (!calStatus.ok()) return calStatus;
  Status accStatus = _ensureChargeAccumulatorReadable();
  if (!accStatus.ok()) {
    return accStatus;
  }

  uint16_t diag = 0;
  Status st = _readDiagAlertTracked(diag);
  if (!st.ok()) return st;

  st = _validateAccumulatorDiag(diag, cmd::DIAG_CHARGEOF, "Charge accumulator overflow");
  if (!st.ok()) {
    return st;
  }

  uint64_t raw40 = 0;
  st = readReg40(cmd::REG_CHARGE, raw40);
  if (!st.ok()) return st;

  int64_t signed40 = _signExtend40(raw40);
  out = static_cast<double>(_currentLsb) * static_cast<double>(signed40);
  return Status::Ok();
}

Status INA228::isConversionReady(bool& ready) {
  if (_config.nowMs == nullptr && _trigPending) {
    ready = false;
    return Status::Ok();
  }
  return pollConversionReady(_nowMs(), ready);
}

Status INA228::pollConversionReady(uint32_t nowMs, bool& ready) {
  ready = false;
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (_deferredTimeOrigin == DeferredTimeOrigin::TRIGGERED_CONVERSION) {
    _trigStartMs = nowMs;
    _deferredTimeOrigin = DeferredTimeOrigin::NONE;
    return Status::Ok();
  }
  if (_trigPending && !_triggerDeadlineElapsed(nowMs)) {
    return Status::Ok();
  }

  uint16_t diag = 0;
  Status st = _readDiagAlertTracked(diag);
  if (!st.ok()) return st;

  ready = (diag & cmd::DIAG_CNVRF) != 0;
  return Status::Ok();
}

Status INA228::pollMeasurementReady(uint32_t nowMs, uint8_t maxInstructions,
                                    bool& ready) {
  ready = false;
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (maxInstructions == 0) {
    return Status::Error(Err::INVALID_PARAM, "maxInstructions must be > 0");
  }
  if (_cooperativeJobActive()) {
    return Status::Error(Err::BUSY, "Another cooperative job is active");
  }
  Status allowed = _ensureNormalI2cAllowed();
  if (!allowed.ok()) return allowed;
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (_deferredTimeOrigin == DeferredTimeOrigin::TRIGGERED_CONVERSION) {
    _trigStartMs = nowMs;
    _deferredTimeOrigin = DeferredTimeOrigin::NONE;
    return Status::Ok();
  }
  if (!_trigPending) {
    ready = true;
    return Status::Ok();
  }
  if (!_triggerDeadlineElapsed(nowMs)) {
    return Status::Ok();
  }

  uint16_t diag = 0;
  Status st = _readDiagAlertTracked(diag);
  if (!st.ok()) return st;

  ready = (diag & cmd::DIAG_CNVRF) != 0;
  return Status::Ok();
}

// ===========================================================================
// Configuration
// ===========================================================================

Status INA228::setMode(Mode mode) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!isValidMode(mode)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid mode");
  }

  Status st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig(mode));
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
    return st;
  }

  _config.mode = mode;
  if (isTriggeredMode(mode)) {
    _markTriggeredConversionStarted();
    return Status{Err::IN_PROGRESS, 0, "Conversion started"};
  }
  _completeTriggeredConversion();
  return Status::Ok();
}

Status INA228::getMode(Mode& out) const {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  out = _config.mode;
  return Status::Ok();
}

Status INA228::triggerConversion(Mode mode) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!isTriggeredMode(mode)) {
    return Status::Error(Err::INVALID_PARAM, "Not a triggered mode (0x1-0x7)");
  }

  Status st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig(mode));
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
    return st;
  }

  _config.mode = mode;
  _markTriggeredConversionStarted();
  return Status{Err::IN_PROGRESS, 0, "Conversion started"};
}

Status INA228::startTriggeredMeasurement(Mode mode) {
  return triggerConversion(mode);
}

Status INA228::setVbusConvTime(ConvTime ct) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!isValidConvTime(ct)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid conversion time");
  }

  const ConvTime old = _config.vbusConvTime;
  _config.vbusConvTime = ct;
  Status st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
  if (!st.ok()) {
    _config.vbusConvTime = old;
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
  } else if (isTriggeredMode(_config.mode)) {
    _markTriggeredConversionStarted();
  } else {
    _invalidateAccumulatorEpoch();
  }
  return st;
}

Status INA228::setVshuntConvTime(ConvTime ct) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!isValidConvTime(ct)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid conversion time");
  }

  const ConvTime old = _config.vshuntConvTime;
  _config.vshuntConvTime = ct;
  Status st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
  if (!st.ok()) {
    _config.vshuntConvTime = old;
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
  } else if (isTriggeredMode(_config.mode)) {
    _markTriggeredConversionStarted();
  } else {
    _invalidateAccumulatorEpoch();
  }
  return st;
}

Status INA228::setTempConvTime(ConvTime ct) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!isValidConvTime(ct)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid conversion time");
  }

  const ConvTime old = _config.vtempConvTime;
  _config.vtempConvTime = ct;
  Status st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
  if (!st.ok()) {
    _config.vtempConvTime = old;
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
  } else if (isTriggeredMode(_config.mode)) {
    _markTriggeredConversionStarted();
  } else {
    _invalidateAccumulatorEpoch();
  }
  return st;
}

Status INA228::setAveraging(Averaging avg) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!isValidAveraging(avg)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid averaging count");
  }

  const Averaging old = _config.averaging;
  _config.averaging = avg;
  Status st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
  if (!st.ok()) {
    _config.averaging = old;
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
  } else if (isTriggeredMode(_config.mode)) {
    _markTriggeredConversionStarted();
  } else {
    _invalidateAccumulatorEpoch();
  }
  return st;
}

Status INA228::setAdcRange(AdcRange range) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!isValidAdcRange(range)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid ADC range");
  }

  const AdcRange oldRange = _config.adcRange;
  const float oldCurrentLsb = _currentLsb;
  const uint16_t oldShuntCal = _shuntCal;
  const bool oldClamped = _calibrationClamped;
  const bool oldMaxCurrentExceedsRange = _maxCurrentExceedsShuntRange;
  const CalibrationPlan oldPlan = _calibrationPlan;
  const uint16_t oldConfigReg = _buildConfig();

  uint16_t newShuntCal = 0;
  float newCurrentLsb = 0.0f;
  bool newClamped = false;
  bool newMaxCurrentExceedsRange = false;
  CalibrationPlan newPlan = oldPlan;
  newPlan.shuntFullScaleMicrovolts =
      range == AdcRange::MV_40_96 ? 40960U : 163840U;
  if (_usesFixedCalibration) {
    Status planStatus = calculateCalibration(_config.calibration, range, newPlan);
    if (!planStatus.ok()) return planStatus;
    newShuntCal = newPlan.shuntCal;
    const double fixedShuntOhms =
        static_cast<double>(_config.calibration.shuntMicroOhms) * 1.0e-6;
    const double rangeMultiplier =
        range == AdcRange::MV_40_96 ? 4.0 : 1.0;
    newCurrentLsb = static_cast<float>(static_cast<double>(newPlan.shuntCal) /
        (cmd::SHUNT_CAL_FACTOR * fixedShuntOhms * rangeMultiplier));
    newClamped = newPlan.clamped;
    newMaxCurrentExceedsRange = newPlan.maxCurrentExceedsShuntRange;
  } else if (_config.shuntResistanceOhm > 0.0f &&
             _config.maxExpectedCurrentA > 0.0f) {
    Status st = buildLegacyCalibrationPlan(_config.shuntResistanceOhm,
                                           _config.maxExpectedCurrentA,
                                           range, newPlan, newCurrentLsb);
    if (!st.ok()) {
      return st;
    }
    newShuntCal = newPlan.shuntCal;
    newClamped = newPlan.clamped;
    newMaxCurrentExceedsRange = newPlan.maxCurrentExceedsShuntRange;
    if (newClamped || newMaxCurrentExceedsRange) {
      return Status::Error(Err::INVALID_CONFIG,
                           newClamped ? "SHUNT_CAL would clamp" :
                                        "Maximum current exceeds shunt range");
    }
  }

  _config.adcRange = range;
  const uint16_t newConfigReg = _buildConfig();
  _config.adcRange = oldRange;

  Status st = writeReg16(cmd::REG_CONFIG, newConfigReg);
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_CONFIG, st);
    return st;
  }

  st = writeReg16(cmd::REG_SHUNT_CAL, newShuntCal);
  if (!st.ok()) {
    Status rollback = writeReg16(cmd::REG_CONFIG, oldConfigReg);
    _config.adcRange = oldRange;
    _currentLsb = oldCurrentLsb;
    _shuntCal = oldShuntCal;
    _calibrationClamped = oldClamped;
    _maxCurrentExceedsShuntRange = oldMaxCurrentExceedsRange;
    _calibrationPlan = oldPlan;
    _markHardwareDirty(cmd::REG_SHUNT_CAL, st);
    if (!rollback.ok()) {
      _markHardwareDirty(cmd::REG_CONFIG, rollback);
    }
    return st;
  } else {
    _config.adcRange = range;
    _currentLsb = newCurrentLsb;
    _shuntCal = newShuntCal;
    _calibrationClamped = newClamped;
    _maxCurrentExceedsShuntRange = newMaxCurrentExceedsRange;
    _calibrationPlan = newPlan;
    _clearHardwareDirty();
    if (range != oldRange) {
      _markThresholdsDirty();
      ++_configurationGeneration;
      if (_configurationGeneration == 0) ++_configurationGeneration;
      _invalidateAccumulatorEpoch();
    }
  }
  return st;
}

Status INA228::setCalibration(float shuntOhm, float maxCurrentA) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (_usesFixedCalibration) {
    return Status::Error(Err::INVALID_CONFIG,
                         "Rebind to change a fixed-unit calibration contract");
  }
  CalibrationPlan newPlan{};
  float newCurrentLsb = 0.0f;
  Status st = buildLegacyCalibrationPlan(shuntOhm, maxCurrentA,
                                         _config.adcRange, newPlan,
                                         newCurrentLsb);
  if (!st.ok()) {
    return st;
  }
  if (newPlan.clamped || newPlan.maxCurrentExceedsShuntRange) {
    return Status::Error(
        Err::INVALID_CONFIG,
        newPlan.clamped ? "SHUNT_CAL would clamp" :
                          "Maximum current exceeds shunt range");
  }

  st = writeReg16(cmd::REG_SHUNT_CAL, newPlan.shuntCal);
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_SHUNT_CAL, st);
    return st;
  }

  _config.shuntResistanceOhm = shuntOhm;
  _config.maxExpectedCurrentA = maxCurrentA;
  _currentLsb = newCurrentLsb;
  _shuntCal = newPlan.shuntCal;
  _calibrationClamped = newPlan.clamped;
  _maxCurrentExceedsShuntRange = newPlan.maxCurrentExceedsShuntRange;
  _calibrationPlan = newPlan;
  _clearHardwareDirty();
  _markThresholdsDirty();
  ++_configurationGeneration;
  if (_configurationGeneration == 0) ++_configurationGeneration;
  _invalidateAccumulatorEpoch();
  return Status::Ok();
}

Status INA228::setShuntTempCoeff(uint16_t ppmPerC) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (ppmPerC > cmd::TEMPCO_MAX) {
    return Status::Error(Err::INVALID_PARAM, "Temp coeff exceeds 16383 ppm/C");
  }

  const uint16_t old = _config.shuntTempCoeffPpmC;
  _config.shuntTempCoeffPpmC = ppmPerC;
  uint16_t val = ppmPerC & cmd::MASK_SHUNT_TEMPCO;
  Status st = writeReg16(cmd::REG_SHUNT_TEMPCO, val);
  if (!st.ok()) {
    _config.shuntTempCoeffPpmC = old;
    _markHardwareDirty(cmd::REG_SHUNT_TEMPCO, st);
  } else {
    _invalidateAccumulatorEpoch();
  }
  return st;
}

Status INA228::setTempCompensation(bool enable) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;

  const bool old = _config.tempCompEnabled;
  _config.tempCompEnabled = enable;
  Status st = writeReg16(cmd::REG_CONFIG, _buildConfig());
  if (!st.ok()) {
    _config.tempCompEnabled = old;
    _markHardwareDirty(cmd::REG_CONFIG, st);
  } else {
    _invalidateAccumulatorEpoch();
  }
  return st;
}

Status INA228::setConversionDelay(uint8_t steps2ms) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;

  const uint8_t old = _config.convDelayMs2;
  _config.convDelayMs2 = steps2ms;
  Status st = writeReg16(cmd::REG_CONFIG, _buildConfig());
  if (!st.ok()) {
    _config.convDelayMs2 = old;
    _markHardwareDirty(cmd::REG_CONFIG, st);
  } else {
    _invalidateAccumulatorEpoch();
  }
  return st;
}

Status INA228::startApplyCalibration() {
  uint32_t operationId = 0;
  return startReinitialize(0, operationId);
}

Status INA228::pollApplyCalibration(uint32_t nowMs, uint8_t maxInstructions) {
  if (!_cooperativeJobActive() ||
      _jobSnapshot.kind != JobKind::REINITIALIZE) {
    return Status::Error(Err::INVALID_PARAM, "No config replay job active");
  }
  const uint32_t operationId = _jobSnapshot.operationId;
  Status polled = pollJob(nowMs, maxInstructions);
  if (polled.inProgress()) return polled;
  JobResult result{};
  Status taken = takeJobResult(operationId, result);
  return taken.ok() ? polled : taken;
}

// ===========================================================================
// Alert Configuration
// ===========================================================================

Status INA228::readDiagAlert(DiagAlert& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint16_t raw = 0;
  Status st = _readDiagAlertTracked(raw);
  if (!st.ok()) return st;

  parseDiagAlert(raw, out);

  return Status::Ok();
}

Status INA228::readDiagAlertRaw(uint16_t& raw) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  return _readDiagAlertTracked(raw);
}

Status INA228::readAndClearDiagAlert(uint16_t& raw) {
  return readDiagAlertRaw(raw);
}

Status INA228::setAlertLatch(bool latch) {
  Status st = _setAlertConfigBit(cmd::DIAG_ALATCH, latch);
  if (st.ok()) _config.alerts.latched = latch;
  return st;
}

Status INA228::setConversionReadyAlert(bool enable) {
  Status st = _setAlertConfigBit(cmd::DIAG_CNVR, enable);
  if (st.ok()) _config.alerts.conversionReady = enable;
  return st;
}

Status INA228::setSlowAlert(bool enable) {
  Status st = _setAlertConfigBit(cmd::DIAG_SLOWALERT, enable);
  if (st.ok()) _config.alerts.slowAlert = enable;
  return st;
}

Status INA228::setAlertPolarity(bool activeHigh) {
  Status st = _setAlertConfigBit(cmd::DIAG_APOL, activeHigh);
  if (st.ok()) _config.alerts.activeHigh = activeHigh;
  return st;
}

Status INA228::setShuntOvervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  uint16_t regVal = 0;
  Status encoded = encodeShuntThreshold(voltageV, _config.adcRange, regVal);
  return encoded.ok() ? _writeThresholdRegister(cmd::REG_SOVL, regVal) : encoded;
}

Status INA228::setShuntUndervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  uint16_t regVal = 0;
  Status encoded = encodeShuntThreshold(voltageV, _config.adcRange, regVal);
  return encoded.ok() ? _writeThresholdRegister(cmd::REG_SUVL, regVal) : encoded;
}

Status INA228::setBusOvervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  uint16_t regVal = 0;
  Status encoded = encodeBusThreshold(voltageV, regVal);
  return encoded.ok() ? _writeThresholdRegister(cmd::REG_BOVL, regVal) : encoded;
}

Status INA228::setBusUndervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  uint16_t regVal = 0;
  Status encoded = encodeBusThreshold(voltageV, regVal);
  return encoded.ok() ? _writeThresholdRegister(cmd::REG_BUVL, regVal) : encoded;
}

Status INA228::setTemperatureOverlimitThreshold(float tempC) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!std::isfinite(tempC)) {
    return Status::Error(Err::INVALID_PARAM, "Temperature threshold must be finite");
  }

  const double scaled = std::round(static_cast<double>(tempC) / cmd::TEMP_LSB);
  if (scaled < -32768.0 || scaled > 32767.0) {
    return Status::Error(Err::INVALID_PARAM, "Temperature threshold out of range");
  }
  auto regVal = static_cast<int16_t>(scaled);
  return _writeThresholdRegister(cmd::REG_TEMP_LIMIT, static_cast<uint16_t>(regVal));
}

Status INA228::setPowerOverlimitThreshold(float powerW) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status calStatus = _ensureCalibrated();
  if (!calStatus.ok()) return calStatus;
  if (!std::isfinite(powerW) || powerW < 0.0f) {
    return Status::Error(Err::INVALID_PARAM, "Power threshold out of range");
  }

  const double powerLsb =
      cmd::POWER_COEFF * static_cast<double>(_currentLsb);
  const double limitLsb = 256.0 * powerLsb;
  const double scaled = std::round(static_cast<double>(powerW) / limitLsb);
  if (scaled < 0.0 || scaled > 65535.0) {
    return Status::Error(Err::INVALID_PARAM, "Power threshold out of range");
  }
  auto regVal = static_cast<uint16_t>(scaled);
  return _writeThresholdRegister(cmd::REG_PWR_LIMIT, regVal);
}

// ===========================================================================
// Device Control
// ===========================================================================

Status INA228::softReset() {
  return Status::Error(Err::INVALID_CONFIG,
                       "Use startReset()/pollJob() for bounded startup wait");
}

Status INA228::startResetJob() {
  uint32_t operationId = 0;
  return startReset(0, operationId);
}

Status INA228::pollResetJob(uint32_t nowMs, uint8_t maxInstructions) {
  if (!_cooperativeJobActive() || _jobSnapshot.kind != JobKind::RESET) {
    return Status::Error(Err::INVALID_PARAM, "No reset job active");
  }
  const uint32_t operationId = _jobSnapshot.operationId;
  Status polled = pollJob(nowMs, maxInstructions);
  if (polled.inProgress()) return polled;
  JobResult result{};
  Status taken = takeJobResult(operationId, result);
  return taken.ok() ? polled : taken;
}

Status INA228::resetAccumulators() {
  uint32_t operationId = 0;
  Status staged = startAccumulatorReset(0, operationId);
  if (!staged.ok()) return staged;
  staged = _pollJobImpl(_nowMs(), 2, _config.nowMs != nullptr);
  JobResult result{};
  Status taken = takeJobResult(operationId, result);
  return taken.ok() ? staged : taken;
}

Status INA228::readManufacturerId(uint16_t& id) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  return readReg16(cmd::REG_MANUFACTURER_ID, id);
}

Status INA228::readDeviceId(uint16_t& id) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  return readReg16(cmd::REG_DEVICE_ID, id);
}

// ===========================================================================
// Timing
// ===========================================================================

uint32_t INA228::estimateConversionTimeUs() const {
  uint32_t timeUs = 0;
  if (modeHasShunt(_config.mode)) {
    timeUs += convTimeUs(_config.vshuntConvTime);
  }
  if (modeHasBus(_config.mode)) {
    timeUs += convTimeUs(_config.vbusConvTime);
  }
  if (modeHasTemp(_config.mode)) {
    timeUs += convTimeUs(_config.vtempConvTime);
  }
  if (timeUs == 0U) {
    return 0;
  }
  timeUs *= avgCount(_config.averaging);

  // Add conversion delay
  timeUs += static_cast<uint32_t>(_config.convDelayMs2) * 2000U;

  return timeUs;
}

uint32_t INA228::estimateConversionTimeMs() const {
  return (estimateConversionTimeUs() + 999U) / 1000U;
}

// ===========================================================================
// Transport Wrappers
// ===========================================================================

Status INA228::_i2cWriteReadRaw(const uint8_t* txBuf, size_t txLen,
                                uint8_t* rxBuf, size_t rxLen) {
  if (!_hardwareAccessAllowed()) {
    return Status::Error(Err::BUSY, "Cooperative job owns hardware access");
  }
  if (txBuf == nullptr || txLen == 0 || (rxLen > 0 && rxBuf == nullptr)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid I2C buffer");
  }
  if (_config.i2cWriteRead == nullptr) {
    return Status::Error(Err::INVALID_CONFIG, "I2C write-read not set");
  }
  return _config.i2cWriteRead(_config.i2cAddress, txBuf, txLen, rxBuf, rxLen,
                              _config.i2cTimeoutMs, _config.i2cUser);
}

Status INA228::_i2cWriteRaw(const uint8_t* buf, size_t len) {
  if (!_hardwareAccessAllowed()) {
    return Status::Error(Err::BUSY, "Cooperative job owns hardware access");
  }
  if (buf == nullptr || len == 0) {
    return Status::Error(Err::INVALID_PARAM, "Invalid I2C buffer");
  }
  if (_config.i2cWrite == nullptr) {
    return Status::Error(Err::INVALID_CONFIG, "I2C write not set");
  }
  return _config.i2cWrite(_config.i2cAddress, buf, len, _config.i2cTimeoutMs,
                          _config.i2cUser);
}

Status INA228::_i2cWriteReadTracked(const uint8_t* txBuf, size_t txLen,
                                    uint8_t* rxBuf, size_t rxLen) {
  if (txBuf == nullptr || txLen == 0 || (rxLen > 0 && rxBuf == nullptr)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid I2C buffer");
  }
  Status allowed = _ensureNormalI2cAllowed();
  if (!allowed.ok()) {
    return allowed;
  }

  Status st = _i2cWriteReadRaw(txBuf, txLen, rxBuf, rxLen);
  if (st.code == Err::INVALID_CONFIG || st.code == Err::INVALID_PARAM) {
    return st;
  }
  return _updateHealth(st);
}

Status INA228::_i2cWriteTracked(const uint8_t* buf, size_t len) {
  if (buf == nullptr || len == 0) {
    return Status::Error(Err::INVALID_PARAM, "Invalid I2C buffer");
  }
  Status allowed = _ensureNormalI2cAllowed();
  if (!allowed.ok()) {
    return allowed;
  }

  Status st = _i2cWriteRaw(buf, len);
  if (st.code == Err::INVALID_CONFIG || st.code == Err::INVALID_PARAM) {
    return st;
  }
  return _updateHealth(st);
}

// ===========================================================================
// Register Access
// ===========================================================================

Status INA228::readReg16(uint8_t reg, uint16_t& value) {
  uint8_t buf[2] = {};
  Status st = _i2cWriteReadTracked(&reg, 1, buf, 2);
  if (!st.ok()) return st;

  value = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
  return Status::Ok();
}

Status INA228::readReg24(uint8_t reg, uint32_t& value) {
  uint8_t buf[3] = {};
  Status st = _i2cWriteReadTracked(&reg, 1, buf, 3);
  if (!st.ok()) return st;

  value = (static_cast<uint32_t>(buf[0]) << 16) |
          (static_cast<uint32_t>(buf[1]) << 8) |
          buf[2];
  return Status::Ok();
}

Status INA228::readReg40(uint8_t reg, uint64_t& value) {
  uint8_t buf[5] = {};
  Status st = _i2cWriteReadTracked(&reg, 1, buf, 5);
  if (!st.ok()) return st;

  value = (static_cast<uint64_t>(buf[0]) << 32) |
          (static_cast<uint64_t>(buf[1]) << 24) |
          (static_cast<uint64_t>(buf[2]) << 16) |
          (static_cast<uint64_t>(buf[3]) << 8) |
          buf[4];
  return Status::Ok();
}

Status INA228::writeReg16(uint8_t reg, uint16_t value) {
  const uint8_t payload[3] = {
    reg,
    static_cast<uint8_t>(value >> 8),
    static_cast<uint8_t>(value & 0xFF)
  };
  return _i2cWriteTracked(payload, 3);
}

Status INA228::readRegister16(uint8_t reg, uint16_t& value) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (reg == cmd::REG_DIAG_ALRT) {
    return _readDiagAlertTracked(value);
  }
  return readReg16(reg, value);
}

Status INA228::readRegister24(uint8_t reg, uint32_t& value) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  return readReg24(reg, value);
}

Status INA228::readRegister40(uint8_t reg, uint64_t& value) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  return readReg40(reg, value);
}

Status INA228::writeRegister16(uint8_t reg, uint16_t value) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (!_hardwareAccessAllowed()) {
    return Status::Error(Err::BUSY, "Cooperative job owns hardware access");
  }
  Status st = writeReg16(reg, value);
  if (!st.ok() && _isThresholdRegister(reg)) {
    // A failed threshold write may still have landed; the sticky advisory is
    // the only way the owner learns the limit needs reapplying.
    _markThresholdsDirty();
  }
  if (!st.ok() &&
      (reg == cmd::REG_DIAG_ALRT || reg == cmd::REG_CONFIG ||
       reg == cmd::REG_SHUNT_CAL || reg == cmd::REG_ADC_CONFIG ||
       reg == cmd::REG_SHUNT_TEMPCO)) {
    _markHardwareDirty(reg, st);
    if (reg == cmd::REG_CONFIG && (value & cmd::CONFIG_RST) != 0) {
      _invalidateTriggeredConversionTiming();
    }
  } else if (st.ok() && reg == cmd::REG_DIAG_ALRT) {
    _diagAlertConfigBits = value & cmd::DIAG_CONFIG_MASK;
    _config.alerts.latched = (value & cmd::DIAG_ALATCH) != 0;
    _config.alerts.conversionReady = (value & cmd::DIAG_CNVR) != 0;
    _config.alerts.slowAlert = (value & cmd::DIAG_SLOWALERT) != 0;
    _config.alerts.activeHigh = (value & cmd::DIAG_APOL) != 0;
  } else if (st.ok() && reg == cmd::REG_CONFIG &&
             ((value & cmd::CONFIG_RST) != 0)) {
    _invalidateAccumulatorEpoch();
    _markConfigReplayDirty(
        Status::Error(Err::HARDWARE_DIRTY, "Raw software reset write"));
    _markCalibrationDirty(
        Status::Error(Err::HARDWARE_DIRTY, "Raw software reset write"));
    _markThresholdsDirty();
  } else if (st.ok() && reg == cmd::REG_CONFIG &&
             ((value & cmd::CONFIG_RSTACC) != 0)) {
    _invalidateAccumulatorEpoch();
    _markHardwareDirty(reg);
  } else if (st.ok() && (reg == cmd::REG_CONFIG || reg == cmd::REG_SHUNT_CAL ||
                         reg == cmd::REG_ADC_CONFIG || reg == cmd::REG_SHUNT_TEMPCO)) {
    _markHardwareDirty(reg);
  } else if (st.ok() && _isThresholdRegister(reg)) {
    _markThresholdsDirty();
  }
  return st;
}

Status INA228::_readReg16Raw(uint8_t reg, uint16_t& value) {
  uint8_t buf[2] = {};
  Status st = _i2cWriteReadRaw(&reg, 1, buf, 2);
  if (!st.ok()) return st;

  value = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
  return Status::Ok();
}

Status INA228::_readDiagAlertTracked(uint16_t& raw) {
  Status st = readReg16(cmd::REG_DIAG_ALRT, raw);
  if (st.ok()) {
    _captureDiagAlert(raw);
  }
  return st;
}

void INA228::_captureDiagAlert(uint16_t raw) {
  _captureDiagAlert(raw, _nowMs());
}

void INA228::_captureDiagAlert(uint16_t raw, uint32_t observedAtMs) {
  const bool hadSnapshot = _diagAlertSnapshot.valid;
  const uint16_t preservedEvidence =
      _diagAlertSnapshot.valid ? (_diagAlertSnapshot.raw & DIAG_EVIDENCE_MASK) : 0;
  const uint16_t combinedRaw =
      (raw & cmd::DIAG_CONFIG_MASK) | (raw & cmd::DIAG_MEMSTAT) |
      preservedEvidence | (raw & DIAG_EVIDENCE_MASK);
  _diagAlertSnapshot.valid = true;
  _diagAlertSnapshot.raw = combinedRaw;
  parseDiagAlert(combinedRaw, _diagAlertSnapshot.diag);
  const uint16_t observedEvents = raw & DIAG_EVIDENCE_MASK;
  if (!hadSnapshot || observedEvents != 0) {
    _diagAlertSnapshot.capturedMs = observedAtMs;
  }
  _diagnosticEvents.valid = true;
  _diagnosticEvents.latestRaw = raw;
  const uint16_t newlyObserved = static_cast<uint16_t>(
      observedEvents & ~_diagnosticEvents.stickyEvents);
  _diagnosticEvents.newlyObservedEvents = newlyObserved;
  _diagnosticEvents.stickyEvents |= observedEvents;
  if (observedEvents != 0) {
    _diagnosticEvents.observedAtMs = observedAtMs;
    for (uint8_t bit = 0; bit < 16; ++bit) {
      const uint16_t bitMask = static_cast<uint16_t>(1U) << bit;
      if ((newlyObserved & bitMask) != 0) {
        _diagnosticEvents.firstObservedAtMs[bit] = observedAtMs;
      }
    }
  }
  if (_trigPending && ((raw & cmd::DIAG_CNVRF) != 0)) {
    _completeTriggeredConversion();
  }
}

Status INA228::_writeDiagAlertConfig(uint16_t configBits) {
  const uint16_t sanitized = configBits & cmd::DIAG_CONFIG_MASK;
  Status st = writeReg16(cmd::REG_DIAG_ALRT, sanitized);
  if (st.ok()) {
    _diagAlertConfigBits = sanitized;
  } else {
    _markHardwareDirty(cmd::REG_DIAG_ALRT, st);
  }
  return st;
}

Status INA228::_setAlertConfigBit(uint16_t bit, bool enabled) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;

  uint16_t configBits = _diagAlertConfigBits;
  if (enabled) {
    configBits |= bit;
  } else {
    configBits &= static_cast<uint16_t>(~bit);
  }
  return _writeDiagAlertConfig(configBits);
}

// ===========================================================================
// Health Management
// ===========================================================================

Status INA228::_updateHealth(const Status& st) {
  // Health describes steady-state transport. Initialization and recovery
  // traffic is deliberately excluded so a recovery attempt cannot inflate the
  // counters that gate recovery.
  if (!_initialized) {
    return st;
  }
  if (st.inProgress()) {
    return st;
  }

  const uint32_t now = _nowMs();
  const uint32_t maxU32 = std::numeric_limits<uint32_t>::max();
  const uint8_t maxU8 = std::numeric_limits<uint8_t>::max();

  if (st.ok()) {
    _lastOkMs = now;
    if (_totalSuccess < maxU32) {
      _totalSuccess++;
    }
    _consecutiveFailures = 0;

    _driverState = DriverState::READY;
    return st;
  }

  _lastError = st;
  _lastErrorMs = now;
  if (_totalFailures < maxU32) {
    _totalFailures++;
  }
  if (_consecutiveFailures < maxU8) {
    _consecutiveFailures++;
  }

  if (_config.healthPolicy == HealthPolicy::LATCH_OFFLINE &&
      _consecutiveFailures >= _config.offlineThreshold) {
    _driverState = DriverState::OFFLINE;
  } else {
    _driverState = DriverState::DEGRADED;
  }

  return st;
}

Status INA228::_ensureNormalI2cAllowed() const {
  if (!_hardwareAccessAllowed()) {
    return Status::Error(Err::BUSY, "Cooperative job owns hardware access");
  }
  if (_config.healthPolicy == HealthPolicy::LATCH_OFFLINE && _initialized &&
      _driverState == DriverState::OFFLINE && !_jobPollActive) {
    return Status::Error(Err::BUSY, "Driver is offline; call recover()");
  }
  return Status::Ok();
}

// ===========================================================================
// Internal Helpers
// ===========================================================================

Status INA228::_ensureMeasurementReadyForRead() {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) {
    return clean;
  }
  if (!_trigPending) {
    return Status::Ok();
  }
  if (_config.nowMs == nullptr ||
      _deferredTimeOrigin == DeferredTimeOrigin::TRIGGERED_CONVERSION) {
    return Status::Error(Err::MEASUREMENT_NOT_READY,
                         "Triggered conversion requires explicit readiness poll");
  }
  const uint32_t now = _nowMs();
  if (!_triggerDeadlineElapsed(now)) {
    return Status::Error(Err::MEASUREMENT_NOT_READY, "Triggered conversion not ready");
  }

  bool ready = false;
  Status st = pollConversionReady(now, ready);
  if (!st.ok()) {
    return st;
  }
  if (!ready) {
    return Status::Error(Err::MEASUREMENT_NOT_READY, "Triggered conversion not ready");
  }

  return Status::Ok();
}

Status INA228::_prepareCalibratedMeasurementRead(uint16_t& diagAlert) {
  Status ready = _ensureMeasurementReadyForRead();
  if (!ready.ok()) return ready;
  Status calibrated = _ensureCalibrated();
  if (!calibrated.ok()) return calibrated;
  return _readAndValidateMathDiag(diagAlert);
}

Status INA228::_ensureHardwareClean() const {
  if (!_hardwareAccessAllowed()) {
    return Status::Error(Err::BUSY, "Cooperative job owns hardware access");
  }
  if (_hardwareState != HardwareState::SYNCHRONIZED) {
    return Status::Error(Err::HARDWARE_STATE_UNKNOWN,
                         "Hardware state is not verified");
  }
  if (_hardwareDirty) {
    return Status::Error(Err::HARDWARE_DIRTY,
                         "Hardware config may not match cache; call recover()",
                         static_cast<int32_t>(_dirtyRegisterMask & 0x7FFFFFFFULL));
  }
  return Status::Ok();
}

Status INA228::_ensureCalibrated() const {
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) {
    return clean;
  }
  if (!std::isfinite(_currentLsb) || _currentLsb <= 0.0f || _shuntCal == 0) {
    return Status::Error(Err::INVALID_CONFIG, "Current calibration required");
  }
  return Status::Ok();
}

bool INA228::_triggerDeadlineElapsed(uint32_t nowMs) const {
  return _deferredTimeOrigin != DeferredTimeOrigin::TRIGGERED_CONVERSION &&
         waitElapsed(nowMs, _trigStartMs, estimateConversionTimeMs());
}

bool INA228::_modeSupportsEnergyAccumulation() const {
  return isContinuousMode(_config.mode) && modeHasShunt(_config.mode) &&
         modeHasBus(_config.mode);
}

bool INA228::_modeSupportsChargeAccumulation() const {
  return isContinuousMode(_config.mode) && modeHasShunt(_config.mode);
}

Status INA228::_ensureEnergyAccumulatorReadable() const {
  if (!_modeSupportsEnergyAccumulation()) {
    return Status::Error(Err::ACCUMULATION_INVALID,
                         "Energy requires continuous shunt and bus conversion",
                         static_cast<int32_t>(_config.mode));
  }
  if (!_accumulationReady || _accumulatorGeneration == 0 ||
      _accumulatorGeneration != _configurationGeneration) {
    return Status::Error(Err::ACCUMULATION_INVALID,
                         "Energy accumulation not ready");
  }
  return Status::Ok();
}

Status INA228::_ensureChargeAccumulatorReadable() const {
  if (!_modeSupportsChargeAccumulation()) {
    return Status::Error(Err::ACCUMULATION_INVALID,
                         "Charge requires continuous shunt conversion",
                         static_cast<int32_t>(_config.mode));
  }
  if (!_accumulationReady || _accumulatorGeneration == 0 ||
      _accumulatorGeneration != _configurationGeneration) {
    return Status::Error(Err::ACCUMULATION_INVALID,
                         "Charge accumulation not ready");
  }
  return Status::Ok();
}

Status INA228::_readAndValidateMathDiag(uint16_t& raw) {
  raw = 0;
  Status st = _readDiagAlertTracked(raw);
  if (!st.ok()) {
    return st;
  }
  if ((raw & cmd::DIAG_MATHOF) != 0) {
    return Status::Error(Err::MATH_OVERFLOW, "INA228 math overflow",
                         static_cast<int32_t>(raw & DIAG_EVIDENCE_MASK));
  }
  return Status::Ok();
}

Status INA228::_validateAccumulatorDiag(uint16_t raw, uint16_t overflowBit,
                                        const char* overflowMsg) const {
  const uint16_t accumulatorFlags =
      raw & (cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF | cmd::DIAG_MATHOF);
  if ((raw & cmd::DIAG_MATHOF) != 0) {
    return Status::Error(Err::MATH_OVERFLOW, "INA228 math overflow",
                         static_cast<int32_t>(accumulatorFlags));
  }
  if ((raw & overflowBit) != 0) {
    return Status::Error(Err::ACCUMULATION_OVERFLOW, overflowMsg,
                         static_cast<int32_t>(accumulatorFlags));
  }
  return Status::Ok();
}

void INA228::_markTriggeredConversionStarted() {
  _invalidateAccumulatorEpoch();
  _trigPending = true;
  _trigStartMs = 0;
  _armPostWriteTimeOrigin(DeferredTimeOrigin::TRIGGERED_CONVERSION);
  _clearCapturedConversionReadyFlag();
}

void INA228::_invalidateTriggeredConversionTiming() {
  _trigPending = false;
  _trigStartMs = 0;
  if (_deferredTimeOrigin == DeferredTimeOrigin::TRIGGERED_CONVERSION) {
    _deferredTimeOrigin = DeferredTimeOrigin::NONE;
  }
}

void INA228::_completeTriggeredConversion() {
  const bool wasTriggeredMode = isTriggeredMode(_config.mode);
  _invalidateTriggeredConversionTiming();
  _invalidateAccumulatorEpoch();
  if (wasTriggeredMode) {
    _config.mode = Mode::SHUTDOWN;
  }
}

void INA228::_clearCapturedConversionReadyFlag() {
  if (!_diagAlertSnapshot.valid || ((_diagAlertSnapshot.raw & cmd::DIAG_CNVRF) == 0)) {
    return;
  }
  _diagAlertSnapshot.raw &= static_cast<uint16_t>(~cmd::DIAG_CNVRF);
  parseDiagAlert(_diagAlertSnapshot.raw, _diagAlertSnapshot.diag);
}

void INA228::_clearCapturedAccumulatorEvidence() {
  if (!_diagAlertSnapshot.valid) {
    return;
  }
  _diagAlertSnapshot.raw &=
      static_cast<uint16_t>(~(cmd::DIAG_CNVRF | cmd::DIAG_ENERGYOF |
                             cmd::DIAG_CHARGEOF | cmd::DIAG_MATHOF));
  parseDiagAlert(_diagAlertSnapshot.raw, _diagAlertSnapshot.diag);
}

void INA228::_markHardwareDirty(uint8_t reg) {
  _markHardwareDirty(reg, Status::Error(Err::HARDWARE_DIRTY,
                                        "Hardware register may differ from cache"));
}

void INA228::_markHardwareDirty(uint8_t reg, const Status& cause) {
  if (!_hardwareDirty) {
    _hardwareDirtyCause = cause.ok()
                              ? Status::Error(Err::HARDWARE_DIRTY,
                                              "Hardware register may differ from cache")
                              : cause;
  }
  _hardwareDirty = true;
  if (_bound) {
    _hardwareState = HardwareState::RESYNC_REQUIRED;
  }
  if (reg < 64) {
    _dirtyRegisterMask |= (uint64_t{1} << reg);
  }
  if (reg == cmd::REG_ADC_CONFIG) {
    _invalidateTriggeredConversionTiming();
  }
}

void INA228::_markConfigReplayDirty(const Status& cause) {
  _markHardwareDirty(cmd::REG_CONFIG, cause);
  _markHardwareDirty(cmd::REG_ADC_CONFIG, cause);
  _markHardwareDirty(cmd::REG_DIAG_ALRT, cause);
  _markHardwareDirty(cmd::REG_SHUNT_TEMPCO, cause);
}

void INA228::_markCalibrationDirty(const Status& cause) {
  _markHardwareDirty(cmd::REG_SHUNT_CAL, cause);
}

void INA228::_clearHardwareDirty() {
  _hardwareDirty = false;
  _dirtyRegisterMask = 0;
  _hardwareDirtyCause = Status::Ok();
}

void INA228::_markThresholdsDirty() {
  _thresholdsDirty = true;
}

Status INA228::_writeThresholdRegister(uint8_t reg, uint16_t value) {
  Status st = writeReg16(reg, value);
  if (!st.ok()) {
    // The write may still have landed; the owner must reapply the limit.
    _markThresholdsDirty();
  }
  return st;
}

bool INA228::_isThresholdRegister(uint8_t reg) const {
  return reg == cmd::REG_SOVL ||
         reg == cmd::REG_SUVL ||
         reg == cmd::REG_BOVL ||
         reg == cmd::REG_BUVL ||
         reg == cmd::REG_TEMP_LIMIT ||
         reg == cmd::REG_PWR_LIMIT;
}

uint32_t INA228::_nowMs() const {
  if (_config.nowMs != nullptr) {
    return _config.nowMs(_config.timeUser);
  }
  return 0;
}

void INA228::_armPostWriteTimeOrigin(DeferredTimeOrigin origin) {
  if (_config.nowMs == nullptr) {
    _deferredTimeOrigin = origin;
    return;
  }

  const uint32_t postWriteMs = _nowMs();
  if (origin == DeferredTimeOrigin::JOB_WAIT) {
    _jobWaitStartMs = postWriteMs;
  } else if (origin == DeferredTimeOrigin::TRIGGERED_CONVERSION) {
    _trigStartMs = postWriteMs;
  }
  _deferredTimeOrigin = DeferredTimeOrigin::NONE;
}

uint16_t INA228::_buildAdcConfig() const {
  return _buildAdcConfig(_config.mode);
}

uint16_t INA228::_buildAdcConfig(Mode mode) const {
  return static_cast<uint16_t>(
    (static_cast<uint16_t>(mode) << cmd::BIT_ADC_MODE) |
    (static_cast<uint16_t>(_config.vbusConvTime) << cmd::BIT_ADC_VBUSCT) |
    (static_cast<uint16_t>(_config.vshuntConvTime) << cmd::BIT_ADC_VSHCT) |
    (static_cast<uint16_t>(_config.vtempConvTime) << cmd::BIT_ADC_VTCT) |
    (static_cast<uint16_t>(_config.averaging) << cmd::BIT_ADC_AVG)
  );
}

uint16_t INA228::_buildConfig() const {
  uint16_t cfg = 0;
  cfg |= (static_cast<uint16_t>(_config.convDelayMs2) << cmd::BIT_CONFIG_CONVDLY);
  if (_config.tempCompEnabled) {
    cfg |= cmd::CONFIG_TEMPCOMP;
  }
  if (_config.adcRange == AdcRange::MV_40_96) {
    cfg |= cmd::CONFIG_ADCRANGE;
  }
  return cfg;
}

int32_t INA228::_signExtend20(uint32_t raw24) {
  const uint32_t value = (raw24 >> 4) & 0x000FFFFFu;
  if ((value & 0x00080000u) == 0) {
    return static_cast<int32_t>(value);
  }
  const uint32_t magnitude = ((~value) & 0x000FFFFFu) + 1u;
  return -static_cast<int32_t>(magnitude);
}

int64_t INA228::_signExtend40(uint64_t raw40) {
  const uint64_t value = raw40 & 0x000000FFFFFFFFFFULL;
  if ((value & 0x0000008000000000ULL) == 0) {
    return static_cast<int64_t>(value);
  }
  const uint64_t magnitude = ((~value) & 0x000000FFFFFFFFFFULL) + 1ULL;
  return -static_cast<int64_t>(magnitude);
}

}  // namespace INA228
