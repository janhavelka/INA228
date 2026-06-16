/**
 * @file INA228.cpp
 * @brief INA228 driver implementation.
 */

#include "INA228/INA228.h"

#include <cstring>
#include <limits>
#include <cmath>

namespace INA228 {
namespace {

static constexpr size_t MAX_WRITE_LEN = 6;
static constexpr uint8_t RESET_VERIFY_ATTEMPTS = 3;
static constexpr uint32_t RESET_STARTUP_MS =
    (cmd::POR_STARTUP_US + 999U) / 1000U;
static constexpr uint16_t DIAG_EVIDENCE_MASK =
    cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF | cmd::DIAG_MATHOF |
    cmd::DIAG_TMPOL | cmd::DIAG_SHNTOL | cmd::DIAG_SHNTUL |
    cmd::DIAG_BUSOL | cmd::DIAG_BUSUL | cmd::DIAG_POL |
    cmd::DIAG_CNVRF;

class ScopedOfflineI2cAllowance {
public:
  explicit ScopedOfflineI2cAllowance(bool& flag, bool allow) : _flag(flag), _old(flag) {
    _flag = allow;
  }

  ~ScopedOfflineI2cAllowance() {
    _flag = _old;
  }

  ScopedOfflineI2cAllowance(const ScopedOfflineI2cAllowance&) = delete;
  ScopedOfflineI2cAllowance& operator=(const ScopedOfflineI2cAllowance&) = delete;

private:
  bool& _flag;
  bool _old;
};

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

Status INA228::begin(const Config& config) {
  _config = Config{};
  _initialized = false;
  _driverState = DriverState::UNINIT;
  _allowOfflineI2c = false;

  _lastOkMs = 0;
  _lastErrorMs = 0;
  _lastError = Status::Ok();
  _consecutiveFailures = 0;
  _totalFailures = 0;
  _totalSuccess = 0;

  _currentLsb = 0.0f;
  _shuntCal = 0;
  _calibrationClamped = false;
  _maxCurrentExceedsShuntRange = false;
  _hardwareDirty = false;
  _dirtyRegisterMask = 0;
  _hardwareDirtyCause = Status::Ok();
  _thresholdsDirty = false;
  _trigPending = false;
  _trigStartMs = 0;
  _accumulationReady = false;
  _diagAlertConfigBits = 0;
  _diagAlertSnapshot = DiagAlertSnapshot{};
  _clearAsyncJob();

  if (config.i2cWrite == nullptr || config.i2cWriteRead == nullptr) {
    return Status::Error(Err::INVALID_CONFIG, "I2C callbacks not set");
  }
  if (config.i2cTimeoutMs == 0) {
    return Status::Error(Err::INVALID_CONFIG, "I2C timeout must be > 0");
  }
  if (!isValidAddress(config.i2cAddress)) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid I2C address (0x40-0x4F)");
  }
  if (!isValidMode(config.mode)) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid operating mode");
  }
  if (!isValidConvTime(config.vbusConvTime) ||
      !isValidConvTime(config.vshuntConvTime) ||
      !isValidConvTime(config.vtempConvTime)) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid conversion time");
  }
  if (!isValidAveraging(config.averaging)) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid averaging count");
  }
  if (!isValidAdcRange(config.adcRange)) {
    return Status::Error(Err::INVALID_CONFIG, "Invalid ADC range");
  }
  if (config.shuntTempCoeffPpmC > cmd::TEMPCO_MAX) {
    return Status::Error(Err::INVALID_CONFIG, "Shunt temp coeff exceeds 16383");
  }
  if (!std::isfinite(config.shuntResistanceOhm) ||
      !std::isfinite(config.maxExpectedCurrentA)) {
    return Status::Error(Err::INVALID_CONFIG, "Calibration values must be finite");
  }
  if ((config.shuntResistanceOhm > 0.0f) != (config.maxExpectedCurrentA > 0.0f)) {
    return Status::Error(Err::INVALID_CONFIG, "Incomplete calibration values");
  }
  if (config.shuntResistanceOhm < 0.0f || config.maxExpectedCurrentA < 0.0f) {
    return Status::Error(Err::INVALID_CONFIG, "Calibration values must be >= 0");
  }
  if (config.shuntResistanceOhm > 0.0f) {
    uint16_t cal = 0;
    float lsb = 0.0f;
    bool clamped = false;
    bool maxCurrentExceedsRange = false;
    Status calStatus = computeCalibration(config.shuntResistanceOhm,
                                          config.maxExpectedCurrentA,
                                          config.adcRange,
                                          cal,
                                          lsb,
                                          clamped,
                                          maxCurrentExceedsRange);
    if (!calStatus.ok()) {
      return Status::Error(Err::INVALID_CONFIG, calStatus.msg, calStatus.detail);
    }
  }

  _config = config;
  if (_config.offlineThreshold == 0) {
    _config.offlineThreshold = 1;
  }

  auto failBeginAfterConfig = [this](const Status& failure) {
    _config = Config{};
    _allowOfflineI2c = false;
    _currentLsb = 0.0f;
    _shuntCal = 0;
    _calibrationClamped = false;
    _maxCurrentExceedsShuntRange = false;
    _hardwareDirty = false;
    _dirtyRegisterMask = 0;
    _hardwareDirtyCause = Status::Ok();
    _thresholdsDirty = false;
    _trigPending = false;
    _trigStartMs = 0;
    _accumulationReady = false;
    _diagAlertConfigBits = 0;
    _clearAsyncJob();
    return failure;
  };

  // Verify device identity using raw reads (before health tracking is active)
  uint16_t mfgId = 0;
  Status st = _readReg16Raw(cmd::REG_MANUFACTURER_ID, mfgId);
  if (!st.ok()) {
    return failBeginAfterConfig(
        mapPresenceReadFailure(st, "Device not responding"));
  }
  if (mfgId != cmd::MANUFACTURER_ID) {
    return failBeginAfterConfig(
        Status::Error(Err::DEVICE_ID_MISMATCH, "Manufacturer ID mismatch",
                      static_cast<int32_t>(mfgId)));
  }

  uint16_t devId = 0;
  st = _readReg16Raw(cmd::REG_DEVICE_ID, devId);
  if (!st.ok()) {
    return failBeginAfterConfig(
        mapPresenceReadFailure(st, "Device ID read failed"));
  }
  if (devId != cmd::DEVICE_ID) {
    return failBeginAfterConfig(
        Status::Error(Err::DEVICE_ID_MISMATCH, "Device ID mismatch",
                      static_cast<int32_t>(devId)));
  }

  // Check MEMSTAT bit
  uint16_t diagAlrt = 0;
  st = _readDiagAlertRaw(diagAlrt);
  if (!st.ok()) {
    return failBeginAfterConfig(
        mapPresenceReadFailure(st, "DIAG_ALRT read failed"));
  }
  if ((diagAlrt & cmd::DIAG_MEMSTAT) == 0) {
    return failBeginAfterConfig(
        Status::Error(Err::MEMORY_ERROR, "NV trim memory checksum error"));
  }

  // Apply configuration
  st = _applyConfig();
  if (!st.ok()) {
    return failBeginAfterConfig(st);
  }

  // Apply calibration if provided
  st = _applyCalibration();
  if (!st.ok()) {
    return failBeginAfterConfig(st);
  }

  _initialized = true;
  _driverState = DriverState::READY;
  if (isTriggeredMode(_config.mode)) {
    _markTriggeredConversionStarted(_nowMs());
  }

  return Status::Ok();
}

