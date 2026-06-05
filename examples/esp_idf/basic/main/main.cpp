/**
 * @file main.cpp
 * @brief Native ESP-IDF INA228 bring-up CLI.
 *
 * The ESP-IDF example intentionally uses native IDF entry, timing, GPIO, I2C,
 * delays, and fixed command buffers. It does not include Arduino headers,
 * Arduino CLI sources, or compatibility facades.
 *
 * This is diagnostic single-owner example glue. Shared-bus or multitask
 * applications need an external bus manager, locking, and stable device handles.
 */

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "INA228/INA228.h"
#include "Ina228IdfI2cTransport.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

static constexpr int I2C_SDA = 8;
static constexpr int I2C_SCL = 9;
static constexpr uint32_t I2C_FREQ_HZ = 400000U;
static constexpr uint16_t I2C_TIMEOUT_MS = 50U;
static constexpr uint8_t DEFAULT_I2C_ADDRESS = 0x40U;
static constexpr uint8_t INA228_ADDR_MIN = 0x40U;
static constexpr uint8_t INA228_ADDR_MAX = 0x4FU;
static constexpr size_t MAX_LINE_LEN = 128U;
static constexpr uint32_t STRESS_PROGRESS_UPDATES = 10U;

static constexpr const char* COLOR_RESET = "\033[0m";
static constexpr const char* COLOR_RED = "\033[31m";
static constexpr const char* COLOR_GREEN = "\033[32m";
static constexpr const char* COLOR_YELLOW = "\033[33m";
static constexpr const char* COLOR_CYAN = "\033[36m";

INA228::INA228 device;
bool verboseMode = false;
uint8_t selectedAddress = DEFAULT_I2C_ADDRESS;

struct ProbeSnapshot {
  uint8_t address = DEFAULT_I2C_ADDRESS;
  uint16_t manufacturerId = 0;
  uint16_t deviceId = 0;
  uint16_t diagAlert = 0;
};

struct HealthSnapshot {
  INA228::DriverState state = INA228::DriverState::UNINIT;
  bool online = false;
  uint8_t consecutiveFailures = 0;
  uint32_t totalSuccess = 0;
  uint32_t totalFailures = 0;

  void capture() {
    state = device.state();
    online = device.isOnline();
    consecutiveFailures = device.consecutiveFailures();
    totalSuccess = device.totalSuccess();
    totalFailures = device.totalFailures();
  }
};

uint32_t nowMs() {
  return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

void sleepMs(uint32_t ms) {
  vTaskDelay(pdMS_TO_TICKS(ms == 0U ? 1U : ms));
}

const char* boolStr(bool value) {
  return value ? "yes" : "no";
}

void trimInPlace(char* text) {
  if (text == nullptr) {
    return;
  }
  char* start = text;
  while (*start != '\0' && std::isspace(static_cast<unsigned char>(*start))) {
    ++start;
  }
  if (start != text) {
    std::memmove(text, start, std::strlen(start) + 1U);
  }
  size_t len = std::strlen(text);
  while (len > 0U && std::isspace(static_cast<unsigned char>(text[len - 1U]))) {
    text[--len] = '\0';
  }
}

bool startsWith(const char* text, const char* prefix) {
  return std::strncmp(text, prefix, std::strlen(prefix)) == 0;
}

const char* argAfter(const char* text, const char* prefix) {
  if (!startsWith(text, prefix)) {
    return nullptr;
  }
  return text + std::strlen(prefix);
}

bool splitTwoArgs(const char* input, char* first, size_t firstLen, char* second, size_t secondLen) {
  if (input == nullptr || first == nullptr || second == nullptr || firstLen == 0U || secondLen == 0U) {
    return false;
  }
  char copy[MAX_LINE_LEN] = {};
  std::strncpy(copy, input, sizeof(copy) - 1U);
  trimInPlace(copy);
  char* split = std::strchr(copy, ' ');
  if (split == nullptr) {
    return false;
  }
  *split = '\0';
  char* tail = split + 1;
  trimInPlace(copy);
  trimInPlace(tail);
  if (copy[0] == '\0' || tail[0] == '\0') {
    return false;
  }
  std::strncpy(first, copy, firstLen - 1U);
  std::strncpy(second, tail, secondLen - 1U);
  first[firstLen - 1U] = '\0';
  second[secondLen - 1U] = '\0';
  return true;
}

bool parseU32(const char* token, uint32_t& out) {
  if (token == nullptr || token[0] == '\0') {
    return false;
  }
  char* end = nullptr;
  const unsigned long value = std::strtoul(token, &end, 0);
  if (end == token || *end != '\0') {
    return false;
  }
  out = static_cast<uint32_t>(value);
  return true;
}

bool parseI32(const char* token, int32_t& out) {
  if (token == nullptr || token[0] == '\0') {
    return false;
  }
  char* end = nullptr;
  const long value = std::strtol(token, &end, 0);
  if (end == token || *end != '\0') {
    return false;
  }
  out = static_cast<int32_t>(value);
  return true;
}

bool parseFloatArg(const char* token, float& out) {
  if (token == nullptr || token[0] == '\0') {
    return false;
  }
  char* end = nullptr;
  const float value = std::strtof(token, &end);
  if (end == token || *end != '\0') {
    return false;
  }
  out = value;
  return true;
}

bool parseBool01(const char* token, bool& out) {
  int32_t value = 0;
  if (!parseI32(token, value) || (value != 0 && value != 1)) {
    return false;
  }
  out = (value != 0);
  return true;
}

bool isValidIna228Address(uint8_t address) {
  return address >= INA228_ADDR_MIN && address <= INA228_ADDR_MAX;
}

bool parseAddressArg(const char* text, uint8_t& outAddress) {
  uint32_t value = 0;
  if (!parseU32(text, value) || value > 0xFFU) {
    return false;
  }
  const uint8_t address = static_cast<uint8_t>(value);
  if (!isValidIna228Address(address)) {
    return false;
  }
  outAddress = address;
  return true;
}

const char* errToStr(INA228::Err err) {
  using INA228::Err;
  switch (err) {
    case Err::OK: return "OK";
    case Err::NOT_INITIALIZED: return "NOT_INITIALIZED";
    case Err::INVALID_CONFIG: return "INVALID_CONFIG";
    case Err::I2C_ERROR: return "I2C_ERROR";
    case Err::TIMEOUT: return "TIMEOUT";
    case Err::INVALID_PARAM: return "INVALID_PARAM";
    case Err::DEVICE_NOT_FOUND: return "DEVICE_NOT_FOUND";
    case Err::DEVICE_ID_MISMATCH: return "DEVICE_ID_MISMATCH";
    case Err::MEMORY_ERROR: return "MEMORY_ERROR";
    case Err::MEASUREMENT_NOT_READY: return "MEASUREMENT_NOT_READY";
    case Err::MATH_OVERFLOW: return "MATH_OVERFLOW";
    case Err::BUSY: return "BUSY";
    case Err::IN_PROGRESS: return "IN_PROGRESS";
    case Err::I2C_NACK_ADDR: return "I2C_NACK_ADDR";
    case Err::I2C_NACK_DATA: return "I2C_NACK_DATA";
    case Err::I2C_TIMEOUT: return "I2C_TIMEOUT";
    case Err::I2C_BUS: return "I2C_BUS";
    case Err::ACCUMULATION_INVALID: return "ACCUMULATION_INVALID";
    case Err::ACCUMULATION_OVERFLOW: return "ACCUMULATION_OVERFLOW";
    case Err::HARDWARE_DIRTY: return "HARDWARE_DIRTY";
    default: return "UNKNOWN";
  }
}

const char* stateToStr(INA228::DriverState state) {
  using INA228::DriverState;
  switch (state) {
    case DriverState::UNINIT: return "UNINIT";
    case DriverState::READY: return "READY";
    case DriverState::DEGRADED: return "DEGRADED";
    case DriverState::OFFLINE: return "OFFLINE";
    default: return "UNKNOWN";
  }
}

const char* modeToStr(INA228::Mode mode) {
  using INA228::Mode;
  switch (mode) {
    case Mode::SHUTDOWN: return "SHUTDOWN";
    case Mode::TRIG_BUS: return "TRIG_BUS";
    case Mode::TRIG_SHUNT: return "TRIG_SHUNT";
    case Mode::TRIG_SHUNT_BUS: return "TRIG_SHUNT_BUS";
    case Mode::TRIG_TEMP: return "TRIG_TEMP";
    case Mode::TRIG_TEMP_BUS: return "TRIG_TEMP_BUS";
    case Mode::TRIG_TEMP_SHUNT: return "TRIG_TEMP_SHUNT";
    case Mode::TRIG_ALL: return "TRIG_ALL";
    case Mode::SHUTDOWN2: return "SHUTDOWN2";
    case Mode::CONT_BUS: return "CONT_BUS";
    case Mode::CONT_SHUNT: return "CONT_SHUNT";
    case Mode::CONT_SHUNT_BUS: return "CONT_SHUNT_BUS";
    case Mode::CONT_TEMP: return "CONT_TEMP";
    case Mode::CONT_TEMP_BUS: return "CONT_TEMP_BUS";
    case Mode::CONT_TEMP_SHUNT: return "CONT_TEMP_SHUNT";
    case Mode::CONT_ALL: return "CONT_ALL";
    default: return "UNKNOWN";
  }
}

bool modeIncludesConversion(INA228::Mode mode) {
  return (static_cast<uint8_t>(mode) & 0x07U) != 0U;
}

const char* convTimeToStr(INA228::ConvTime ct) {
  using INA228::ConvTime;
  switch (ct) {
    case ConvTime::US_50: return "50us";
    case ConvTime::US_84: return "84us";
    case ConvTime::US_150: return "150us";
    case ConvTime::US_280: return "280us";
    case ConvTime::US_540: return "540us";
    case ConvTime::US_1052: return "1052us";
    case ConvTime::US_2074: return "2074us";
    case ConvTime::US_4120: return "4120us";
    default: return "UNKNOWN";
  }
}

const char* avgToStr(INA228::Averaging avg) {
  using INA228::Averaging;
  switch (avg) {
    case Averaging::AVG_1: return "1";
    case Averaging::AVG_4: return "4";
    case Averaging::AVG_16: return "16";
    case Averaging::AVG_64: return "64";
    case Averaging::AVG_128: return "128";
    case Averaging::AVG_256: return "256";
    case Averaging::AVG_512: return "512";
    case Averaging::AVG_1024: return "1024";
    default: return "UNKNOWN";
  }
}

const char* adcRangeToStr(INA228::AdcRange range) {
  return (range == INA228::AdcRange::MV_40_96) ? "+/-40.96mV" : "+/-163.84mV";
}

uint8_t configuredAddress() {
  return device.isInitialized() ? device.getConfig().i2cAddress : selectedAddress;
}

void printStatus(const INA228::Status& st) {
  std::printf("  Status: %s%s%s (code=%u, detail=%ld)\n",
              st.ok() ? COLOR_GREEN : COLOR_RED,
              errToStr(st.code),
              COLOR_RESET,
              static_cast<unsigned>(st.code),
              static_cast<long>(st.detail));
  if (st.msg != nullptr && st.msg[0] != '\0') {
    std::printf("  Message: %s%s%s\n", COLOR_YELLOW, st.msg, COLOR_RESET);
  }
}

INA228::Config makeExampleConfig(uint8_t address) {
  (void)ina228IdfSelectDeviceAddress(address);
  INA228::Config cfg;
  cfg.i2cWrite = ina228IdfI2cWrite;
  cfg.i2cWriteRead = ina228IdfI2cWriteRead;
  cfg.i2cUser = &ina228IdfTransportContext();
  cfg.nowMs = ina228IdfNowMs;
  cfg.i2cAddress = address;
  cfg.i2cTimeoutMs = I2C_TIMEOUT_MS;
  cfg.mode = INA228::Mode::CONT_ALL;
  cfg.shuntResistanceOhm = 0.015f;
  cfg.maxExpectedCurrentA = 10.0f;
  cfg.offlineThreshold = 5;
  return cfg;
}

INA228::Status readRegister16AtAddress(uint8_t address, uint8_t reg, uint16_t& value) {
  if (!isValidIna228Address(address)) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "Address must be 0x40-0x4F",
                                 static_cast<int32_t>(address));
  }
  uint8_t tx = reg;
  uint8_t rx[2] = {};
  INA228::Status st = ina228IdfI2cWriteReadAt(address, &tx, 1U, rx, sizeof(rx),
                                              I2C_TIMEOUT_MS, &ina228IdfTransportContext());
  if (!st.ok()) {
    return st;
  }
  value = static_cast<uint16_t>((static_cast<uint16_t>(rx[0]) << 8) | rx[1]);
  return INA228::Status::Ok();
}

