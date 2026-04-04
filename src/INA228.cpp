/**
 * @file INA228.cpp
 * @brief INA228 driver implementation.
 */

#include "INA228/INA228.h"

#include <Arduino.h>
#include <cstring>
#include <limits>
#include <cmath>

namespace INA228 {
namespace {

static constexpr size_t MAX_WRITE_LEN = 6;

static bool isValidAddress(uint8_t addr) {
  return addr >= 0x40 && addr <= 0x4F;
}

static bool isValidConvTime(ConvTime ct) {
  return static_cast<uint8_t>(ct) <= 0x07;
}

static bool isValidAveraging(Averaging avg) {
  return static_cast<uint8_t>(avg) <= 0x07;
}

static bool isValidMode(Mode mode) {
  return static_cast<uint8_t>(mode) <= 0x0F;
}

static bool isTriggeredMode(Mode mode) {
  const uint8_t m = static_cast<uint8_t>(mode);
  return m >= 1 && m <= 7;
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

}  // namespace

// ===========================================================================
// Lifecycle
// ===========================================================================

Status INA228::begin(const Config& config) {
  _initialized = false;
  _driverState = DriverState::UNINIT;

  _lastOkMs = 0;
  _lastErrorMs = 0;
  _lastError = Status::Ok();
  _consecutiveFailures = 0;
  _totalFailures = 0;
  _totalSuccess = 0;

  _currentLsb = 0.0f;
  _shuntCal = 0;
  _trigPending = false;
  _trigStartMs = 0;

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
  if (config.shuntTempCoeffPpmC > cmd::TEMPCO_MAX) {
    return Status::Error(Err::INVALID_CONFIG, "Shunt temp coeff exceeds 16383");
  }

  _config = config;
  if (_config.offlineThreshold == 0) {
    _config.offlineThreshold = 1;
  }

  // Verify device identity using raw reads (before health tracking is active)
  uint16_t mfgId = 0;
  Status st = _readReg16Raw(cmd::REG_MANUFACTURER_ID, mfgId);
  if (!st.ok()) {
    return Status::Error(Err::DEVICE_NOT_FOUND, "Device not responding", st.detail);
  }
  if (mfgId != cmd::MANUFACTURER_ID) {
    return Status::Error(Err::DEVICE_ID_MISMATCH, "Manufacturer ID mismatch",
                        static_cast<int32_t>(mfgId));
  }

  uint16_t devId = 0;
  st = _readReg16Raw(cmd::REG_DEVICE_ID, devId);
  if (!st.ok()) {
    return Status::Error(Err::DEVICE_NOT_FOUND, "Device ID read failed", st.detail);
  }
  if (devId != cmd::DEVICE_ID) {
    return Status::Error(Err::DEVICE_ID_MISMATCH, "Device ID mismatch",
                        static_cast<int32_t>(devId));
  }

  // Check MEMSTAT bit
  uint16_t diagAlrt = 0;
  st = _readReg16Raw(cmd::REG_DIAG_ALRT, diagAlrt);
  if (!st.ok()) {
    return Status::Error(Err::DEVICE_NOT_FOUND, "DIAG_ALRT read failed", st.detail);
  }
  if ((diagAlrt & cmd::DIAG_MEMSTAT) == 0) {
    return Status::Error(Err::MEMORY_ERROR, "NV trim memory checksum error");
  }

  // Apply configuration
  st = _applyConfig();
  if (!st.ok()) {
    return st;
  }

  // Apply calibration if provided
  st = _applyCalibration();
  if (!st.ok()) {
    return st;
  }

  _initialized = true;
  _driverState = DriverState::READY;

  return Status::Ok();
}

void INA228::tick(uint32_t nowMs) {
  (void)nowMs;
  // INA228 is fully synchronous - tick() is a no-op.
  // Conversion readiness is checked explicitly via isConversionReady().
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
  _currentLsb = 0.0f;
  _shuntCal = 0;
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
    return Status::Error(Err::DEVICE_NOT_FOUND, "Device not responding", st.detail);
  }
  if (mfgId != cmd::MANUFACTURER_ID) {
    return Status::Error(Err::DEVICE_ID_MISMATCH, "Manufacturer ID mismatch",
                        static_cast<int32_t>(mfgId));
  }