void INA228::tick(uint32_t nowMs) {
  if (!_initialized) {
    return;
  }
  if (!_trigPending && (_accumulationReady || !_modeSupportsAnyAccumulation())) {
    return;
  }
  bool ready = false;
  (void)pollConversionReady(nowMs, ready);
}

void INA228::end() {
  if (_initialized) {
    // Best-effort: put device into shutdown mode.
    // Uses raw I2C to avoid health tracking during shutdown.
    uint16_t adcCfg = _buildAdcConfig();
    adcCfg = (adcCfg & ~cmd::MASK_ADC_MODE) |
             (static_cast<uint16_t>(Mode::SHUTDOWN) << cmd::BIT_ADC_MODE);
    const uint8_t payload[3] = {
      cmd::REG_ADC_CONFIG,
      static_cast<uint8_t>(adcCfg >> 8),
      static_cast<uint8_t>(adcCfg & 0xFF)
    };
    (void)_i2cWriteRaw(payload, sizeof(payload));
  }

  _initialized = false;
  _driverState = DriverState::UNINIT;
  _trigPending = false;
  _trigStartMs = 0;
  _accumulationReady = false;
  _currentLsb = 0.0f;
  _shuntCal = 0;
  _calibrationClamped = false;
  _maxCurrentExceedsShuntRange = false;
  _hardwareDirty = false;
  _dirtyRegisterMask = 0;
  _hardwareDirtyCause = Status::Ok();
  _thresholdsDirty = false;
  _diagAlertConfigBits = 0;
  _diagAlertSnapshot = DiagAlertSnapshot{};
  _clearAsyncJob();
}

// ===========================================================================
// Diagnostics
// ===========================================================================

Status INA228::probe() {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint16_t mfgId = 0;
  Status st = _readReg16Raw(cmd::REG_MANUFACTURER_ID, mfgId);
  if (!st.ok()) {
    return mapPresenceReadFailure(st, "Device not responding");
  }
  if (mfgId != cmd::MANUFACTURER_ID) {
    return Status::Error(Err::DEVICE_ID_MISMATCH, "Manufacturer ID mismatch",
                        static_cast<int32_t>(mfgId));
  }

  uint16_t devId = 0;
  st = _readReg16Raw(cmd::REG_DEVICE_ID, devId);
  if (!st.ok()) {
    return mapPresenceReadFailure(st, "Device ID read failed");
  }
  if (devId != cmd::DEVICE_ID) {
    return Status::Error(Err::DEVICE_ID_MISMATCH, "Device ID mismatch",
                        static_cast<int32_t>(devId));
  }

  return Status::Ok();
}

Status INA228::recover() {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  const bool startedOffline = _driverState == DriverState::OFFLINE;
  ScopedOfflineI2cAllowance allowOfflineI2c(_allowOfflineI2c, true);
  Status result = [this]() -> Status {
    _markAccumulationInvalid();
    Status st = _verifyIdentityAndMemstat(true);
    if (!st.ok()) return st;

    _trigPending = false;
    _trigStartMs = 0;

    st = _resyncCachedHardware();
    if (!st.ok()) {
      return st;
    }
    _clearHardwareDirty();
    if (isTriggeredMode(_config.mode)) {
      _markTriggeredConversionStarted(_nowMs());
    }

    return Status::Ok();
  }();
  if (startedOffline && !result.ok() && !result.inProgress()) {
    _reassertOfflineLatch();
  }
  return result;
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
  out.calibrated = _currentLsb > 0.0f && _shuntCal > 0 && !_hardwareDirty;
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }
  Status calStatus = _ensureCalibrated();
  if (!calStatus.ok()) return calStatus;

  uint16_t diag = 0;
  Status st = _readAndValidateMathDiag(diag);
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
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
  st = _readAccumulatorDiag(diag);
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (maxInstructions == 0) {
    return Status::Error(Err::INVALID_PARAM, "maxInstructions must be > 0");
  }
  if (_asyncJob != AsyncJob::NONE && _asyncJob != AsyncJob::POWER_SAMPLE) {
    return Status::Error(Err::BUSY, "Another fixed-step job is active");
  }

  if (_asyncJob == AsyncJob::NONE) {
    Status clean = _ensureHardwareClean();
    if (!clean.ok()) return clean;
    Status calStatus = _ensureCalibrated();
    if (!calStatus.ok()) return calStatus;
    if (_trigPending && !_triggerDeadlineElapsed(_nowMs())) {
      return Status::Error(Err::MEASUREMENT_NOT_READY,
                           "Triggered conversion not ready");
    }

    _powerSampleRawScratch = RawSample{};
    _powerSampleIntegerScratch = IntegerSample{};
    _powerSampleStep = _trigPending ? PowerSampleStep::DIAG_READY
                                    : PowerSampleStep::VSHUNT;
    _asyncJob = AsyncJob::POWER_SAMPLE;
  }

  uint8_t used = 0;
  while (used < maxInstructions) {
    uint32_t raw24 = 0;
    uint16_t raw16 = 0;
    Status st = Status::Ok();

    switch (_powerSampleStep) {
      case PowerSampleStep::DIAG_READY: {
        uint16_t diag = 0;
        st = _readDiagAlertTracked(diag);
        ++used;
        if (!st.ok()) {
          _clearAsyncJob();
          return st;
        }
        _powerSampleRawScratch.diagAlertValid = true;
        _powerSampleRawScratch.diagAlertRaw = diag;
        _powerSampleRawScratch.mathOverflow = (diag & cmd::DIAG_MATHOF) != 0;
        if ((diag & cmd::DIAG_CNVRF) == 0) {
          _clearAsyncJob();
          return Status::Error(Err::MEASUREMENT_NOT_READY,
                               "Triggered conversion not ready");
        }
        _powerSampleStep = PowerSampleStep::VSHUNT;
        break;
      }

      case PowerSampleStep::VSHUNT:
        st = readReg24(cmd::REG_VSHUNT, raw24);
        ++used;
        if (!st.ok()) {
          _clearAsyncJob();
          return st;
        }
        _powerSampleRawScratch.vshunt = _signExtend20(raw24);
        _powerSampleStep = PowerSampleStep::VBUS;
        break;

      case PowerSampleStep::VBUS:
        st = readReg24(cmd::REG_VBUS, raw24);
        ++used;
        if (!st.ok()) {
          _clearAsyncJob();
          return st;
        }
        _powerSampleRawScratch.vbus = raw24 >> 4;
        _powerSampleStep = PowerSampleStep::DIETEMP;
        break;

      case PowerSampleStep::DIETEMP:
        st = readReg16(cmd::REG_DIETEMP, raw16);
        ++used;
        if (!st.ok()) {
          _clearAsyncJob();
          return st;
        }
        _powerSampleRawScratch.dietemp = static_cast<int16_t>(raw16);
        _powerSampleStep = PowerSampleStep::CURRENT;
        break;

      case PowerSampleStep::CURRENT:
        st = readReg24(cmd::REG_CURRENT, raw24);
        ++used;
        if (!st.ok()) {
          _clearAsyncJob();
          return st;
        }
        _powerSampleRawScratch.current = _signExtend20(raw24);
        _powerSampleStep = PowerSampleStep::POWER;
        break;

      case PowerSampleStep::POWER:
        st = readReg24(cmd::REG_POWER, raw24);
        ++used;
        if (!st.ok()) {
          _clearAsyncJob();
          return st;
        }
        _powerSampleRawScratch.power = raw24;
        st = _fillPowerSampleUnits(_powerSampleRawScratch,
                                   _powerSampleIntegerScratch);
        if (!st.ok()) {
          _clearAsyncJob();
          return st;
        }
        rawOut = _powerSampleRawScratch;
        integerOut = _powerSampleIntegerScratch;
        _clearAsyncJob();
        return Status::Ok();

      case PowerSampleStep::IDLE:
      default:
        _clearAsyncJob();
        return Status::Error(Err::BUSY, "No power sample job active");
    }
  }

  return Status{Err::IN_PROGRESS, 0, "Power sample read in progress"};
}

