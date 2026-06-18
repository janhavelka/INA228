/// @file main.cpp
/// @brief Basic bringup example for INA228
/// @note This is an EXAMPLE, not part of the library

#include <cstdlib>
#include <limits>

#include <Arduino.h>

#include "examples/common/CliStyle.h"
#include "examples/common/Log.h"
#include "examples/common/BoardConfig.h"
#include "examples/common/BusDiag.h"
#include "examples/common/I2cTransport.h"
#include "examples/common/I2cScanner.h"

#include "INA228/INA228.h"

// ============================================================================
// Globals
// ============================================================================

struct StressStats {
  bool active = false;
  uint32_t startMs = 0;
  uint32_t endMs = 0;
  int target = 0;
  int attempts = 0;
  int success = 0;
  uint32_t errors = 0;
  bool hasSample = false;
  float minVbus = 0.0f;
  float maxVbus = 0.0f;
  float minCurrent = 0.0f;
  float maxCurrent = 0.0f;
  float minPower = 0.0f;
  float maxPower = 0.0f;
  double sumVbus = 0.0;
  double sumCurrent = 0.0;
  double sumPower = 0.0;
  INA228::Status lastError = INA228::Status::Ok();
};

INA228::INA228 device;
bool verboseMode = false;
StressStats stressStats;
static constexpr uint8_t DEFAULT_I2C_ADDRESS = board::INA228_I2C_ADDR;
static constexpr uint8_t INA228_ADDR_MIN = 0x40;
static constexpr uint8_t INA228_ADDR_MAX = 0x4F;
static constexpr size_t CLI_MAX_LINE_LEN = 128;
static constexpr uint8_t CLI_MAX_BYTES_PER_LOOP = 32;
static constexpr int MAX_STRESS_COUNT = 100000;
uint8_t selectedAddress = DEFAULT_I2C_ADDRESS;

struct ProbeSnapshot {
  uint8_t address = DEFAULT_I2C_ADDRESS;
  uint16_t manufacturerId = 0;
  uint16_t deviceId = 0;
  uint16_t diagAlert = 0;
};

const char* errToStr(INA228::Err err);

// ============================================================================
// Helper Functions
// ============================================================================

uint32_t exampleNowMs(void*) {
  return millis();
}

bool isValidIna228Address(uint8_t address) {
  return address >= INA228_ADDR_MIN && address <= INA228_ADDR_MAX;
}

uint8_t configuredAddress() {
  if (device.isInitialized()) {
    return device.getConfig().i2cAddress;
  }
  return selectedAddress;
}

INA228::Status checkAddressAck(uint8_t address) {
  if (!isValidIna228Address(address)) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM,
                                 "Address must be 0x40-0x4F",
                                 static_cast<int32_t>(address));
  }

  return transport::probeAddress(address, board::I2C_TIMEOUT_MS);
}

INA228::Config makeExampleConfig(uint8_t address) {
  (void)transport::selectAddress(address);
  INA228::Config cfg;
  cfg.i2cWrite = transport::wireWrite;
  cfg.i2cWriteRead = transport::wireWriteRead;
  cfg.i2cUser = transport::configUser();
  cfg.nowMs = transport::arduinoNowMs;
  cfg.i2cAddress = address;
  cfg.i2cTimeoutMs = board::I2C_TIMEOUT_MS;
  cfg.mode = INA228::Mode::CONT_ALL;
  cfg.shuntResistanceOhm = 0.015f;
  cfg.maxExpectedCurrentA = 10.0f;
  cfg.offlineThreshold = 5;
  return cfg;
}

INA228::Status readRegister16AtAddress(uint8_t address, uint8_t reg, uint16_t& value) {
  if (!isValidIna228Address(address)) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM,
                                 "Address must be 0x40-0x4F",
                                 static_cast<int32_t>(address));
  }

  uint8_t tx = reg;
  uint8_t rx[2] = {};
  INA228::Status st = transport::wireWriteReadAt(address, &tx, 1, rx, sizeof(rx),
                                                 board::I2C_TIMEOUT_MS);
  if (!st.ok()) {
    return st;
  }

  value = static_cast<uint16_t>((static_cast<uint16_t>(rx[0]) << 8) | rx[1]);
  return INA228::Status::Ok();
}

INA228::Status probeAddressRaw(uint8_t address, ProbeSnapshot& out) {
  out = {};
  out.address = address;

  INA228::Status st = checkAddressAck(address);
  if (!st.ok()) {
    return st;
  }

  st = readRegister16AtAddress(address, INA228::cmd::REG_MANUFACTURER_ID,
                               out.manufacturerId);
  if (!st.ok()) {
    return st;
  }
  if (out.manufacturerId != INA228::cmd::MANUFACTURER_ID) {
    return INA228::Status::Error(INA228::Err::DEVICE_ID_MISMATCH,
                                 "Manufacturer ID mismatch",
                                 static_cast<int32_t>(out.manufacturerId));
  }

  st = readRegister16AtAddress(address, INA228::cmd::REG_DEVICE_ID, out.deviceId);
  if (!st.ok()) {
    return st;
  }
  if (out.deviceId != INA228::cmd::DEVICE_ID) {
    return INA228::Status::Error(INA228::Err::DEVICE_ID_MISMATCH,
                                 "Device ID mismatch",
                                 static_cast<int32_t>(out.deviceId));
  }

  st = readRegister16AtAddress(address, INA228::cmd::REG_DIAG_ALRT, out.diagAlert);
  if (!st.ok()) {
    return st;
  }
  if ((out.diagAlert & INA228::cmd::DIAG_MEMSTAT) == 0U) {
    return INA228::Status::Error(INA228::Err::MEMORY_ERROR,
                                 "NV trim memory checksum error");
  }

  return INA228::Status::Ok();
}

void printProbeSnapshot(const ProbeSnapshot& snapshot) {
  Serial.printf("  Address: 0x%02X\n", snapshot.address);
  Serial.printf("  Manufacturer ID: 0x%04X\n", snapshot.manufacturerId);
  Serial.printf("  Device ID: 0x%04X\n", snapshot.deviceId);
  Serial.printf("  DIAG_ALRT: 0x%04X\n", snapshot.diagAlert);
  Serial.printf("  MEMSTAT: %s\n",
                ((snapshot.diagAlert & INA228::cmd::DIAG_MEMSTAT) != 0U) ? "OK" : "FAIL");
}

void scanIna228Addresses() {
  Serial.println("=== INA228 Address Probe (0x40-0x4F) ===");
  Serial.println("Note: INA228 probes read DIAG_ALRT for MEMSTAT and can clear CNVRF/latched diagnostic evidence.");
  uint8_t healthyCount = 0;
  for (uint8_t address = INA228_ADDR_MIN; address <= INA228_ADDR_MAX; ++address) {
    ProbeSnapshot snapshot{};
    const INA228::Status st = probeAddressRaw(address, snapshot);

    Serial.printf("  0x%02X: ", address);
    if (st.ok()) {
      healthyCount++;
      Serial.printf("%sINA228%s (MFG=0x%04X DEV=0x%04X MEMSTAT=OK)\n",
                    LOG_COLOR_GREEN,
                    LOG_COLOR_RESET,
                    snapshot.manufacturerId,
                    snapshot.deviceId);
      continue;
    }

    switch (st.code) {
      case INA228::Err::I2C_NACK_ADDR:
        Serial.println("--");
        break;
      case INA228::Err::DEVICE_ID_MISMATCH:
        if (snapshot.manufacturerId != 0U &&
            snapshot.manufacturerId != INA228::cmd::MANUFACTURER_ID) {
          Serial.printf("%snot INA228%s (MFG=0x%04X)\n",
                        LOG_COLOR_YELLOW,
                        LOG_COLOR_RESET,
                        snapshot.manufacturerId);
        } else {
          Serial.printf("%sdevice ID mismatch%s (DEV=0x%04X)\n",
                        LOG_COLOR_YELLOW,
                        LOG_COLOR_RESET,
                        snapshot.deviceId);
        }
        break;
      case INA228::Err::MEMORY_ERROR:
        Serial.printf("%sINA228 with MEMSTAT fault%s (DIAG_ALRT=0x%04X)\n",
                      LOG_COLOR_RED,
                      LOG_COLOR_RESET,
                      snapshot.diagAlert);
        break;
      default:
        Serial.printf("%s%s%s", LOG_COLOR_RED, errToStr(st.code), LOG_COLOR_RESET);
        if (st.detail != 0) {
          Serial.printf(" (detail=%ld)", static_cast<long>(st.detail));
        }
        Serial.println();
        break;
    }
  }

  Serial.printf("  Healthy INA228 devices: %u\n", healthyCount);
}

uint8_t detectHealthyIna228Addresses(ProbeSnapshot* matches, uint8_t maxMatches) {
  uint8_t count = 0;
  for (uint8_t address = INA228_ADDR_MIN; address <= INA228_ADDR_MAX; ++address) {
    ProbeSnapshot snapshot{};
    const INA228::Status st = probeAddressRaw(address, snapshot);
    if (!st.ok()) {
      continue;
    }
    if (count < maxMatches) {
      matches[count] = snapshot;
    }
    count++;
  }
  return count;
}

bool parseAddressArg(const String& text, uint8_t& outAddress) {
  String trimmed = text;
  trimmed.trim();
  if (trimmed.length() == 0) {
    return false;
  }

  char* end = nullptr;
  const unsigned long value = std::strtoul(trimmed.c_str(), &end, 0);
  if (end == nullptr || *end != '\0' || value > 0xFFUL) {
    return false;
  }

  const uint8_t address = static_cast<uint8_t>(value);
  if (!isValidIna228Address(address)) {
    return false;
  }

  outAddress = address;
  return true;
}