INA228::Status probeAddressRaw(uint8_t address, ProbeSnapshot& out) {
  out = {};
  out.address = address;
  INA228::Status st = ina228IdfProbeAddress(address, I2C_TIMEOUT_MS);
  if (!st.ok()) {
    return st;
  }
  st = readRegister16AtAddress(address, INA228::cmd::REG_MANUFACTURER_ID, out.manufacturerId);
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
  std::printf("  Address: 0x%02X\n", snapshot.address);
  std::printf("  Manufacturer ID: 0x%04X\n", snapshot.manufacturerId);
  std::printf("  Device ID: 0x%04X\n", snapshot.deviceId);
  std::printf("  DIAG_ALRT: 0x%04X\n", snapshot.diagAlert);
  std::printf("  MEMSTAT: %s\n",
              ((snapshot.diagAlert & INA228::cmd::DIAG_MEMSTAT) != 0U) ? "OK" : "FAIL");
}

void scanBus() {
  std::printf("=== I2C Scan ===\n");
  uint32_t found = 0;
  for (uint8_t address = 0x08U; address <= 0x77U; ++address) {
    INA228::Status st = ina228IdfProbeAddress(address, I2C_TIMEOUT_MS);
    if (st.ok()) {
      std::printf("  0x%02X%s\n", address,
                  (address >= INA228_ADDR_MIN && address <= INA228_ADDR_MAX) ? " (INA228 range)" : "");
      ++found;
    }
  }
  std::printf("Found %lu device(s)\n", static_cast<unsigned long>(found));
}

void scanIna228Addresses() {
  std::printf("=== INA228 Address Probe (0x40-0x4F) ===\n");
  std::printf("Note: INA228 probes read DIAG_ALRT for MEMSTAT and can clear "
              "CNVRF/latched diagnostic evidence.\n");
  uint8_t healthyCount = 0;
  for (uint8_t address = INA228_ADDR_MIN; address <= INA228_ADDR_MAX; ++address) {
    ProbeSnapshot snapshot;
    INA228::Status st = probeAddressRaw(address, snapshot);
    std::printf("  0x%02X: ", address);
    if (st.ok()) {
      ++healthyCount;
      std::printf("%sINA228%s (MFG=0x%04X DEV=0x%04X MEMSTAT=OK)\n",
                  COLOR_GREEN, COLOR_RESET, snapshot.manufacturerId, snapshot.deviceId);
    } else if (st.code == INA228::Err::I2C_NACK_ADDR) {
      std::printf("--\n");
    } else if (st.code == INA228::Err::DEVICE_ID_MISMATCH) {
      std::printf("%sID mismatch%s (MFG=0x%04X DEV=0x%04X)\n",
                  COLOR_YELLOW, COLOR_RESET, snapshot.manufacturerId, snapshot.deviceId);
    } else if (st.code == INA228::Err::MEMORY_ERROR) {
      std::printf("%sMEMSTAT fault%s (DIAG_ALRT=0x%04X)\n",
                  COLOR_RED, COLOR_RESET, snapshot.diagAlert);
    } else {
      std::printf("%s%s%s\n", COLOR_RED, errToStr(st.code), COLOR_RESET);
    }
  }
  std::printf("  Healthy INA228 devices: %u\n", healthyCount);
}

uint8_t detectHealthyIna228Addresses(ProbeSnapshot* matches, uint8_t maxMatches) {
  uint8_t count = 0;
  for (uint8_t address = INA228_ADDR_MIN; address <= INA228_ADDR_MAX; ++address) {
    ProbeSnapshot snapshot;
    INA228::Status st = probeAddressRaw(address, snapshot);
    if (st.ok()) {
      if (count < maxMatches) {
        matches[count] = snapshot;
      }
      ++count;
    }
  }
  return count;
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
  const uint8_t count = detectHealthyIna228Addresses(matches, 16U);
  if (count == 1U && matches[0].address != address) {
    std::printf("Configured address 0x%02X failed; detected INA228 at 0x%02X\n",
                address, matches[0].address);
    selectedAddress = matches[0].address;
    st = device.begin(makeExampleConfig(matches[0].address));
    if (st.ok()) {
      selectedAddress = matches[0].address;
    }
  } else if (count > 1U) {
    std::printf("Multiple INA228 devices detected; choose one with init <addr>\n");
  } else {
    std::printf("No healthy INA228 detected on addresses 0x40-0x4F\n");
  }
  return st;
}

void printHealthDiff(const HealthSnapshot& before, const HealthSnapshot& after) {
  bool changed = false;
  if (before.state != after.state) {
    std::printf("  State: %s -> %s\n", stateToStr(before.state), stateToStr(after.state));
    changed = true;
  }
  if (before.online != after.online) {
    std::printf("  Online: %s -> %s\n", before.online ? "YES" : "NO",
                after.online ? "YES" : "NO");
    changed = true;
  }
  if (before.consecutiveFailures != after.consecutiveFailures) {
    std::printf("  ConsecFail: %u -> %u\n", before.consecutiveFailures,
                after.consecutiveFailures);
    changed = true;
  }
  if (before.totalSuccess != after.totalSuccess) {
    std::printf("  TotalOK: %lu -> %lu (+%lu)\n",
                static_cast<unsigned long>(before.totalSuccess),
                static_cast<unsigned long>(after.totalSuccess),
                static_cast<unsigned long>(after.totalSuccess - before.totalSuccess));
    changed = true;
  }
  if (before.totalFailures != after.totalFailures) {
    std::printf("  TotalFail: %lu -> %lu (+%lu)\n",
                static_cast<unsigned long>(before.totalFailures),
                static_cast<unsigned long>(after.totalFailures),
                static_cast<unsigned long>(after.totalFailures - before.totalFailures));
    changed = true;
  }
  if (!changed) {
    std::printf("  (no health changes)\n");
  }
}