Status INA228::readIntegerSample(IntegerSample& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }
  Status calStatus = _ensureCalibrated();
  if (!calStatus.ok()) return calStatus;

  uint16_t diag = 0;
  Status st = _readAndValidateMathDiag(diag);
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }
  Status calStatus = _ensureCalibrated();
  if (!calStatus.ok()) return calStatus;

  uint16_t diag = 0;
  Status st = _readAndValidateMathDiag(diag);
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status readyStatus = _ensureMeasurementReadyForRead();
  if (!readyStatus.ok()) {
    return readyStatus;
  }
  Status calStatus = _ensureCalibrated();
  if (!calStatus.ok()) return calStatus;

  uint16_t diag = 0;
  Status st = _readAndValidateMathDiag(diag);
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
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
  Status st = _readAccumulatorDiag(diag);
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
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
  Status st = _readAccumulatorDiag(diag);
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
  return pollConversionReady(_nowMs(), ready);
}

Status INA228::pollConversionReady(uint32_t nowMs, bool& ready) {
  ready = false;
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
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
  if (_asyncJob != AsyncJob::NONE) {
    return Status::Error(Err::BUSY, "Another fixed-step job is active");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
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

  uint16_t adcCfg = _buildAdcConfig();
  adcCfg = (adcCfg & ~cmd::MASK_ADC_MODE) |
           (static_cast<uint16_t>(mode) << cmd::BIT_ADC_MODE);
  Status st = writeReg16(cmd::REG_ADC_CONFIG, adcCfg);
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
    return st;
  }

  _config.mode = mode;
  _markAccumulationInvalid();
  if (isTriggeredMode(mode)) {
    _markTriggeredConversionStarted(_nowMs());
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

  uint16_t adcCfg = _buildAdcConfig();
  adcCfg = (adcCfg & ~cmd::MASK_ADC_MODE) |
           (static_cast<uint16_t>(mode) << cmd::BIT_ADC_MODE);
  Status st = writeReg16(cmd::REG_ADC_CONFIG, adcCfg);
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
    return st;
  }

  _config.mode = mode;
  _markAccumulationInvalid();
  _markTriggeredConversionStarted(_nowMs());
  return Status{Err::IN_PROGRESS, 0, "Conversion started"};
}