INA228::Status initializeDevice(uint8_t address, bool allowAutoDetectFallback) {
  selectedAddress = address;
  device.end();

  INA228::Status st = device.begin(makeExampleConfig(address));
  if (st.ok()) {
    selectedAddress = address;
    return st;
  }

  if (!allowAutoDetectFallback) {
    return st;
  }

  ProbeSnapshot matches[16] = {};
  const uint8_t count = detectHealthyIna228Addresses(matches, 16);
  if (count == 1U && matches[0].address != address) {
    LOGW("Configured address 0x%02X failed; detected INA228 at 0x%02X",
         address, matches[0].address);
    selectedAddress = matches[0].address;
    st = device.begin(makeExampleConfig(matches[0].address));
    if (st.ok()) {
      selectedAddress = matches[0].address;
    }
    return st;
  }

  if (count > 1U) {
    LOGW("Multiple INA228 devices detected on the bus; choose address explicitly with init <addr>");
    for (uint8_t i = 0; i < count; ++i) {
      LOGI("INA228 candidate at 0x%02X", matches[i].address);
    }
  } else {
    LOGW("No healthy INA228 detected on addresses 0x40-0x4F");
  }

  return st;
}

const char* errToStr(INA228::Err err) {
  using namespace INA228;
  switch (err) {
    case Err::OK:                    return "OK";
    case Err::NOT_INITIALIZED:       return "NOT_INITIALIZED";
    case Err::INVALID_CONFIG:        return "INVALID_CONFIG";
    case Err::I2C_ERROR:             return "I2C_ERROR";
    case Err::TIMEOUT:               return "TIMEOUT";
    case Err::INVALID_PARAM:         return "INVALID_PARAM";
    case Err::DEVICE_NOT_FOUND:      return "DEVICE_NOT_FOUND";
    case Err::DEVICE_ID_MISMATCH:    return "DEVICE_ID_MISMATCH";
    case Err::MEMORY_ERROR:          return "MEMORY_ERROR";
    case Err::MEASUREMENT_NOT_READY: return "MEASUREMENT_NOT_READY";
    case Err::MATH_OVERFLOW:         return "MATH_OVERFLOW";
    case Err::BUSY:                  return "BUSY";
    case Err::IN_PROGRESS:           return "IN_PROGRESS";
    case Err::I2C_NACK_ADDR:         return "I2C_NACK_ADDR";
    case Err::I2C_NACK_DATA:         return "I2C_NACK_DATA";
    case Err::I2C_TIMEOUT:           return "I2C_TIMEOUT";
    case Err::I2C_BUS:               return "I2C_BUS";
    case Err::ACCUMULATION_INVALID:  return "ACCUMULATION_INVALID";
    case Err::ACCUMULATION_OVERFLOW: return "ACCUMULATION_OVERFLOW";
    case Err::HARDWARE_DIRTY:        return "HARDWARE_DIRTY";
    default:                         return "UNKNOWN";
  }
}

const char* stateToStr(INA228::DriverState st) {
  using namespace INA228;
  switch (st) {
    case DriverState::UNINIT:   return "UNINIT";
    case DriverState::READY:    return "READY";
    case DriverState::DEGRADED: return "DEGRADED";
    case DriverState::OFFLINE:  return "OFFLINE";
    default:                    return "UNKNOWN";
  }
}

const char* stateColor(INA228::DriverState st, bool online, uint8_t consecutiveFailures) {
  if (st == INA228::DriverState::UNINIT) {
    return LOG_COLOR_YELLOW;
  }
  return LOG_COLOR_STATE(online, consecutiveFailures);
}

const char* goodIfZeroColor(uint32_t value) {
  return (value == 0U) ? LOG_COLOR_GREEN : LOG_COLOR_RED;
}

const char* goodIfNonZeroColor(uint32_t value) {
  return (value > 0U) ? LOG_COLOR_GREEN : LOG_COLOR_YELLOW;
}

const char* onOffColor(bool enabled) {
  return enabled ? LOG_COLOR_GREEN : LOG_COLOR_RESET;
}

const char* skipCountColor(uint32_t value) {
  return (value > 0U) ? LOG_COLOR_YELLOW : LOG_COLOR_RESET;
}

const char* successRateColor(float pct) {
  if (pct >= 99.9f) return LOG_COLOR_GREEN;
  if (pct >= 80.0f) return LOG_COLOR_YELLOW;
  return LOG_COLOR_RED;
}

const char* modeToStr(INA228::Mode mode) {
  using INA228::Mode;
  switch (mode) {
    case Mode::SHUTDOWN:         return "SHUTDOWN";
    case Mode::TRIG_BUS:         return "TRIG_BUS";
    case Mode::TRIG_SHUNT:       return "TRIG_SHUNT";
    case Mode::TRIG_SHUNT_BUS:   return "TRIG_SHUNT_BUS";
    case Mode::TRIG_TEMP:        return "TRIG_TEMP";
    case Mode::TRIG_TEMP_BUS:    return "TRIG_TEMP_BUS";
    case Mode::TRIG_TEMP_SHUNT:  return "TRIG_TEMP_SHUNT";
    case Mode::TRIG_ALL:         return "TRIG_ALL";
    case Mode::SHUTDOWN2:        return "SHUTDOWN2";
    case Mode::CONT_BUS:         return "CONT_BUS";
    case Mode::CONT_SHUNT:       return "CONT_SHUNT";
    case Mode::CONT_SHUNT_BUS:   return "CONT_SHUNT_BUS";
    case Mode::CONT_TEMP:        return "CONT_TEMP";
    case Mode::CONT_TEMP_BUS:    return "CONT_TEMP_BUS";
    case Mode::CONT_TEMP_SHUNT:  return "CONT_TEMP_SHUNT";
    case Mode::CONT_ALL:         return "CONT_ALL";
    default:                     return "UNKNOWN";
  }
}

bool modeIncludesConversion(INA228::Mode mode) {
  return (static_cast<uint8_t>(mode) & 0x07U) != 0U;
}

const char* convTimeToStr(INA228::ConvTime ct) {
  using INA228::ConvTime;
  switch (ct) {
    case ConvTime::US_50:   return "50us";
    case ConvTime::US_84:   return "84us";
    case ConvTime::US_150:  return "150us";
    case ConvTime::US_280:  return "280us";
    case ConvTime::US_540:  return "540us";
    case ConvTime::US_1052: return "1052us";
    case ConvTime::US_2074: return "2074us";
    case ConvTime::US_4120: return "4120us";
    default:                return "UNKNOWN";
  }
}

const char* avgToStr(INA228::Averaging avg) {
  using INA228::Averaging;
  switch (avg) {
    case Averaging::AVG_1:    return "1";
    case Averaging::AVG_4:    return "4";
    case Averaging::AVG_16:   return "16";
    case Averaging::AVG_64:   return "64";
    case Averaging::AVG_128:  return "128";
    case Averaging::AVG_256:  return "256";
    case Averaging::AVG_512:  return "512";
    case Averaging::AVG_1024: return "1024";
    default:                  return "UNKNOWN";
  }
}

const char* adcRangeToStr(INA228::AdcRange range) {
  using INA228::AdcRange;
  switch (range) {
    case AdcRange::MV_163_84: return "+/-163.84mV";
    case AdcRange::MV_40_96:  return "+/-40.96mV";
    default:                  return "UNKNOWN";
  }
}

// ============================================================================
// Print Helpers
// ============================================================================

void printStatus(const INA228::Status& st) {
  Serial.printf("  Status: %s%s%s (code=%u, detail=%ld)\n",
                LOG_COLOR_RESULT(st.ok()),
                errToStr(st.code),
                LOG_COLOR_RESET,
                static_cast<unsigned>(st.code),
                static_cast<long>(st.detail));
  if (st.msg && st.msg[0]) {
    Serial.printf("  Message: %s%s%s\n", LOG_COLOR_YELLOW, st.msg, LOG_COLOR_RESET);
  }
}

void printDriverHealth() {
  const uint32_t now = millis();
  const uint32_t totalOk = device.totalSuccess();
  const uint32_t totalFail = device.totalFailures();
  const uint32_t total = totalOk + totalFail;
  const float successRate = (total > 0U)
                                ? (100.0f * static_cast<float>(totalOk) / static_cast<float>(total))
                                : 0.0f;
  const INA228::Status lastErr = device.lastError();
  const INA228::DriverState st = device.state();
  const bool online = device.isOnline();

  Serial.println("=== Driver Health ===");
  Serial.printf("  Configured address: 0x%02X\n", configuredAddress());
  Serial.printf("  State: %s%s%s\n",
                stateColor(st, online, device.consecutiveFailures()),
                stateToStr(st),
                LOG_COLOR_RESET);
  Serial.printf("  Online: %s%s%s\n",
                online ? LOG_COLOR_GREEN : LOG_COLOR_RED,
                log_bool_str(online),
                LOG_COLOR_RESET);
  Serial.printf("  Consecutive failures: %s%u%s\n",
                goodIfZeroColor(device.consecutiveFailures()),
                device.consecutiveFailures(),
                LOG_COLOR_RESET);
  Serial.printf("  Total success: %s%lu%s\n",
                goodIfNonZeroColor(totalOk),
                static_cast<unsigned long>(totalOk),
                LOG_COLOR_RESET);
  Serial.printf("  Total failures: %s%lu%s\n",
                goodIfZeroColor(totalFail),
                static_cast<unsigned long>(totalFail),
                LOG_COLOR_RESET);
  Serial.printf("  Success rate: %s%.1f%%%s\n",
                successRateColor(successRate),
                successRate,
                LOG_COLOR_RESET);

  const uint32_t lastOkMs = device.lastOkMs();
  if (lastOkMs > 0U) {
    Serial.printf("  Last OK: %lu ms ago (at %lu ms)\n",
                  static_cast<unsigned long>(now - lastOkMs),
                  static_cast<unsigned long>(lastOkMs));
  } else {
    Serial.println("  Last OK: never");
  }

  const uint32_t lastErrorMs = device.lastErrorMs();
  if (lastErrorMs > 0U) {
    Serial.printf("  Last error: %lu ms ago (at %lu ms)\n",
                  static_cast<unsigned long>(now - lastErrorMs),
                  static_cast<unsigned long>(lastErrorMs));
  } else {
    Serial.println("  Last error: never");
  }

  if (!lastErr.ok()) {
    Serial.printf("  Error code: %s%s%s\n",
                  LOG_COLOR_RED,
                  errToStr(lastErr.code),
                  LOG_COLOR_RESET);
    Serial.printf("  Error detail: %ld\n", static_cast<long>(lastErr.detail));
    if (lastErr.msg && lastErr.msg[0]) {
      Serial.printf("  Error msg: %s\n", lastErr.msg);
    }
  }
}