void printDriverHealth() {
  const uint32_t now = nowMs();
  const uint32_t ok = device.totalSuccess();
  const uint32_t fail = device.totalFailures();
  const uint32_t total = ok + fail;
  const float successRate = total > 0U ? (100.0f * static_cast<float>(ok)) /
                                             static_cast<float>(total)
                                       : 0.0f;
  const INA228::Status lastErr = device.lastError();
  std::printf("=== Driver Health ===\n");
  std::printf("  Configured address: 0x%02X\n", configuredAddress());
  std::printf("  State: %s\n", stateToStr(device.state()));
  std::printf("  Online: %s\n", boolStr(device.isOnline()));
  std::printf("  Consecutive failures: %u\n", device.consecutiveFailures());
  std::printf("  Total success: %lu\n", static_cast<unsigned long>(ok));
  std::printf("  Total failures: %lu\n", static_cast<unsigned long>(fail));
  std::printf("  Success rate: %.1f%%\n", successRate);
  if (device.lastOkMs() > 0U) {
    std::printf("  Last OK: %lu ms ago (at %lu ms)\n",
                static_cast<unsigned long>(now - device.lastOkMs()),
                static_cast<unsigned long>(device.lastOkMs()));
  } else {
    std::printf("  Last OK: never\n");
  }
  if (device.lastErrorMs() > 0U) {
    std::printf("  Last error: %lu ms ago (at %lu ms)\n",
                static_cast<unsigned long>(now - device.lastErrorMs()),
                static_cast<unsigned long>(device.lastErrorMs()));
  } else {
    std::printf("  Last error: never\n");
  }
  if (!lastErr.ok()) {
    std::printf("  Error code: %s\n", errToStr(lastErr.code));
    std::printf("  Error detail: %ld\n", static_cast<long>(lastErr.detail));
    if (lastErr.msg != nullptr && lastErr.msg[0] != '\0') {
      std::printf("  Error msg: %s\n", lastErr.msg);
    }
  }
}

float shuntLimitToMv(uint16_t raw) {
  const double lsb = (device.getConfig().adcRange == INA228::AdcRange::MV_40_96)
                         ? INA228::cmd::SHUNT_THRESHOLD_LSB_RANGE1
                         : INA228::cmd::SHUNT_THRESHOLD_LSB_RANGE0;
  return static_cast<float>(static_cast<int16_t>(raw) * lsb * 1000.0);
}

float busLimitToV(uint16_t raw) {
  return static_cast<float>((raw & 0x7FFFU) * INA228::cmd::BUS_THRESHOLD_LSB);
}

float tempLimitToC(uint16_t raw) {
  return static_cast<float>(static_cast<int16_t>(raw) * INA228::cmd::TEMP_LSB);
}

double powerLimitToW(uint16_t raw) {
  const float currentLsb = device.currentLsb();
  if (currentLsb <= 0.0f) {
    return 0.0;
  }
  return static_cast<double>(raw) * 256.0 * INA228::cmd::POWER_COEFF *
         static_cast<double>(currentLsb);
}

void printMeasurement() {
  INA228::Measurement m;
  INA228::Status st = device.readMeasurement(m);
  if (!st.ok()) {
    printStatus(st);
    return;
  }
  std::printf("  Vbus:    %.4f V\n", m.busVoltageV);
  std::printf("  Vshunt:  %.7f V\n", m.shuntVoltageV);
  std::printf("  Temp:    %.2f C\n", m.temperatureC);
  std::printf("  Current: %.6f A\n", m.currentA);
  std::printf("  Power:   %.6f W\n", m.powerW);
  std::printf("  Energy:  %.9f J\n", m.energyJ);
  std::printf("  Charge:  %.9f C\n", m.chargeC);
  std::printf("  Accum valid: energy=%s charge=%s\n",
              boolStr(m.energyValid), boolStr(m.chargeValid));
  std::printf("  Accum overflow: ENERGYOF=%s CHARGEOF=%s MATHOF=%s\n",
              boolStr(m.energyOverflow), boolStr(m.chargeOverflow),
              boolStr(m.mathOverflow));
  if (m.diagAlertValid) {
    std::printf("  DIAG_ALRT snapshot before accumulator reads: 0x%04X\n",
                m.diagAlertRaw);
  }
}

void printRawSample() {
  INA228::RawSample raw;
  INA228::Status st = device.readRawSample(raw);
  if (!st.ok()) {
    printStatus(st);
    return;
  }
  std::printf("=== Raw Registers ===\n");
  std::printf("  Vshunt: %ld (0x%06lX)\n", static_cast<long>(raw.vshunt),
              static_cast<unsigned long>(raw.vshunt & 0xFFFFF));
  std::printf("  Vbus:   %lu (0x%06lX)\n", static_cast<unsigned long>(raw.vbus),
              static_cast<unsigned long>(raw.vbus & 0xFFFFF));
  std::printf("  Temp:   %d (0x%04X)\n", static_cast<int>(raw.dietemp),
              static_cast<unsigned>(raw.dietemp & 0xFFFF));
  std::printf("  Current:%ld (0x%06lX)\n", static_cast<long>(raw.current),
              static_cast<unsigned long>(raw.current & 0xFFFFF));
  std::printf("  Power:  %lu (0x%06lX)\n", static_cast<unsigned long>(raw.power),
              static_cast<unsigned long>(raw.power));
  std::printf("  Energy: %llu\n", static_cast<unsigned long long>(raw.energy));
  std::printf("  Charge: %lld\n", static_cast<long long>(raw.charge));
  std::printf("  Accum valid: energy=%s charge=%s\n",
              boolStr(raw.energyValid), boolStr(raw.chargeValid));
  std::printf("  Accum overflow: ENERGYOF=%s CHARGEOF=%s MATHOF=%s\n",
              boolStr(raw.energyOverflow), boolStr(raw.chargeOverflow),
              boolStr(raw.mathOverflow));
  if (raw.diagAlertValid) {
    std::printf("  DIAG_ALRT snapshot before accumulator reads: 0x%04X\n",
                raw.diagAlertRaw);
  }
}

void printDiag() {
  INA228::DiagAlert diag;
  INA228::Status st = device.readDiagAlert(diag);
  if (!st.ok()) {
    printStatus(st);
    return;
  }
  std::printf("=== DIAG_ALRT Flags ===\n");
  std::printf("  Note: this read is destructive/status-clearing for CNVRF and "
              "latched diagnostic evidence.\n");
  std::printf("  MEMSTAT:   %s\n", boolStr(diag.memstat));
  std::printf("  CNVRF:     %s\n", boolStr(diag.cnvrf));
  std::printf("  ALATCH:    %s\n", boolStr(diag.alatch));
  std::printf("  CNVR:      %s\n", boolStr(diag.cnvr));
  std::printf("  SLOWALERT: %s\n", boolStr(diag.slowAlert));
  std::printf("  APOL:      %s\n", boolStr(diag.apol));
  std::printf("  ENERGYOF:  %s\n", boolStr(diag.energyOF));
  std::printf("  CHARGEOF:  %s\n", boolStr(diag.chargeOF));
  std::printf("  MATHOF:    %s\n", boolStr(diag.mathOF));
  std::printf("  TMPOL:     %s\n", boolStr(diag.tmpOL));
  std::printf("  SHNTOL:    %s\n", boolStr(diag.shntOL));
  std::printf("  SHNTUL:    %s\n", boolStr(diag.shntUL));
  std::printf("  BUSOL:     %s\n", boolStr(diag.busOL));
  std::printf("  BUSUL:     %s\n", boolStr(diag.busUL));
  std::printf("  POL:       %s\n", boolStr(diag.pOL));
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
  std::printf("=== Alert Limits ===\n");
  std::printf("  SOVL:       0x%04X  %.3f mV\n", sovl, static_cast<double>(shuntLimitToMv(sovl)));
  std::printf("  SUVL:       0x%04X  %.3f mV\n", suvl, static_cast<double>(shuntLimitToMv(suvl)));
  std::printf("  BOVL:       0x%04X  %.4f V\n", bovl, static_cast<double>(busLimitToV(bovl)));
  std::printf("  BUVL:       0x%04X  %.4f V\n", buvl, static_cast<double>(busLimitToV(buvl)));
  std::printf("  TEMP_LIMIT: 0x%04X  %.2f C\n", temp, static_cast<double>(tempLimitToC(temp)));
  if (device.currentLsb() > 0.0f) {
    std::printf("  PWR_LIMIT:  0x%04X  %.6f W\n", power, powerLimitToW(power));
  } else {
    std::printf("  PWR_LIMIT:  0x%04X  requires calibration for W\n", power);
  }
}

void printShuntAlertLimit(const char* label, uint8_t reg) {
  uint16_t raw = 0;
  INA228::Status st = device.readRegister16(reg, raw);
  if (st.ok()) {
    std::printf("%s: 0x%04X  %.3f mV\n", label, raw, static_cast<double>(shuntLimitToMv(raw)));
  } else {
    printStatus(st);
  }
}