Status INA228::startTriggeredMeasurement(Mode mode) {
  if (_asyncJob != AsyncJob::NONE) {
    return Status::Error(Err::BUSY, "Another fixed-step job is active");
  }
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
    _markTriggeredConversionStarted(_nowMs());
  } else {
    _markAccumulationInvalid();
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
    _markTriggeredConversionStarted(_nowMs());
  } else {
    _markAccumulationInvalid();
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
    _markTriggeredConversionStarted(_nowMs());
  } else {
    _markAccumulationInvalid();
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
    _markTriggeredConversionStarted(_nowMs());
  } else {
    _markAccumulationInvalid();
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
  const uint16_t oldConfigReg = _buildConfig();

  uint16_t newShuntCal = 0;
  float newCurrentLsb = 0.0f;
  bool newClamped = false;
  bool newMaxCurrentExceedsRange = false;
  if (_config.shuntResistanceOhm > 0.0f && _config.maxExpectedCurrentA > 0.0f) {
    Status st = computeCalibration(_config.shuntResistanceOhm,
                                   _config.maxExpectedCurrentA,
                                   range,
                                   newShuntCal,
                                   newCurrentLsb,
                                   newClamped,
                                   newMaxCurrentExceedsRange);
    if (!st.ok()) {
      return st;
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
    _clearHardwareDirty();
    _markAccumulationInvalid();
    if (range != oldRange) {
      _markThresholdsDirty();
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
  Status st = validatePositiveFinite(shuntOhm, "Shunt must be finite and > 0");
  if (!st.ok()) {
    return st;
  }
  st = validatePositiveFinite(maxCurrentA, "Max current must be finite and > 0");
  if (!st.ok()) {
    return st;
  }

  const float oldShuntOhm = _config.shuntResistanceOhm;
  const float oldMaxCurrentA = _config.maxExpectedCurrentA;
  const float oldCurrentLsb = _currentLsb;
  const uint16_t oldShuntCal = _shuntCal;
  const bool oldClamped = _calibrationClamped;
  const bool oldMaxCurrentExceedsRange = _maxCurrentExceedsShuntRange;
  _config.shuntResistanceOhm = shuntOhm;
  _config.maxExpectedCurrentA = maxCurrentA;
  st = _applyCalibration();
  if (!st.ok()) {
    _config.shuntResistanceOhm = oldShuntOhm;
    _config.maxExpectedCurrentA = oldMaxCurrentA;
    _currentLsb = oldCurrentLsb;
    _shuntCal = oldShuntCal;
    _calibrationClamped = oldClamped;
    _maxCurrentExceedsShuntRange = oldMaxCurrentExceedsRange;
    if (st.code != Err::INVALID_PARAM && st.code != Err::INVALID_CONFIG) {
      _markHardwareDirty(cmd::REG_SHUNT_CAL);
    }
  } else {
    _clearHardwareDirty();
    _markAccumulationInvalid();
    _markThresholdsDirty();
  }
  return st;
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
    _markAccumulationInvalid();
  }
  return st;
}

Status INA228::startApplyCalibration() {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (_asyncJob != AsyncJob::NONE) {
    return Status::Error(Err::BUSY, "Another fixed-step job is active");
  }

  _asyncJob = AsyncJob::APPLY_CALIBRATION;
  _applyStep = ApplyStep::ADC_SHUTDOWN;
  return Status{Err::IN_PROGRESS, 0, "Apply calibration job started"};
}

Status INA228::pollApplyCalibration(uint32_t nowMs, uint8_t maxInstructions) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (maxInstructions == 0) {
    return Status::Error(Err::INVALID_PARAM, "maxInstructions must be > 0");
  }
  if (_asyncJob != AsyncJob::APPLY_CALIBRATION) {
    return Status::Error(Err::BUSY, "No apply calibration job active");
  }

  uint8_t used = 0;
  while (used < maxInstructions) {
    const ApplyStep current = _applyStep;
    Status st = _writeApplyStep(current);
    ++used;
    if (!st.ok()) {
      if (current != ApplyStep::ADC_CONFIG) {
        _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
      }
      _clearAsyncJob();
      return st;
    }

    switch (current) {
      case ApplyStep::ADC_SHUTDOWN:
        _applyStep = ApplyStep::CONFIG;
        break;
      case ApplyStep::CONFIG:
        _applyStep = ApplyStep::DIAG_ALRT;
        break;
      case ApplyStep::DIAG_ALRT:
        _applyStep = ApplyStep::TEMPCO;
        break;
      case ApplyStep::TEMPCO:
        _applyStep = ApplyStep::SHUNT_CAL;
        break;
      case ApplyStep::SHUNT_CAL:
        _applyStep = ApplyStep::ADC_CONFIG;
        break;
      case ApplyStep::ADC_CONFIG:
        _clearHardwareDirty();
        if (isTriggeredMode(_config.mode)) {
          _markTriggeredConversionStarted(nowMs);
        } else {
          _completeTriggeredConversion();
          _markAccumulationInvalid();
        }
        _clearAsyncJob();
        return Status::Ok();
      case ApplyStep::IDLE:
      case ApplyStep::DONE:
      default:
        _clearAsyncJob();
        return Status::Error(Err::BUSY, "No apply calibration job active");
    }
  }

  return Status{Err::IN_PROGRESS, 0, "Apply calibration job in progress"};
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
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;

  uint16_t diag = _diagAlertConfigBits;
  if (latch) {
    diag |= cmd::DIAG_ALATCH;
  } else {
    diag &= ~cmd::DIAG_ALATCH;
  }
  return _writeDiagAlertConfig(diag);
}

Status INA228::setConversionReadyAlert(bool enable) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;

  uint16_t diag = _diagAlertConfigBits;
  if (enable) {
    diag |= cmd::DIAG_CNVR;
  } else {
    diag &= ~cmd::DIAG_CNVR;
  }
  return _writeDiagAlertConfig(diag);
}

Status INA228::setSlowAlert(bool enable) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;

  uint16_t diag = _diagAlertConfigBits;
  if (enable) {
    diag |= cmd::DIAG_SLOWALERT;
  } else {
    diag &= ~cmd::DIAG_SLOWALERT;
  }
  return _writeDiagAlertConfig(diag);
}

Status INA228::setAlertPolarity(bool activeHigh) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;

  uint16_t diag = _diagAlertConfigBits;
  if (activeHigh) {
    diag |= cmd::DIAG_APOL;
  } else {
    diag &= ~cmd::DIAG_APOL;
  }
  return _writeDiagAlertConfig(diag);
}

Status INA228::setShuntOvervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!std::isfinite(voltageV)) {
    return Status::Error(Err::INVALID_PARAM, "Threshold must be finite");
  }

  const double lsb = (_config.adcRange == AdcRange::MV_40_96)
                       ? cmd::SHUNT_THRESHOLD_LSB_RANGE1
                       : cmd::SHUNT_THRESHOLD_LSB_RANGE0;
  const double scaled = std::round(static_cast<double>(voltageV) / lsb);
  if (scaled < -32768.0 || scaled > 32767.0) {
    return Status::Error(Err::INVALID_PARAM, "Shunt threshold out of range");
  }
  auto regVal = static_cast<int16_t>(scaled);
  return writeReg16(cmd::REG_SOVL, static_cast<uint16_t>(regVal));
}

Status INA228::setShuntUndervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!std::isfinite(voltageV)) {
    return Status::Error(Err::INVALID_PARAM, "Threshold must be finite");
  }

  const double lsb = (_config.adcRange == AdcRange::MV_40_96)
                       ? cmd::SHUNT_THRESHOLD_LSB_RANGE1
                       : cmd::SHUNT_THRESHOLD_LSB_RANGE0;
  const double scaled = std::round(static_cast<double>(voltageV) / lsb);
  if (scaled < -32768.0 || scaled > 32767.0) {
    return Status::Error(Err::INVALID_PARAM, "Shunt threshold out of range");
  }
  auto regVal = static_cast<int16_t>(scaled);
  return writeReg16(cmd::REG_SUVL, static_cast<uint16_t>(regVal));
}

Status INA228::setBusOvervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!std::isfinite(voltageV) || voltageV < 0.0f || voltageV > 85.0f) {
    return Status::Error(Err::INVALID_PARAM, "Bus threshold out of range");
  }

  auto regVal = static_cast<uint16_t>(std::round(
      static_cast<double>(voltageV) / cmd::BUS_THRESHOLD_LSB));
  regVal &= 0x7FFF;  // bit 15 reserved
  return writeReg16(cmd::REG_BOVL, regVal);
}

