/// @file main.cpp
/// @brief Basic bringup example for INA228
/// @note This is an EXAMPLE, not part of the library

#include <Arduino.h>
#include <limits>
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

// ============================================================================
// Helper Functions
// ============================================================================

uint32_t exampleNowMs(void*) {
  return millis();
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
  Serial.printf("  Energy: %lld\n", static_cast<long long>(raw.energy));
  Serial.printf("  Charge: %lld\n", static_cast<long long>(raw.charge));
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
  INA228::Mode mode;
  INA228::Status st = device.getMode(mode);
  if (!st.ok()) {
    printStatus(st);
    return;
  }

  Serial.println("=== Active Settings ===");
  Serial.printf("  Mode:           %s (%u)\n", modeToStr(mode), static_cast<unsigned>(mode));
  Serial.printf("  Est. conv time: %lu us\n",
                static_cast<unsigned long>(device.estimateConversionTimeUs()));
  Serial.printf("  CURRENT_LSB:    %.9f A\n", device.currentLsb());
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

  Serial.println("=== INA228 selftest (safe commands) ===");

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
  INA228::Mode mode;
  st = device.getMode(mode);
  reportCheck("getMode", st.ok(), st.ok() ? "" : errToStr(st.code));

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
  const uint32_t convUs = device.estimateConversionTimeUs();
  reportCheck("estimateConversionTimeUs>0", convUs > 0U, "");

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
  auto helpSection = [](const char* title) {
    Serial.printf("\n%s[%s]%s\n", LOG_COLOR_GREEN, title, LOG_COLOR_RESET);
  };
  auto helpItem = [](const char* cmd, const char* desc) {
    Serial.printf("  %s%-32s%s - %s\n", LOG_COLOR_CYAN, cmd, LOG_COLOR_RESET, desc);
  };

  Serial.println();
  Serial.printf("%s=== INA228 CLI Help ===%s\n", LOG_COLOR_CYAN, LOG_COLOR_RESET);

  helpSection("Common");
  helpItem("help / ?", "Show this help");
  helpItem("version / ver", "Print firmware and library version info");
  helpItem("scan", "Scan I2C bus");
  helpItem("read", "Read all measurements");
  helpItem("raw", "Read raw register values");
  helpItem("timing", "Show conversion timing and calibration info");

  helpSection("Measurement");
  helpItem("vbus", "Read bus voltage");
  helpItem("vshunt", "Read shunt voltage");
  helpItem("temp", "Read die temperature");
  helpItem("current", "Read current");
  helpItem("power", "Read power");
  helpItem("energy", "Read accumulated energy");
  helpItem("charge", "Read accumulated charge");
  helpItem("ready", "Check if conversion is ready");
  helpItem("trigger [mode]", "Trigger single-shot conversion (0-7)");

  helpSection("Configuration");
  helpItem("mode [0..15]", "Set or show operating mode");
  helpItem("convtime [vbus|vsh|temp <0..7>]", "Set conversion time per channel");
  helpItem("averaging [0..7]", "Set averaging count");
  helpItem("adcrange [0|1]", "Set shunt ADC range");
  helpItem("cfg / settings", "Show active settings");
  helpItem("init", "Re-initialize device");
  helpItem("end", "Shutdown driver");
  helpItem("reset", "Software reset device");
  helpItem("rstacc", "Reset energy/charge accumulators");

  helpSection("Alert & Diagnostics");
  helpItem("diag", "Read diagnostic/alert flags");
  helpItem("diagraw", "Read raw DIAG_ALRT register");
  helpItem("mfgid", "Read manufacturer ID (expect 0x5449)");
  helpItem("devid", "Read device ID (expect 0x2281)");

  helpSection("Diagnostics");
  helpItem("drv", "Show driver state and health");
  helpItem("probe", "Probe device (no health tracking)");
  helpItem("recover", "Manual recovery attempt");
  helpItem("verbose [0|1]", "Enable/disable verbose output");
  helpItem("stress [N]", "Run N measurement cycles (default 10)");
  helpItem("stress_mix [N]", "Run N mixed-operation cycles (default 50)");
  helpItem("selftest", "Run safe command self-test report");
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
    LOGI("triggerConversion(TRIG_ALL): %s%s%s",
         LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
    return;
  }

  if (cmd.startsWith("trigger ")) {
    const int val = cmd.substring(8).toInt();
    if (val < 0 || val > 7) {
      LOGW("Invalid trigger mode (0-7 for TRIG_* modes)");
      return;
    }
    auto st = device.triggerConversion(static_cast<Mode>(val));
    LOGI("triggerConversion(%d): %s%s%s",
         val, LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
    if (!st.ok()) printStatus(st);
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

  if (cmd == "averaging") {
    LOGI("Use: averaging <0..7> (0=1, 1=4, 2=16, 3=64, 4=128, 5=256, 6=512, 7=1024)");
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
    LOGI("Use: adcrange <0|1> (0=+/-163.84mV, 1=+/-40.96mV)");
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

  if (cmd == "settings" || cmd == "cfg") {
    printSettings();
    return;
  }

  if (cmd == "init") {
    Config cfg;
    cfg.i2cWrite = transport::wireWrite;
    cfg.i2cWriteRead = transport::wireWriteRead;
    cfg.i2cUser = &Wire;
    cfg.nowMs = exampleNowMs;
    cfg.i2cAddress = 0x40;
    cfg.i2cTimeoutMs = board::I2C_TIMEOUT_MS;
    cfg.mode = Mode::CONT_ALL;
    cfg.shuntResistanceOhm = 0.015f;
    cfg.maxExpectedCurrentA = 10.0f;
    cfg.offlineThreshold = 5;
    auto st = device.begin(cfg);
    LOGI("begin(): %s%s%s", LOG_COLOR_RESULT(st.ok()), errToStr(st.code), LOG_COLOR_RESET);
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
    } else {
      printStatus(st);
    }
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
    LOGI("Probing device (no health tracking)...");
    auto st = device.probe();
    printStatus(st);
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
    if (count <= 0 || count > 100000) {
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

  INA228::Config cfg;
  cfg.i2cWrite = transport::wireWrite;
  cfg.i2cWriteRead = transport::wireWriteRead;
  cfg.i2cUser = &Wire;
  cfg.nowMs = exampleNowMs;
  cfg.i2cAddress = 0x40;
  cfg.i2cTimeoutMs = board::I2C_TIMEOUT_MS;
  cfg.mode = INA228::Mode::CONT_ALL;
  cfg.shuntResistanceOhm = 0.015f;
  cfg.maxExpectedCurrentA = 10.0f;
  cfg.offlineThreshold = 5;

  INA228::Status st = device.begin(cfg);
  if (!st.ok()) {
    LOGE("Failed to initialize device");
    printStatus(st);
    return;
  }

  LOGI("Device initialized successfully");
  LOGI("CURRENT_LSB: %.9f A", device.currentLsb());
  LOGI("Conv time: ~%lu ms",
       static_cast<unsigned long>(device.estimateConversionTimeMs()));

  printDriverHealth();
  printHelp();
  Serial.print("> ");
}

void loop() {
  device.tick(millis());

  static String inputBuffer;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        processCommand(inputBuffer);
        inputBuffer = "";
        Serial.print("> ");
      }
    } else {
      inputBuffer += c;
    }
  }
}