void printVersionInfo() {
  Serial.println("=== Version Info ===");
  Serial.printf("  Example firmware build: %s %s\n", __DATE__, __TIME__);
  Serial.printf("  INA228 library version: %s\n", INA228::VERSION);
  Serial.printf("  INA228 library full: %s\n", INA228::VERSION_FULL);
  Serial.printf("  INA228 library build: %s\n", INA228::BUILD_TIMESTAMP);
  Serial.printf("  INA228 library commit: %s (%s)\n", INA228::GIT_COMMIT, INA228::GIT_STATUS);
}

bool parseU32(const String& token, uint32_t& out) {
  char* end = nullptr;
  const unsigned long value = strtoul(token.c_str(), &end, 0);
  if (end == token.c_str() || *end != '\0') {
    return false;
  }
  out = static_cast<uint32_t>(value);
  return true;
}

bool parseFloat(const String& token, float& out) {
  char* end = nullptr;
  const float value = strtof(token.c_str(), &end);
  if (end == token.c_str() || *end != '\0') {
    return false;
  }
  out = value;
  return true;
}

float shuntLimitToMv(uint16_t raw) {
  const double lsb = (device.getConfig().adcRange == INA228::AdcRange::MV_40_96)
                         ? INA228::cmd::SHUNT_THRESHOLD_LSB_RANGE1
                         : INA228::cmd::SHUNT_THRESHOLD_LSB_RANGE0;
  return static_cast<float>(static_cast<int16_t>(raw) * lsb * 1000.0);
}

float busLimitToV(uint16_t raw) {
  return static_cast<float>((raw & 0x7FFFu) * INA228::cmd::BUS_THRESHOLD_LSB);
}

float tempLimitToC(uint16_t raw) {
  return static_cast<float>(static_cast<int16_t>(raw) * INA228::cmd::TEMP_LSB);
}

double powerLimitToW(uint16_t raw) {
  const float currentLsb = device.currentLsb();
  if (currentLsb <= 0.0f) {
    return 0.0;
  }
  return static_cast<double>(raw) * 256.0 *
         INA228::cmd::POWER_COEFF * static_cast<double>(currentLsb);
}

void printMeasurement() {
  INA228::Measurement m{};
  auto st = device.readMeasurement(m);
  if (!st.ok()) {
    LOGE("readMeasurement failed:");
    printStatus(st);
    return;
  }

  Serial.printf("  Vbus:    %.4f V\n", m.busVoltageV);
  Serial.printf("  Vshunt:  %.7f V\n", m.shuntVoltageV);
  Serial.printf("  Temp:    %.2f C\n", m.temperatureC);
  Serial.printf("  Current: %.6f A\n", m.currentA);
  Serial.printf("  Power:   %.6f W\n", m.powerW);
  Serial.printf("  Energy:  %.9f J\n", m.energyJ);
  Serial.printf("  Charge:  %.9f C\n", m.chargeC);
  Serial.printf("  Accum valid: energy=%s charge=%s\n",
                log_bool_str(m.energyValid),
                log_bool_str(m.chargeValid));
  Serial.printf("  Accum overflow: ENERGYOF=%s CHARGEOF=%s MATHOF=%s\n",
                log_bool_str(m.energyOverflow),
                log_bool_str(m.chargeOverflow),
                log_bool_str(m.mathOverflow));
  if (m.diagAlertValid) {
    Serial.printf("  DIAG_ALRT snapshot before accumulator reads: 0x%04X\n",
                  m.diagAlertRaw);
  }
}

void printRawSample() {
  INA228::RawSample raw{};
  auto st = device.readRawSample(raw);
  if (!st.ok()) {
    printStatus(st);
    return;
  }

  Serial.println("=== Raw Registers ===");
  Serial.printf("  Vshunt: %ld (0x%06lX)\n",
                static_cast<long>(raw.vshunt),
                static_cast<unsigned long>(raw.vshunt & 0xFFFFF));
  Serial.printf("  Vbus:   %ld (0x%06lX)\n",
                static_cast<long>(raw.vbus),
                static_cast<unsigned long>(raw.vbus & 0xFFFFF));
  Serial.printf("  Temp:   %d (0x%04X)\n",
                static_cast<int>(raw.dietemp),
                static_cast<unsigned>(raw.dietemp & 0xFFFF));
  Serial.printf("  Current:%ld (0x%06lX)\n",
                static_cast<long>(raw.current),
                static_cast<unsigned long>(raw.current & 0xFFFFF));
  Serial.printf("  Power:  %lu (0x%06lX)\n",
                static_cast<unsigned long>(raw.power),
                static_cast<unsigned long>(raw.power));
  Serial.printf("  Energy: %llu\n", static_cast<unsigned long long>(raw.energy));
  Serial.printf("  Charge: %lld\n", static_cast<long long>(raw.charge));
  Serial.printf("  Accum valid: energy=%s charge=%s\n",
                log_bool_str(raw.energyValid),
                log_bool_str(raw.chargeValid));
  Serial.printf("  Accum overflow: ENERGYOF=%s CHARGEOF=%s MATHOF=%s\n",
                log_bool_str(raw.energyOverflow),
                log_bool_str(raw.chargeOverflow),
                log_bool_str(raw.mathOverflow));
  if (raw.diagAlertValid) {
    Serial.printf("  DIAG_ALRT snapshot before accumulator reads: 0x%04X\n",
                  raw.diagAlertRaw);
  }
}

void printDiag() {
  INA228::DiagAlert diag{};
  auto st = device.readDiagAlert(diag);
  if (!st.ok()) {
    LOGE("readDiagAlert failed:");
    printStatus(st);
    return;
  }

  Serial.println("=== DIAG_ALRT Flags ===");
  Serial.println("  Note: this read is destructive/status-clearing for CNVRF and latched diagnostic evidence.");
  Serial.printf("  MEMSTAT:   %s\n", log_bool_str(diag.memstat));
  Serial.printf("  CNVRF:     %s\n", log_bool_str(diag.cnvrf));
  Serial.printf("  ALATCH:    %s\n", log_bool_str(diag.alatch));
  Serial.printf("  CNVR:      %s\n", log_bool_str(diag.cnvr));
  Serial.printf("  SLOWALERT: %s\n", log_bool_str(diag.slowAlert));
  Serial.printf("  APOL:      %s\n", log_bool_str(diag.apol));
  Serial.printf("  ENERGYOF:  %s\n", log_bool_str(diag.energyOF));
  Serial.printf("  CHARGEOF:  %s\n", log_bool_str(diag.chargeOF));
  Serial.printf("  MATHOF:    %s\n", log_bool_str(diag.mathOF));
  Serial.printf("  TMPOL:     %s\n", log_bool_str(diag.tmpOL));
  Serial.printf("  SHNTOL:    %s\n", log_bool_str(diag.shntOL));
  Serial.printf("  SHNTUL:    %s\n", log_bool_str(diag.shntUL));
  Serial.printf("  BUSOL:     %s\n", log_bool_str(diag.busOL));
  Serial.printf("  BUSUL:     %s\n", log_bool_str(diag.busUL));
  Serial.printf("  POL:       %s\n", log_bool_str(diag.pOL));
}

void printAlertLimits() {
  uint16_t sovl = 0;
  uint16_t suvl = 0;
  uint16_t bovl = 0;
  uint16_t buvl = 0;
  uint16_t temp = 0;
  uint16_t power = 0;

  INA228::Status st = device.readRegister16(INA228::cmd::REG_SOVL, sovl);
  if (st.ok()) st = device.readRegister16(INA228::cmd::REG_SUVL, suvl);
  if (st.ok()) st = device.readRegister16(INA228::cmd::REG_BOVL, bovl);
  if (st.ok()) st = device.readRegister16(INA228::cmd::REG_BUVL, buvl);
  if (st.ok()) st = device.readRegister16(INA228::cmd::REG_TEMP_LIMIT, temp);
  if (st.ok()) st = device.readRegister16(INA228::cmd::REG_PWR_LIMIT, power);
  if (!st.ok()) {
    printStatus(st);
    return;
  }

  Serial.println("=== Alert Limits ===");
  Serial.printf("  SOVL:      0x%04X  %.3f mV\n", sovl, static_cast<double>(shuntLimitToMv(sovl)));
  Serial.printf("  SUVL:      0x%04X  %.3f mV\n", suvl, static_cast<double>(shuntLimitToMv(suvl)));
  Serial.printf("  BOVL:      0x%04X  %.4f V\n", bovl, static_cast<double>(busLimitToV(bovl)));
  Serial.printf("  BUVL:      0x%04X  %.4f V\n", buvl, static_cast<double>(busLimitToV(buvl)));
  Serial.printf("  TEMP_LIMIT:0x%04X  %.2f C\n", temp, static_cast<double>(tempLimitToC(temp)));
  if (device.currentLsb() > 0.0f) {
    Serial.printf("  PWR_LIMIT: 0x%04X  %.6f W\n", power, powerLimitToW(power));
  } else {
    Serial.printf("  PWR_LIMIT: 0x%04X  requires calibration for W\n", power);
  }
}

void printShuntAlertLimit(const char* label, uint8_t reg) {
  uint16_t raw = 0;
  INA228::Status st = device.readRegister16(reg, raw);
  if (!st.ok()) {
    printStatus(st);
    return;
  }

  Serial.printf("%s: 0x%04X  %.3f mV\n",
                label,
                raw,
                static_cast<double>(shuntLimitToMv(raw)));
}