  uint16_t devId = 0;
  st = _readReg16Raw(cmd::REG_DEVICE_ID, devId);
  if (!st.ok()) {
    return Status::Error(Err::DEVICE_NOT_FOUND, "Device ID read failed", st.detail);
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

  // Use tracked reads to update health on success/failure
  uint16_t mfgId = 0;
  Status st = readReg16(cmd::REG_MANUFACTURER_ID, mfgId);
  if (!st.ok()) {
    return st;
  }
  if (mfgId != cmd::MANUFACTURER_ID) {
    return Status::Error(Err::DEVICE_ID_MISMATCH, "Manufacturer ID mismatch",
                        static_cast<int32_t>(mfgId));
  }

  // Re-apply configuration (device may have power-cycled)
  st = _applyConfig();
  if (!st.ok()) {
    return st;
  }

  st = _applyCalibration();
  if (!st.ok()) {
    return st;
  }

  return Status::Ok();
}

// ===========================================================================
// Measurement API
// ===========================================================================

Status INA228::readMeasurement(Measurement& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  // Read shunt voltage (24-bit)
  uint32_t raw24 = 0;
  Status st = readReg24(cmd::REG_VSHUNT, raw24);
  if (!st.ok()) return st;
  int32_t vshRaw = _signExtend20(raw24);
  const double vshLsb = (_config.adcRange == AdcRange::MV_40_96)
                          ? cmd::VSHUNT_LSB_RANGE1
                          : cmd::VSHUNT_LSB_RANGE0;
  out.shuntVoltageV = static_cast<float>(vshRaw * vshLsb);

  // Read bus voltage (24-bit)
  st = readReg24(cmd::REG_VBUS, raw24);
  if (!st.ok()) return st;
  int32_t vbusRaw = _signExtend20(raw24);
  out.busVoltageV = static_cast<float>(vbusRaw * cmd::VBUS_LSB);

  // Read temperature (16-bit)
  uint16_t raw16 = 0;
  st = readReg16(cmd::REG_DIETEMP, raw16);
  if (!st.ok()) return st;
  out.temperatureC = static_cast<float>(static_cast<int16_t>(raw16) * cmd::TEMP_LSB);

  // Read current (24-bit, requires calibration)
  st = readReg24(cmd::REG_CURRENT, raw24);
  if (!st.ok()) return st;
  int32_t curRaw = _signExtend20(raw24);
  out.currentA = _currentLsb * static_cast<float>(curRaw);

  // Read power (24-bit, unsigned)
  st = readReg24(cmd::REG_POWER, raw24);
  if (!st.ok()) return st;
  out.powerW = static_cast<float>(cmd::POWER_COEFF * _currentLsb * raw24);

  // Read energy (40-bit, unsigned)
  uint64_t raw40 = 0;
  st = readReg40(cmd::REG_ENERGY, raw40);
  if (!st.ok()) return st;
  out.energyJ = cmd::ENERGY_COEFF * cmd::POWER_COEFF *
                static_cast<double>(_currentLsb) * static_cast<double>(raw40);

  // Read charge (40-bit, signed)
  st = readReg40(cmd::REG_CHARGE, raw40);
  if (!st.ok()) return st;
  int64_t chargeSigned = _signExtend40(raw40);
  out.chargeC = static_cast<double>(_currentLsb) * static_cast<double>(chargeSigned);

  return Status::Ok();
}

Status INA228::readRawSample(RawSample& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint32_t raw24 = 0;
  Status st = readReg24(cmd::REG_VSHUNT, raw24);
  if (!st.ok()) return st;
  out.vshunt = _signExtend20(raw24);

  st = readReg24(cmd::REG_VBUS, raw24);
  if (!st.ok()) return st;
  out.vbus = _signExtend20(raw24);

  uint16_t raw16 = 0;
  st = readReg16(cmd::REG_DIETEMP, raw16);
  if (!st.ok()) return st;
  out.dietemp = static_cast<int16_t>(raw16);

  st = readReg24(cmd::REG_CURRENT, raw24);
  if (!st.ok()) return st;
  out.current = _signExtend20(raw24);

  st = readReg24(cmd::REG_POWER, raw24);
  if (!st.ok()) return st;
  out.power = raw24;

  uint64_t raw40 = 0;
  st = readReg40(cmd::REG_ENERGY, raw40);
  if (!st.ok()) return st;
  out.energy = static_cast<int64_t>(raw40);

  st = readReg40(cmd::REG_CHARGE, raw40);
  if (!st.ok()) return st;
  out.charge = _signExtend40(raw40);

  return Status::Ok();
}

Status INA228::readShuntVoltage(float& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
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

  uint32_t raw24 = 0;
  Status st = readReg24(cmd::REG_VBUS, raw24);
  if (!st.ok()) return st;

  int32_t raw = _signExtend20(raw24);
  out = static_cast<float>(raw * cmd::VBUS_LSB);
  return Status::Ok();
}

Status INA228::readTemperature(float& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
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

  uint32_t raw24 = 0;
  Status st = readReg24(cmd::REG_CURRENT, raw24);
  if (!st.ok()) return st;

  int32_t raw = _signExtend20(raw24);
  out = _currentLsb * static_cast<float>(raw);
  return Status::Ok();
}

Status INA228::readPower(float& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint32_t raw24 = 0;
  Status st = readReg24(cmd::REG_POWER, raw24);
  if (!st.ok()) return st;

  out = static_cast<float>(cmd::POWER_COEFF * _currentLsb * raw24);
  return Status::Ok();
}

Status INA228::readEnergy(double& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint64_t raw40 = 0;
  Status st = readReg40(cmd::REG_ENERGY, raw40);
  if (!st.ok()) return st;

  out = cmd::ENERGY_COEFF * cmd::POWER_COEFF *
        static_cast<double>(_currentLsb) * static_cast<double>(raw40);
  return Status::Ok();
}

Status INA228::readCharge(double& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint64_t raw40 = 0;
  Status st = readReg40(cmd::REG_CHARGE, raw40);
  if (!st.ok()) return st;

  int64_t signed40 = _signExtend40(raw40);
  out = static_cast<double>(_currentLsb) * static_cast<double>(signed40);
  return Status::Ok();
}

Status INA228::isConversionReady(bool& ready) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint16_t diag = 0;
  Status st = readReg16(cmd::REG_DIAG_ALRT, diag);
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
  if (!isValidMode(mode)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid mode");
  }