void printBusAlertLimit(const char* label, uint8_t reg) {
  uint16_t raw = 0;
  INA228::Status st = device.readRegister16(reg, raw);
  if (st.ok()) {
    std::printf("%s: 0x%04X  %.4f V\n", label, raw, static_cast<double>(busLimitToV(raw)));
  } else {
    printStatus(st);
  }
}

void printTemperatureAlertLimit() {
  uint16_t raw = 0;
  INA228::Status st = device.readRegister16(INA228::cmd::REG_TEMP_LIMIT, raw);
  if (st.ok()) {
    std::printf("TEMP_LIMIT: 0x%04X  %.2f C\n", raw, static_cast<double>(tempLimitToC(raw)));
  } else {
    printStatus(st);
  }
}

void printPowerAlertLimit() {
  uint16_t raw = 0;
  INA228::Status st = device.readRegister16(INA228::cmd::REG_PWR_LIMIT, raw);
  if (!st.ok()) {
    printStatus(st);
    return;
  }
  if (device.currentLsb() > 0.0f) {
    std::printf("PWR_LIMIT: 0x%04X  %.6f W\n", raw, powerLimitToW(raw));
  } else {
    std::printf("PWR_LIMIT: 0x%04X  requires calibration for W\n", raw);
  }
}

void printTimingInfo() {
  bool ready = false;
  INA228::Status st = device.isConversionReady(ready);
  if (!st.ok()) {
    printStatus(st);
    return;
  }
  std::printf("Conversion ready: %s\n", ready ? "YES" : "NO");
  std::printf("Estimated conversion time: %lu us (%lu ms)\n",
              static_cast<unsigned long>(device.estimateConversionTimeUs()),
              static_cast<unsigned long>(device.estimateConversionTimeMs()));
  std::printf("CURRENT_LSB: %.9f A\n", device.currentLsb());
}

void printSettings() {
  INA228::SettingsSnapshot snap;
  INA228::Status st = device.getSettings(snap);
  if (!st.ok()) {
    printStatus(st);
    return;
  }
  std::printf("=== Active Settings ===\n");
  std::printf("  Initialized:       %s\n", boolStr(snap.initialized));
  std::printf("  State:             %s\n", stateToStr(snap.state));
  std::printf("  Address:           0x%02X\n", snap.i2cAddress);
  std::printf("  I2C timeout:       %lu ms\n", static_cast<unsigned long>(snap.i2cTimeoutMs));
  std::printf("  Offline threshold: %u\n", static_cast<unsigned>(snap.offlineThreshold));
  std::printf("  nowMs hook:        %s\n", boolStr(snap.hasNowMsHook));
  std::printf("  Mode:              %s (%u)\n", modeToStr(snap.mode), static_cast<unsigned>(snap.mode));
  std::printf("  VBUSCT/VSHCT/VTCT: %s / %s / %s\n",
              convTimeToStr(snap.vbusConvTime),
              convTimeToStr(snap.vshuntConvTime),
              convTimeToStr(snap.vtempConvTime));
  std::printf("  Averaging:         %s samples\n", avgToStr(snap.averaging));
  std::printf("  ADC range:         %s\n", adcRangeToStr(snap.adcRange));
  std::printf("  Conversion delay:  %u x 2 ms (%u ms)\n",
              snap.convDelayMs2, static_cast<unsigned>(snap.convDelayMs2) * 2U);
  std::printf("  Temp compensation: %s  tempco=%u ppm/degC\n",
              boolStr(snap.tempCompEnabled), snap.shuntTempCoeffPpmC);
  std::printf("  Calibration:       Rshunt=%.6f ohm  MaxCurrent=%.6f A\n",
              snap.shuntResistanceOhm, snap.maxExpectedCurrentA);
  std::printf("  Calibration state: calibrated=%s clamped=%s rangeExceeded=%s\n",
              boolStr(snap.calibrated),
              boolStr(snap.calibrationClamped),
              boolStr(snap.maxCurrentExceedsShuntRange));
  std::printf("  Hardware dirty:    %s mask=0x%08lX%08lX\n",
              boolStr(snap.hardwareDirty),
              static_cast<unsigned long>(snap.dirtyRegisterMask >> 32),
              static_cast<unsigned long>(snap.dirtyRegisterMask & 0xFFFFFFFFULL));
  std::printf("  Dirty cause:       %s detail=%ld\n",
              errToStr(snap.hardwareDirtyCause.code),
              static_cast<long>(snap.hardwareDirtyCause.detail));
  std::printf("  Thresholds dirty:  %s\n", boolStr(snap.thresholdsDirty));
  std::printf("  Triggered state:   pending=%s start=%lu ms\n",
              boolStr(snap.triggeredConversionPending),
              static_cast<unsigned long>(snap.triggeredConversionStartMs));
  std::printf("  Est. conv time:    %lu us\n",
              static_cast<unsigned long>(device.estimateConversionTimeUs()));
  std::printf("  CURRENT_LSB:       %.9f A\n", snap.currentLsb);
  std::printf("  SHUNT_CAL:         0x%04X\n", snap.shuntCal);
}

void printHelpItem(const char* command, const char* description) {
  std::printf("  %s%-36s%s - %s\n", COLOR_CYAN, command, COLOR_RESET, description);
}

void printHelp() {
  std::printf("\n%s=== INA228 CLI Help ===%s\n", COLOR_CYAN, COLOR_RESET);
  std::printf("%sSafety:%s This example does not make 85 V systems safe. Use "
              "qualified design practices, isolation where needed, fusing, "
              "creepage/clearance, shunt power checks, and USB-ground care.\n",
              COLOR_YELLOW, COLOR_RESET);
  std::printf("%sIDF bus model:%s single-owner diagnostic glue; shared buses "
              "need an external manager and lock.\n",
              COLOR_YELLOW, COLOR_RESET);
  std::printf("\n%s[Common]%s\n", COLOR_GREEN, COLOR_RESET);
  printHelpItem("help / ?", "Show this help");
  printHelpItem("version / ver", "Print firmware and library version info");
  printHelpItem("scan", "Scan I2C bus and probe 0x40-0x4F for INA228 IDs");
  printHelpItem("scanina", "Probe INA228 IDs; reads DIAG_ALRT/MEMSTAT");
  printHelpItem("read", "Read all measurements with accumulator validity flags");
  printHelpItem("raw", "Read raw register values with validity flags");
  printHelpItem("timing", "Show conversion timing and calibration info");

  std::printf("\n%s[Measurement]%s\n", COLOR_GREEN, COLOR_RESET);
  printHelpItem("vbus", "Read bus voltage");
  printHelpItem("vshunt", "Read shunt voltage");
  printHelpItem("temp", "Read die temperature");
  printHelpItem("current", "Read current");
  printHelpItem("power", "Read power");
  printHelpItem("energy", "Read accumulated energy (continuous accumulation only)");
  printHelpItem("charge", "Read accumulated charge (continuous accumulation only)");
  printHelpItem("ready", "Check if conversion is ready");
  printHelpItem("trigger [mode]", "Trigger single-shot conversion (0-7)");

  std::printf("\n%s[Configuration]%s\n", COLOR_GREEN, COLOR_RESET);
  printHelpItem("mode [0..15]", "Set or show operating mode");
  printHelpItem("convtime [vbus|vsh|temp <0..7>]", "Set conversion time per channel");
  printHelpItem("averaging [0..7]", "Set averaging count");
  printHelpItem("adcrange [0|1]", "Set shunt ADC range");
  printHelpItem("cal <shunt_ohm> <max_current_a>", "Show or update calibration");
  printHelpItem("tempco [ppm]", "Show or set shunt temp coefficient");
  printHelpItem("tempcomp [0|1]", "Show or enable temp compensation");
  printHelpItem("delay [0..255]", "Show or set conversion delay (2 ms steps)");
  printHelpItem("cfg / settings", "Show active settings");
  printHelpItem("addr [0x40..0x4F]", "Show or set target INA228 address");
  printHelpItem("init [0x40..0x4F]", "Re-initialize device at current or given address");
  printHelpItem("end", "Shutdown driver");
  printHelpItem("reset", "Software reset device");
  printHelpItem("rstacc", "Reset energy/charge accumulators");

  std::printf("\n%s[Alert & Diagnostics]%s\n", COLOR_GREEN, COLOR_RESET);
  printHelpItem("diag", "Read DIAG_ALRT flags (destructive/status-clearing)");
  printHelpItem("diagraw", "Read raw DIAG_ALRT (destructive/status-clearing)");
  printHelpItem("limits", "Read alert limit registers with decoded units");
  printHelpItem("alatch [0|1]", "Show or set alert latch mode");
  printHelpItem("cnvralert [0|1]", "Show or enable conversion-ready alert output");
  printHelpItem("alslow [0|1]", "Show or set slow-alert mode");
  printHelpItem("apol [0|1]", "Show or set alert polarity");
  printHelpItem("sovl [volts]", "Show or set shunt overvoltage threshold");
  printHelpItem("suvl [volts]", "Show or set shunt undervoltage threshold");
  printHelpItem("bovl [volts]", "Show or set bus overvoltage threshold");
  printHelpItem("buvl [volts]", "Show or set bus undervoltage threshold");
  printHelpItem("tmplim [degC]", "Show or set temperature over-limit threshold");
  printHelpItem("pwrlim [watts]", "Show or set power over-limit threshold");
  printHelpItem("mfgid", "Read manufacturer ID (expect 0x5449)");
  printHelpItem("devid", "Read device ID (expect 0x2281)");
  printHelpItem("reg16 <addr>", "Read 16-bit register (diagnostic; may clear flags)");
  printHelpItem("reg24 <addr>", "Read 24-bit register (diagnostic; may clear flags)");
  printHelpItem("reg40 <addr>", "Read 40-bit register (diagnostic; may clear flags)");
  printHelpItem("wreg16 <addr> <val>", "Write 16-bit register (diagnostic only; may desync cached config)");

  std::printf("\n%s[Diagnostics]%s\n", COLOR_GREEN, COLOR_RESET);
  printHelpItem("drv", "Show driver state and health");
  printHelpItem("probe", "Probe device; reads DIAG_ALRT; no health tracking");
  printHelpItem("recover", "Manual recovery attempt");
  printHelpItem("verbose [0|1]", "Enable/disable verbose output");
  printHelpItem("stress [N]", "Run N measurement cycles (default 10)");
  printHelpItem("stress_mix [N]", "Run N mixed-operation cycles (default 50)");
  printHelpItem("selftest", "Run diagnostic self-test; reads DIAG_ALRT");
}