void printBusAlertLimit(const char* label, uint8_t reg) {
  uint16_t raw = 0;
  INA228::Status st = device.readRegister16(reg, raw);
  if (!st.ok()) {
    printStatus(st);
    return;
  }

  Serial.printf("%s: 0x%04X  %.4f V\n",
                label,
                raw,
                static_cast<double>(busLimitToV(raw)));
}

void printTemperatureAlertLimit() {
  uint16_t raw = 0;
  INA228::Status st = device.readRegister16(INA228::cmd::REG_TEMP_LIMIT, raw);
  if (!st.ok()) {
    printStatus(st);
    return;
  }

  Serial.printf("TEMP_LIMIT: 0x%04X  %.2f C\n",
                raw,
                static_cast<double>(tempLimitToC(raw)));
}

void printPowerAlertLimit() {
  uint16_t raw = 0;
  INA228::Status st = device.readRegister16(INA228::cmd::REG_PWR_LIMIT, raw);
  if (!st.ok()) {
    printStatus(st);
    return;
  }

  if (device.currentLsb() > 0.0f) {
    Serial.printf("PWR_LIMIT: 0x%04X  %.6f W\n", raw, powerLimitToW(raw));
  } else {
    Serial.printf("PWR_LIMIT: 0x%04X  requires calibration for W\n", raw);
  }
}

void printTimingInfo() {
  bool ready = false;
  const INA228::Status st = device.isConversionReady(ready);
  if (!st.ok()) {
    printStatus(st);
    return;
  }
  Serial.printf("Conversion ready: %s\n", ready ? "YES" : "NO");
  Serial.printf("Estimated conversion time: %lu us (%lu ms)\n",
                static_cast<unsigned long>(device.estimateConversionTimeUs()),
                static_cast<unsigned long>(device.estimateConversionTimeMs()));
  Serial.printf("CURRENT_LSB: %.9f A\n", device.currentLsb());
}

void printSettings() {
  INA228::SettingsSnapshot snap;
  INA228::Status st = device.getSettings(snap);
  if (!st.ok()) {
    printStatus(st);
    return;
  }

  Serial.println("=== Active Settings ===");
  Serial.printf("  Initialized:      %s\n", log_bool_str(snap.initialized));
  Serial.printf("  State:            %s\n", stateToStr(snap.state));
  Serial.printf("  Address:          0x%02X\n", snap.i2cAddress);
  Serial.printf("  I2C timeout:      %lu ms\n", static_cast<unsigned long>(snap.i2cTimeoutMs));
  Serial.printf("  Offline threshold:%u\n", static_cast<unsigned>(snap.offlineThreshold));
  Serial.printf("  nowMs hook:       %s\n", log_bool_str(snap.hasNowMsHook));
  Serial.printf("  Mode:             %s (%u)\n", modeToStr(snap.mode), static_cast<unsigned>(snap.mode));
  Serial.printf("  VBUSCT/VSHCT/VTCT:%s / %s / %s\n",
                convTimeToStr(snap.vbusConvTime),
                convTimeToStr(snap.vshuntConvTime),
                convTimeToStr(snap.vtempConvTime));
  Serial.printf("  Averaging:        %s samples\n", avgToStr(snap.averaging));
  Serial.printf("  ADC range:        %s\n", adcRangeToStr(snap.adcRange));
  Serial.printf("  Conversion delay: %u x 2 ms (%u ms)\n",
                snap.convDelayMs2,
                static_cast<unsigned>(snap.convDelayMs2) * 2u);
  Serial.printf("  Temp compensation:%s  tempco=%u ppm/degC\n",
                log_bool_str(snap.tempCompEnabled),
                snap.shuntTempCoeffPpmC);
  Serial.printf("  Calibration:      Rshunt=%.6f ohm  MaxCurrent=%.6f A\n",
                snap.shuntResistanceOhm,
                snap.maxExpectedCurrentA);
  Serial.printf("  Calibration state:calibrated=%s clamped=%s rangeExceeded=%s\n",
                log_bool_str(snap.calibrated),
                log_bool_str(snap.calibrationClamped),
                log_bool_str(snap.maxCurrentExceedsShuntRange));
  Serial.printf("  Hardware dirty:   %s mask=0x%08lX%08lX\n",
                log_bool_str(snap.hardwareDirty),
                static_cast<unsigned long>(snap.dirtyRegisterMask >> 32),
                static_cast<unsigned long>(snap.dirtyRegisterMask & 0xFFFFFFFFULL));
  Serial.printf("  Dirty cause:      %s detail=%ld\n",
                errToStr(snap.hardwareDirtyCause.code),
                static_cast<long>(snap.hardwareDirtyCause.detail));
  Serial.printf("  Thresholds dirty: %s\n", log_bool_str(snap.thresholdsDirty));
  Serial.printf("  Triggered state:  pending=%s start=%lu ms\n",
                log_bool_str(snap.triggeredConversionPending),
                static_cast<unsigned long>(snap.triggeredConversionStartMs));
  Serial.printf("  Est. conv time:   %lu us\n",
                static_cast<unsigned long>(device.estimateConversionTimeUs()));
  Serial.printf("  CURRENT_LSB:      %.9f A\n", snap.currentLsb);
  Serial.printf("  SHUNT_CAL:        0x%04X\n", snap.shuntCal);
}

void printVerboseState() {
  Serial.printf("  Verbose: %s%s%s\n",
                onOffColor(verboseMode),
                verboseMode ? "ON" : "OFF",
                LOG_COLOR_RESET);
}

// ============================================================================
// Stress Test
// ============================================================================

void resetStressStats(int target) {
  stressStats.active = true;
  stressStats.startMs = millis();
  stressStats.endMs = 0;
  stressStats.target = target;
  stressStats.attempts = 0;
  stressStats.success = 0;
  stressStats.errors = 0;
  stressStats.hasSample = false;
  stressStats.minVbus = std::numeric_limits<float>::max();
  stressStats.maxVbus = std::numeric_limits<float>::lowest();
  stressStats.minCurrent = std::numeric_limits<float>::max();
  stressStats.maxCurrent = std::numeric_limits<float>::lowest();
  stressStats.minPower = std::numeric_limits<float>::max();
  stressStats.maxPower = std::numeric_limits<float>::lowest();
  stressStats.sumVbus = 0.0;
  stressStats.sumCurrent = 0.0;
  stressStats.sumPower = 0.0;
  stressStats.lastError = INA228::Status::Ok();
}

void noteStressError(const INA228::Status& st) {
  stressStats.errors++;
  stressStats.lastError = st;
}

void updateStressStats(const INA228::Measurement& m) {
  if (!stressStats.hasSample) {
    stressStats.minVbus = m.busVoltageV;
    stressStats.maxVbus = m.busVoltageV;
    stressStats.minCurrent = m.currentA;
    stressStats.maxCurrent = m.currentA;
    stressStats.minPower = m.powerW;
    stressStats.maxPower = m.powerW;
    stressStats.hasSample = true;
  } else {
    if (m.busVoltageV < stressStats.minVbus) stressStats.minVbus = m.busVoltageV;
    if (m.busVoltageV > stressStats.maxVbus) stressStats.maxVbus = m.busVoltageV;
    if (m.currentA < stressStats.minCurrent) stressStats.minCurrent = m.currentA;
    if (m.currentA > stressStats.maxCurrent) stressStats.maxCurrent = m.currentA;
    if (m.powerW < stressStats.minPower) stressStats.minPower = m.powerW;
    if (m.powerW > stressStats.maxPower) stressStats.maxPower = m.powerW;
  }

  stressStats.sumVbus += m.busVoltageV;
  stressStats.sumCurrent += m.currentA;
  stressStats.sumPower += m.powerW;
  stressStats.success++;
}

void finishStressStats() {
  stressStats.active = false;
  stressStats.endMs = millis();
  const uint32_t durationMs = stressStats.endMs - stressStats.startMs;

  Serial.println("=== Stress Summary ===");
  Serial.printf("  Target: %d\n", stressStats.target);
  Serial.printf("  Attempts: %d\n", stressStats.attempts);
  Serial.printf("  Success: %s%d%s\n",
                goodIfNonZeroColor(static_cast<uint32_t>(stressStats.success)),
                stressStats.success,
                LOG_COLOR_RESET);
  Serial.printf("  Errors: %s%lu%s\n",
                goodIfZeroColor(stressStats.errors),
                static_cast<unsigned long>(stressStats.errors),
                LOG_COLOR_RESET);
  Serial.printf("  Duration: %lu ms\n", static_cast<unsigned long>(durationMs));
  if (durationMs > 0) {
    const float rate = 1000.0f * static_cast<float>(stressStats.attempts) /
                       static_cast<float>(durationMs);
    Serial.printf("  Rate: %.2f samples/s\n", rate);
  }

  if (stressStats.success > 0) {
    const float avgVbus = static_cast<float>(stressStats.sumVbus / stressStats.success);
    const float avgCurrent = static_cast<float>(stressStats.sumCurrent / stressStats.success);
    const float avgPower = static_cast<float>(stressStats.sumPower / stressStats.success);
    Serial.printf("  Vbus V:    min=%.4f avg=%.4f max=%.4f\n",
                  stressStats.minVbus, avgVbus, stressStats.maxVbus);
    Serial.printf("  Current A: min=%.6f avg=%.6f max=%.6f\n",
                  stressStats.minCurrent, avgCurrent, stressStats.maxCurrent);
    Serial.printf("  Power W:   min=%.6f avg=%.6f max=%.6f\n",
                  stressStats.minPower, avgPower, stressStats.maxPower);
  } else {
    Serial.println("  No valid samples");
  }

  if (!stressStats.lastError.ok()) {
    Serial.printf("  Last error: %s\n", errToStr(stressStats.lastError.code));
    if (stressStats.lastError.msg && stressStats.lastError.msg[0]) {
      Serial.printf("  Message: %s\n", stressStats.lastError.msg);
    }
  }
}