  uint16_t adcCfg = _buildAdcConfig();
  adcCfg = (adcCfg & ~cmd::MASK_ADC_MODE) |
           (static_cast<uint16_t>(mode) << cmd::BIT_ADC_MODE);
  Status st = writeReg16(cmd::REG_ADC_CONFIG, adcCfg);
  if (!st.ok()) return st;

  _config.mode = mode;
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
  if (!isTriggeredMode(mode)) {
    return Status::Error(Err::INVALID_PARAM, "Not a triggered mode (0x1-0x7)");
  }

  uint16_t adcCfg = _buildAdcConfig();
  adcCfg = (adcCfg & ~cmd::MASK_ADC_MODE) |
           (static_cast<uint16_t>(mode) << cmd::BIT_ADC_MODE);
  Status st = writeReg16(cmd::REG_ADC_CONFIG, adcCfg);
  if (!st.ok()) return st;

  _config.mode = mode;
  _trigPending = true;
  _trigStartMs = _nowMs();
  return Status::Ok();
}

Status INA228::setVbusConvTime(ConvTime ct) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (!isValidConvTime(ct)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid conversion time");
  }

  _config.vbusConvTime = ct;
  return writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
}

Status INA228::setVshuntConvTime(ConvTime ct) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (!isValidConvTime(ct)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid conversion time");
  }

  _config.vshuntConvTime = ct;
  return writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
}

Status INA228::setTempConvTime(ConvTime ct) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (!isValidConvTime(ct)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid conversion time");
  }

  _config.vtempConvTime = ct;
  return writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
}

Status INA228::setAveraging(Averaging avg) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (!isValidAveraging(avg)) {
    return Status::Error(Err::INVALID_PARAM, "Invalid averaging count");
  }

  _config.averaging = avg;
  return writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
}

Status INA228::setAdcRange(AdcRange range) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  _config.adcRange = range;
  Status st = writeReg16(cmd::REG_CONFIG, _buildConfig());
  if (!st.ok()) return st;

  // Range change requires recalibration
  return _applyCalibration();
}

Status INA228::setCalibration(float shuntOhm, float maxCurrentA) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (shuntOhm <= 0.0f || maxCurrentA <= 0.0f) {
    return Status::Error(Err::INVALID_PARAM, "Shunt/current must be > 0");
  }

  _config.shuntResistanceOhm = shuntOhm;
  _config.maxExpectedCurrentA = maxCurrentA;
  return _applyCalibration();
}

Status INA228::setShuntTempCoeff(uint16_t ppmPerC) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (ppmPerC > cmd::TEMPCO_MAX) {
    return Status::Error(Err::INVALID_PARAM, "Temp coeff exceeds 16383 ppm/C");
  }

  _config.shuntTempCoeffPpmC = ppmPerC;
  uint16_t val = ppmPerC & cmd::MASK_SHUNT_TEMPCO;
  return writeReg16(cmd::REG_SHUNT_TEMPCO, val);
}