void printVersionInfo() {
  std::printf("=== Version Info ===\n");
  std::printf("  Example firmware build: %s %s\n", __DATE__, __TIME__);
  std::printf("  INA228 library version: %s\n", INA228::VERSION);
  std::printf("  INA228 library full: %s\n", INA228::VERSION_FULL);
  std::printf("  INA228 library build: %s\n", INA228::BUILD_TIMESTAMP);
  std::printf("  INA228 library commit: %s (%s)\n", INA228::GIT_COMMIT, INA228::GIT_STATUS);
}

uint32_t stressProgressStep(uint32_t total) {
  const uint32_t step = total / STRESS_PROGRESS_UPDATES;
  return step == 0U ? 1U : step;
}

void printStressProgress(uint32_t completed, uint32_t total, uint32_t ok, uint32_t fail) {
  const uint32_t step = stressProgressStep(total);
  if (completed != total && (completed % step) != 0U) {
    return;
  }
  const float pct = total > 0U ? (100.0f * static_cast<float>(completed)) /
                                    static_cast<float>(total)
                               : 0.0f;
  std::printf("  Progress: %lu/%lu (%.0f%%, ok=%lu, fail=%lu)\n",
              static_cast<unsigned long>(completed),
              static_cast<unsigned long>(total),
              pct,
              static_cast<unsigned long>(ok),
              static_cast<unsigned long>(fail));
}

void runStress(uint32_t count) {
  const uint32_t start = nowMs();
  HealthSnapshot before;
  before.capture();
  uint32_t ok = 0;
  uint32_t fail = 0;
  INA228::Status firstFailure = INA228::Status::Ok();
  INA228::Status lastFailure = INA228::Status::Ok();
  for (uint32_t i = 0; i < count; ++i) {
    INA228::Measurement m;
    INA228::Status st = device.readMeasurement(m);
    if (st.ok()) {
      ++ok;
      if (verboseMode) {
        std::printf("  %lu: Vbus=%.4f V Current=%.6f A Power=%.6f W\n",
                    static_cast<unsigned long>(i + 1U), m.busVoltageV, m.currentA, m.powerW);
      }
    } else {
      ++fail;
      if (firstFailure.ok()) {
        firstFailure = st;
      }
      lastFailure = st;
    }
    printStressProgress(i + 1U, count, ok, fail);
  }
  const uint32_t elapsed = nowMs() - start;
  HealthSnapshot after;
  after.capture();
  std::printf("=== Stress Summary ===\n");
  std::printf("  Target: %lu\n", static_cast<unsigned long>(count));
  std::printf("  Success: %lu\n", static_cast<unsigned long>(ok));
  std::printf("  Errors: %lu\n", static_cast<unsigned long>(fail));
  std::printf("  Duration: %lu ms\n", static_cast<unsigned long>(elapsed));
  if (elapsed > 0U) {
    std::printf("  Rate: %.2f samples/s\n", (1000.0f * static_cast<float>(count)) /
                                               static_cast<float>(elapsed));
  }
  std::printf("  Health changes:\n");
  printHealthDiff(before, after);
  if (!firstFailure.ok()) {
    std::printf("  First failure:\n");
    printStatus(firstFailure);
    std::printf("  Last failure:\n");
    printStatus(lastFailure);
  }
}

void runStressMix(uint32_t count) {
  const uint32_t start = nowMs();
  HealthSnapshot before;
  before.capture();
  uint32_t ok = 0;
  uint32_t fail = 0;
  INA228::Status firstFailure = INA228::Status::Ok();
  INA228::Status lastFailure = INA228::Status::Ok();
  for (uint32_t i = 0; i < count; ++i) {
    INA228::Status st = INA228::Status::Ok();
    switch (i % 8U) {
      case 0: {
        INA228::Measurement m;
        st = device.readMeasurement(m);
        break;
      }
      case 1: {
        INA228::RawSample raw;
        st = device.readRawSample(raw);
        break;
      }
      case 2: {
        bool ready = false;
        st = device.isConversionReady(ready);
        break;
      }
      case 3: {
        uint16_t raw = 0;
        st = device.readDiagAlertRaw(raw);
        break;
      }
      case 4: {
        uint16_t id = 0;
        st = device.readManufacturerId(id);
        break;
      }
      case 5: {
        uint32_t value = 0;
        st = device.readRegister24(INA228::cmd::REG_VBUS, value);
        break;
      }
      case 6:
        st = device.setAveraging(device.getConfig().averaging);
        break;
      default:
        st = device.setMode(device.getConfig().mode);
        break;
    }
    if (st.ok() || st.inProgress() || st.code == INA228::Err::MEASUREMENT_NOT_READY) {
      ++ok;
    } else {
      ++fail;
      if (firstFailure.ok()) {
        firstFailure = st;
      }
      lastFailure = st;
    }
    device.tick(nowMs());
    printStressProgress(i + 1U, count, ok, fail);
  }
  const uint32_t elapsed = nowMs() - start;
  HealthSnapshot after;
  after.capture();
  std::printf("=== stress_mix summary ===\n");
  std::printf("  Total: ok=%lu fail=%lu (%.2f%%)\n",
              static_cast<unsigned long>(ok),
              static_cast<unsigned long>(fail),
              count > 0U ? (100.0f * static_cast<float>(ok)) / static_cast<float>(count) : 0.0f);
  std::printf("  Duration: %lu ms\n", static_cast<unsigned long>(elapsed));
  if (elapsed > 0U) {
    std::printf("  Rate: %.2f ops/s\n", (1000.0f * static_cast<float>(count)) /
                                           static_cast<float>(elapsed));
  }
  std::printf("  Health changes:\n");
  printHealthDiff(before, after);
  if (!firstFailure.ok()) {
    std::printf("  First failure:\n");
    printStatus(firstFailure);
    std::printf("  Last failure:\n");
    printStatus(lastFailure);
  }
}

struct SelftestStats {
  uint32_t pass = 0;
  uint32_t fail = 0;
  uint32_t skip = 0;
};

void reportSelftest(SelftestStats& stats, const char* name, bool passed, const char* note = "") {
  if (passed) {
    ++stats.pass;
  } else {
    ++stats.fail;
  }
  std::printf("  [%s] %s", passed ? "PASS" : "FAIL", name);
  if (note != nullptr && note[0] != '\0') {
    std::printf(" - %s", note);
  }
  std::printf("\n");
}

void skipSelftest(SelftestStats& stats, const char* name, const char* note) {
  ++stats.skip;
  std::printf("  [SKIP] %s - %s\n", name, note);
}