void runStress(int count) {
  resetStressStats(count);

  for (int i = 0; i < count; ++i) {
    device.tick(millis());
    INA228::Measurement m{};
    INA228::Status st = device.readMeasurement(m);
    stressStats.attempts++;

    if (st.ok()) {
      updateStressStats(m);
    } else {
      noteStressError(st);
      if (verboseMode) {
        Serial.printf("  [%d] failed: %s\n", i, errToStr(st.code));
      }
    }
    yield();
  }

  finishStressStats();
}

void runStressMix(int count) {
  struct OpStats {
    const char* name;
    uint32_t ok;
    uint32_t fail;
  };
  OpStats stats[] = {
      {"measure",  0, 0},
      {"vbus",     0, 0},
      {"current",  0, 0},
      {"temp",     0, 0},
      {"diag",     0, 0},
      {"mfgid",    0, 0},
      {"devid",    0, 0},
  };
  const int opCount = static_cast<int>(sizeof(stats) / sizeof(stats[0]));

  const uint32_t succBefore = device.totalSuccess();
  const uint32_t failBefore = device.totalFailures();
  const uint32_t startMs = millis();

  for (int i = 0; i < count; ++i) {
    device.tick(millis());
    const int op = i % opCount;
    INA228::Status st = INA228::Status::Ok();

    switch (op) {
      case 0: {
        INA228::Measurement m{};
        st = device.readMeasurement(m);
        break;
      }
      case 1: {
        float v = 0;
        st = device.readBusVoltage(v);
        break;
      }
      case 2: {
        float c = 0;
        st = device.readCurrent(c);
        break;
      }
      case 3: {
        float t = 0;
        st = device.readTemperature(t);
        break;
      }
      case 4: {
        INA228::DiagAlert diag{};
        st = device.readDiagAlert(diag);
        break;
      }
      case 5: {
        uint16_t id = 0;
        st = device.readManufacturerId(id);
        break;
      }
      case 6: {
        uint16_t id = 0;
        st = device.readDeviceId(id);
        break;
      }
      default:
        break;
    }

    if (st.ok()) {
      stats[op].ok++;
    } else {
      stats[op].fail++;
      if (verboseMode) {
        Serial.printf("  [%d] %s failed: %s\n", i, stats[op].name, errToStr(st.code));
      }
    }
    yield();
  }

  const uint32_t elapsed = millis() - startMs;
  uint32_t okTotal = 0;
  uint32_t failTotal = 0;
  for (int i = 0; i < opCount; ++i) {
    okTotal += stats[i].ok;
    failTotal += stats[i].fail;
  }

  Serial.println("=== stress_mix summary ===");
  const float successPct =
      (count > 0) ? (100.0f * static_cast<float>(okTotal) / static_cast<float>(count)) : 0.0f;
  Serial.printf("  Total: %sok=%lu%s %sfail=%lu%s (%s%.2f%%%s)\n",
                goodIfNonZeroColor(okTotal),
                static_cast<unsigned long>(okTotal),
                LOG_COLOR_RESET,
                goodIfZeroColor(failTotal),
                static_cast<unsigned long>(failTotal),
                LOG_COLOR_RESET,
                successRateColor(successPct),
                successPct,
                LOG_COLOR_RESET);
  Serial.printf("  Duration: %lu ms\n", static_cast<unsigned long>(elapsed));
  if (elapsed > 0) {
    Serial.printf("  Rate: %.2f ops/s\n", (1000.0f * static_cast<float>(count)) / elapsed);
  }
  for (int i = 0; i < opCount; ++i) {
    Serial.printf("  %-10s %sok=%lu%s %sfail=%lu%s\n",
                  stats[i].name,
                  goodIfNonZeroColor(stats[i].ok),
                  static_cast<unsigned long>(stats[i].ok),
                  LOG_COLOR_RESET,
                  goodIfZeroColor(stats[i].fail),
                  static_cast<unsigned long>(stats[i].fail),
                  LOG_COLOR_RESET);
  }
  const uint32_t successDelta = device.totalSuccess() - succBefore;
  const uint32_t failDelta = device.totalFailures() - failBefore;
  Serial.printf("  Health delta: %ssuccess +%lu%s, %sfailures +%lu%s\n",
                goodIfNonZeroColor(successDelta),
                static_cast<unsigned long>(successDelta),
                LOG_COLOR_RESET,
                goodIfZeroColor(failDelta),
                static_cast<unsigned long>(failDelta),
                LOG_COLOR_RESET);
}

// ============================================================================
// Self Test
// ============================================================================

void runSelfTest() {
  struct Result {
    uint32_t pass = 0;
    uint32_t fail = 0;
    uint32_t skip = 0;
  } result;

  enum class Outcome : uint8_t { PASS, FAIL, SKIP };
  auto report = [&](const char* name, Outcome outcome, const char* note) {
    const bool ok = (outcome == Outcome::PASS);
    const bool skip = (outcome == Outcome::SKIP);
    const char* color = skip ? LOG_COLOR_YELLOW : LOG_COLOR_RESULT(ok);
    const char* tag = skip ? "SKIP" : (ok ? "PASS" : "FAIL");
    Serial.printf("  [%s%s%s] %s", color, tag, LOG_COLOR_RESET, name);
    if (note && note[0]) {
      Serial.printf(" - %s", note);
    }
    Serial.println();
    if (skip) {
      result.skip++;
    } else if (ok) {
      result.pass++;
    } else {
      result.fail++;
    }
  };
  auto reportCheck = [&](const char* name, bool ok, const char* note) {
    report(name, ok ? Outcome::PASS : Outcome::FAIL, note);
  };
  auto reportSkip = [&](const char* name, const char* note) {
    report(name, Outcome::SKIP, note);
  };

  Serial.println("=== INA228 selftest (diagnostic commands; reads DIAG_ALRT) ===");
  Serial.println("Note: DIAG_ALRT reads can clear CNVRF and latched evidence.");

  const uint32_t succBefore = device.totalSuccess();
  const uint32_t failBefore = device.totalFailures();
  const uint8_t consBefore = device.consecutiveFailures();

  // Probe (no health tracking)
  INA228::Status st = device.probe();
  if (st.code == INA228::Err::NOT_INITIALIZED) {
    reportSkip("probe responds", "driver not initialized");
    reportSkip("remaining checks", "selftest aborted");
    Serial.printf("Selftest result: pass=%s%lu%s fail=%s%lu%s skip=%s%lu%s\n",
                  goodIfNonZeroColor(result.pass), static_cast<unsigned long>(result.pass), LOG_COLOR_RESET,
                  goodIfZeroColor(result.fail), static_cast<unsigned long>(result.fail), LOG_COLOR_RESET,
                  skipCountColor(result.skip), static_cast<unsigned long>(result.skip), LOG_COLOR_RESET);
    return;
  }
  reportCheck("probe responds", st.ok(), st.ok() ? "" : errToStr(st.code));
  const bool probeNoTrack = device.totalSuccess() == succBefore &&
                            device.totalFailures() == failBefore &&
                            device.consecutiveFailures() == consBefore;
  reportCheck("probe no-health-side-effects", probeNoTrack, "");

  // Manufacturer ID
  uint16_t mfgId = 0;
  st = device.readManufacturerId(mfgId);
  reportCheck("readManufacturerId", st.ok(), st.ok() ? "" : errToStr(st.code));
  reportCheck("manufacturer id = 0x5449", st.ok() && mfgId == 0x5449, "");

  // Device ID
  uint16_t devId = 0;
  st = device.readDeviceId(devId);
  reportCheck("readDeviceId", st.ok(), st.ok() ? "" : errToStr(st.code));
  reportCheck("device id = 0x2281", st.ok() && devId == 0x2281, "");

  // Mode read
  INA228::Mode mode = INA228::Mode::SHUTDOWN;
  st = device.getMode(mode);
  const bool modeOk = st.ok();
  reportCheck("getMode", modeOk, modeOk ? "" : errToStr(st.code));

  // Conversion ready
  bool ready = false;
  st = device.isConversionReady(ready);
  reportCheck("isConversionReady", st.ok(), st.ok() ? "" : errToStr(st.code));

  // Diagnostic register
  INA228::DiagAlert diag{};
  st = device.readDiagAlert(diag);
  reportCheck("readDiagAlert", st.ok(), st.ok() ? "" : errToStr(st.code));
  reportCheck("MEMSTAT ok", st.ok() && diag.memstat, "NV memory integrity");

  // Measurement
  INA228::Measurement m{};
  st = device.readMeasurement(m);
  reportCheck("readMeasurement", st.ok(), st.ok() ? "" : errToStr(st.code));
  const bool mRange = (m.busVoltageV >= -0.5f && m.busVoltageV <= 86.0f) &&
                      (m.temperatureC > -60.0f && m.temperatureC < 200.0f);
  reportCheck("measurement in plausible range", st.ok() && mRange, "");

  // Raw sample
  INA228::RawSample raw{};
  st = device.readRawSample(raw);
  reportCheck("readRawSample", st.ok(), st.ok() ? "" : errToStr(st.code));

  // Timing
  INA228::Mode timingMode = INA228::Mode::SHUTDOWN;
  st = device.getMode(timingMode);
  const uint32_t convUs = device.estimateConversionTimeUs();
  if (st.ok()) {
    const bool activeConversion = modeIncludesConversion(timingMode);
    reportCheck("estimateConversionTimeUs",
                activeConversion ? (convUs > 0U) : (convUs == 0U),
                activeConversion ? "" : "shutdown/no active channels");
  } else {
    reportSkip("estimateConversionTimeUs", "mode unavailable");
  }

  // Current LSB
  const float lsb = device.currentLsb();
  reportCheck("currentLsb>0 (calibrated)", lsb > 0.0f, "");

  // Recovery
  st = device.recover();
  reportCheck("recover", st.ok(), st.ok() ? "" : errToStr(st.code));
  reportCheck("isOnline", device.isOnline(), "");

  Serial.printf("Selftest result: pass=%s%lu%s fail=%s%lu%s skip=%s%lu%s\n",
                goodIfNonZeroColor(result.pass), static_cast<unsigned long>(result.pass), LOG_COLOR_RESET,
                goodIfZeroColor(result.fail), static_cast<unsigned long>(result.fail), LOG_COLOR_RESET,
                skipCountColor(result.skip), static_cast<unsigned long>(result.skip), LOG_COLOR_RESET);
}