Status INA228::setTempCompensation(bool enable) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  _config.tempCompEnabled = enable;
  return writeReg16(cmd::REG_CONFIG, _buildConfig());
}

Status INA228::setConversionDelay(uint8_t steps2ms) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  _config.convDelayMs2 = steps2ms;
  return writeReg16(cmd::REG_CONFIG, _buildConfig());
}

// ===========================================================================
// Alert Configuration
// ===========================================================================

Status INA228::readDiagAlert(DiagAlert& out) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint16_t raw = 0;
  Status st = readReg16(cmd::REG_DIAG_ALRT, raw);
  if (!st.ok()) return st;

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

  return Status::Ok();
}

Status INA228::readDiagAlertRaw(uint16_t& raw) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  return readReg16(cmd::REG_DIAG_ALRT, raw);
}

Status INA228::setAlertLatch(bool latch) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint16_t diag = 0;
  Status st = readReg16(cmd::REG_DIAG_ALRT, diag);
  if (!st.ok()) return st;

  if (latch) {
    diag |= cmd::DIAG_ALATCH;
  } else {
    diag &= ~cmd::DIAG_ALATCH;
  }
  return writeReg16(cmd::REG_DIAG_ALRT, diag);
}

Status INA228::setConversionReadyAlert(bool enable) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint16_t diag = 0;
  Status st = readReg16(cmd::REG_DIAG_ALRT, diag);
  if (!st.ok()) return st;

  if (enable) {
    diag |= cmd::DIAG_CNVR;
  } else {
    diag &= ~cmd::DIAG_CNVR;
  }
  return writeReg16(cmd::REG_DIAG_ALRT, diag);
}

Status INA228::setSlowAlert(bool enable) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint16_t diag = 0;
  Status st = readReg16(cmd::REG_DIAG_ALRT, diag);
  if (!st.ok()) return st;

  if (enable) {
    diag |= cmd::DIAG_SLOWALERT;
  } else {
    diag &= ~cmd::DIAG_SLOWALERT;
  }
  return writeReg16(cmd::REG_DIAG_ALRT, diag);
}

Status INA228::setAlertPolarity(bool activeHigh) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint16_t diag = 0;
  Status st = readReg16(cmd::REG_DIAG_ALRT, diag);
  if (!st.ok()) return st;

  if (activeHigh) {
    diag |= cmd::DIAG_APOL;
  } else {
    diag &= ~cmd::DIAG_APOL;
  }
  return writeReg16(cmd::REG_DIAG_ALRT, diag);
}

Status INA228::setShuntOvervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  const double lsb = (_config.adcRange == AdcRange::MV_40_96)
                       ? cmd::SHUNT_THRESHOLD_LSB_RANGE1
                       : cmd::SHUNT_THRESHOLD_LSB_RANGE0;
  auto regVal = static_cast<int16_t>(std::round(voltageV / lsb));
  return writeReg16(cmd::REG_SOVL, static_cast<uint16_t>(regVal));
}

Status INA228::setShuntUndervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  const double lsb = (_config.adcRange == AdcRange::MV_40_96)
                       ? cmd::SHUNT_THRESHOLD_LSB_RANGE1
                       : cmd::SHUNT_THRESHOLD_LSB_RANGE0;
  auto regVal = static_cast<int16_t>(std::round(voltageV / lsb));
  return writeReg16(cmd::REG_SUVL, static_cast<uint16_t>(regVal));
}

Status INA228::setBusOvervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  auto regVal = static_cast<uint16_t>(std::round(voltageV / cmd::BUS_THRESHOLD_LSB));
  regVal &= 0x7FFF;  // bit 15 reserved
  return writeReg16(cmd::REG_BOVL, regVal);
}

Status INA228::setBusUndervoltageThreshold(float voltageV) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  auto regVal = static_cast<uint16_t>(std::round(voltageV / cmd::BUS_THRESHOLD_LSB));
  regVal &= 0x7FFF;  // bit 15 reserved
  return writeReg16(cmd::REG_BUVL, regVal);
}

Status INA228::setTemperatureOverlimitThreshold(float tempC) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  auto regVal = static_cast<int16_t>(std::round(tempC / cmd::TEMP_LSB));
  return writeReg16(cmd::REG_TEMP_LIMIT, static_cast<uint16_t>(regVal));
}