void runSelfTest() {
  SelftestStats stats;
  std::printf("=== INA228 selftest (diagnostic commands; reads DIAG_ALRT) ===\n");
  std::printf("Note: DIAG_ALRT reads can clear CNVRF and latched evidence.\n");
  ProbeSnapshot snapshot;
  INA228::Status st = probeAddressRaw(configuredAddress(), snapshot);
  if (!st.ok()) {
    reportSelftest(stats, "probe responds", false, errToStr(st.code));
    skipSelftest(stats, "remaining checks", "probe failed");
    std::printf("Selftest result: pass=%lu fail=%lu skip=%lu\n",
                static_cast<unsigned long>(stats.pass),
                static_cast<unsigned long>(stats.fail),
                static_cast<unsigned long>(stats.skip));
    return;
  }
  reportSelftest(stats, "probe responds", true);
  reportSelftest(stats, "manufacturer ID", snapshot.manufacturerId == INA228::cmd::MANUFACTURER_ID);
  reportSelftest(stats, "device ID", snapshot.deviceId == INA228::cmd::DEVICE_ID);
  reportSelftest(stats, "MEMSTAT", (snapshot.diagAlert & INA228::cmd::DIAG_MEMSTAT) != 0U);

  INA228::Mode mode = INA228::Mode::SHUTDOWN;
  st = device.getMode(mode);
  const bool modeOk = st.ok();
  reportSelftest(stats, "getMode", modeOk, modeOk ? "" : errToStr(st.code));

  uint16_t raw16 = 0;
  st = device.readDiagAlertRaw(raw16);
  reportSelftest(stats, "diagraw", st.ok(), st.ok() ? "" : errToStr(st.code));
  bool ready = false;
  st = device.isConversionReady(ready);
  reportSelftest(stats, "ready", st.ok(), st.ok() ? "" : errToStr(st.code));
  INA228::Measurement m;
  st = device.readMeasurement(m);
  reportSelftest(stats, "readMeasurement", st.ok(), st.ok() ? "" : errToStr(st.code));
  st = device.setAveraging(device.getConfig().averaging);
  reportSelftest(stats, "setAveraging(current)", st.ok(), st.ok() ? "" : errToStr(st.code));
  st = device.setMode(device.getConfig().mode);
  const bool modeAccepted = st.ok() || st.inProgress();
  reportSelftest(stats, "setMode(current)", modeAccepted, modeAccepted ? "" : errToStr(st.code));
  INA228::Mode timingMode = INA228::Mode::SHUTDOWN;
  st = device.getMode(timingMode);
  const uint32_t convUs = device.estimateConversionTimeUs();
  if (st.ok()) {
    const bool activeConversion = modeIncludesConversion(timingMode);
    reportSelftest(stats, "estimateConversionTimeUs",
                   activeConversion ? (convUs > 0U) : (convUs == 0U),
                   activeConversion ? "" : "shutdown/no active channels");
  } else {
    skipSelftest(stats, "estimateConversionTimeUs", "mode unavailable");
  }
  st = device.recover();
  reportSelftest(stats, "recover", st.ok(), st.ok() ? "" : errToStr(st.code));
  reportSelftest(stats, "isOnline", device.isOnline());
  std::printf("Selftest result: pass=%lu fail=%lu skip=%lu\n",
              static_cast<unsigned long>(stats.pass),
              static_cast<unsigned long>(stats.fail),
              static_cast<unsigned long>(stats.skip));
}

void handleMeasurementCommand(const char* cmd) {
  if (std::strcmp(cmd, "vbus") == 0) {
    float v = 0.0f;
    INA228::Status st = device.readBusVoltage(v);
    if (st.ok()) std::printf("Vbus: %.4f V\n", v); else printStatus(st);
  } else if (std::strcmp(cmd, "vshunt") == 0) {
    float v = 0.0f;
    INA228::Status st = device.readShuntVoltage(v);
    if (st.ok()) std::printf("Vshunt: %.7f V\n", v); else printStatus(st);
  } else if (std::strcmp(cmd, "temp") == 0) {
    float t = 0.0f;
    INA228::Status st = device.readTemperature(t);
    if (st.ok()) std::printf("Temp: %.2f C\n", t); else printStatus(st);
  } else if (std::strcmp(cmd, "current") == 0) {
    float i = 0.0f;
    INA228::Status st = device.readCurrent(i);
    if (st.ok()) std::printf("Current: %.6f A\n", i); else printStatus(st);
  } else if (std::strcmp(cmd, "power") == 0) {
    float p = 0.0f;
    INA228::Status st = device.readPower(p);
    if (st.ok()) std::printf("Power: %.6f W\n", p); else printStatus(st);
  } else if (std::strcmp(cmd, "energy") == 0) {
    double e = 0.0;
    INA228::Status st = device.readEnergy(e);
    if (st.ok()) std::printf("Energy: %.9f J\n", e); else printStatus(st);
  } else if (std::strcmp(cmd, "charge") == 0) {
    double q = 0.0;
    INA228::Status st = device.readCharge(q);
    if (st.ok()) std::printf("Charge: %.9f C\n", q); else printStatus(st);
  }
}