// ============================================================================
// Help
// ============================================================================

void printHelp() {
  Serial.println();
  cli::printHelpHeader("INA228 CLI Help");
  LOGW("Safety: this example does not make 85 V systems safe. Use qualified design practices, isolation where needed, fusing, creepage/clearance, shunt power checks, and USB-ground care.");

  cli::printHelpSection("Common");
  cli::printHelpItem("help / ?", "Show this help");
  cli::printHelpItem("version / ver", "Print firmware and library version info");
  cli::printHelpItem("scan", "Scan I2C bus and probe 0x40-0x4F for INA228 IDs");
  cli::printHelpItem("scanina", "Probe INA228 IDs; reads DIAG_ALRT/MEMSTAT");
  cli::printHelpItem("read", "Read all measurements with accumulator validity flags");
  cli::printHelpItem("raw", "Read raw register values with validity flags");
  cli::printHelpItem("timing", "Show conversion timing and calibration info");

  cli::printHelpSection("Measurement");
  cli::printHelpItem("vbus", "Read bus voltage");
  cli::printHelpItem("vshunt", "Read shunt voltage");
  cli::printHelpItem("temp", "Read die temperature");
  cli::printHelpItem("current", "Read current");
  cli::printHelpItem("power", "Read power");
  cli::printHelpItem("energy", "Read accumulated energy (continuous accumulation only)");
  cli::printHelpItem("charge", "Read accumulated charge (continuous accumulation only)");
  cli::printHelpItem("ready", "Check if conversion is ready");
  cli::printHelpItem("trigger [mode]", "Trigger single-shot conversion (0-7)");

  cli::printHelpSection("Configuration");
  cli::printHelpItem("mode [0..15]", "Set or show operating mode");
  cli::printHelpItem("convtime [vbus|vsh|temp <0..7>]", "Set conversion time per channel");
  cli::printHelpItem("averaging [0..7]", "Set averaging count");
  cli::printHelpItem("adcrange [0|1]", "Set shunt ADC range");
  cli::printHelpItem("cal <shunt_ohm> <max_current_a>", "Show or update calibration");
  cli::printHelpItem("tempco [ppm]", "Show or set shunt temp coefficient");
  cli::printHelpItem("tempcomp [0|1]", "Show or enable temp compensation");
  cli::printHelpItem("delay [0..255]", "Show or set conversion delay (2 ms steps)");
  cli::printHelpItem("cfg / settings", "Show active settings");
  cli::printHelpItem("addr [0x40..0x4F]", "Show or set target INA228 address");
  cli::printHelpItem("init [0x40..0x4F]", "Re-initialize device at current or given address");
  cli::printHelpItem("end", "Shutdown driver");
  cli::printHelpItem("reset", "Software reset device");
  cli::printHelpItem("rstacc", "Reset energy/charge accumulators");

  cli::printHelpSection("Alert & Diagnostics");
  cli::printHelpItem("diag", "Read DIAG_ALRT flags (destructive/status-clearing)");
  cli::printHelpItem("diagraw", "Read raw DIAG_ALRT (destructive/status-clearing)");
  cli::printHelpItem("limits", "Read alert limit registers with decoded units");
  cli::printHelpItem("alatch [0|1]", "Show or set alert latch mode");
  cli::printHelpItem("cnvralert [0|1]", "Show or enable conversion-ready alert output");
  cli::printHelpItem("alslow [0|1]", "Show or set slow-alert mode");
  cli::printHelpItem("apol [0|1]", "Show or set alert polarity");
  cli::printHelpItem("sovl [volts]", "Show or set shunt overvoltage threshold");
  cli::printHelpItem("suvl [volts]", "Show or set shunt undervoltage threshold");
  cli::printHelpItem("bovl [volts]", "Show or set bus overvoltage threshold");
  cli::printHelpItem("buvl [volts]", "Show or set bus undervoltage threshold");
  cli::printHelpItem("tmplim [degC]", "Show or set temperature over-limit threshold");
  cli::printHelpItem("pwrlim [watts]", "Show or set power over-limit threshold");
  cli::printHelpItem("mfgid", "Read manufacturer ID (expect 0x5449)");
  cli::printHelpItem("devid", "Read device ID (expect 0x2281)");
  cli::printHelpItem("reg16 <addr>", "Read 16-bit register (diagnostic; may clear flags)");
  cli::printHelpItem("reg24 <addr>", "Read 24-bit register (diagnostic; may clear flags)");
  cli::printHelpItem("reg40 <addr>", "Read 40-bit register (diagnostic; may clear flags)");
  cli::printHelpItem("wreg16 <addr> <val>", "Write 16-bit register (diagnostic only; may desync cached config)");

  cli::printHelpSection("Diagnostics");
  cli::printHelpItem("drv", "Show driver state and health");
  cli::printHelpItem("probe", "Probe device; reads DIAG_ALRT; no health tracking");
  cli::printHelpItem("recover", "Manual recovery attempt");
  cli::printHelpItem("verbose [0|1]", "Enable/disable verbose output");
  cli::printHelpItem("stress [N]", "Run N measurement cycles (default 10)");
  cli::printHelpItem("stress_mix [N]", "Run N mixed-operation cycles (default 50)");
  cli::printHelpItem("selftest", "Run diagnostic self-test; reads DIAG_ALRT");
}

// ============================================================================
// Command Processing
// ============================================================================