Status INA228::setBusUndervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;
  if (!std::isfinite(voltageV) || voltageV < 0.0f || voltageV > 85.0f) {
    return Status::Error(Err::INVALID_PARAM, "Bus threshold out of range");
  }

  auto regVal = static_cast<uint16_t>(std::round(
      static_cast<double>(voltageV) / cmd::BUS_THRESHOLD_LSB));
  regVal &= 0x7FFF;  // bit 15 reserved
  return writeReg16(cmd::REG_BUVL, regVal);
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
  return writeReg16(cmd::REG_TEMP_LIMIT, static_cast<uint16_t>(regVal));
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

  const double powerLsb = cmd::POWER_COEFF * _currentLsb;
  const double limitLsb = 256.0 * powerLsb;
  const double scaled = std::round(static_cast<double>(powerW) / limitLsb);
  if (scaled < 0.0 || scaled > 65535.0) {
    return Status::Error(Err::INVALID_PARAM, "Power threshold out of range");
  }
  auto regVal = static_cast<uint16_t>(scaled);
  return writeReg16(cmd::REG_PWR_LIMIT, regVal);
}

// ===========================================================================
// Device Control
// ===========================================================================

Status INA228::softReset() {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  const bool startedOffline = _driverState == DriverState::OFFLINE;
  ScopedOfflineI2cAllowance allowOfflineI2c(_allowOfflineI2c, true);
  Status st = writeReg16(cmd::REG_CONFIG, cmd::CONFIG_RST);
  if (!st.ok()) {
    _markConfigReplayDirty(st);
    _markCalibrationDirty(st);
    _markThresholdsDirty();
    if (startedOffline) {
      _reassertOfflineLatch();
    }
    return st;
  }
  _markAccumulationInvalid();
  _trigPending = false;
  _trigStartMs = 0;
  const Status resetPending =
      Status::Error(Err::HARDWARE_DIRTY, "Software reset replay pending");
  _markConfigReplayDirty(resetPending);
  _markCalibrationDirty(resetPending);
  _markThresholdsDirty();

  st = _verifyResetComplete();
  if (!st.ok()) {
    if (startedOffline) {
      _reassertOfflineLatch();
    }
    return st;
  }

  st = _verifyIdentityAndMemstat(true);
  if (!st.ok()) {
    if (startedOffline) {
      _reassertOfflineLatch();
    }
    return st;
  }

  st = _resyncCachedHardware();
  if (st.ok() && isTriggeredMode(_config.mode)) {
    _clearHardwareDirty();
    _markTriggeredConversionStarted(_nowMs());
  } else if (st.ok()) {
    _clearHardwareDirty();
    _markAccumulationInvalid();
  }
  if (startedOffline && !st.ok() && !st.inProgress()) {
    _reassertOfflineLatch();
  }
  return st;
}

Status INA228::startResetJob() {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (_asyncJob != AsyncJob::NONE) {
    return Status::Error(Err::BUSY, "Another fixed-step job is active");
  }

  _asyncJob = AsyncJob::RESET;
  _resetStep = ResetStep::WRITE_RESET;
  _resetStartMs = 0;
  _resetVerifyAttempts = 0;
  _resetStartedOffline = _driverState == DriverState::OFFLINE;
  _resetDesiredDiagConfig = _diagAlertConfigBits;
  return Status{Err::IN_PROGRESS, 0, "Reset job started"};
}

Status INA228::pollResetJob(uint32_t nowMs, uint8_t maxInstructions) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (maxInstructions == 0) {
    return Status::Error(Err::INVALID_PARAM, "maxInstructions must be > 0");
  }
  if (_asyncJob != AsyncJob::RESET) {
    return Status::Error(Err::BUSY, "No reset job active");
  }

  ScopedOfflineI2cAllowance allowOfflineI2c(_allowOfflineI2c, true);
  auto failResetJob = [this](const Status& failure) {
    if (_resetStartedOffline && !failure.inProgress()) {
      _reassertOfflineLatch();
    }
    _clearAsyncJob();
    return failure;
  };

  uint8_t used = 0;
  while (used < maxInstructions) {
    Status st = Status::Ok();

    switch (_resetStep) {
      case ResetStep::WRITE_RESET:
        st = writeReg16(cmd::REG_CONFIG, cmd::CONFIG_RST);
        ++used;
        if (!st.ok()) {
          _markConfigReplayDirty(st);
          _markCalibrationDirty(st);
          _markThresholdsDirty();
          return failResetJob(st);
        }
        _markAccumulationInvalid();
        _trigPending = false;
        _trigStartMs = 0;
        {
          const Status resetPending =
              Status::Error(Err::HARDWARE_DIRTY, "Software reset replay pending");
          _markConfigReplayDirty(resetPending);
          _markCalibrationDirty(resetPending);
        }
        _markThresholdsDirty();
        _resetStartMs = nowMs;
        _resetVerifyAttempts = 0;
        _resetStep = ResetStep::WAIT_STARTUP;
        break;

      case ResetStep::WAIT_STARTUP:
        if ((nowMs - _resetStartMs) < RESET_STARTUP_MS) {
          return Status{Err::IN_PROGRESS, 0, "Reset startup wait in progress"};
        }
        _resetStep = ResetStep::VERIFY_CONFIG;
        break;

      case ResetStep::VERIFY_CONFIG: {
        uint16_t cfg = 0;
        st = readReg16(cmd::REG_CONFIG, cfg);
        ++used;
        if (!st.ok()) {
          return failResetJob(st);
        }
        if ((cfg & (cmd::CONFIG_RST | cmd::CONFIG_RSTACC)) == 0) {
          _resetStep = ResetStep::READ_MANUFACTURER;
        } else {
          ++_resetVerifyAttempts;
          if (_resetVerifyAttempts >= RESET_VERIFY_ATTEMPTS) {
            st = _recordFailure(
                Status::Error(Err::TIMEOUT, "CONFIG reset bits did not clear",
                              static_cast<int32_t>(cfg)));
            return failResetJob(st);
          }
        }
        break;
      }

      case ResetStep::READ_MANUFACTURER: {
        uint16_t mfgId = 0;
        st = readReg16(cmd::REG_MANUFACTURER_ID, mfgId);
        ++used;
        if (!st.ok()) {
          return failResetJob(st);
        }
        if (mfgId != cmd::MANUFACTURER_ID) {
          st = _recordFailure(
              Status::Error(Err::DEVICE_ID_MISMATCH,
                            "Manufacturer ID mismatch",
                            static_cast<int32_t>(mfgId)));
          return failResetJob(st);
        }
        _resetStep = ResetStep::READ_DEVICE;
        break;
      }

      case ResetStep::READ_DEVICE: {
        uint16_t devId = 0;
        st = readReg16(cmd::REG_DEVICE_ID, devId);
        ++used;
        if (!st.ok()) {
          return failResetJob(st);
        }
        if (devId != cmd::DEVICE_ID) {
          st = _recordFailure(
              Status::Error(Err::DEVICE_ID_MISMATCH, "Device ID mismatch",
                            static_cast<int32_t>(devId)));
          return failResetJob(st);
        }
        _resetStep = ResetStep::READ_DIAG;
        break;
      }

      case ResetStep::READ_DIAG: {
        uint16_t diagAlrt = 0;
        st = _readDiagAlertTracked(diagAlrt);
        _diagAlertConfigBits = _resetDesiredDiagConfig;
        ++used;
        if (!st.ok()) {
          return failResetJob(st);
        }
        if ((diagAlrt & cmd::DIAG_MEMSTAT) == 0) {
          st = _recordFailure(
              Status::Error(Err::MEMORY_ERROR,
                            "NV trim memory checksum error"));
          return failResetJob(st);
        }
        _resetStep = ResetStep::APPLY_ADC_SHUTDOWN;
        break;
      }

      case ResetStep::APPLY_ADC_SHUTDOWN:
      case ResetStep::APPLY_CONFIG:
      case ResetStep::APPLY_DIAG_ALRT:
      case ResetStep::APPLY_TEMPCO:
      case ResetStep::APPLY_SHUNT_CAL:
      case ResetStep::APPLY_ADC_CONFIG: {
        ApplyStep applyStep = ApplyStep::IDLE;
        switch (_resetStep) {
          case ResetStep::APPLY_ADC_SHUTDOWN:
            applyStep = ApplyStep::ADC_SHUTDOWN;
            break;
          case ResetStep::APPLY_CONFIG:
            applyStep = ApplyStep::CONFIG;
            break;
          case ResetStep::APPLY_DIAG_ALRT:
            applyStep = ApplyStep::DIAG_ALRT;
            break;
          case ResetStep::APPLY_TEMPCO:
            applyStep = ApplyStep::TEMPCO;
            break;
          case ResetStep::APPLY_SHUNT_CAL:
            applyStep = ApplyStep::SHUNT_CAL;
            break;
          case ResetStep::APPLY_ADC_CONFIG:
            applyStep = ApplyStep::ADC_CONFIG;
            break;
          default:
            break;
        }

        st = _writeApplyStep(applyStep);
        ++used;
        if (!st.ok()) {
          return failResetJob(st);
        }

        switch (_resetStep) {
          case ResetStep::APPLY_ADC_SHUTDOWN:
            _resetStep = ResetStep::APPLY_CONFIG;
            break;
          case ResetStep::APPLY_CONFIG:
            _resetStep = ResetStep::APPLY_DIAG_ALRT;
            break;
          case ResetStep::APPLY_DIAG_ALRT:
            _resetStep = ResetStep::APPLY_TEMPCO;
            break;
          case ResetStep::APPLY_TEMPCO:
            _resetStep = ResetStep::APPLY_SHUNT_CAL;
            break;
          case ResetStep::APPLY_SHUNT_CAL:
            _resetStep = ResetStep::APPLY_ADC_CONFIG;
            break;
          case ResetStep::APPLY_ADC_CONFIG:
            _clearHardwareDirty();
            if (isTriggeredMode(_config.mode)) {
              _markTriggeredConversionStarted(nowMs);
            } else {
              _markAccumulationInvalid();
            }
            _clearAsyncJob();
            return Status::Ok();
          default:
            break;
        }
        break;
      }

      case ResetStep::IDLE:
      case ResetStep::DONE:
      default:
        _clearAsyncJob();
        return Status::Error(Err::BUSY, "No reset job active");
    }
  }

  return Status{Err::IN_PROGRESS, 0, "Reset job in progress"};
}