Status INA228::setPowerOverlimitThreshold(float powerW) {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }
  if (_currentLsb == 0.0f) {
    return Status::Error(Err::INVALID_PARAM, "Calibration required for power threshold");
  }

  const double powerLsb = cmd::POWER_COEFF * _currentLsb;
  const double limitLsb = 256.0 * powerLsb;
  auto regVal = static_cast<uint16_t>(std::round(powerW / limitLsb));
  return writeReg16(cmd::REG_PWR_LIMIT, regVal);
}

// ===========================================================================
// Device Control
// ===========================================================================

Status INA228::softReset() {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  Status st = writeReg16(cmd::REG_CONFIG, cmd::CONFIG_RST);
  if (!st.ok()) return st;

  // After reset, re-apply all configuration
  st = _applyConfig();
  if (!st.ok()) return st;

  return _applyCalibration();
}

Status INA228::resetAccumulators() {
  if (!_initialized) {
    return Status::Error(Err::NOT_INITIALIZED, "begin() not called");
  }

  uint16_t cfg = _buildConfig();
  cfg |= cmd::CONFIG_RSTACC;
  return writeReg16(cmd::REG_CONFIG, cfg);
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

Status INA228::_readReg16Raw(uint8_t reg, uint16_t& value) {
  uint8_t buf[2] = {};
  Status st = _i2cWriteReadRaw(&reg, 1, buf, 2);
  if (!st.ok()) return st;

  value = (static_cast<uint16_t>(buf[0]) << 8) | buf[1];
  return Status::Ok();
}

// ===========================================================================
// Health Management
// ===========================================================================

Status INA228::_updateHealth(const Status& st) {
  if (!_initialized) {
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

// ===========================================================================
// Internal Helpers
// ===========================================================================

Status INA228::_applyConfig() {
  // Write CONFIG register
  Status st = writeReg16(cmd::REG_CONFIG, _buildConfig());
  if (!st.ok()) return st;

  // Write ADC_CONFIG register
  st = writeReg16(cmd::REG_ADC_CONFIG, _buildAdcConfig());
  if (!st.ok()) return st;

  // Write temperature coefficient if enabled
  if (_config.tempCompEnabled && _config.shuntTempCoeffPpmC > 0) {
    uint16_t tempco = _config.shuntTempCoeffPpmC & cmd::MASK_SHUNT_TEMPCO;
    st = writeReg16(cmd::REG_SHUNT_TEMPCO, tempco);
    if (!st.ok()) return st;
  }

  return Status::Ok();
}

Status INA228::_applyCalibration() {
  if (_config.shuntResistanceOhm <= 0.0f || _config.maxExpectedCurrentA <= 0.0f) {
    // No calibration: leave SHUNT_CAL at default (or zero current LSB)
    _currentLsb = 0.0f;
    _shuntCal = 0;
    return Status::Ok();
  }

  // CURRENT_LSB = Max_Expected_Current / 2^19
  _currentLsb = _config.maxExpectedCurrentA / 524288.0f;  // 2^19

  // SHUNT_CAL = 13107.2 × 10^6 × CURRENT_LSB × RSHUNT
  double calValue = cmd::SHUNT_CAL_FACTOR *
                    static_cast<double>(_currentLsb) *
                    static_cast<double>(_config.shuntResistanceOhm);

  // For ADCRANGE = 1, multiply by 4
  if (_config.adcRange == AdcRange::MV_40_96) {
    calValue *= 4.0;
  }

  // Clamp to 15-bit range
  if (calValue > static_cast<double>(cmd::MASK_SHUNT_CAL)) {
    calValue = static_cast<double>(cmd::MASK_SHUNT_CAL);
  }
  if (calValue < 0.0) {
    calValue = 0.0;
  }

  _shuntCal = static_cast<uint16_t>(std::round(calValue)) & cmd::MASK_SHUNT_CAL;
  return writeReg16(cmd::REG_SHUNT_CAL, _shuntCal);
}

uint32_t INA228::_nowMs() const {
  if (_config.nowMs != nullptr) {
    return _config.nowMs(_config.timeUser);
  }
  return millis();
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
  // 20-bit value is in bits 23:4 of the 24-bit register
  int32_t val = static_cast<int32_t>(raw24 >> 4);
  // Sign extend from bit 19
  if (val & 0x80000) {
    val |= static_cast<int32_t>(0xFFF00000);
  }
  return val;
}

int64_t INA228::_signExtend40(uint64_t raw40) {
  // 40-bit two's complement
  if (raw40 & 0x8000000000ULL) {
    return static_cast<int64_t>(raw40 | 0xFFFFFF0000000000ULL);
  }
  return static_cast<int64_t>(raw40);
}

}  // namespace INA228