void processCommand(const String& cmdLine) {
  String cmd = cmdLine;
  cmd.trim();
  if (cmd.length() == 0) {
    return;
  }

  using namespace INA228;

  // --- Common ---
  if (cmd == "help" || cmd == "?") {
    printHelp();
    return;
  }

  if (cmd == "version" || cmd == "ver") {
    printVersionInfo();
    return;
  }

  if (cmd == "scan") {
    bus_diag::scan();
    scanIna228Addresses();
    return;
  }

  if (cmd == "scanina") {
    scanIna228Addresses();
    return;
  }

  if (cmd == "read") {
    LOGI("Reading all measurements:");
    printMeasurement();
    return;
  }

  if (cmd == "raw") {
    printRawSample();
    return;
  }

  if (cmd == "timing") {
    printTimingInfo();
    return;
  }

  // --- Measurement ---
  if (cmd == "vbus") {
    float v = 0;
    auto st = device.readBusVoltage(v);
    if (st.ok()) { LOGI("Vbus: %.4f V", v); }
    else { printStatus(st); }
    return;
  }

  if (cmd == "vshunt") {
    float v = 0;
    auto st = device.readShuntVoltage(v);
    if (st.ok()) { LOGI("Vshunt: %.7f V", v); }
    else { printStatus(st); }
    return;
  }

  if (cmd == "temp") {
    float t = 0;
    auto st = device.readTemperature(t);
    if (st.ok()) { LOGI("Temp: %.2f C", t); }
    else { printStatus(st); }
    return;
  }

  if (cmd == "current") {
    float i = 0;
    auto st = device.readCurrent(i);
    if (st.ok()) { LOGI("Current: %.6f A", i); }
    else { printStatus(st); }
    return;
  }

  if (cmd == "power") {
    float p = 0;
    auto st = device.readPower(p);
    if (st.ok()) { LOGI("Power: %.6f W", p); }
    else { printStatus(st); }
    return;
  }

  if (cmd == "energy") {
    double e = 0;
    auto st = device.readEnergy(e);
    if (st.ok()) { LOGI("Energy: %.9f J", e); }
    else { printStatus(st); }
    return;
  }

  if (cmd == "charge") {
    double q = 0;
    auto st = device.readCharge(q);
    if (st.ok()) { LOGI("Charge: %.9f C", q); }
    else { printStatus(st); }
    return;
  }

  if (cmd == "ready") {
    bool rdy = false;
    auto st = device.isConversionReady(rdy);
    if (st.ok()) { LOGI("Conversion ready: %s", log_bool_str(rdy)); }
    else { printStatus(st); }
    return;
  }

  if (cmd == "trigger") {
    auto st = device.triggerConversion(Mode::TRIG_ALL);
    const bool accepted = st.ok() || st.inProgress();
    LOGI("triggerConversion(TRIG_ALL): %s%s%s",
         LOG_COLOR_RESULT(accepted), errToStr(st.code), LOG_COLOR_RESET);
    if (!accepted) printStatus(st);
    return;
  }

  if (cmd.startsWith("trigger ")) {
    const int val = cmd.substring(8).toInt();
    if (val < 0 || val > 7) {
      LOGW("Invalid trigger mode (0-7 for TRIG_* modes)");
      return;
    }
    auto st = device.triggerConversion(static_cast<Mode>(val));
    const bool accepted = st.ok() || st.inProgress();
    LOGI("triggerConversion(%d): %s%s%s",
         val, LOG_COLOR_RESULT(accepted), errToStr(st.code), LOG_COLOR_RESET);
    if (!accepted) printStatus(st);
    return;
  }

  // --- Configuration ---
  if (cmd == "mode") {
    Mode mode;
    auto st = device.getMode(mode);
    if (st.ok()) {
      Serial.printf("Mode: %s (%u)\n", modeToStr(mode), static_cast<unsigned>(mode));
    } else {
      printStatus(st);
    }
    return;
  }

  if (cmd.startsWith("mode ")) {
    const int val = cmd.substring(5).toInt();
    if (val < 0 || val > 15) {
      LOGW("Invalid mode (0-15)");
      return;
    }
    auto st = device.setMode(static_cast<Mode>(val));
    LOGI("setMode(%d = %s): %s%s%s",
         val, modeToStr(static_cast<Mode>(val)),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd.startsWith("convtime ")) {
    String args = cmd.substring(9);
    args.trim();
    const int split = args.indexOf(' ');
    if (split < 0) {
      LOGW("Usage: convtime vbus|vsh|temp <0..7>");
      return;
    }
    const String which = args.substring(0, split);
    String value = args.substring(split + 1);
    value.trim();
    const int val = value.toInt();
    if (val < 0 || val > 7) {
      LOGW("Invalid conversion time index (0-7)");
      return;
    }
    const ConvTime ct = static_cast<ConvTime>(val);
    Status st;
    if (which == "vbus") {
      st = device.setVbusConvTime(ct);
    } else if (which == "vsh" || which == "vshunt") {
      st = device.setVshuntConvTime(ct);
    } else if (which == "temp") {
      st = device.setTempConvTime(ct);
    } else {
      LOGW("Invalid target: %s (use vbus|vsh|temp)", which.c_str());
      return;
    }
    LOGI("setConvTime(%s, %s): %s%s%s",
         which.c_str(), convTimeToStr(ct),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "convtime") {
    const auto& cfg = device.getConfig();
    Serial.printf("Conversion times: VBUS=%s  VSHUNT=%s  TEMP=%s\n",
                  convTimeToStr(cfg.vbusConvTime),
                  convTimeToStr(cfg.vshuntConvTime),
                  convTimeToStr(cfg.vtempConvTime));
    return;
  }

  if (cmd == "averaging") {
    Serial.printf("Averaging: %s samples\n", avgToStr(device.getConfig().averaging));
    return;
  }

  if (cmd.startsWith("averaging ")) {
    const int val = cmd.substring(10).toInt();
    if (val < 0 || val > 7) {
      LOGW("Invalid averaging index (0-7)");
      return;
    }
    auto st = device.setAveraging(static_cast<Averaging>(val));
    LOGI("setAveraging(%s): %s%s%s",
         avgToStr(static_cast<Averaging>(val)),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "adcrange") {
    Serial.printf("ADC range: %s\n", adcRangeToStr(device.getConfig().adcRange));
    return;
  }

  if (cmd.startsWith("adcrange ")) {
    const int val = cmd.substring(9).toInt();
    if (val < 0 || val > 1) {
      LOGW("Invalid ADC range (0 or 1)");
      return;
    }
    auto st = device.setAdcRange(static_cast<AdcRange>(val));
    LOGI("setAdcRange(%s): %s%s%s",
         adcRangeToStr(static_cast<AdcRange>(val)),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "cal") {
    const auto& cfg = device.getConfig();
    Serial.printf("Calibration: Rshunt=%.6f ohm  MaxCurrent=%.6f A  CURRENT_LSB=%.9f A\n",
                  cfg.shuntResistanceOhm,
                  cfg.maxExpectedCurrentA,
                  device.currentLsb());
    return;
  }

  if (cmd.startsWith("cal ")) {
    String args = cmd.substring(4);
    args.trim();
    const int split = args.indexOf(' ');
    if (split < 0) {
      LOGW("Usage: cal <shunt_ohm> <max_current_a>");
      return;
    }

    float shuntOhm = 0.0f;
    float maxCurrentA = 0.0f;
    String shuntTok = args.substring(0, split);
    String maxTok = args.substring(split + 1);
    maxTok.trim();
    if (!parseFloat(shuntTok, shuntOhm) ||
        !parseFloat(maxTok, maxCurrentA) ||
        shuntOhm <= 0.0f || maxCurrentA <= 0.0f) {
      LOGW("Usage: cal <shunt_ohm> <max_current_a>");
      return;
    }

    auto st = device.setCalibration(shuntOhm, maxCurrentA);
    LOGI("setCalibration(%.6f, %.6f): %s%s%s",
         shuntOhm, maxCurrentA,
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "tempco") {
    LOGI("Shunt temp coeff: %u ppm/degC", device.getConfig().shuntTempCoeffPpmC);
    return;
  }

  if (cmd.startsWith("tempco ")) {
    uint32_t ppm = 0;
    if (!parseU32(cmd.substring(7), ppm) || ppm > INA228::cmd::TEMPCO_MAX) {
      LOGW("Usage: tempco <0..16383>");
      return;
    }
    auto st = device.setShuntTempCoeff(static_cast<uint16_t>(ppm));
    LOGI("setShuntTempCoeff(%lu): %s%s%s",
         static_cast<unsigned long>(ppm),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "tempcomp") {
    LOGI("Temperature compensation: %s", log_bool_str(device.getConfig().tempCompEnabled));
    return;
  }

  if (cmd.startsWith("tempcomp ")) {
    const int val = cmd.substring(9).toInt();
    if (val != 0 && val != 1) {
      LOGW("Usage: tempcomp <0|1>");
      return;
    }
    auto st = device.setTempCompensation(val != 0);
    LOGI("setTempCompensation(%s): %s%s%s",
         log_bool_str(val != 0),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "delay") {
    const auto& cfg = device.getConfig();
    LOGI("Conversion delay: %u x 2 ms (%u ms)",
         cfg.convDelayMs2,
         static_cast<unsigned>(cfg.convDelayMs2) * 2u);
    return;
  }

  if (cmd.startsWith("delay ")) {
    uint32_t steps = 0;
    if (!parseU32(cmd.substring(6), steps) || steps > 255u) {
      LOGW("Usage: delay <0..255>");
      return;
    }
    auto st = device.setConversionDelay(static_cast<uint8_t>(steps));
    LOGI("setConversionDelay(%lu): %s%s%s",
         static_cast<unsigned long>(steps),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "settings" || cmd == "cfg") {
    printSettings();
    return;
  }

  if (cmd == "addr") {
    Serial.printf("Target INA228 address: 0x%02X\n", selectedAddress);
    if (device.isInitialized()) {
      Serial.printf("Active driver address: 0x%02X\n", device.getConfig().i2cAddress);
    } else {
      Serial.println("Driver is not initialized");
    }
    return;
  }

  if (cmd.startsWith("addr ")) {
    uint8_t address = 0;
    if (!parseAddressArg(cmd.substring(5), address)) {
      LOGW("Invalid address. Use 0x40-0x4F");
      return;
    }
    selectedAddress = address;
    LOGI("Selected INA228 address set to 0x%02X (run init to apply)", selectedAddress);
    return;
  }

  if (cmd == "init" || cmd.startsWith("init ")) {
    uint8_t address = selectedAddress;
    bool allowAutoDetectFallback = true;
    if (cmd.length() > 4) {
      if (!parseAddressArg(cmd.substring(4), address)) {
        LOGW("Invalid address. Use init 0x40-0x4F");
        return;
      }
      allowAutoDetectFallback = false;
    }

    auto st = initializeDevice(address, allowAutoDetectFallback);
    LOGI("begin(0x%02X): %s%s%s",
         configuredAddress(),
         LOG_COLOR_RESULT(st.ok()),
         errToStr(st.code),
         LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "end") {
    device.end();
    LOGI("Device shut down.");
    return;
  }

  if (cmd == "reset") {
    auto st = device.softReset();
    LOGI("softReset(): %s%s%s", LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "rstacc") {
    auto st = device.resetAccumulators();
    LOGI("resetAccumulators(): %s%s%s", LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  // --- Alert & Diagnostics ---
  if (cmd == "diag") {
    printDiag();
    return;
  }

  if (cmd == "diagraw") {
    uint16_t raw = 0;
    auto st = device.readDiagAlertRaw(raw);
    if (st.ok()) {
      LOGI("DIAG_ALRT raw: 0x%04X", raw);
      LOGW("DIAG_ALRT reads are destructive/status-clearing.");
    } else {
      printStatus(st);
    }
    return;
  }

  if (cmd == "limits") {
    printAlertLimits();
    return;
  }

  if (cmd == "alatch" || cmd == "cnvralert" || cmd == "alslow" || cmd == "apol") {
    INA228::DiagAlert diag{};
    auto st = device.readDiagAlert(diag);
    if (!st.ok()) {
      printStatus(st);
      return;
    }

    if (cmd == "alatch") {
      LOGI("Alert latch: %s", log_bool_str(diag.alatch));
    } else if (cmd == "cnvralert") {
      LOGI("Conversion-ready alert: %s", log_bool_str(diag.cnvr));
    } else if (cmd == "alslow") {
      LOGI("Slow alert: %s", log_bool_str(diag.slowAlert));
    } else {
      LOGI("Alert polarity: %s", diag.apol ? "active-high" : "active-low");
    }
    return;
  }

  if (cmd.startsWith("alatch ")) {
    const int val = cmd.substring(7).toInt();
    if (val != 0 && val != 1) {
      LOGW("Usage: alatch <0|1>");
      return;
    }
    auto st = device.setAlertLatch(val != 0);
    LOGI("setAlertLatch(%s): %s%s%s",
         log_bool_str(val != 0),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd.startsWith("cnvralert ")) {
    const int val = cmd.substring(10).toInt();
    if (val != 0 && val != 1) {
      LOGW("Usage: cnvralert <0|1>");
      return;
    }
    auto st = device.setConversionReadyAlert(val != 0);
    LOGI("setConversionReadyAlert(%s): %s%s%s",
         log_bool_str(val != 0),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd.startsWith("alslow ")) {
    const int val = cmd.substring(7).toInt();
    if (val != 0 && val != 1) {
      LOGW("Usage: alslow <0|1>");
      return;
    }
    auto st = device.setSlowAlert(val != 0);
    LOGI("setSlowAlert(%s): %s%s%s",
         log_bool_str(val != 0),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd.startsWith("apol ")) {
    const int val = cmd.substring(5).toInt();
    if (val != 0 && val != 1) {
      LOGW("Usage: apol <0|1>");
      return;
    }
    auto st = device.setAlertPolarity(val != 0);
    LOGI("setAlertPolarity(%s): %s%s%s",
         log_bool_str(val != 0),
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "sovl") {
    printShuntAlertLimit("SOVL", INA228::cmd::REG_SOVL);
    return;
  }

  if (cmd.startsWith("sovl ")) {
    float value = 0.0f;
    if (!parseFloat(cmd.substring(5), value)) {
      LOGW("Usage: sovl <volts>");
      return;
    }
    auto st = device.setShuntOvervoltageThreshold(value);
    LOGI("setShuntOvervoltageThreshold(%.7f): %s%s%s",
         value, LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "suvl") {
    printShuntAlertLimit("SUVL", INA228::cmd::REG_SUVL);
    return;
  }

  if (cmd.startsWith("suvl ")) {
    float value = 0.0f;
    if (!parseFloat(cmd.substring(5), value)) {
      LOGW("Usage: suvl <volts>");
      return;
    }
    auto st = device.setShuntUndervoltageThreshold(value);
    LOGI("setShuntUndervoltageThreshold(%.7f): %s%s%s",
         value, LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "bovl") {
    printBusAlertLimit("BOVL", INA228::cmd::REG_BOVL);
    return;
  }

  if (cmd.startsWith("bovl ")) {
    float value = 0.0f;
    if (!parseFloat(cmd.substring(5), value)) {
      LOGW("Usage: bovl <volts>");
      return;
    }
    auto st = device.setBusOvervoltageThreshold(value);
    LOGI("setBusOvervoltageThreshold(%.4f): %s%s%s",
         value, LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "buvl") {
    printBusAlertLimit("BUVL", INA228::cmd::REG_BUVL);
    return;
  }

  if (cmd.startsWith("buvl ")) {
    float value = 0.0f;
    if (!parseFloat(cmd.substring(5), value)) {
      LOGW("Usage: buvl <volts>");
      return;
    }
    auto st = device.setBusUndervoltageThreshold(value);
    LOGI("setBusUndervoltageThreshold(%.4f): %s%s%s",
         value, LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "tmplim") {
    printTemperatureAlertLimit();
    return;
  }

  if (cmd.startsWith("tmplim ")) {
    float value = 0.0f;
    if (!parseFloat(cmd.substring(7), value)) {
      LOGW("Usage: tmplim <degC>");
      return;
    }
    auto st = device.setTemperatureOverlimitThreshold(value);
    LOGI("setTemperatureOverlimitThreshold(%.2f): %s%s%s",
         value, LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "pwrlim") {
    printPowerAlertLimit();
    return;
  }

  if (cmd.startsWith("pwrlim ")) {
    float value = 0.0f;
    if (!parseFloat(cmd.substring(7), value)) {
      LOGW("Usage: pwrlim <watts>");
      return;
    }
    auto st = device.setPowerOverlimitThreshold(value);
    LOGI("setPowerOverlimitThreshold(%.6f): %s%s%s",
         value, LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd == "mfgid") {
    uint16_t id = 0;
    auto st = device.readManufacturerId(id);
    if (st.ok()) { LOGI("Manufacturer ID: 0x%04X", id); }
    else { printStatus(st); }
    return;
  }

  if (cmd == "devid") {
    uint16_t id = 0;
    auto st = device.readDeviceId(id);
    if (st.ok()) { LOGI("Device ID: 0x%04X", id); }
    else { printStatus(st); }
    return;
  }

  if (cmd.startsWith("wreg16 ")) {
    String args = cmd.substring(7);
    args.trim();
    const int split = args.indexOf(' ');
    if (split < 0) {
      LOGW("Usage: wreg16 <addr> <val> (diagnostic only)");
      return;
    }
    uint32_t addr = 0;
    uint32_t value = 0;
    if (!parseU32(args.substring(0, split), addr) ||
        !parseU32(args.substring(split + 1), value) ||
        addr > 0xFFu || value > 0xFFFFu) {
      LOGW("Usage: wreg16 <addr> <val> (diagnostic only)");
      return;
    }
    auto st = device.writeRegister16(static_cast<uint8_t>(addr), static_cast<uint16_t>(value));
    printStatus(st);
    return;
  }

  if (cmd.startsWith("reg16 ")) {
    uint32_t addr = 0;
    if (!parseU32(cmd.substring(6), addr) || addr > 0xFFu) {
      LOGW("Usage: reg16 <addr>");
      return;
    }
    uint16_t value = 0;
    auto st = device.readRegister16(static_cast<uint8_t>(addr), value);
    if (!st.ok()) {
      printStatus(st);
      return;
    }
    Serial.printf("  Reg 0x%02lX = 0x%04X (%u)\n",
                  static_cast<unsigned long>(addr),
                  value,
                  value);
    return;
  }

  if (cmd.startsWith("reg24 ")) {
    uint32_t addr = 0;
    if (!parseU32(cmd.substring(6), addr) || addr > 0xFFu) {
      LOGW("Usage: reg24 <addr>");
      return;
    }
    uint32_t value = 0;
    auto st = device.readRegister24(static_cast<uint8_t>(addr), value);
    if (!st.ok()) {
      printStatus(st);
      return;
    }
    Serial.printf("  Reg 0x%02lX = 0x%06lX (%lu)\n",
                  static_cast<unsigned long>(addr),
                  static_cast<unsigned long>(value),
                  static_cast<unsigned long>(value));
    return;
  }

  if (cmd.startsWith("reg40 ")) {
    uint32_t addr = 0;
    if (!parseU32(cmd.substring(6), addr) || addr > 0xFFu) {
      LOGW("Usage: reg40 <addr>");
      return;
    }
    uint64_t value = 0;
    auto st = device.readRegister40(static_cast<uint8_t>(addr), value);
    if (!st.ok()) {
      printStatus(st);
      return;
    }
    Serial.printf("  Reg 0x%02lX = 0x%02lX%08lX\n",
                  static_cast<unsigned long>(addr),
                  static_cast<unsigned long>((value >> 32) & 0xFFu),
                  static_cast<unsigned long>(value & 0xFFFFFFFFu));
    return;
  }

  // --- Diagnostics ---
  if (cmd == "drv") {
    printDriverHealth();
    Mode mode;
    if (device.getMode(mode).ok()) {
      Serial.printf("  Mode: %s\n", modeToStr(mode));
    }
    return;
  }

  if (cmd == "probe") {
    const uint8_t address = configuredAddress();
    LOGI("Probing address 0x%02X (raw, no health tracking; reads DIAG_ALRT)...",
         address);
    LOGW("DIAG_ALRT reads can clear CNVRF and latched evidence.");
    ProbeSnapshot snapshot{};
    auto st = probeAddressRaw(address, snapshot);
    printStatus(st);
    if (st.ok()) {
      printProbeSnapshot(snapshot);
    }
    return;
  }

  if (cmd == "recover") {
    LOGI("Attempting recovery...");
    auto st = device.recover();
    printStatus(st);
    printDriverHealth();
    return;
  }

  if (cmd == "verbose") {
    printVerboseState();
    return;
  }

  if (cmd.startsWith("verbose ")) {
    const int val = cmd.substring(8).toInt();
    verboseMode = (val != 0);
    LOGI("Verbose mode: %s%s%s",
         onOffColor(verboseMode),
         verboseMode ? "ON" : "OFF",
         LOG_COLOR_RESET);
    return;
  }

  if (cmd == "selftest") {
    runSelfTest();
    return;
  }

  if (cmd == "stress_mix") {
    runStressMix(50);
    return;
  }

  if (cmd.startsWith("stress_mix ")) {
    int count = cmd.substring(11).toInt();
    if (count <= 0 || count > MAX_STRESS_COUNT) {
      LOGW("Invalid stress_mix count");
      return;
    }
    runStressMix(count);
    return;
  }

  if (cmd.startsWith("stress")) {
    int count = 10;
    if (cmd.length() > 6) {
      count = cmd.substring(6).toInt();
    }
    if (count <= 0) {
      LOGW("Invalid stress count");
      return;
    }
    if (count > MAX_STRESS_COUNT) {
      LOGW("Invalid stress count");
      return;
    }
    runStress(count);
    return;
  }

  LOGW("Unknown command: %s", cmd.c_str());
}

// ============================================================================
// Setup and Loop
// ============================================================================

void setup() {
  log_begin(115200);

  LOGI("=== INA228 Bringup Example ===");

  if (!board::initI2c()) {
    LOGE("Failed to initialize I2C");
    return;
  }
  LOGI("I2C initialized (SDA=%d, SCL=%d)", board::I2C_SDA, board::I2C_SCL);

  bus_diag::scan();
  scanIna228Addresses();

  INA228::Status st = initializeDevice(selectedAddress, true);
  if (!st.ok()) {
    LOGE("Failed to initialize device");
    printStatus(st);
    LOGW("Device remains uninitialized. Use scanina, addr <0x40-0x4F>, and init [addr].");
  } else {
    LOGI("Device initialized successfully at 0x%02X", configuredAddress());
    LOGI("CURRENT_LSB: %.9f A", device.currentLsb());
    LOGI("Conv time: ~%lu ms",
         static_cast<unsigned long>(device.estimateConversionTimeMs()));

    printDriverHealth();
  }
  printHelp();
  cli::printPrompt();
}

void loop() {
  device.tick(millis());

  static char inputBuffer[CLI_MAX_LINE_LEN] = {};
  static size_t inputLen = 0;
  static bool inputOverflow = false;

  uint8_t processed = 0;
  while (Serial.available() && processed < CLI_MAX_BYTES_PER_LOOP) {
    ++processed;
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (inputOverflow) {
        LOGW("Input line too long");
      } else if (inputLen > 0) {
        inputBuffer[inputLen] = '\0';
        processCommand(String(inputBuffer));
      }
      if (inputOverflow || inputLen > 0) {
        cli::printPrompt();
      }
      inputLen = 0;
      inputBuffer[0] = '\0';
      inputOverflow = false;
    } else {
      if (inputLen + 1U < CLI_MAX_LINE_LEN) {
        inputBuffer[inputLen++] = c;
        inputBuffer[inputLen] = '\0';
      } else {
        inputOverflow = true;
      }
    }
  }
}