Status INA228::resetAccumulators() {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) return clean;

  uint16_t cfg = _buildConfig();
  Status st = writeReg16(cmd::REG_CONFIG, cfg | cmd::CONFIG_RSTACC);
  if (!st.ok()) {
    if (st.code != Err::BUSY &&
        st.code != Err::INVALID_CONFIG &&
        st.code != Err::INVALID_PARAM &&
        st.code != Err::NOT_INITIALIZED) {
      _markAccumulationInvalid();
      _markHardwareDirty(cmd::REG_CONFIG, st);
    }
    return st;
  }

  _markAccumulationInvalid();
  st = writeReg16(cmd::REG_CONFIG, cfg);
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_CONFIG, st);
    return st;
  }

  uint16_t readback = 0;
  st = readReg16(cmd::REG_CONFIG, readback);
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_CONFIG, st);
    return st;
  }
  if ((readback & (cmd::CONFIG_RST | cmd::CONFIG_RSTACC)) != 0) {
    st = _recordFailure(
        Status::Error(Err::TIMEOUT, "CONFIG reset bits did not clear",
                      static_cast<int32_t>(readback)));
    _markHardwareDirty(cmd::REG_CONFIG, st);
  }
  return st;
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
  uint8_t payload[3] = {
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
  Status st = writeReg16(reg, value);
  if (!st.ok() &&
      (reg == cmd::REG_DIAG_ALRT || reg == cmd::REG_CONFIG ||
       reg == cmd::REG_SHUNT_CAL || reg == cmd::REG_ADC_CONFIG ||
       reg == cmd::REG_SHUNT_TEMPCO)) {
    _markHardwareDirty(reg, st);
  } else if (st.ok() && reg == cmd::REG_DIAG_ALRT) {
    _diagAlertConfigBits = value & cmd::DIAG_CONFIG_MASK;
  } else if (st.ok() && reg == cmd::REG_CONFIG &&
             ((value & cmd::CONFIG_RST) != 0)) {
    _markAccumulationInvalid();
    _markConfigReplayDirty(
        Status::Error(Err::HARDWARE_DIRTY, "Raw software reset write"));
    _markCalibrationDirty(
        Status::Error(Err::HARDWARE_DIRTY, "Raw software reset write"));
    _markThresholdsDirty();
  } else if (st.ok() && reg == cmd::REG_CONFIG &&
             ((value & cmd::CONFIG_RSTACC) != 0)) {
    _markAccumulationInvalid();
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

Status INA228::_readDiagAlertRaw(uint16_t& raw) {
  Status st = _readReg16Raw(cmd::REG_DIAG_ALRT, raw);
  if (st.ok()) {
    _captureDiagAlert(raw);
  }
  return st;
}

void INA228::_captureDiagAlert(uint16_t raw) {
  const uint16_t preservedEvidence =
      _diagAlertSnapshot.valid ? (_diagAlertSnapshot.raw & DIAG_EVIDENCE_MASK) : 0;
  const uint16_t combinedRaw =
      (raw & cmd::DIAG_CONFIG_MASK) | (raw & cmd::DIAG_MEMSTAT) |
      preservedEvidence | (raw & DIAG_EVIDENCE_MASK);
  _diagAlertSnapshot.valid = true;
  _diagAlertSnapshot.raw = combinedRaw;
  parseDiagAlert(combinedRaw, _diagAlertSnapshot.diag);
  _diagAlertSnapshot.capturedMs = _nowMs();
  _diagAlertConfigBits = raw & cmd::DIAG_CONFIG_MASK;
  if (((raw & cmd::DIAG_CNVRF) != 0) && _modeSupportsAnyAccumulation()) {
    _accumulationReady = true;
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

// ===========================================================================
// Health Management
// ===========================================================================

Status INA228::_updateHealth(const Status& st) {
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

  if (_consecutiveFailures >= _config.offlineThreshold) {
    _driverState = DriverState::OFFLINE;
  } else {
    _driverState = DriverState::DEGRADED;
  }

  return st;
}

Status INA228::_recordFailure(const Status& st) {
  if (st.ok() || st.inProgress() ||
      st.code == Err::INVALID_CONFIG ||
      st.code == Err::INVALID_PARAM ||
      st.code == Err::NOT_INITIALIZED) {
    return st;
  }

  const uint32_t now = _nowMs();
  const uint32_t maxU32 = std::numeric_limits<uint32_t>::max();
  const uint8_t maxU8 = std::numeric_limits<uint8_t>::max();

  _lastError = st;
  _lastErrorMs = now;
  if (_totalFailures < maxU32) {
    _totalFailures++;
  }
  if (_consecutiveFailures < maxU8) {
    _consecutiveFailures++;
  }

  if (_initialized) {
    if (_consecutiveFailures >= _config.offlineThreshold) {
      _driverState = DriverState::OFFLINE;
    } else {
      _driverState = DriverState::DEGRADED;
    }
  }

  return st;
}

void INA228::_reassertOfflineLatch() {
  _driverState = DriverState::OFFLINE;
  const uint8_t threshold = _config.offlineThreshold == 0 ? 1 : _config.offlineThreshold;
  if (_consecutiveFailures < threshold) {
    _consecutiveFailures = threshold;
  }
}

Status INA228::_ensureNormalI2cAllowed() const {
  if (_initialized && _driverState == DriverState::OFFLINE && !_allowOfflineI2c) {
    return Status::Error(Err::BUSY, "Driver is offline; call recover()");
  }
  return Status::Ok();
}

// ===========================================================================
// Internal Helpers
// ===========================================================================

Status INA228::_ensureMeasurementReadyForRead() {
  Status clean = _ensureHardwareClean();
  if (!clean.ok()) {
    return clean;
  }
  if (!_trigPending) {
    return Status::Ok();
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

Status INA228::_ensureHardwareClean() const {
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

Status INA228::_verifyResetComplete() {
  uint16_t cfg = 0;
  for (uint8_t attempt = 0; attempt < RESET_VERIFY_ATTEMPTS; ++attempt) {
    Status st = readReg16(cmd::REG_CONFIG, cfg);
    if (!st.ok()) {
      return st;
    }
    if ((cfg & (cmd::CONFIG_RST | cmd::CONFIG_RSTACC)) == 0) {
      return Status::Ok();
    }
  }
  return _recordFailure(
      Status::Error(Err::TIMEOUT, "CONFIG reset bits did not clear",
                    static_cast<int32_t>(cfg)));
}

Status INA228::_verifyIdentityAndMemstat(bool preserveAlertConfig) {
  const uint16_t desiredDiagConfig = _diagAlertConfigBits;

  uint16_t mfgId = 0;
  Status st = readReg16(cmd::REG_MANUFACTURER_ID, mfgId);
  if (!st.ok()) {
    return st;
  }
  if (mfgId != cmd::MANUFACTURER_ID) {
    return _recordFailure(
        Status::Error(Err::DEVICE_ID_MISMATCH, "Manufacturer ID mismatch",
                      static_cast<int32_t>(mfgId)));
  }

  uint16_t devId = 0;
  st = readReg16(cmd::REG_DEVICE_ID, devId);
  if (!st.ok()) {
    return st;
  }
  if (devId != cmd::DEVICE_ID) {
    return _recordFailure(
        Status::Error(Err::DEVICE_ID_MISMATCH, "Device ID mismatch",
                      static_cast<int32_t>(devId)));
  }

  uint16_t diagAlrt = 0;
  st = _readDiagAlertTracked(diagAlrt);
  if (preserveAlertConfig) {
    _diagAlertConfigBits = desiredDiagConfig;
  }
  if (!st.ok()) {
    return st;
  }
  if ((diagAlrt & cmd::DIAG_MEMSTAT) == 0) {
    return _recordFailure(
        Status::Error(Err::MEMORY_ERROR, "NV trim memory checksum error"));
  }
  return Status::Ok();
}

Status INA228::_resyncCachedHardware() {
  uint16_t shutdownAdc = _buildAdcConfig();
  shutdownAdc = static_cast<uint16_t>(
      (shutdownAdc & ~cmd::MASK_ADC_MODE) |
      (static_cast<uint16_t>(Mode::SHUTDOWN) << cmd::BIT_ADC_MODE));

  Status st = writeReg16(cmd::REG_ADC_CONFIG, shutdownAdc);
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
    return st;
  }

  st = _applyStaticConfig();
  if (!st.ok()) {
    _markConfigReplayDirty(st);
    return st;
  }

  st = _applyCalibration();
  if (!st.ok()) {
    _markCalibrationDirty(st);
    return st;
  }

  st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
  if (!st.ok()) {
    _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
  }
  return st;
}

Status INA228::_writeApplyStep(ApplyStep step) {
  switch (step) {
    case ApplyStep::ADC_SHUTDOWN: {
      uint16_t shutdownAdc = _buildAdcConfig();
      shutdownAdc = static_cast<uint16_t>(
          (shutdownAdc & ~cmd::MASK_ADC_MODE) |
          (static_cast<uint16_t>(Mode::SHUTDOWN) << cmd::BIT_ADC_MODE));
      Status st = writeReg16(cmd::REG_ADC_CONFIG, shutdownAdc);
      if (!st.ok()) {
        _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
      }
      return st;
    }

    case ApplyStep::CONFIG: {
      Status st = writeReg16(cmd::REG_CONFIG, _buildConfig());
      if (!st.ok()) {
        _markHardwareDirty(cmd::REG_CONFIG, st);
      }
      return st;
    }

    case ApplyStep::DIAG_ALRT:
      return _writeDiagAlertConfig(_diagAlertConfigBits);

    case ApplyStep::TEMPCO: {
      const uint16_t tempco =
          _config.shuntTempCoeffPpmC & cmd::MASK_SHUNT_TEMPCO;
      Status st = writeReg16(cmd::REG_SHUNT_TEMPCO, tempco);
      if (!st.ok()) {
        _markHardwareDirty(cmd::REG_SHUNT_TEMPCO, st);
      }
      return st;
    }

    case ApplyStep::SHUNT_CAL: {
      Status st = _applyCalibration();
      if (!st.ok()) {
        _markCalibrationDirty(st);
      }
      return st;
    }

    case ApplyStep::ADC_CONFIG: {
      Status st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
      if (!st.ok()) {
        _markHardwareDirty(cmd::REG_ADC_CONFIG, st);
      }
      return st;
    }

    case ApplyStep::IDLE:
    case ApplyStep::DONE:
    default:
      return Status::Error(Err::BUSY, "No apply step active");
  }
}

Status INA228::_fillPowerSampleUnits(const RawSample& raw,
                                     IntegerSample& out) const {
  return convertRawSample(raw, out);
}

void INA228::_clearAsyncJob() {
  _asyncJob = AsyncJob::NONE;
  _powerSampleStep = PowerSampleStep::IDLE;
  _powerSampleRawScratch = RawSample{};
  _powerSampleIntegerScratch = IntegerSample{};
  _applyStep = ApplyStep::IDLE;
  _resetStep = ResetStep::IDLE;
  _resetStartMs = 0;
  _resetVerifyAttempts = 0;
  _resetStartedOffline = false;
  _resetDesiredDiagConfig = 0;
}

bool INA228::_triggerDeadlineElapsed(uint32_t nowMs) const {
  return (nowMs - _trigStartMs) >= estimateConversionTimeMs();
}

bool INA228::_modeSupportsEnergyAccumulation() const {
  return isContinuousMode(_config.mode) && modeHasShunt(_config.mode) &&
         modeHasBus(_config.mode);
}

bool INA228::_modeSupportsChargeAccumulation() const {
  return isContinuousMode(_config.mode) && modeHasShunt(_config.mode);
}

bool INA228::_modeSupportsAnyAccumulation() const {
  return _modeSupportsEnergyAccumulation() || _modeSupportsChargeAccumulation();
}

void INA228::_markAccumulationInvalid() {
  _accumulationReady = false;
}

Status INA228::_ensureEnergyAccumulatorReadable() const {
  if (!_modeSupportsEnergyAccumulation()) {
    return Status::Error(Err::ACCUMULATION_INVALID,
                         "Energy requires continuous shunt and bus conversion",
                         static_cast<int32_t>(_config.mode));
  }
  if (!_accumulationReady) {
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
  if (!_accumulationReady) {
    return Status::Error(Err::ACCUMULATION_INVALID,
                         "Charge accumulation not ready");
  }
  return Status::Ok();
}

Status INA228::_readAccumulatorDiag(uint16_t& raw) {
  raw = 0;
  Status st = _readDiagAlertTracked(raw);
  if (!st.ok()) {
    return st;
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

void INA228::_markTriggeredConversionStarted(uint32_t nowMs) {
  _markAccumulationInvalid();
  _trigPending = true;
  _trigStartMs = nowMs;
  _clearCapturedConversionReadyFlag();
}

void INA228::_completeTriggeredConversion() {
  const bool wasTriggeredMode = isTriggeredMode(_config.mode);
  _trigPending = false;
  _trigStartMs = 0;
  _markAccumulationInvalid();
  if (wasTriggeredMode) {
    _config.mode = Mode::SHUTDOWN;
  }
}

void INA228::_clearCapturedConversionReadyFlag() {
  if (!_diagAlertSnapshot.valid || ((_diagAlertSnapshot.raw & cmd::DIAG_CNVRF) == 0)) {
    return;
  }
  _diagAlertSnapshot.raw &= ~cmd::DIAG_CNVRF;
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
  if (reg < 64) {
    _dirtyRegisterMask |= (uint64_t{1} << reg);
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

bool INA228::_isThresholdRegister(uint8_t reg) const {
  return reg == cmd::REG_SOVL ||
         reg == cmd::REG_SUVL ||
         reg == cmd::REG_BOVL ||
         reg == cmd::REG_BUVL ||
         reg == cmd::REG_TEMP_LIMIT ||
         reg == cmd::REG_PWR_LIMIT;
}

Status INA228::_applyConfig() {
  Status st = _applyStaticConfig();
  if (!st.ok()) {
    return st;
  }

  return writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
}

Status INA228::_applyStaticConfig() {
  // Write CONFIG register
  Status st = writeReg16(cmd::REG_CONFIG, _buildConfig());
  if (!st.ok()) return st;

  st = _writeDiagAlertConfig(_diagAlertConfigBits);
  if (!st.ok()) return st;

  // Program the coefficient every time; TEMPCOMP only gates its use.
  const uint16_t tempco = _config.shuntTempCoeffPpmC & cmd::MASK_SHUNT_TEMPCO;
  st = writeReg16(cmd::REG_SHUNT_TEMPCO, tempco);
  if (!st.ok()) return st;

  return Status::Ok();
}

Status INA228::_applyCalibration() {
  if (_config.shuntResistanceOhm <= 0.0f || _config.maxExpectedCurrentA <= 0.0f) {
    Status st = writeReg16(cmd::REG_SHUNT_CAL, 0);
    if (!st.ok()) {
      return st;
    }
    _currentLsb = 0.0f;
    _shuntCal = 0;
    _calibrationClamped = false;
    _maxCurrentExceedsShuntRange = false;
    return Status::Ok();
  }

  uint16_t newShuntCal = 0;
  float newCurrentLsb = 0.0f;
  bool newClamped = false;
  bool newMaxCurrentExceedsRange = false;
  Status st = computeCalibration(_config.shuntResistanceOhm,
                                 _config.maxExpectedCurrentA,
                                 _config.adcRange,
                                 newShuntCal,
                                 newCurrentLsb,
                                 newClamped,
                                 newMaxCurrentExceedsRange);
  if (!st.ok()) {
    return st;
  }

  st = writeReg16(cmd::REG_SHUNT_CAL, newShuntCal);
  if (!st.ok()) {
    return st;
  }

  _shuntCal = newShuntCal;
  _currentLsb = newCurrentLsb;
  _calibrationClamped = newClamped;
  _maxCurrentExceedsShuntRange = newMaxCurrentExceedsRange;
  return Status::Ok();
}

uint32_t INA228::_nowMs() const {
  if (_config.nowMs != nullptr) {
    return _config.nowMs(_config.timeUser);
  }
  return 0;
}

uint16_t INA228::_buildAdcConfig() const {
  return static_cast<uint16_t>(
    (static_cast<uint16_t>(_config.mode) << cmd::BIT_ADC_MODE) |
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