void processCommand(char* cmd) {
  trimInPlace(cmd);
  if (cmd[0] == '\0') {
    return;
  }
  const char* arg = nullptr;

  if (std::strcmp(cmd, "help") == 0 || std::strcmp(cmd, "?") == 0) {
    printHelp();
  } else if (std::strcmp(cmd, "version") == 0 || std::strcmp(cmd, "ver") == 0) {
    printVersionInfo();
  } else if (std::strcmp(cmd, "scan") == 0) {
    scanBus();
    scanIna228Addresses();
  } else if (std::strcmp(cmd, "scanina") == 0) {
    scanIna228Addresses();
  } else if (std::strcmp(cmd, "read") == 0) {
    printMeasurement();
  } else if (std::strcmp(cmd, "raw") == 0) {
    printRawSample();
  } else if (std::strcmp(cmd, "timing") == 0) {
    printTimingInfo();
  } else if (std::strcmp(cmd, "vbus") == 0 || std::strcmp(cmd, "vshunt") == 0 ||
             std::strcmp(cmd, "temp") == 0 || std::strcmp(cmd, "current") == 0 ||
             std::strcmp(cmd, "power") == 0 || std::strcmp(cmd, "energy") == 0 ||
             std::strcmp(cmd, "charge") == 0) {
    handleMeasurementCommand(cmd);
  } else if (std::strcmp(cmd, "ready") == 0) {
    bool ready = false;
    INA228::Status st = device.isConversionReady(ready);
    if (st.ok()) std::printf("Conversion ready: %s\n", boolStr(ready)); else printStatus(st);
  } else if (std::strcmp(cmd, "trigger") == 0) {
    INA228::Status st = device.triggerConversion(INA228::Mode::TRIG_ALL);
    const bool accepted = st.ok() || st.inProgress();
    std::printf("triggerConversion(TRIG_ALL): %s\n", errToStr(st.code));
    if (!accepted) printStatus(st);
  } else if ((arg = argAfter(cmd, "trigger ")) != nullptr) {
    int32_t value = 0;
    if (!parseI32(arg, value) || value < 0 || value > 7) {
      std::printf("Invalid trigger mode (0-7 for TRIG_* modes)\n");
      return;
    }
    INA228::Status st = device.triggerConversion(static_cast<INA228::Mode>(value));
    std::printf("triggerConversion(%ld): %s\n", static_cast<long>(value), errToStr(st.code));
    if (!(st.ok() || st.inProgress())) printStatus(st);
  } else if (std::strcmp(cmd, "mode") == 0) {
    INA228::Mode mode = INA228::Mode::SHUTDOWN;
    INA228::Status st = device.getMode(mode);
    if (st.ok()) std::printf("Mode: %s (%u)\n", modeToStr(mode), static_cast<unsigned>(mode));
    else printStatus(st);
  } else if ((arg = argAfter(cmd, "mode ")) != nullptr) {
    int32_t value = 0;
    if (!parseI32(arg, value) || value < 0 || value > 15) {
      std::printf("Invalid mode (0-15)\n");
      return;
    }
    INA228::Status st = device.setMode(static_cast<INA228::Mode>(value));
    std::printf("setMode(%ld = %s): %s\n", static_cast<long>(value),
                modeToStr(static_cast<INA228::Mode>(value)), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "convtime") == 0) {
    const INA228::Config& cfg = device.getConfig();
    std::printf("Conversion times: VBUS=%s  VSHUNT=%s  TEMP=%s\n",
                convTimeToStr(cfg.vbusConvTime),
                convTimeToStr(cfg.vshuntConvTime),
                convTimeToStr(cfg.vtempConvTime));
  } else if ((arg = argAfter(cmd, "convtime ")) != nullptr) {
    char which[16] = {};
    char valueText[16] = {};
    uint32_t value = 0;
    if (!splitTwoArgs(arg, which, sizeof(which), valueText, sizeof(valueText)) ||
        !parseU32(valueText, value) || value > 7U) {
      std::printf("Usage: convtime vbus|vsh|temp <0..7>\n");
      return;
    }
    const INA228::ConvTime ct = static_cast<INA228::ConvTime>(value);
    INA228::Status st = INA228::Status::Ok();
    if (std::strcmp(which, "vbus") == 0) {
      st = device.setVbusConvTime(ct);
    } else if (std::strcmp(which, "vsh") == 0 || std::strcmp(which, "vshunt") == 0) {
      st = device.setVshuntConvTime(ct);
    } else if (std::strcmp(which, "temp") == 0) {
      st = device.setTempConvTime(ct);
    } else {
      std::printf("Invalid target: %s (use vbus|vsh|temp)\n", which);
      return;
    }
    std::printf("setConvTime(%s, %s): %s\n", which, convTimeToStr(ct), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "averaging") == 0) {
    std::printf("Averaging: %s samples\n", avgToStr(device.getConfig().averaging));
  } else if ((arg = argAfter(cmd, "averaging ")) != nullptr) {
    uint32_t value = 0;
    if (!parseU32(arg, value) || value > 7U) {
      std::printf("Invalid averaging index (0-7)\n");
      return;
    }
    INA228::Status st = device.setAveraging(static_cast<INA228::Averaging>(value));
    std::printf("setAveraging(%s): %s\n", avgToStr(static_cast<INA228::Averaging>(value)),
                errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "adcrange") == 0) {
    std::printf("ADC range: %s\n", adcRangeToStr(device.getConfig().adcRange));
  } else if ((arg = argAfter(cmd, "adcrange ")) != nullptr) {
    uint32_t value = 0;
    if (!parseU32(arg, value) || value > 1U) {
      std::printf("Invalid ADC range (0 or 1)\n");
      return;
    }
    INA228::Status st = device.setAdcRange(static_cast<INA228::AdcRange>(value));
    std::printf("setAdcRange(%s): %s\n", adcRangeToStr(static_cast<INA228::AdcRange>(value)),
                errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "cal") == 0) {
    const INA228::Config& cfg = device.getConfig();
    std::printf("Calibration: Rshunt=%.6f ohm  MaxCurrent=%.6f A  CURRENT_LSB=%.9f A\n",
                cfg.shuntResistanceOhm, cfg.maxExpectedCurrentA, device.currentLsb());
  } else if ((arg = argAfter(cmd, "cal ")) != nullptr) {
    char shuntText[24] = {};
    char currentText[24] = {};
    float shuntOhm = 0.0f;
    float maxCurrentA = 0.0f;
    if (!splitTwoArgs(arg, shuntText, sizeof(shuntText), currentText, sizeof(currentText)) ||
        !parseFloatArg(shuntText, shuntOhm) ||
        !parseFloatArg(currentText, maxCurrentA) ||
        shuntOhm <= 0.0f || maxCurrentA <= 0.0f) {
      std::printf("Usage: cal <shunt_ohm> <max_current_a>\n");
      return;
    }
    INA228::Status st = device.setCalibration(shuntOhm, maxCurrentA);
    std::printf("setCalibration(%.6f, %.6f): %s\n", shuntOhm, maxCurrentA, errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "tempco") == 0) {
    std::printf("Shunt temp coeff: %u ppm/degC\n", device.getConfig().shuntTempCoeffPpmC);
  } else if ((arg = argAfter(cmd, "tempco ")) != nullptr) {
    uint32_t ppm = 0;
    if (!parseU32(arg, ppm) || ppm > INA228::cmd::TEMPCO_MAX) {
      std::printf("Usage: tempco <0..16383>\n");
      return;
    }
    INA228::Status st = device.setShuntTempCoeff(static_cast<uint16_t>(ppm));
    std::printf("setShuntTempCoeff(%lu): %s\n", static_cast<unsigned long>(ppm), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "tempcomp") == 0) {
    std::printf("Temperature compensation: %s\n", boolStr(device.getConfig().tempCompEnabled));
  } else if ((arg = argAfter(cmd, "tempcomp ")) != nullptr) {
    bool value = false;
    if (!parseBool01(arg, value)) {
      std::printf("Usage: tempcomp <0|1>\n");
      return;
    }
    INA228::Status st = device.setTempCompensation(value);
    std::printf("setTempCompensation(%s): %s\n", boolStr(value), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "delay") == 0) {
    const INA228::Config& cfg = device.getConfig();
    std::printf("Conversion delay: %u x 2 ms (%u ms)\n",
                cfg.convDelayMs2, static_cast<unsigned>(cfg.convDelayMs2) * 2U);
  } else if ((arg = argAfter(cmd, "delay ")) != nullptr) {
    uint32_t steps = 0;
    if (!parseU32(arg, steps) || steps > 255U) {
      std::printf("Usage: delay <0..255>\n");
      return;
    }
    INA228::Status st = device.setConversionDelay(static_cast<uint8_t>(steps));
    std::printf("setConversionDelay(%lu): %s\n", static_cast<unsigned long>(steps), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "cfg") == 0 || std::strcmp(cmd, "settings") == 0) {
    printSettings();
  } else if (std::strcmp(cmd, "addr") == 0) {
    std::printf("Target INA228 address: 0x%02X\n", selectedAddress);
    if (device.isInitialized()) {
      std::printf("Active driver address: 0x%02X\n", device.getConfig().i2cAddress);
    } else {
      std::printf("Driver is not initialized\n");
    }
  } else if ((arg = argAfter(cmd, "addr ")) != nullptr) {
    uint8_t address = 0;
    if (!parseAddressArg(arg, address)) {
      std::printf("Invalid address. Use 0x40-0x4F\n");
      return;
    }
    selectedAddress = address;
    std::printf("Selected INA228 address set to 0x%02X (run init to apply)\n", selectedAddress);
  } else if (std::strcmp(cmd, "init") == 0 || (arg = argAfter(cmd, "init ")) != nullptr) {
    uint8_t address = selectedAddress;
    bool allowFallback = true;
    if (arg != nullptr) {
      if (!parseAddressArg(arg, address)) {
        std::printf("Invalid address. Use init 0x40-0x4F\n");
        return;
      }
      allowFallback = false;
    }
    INA228::Status st = initializeDevice(address, allowFallback);
    std::printf("begin(0x%02X): %s\n", configuredAddress(), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "end") == 0) {
    device.end();
    std::printf("Device shut down.\n");
  } else if (std::strcmp(cmd, "reset") == 0) {
    INA228::Status st = device.softReset();
    std::printf("softReset(): %s\n", errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "rstacc") == 0) {
    INA228::Status st = device.resetAccumulators();
    std::printf("resetAccumulators(): %s\n", errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "diag") == 0) {
    printDiag();
  } else if (std::strcmp(cmd, "diagraw") == 0) {
    uint16_t raw = 0;
    INA228::Status st = device.readDiagAlertRaw(raw);
    if (st.ok()) {
      std::printf("DIAG_ALRT raw: 0x%04X\n", raw);
      std::printf("Note: DIAG_ALRT reads are destructive/status-clearing.\n");
    } else {
      printStatus(st);
    }
  } else if (std::strcmp(cmd, "limits") == 0) {
    printAlertLimits();
  } else if (std::strcmp(cmd, "alatch") == 0 || std::strcmp(cmd, "cnvralert") == 0 ||
             std::strcmp(cmd, "alslow") == 0 || std::strcmp(cmd, "apol") == 0) {
    INA228::DiagAlert diag;
    INA228::Status st = device.readDiagAlert(diag);
    if (!st.ok()) {
      printStatus(st);
      return;
    }
    if (std::strcmp(cmd, "alatch") == 0) std::printf("Alert latch: %s\n", boolStr(diag.alatch));
    else if (std::strcmp(cmd, "cnvralert") == 0) std::printf("Conversion-ready alert: %s\n", boolStr(diag.cnvr));
    else if (std::strcmp(cmd, "alslow") == 0) std::printf("Slow alert: %s\n", boolStr(diag.slowAlert));
    else std::printf("Alert polarity: %s\n", diag.apol ? "active-high" : "active-low");
  } else if ((arg = argAfter(cmd, "alatch ")) != nullptr) {
    bool value = false;
    if (!parseBool01(arg, value)) { std::printf("Usage: alatch <0|1>\n"); return; }
    INA228::Status st = device.setAlertLatch(value);
    std::printf("setAlertLatch(%s): %s\n", boolStr(value), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if ((arg = argAfter(cmd, "cnvralert ")) != nullptr) {
    bool value = false;
    if (!parseBool01(arg, value)) { std::printf("Usage: cnvralert <0|1>\n"); return; }
    INA228::Status st = device.setConversionReadyAlert(value);
    std::printf("setConversionReadyAlert(%s): %s\n", boolStr(value), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if ((arg = argAfter(cmd, "alslow ")) != nullptr) {
    bool value = false;
    if (!parseBool01(arg, value)) { std::printf("Usage: alslow <0|1>\n"); return; }
    INA228::Status st = device.setSlowAlert(value);
    std::printf("setSlowAlert(%s): %s\n", boolStr(value), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if ((arg = argAfter(cmd, "apol ")) != nullptr) {
    bool value = false;
    if (!parseBool01(arg, value)) { std::printf("Usage: apol <0|1>\n"); return; }
    INA228::Status st = device.setAlertPolarity(value);
    std::printf("setAlertPolarity(%s): %s\n", boolStr(value), errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "sovl") == 0) {
    printShuntAlertLimit("SOVL", INA228::cmd::REG_SOVL);
  } else if ((arg = argAfter(cmd, "sovl ")) != nullptr) {
    float value = 0.0f;
    if (!parseFloatArg(arg, value)) { std::printf("Usage: sovl <volts>\n"); return; }
    INA228::Status st = device.setShuntOvervoltageThreshold(value);
    std::printf("setShuntOvervoltageThreshold(%.7f): %s\n", value, errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "suvl") == 0) {
    printShuntAlertLimit("SUVL", INA228::cmd::REG_SUVL);
  } else if ((arg = argAfter(cmd, "suvl ")) != nullptr) {
    float value = 0.0f;
    if (!parseFloatArg(arg, value)) { std::printf("Usage: suvl <volts>\n"); return; }
    INA228::Status st = device.setShuntUndervoltageThreshold(value);
    std::printf("setShuntUndervoltageThreshold(%.7f): %s\n", value, errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "bovl") == 0) {
    printBusAlertLimit("BOVL", INA228::cmd::REG_BOVL);
  } else if ((arg = argAfter(cmd, "bovl ")) != nullptr) {
    float value = 0.0f;
    if (!parseFloatArg(arg, value)) { std::printf("Usage: bovl <volts>\n"); return; }
    INA228::Status st = device.setBusOvervoltageThreshold(value);
    std::printf("setBusOvervoltageThreshold(%.4f): %s\n", value, errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "buvl") == 0) {
    printBusAlertLimit("BUVL", INA228::cmd::REG_BUVL);
  } else if ((arg = argAfter(cmd, "buvl ")) != nullptr) {
    float value = 0.0f;
    if (!parseFloatArg(arg, value)) { std::printf("Usage: buvl <volts>\n"); return; }
    INA228::Status st = device.setBusUndervoltageThreshold(value);
    std::printf("setBusUndervoltageThreshold(%.4f): %s\n", value, errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "tmplim") == 0) {
    printTemperatureAlertLimit();
  } else if ((arg = argAfter(cmd, "tmplim ")) != nullptr) {
    float value = 0.0f;
    if (!parseFloatArg(arg, value)) { std::printf("Usage: tmplim <degC>\n"); return; }
    INA228::Status st = device.setTemperatureOverlimitThreshold(value);
    std::printf("setTemperatureOverlimitThreshold(%.2f): %s\n", value, errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "pwrlim") == 0) {
    printPowerAlertLimit();
  } else if ((arg = argAfter(cmd, "pwrlim ")) != nullptr) {
    float value = 0.0f;
    if (!parseFloatArg(arg, value)) { std::printf("Usage: pwrlim <watts>\n"); return; }
    INA228::Status st = device.setPowerOverlimitThreshold(value);
    std::printf("setPowerOverlimitThreshold(%.6f): %s\n", value, errToStr(st.code));
    if (!st.ok()) printStatus(st);
  } else if (std::strcmp(cmd, "mfgid") == 0) {
    uint16_t id = 0;
    INA228::Status st = device.readManufacturerId(id);
    if (st.ok()) std::printf("Manufacturer ID: 0x%04X\n", id); else printStatus(st);
  } else if (std::strcmp(cmd, "devid") == 0) {
    uint16_t id = 0;
    INA228::Status st = device.readDeviceId(id);
    if (st.ok()) std::printf("Device ID: 0x%04X\n", id); else printStatus(st);
  } else if ((arg = argAfter(cmd, "wreg16 ")) != nullptr) {
    char regText[16] = {};
    char valueText[16] = {};
    uint32_t reg = 0;
    uint32_t value = 0;
    if (!splitTwoArgs(arg, regText, sizeof(regText), valueText, sizeof(valueText)) ||
        !parseU32(regText, reg) || !parseU32(valueText, value) ||
        reg > 0xFFU || value > 0xFFFFU) {
      std::printf("Usage: wreg16 <addr> <val> (diagnostic only)\n");
      return;
    }
    printStatus(device.writeRegister16(static_cast<uint8_t>(reg), static_cast<uint16_t>(value)));
  } else if ((arg = argAfter(cmd, "reg16 ")) != nullptr) {
    uint32_t reg = 0;
    if (!parseU32(arg, reg) || reg > 0xFFU) { std::printf("Usage: reg16 <addr>\n"); return; }
    uint16_t value = 0;
    INA228::Status st = device.readRegister16(static_cast<uint8_t>(reg), value);
    if (st.ok()) std::printf("  Reg 0x%02lX = 0x%04X (%u)\n", static_cast<unsigned long>(reg), value, value);
    else printStatus(st);
  } else if ((arg = argAfter(cmd, "reg24 ")) != nullptr) {
    uint32_t reg = 0;
    if (!parseU32(arg, reg) || reg > 0xFFU) { std::printf("Usage: reg24 <addr>\n"); return; }
    uint32_t value = 0;
    INA228::Status st = device.readRegister24(static_cast<uint8_t>(reg), value);
    if (st.ok()) std::printf("  Reg 0x%02lX = 0x%06lX (%lu)\n", static_cast<unsigned long>(reg), static_cast<unsigned long>(value), static_cast<unsigned long>(value));
    else printStatus(st);
  } else if ((arg = argAfter(cmd, "reg40 ")) != nullptr) {
    uint32_t reg = 0;
    if (!parseU32(arg, reg) || reg > 0xFFU) { std::printf("Usage: reg40 <addr>\n"); return; }
    uint64_t value = 0;
    INA228::Status st = device.readRegister40(static_cast<uint8_t>(reg), value);
    if (st.ok()) std::printf("  Reg 0x%02lX = 0x%02lX%08lX\n", static_cast<unsigned long>(reg), static_cast<unsigned long>((value >> 32) & 0xFFU), static_cast<unsigned long>(value & 0xFFFFFFFFU));
    else printStatus(st);
  } else if (std::strcmp(cmd, "drv") == 0) {
    printDriverHealth();
    INA228::Mode mode = INA228::Mode::SHUTDOWN;
    if (device.getMode(mode).ok()) {
      std::printf("  Mode: %s\n", modeToStr(mode));
    }
  } else if (std::strcmp(cmd, "probe") == 0) {
    const uint8_t address = configuredAddress();
    std::printf("Probing address 0x%02X (raw, no health tracking; reads DIAG_ALRT)...\n",
                address);
    std::printf("Note: DIAG_ALRT reads can clear CNVRF and latched evidence.\n");
    ProbeSnapshot snapshot;
    INA228::Status st = probeAddressRaw(address, snapshot);
    printStatus(st);
    if (st.ok()) printProbeSnapshot(snapshot);
  } else if (std::strcmp(cmd, "recover") == 0) {
    std::printf("Attempting recovery...\n");
    HealthSnapshot before;
    before.capture();
    INA228::Status st = device.recover();
    printStatus(st);
    HealthSnapshot after;
    after.capture();
    std::printf("  Health changes:\n");
    printHealthDiff(before, after);
    printDriverHealth();
  } else if (std::strcmp(cmd, "verbose") == 0) {
    std::printf("Verbose mode: %s\n", verboseMode ? "ON" : "OFF");
  } else if ((arg = argAfter(cmd, "verbose ")) != nullptr) {
    bool value = false;
    if (!parseBool01(arg, value)) { std::printf("Usage: verbose [0|1]\n"); return; }
    verboseMode = value;
    std::printf("Verbose mode: %s\n", verboseMode ? "ON" : "OFF");
  } else if (std::strcmp(cmd, "selftest") == 0) {
    runSelfTest();
  } else if (std::strcmp(cmd, "stress_mix") == 0) {
    runStressMix(50U);
  } else if ((arg = argAfter(cmd, "stress_mix ")) != nullptr) {
    int32_t count = 0;
    if (!parseI32(arg, count) || count <= 0 || count > 100000) {
      std::printf("Invalid stress_mix count\n");
      return;
    }
    runStressMix(static_cast<uint32_t>(count));
  } else if (std::strcmp(cmd, "stress") == 0) {
    runStress(10U);
  } else if ((arg = argAfter(cmd, "stress ")) != nullptr) {
    int32_t count = 0;
    if (!parseI32(arg, count) || count <= 0) {
      std::printf("Invalid stress count\n");
      return;
    }
    runStress(static_cast<uint32_t>(count));
  } else {
    std::printf("Unknown command: %s\n", cmd);
  }
}

bool initExample() {
  std::printf("=== INA228 ESP-IDF Bringup Example ===\n");
  if (!ina228IdfInitI2c(I2C_SDA, I2C_SCL, I2C_FREQ_HZ, I2C_TIMEOUT_MS, selectedAddress)) {
    std::printf("Failed to initialize I2C: %s\n", esp_err_to_name(ina228IdfLastError()));
    return false;
  }
  std::printf("I2C initialized (SDA=%d, SCL=%d)\n", I2C_SDA, I2C_SCL);
  scanBus();
  scanIna228Addresses();

  INA228::Status st = initializeDevice(selectedAddress, true);
  if (!st.ok()) {
    std::printf("Failed to initialize device\n");
    printStatus(st);
    std::printf("Device remains uninitialized. Use scanina, addr <0x40-0x4F>, and init [addr].\n");
    return false;
  }
  std::printf("Device initialized successfully at 0x%02X\n", configuredAddress());
  std::printf("CURRENT_LSB: %.9f A\n", device.currentLsb());
  std::printf("Conv time: ~%lu ms\n", static_cast<unsigned long>(device.estimateConversionTimeMs()));
  printDriverHealth();
  return true;
}

}  // namespace

extern "C" void app_main(void) {
  (void)initExample();
  printHelp();
  std::printf("> ");
  std::fflush(stdout);

  char line[MAX_LINE_LEN] = {};
  while (true) {
    device.tick(nowMs());
    if (std::fgets(line, sizeof(line), stdin) != nullptr) {
      line[sizeof(line) - 1U] = '\0';
      processCommand(line);
      std::printf("> ");
      std::fflush(stdout);
    } else {
      sleepMs(10U);
    }
  }
}
