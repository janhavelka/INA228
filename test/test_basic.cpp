/// @file test_basic.cpp
/// @brief Native contract tests for INA228 lifecycle and health behavior.

#include <unity.h>

#include <limits>
#include <type_traits>

#include "Arduino.h"
#include "Wire.h"

SerialClass Serial;
TwoWire Wire;

#include "INA228/INA228.h"
#include "common/I2cTransport.h"

static_assert(std::is_default_constructible<::INA228::INA228>::value,
              "INA228 must remain default constructible");
static_assert(!std::is_copy_constructible<::INA228::INA228>::value,
              "INA228 must not be copy constructible");
static_assert(!std::is_copy_assignable<::INA228::INA228>::value,
              "INA228 must not be copy assignable");
static_assert(!std::is_move_constructible<::INA228::INA228>::value,
              "INA228 must not be move constructible");
static_assert(!std::is_move_assignable<::INA228::INA228>::value,
              "INA228 must not be move assignable");
static_assert(std::is_standard_layout<::INA228::CalibrationPlan>::value,
              "CalibrationPlan must remain a plain fixed-memory contract");
static_assert(std::is_trivially_copyable<::INA228::JobSnapshot>::value,
              "JobSnapshot must remain trivially copyable");
static_assert(std::is_trivially_copyable<::INA228::JobResult>::value,
              "JobResult must remain trivially copyable");
static_assert(std::is_trivially_copyable<::INA228::InstantaneousSample>::value,
              "InstantaneousSample must remain trivially copyable");
static_assert(sizeof(::INA228::INA228) <= 1024,
              "Driver fixed-memory footprint unexpectedly grew above 1 KiB");
static_assert(sizeof(::INA228::JobResult) <= 256,
              "Terminal result fixed-memory footprint unexpectedly grew");

using namespace INA228;

namespace {

enum class TransferKind : uint8_t {
  WRITE,
  WRITE_READ
};

struct TransferEvent {
  TransferKind kind = TransferKind::WRITE;
  uint8_t address = 0;
  uint8_t reg = 0;
  uint32_t timeoutMs = 0;
  size_t txLen = 0;
  size_t rxLen = 0;
};

struct FakeBus {
  uint16_t reg16[64] = {};
  uint32_t reg24[64] = {};
  uint64_t reg40[64] = {};
  uint16_t manufacturerId = cmd::MANUFACTURER_ID;
  uint16_t deviceId = cmd::DEVICE_ID;
  uint16_t diagAlrt = cmd::DIAG_ALRT_RESET;
  bool clearDiagOnRead = false;
  uint16_t diagClearOnReadMask = cmd::DIAG_CLEAR_ON_READ_MASK;
  uint32_t nowMs = 1000;
  uint32_t writeCalls = 0;
  uint32_t readCalls = 0;
  uint8_t lastWriteAddr = 0;
  uint8_t lastReadAddr = 0;
  uint8_t lastWriteReg = 0;
  uint16_t lastWrite16 = 0;
  uint8_t readHistory[256] = {};
  size_t readHistoryCount = 0;
  size_t writeAttemptCount = 0;
  TransferEvent transferHistory[512] = {};
  size_t transferHistoryCount = 0;
  uint8_t writeFailureRegs[8] = {};
  size_t writeFailureCount = 0;
  uint8_t nthWriteFailureRegs[8] = {};
  uint8_t nthWriteFailureMatches[8] = {};
  Status nthWriteFailureStatus[8] = {};
  size_t nthWriteFailureCount = 0;
  uint8_t nthReadFailureRegs[8] = {};
  uint8_t nthReadFailureMatches[8] = {};
  Status nthReadFailureStatus[8] = {};
  size_t nthReadFailureCount = 0;
  uint8_t writeMatchCount[64] = {};
  uint8_t readMatchCount[64] = {};
  bool autoClearAccumulatorReset = false;
  uint8_t configReadOverrideRemaining = 0;
  uint16_t configReadOverrideValue = 0;
  uint8_t applyThenFailWriteReg = 0xFF;
  uint8_t applyThenFailWriteMatch = 0;
  uint8_t successfulWriteDurationReg = 0xFF;
  uint8_t successfulWriteDurationMatch = 0;
  uint32_t advanceNowMsOnWrite = 0;
  uint8_t consumeThenFailReadReg = 0xFF;
  uint8_t consumeThenFailReadMatch = 0;

  int readErrorRemaining = 0;
  int writeErrorRemaining = 0;
  Status readError = Status::Error(Err::I2C_ERROR, "forced read error", -1);
  Status writeError = Status::Error(Err::I2C_ERROR, "forced write error", -2);
};

Status fakeWrite(uint8_t addr, const uint8_t* data, size_t len, uint32_t timeoutMs,
                 void* user) {
  FakeBus* bus = static_cast<FakeBus*>(user);
  bus->writeCalls++;
  bus->lastWriteAddr = addr;
  if (data == nullptr || len == 0) {
    return Status::Error(Err::INVALID_PARAM, "invalid fake write args");
  }
  const uint8_t reg = data[0];
  if (bus->transferHistoryCount <
      sizeof(bus->transferHistory) / sizeof(bus->transferHistory[0])) {
    bus->transferHistory[bus->transferHistoryCount++] =
        TransferEvent{TransferKind::WRITE, addr, reg, timeoutMs, len, 0};
  }
  const uint16_t value = (len == 3) ?
      static_cast<uint16_t>((static_cast<uint16_t>(data[1]) << 8) | data[2]) : 0;
  if (reg < 64) {
    bus->writeMatchCount[reg]++;
  }
  bus->writeAttemptCount++;
  const bool applyThenFail =
      bus->applyThenFailWriteReg == reg &&
      bus->applyThenFailWriteMatch == bus->writeMatchCount[reg];
  for (size_t i = 0; i < bus->nthWriteFailureCount; ++i) {
    if (bus->nthWriteFailureRegs[i] == reg &&
        bus->nthWriteFailureMatches[i] == bus->writeMatchCount[reg]) {
      const Status status = bus->nthWriteFailureStatus[i];
      for (size_t j = i + 1; j < bus->nthWriteFailureCount; ++j) {
        bus->nthWriteFailureRegs[j - 1] = bus->nthWriteFailureRegs[j];
        bus->nthWriteFailureMatches[j - 1] = bus->nthWriteFailureMatches[j];
        bus->nthWriteFailureStatus[j - 1] = bus->nthWriteFailureStatus[j];
      }
      bus->nthWriteFailureCount--;
      return status;
    }
  }
  if (bus->writeFailureCount > 0 && bus->writeFailureRegs[0] == reg) {
    for (size_t i = 1; i < bus->writeFailureCount; ++i) {
      bus->writeFailureRegs[i - 1] = bus->writeFailureRegs[i];
    }
    bus->writeFailureCount--;
    return bus->writeError;
  }
  if (bus->writeErrorRemaining > 0) {
    bus->writeErrorRemaining--;
    return bus->writeError;
  }
  if (len == 3) {
    bus->lastWriteReg = reg;
    bus->lastWrite16 = value;
    if (reg < 64) {
      bus->reg16[reg] = value;
    }
    if (reg == cmd::REG_CONFIG && ((value & cmd::CONFIG_RST) != 0)) {
      for (size_t i = 0; i < 64; ++i) {
        bus->reg16[i] = 0;
        bus->reg24[i] = 0;
        bus->reg40[i] = 0;
      }
      bus->reg16[cmd::REG_ADC_CONFIG] = cmd::ADC_CONFIG_RESET;
      bus->reg16[cmd::REG_SHUNT_CAL] = cmd::SHUNT_CAL_RESET;
      bus->reg16[cmd::REG_SHUNT_TEMPCO] = 0;
      bus->reg16[cmd::REG_SOVL] = cmd::SOVL_RESET;
      bus->reg16[cmd::REG_SUVL] = cmd::SUVL_RESET;
      bus->reg16[cmd::REG_BOVL] = cmd::BOVL_RESET;
      bus->reg16[cmd::REG_BUVL] = cmd::BUVL_RESET;
      bus->reg16[cmd::REG_TEMP_LIMIT] = cmd::TEMP_LIMIT_RESET;
      bus->reg16[cmd::REG_PWR_LIMIT] = cmd::PWR_LIMIT_RESET;
      bus->diagAlrt = cmd::DIAG_ALRT_RESET;
      bus->reg16[cmd::REG_CONFIG] = cmd::CONFIG_RST;
    }
    if (reg == cmd::REG_CONFIG && ((value & cmd::CONFIG_RSTACC) != 0)) {
      bus->reg40[cmd::REG_ENERGY] = 0;
      bus->reg40[cmd::REG_CHARGE] = 0;
      bus->diagAlrt &= ~(cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF |
                         cmd::DIAG_MATHOF | cmd::DIAG_CNVRF);
      if (bus->autoClearAccumulatorReset) {
        bus->reg16[cmd::REG_CONFIG] &= ~cmd::CONFIG_RSTACC;
      }
    }
    if (reg == cmd::REG_DIAG_ALRT) {
      bus->diagAlrt = (bus->diagAlrt & ~cmd::DIAG_CONFIG_MASK) |
                      (value & cmd::DIAG_CONFIG_MASK);
    } else if (reg == cmd::REG_ADC_CONFIG) {
      const uint16_t modeBits = value & cmd::MASK_ADC_MODE;
      const uint16_t shutdownBits =
          static_cast<uint16_t>(Mode::SHUTDOWN) << cmd::BIT_ADC_MODE;
      const uint16_t shutdown2Bits =
          static_cast<uint16_t>(Mode::SHUTDOWN2) << cmd::BIT_ADC_MODE;
      if (modeBits != shutdownBits && modeBits != shutdown2Bits) {
        bus->diagAlrt &= ~cmd::DIAG_CNVRF;
      }
    }
  }
  if (applyThenFail) {
    bus->applyThenFailWriteReg = 0xFF;
    bus->applyThenFailWriteMatch = 0;
    return bus->writeError;
  }
  if (bus->successfulWriteDurationReg == reg &&
      bus->successfulWriteDurationMatch == bus->writeMatchCount[reg]) {
    bus->nowMs += bus->advanceNowMsOnWrite;
    bus->successfulWriteDurationReg = 0xFF;
    bus->successfulWriteDurationMatch = 0;
    bus->advanceNowMsOnWrite = 0;
  }
  return Status::Ok();
}

Status fakeWriteRead(uint8_t addr, const uint8_t* txData, size_t txLen, uint8_t* rxData,
                     size_t rxLen, uint32_t timeoutMs, void* user) {
  FakeBus* bus = static_cast<FakeBus*>(user);
  bus->readCalls++;
  bus->lastReadAddr = addr;
  if (txData == nullptr || txLen == 0 || (rxLen > 0 && rxData == nullptr)) {
    return Status::Error(Err::INVALID_PARAM, "invalid fake write-read args");
  }
  if (bus->readErrorRemaining > 0) {
    bus->readErrorRemaining--;
    return bus->readError;
  }

  const uint8_t reg = txData[0];
  if (bus->transferHistoryCount <
      sizeof(bus->transferHistory) / sizeof(bus->transferHistory[0])) {
    bus->transferHistory[bus->transferHistoryCount++] =
        TransferEvent{TransferKind::WRITE_READ, addr, reg, timeoutMs,
                      txLen, rxLen};
  }
  if (reg < 64) {
    bus->readMatchCount[reg]++;
  }
  const bool consumeThenFail =
      bus->consumeThenFailReadReg == reg &&
      bus->consumeThenFailReadMatch == bus->readMatchCount[reg];
  if (bus->readHistoryCount < sizeof(bus->readHistory)) {
    bus->readHistory[bus->readHistoryCount++] = reg;
  }
  for (size_t i = 0; i < bus->nthReadFailureCount; ++i) {
    if (bus->nthReadFailureRegs[i] == reg &&
        bus->nthReadFailureMatches[i] == bus->readMatchCount[reg]) {
      const Status status = bus->nthReadFailureStatus[i];
      for (size_t j = i + 1; j < bus->nthReadFailureCount; ++j) {
        bus->nthReadFailureRegs[j - 1] = bus->nthReadFailureRegs[j];
        bus->nthReadFailureMatches[j - 1] = bus->nthReadFailureMatches[j];
        bus->nthReadFailureStatus[j - 1] = bus->nthReadFailureStatus[j];
      }
      bus->nthReadFailureCount--;
      return status;
    }
  }
  for (size_t i = 0; i < rxLen; ++i) {
    rxData[i] = 0;
  }

  // Manufacturer ID: 0x5449
  if (reg == cmd::REG_MANUFACTURER_ID && rxLen >= 2) {
    rxData[0] = static_cast<uint8_t>(bus->manufacturerId >> 8);
    rxData[1] = static_cast<uint8_t>(bus->manufacturerId & 0xFF);
  }
  // Device ID: 0x2281
  else if (reg == cmd::REG_DEVICE_ID && rxLen >= 2) {
    rxData[0] = static_cast<uint8_t>(bus->deviceId >> 8);
    rxData[1] = static_cast<uint8_t>(bus->deviceId & 0xFF);
  }
  // DIAG_ALRT: MEMSTAT=1 (bit 0)
  else if (reg == cmd::REG_DIAG_ALRT && rxLen >= 2) {
    const uint16_t diag = bus->diagAlrt;
    rxData[0] = static_cast<uint8_t>(diag >> 8);
    rxData[1] = static_cast<uint8_t>(diag & 0xFF);
    if (bus->clearDiagOnRead) {
      bus->diagAlrt &= ~bus->diagClearOnReadMask;
    }
  } else if (rxLen == 2 && reg < 64) {
    if (reg == cmd::REG_CONFIG && bus->configReadOverrideRemaining > 0) {
      bus->configReadOverrideRemaining--;
      rxData[0] = static_cast<uint8_t>(bus->configReadOverrideValue >> 8);
      rxData[1] = static_cast<uint8_t>(bus->configReadOverrideValue & 0xFF);
      return Status::Ok();
    }
    if (reg == cmd::REG_CONFIG && ((bus->reg16[reg] & cmd::CONFIG_RST) != 0)) {
      bus->reg16[reg] &= ~cmd::CONFIG_RST;
    }
    const uint16_t value = bus->reg16[reg];
    rxData[0] = static_cast<uint8_t>(value >> 8);
    rxData[1] = static_cast<uint8_t>(value & 0xFF);
  } else if (rxLen == 3 && reg < 64) {
    const uint32_t value = bus->reg24[reg] & 0xFFFFFFu;
    rxData[0] = static_cast<uint8_t>((value >> 16) & 0xFF);
    rxData[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    rxData[2] = static_cast<uint8_t>(value & 0xFF);
  } else if (rxLen == 5 && reg < 64) {
    const uint64_t value = bus->reg40[reg] & 0xFFFFFFFFFFULL;
    rxData[0] = static_cast<uint8_t>((value >> 32) & 0xFF);
    rxData[1] = static_cast<uint8_t>((value >> 24) & 0xFF);
    rxData[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    rxData[3] = static_cast<uint8_t>((value >> 8) & 0xFF);
    rxData[4] = static_cast<uint8_t>(value & 0xFF);
    if (reg == cmd::REG_ENERGY) {
      bus->diagAlrt &= ~cmd::DIAG_ENERGYOF;
    } else if (reg == cmd::REG_CHARGE) {
      bus->diagAlrt &= ~cmd::DIAG_CHARGEOF;
    }
  }

  if (consumeThenFail) {
    bus->consumeThenFailReadReg = 0xFF;
    bus->consumeThenFailReadMatch = 0;
    return bus->readError;
  }
  return Status::Ok();
}

uint32_t fakeNowMs(void* user) {
  return static_cast<FakeBus*>(user)->nowMs;
}

Config makeConfig(FakeBus& bus) {
  Config cfg;
  cfg.i2cWrite = fakeWrite;
  cfg.i2cWriteRead = fakeWriteRead;
  cfg.i2cUser = &bus;
  cfg.nowMs = fakeNowMs;
  cfg.timeUser = &bus;
  cfg.i2cTimeoutMs = 10;
  cfg.offlineThreshold = 3;
  cfg.mode = Mode::CONT_ALL;
  return cfg;
}

Config makeCooperativeConfig(FakeBus& bus) {
  Config cfg = makeConfig(bus);
  cfg.i2cAddress = 0x41;
  cfg.i2cTimeoutMs = 20;
  cfg.offlineThreshold = 1;
  cfg.healthPolicy = HealthPolicy::PASSIVE;
  cfg.calibration.shuntMicroOhms = 25000;
  cfg.calibration.mode = CalibrationMode::EXPLICIT_CURRENT_LSB;
  cfg.calibration.maxCurrentMilliAmps = 2500;
  cfg.calibration.currentLsbNanoAmps = 5000;
  return cfg;
}

bool wasRegisterRead(const FakeBus& bus, uint8_t reg) {
  for (size_t i = 0; i < bus.readHistoryCount; ++i) {
    if (bus.readHistory[i] == reg) {
      return true;
    }
  }
  return false;
}

void clearReadHistory(FakeBus& bus) {
  bus.readHistoryCount = 0;
  for (size_t i = 0; i < 64; ++i) {
    bus.readMatchCount[i] = 0;
  }
}

void resetWriteTracking(FakeBus& bus) {
  bus.writeAttemptCount = 0;
  bus.lastWriteReg = 0;
  bus.lastWrite16 = 0;
  for (size_t i = 0; i < 64; ++i) {
    bus.writeMatchCount[i] = 0;
  }
}

void clearTransferHistory(FakeBus& bus) {
  bus.transferHistoryCount = 0;
}

Status pollCooperativeToTerminal(INA228::INA228& dev, FakeBus& bus,
                                 uint8_t budget = 1, uint8_t maxPolls = 80) {
  Status st{Err::IN_PROGRESS, 0, "test poll pending"};
  for (uint8_t poll = 0; poll < maxPolls; ++poll) {
    const uint32_t transfersBefore = bus.readCalls + bus.writeCalls;
    st = dev.pollJob(bus.nowMs, budget);
    const uint32_t used = bus.readCalls + bus.writeCalls - transfersBefore;
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(budget, used);
    if (!st.inProgress()) {
      return st;
    }
    if (used == 0) {
      ++bus.nowMs;
    }
  }
  TEST_FAIL_MESSAGE("cooperative job did not reach a terminal state");
  return Status::Error(Err::TIMEOUT, "test poll limit reached");
}

uint32_t initializeCooperativeDevice(INA228::INA228& dev, FakeBus& bus,
                                     uint32_t requestToken = 1) {
  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startInitialize(requestToken, operationId).ok());
  TEST_ASSERT_NOT_EQUAL(0u, operationId);
  const Status st = pollCooperativeToTerminal(dev, bus);
  TEST_ASSERT_TRUE(st.ok());
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobState::SUCCEEDED),
                          static_cast<uint8_t>(result.job.state));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::CONFIRMED),
                          static_cast<uint8_t>(result.job.effect));
  TEST_ASSERT_EQUAL_UINT32(requestToken, result.job.requestToken);
  return operationId;
}

void queueWriteFailure(FakeBus& bus, uint8_t reg) {
  if (bus.writeFailureCount < sizeof(bus.writeFailureRegs)) {
    bus.writeFailureRegs[bus.writeFailureCount++] = reg;
  }
}

void queueNthWriteFailure(FakeBus& bus, uint8_t reg, uint8_t nthMatch) {
  if (bus.nthWriteFailureCount < sizeof(bus.nthWriteFailureRegs)) {
    const size_t index = bus.nthWriteFailureCount++;
    bus.nthWriteFailureRegs[index] = reg;
    bus.nthWriteFailureMatches[index] = nthMatch;
    bus.nthWriteFailureStatus[index] = bus.writeError;
  }
}

void queueNthReadFailure(FakeBus& bus, uint8_t reg, uint8_t nthMatch) {
  if (bus.nthReadFailureCount < sizeof(bus.nthReadFailureRegs)) {
    const size_t index = bus.nthReadFailureCount++;
    bus.nthReadFailureRegs[index] = reg;
    bus.nthReadFailureMatches[index] = nthMatch;
    bus.nthReadFailureStatus[index] = bus.readError;
  }
}



}  // namespace

void setUp() {
  setMillis(0);
  Wire._clearEndTransmissionResult();
  Wire._clearRequestFromOverride();
}

void tearDown() {}

void loadPositiveMeasurementRegisters(FakeBus& bus) {
  bus.reg24[cmd::REG_VSHUNT] = 0x4BF000;
  bus.reg24[cmd::REG_VBUS] = 0x3C0000;
  bus.reg16[cmd::REG_DIETEMP] = 0x0C80;
  bus.reg24[cmd::REG_CURRENT] = 0x4CCCC0;
  bus.reg24[cmd::REG_POWER] = 0x48000C;
  bus.reg40[cmd::REG_ENERGY] = 0x003F480000ULL;
  bus.reg40[cmd::REG_CHARGE] = 0x0043800000ULL;
}



RawSample sentinelRawSample() {
  RawSample raw{};
  raw.vshunt = -1;
  raw.vbus = 2;
  raw.dietemp = -3;
  raw.current = -4;
  raw.power = 5;
  raw.energy = 6;
  raw.charge = -7;
  raw.energyValid = true;
  raw.chargeValid = true;
  raw.energyOverflow = true;
  raw.chargeOverflow = true;
  raw.mathOverflow = true;
  raw.diagAlertValid = true;
  raw.diagAlertRaw = 0x5AA5;
  return raw;
}

void assertSentinelRawSample(const RawSample& raw) {
  TEST_ASSERT_EQUAL_INT32(-1, raw.vshunt);
  TEST_ASSERT_EQUAL_UINT32(2u, raw.vbus);
  TEST_ASSERT_EQUAL_INT16(-3, raw.dietemp);
  TEST_ASSERT_EQUAL_INT32(-4, raw.current);
  TEST_ASSERT_EQUAL_UINT32(5u, raw.power);
  TEST_ASSERT_EQUAL_UINT64(6u, raw.energy);
  TEST_ASSERT_EQUAL_INT64(-7, raw.charge);
  TEST_ASSERT_TRUE(raw.energyValid);
  TEST_ASSERT_TRUE(raw.chargeValid);
  TEST_ASSERT_TRUE(raw.energyOverflow);
  TEST_ASSERT_TRUE(raw.chargeOverflow);
  TEST_ASSERT_TRUE(raw.mathOverflow);
  TEST_ASSERT_TRUE(raw.diagAlertValid);
  TEST_ASSERT_EQUAL_HEX16(0x5AA5u, raw.diagAlertRaw);
}

void assertPositiveIntegerSample(const IntegerSample& sample) {
  TEST_ASSERT_EQUAL_INT32(97200, sample.shuntMicrovolts);
  TEST_ASSERT_EQUAL_UINT32(48000u, sample.busMillivolts);
  TEST_ASSERT_EQUAL_INT32(25000, sample.dieTemperatureMilliC);
  TEST_ASSERT_EQUAL_INT32(6000, sample.currentMilliamps);
  TEST_ASSERT_EQUAL_UINT32(288001u, sample.powerMilliwatts);
}




// ===========================================================================
// Status tests
// ===========================================================================

void test_status_ok() {
  Status st = Status::Ok();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::OK), static_cast<uint8_t>(st.code));
}

void test_status_error() {
  Status st = Status::Error(Err::I2C_ERROR, "Test error", 42);
  TEST_ASSERT_FALSE(st.ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR), static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_INT32(42, st.detail);
}

void test_status_in_progress() {
  Status st{Err::IN_PROGRESS, 0, "In progress"};
  TEST_ASSERT_FALSE(st.ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::IN_PROGRESS), static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE(st.inProgress());
}

void test_status_is_and_bool_conversion() {
  Status ok = Status::Ok();
  TEST_ASSERT_TRUE(ok.is(Err::OK));
  TEST_ASSERT_TRUE(static_cast<bool>(ok));

  Status st = Status::Error(Err::I2C_TIMEOUT, "timeout", -7);
  TEST_ASSERT_TRUE(st.is(Err::I2C_TIMEOUT));
  TEST_ASSERT_FALSE(st.is(Err::I2C_BUS));
  TEST_ASSERT_FALSE(static_cast<bool>(st));
}

void test_err_name_contract_and_append_values() {
  struct Case {
    Err err;
    const char* name;
  };
  const Case cases[] = {
      {Err::OK, "OK"},
      {Err::NOT_INITIALIZED, "NOT_INITIALIZED"},
      {Err::INVALID_CONFIG, "INVALID_CONFIG"},
      {Err::I2C_ERROR, "I2C_ERROR"},
      {Err::TIMEOUT, "TIMEOUT"},
      {Err::INVALID_PARAM, "INVALID_PARAM"},
      {Err::DEVICE_NOT_FOUND, "DEVICE_NOT_FOUND"},
      {Err::DEVICE_ID_MISMATCH, "DEVICE_ID_MISMATCH"},
      {Err::MEMORY_ERROR, "MEMORY_ERROR"},
      {Err::MEASUREMENT_NOT_READY, "MEASUREMENT_NOT_READY"},
      {Err::MATH_OVERFLOW, "MATH_OVERFLOW"},
      {Err::BUSY, "BUSY"},
      {Err::IN_PROGRESS, "IN_PROGRESS"},
      {Err::I2C_NACK_ADDR, "I2C_NACK_ADDR"},
      {Err::I2C_NACK_DATA, "I2C_NACK_DATA"},
      {Err::I2C_TIMEOUT, "I2C_TIMEOUT"},
      {Err::I2C_BUS, "I2C_BUS"},
      {Err::ACCUMULATION_INVALID, "ACCUMULATION_INVALID"},
      {Err::ACCUMULATION_OVERFLOW, "ACCUMULATION_OVERFLOW"},
      {Err::HARDWARE_DIRTY, "HARDWARE_DIRTY"},
      {Err::I2C_NACK_UNKNOWN_PHASE, "I2C_NACK_UNKNOWN_PHASE"},
  };

  for (const Case& c : cases) {
    TEST_ASSERT_EQUAL_STRING(c.name, errName(c.err));
  }

  TEST_ASSERT_EQUAL_UINT8(13u, static_cast<uint8_t>(Err::I2C_NACK_ADDR));
  TEST_ASSERT_EQUAL_UINT8(14u, static_cast<uint8_t>(Err::I2C_NACK_DATA));
  TEST_ASSERT_EQUAL_UINT8(15u, static_cast<uint8_t>(Err::I2C_TIMEOUT));
  TEST_ASSERT_EQUAL_UINT8(16u, static_cast<uint8_t>(Err::I2C_BUS));
  TEST_ASSERT_EQUAL_UINT8(17u, static_cast<uint8_t>(Err::ACCUMULATION_INVALID));
  TEST_ASSERT_EQUAL_UINT8(18u, static_cast<uint8_t>(Err::ACCUMULATION_OVERFLOW));
  TEST_ASSERT_EQUAL_UINT8(19u, static_cast<uint8_t>(Err::HARDWARE_DIRTY));
  TEST_ASSERT_EQUAL_UINT8(20u, static_cast<uint8_t>(Err::I2C_NACK_UNKNOWN_PHASE));
}

// ===========================================================================
// Config defaults
// ===========================================================================

void test_config_defaults() {
  Config cfg;
  TEST_ASSERT_NULL(cfg.i2cWrite);
  TEST_ASSERT_NULL(cfg.i2cWriteRead);
  TEST_ASSERT_EQUAL_HEX8(0x40, cfg.i2cAddress);
  TEST_ASSERT_EQUAL_UINT32(50, cfg.i2cTimeoutMs);
  TEST_ASSERT_EQUAL_UINT8(5, cfg.offlineThreshold);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::CONT_ALL),
                          static_cast<uint8_t>(cfg.mode));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_1052),
                          static_cast<uint8_t>(cfg.vbusConvTime));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_1052),
                          static_cast<uint8_t>(cfg.vshuntConvTime));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_1052),
                          static_cast<uint8_t>(cfg.vtempConvTime));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Averaging::AVG_1),
                          static_cast<uint8_t>(cfg.averaging));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcRange::MV_163_84),
                          static_cast<uint8_t>(cfg.adcRange));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, cfg.shuntResistanceOhm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, cfg.maxExpectedCurrentA);
}


void test_get_settings_before_begin_is_cache_only_uninitialized_snapshot() {
  INA228::INA228 dev;
  SettingsSnapshot settings{};
  DiagAlertSnapshot diag{};

  Status st = dev.getSettings(settings);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(settings.initialized);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                          static_cast<uint8_t>(settings.state));

  st = dev.getDiagAlertSnapshot(diag);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(diag.valid);
}

void test_driver_state_alias_matches_state() {
  FakeBus bus;
  INA228::INA228 dev;

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(dev.state()),
                          static_cast<uint8_t>(dev.driverState()));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                          static_cast<uint8_t>(dev.driverState()));

  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(dev.state()),
                          static_cast<uint8_t>(dev.driverState()));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::READY),
                          static_cast<uint8_t>(dev.driverState()));

  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::I2C_BUS, "forced bus error", -11);
  float volts = 0.0f;
  Status st = dev.readBusVoltage(volts);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_BUS),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(dev.state()),
                          static_cast<uint8_t>(dev.driverState()));
}

void test_get_settings_is_bus_silent_and_does_not_consume_diag() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  clearReadHistory(bus);
  resetWriteTracking(bus);
  const uint32_t readsBefore = bus.readCalls;
  const uint32_t writesBefore = bus.writeCalls;
  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF | cmd::DIAG_SHNTOL;

  SettingsSnapshot settings{};
  Status st = dev.getSettings(settings);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);
  TEST_ASSERT_EQUAL_UINT32(0u, bus.readHistoryCount);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF | cmd::DIAG_SHNTOL,
                          bus.diagAlrt);

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.valid);
  TEST_ASSERT_FALSE(snap.diag.cnvrf);
  TEST_ASSERT_FALSE(snap.diag.shntOL);
}

// ===========================================================================
// Lifecycle tests
// ===========================================================================

void test_begin_rejects_missing_callbacks() {
  INA228::INA228 dev;
  Config cfg;
  Status st = dev.begin(cfg);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                          static_cast<uint8_t>(dev.state()));
}

void test_begin_success_sets_ready_without_health_counts() {
  FakeBus bus;
  INA228::INA228 dev;
  Status st = dev.begin(makeConfig(bus));
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::READY),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_TRUE(dev.isOnline());
  TEST_ASSERT_GREATER_THAN_UINT32(0u, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_EQUAL_UINT32(0u, dev.totalSuccess());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.totalFailures());
  TEST_ASSERT_EQUAL_UINT8(0u, dev.consecutiveFailures());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.lastOkMs());
}

void test_configured_i2c_address_reaches_transport_callbacks() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.i2cAddress = 0x4F;

  Status st = dev.begin(cfg);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX8(0x4F, bus.lastReadAddr);
  TEST_ASSERT_EQUAL_HEX8(0x4F, bus.lastWriteAddr);
}

void test_begin_rejects_invalid_address() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.i2cAddress = 0x39;  // below valid range
  Status st = dev.begin(cfg);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
}

void test_begin_rejects_zero_timeout() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.i2cTimeoutMs = 0;
  Status st = dev.begin(cfg);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
}

void test_begin_rejects_invalid_adc_range() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.adcRange = static_cast<AdcRange>(2);
  Status st = dev.begin(cfg);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
}




void test_begin_rejects_identity_and_memstat_mismatch_without_writes() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    bus.manufacturerId = 0x1234;
    Status st = dev.begin(makeConfig(bus));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_ID_MISMATCH),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(0x1234, st.detail);
    TEST_ASSERT_FALSE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT32(0u, bus.writeCalls);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    bus.deviceId = 0x4321;
    Status st = dev.begin(makeConfig(bus));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_ID_MISMATCH),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(0x4321, st.detail);
    TEST_ASSERT_FALSE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT32(0u, bus.writeCalls);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    bus.diagAlrt = 0;
    Status st = dev.begin(makeConfig(bus));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MEMORY_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FALSE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT32(0u, bus.writeCalls);
  }
}


void test_begin_normalizes_offline_threshold_on_stored_copy() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.offlineThreshold = 0;

  Status st = dev.begin(cfg);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT8(0u, cfg.offlineThreshold);
  TEST_ASSERT_EQUAL_UINT8(1u, dev.getConfig().offlineThreshold);
}

void test_begin_programs_tempco_even_when_tempcomp_disabled() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.tempCompEnabled = false;
  cfg.shuntTempCoeffPpmC = 3900;

  Status st = dev.begin(cfg);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(3900u, bus.reg16[cmd::REG_SHUNT_TEMPCO]);
  TEST_ASSERT_FALSE(dev.getConfig().tempCompEnabled);
  TEST_ASSERT_EQUAL_UINT16(3900u, dev.getConfig().shuntTempCoeffPpmC);
}

void test_begin_rejects_non_finite_calibration() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = std::numeric_limits<float>::quiet_NaN();
  cfg.maxExpectedCurrentA = 10.0f;
  Status st = dev.begin(cfg);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
}

void test_end_returns_to_uninit() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  dev.end();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                          static_cast<uint8_t>(dev.state()));
}

// ===========================================================================
// Missing NowMs hook
// ===========================================================================

void test_missing_now_ms_uses_zero_for_health_timestamps() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.nowMs = nullptr;
  cfg.timeUser = nullptr;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  setMillis(4321);
  Status st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.lastOkMs());
}

void test_begin_without_now_ms_keeps_zero_health_timestamp() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.nowMs = nullptr;
  cfg.timeUser = nullptr;

  Status st = dev.begin(cfg);
  TEST_ASSERT_TRUE(st.ok());
  setMillis(4242u);
  st = dev.setMode(Mode::CONT_ALL);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.lastOkMs());
}

// ===========================================================================
// Probe (raw I2C, no health tracking)
// ===========================================================================

void test_probe_failure_does_not_update_health() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  const uint32_t beforeSuccess = dev.totalSuccess();
  const uint32_t beforeFailures = dev.totalFailures();
  const DriverState beforeState = dev.state();

  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::I2C_ERROR, "forced probe error", -7);
  Status st = dev.probe();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(beforeSuccess, dev.totalSuccess());
  TEST_ASSERT_EQUAL_UINT32(beforeFailures, dev.totalFailures());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(beforeState),
                          static_cast<uint8_t>(dev.state()));
}

void test_probe_preserves_transport_errors_without_health_tracking() {
  const uint8_t regs[] = {
      cmd::REG_MANUFACTURER_ID,
      cmd::REG_DEVICE_ID,
  };
  struct Case {
    Err transport;
    Err expected;
  };
  const Case cases[] = {
      {Err::I2C_NACK_ADDR, Err::DEVICE_NOT_FOUND},
      {Err::I2C_NACK_DATA, Err::I2C_NACK_DATA},
      {Err::I2C_TIMEOUT, Err::I2C_TIMEOUT},
      {Err::I2C_BUS, Err::I2C_BUS},
      {Err::I2C_ERROR, Err::I2C_ERROR},
      {Err::TIMEOUT, Err::TIMEOUT},
  };

  for (uint8_t reg : regs) {
    for (const Case& c : cases) {
      FakeBus bus;
      INA228::INA228 dev;
      TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
      const uint32_t beforeSuccess = dev.totalSuccess();
      const uint32_t beforeFailures = dev.totalFailures();
      const DriverState beforeState = dev.state();

      clearReadHistory(bus);
      bus.readError = Status::Error(c.transport, "forced probe read error", -44);
      queueNthReadFailure(bus, reg, 1);

      Status st = dev.probe();
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(c.expected),
                              static_cast<uint8_t>(st.code));
      TEST_ASSERT_EQUAL_INT32(-44, st.detail);
      TEST_ASSERT_EQUAL_UINT32(beforeSuccess, dev.totalSuccess());
      TEST_ASSERT_EQUAL_UINT32(beforeFailures, dev.totalFailures());
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(beforeState),
                              static_cast<uint8_t>(dev.state()));
    }
  }
}

// ===========================================================================
// Recover (tracked I2C, updates health)
// ===========================================================================

void test_recover_failure_updates_health_once() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::I2C_ERROR, "forced recover error", -8);
  Status st = dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(1u, dev.totalFailures());
  TEST_ASSERT_EQUAL_UINT8(1u, dev.consecutiveFailures());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(dev.lastError().code));
  TEST_ASSERT_EQUAL_UINT32(bus.nowMs, dev.lastErrorMs());
}






















void test_recover_replay_failures_mark_dirty_for_each_write_position() {
  struct Case {
    uint8_t reg;
    uint8_t nth;
  };
  const Case cases[] = {
      {cmd::REG_ADC_CONFIG, 1},
      {cmd::REG_CONFIG, 1},
      {cmd::REG_DIAG_ALRT, 1},
      {cmd::REG_SHUNT_TEMPCO, 1},
      {cmd::REG_SHUNT_CAL, 1},
      {cmd::REG_ADC_CONFIG, 2},
  };

  for (const Case& c : cases) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_TRUE(dev.setAlertLatch(true).ok());
    TEST_ASSERT_TRUE(dev.writeRegister16(cmd::REG_SHUNT_TEMPCO, 0x1234).ok());

    resetWriteTracking(bus);
    queueNthWriteFailure(bus, c.reg, c.nth);
    Status st = dev.recover();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));

    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << c.reg)) != 0);
  }
}

// ===========================================================================
// Transport helper tests (example layer)
// ===========================================================================

void test_example_transport_maps_wire_errors_and_keeps_timeout_owned_by_init() {
  Wire._clearEndTransmissionResult();
  Wire._clearRequestFromOverride();

  TEST_ASSERT_TRUE(transport::initWire(8, 9, 400000, 77));
  TEST_ASSERT_EQUAL_UINT32(77u, Wire.getTimeOut());

  Wire._setBeginResult(false);
  TEST_ASSERT_FALSE(transport::initWire(8, 9, 400000, 88));
  TEST_ASSERT_EQUAL_UINT32(77u, Wire.getTimeOut());
  Wire._setBeginResult(true);

  const uint8_t byte = 0x55;

  Wire._setEndTransmissionResult(2);
  Status st = transport::wireWrite(0x40, &byte, 1, 123, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_NACK_ADDR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(77u, Wire.getTimeOut());

  Wire._setEndTransmissionResult(3);
  st = transport::wireWrite(0x40, &byte, 1, 999, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_NACK_DATA),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(77u, Wire.getTimeOut());

  Wire._setEndTransmissionResult(4);
  st = transport::wireWrite(0x40, &byte, 1, 999, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_BUS),
                          static_cast<uint8_t>(st.code));

  Wire._setEndTransmissionResult(5);
  st = transport::wireWrite(0x40, &byte, 1, 999, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_TIMEOUT),
                          static_cast<uint8_t>(st.code));

  Wire._setEndTransmissionResult(1);
  st = transport::wireWrite(0x40, &byte, 1, 999, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));
}

void test_example_transport_validates_params_and_handles_write_read() {
  const uint8_t tx = 0x00;
  uint8_t rx = 0;

  Status st = transport::wireWrite(0x40, nullptr, 1, 50, nullptr);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));

  st = transport::wireWrite(0x40, &tx, 0, 50, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));

  st = transport::wireWriteRead(0x40, nullptr, 1, &rx, 1, 50, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));

  st = transport::wireWriteRead(0x40, &tx, 1, nullptr, 1, 50, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));

  Wire._clearEndTransmissionResult();
  Wire._setRequestFromResult(1);
  st = transport::wireWriteRead(0x40, &tx, 1, &rx, 1, 50, &Wire);
  TEST_ASSERT_TRUE(st.ok());

  // A short read re-probes the address to recover the phase that
  // endTransmission(false) cannot report on arduino-esp32. The device still
  // acknowledges here, so the Arduino API cannot recover the failed phase or
  // cause. Report a generic transport failure rather than inventing a NACK.
  Wire._setRequestFromResult(0);
  Wire._queueEndTransmissionResult(0);
  Wire._queueEndTransmissionResult(0);
  st = transport::wireWriteRead(0x40, &tx, 1, &rx, 1, 50, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));

  // When the re-probe also NACKs, the absent device is reported precisely.
  Wire._clearEndTransmissionResult();
  Wire._queueEndTransmissionResult(0);
  Wire._queueEndTransmissionResult(2);
  st = transport::wireWriteRead(0x40, &tx, 1, &rx, 1, 50, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_NACK_ADDR),
                          static_cast<uint8_t>(st.code));
  Wire._clearEndTransmissionResult();
  Wire._clearRequestFromOverride();

  const uint8_t twoBytes[] = {0x00, 0x55};
  Wire._setWriteResult(1);
  Wire._resetTransmissionObservations();
  st = transport::wireWrite(0x40, twoBytes, sizeof(twoBytes), 50, &Wire);
  TEST_ASSERT_TRUE(st.is(Err::I2C_ERROR));
  TEST_ASSERT_EQUAL_UINT32(2u, Wire._beginTransmissionCallCount());
  TEST_ASSERT_EQUAL_UINT32(1u, Wire._endTransmissionCallCount());
  TEST_ASSERT_EQUAL_UINT32(0u, Wire._lastEndTransmissionLength());

  Wire._resetTransmissionObservations();
  st = transport::wireWriteRead(0x40, twoBytes, sizeof(twoBytes), &rx, 1, 50, &Wire);
  TEST_ASSERT_TRUE(st.is(Err::I2C_ERROR));
  TEST_ASSERT_EQUAL_UINT32(2u, Wire._beginTransmissionCallCount());
  TEST_ASSERT_EQUAL_UINT32(1u, Wire._endTransmissionCallCount());
  TEST_ASSERT_EQUAL_UINT32(0u, Wire._lastEndTransmissionLength());
  Wire._clearWriteOverride();

  uint8_t boundary[transport::WIRE_BUFFER_LIMIT + 1U] = {};
  Wire._clearEndTransmissionResult();
  Wire._clearRequestFromOverride();
  TEST_ASSERT_TRUE(transport::wireWrite(
      0x40, boundary, transport::WIRE_BUFFER_LIMIT, 50, &Wire).ok());
  TEST_ASSERT_TRUE(transport::wireWrite(
      0x40, boundary, transport::WIRE_BUFFER_LIMIT + 1U, 50, &Wire)
      .is(Err::INVALID_PARAM));
  TEST_ASSERT_TRUE(transport::wireWriteRead(
      0x40, boundary, transport::WIRE_BUFFER_LIMIT,
      boundary, 1U, 50, &Wire).ok());
  TEST_ASSERT_TRUE(transport::wireWriteRead(
      0x40, boundary, 1U,
      boundary, transport::WIRE_BUFFER_LIMIT, 50, &Wire).ok());
  TEST_ASSERT_TRUE(transport::wireWriteRead(
      0x40, boundary, transport::WIRE_BUFFER_LIMIT + 1U,
      boundary, 1U, 50, &Wire).is(Err::INVALID_PARAM));
  TEST_ASSERT_TRUE(transport::wireWriteRead(
      0x40, boundary, 1U,
      boundary, transport::WIRE_BUFFER_LIMIT + 1U, 50, &Wire)
      .is(Err::INVALID_PARAM));
}

// ===========================================================================
// Conversion time estimate
// ===========================================================================

void test_conversion_time_estimate() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.mode = Mode::CONT_ALL;
  cfg.vbusConvTime = ConvTime::US_1052;
  cfg.vshuntConvTime = ConvTime::US_1052;
  cfg.vtempConvTime = ConvTime::US_1052;
  cfg.averaging = Averaging::AVG_1;
  cfg.convDelayMs2 = 0;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  // 3 channels × 1052 µs × 1 avg = 3156 µs
  TEST_ASSERT_EQUAL_UINT32(3156u, dev.estimateConversionTimeUs());
  TEST_ASSERT_EQUAL_UINT32(4u, dev.estimateConversionTimeMs());  // ceil(3156/1000)
}

void test_conversion_time_with_averaging() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.mode = Mode::CONT_SHUNT_BUS;
  cfg.vbusConvTime = ConvTime::US_540;
  cfg.vshuntConvTime = ConvTime::US_540;
  cfg.averaging = Averaging::AVG_4;
  cfg.convDelayMs2 = 0;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  // 2 channels × 540 µs × 4 avg = 4320 µs
  TEST_ASSERT_EQUAL_UINT32(4320u, dev.estimateConversionTimeUs());
}

void test_shutdown_conversion_time_ignores_configured_delay() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.mode = Mode::SHUTDOWN;
  cfg.convDelayMs2 = 7;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  TEST_ASSERT_EQUAL_UINT32(0u, dev.estimateConversionTimeUs());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.estimateConversionTimeMs());
}

void test_conversion_time_defaults_and_maximum() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.mode = Mode::CONT_ALL;
  cfg.vbusConvTime = ConvTime::US_4120;
  cfg.vshuntConvTime = ConvTime::US_4120;
  cfg.vtempConvTime = ConvTime::US_4120;
  cfg.averaging = Averaging::AVG_1024;
  cfg.convDelayMs2 = 255;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  // CONVDLY 510 ms + (3 channels x 4120 us x 1024 avg) = 13,166,640 us.
  TEST_ASSERT_EQUAL_UINT32(13166640u, dev.estimateConversionTimeUs());
  TEST_ASSERT_EQUAL_UINT32(13167u, dev.estimateConversionTimeMs());
}

void test_conversion_time_table_vectors() {
  const ConvTime times[] = {
      ConvTime::US_50,
      ConvTime::US_84,
      ConvTime::US_150,
      ConvTime::US_280,
      ConvTime::US_540,
      ConvTime::US_1052,
      ConvTime::US_2074,
      ConvTime::US_4120,
  };
  const uint32_t expectedTimes[] = {50, 84, 150, 280, 540, 1052, 2074, 4120};

  for (size_t i = 0; i < sizeof(times) / sizeof(times[0]); ++i) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.mode = Mode::CONT_BUS;
    cfg.vbusConvTime = times[i];
    cfg.averaging = Averaging::AVG_1;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_EQUAL_UINT32(expectedTimes[i], dev.estimateConversionTimeUs());
  }

  const Averaging averages[] = {
      Averaging::AVG_1,
      Averaging::AVG_4,
      Averaging::AVG_16,
      Averaging::AVG_64,
      Averaging::AVG_128,
      Averaging::AVG_256,
      Averaging::AVG_512,
      Averaging::AVG_1024,
  };
  const uint32_t expectedAverages[] = {50, 200, 800, 3200, 6400, 12800, 25600, 51200};

  for (size_t i = 0; i < sizeof(averages) / sizeof(averages[0]); ++i) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.mode = Mode::CONT_BUS;
    cfg.vbusConvTime = ConvTime::US_50;
    cfg.averaging = averages[i];
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_EQUAL_UINT32(expectedAverages[i], dev.estimateConversionTimeUs());
  }

  FakeBus delayBus;
  INA228::INA228 delayDev;
  Config delayCfg = makeConfig(delayBus);
  delayCfg.mode = Mode::CONT_BUS;
  delayCfg.vbusConvTime = ConvTime::US_50;
  delayCfg.averaging = Averaging::AVG_1;
  delayCfg.convDelayMs2 = 7;
  TEST_ASSERT_TRUE(delayDev.begin(delayCfg).ok());
  TEST_ASSERT_EQUAL_UINT32(14050u, delayDev.estimateConversionTimeUs());
}

void test_config_register_encoding_vectors() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
    TEST_ASSERT_EQUAL_HEX16(0xFB68u, bus.reg16[cmd::REG_ADC_CONFIG]);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.mode = Mode::CONT_TEMP_SHUNT;
    cfg.vbusConvTime = ConvTime::US_50;
    cfg.vshuntConvTime = ConvTime::US_84;
    cfg.vtempConvTime = ConvTime::US_150;
    cfg.averaging = Averaging::AVG_16;
    cfg.convDelayMs2 = 7;
    cfg.tempCompEnabled = true;
    cfg.adcRange = AdcRange::MV_40_96;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_EQUAL_HEX16(0xE052u, bus.reg16[cmd::REG_ADC_CONFIG]);
    TEST_ASSERT_EQUAL_HEX16(0x01F0u, bus.reg16[cmd::REG_CONFIG]);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.mode = Mode::CONT_ALL;
    cfg.vbusConvTime = ConvTime::US_4120;
    cfg.vshuntConvTime = ConvTime::US_4120;
    cfg.vtempConvTime = ConvTime::US_4120;
    cfg.averaging = Averaging::AVG_1024;
    cfg.convDelayMs2 = 255;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_EQUAL_HEX16(0xFFFFu, bus.reg16[cmd::REG_ADC_CONFIG]);
    TEST_ASSERT_EQUAL_HEX16(0x3FC0u, bus.reg16[cmd::REG_CONFIG]);
  }
}



void test_conversion_ready_clears_completed_trigger_state() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  Status st = dev.triggerConversion(Mode::TRIG_ALL);
  TEST_ASSERT_TRUE(st.inProgress());
  bus.nowMs += dev.estimateConversionTimeMs();
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;

  bool ready = false;
  st = dev.isConversionReady(ready);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(ready);

  SettingsSnapshot snap;
  st = dev.getSettings(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(snap.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(0u, snap.triggeredConversionStartMs);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::TRIG_ALL),
                          static_cast<uint8_t>(snap.mode));
  TEST_ASSERT_TRUE(dev.estimateConversionTimeUs() > 0u);

  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startVerifyConfiguration(0x2281u, operationId).ok());
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
}

void test_triggered_conversion_gates_reads_until_cnvrf() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  Status st = dev.triggerConversion(Mode::TRIG_ALL);
  TEST_ASSERT_TRUE(st.inProgress());

  float volts = 1.0f;
  st = dev.readBusVoltage(volts);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MEASUREMENT_NOT_READY),
                          static_cast<uint8_t>(st.code));

  bus.nowMs += dev.estimateConversionTimeMs();
  st = dev.readBusVoltage(volts);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MEASUREMENT_NOT_READY),
                          static_cast<uint8_t>(st.code));

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  bus.reg24[cmd::REG_VBUS] = 0x001000;
  st = dev.readBusVoltage(volts);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(volts > 0.0f);

  SettingsSnapshot snap;
  st = dev.getSettings(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(snap.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(0u, snap.triggeredConversionStartMs);
}

void test_tick_timestamp_completes_trigger_without_now_hook() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.nowMs = nullptr;
  cfg.timeUser = nullptr;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  bus.nowMs = 9000U;
  Status st = dev.triggerConversion(Mode::TRIG_ALL);
  TEST_ASSERT_TRUE(st.inProgress());
  TEST_ASSERT_TRUE(dev.setVbusConvTime(ConvTime::US_84).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;

  const uint32_t dueMs = dev.estimateConversionTimeMs();
  const uint32_t anchorMs = 12000U;
  const uint32_t readsBefore = bus.readCalls;
  bool ready = true;
  TEST_ASSERT_TRUE(dev.isConversionReady(ready).ok());
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);

  dev.tick(anchorMs);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  SettingsSnapshot settings{};
  st = dev.getSettings(settings);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(anchorMs, settings.triggeredConversionStartMs);

  dev.tick(anchorMs + dueMs - 1U);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  dev.tick(anchorMs + dueMs);
  TEST_ASSERT_EQUAL_UINT32(readsBefore + 1U, bus.readCalls);

  st = dev.getSettings(settings);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(0u, settings.triggeredConversionStartMs);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::TRIG_ALL),
                          static_cast<uint8_t>(settings.mode));
}

void test_tick_deadline_is_wraparound_safe() {
  FakeBus bus;
  bus.nowMs = std::numeric_limits<uint32_t>::max() - 1u;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  Status st = dev.triggerConversion(Mode::TRIG_ALL);
  TEST_ASSERT_TRUE(st.inProgress());
  const uint32_t startMs = bus.nowMs;
  const uint32_t dueMs = dev.estimateConversionTimeMs();
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  const uint32_t readsBefore = bus.readCalls;

  dev.tick(startMs + dueMs - 1u);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  SettingsSnapshot settings{};
  st = dev.getSettings(settings);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(settings.triggeredConversionPending);

  dev.tick(startMs + dueMs);
  TEST_ASSERT_TRUE(bus.readCalls > readsBefore);
  st = dev.getSettings(settings);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::TRIG_ALL),
                          static_cast<uint8_t>(settings.mode));
}

void test_tick_is_bus_silent_without_a_pending_trigger() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF |
                 cmd::DIAG_ENERGYOF | cmd::DIAG_MATHOF;
  const uint32_t transfersBefore = bus.readCalls + bus.writeCalls;
  dev.tick(1000U);
  TEST_ASSERT_EQUAL_UINT32(transfersBefore,
                           bus.readCalls + bus.writeCalls);

  TEST_ASSERT_TRUE(dev.setVbusConvTime(ConvTime::US_84).ok());
  const uint32_t transfersAfterSetter = bus.readCalls + bus.writeCalls;
  dev.tick(2000U);
  TEST_ASSERT_EQUAL_UINT32(transfersAfterSetter,
                           bus.readCalls + bus.writeCalls);
}

void test_configured_trigger_uses_hooked_post_write_origin_for_initialize_and_reinitialize() {
  FakeBus bus;
  bus.nowMs = std::numeric_limits<uint32_t>::max() - 1U;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);
  cfg.mode = Mode::TRIG_ALL;
  bus.successfulWriteDurationReg = cmd::REG_ADC_CONFIG;
  bus.successfulWriteDurationMatch = 2U;
  bus.advanceNowMsOnWrite = 3U;
  TEST_ASSERT_TRUE(dev.bind(cfg).ok());

  uint32_t operationId = 0;
  const uint32_t preWriteMs = bus.nowMs;
  TEST_ASSERT_TRUE(dev.startInitialize(0x3101U, operationId).ok());
  TEST_ASSERT_TRUE(dev.pollJob(preWriteMs, 14U).ok());
  const uint32_t postWriteMs = bus.nowMs;
  TEST_ASSERT_EQUAL_UINT32(preWriteMs + 3U, postWriteMs);
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT16(14U, result.job.transfersCompleted);

  const uint32_t waitMs = dev.estimateConversionTimeMs();
  clearReadHistory(bus);
  const uint32_t readsBefore = bus.readCalls;
  bool ready = true;
  TEST_ASSERT_TRUE(dev.pollConversionReady(postWriteMs + waitMs - 1U, ready).ok());
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  TEST_ASSERT_TRUE(dev.pollConversionReady(postWriteMs + waitMs, ready).ok());
  TEST_ASSERT_TRUE(ready);
  TEST_ASSERT_EQUAL_UINT32(readsBefore + 1U, bus.readCalls);

  TEST_ASSERT_TRUE(dev.setMode(Mode::TRIG_ALL).inProgress());
  TEST_ASSERT_TRUE(dev.invalidateHardwareState(Status::Ok()).ok());
  clearReadHistory(bus);
  resetWriteTracking(bus);
  clearTransferHistory(bus);
  bus.nowMs = 4000U;
  bus.successfulWriteDurationReg = cmd::REG_ADC_CONFIG;
  bus.successfulWriteDurationMatch = 2U;
  bus.advanceNowMsOnWrite = 3U;
  const uint32_t replayPreWriteMs = bus.nowMs;
  TEST_ASSERT_TRUE(dev.startReinitialize(0x3102U, operationId).ok());
  TEST_ASSERT_TRUE(dev.pollJob(replayPreWriteMs, 14U).ok());
  const uint32_t replayPostWriteMs = bus.nowMs;
  TEST_ASSERT_EQUAL_UINT32(replayPreWriteMs + 3U, replayPostWriteMs);
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT16(14U, result.job.transfersCompleted);
  TEST_ASSERT_EQUAL_UINT32(14U, bus.transferHistoryCount);

  clearReadHistory(bus);
  const uint32_t replayReadsBefore = bus.readCalls;
  ready = true;
  TEST_ASSERT_TRUE(dev.pollConversionReady(
      replayPostWriteMs + waitMs - 1U, ready).ok());
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_UINT32(replayReadsBefore, bus.readCalls);
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  TEST_ASSERT_TRUE(dev.pollConversionReady(
      replayPostWriteMs + waitMs, ready).ok());
  TEST_ASSERT_TRUE(ready);
  TEST_ASSERT_EQUAL_UINT32(replayReadsBefore + 1U, bus.readCalls);
}

void test_configured_trigger_without_hook_anchors_after_successful_terminal() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);
  cfg.mode = Mode::TRIG_ALL;
  cfg.nowMs = nullptr;
  cfg.timeUser = nullptr;
  TEST_ASSERT_TRUE(dev.bind(cfg).ok());

  uint32_t operationId = 0;
  bus.nowMs = 7000U;
  bus.successfulWriteDurationReg = cmd::REG_ADC_CONFIG;
  bus.successfulWriteDurationMatch = 2U;
  bus.advanceNowMsOnWrite = 3U;
  TEST_ASSERT_TRUE(dev.startInitialize(0x3201U, operationId).ok());
  TEST_ASSERT_TRUE(dev.pollJob(7000U, 14U).ok());
  TEST_ASSERT_EQUAL_UINT32(7003U, bus.nowMs);
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT16(14U, result.job.transfersCompleted);

  clearReadHistory(bus);
  const uint32_t readsBefore = bus.readCalls;
  bool ready = true;
  TEST_ASSERT_TRUE(dev.isConversionReady(ready).ok());
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);

  const uint32_t waitMs = dev.estimateConversionTimeMs();
  const uint32_t anchorMs = std::numeric_limits<uint32_t>::max() - 2U;
  TEST_ASSERT_TRUE(dev.pollConversionReady(anchorMs, ready).ok());
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  SettingsSnapshot settings{};
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_TRUE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(anchorMs, settings.triggeredConversionStartMs);

  TEST_ASSERT_TRUE(dev.pollConversionReady(anchorMs + waitMs - 1U, ready).ok());
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  TEST_ASSERT_TRUE(dev.pollConversionReady(anchorMs + waitMs, ready).ok());
  TEST_ASSERT_TRUE(ready);
  TEST_ASSERT_EQUAL_UINT32(readsBefore + 1U, bus.readCalls);
}

void test_hookless_internal_sync_jobs_do_not_consume_trigger_origin() {
  FakeBus bus;
  bus.autoClearAccumulatorReset = true;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);
  cfg.mode = Mode::TRIG_ALL;
  cfg.nowMs = nullptr;
  cfg.timeUser = nullptr;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  SettingsSnapshot settings{};
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_TRUE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(0U, settings.triggeredConversionStartMs);

  clearTransferHistory(bus);
  TEST_ASSERT_TRUE(dev.recover().ok());
  TEST_ASSERT_EQUAL_UINT32(14U, bus.transferHistoryCount);
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_TRUE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(0U, settings.triggeredConversionStartMs);

  clearTransferHistory(bus);
  TEST_ASSERT_TRUE(dev.resetAccumulators().ok());
  TEST_ASSERT_EQUAL_UINT32(2U, bus.transferHistoryCount);
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_TRUE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(0U, settings.triggeredConversionStartMs);

  const uint32_t readsBefore = bus.readCalls;
  bool ready = true;
  TEST_ASSERT_TRUE(dev.pollConversionReady(9000U, ready).ok());
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_EQUAL_UINT32(9000U, settings.triggeredConversionStartMs);
}

void test_hookless_configured_trigger_deferral_lifetime_is_bounded() {
  for (uint8_t terminalKind = 0; terminalKind < 3U; ++terminalKind) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeCooperativeConfig(bus);
    cfg.mode = Mode::TRIG_ALL;
    cfg.nowMs = nullptr;
    cfg.timeUser = nullptr;
    TEST_ASSERT_TRUE(dev.bind(cfg).ok());

    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInitialize(0x3250U + terminalKind,
                                         operationId).ok());
    if (terminalKind < 2U) {
      TEST_ASSERT_TRUE(dev.pollJob(7100U, 9U).inProgress());
      TEST_ASSERT_EQUAL_UINT32(9U, bus.transferHistoryCount);
      TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG,
                             bus.transferHistory[8].reg);
      const Status terminal = terminalKind == 0U
          ? dev.cancelJob()
          : dev.timeoutJob();
      TEST_ASSERT_TRUE(terminal.is(terminalKind == 0U
                                      ? Err::CANCELLED
                                      : Err::OPERATION_TIMEOUT));
    } else {
      queueNthReadFailure(bus, cmd::REG_CONFIG, 1U);
      const Status terminal = dev.pollJob(7100U, 10U);
      TEST_ASSERT_TRUE(terminal.is(Err::I2C_ERROR));
      TEST_ASSERT_EQUAL_UINT32(10U, bus.transferHistoryCount);
    }

    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::PARTIAL),
                            static_cast<uint8_t>(result.job.effect));

    TEST_ASSERT_TRUE(dev.startReinitialize(0x3260U + terminalKind,
                                           operationId).ok());
    const uint32_t beforeNextJob = bus.readCalls + bus.writeCalls;
    TEST_ASSERT_TRUE(dev.pollJob(7200U, 1U).inProgress());
    TEST_ASSERT_EQUAL_UINT32(beforeNextJob + 1U,
                             bus.readCalls + bus.writeCalls);
    TEST_ASSERT_TRUE(dev.cancelJob().is(Err::CANCELLED));
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  }
}

void test_write_duration_injection_is_consumed_only_by_success() {
  for (uint8_t applyThenFail = 0; applyThenFail < 2U; ++applyThenFail) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.nowMs = nullptr;
    cfg.timeUser = nullptr;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    resetWriteTracking(bus);
    bus.nowMs = 6000U;
    bus.successfulWriteDurationReg = cmd::REG_ADC_CONFIG;
    bus.successfulWriteDurationMatch = 1U;
    bus.advanceNowMsOnWrite = 3U;
    if (applyThenFail == 0U) {
      queueNthWriteFailure(bus, cmd::REG_ADC_CONFIG, 1U);
    } else {
      bus.applyThenFailWriteReg = cmd::REG_ADC_CONFIG;
      bus.applyThenFailWriteMatch = 1U;
    }

    const Status st = dev.triggerConversion(Mode::TRIG_ALL);
    TEST_ASSERT_TRUE(st.is(Err::I2C_ERROR));
    TEST_ASSERT_EQUAL_UINT32(6000U, bus.nowMs);
    TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG,
                           bus.successfulWriteDurationReg);
    TEST_ASSERT_EQUAL_UINT8(1U, bus.successfulWriteDurationMatch);
    TEST_ASSERT_EQUAL_UINT32(3U, bus.advanceNowMsOnWrite);
    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_FALSE(settings.triggeredConversionPending);
  }
}

void test_ambiguous_adc_writes_invalidate_pending_trigger_timing() {
  for (uint8_t applyThenFail = 0; applyThenFail < 2U; ++applyThenFail) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.nowMs = nullptr;
    cfg.timeUser = nullptr;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_TRUE(dev.triggerConversion(Mode::TRIG_ALL).inProgress());

    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_TRUE(settings.triggeredConversionPending);
    resetWriteTracking(bus);
    if (applyThenFail == 0U) {
      queueNthWriteFailure(bus, cmd::REG_ADC_CONFIG, 1U);
    } else {
      bus.applyThenFailWriteReg = cmd::REG_ADC_CONFIG;
      bus.applyThenFailWriteMatch = 1U;
    }

    const Status st = dev.triggerConversion(Mode::TRIG_BUS);
    TEST_ASSERT_TRUE(st.is(Err::I2C_ERROR));
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_FALSE(settings.triggeredConversionPending);
    TEST_ASSERT_EQUAL_UINT32(0U, settings.triggeredConversionStartMs);

    const uint32_t readsBefore = bus.readCalls;
    bool ready = true;
    TEST_ASSERT_TRUE(
        dev.pollConversionReady(9000U, ready).is(Err::HARDWARE_STATE_UNKNOWN));
    TEST_ASSERT_FALSE(ready);
    TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  }
}

void test_raw_conversion_invalidators_clear_pending_trigger_timing() {
  const uint8_t registers[] = {cmd::REG_ADC_CONFIG, cmd::REG_CONFIG};
  const uint16_t values[] = {0x0000U, cmd::CONFIG_RST};
  for (size_t i = 0; i < 2U; ++i) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.nowMs = nullptr;
    cfg.timeUser = nullptr;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_TRUE(dev.triggerConversion(Mode::TRIG_ALL).inProgress());
    TEST_ASSERT_TRUE(dev.writeRegister16(registers[i], values[i]).ok());

    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_FALSE(settings.triggeredConversionPending);
    TEST_ASSERT_EQUAL_UINT32(0U, settings.triggeredConversionStartMs);
  }
}

void test_legacy_range_change_uses_validated_calibration_plan_at_boundary() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
    TEST_ASSERT_TRUE(dev.setCalibration(0.015f, 2.7306f).ok());
    TEST_ASSERT_TRUE(dev.setAdcRange(AdcRange::MV_40_96).ok());

    CalibrationPlan plan{};
    TEST_ASSERT_TRUE(dev.getCalibrationPlan(plan).ok());
    TEST_ASSERT_EQUAL_UINT32(40960U, plan.shuntFullScaleMicrovolts);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, plan.selectedCurrentLsbNanoAmps);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, plan.effectiveCurrentLsbNanoAmps);
    TEST_ASSERT_FALSE(plan.clamped);
    TEST_ASSERT_FALSE(plan.maxCurrentExceedsShuntRange);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
    TEST_ASSERT_TRUE(dev.setCalibration(0.015f, 2.7307f).ok());
    CalibrationPlan oldPlan{};
    TEST_ASSERT_TRUE(dev.getCalibrationPlan(oldPlan).ok());
    const uint32_t transfersBefore = bus.readCalls + bus.writeCalls;

    TEST_ASSERT_TRUE(dev.setAdcRange(AdcRange::MV_40_96)
                         .is(Err::INVALID_CONFIG));
    TEST_ASSERT_EQUAL_UINT32(transfersBefore, bus.readCalls + bus.writeCalls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcRange::MV_163_84),
                            static_cast<uint8_t>(dev.getConfig().adcRange));
    CalibrationPlan plan{};
    TEST_ASSERT_TRUE(dev.getCalibrationPlan(plan).ok());
    TEST_ASSERT_EQUAL_HEX16(oldPlan.shuntCal, plan.shuntCal);
    TEST_ASSERT_EQUAL_UINT32(oldPlan.effectiveCurrentLsbNanoAmps,
                             plan.effectiveCurrentLsbNanoAmps);
  }
}

void test_public_diag_read_consuming_cnvrf_does_not_strand_pending_trigger() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  Status st = dev.triggerConversion(Mode::TRIG_ALL);
  TEST_ASSERT_TRUE(st.inProgress());
  bus.nowMs += dev.estimateConversionTimeMs();
  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF | cmd::DIAG_BUSOL;

  uint16_t raw = 0;
  st = dev.readDiagAlertRaw(raw);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE((raw & cmd::DIAG_CNVRF) != 0);
  TEST_ASSERT_FALSE((bus.diagAlrt & cmd::DIAG_CNVRF) != 0);

  SettingsSnapshot settings{};
  st = dev.getSettings(settings);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::TRIG_ALL),
                          static_cast<uint8_t>(settings.mode));

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.valid);
  TEST_ASSERT_TRUE(snap.diag.cnvrf);
  TEST_ASSERT_TRUE(snap.diag.busOL);
}

void test_tick_preserves_diag_alert_evidence_when_polling_cnvrf() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  Status st = dev.triggerConversion(Mode::TRIG_ALL);
  TEST_ASSERT_TRUE(st.inProgress());
  bus.nowMs += dev.estimateConversionTimeMs();

  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_ALATCH |
                 cmd::DIAG_CNVRF | cmd::DIAG_SHNTOL;

  dev.tick(bus.nowMs);

  SettingsSnapshot settings{};
  st = dev.getSettings(settings);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(0u, settings.triggeredConversionStartMs);
  TEST_ASSERT_FALSE((bus.diagAlrt & cmd::DIAG_CNVRF) != 0);
  TEST_ASSERT_FALSE((bus.diagAlrt & cmd::DIAG_SHNTOL) != 0);

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.valid);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_MEMSTAT | cmd::DIAG_ALATCH |
                          cmd::DIAG_CNVRF | cmd::DIAG_SHNTOL,
                          snap.raw);
  TEST_ASSERT_TRUE(snap.diag.cnvrf);
  TEST_ASSERT_TRUE(snap.diag.shntOL);
  TEST_ASSERT_TRUE(snap.diag.memstat);
  TEST_ASSERT_EQUAL_UINT32(bus.nowMs, snap.capturedMs);
}


void test_readiness_path_preserves_diag_alert_evidence_for_measurement_gate() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  Status st = dev.triggerConversion(Mode::TRIG_ALL);
  TEST_ASSERT_TRUE(st.inProgress());
  bus.nowMs += dev.estimateConversionTimeMs();

  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_ALATCH |
                 cmd::DIAG_CNVRF | cmd::DIAG_BUSOL;
  bus.reg24[cmd::REG_VBUS] = 0x001000;

  float volts = 0.0f;
  st = dev.readBusVoltage(volts);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(volts > 0.0f);
  TEST_ASSERT_FALSE((bus.diagAlrt & cmd::DIAG_BUSOL) != 0);

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.valid);
  TEST_ASSERT_TRUE(snap.diag.busOL);
  TEST_ASSERT_TRUE(snap.diag.cnvrf);
}

void test_alert_config_setters_do_not_read_live_diag_alrt() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF |
                 cmd::DIAG_SHNTOL | cmd::DIAG_BUSOL;
  const uint32_t readsBefore = bus.readCalls;

  Status st = dev.setAlertLatch(true);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_DIAG_ALRT, bus.lastWriteReg);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_ALATCH, bus.lastWrite16);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF |
                          cmd::DIAG_SHNTOL | cmd::DIAG_BUSOL | cmd::DIAG_ALATCH,
                          bus.diagAlrt);

  st = dev.setConversionReadyAlert(true);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_ALATCH | cmd::DIAG_CNVR, bus.lastWrite16);

  st = dev.setSlowAlert(true);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_ALATCH | cmd::DIAG_CNVR |
                          cmd::DIAG_SLOWALERT,
                          bus.lastWrite16);

  st = dev.setAlertPolarity(true);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_ALATCH | cmd::DIAG_CNVR |
                          cmd::DIAG_SLOWALERT | cmd::DIAG_APOL,
                          bus.lastWrite16);
  TEST_ASSERT_EQUAL_HEX16(0u, bus.lastWrite16 & ~cmd::DIAG_CONFIG_MASK);
  TEST_ASSERT_TRUE((bus.diagAlrt & cmd::DIAG_SHNTOL) != 0);
  TEST_ASSERT_TRUE((bus.diagAlrt & cmd::DIAG_BUSOL) != 0);
  TEST_ASSERT_TRUE((bus.diagAlrt & cmd::DIAG_CNVRF) != 0);
}

void test_alert_config_write_failure_preserves_cache_and_marks_diag_dirty() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  const uint16_t oldDiag = bus.diagAlrt;
  TEST_ASSERT_FALSE(dev.getConfig().alerts.latched);

  bus.writeError = Status::Error(Err::I2C_NACK_DATA, "forced alert NACK", 17);
  bus.writeErrorRemaining = 1;
  const Status st = dev.setAlertLatch(true);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_NACK_DATA),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_INT32(17, st.detail);
  TEST_ASSERT_EQUAL_STRING("forced alert NACK", st.msg);
  TEST_ASSERT_FALSE(dev.getConfig().alerts.latched);
  TEST_ASSERT_EQUAL_HEX16(oldDiag, bus.diagAlrt);

  SettingsSnapshot settings{};
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_TRUE(settings.hardwareDirty);
  TEST_ASSERT_TRUE((settings.dirtyRegisterMask &
                    (uint64_t{1} << cmd::REG_DIAG_ALRT)) != 0U);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                          static_cast<uint8_t>(dev.hardwareState()));
}

void test_public_read_diag_alert_is_destructive_and_preserved() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_ALATCH |
                 cmd::DIAG_CNVRF | cmd::DIAG_TMPOL |
                 cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF;

  DiagAlert diag{};
  Status st = dev.readDiagAlert(diag);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(diag.memstat);
  TEST_ASSERT_TRUE(diag.alatch);
  TEST_ASSERT_TRUE(diag.cnvrf);
  TEST_ASSERT_TRUE(diag.tmpOL);
  TEST_ASSERT_TRUE(diag.energyOF);
  TEST_ASSERT_TRUE(diag.chargeOF);

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.valid);
  TEST_ASSERT_TRUE(snap.diag.tmpOL);
  TEST_ASSERT_TRUE(snap.diag.cnvrf);

  TEST_ASSERT_FALSE((bus.diagAlrt & cmd::DIAG_CNVRF) != 0);
  TEST_ASSERT_FALSE((bus.diagAlrt & cmd::DIAG_TMPOL) != 0);
  TEST_ASSERT_TRUE((bus.diagAlrt & cmd::DIAG_ENERGYOF) != 0);
  TEST_ASSERT_TRUE((bus.diagAlrt & cmd::DIAG_CHARGEOF) != 0);
}

void test_public_read_diag_alert_raw_is_destructive() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.clearDiagOnRead = true;
  const uint16_t seeded = cmd::DIAG_MEMSTAT | cmd::DIAG_ALATCH |
                          cmd::DIAG_CNVRF | cmd::DIAG_BUSOL |
                          cmd::DIAG_ENERGYOF;
  bus.diagAlrt = seeded;

  uint16_t raw = 0;
  Status st = dev.readDiagAlertRaw(raw);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(seeded, raw);

  st = dev.readDiagAlertRaw(raw);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE((raw & cmd::DIAG_CNVRF) != 0);
  TEST_ASSERT_FALSE((raw & cmd::DIAG_BUSOL) != 0);
  TEST_ASSERT_TRUE((raw & cmd::DIAG_MEMSTAT) != 0);
  TEST_ASSERT_TRUE((raw & cmd::DIAG_ALATCH) != 0);
  TEST_ASSERT_TRUE((raw & cmd::DIAG_ENERGYOF) != 0);
}

// ===========================================================================
// Measurement (basic - reads return zeros)
// ===========================================================================

void test_read_bus_voltage_requires_init() {
  INA228::INA228 dev;
  float v = 99.9f;
  Status st = dev.readBusVoltage(v);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::NOT_INITIALIZED),
                          static_cast<uint8_t>(st.code));
}

void test_read_bus_voltage_zero_on_default() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  float v = 99.9f;
  Status st = dev.readBusVoltage(v);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, v);
}

void test_uncalibrated_current_power_energy_charge_fail_without_i2c() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  const uint32_t readsAfterBegin = bus.readCalls;

  float f = 0.0f;
  Status st = dev.readCurrent(f);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsAfterBegin, bus.readCalls);

  st = dev.readPower(f);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsAfterBegin, bus.readCalls);

  double d = 0.0;
  st = dev.readEnergy(d);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsAfterBegin, bus.readCalls);

  st = dev.readCharge(d);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsAfterBegin, bus.readCalls);

  Measurement m{};
  m.busVoltageV = 99.9f;
  st = dev.readMeasurement(m);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsAfterBegin, bus.readCalls);
}






void test_diag_alert_surfaces_and_preserves_accumulator_overflow_flags() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF |
                 cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF |
                 cmd::DIAG_MATHOF;

  DiagAlert diag{};
  Status st = dev.readDiagAlert(diag);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(diag.energyOF);
  TEST_ASSERT_TRUE(diag.chargeOF);
  TEST_ASSERT_TRUE(diag.mathOF);

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.valid);
  TEST_ASSERT_TRUE(snap.diag.energyOF);
  TEST_ASSERT_TRUE(snap.diag.chargeOF);
  TEST_ASSERT_TRUE(snap.diag.mathOF);
  TEST_ASSERT_TRUE((bus.diagAlrt & cmd::DIAG_ENERGYOF) != 0);
  TEST_ASSERT_TRUE((bus.diagAlrt & cmd::DIAG_CHARGEOF) != 0);
  TEST_ASSERT_TRUE((bus.diagAlrt & cmd::DIAG_MATHOF) != 0);
}






void test_read_raw_sample_uses_unsigned_vbus_and_energy() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  bus.reg24[cmd::REG_VBUS] = 0xFFF000;
  bus.reg40[cmd::REG_ENERGY] = 0xFFFFFFFFFFULL;

  RawSample raw{};
  Status st = dev.readRawSample(raw);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(0x000FFF00u, raw.vbus);
  TEST_ASSERT_EQUAL_UINT64(0xFFFFFFFFFFULL, raw.energy);
}

void test_read_integer_sample_uses_fixed_units_without_accumulators() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  loadPositiveMeasurementRegisters(bus);
  bus.diagAlrt = cmd::DIAG_MEMSTAT;
  clearReadHistory(bus);

  IntegerSample sample{};
  Status st = dev.readIntegerSample(sample);
  TEST_ASSERT_TRUE(st.ok());
  assertPositiveIntegerSample(sample);
  TEST_ASSERT_TRUE(sample.diagAlertValid);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_MEMSTAT, sample.diagAlertRaw);
  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_DIAG_ALRT));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_ENERGY));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_CHARGE));
}

void test_convert_raw_sample_uses_fixed_units_without_i2c() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  RawSample raw{};
  raw.vshunt = 0x4BF00;
  raw.vbus = 0x3C000;
  raw.dietemp = 0x0C80;
  raw.current = 0x4CCCC;
  raw.power = 0x48000C;
  raw.diagAlertValid = true;
  raw.diagAlertRaw = cmd::DIAG_MEMSTAT;

  const uint32_t readsBefore = bus.readCalls;
  const uint32_t writesBefore = bus.writeCalls;
  IntegerSample sample{};
  Status st = dev.convertRawSample(raw, sample);
  TEST_ASSERT_TRUE(st.ok());
  assertPositiveIntegerSample(sample);
  TEST_ASSERT_TRUE(sample.diagAlertValid);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_MEMSTAT, sample.diagAlertRaw);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);
}

void test_integer_sample_requires_calibration_and_preserves_output() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  const uint32_t readsBefore = bus.readCalls;

  IntegerSample sample{};
  sample.busMillivolts = 1234;
  Status st = dev.readIntegerSample(sample);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(1234u, sample.busMillivolts);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
}

void test_convert_raw_sample_reports_math_overflow_evidence() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  RawSample raw{};
  raw.diagAlertValid = true;
  raw.diagAlertRaw = cmd::DIAG_MEMSTAT | cmd::DIAG_MATHOF;

  IntegerSample sample{};
  sample.currentMilliamps = 77;
  Status st = dev.convertRawSample(raw, sample);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MATH_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_INT32(77, sample.currentMilliamps);
}

void test_repeated_current_math_overflow_clears_after_accumulator_reset() {
  FakeBus bus;
  bus.autoClearAccumulatorReset = true;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.reg24[cmd::REG_CURRENT] = 0x100000U;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_MATHOF;

  float current = 7.0f;
  TEST_ASSERT_TRUE(dev.readCurrent(current).is(Err::MATH_OVERFLOW));
  TEST_ASSERT_FLOAT_WITHIN(0.0f, 7.0f, current);
  TEST_ASSERT_TRUE(dev.readCurrent(current).is(Err::MATH_OVERFLOW));
  TEST_ASSERT_FLOAT_WITHIN(0.0f, 7.0f, current);

  TEST_ASSERT_TRUE(dev.resetAccumulators().ok());
  TEST_ASSERT_TRUE(dev.readCurrent(current).ok());
  TEST_ASSERT_TRUE(current > 0.0f);
}

void test_20bit_edge_vectors_and_low_nibble_masking() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  struct Case {
    uint32_t raw24;
    float shuntV;
    float currentA;
  };
  const Case cases[] = {
      {0x000010u, 0.0000003125f, 0.000019073486328125f},
      {0xFFFFF0u, -0.0000003125f, -0.000019073486328125f},
      {0x7FFFF0u, 0.1638396875f, 9.999980926513672f},
      {0x800000u, -0.16384f, -10.0f},
  };

  for (const Case& c : cases) {
    bus.reg24[cmd::REG_VSHUNT] = c.raw24;
    bus.reg24[cmd::REG_CURRENT] = c.raw24;

    float shunt = 0.0f;
    Status st = dev.readShuntVoltage(shunt);
    TEST_ASSERT_TRUE(st.ok());
    TEST_ASSERT_FLOAT_WITHIN(0.00000001f, c.shuntV, shunt);

    float current = 0.0f;
    st = dev.readCurrent(current);
    TEST_ASSERT_TRUE(st.ok());
    TEST_ASSERT_FLOAT_WITHIN(0.00002f, c.currentA, current);
  }

  bus.reg24[cmd::REG_CURRENT] = 0x00001Fu;
  RawSample raw{};
  Status st = dev.readRawSample(raw);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_INT32(1, raw.current);
}

void test_temperature_negative_and_positive_vectors() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  struct Case {
    uint16_t raw16;
    float tempC;
  };
  const Case cases[] = {
      {0xFFFFu, -0.0078125f},
      {0xF380u, -25.0f},
      {0xEC00u, -40.0f},
      {0x3E80u, 125.0f},
  };

  for (const Case& c : cases) {
    bus.reg16[cmd::REG_DIETEMP] = c.raw16;
    float temp = 0.0f;
    Status st = dev.readTemperature(temp);
    TEST_ASSERT_TRUE(st.ok());
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, c.tempC, temp);
  }
}



void test_read_raw_sample_failures_leave_output_unchanged() {
  const uint8_t regs[] = {
      cmd::REG_VSHUNT,
      cmd::REG_VBUS,
      cmd::REG_DIETEMP,
      cmd::REG_CURRENT,
      cmd::REG_POWER,
      cmd::REG_DIAG_ALRT,
      cmd::REG_ENERGY,
      cmd::REG_CHARGE,
  };

  for (uint8_t reg : regs) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
    dev.tick(bus.nowMs);
    loadPositiveMeasurementRegisters(bus);
    clearReadHistory(bus);
    queueNthReadFailure(bus, reg, 1);

    RawSample raw = sentinelRawSample();
    Status st = dev.readRawSample(raw);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    assertSentinelRawSample(raw);
  }
}





void test_poll_measurement_ready_delay_gate_and_diag_budget() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  Status st = dev.startTriggeredMeasurement(Mode::TRIG_ALL);
  TEST_ASSERT_TRUE(st.inProgress());
  clearReadHistory(bus);

  bool ready = true;
  st = dev.pollMeasurementReady(bus.nowMs, 1, ready);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_UINT32(0u, bus.readHistoryCount);

  bus.nowMs += dev.estimateConversionTimeMs();
  bus.diagAlrt = cmd::DIAG_MEMSTAT;
  st = dev.pollMeasurementReady(bus.nowMs, 1, ready);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_UINT32(1u, bus.readHistoryCount);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_DIAG_ALRT, bus.readHistory[0]);

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  st = dev.pollMeasurementReady(bus.nowMs, 1, ready);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(ready);
  TEST_ASSERT_EQUAL_UINT32(2u, bus.readHistoryCount);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_DIAG_ALRT, bus.readHistory[1]);
}












void test_signed_raw_register_vectors_sign_extend() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  bus.reg24[cmd::REG_VSHUNT] = 0xB41000;
  bus.reg24[cmd::REG_CURRENT] = 0x800000;
  bus.reg40[cmd::REG_CHARGE] = 0xFFFFFFFFFFULL;

  RawSample raw{};
  Status st = dev.readRawSample(raw);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_INT32(-311040, raw.vshunt);
  TEST_ASSERT_EQUAL_INT32(-524288, raw.current);
  TEST_ASSERT_EQUAL_INT64(-1, raw.charge);

  bus.reg24[cmd::REG_CURRENT] = 0xFFFFF0;
  bus.reg40[cmd::REG_CHARGE] = 0x8000000000ULL;
  st = dev.readRawSample(raw);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_INT32(-1, raw.current);
  TEST_ASSERT_EQUAL_INT64(-549755813888LL, raw.charge);
}

void test_datasheet_table_8_4_measurement_vectors_and_negative_shunt() {
  FakeBus bus;
  bus.autoClearAccumulatorReset = true;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  TEST_ASSERT_TRUE(dev.resetAccumulators().ok());

  bus.reg24[cmd::REG_VSHUNT] = 0xB41000U;
  float shuntVoltage = 0.0f;
  TEST_ASSERT_TRUE(dev.readShuntVoltage(shuntVoltage).ok());
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, -0.0972f, shuntVoltage);

  loadPositiveMeasurementRegisters(bus);
  bus.diagAlrt = cmd::DIAG_MEMSTAT;
  Measurement measurement{};
  TEST_ASSERT_TRUE(dev.readMeasurement(measurement).ok());
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.0972f, measurement.shuntVoltageV);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 48.0f, measurement.busVoltageV);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, measurement.temperatureC);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 6.0f, measurement.currentA);
  TEST_ASSERT_FLOAT_WITHIN(0.002f, 288.0f, measurement.powerW);
  TEST_ASSERT_TRUE(measurement.energyJ > 1036799.99 &&
                   measurement.energyJ < 1036800.01);
  TEST_ASSERT_TRUE(measurement.chargeC > 21599.99 &&
                   measurement.chargeC < 21600.01);
  TEST_ASSERT_TRUE(measurement.energyValid);
  TEST_ASSERT_TRUE(measurement.chargeValid);
}


void test_invalid_begin_calibration_configs_do_not_touch_i2c() {
  struct Case {
    float shunt;
    float maxCurrent;
  };
  const Case cases[] = {
      {0.0162f, 0.0f},
      {0.0f, 10.0f},
      {-0.0162f, 10.0f},
      {0.0162f, -10.0f},
      {std::numeric_limits<float>::quiet_NaN(), 10.0f},
      {0.0162f, std::numeric_limits<float>::infinity()},
      {1.0e-9f, 1.0e8f},
  };

  for (const Case& c : cases) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = c.shunt;
    cfg.maxExpectedCurrentA = c.maxCurrent;

    Status st = dev.begin(cfg);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FALSE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT32(0u, bus.readCalls);
    TEST_ASSERT_EQUAL_UINT32(0u, bus.writeCalls);
  }
}

void test_set_calibration_invalid_params_do_not_touch_i2c_or_cache() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  TEST_ASSERT_TRUE(dev.setCalibration(0.0162f, 10.0f).ok());
  const Config oldConfig = dev.getConfig();
  const float oldLsb = dev.currentLsb();
  const uint16_t oldReg = bus.reg16[cmd::REG_SHUNT_CAL];
  const uint32_t writesBefore = bus.writeCalls;

  struct Case {
    float shunt;
    float maxCurrent;
  };
  const Case cases[] = {
      {0.0f, 10.0f},
      {-0.0162f, 10.0f},
      {std::numeric_limits<float>::quiet_NaN(), 10.0f},
      {0.0162f, 0.0f},
      {0.0162f, -10.0f},
      {0.0162f, std::numeric_limits<float>::infinity()},
      {1.0e-9f, 1.0e8f},
  };

  for (const Case& c : cases) {
    Status st = dev.setCalibration(c.shunt, c.maxCurrent);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, oldConfig.shuntResistanceOhm,
                             dev.getConfig().shuntResistanceOhm);
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, oldConfig.maxExpectedCurrentA,
                             dev.getConfig().maxExpectedCurrentA);
    TEST_ASSERT_FLOAT_WITHIN(oldLsb * 0.0001f, oldLsb, dev.currentLsb());
    TEST_ASSERT_EQUAL_HEX16(oldReg, bus.reg16[cmd::REG_SHUNT_CAL]);
  }
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);
}

void test_set_calibration_write_failure_preserves_committed_scale() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  TEST_ASSERT_TRUE(dev.setCalibration(0.0162f, 10.0f).ok());

  const Config oldConfig = dev.getConfig();
  const float oldLsb = dev.currentLsb();
  const uint16_t oldReg = bus.reg16[cmd::REG_SHUNT_CAL];
  CalibrationPlan oldPlan{};
  TEST_ASSERT_TRUE(dev.getCalibrationPlan(oldPlan).ok());
  JobSnapshot oldJob{};
  TEST_ASSERT_TRUE(dev.getJobState(oldJob).ok());

  bus.writeError = Status::Error(Err::I2C_TIMEOUT, "forced calibration timeout", 91);
  bus.writeErrorRemaining = 1;
  const Status st = dev.setCalibration(0.020f, 5.0f);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_TIMEOUT),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_INT32(91, st.detail);
  TEST_ASSERT_EQUAL_STRING("forced calibration timeout", st.msg);

  TEST_ASSERT_FLOAT_WITHIN(0.000001f, oldConfig.shuntResistanceOhm,
                           dev.getConfig().shuntResistanceOhm);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, oldConfig.maxExpectedCurrentA,
                           dev.getConfig().maxExpectedCurrentA);
  TEST_ASSERT_FLOAT_WITHIN(oldLsb * 0.0001f, oldLsb, dev.currentLsb());
  TEST_ASSERT_EQUAL_HEX16(oldReg, bus.reg16[cmd::REG_SHUNT_CAL]);

  CalibrationPlan plan{};
  TEST_ASSERT_TRUE(dev.getCalibrationPlan(plan).ok());
  TEST_ASSERT_EQUAL_HEX16(oldPlan.shuntCal, plan.shuntCal);
  TEST_ASSERT_EQUAL_UINT32(oldPlan.selectedCurrentLsbNanoAmps,
                           plan.selectedCurrentLsbNanoAmps);
  TEST_ASSERT_EQUAL_UINT32(oldPlan.effectiveCurrentLsbNanoAmps,
                           plan.effectiveCurrentLsbNanoAmps);

  SettingsSnapshot settings{};
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_TRUE(settings.hardwareDirty);
  TEST_ASSERT_FALSE(settings.calibrated);
  TEST_ASSERT_TRUE((settings.dirtyRegisterMask &
                    (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0U);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                          static_cast<uint8_t>(dev.hardwareState()));
  JobSnapshot job{};
  TEST_ASSERT_TRUE(dev.getJobState(job).ok());
  TEST_ASSERT_EQUAL_UINT32(oldJob.configurationGeneration,
                           job.configurationGeneration);
}



void test_begin_uncalibrated_writes_shunt_cal_zero() {
  FakeBus bus;
  bus.reg16[cmd::REG_SHUNT_CAL] = cmd::SHUNT_CAL_RESET;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.calibrated);
  TEST_ASSERT_FALSE(snap.calibrationClamped);
  TEST_ASSERT_FALSE(snap.maxCurrentExceedsShuntRange);
  TEST_ASSERT_EQUAL_HEX16(0u, snap.shuntCal);
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_SHUNT_CAL]);
}







void test_cached_static_config_write_failures_mark_dirty_registers() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

    bus.writeErrorRemaining = 1;
    Status st = dev.setShuntTempCoeff(100);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT16(0u, dev.getConfig().shuntTempCoeffPpmC);

    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask &
                      (uint64_t{1} << cmd::REG_SHUNT_TEMPCO)) != 0);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

    bus.writeErrorRemaining = 1;
    Status st = dev.setTempCompensation(true);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FALSE(dev.getConfig().tempCompEnabled);

    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask &
                      (uint64_t{1} << cmd::REG_CONFIG)) != 0);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

    bus.writeErrorRemaining = 1;
    Status st = dev.setConversionDelay(5);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT8(0u, dev.getConfig().convDelayMs2);

    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask &
                      (uint64_t{1} << cmd::REG_CONFIG)) != 0);
  }
}


void test_threshold_setters_encode_exact_register_vectors() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  TEST_ASSERT_TRUE(dev.setShuntOvervoltageThreshold(0.010f).ok());
  TEST_ASSERT_EQUAL_HEX16(0x07D0u, bus.reg16[cmd::REG_SOVL]);

  TEST_ASSERT_TRUE(dev.setShuntUndervoltageThreshold(-0.010f).ok());
  TEST_ASSERT_EQUAL_HEX16(0xF830u, bus.reg16[cmd::REG_SUVL]);

  TEST_ASSERT_TRUE(dev.setBusOvervoltageThreshold(12.5f).ok());
  TEST_ASSERT_EQUAL_HEX16(0x0FA0u, bus.reg16[cmd::REG_BOVL]);
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_BOVL] & 0x8000u);

  TEST_ASSERT_TRUE(dev.setBusUndervoltageThreshold(1.25f).ok());
  TEST_ASSERT_EQUAL_HEX16(0x0190u, bus.reg16[cmd::REG_BUVL]);
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_BUVL] & 0x8000u);

  TEST_ASSERT_TRUE(dev.setTemperatureOverlimitThreshold(-25.0f).ok());
  TEST_ASSERT_EQUAL_HEX16(0xF380u, bus.reg16[cmd::REG_TEMP_LIMIT]);

  TEST_ASSERT_TRUE(dev.setPowerOverlimitThreshold(16.0f).ok());
  TEST_ASSERT_EQUAL_HEX16(0x0400u, bus.reg16[cmd::REG_PWR_LIMIT]);
}

void test_threshold_write_failures_preserve_registers() {
  struct Case {
    uint8_t reg;
    uint16_t resetValue;
    float value;
    Status (INA228::INA228::*setter)(float);
  };
  const Case cases[] = {
      {cmd::REG_SOVL, cmd::SOVL_RESET, 0.010f,
       &INA228::INA228::setShuntOvervoltageThreshold},
      {cmd::REG_SUVL, cmd::SUVL_RESET, -0.010f,
       &INA228::INA228::setShuntUndervoltageThreshold},
      {cmd::REG_BOVL, cmd::BOVL_RESET, 12.5f,
       &INA228::INA228::setBusOvervoltageThreshold},
      {cmd::REG_BUVL, cmd::BUVL_RESET, 1.25f,
       &INA228::INA228::setBusUndervoltageThreshold},
      {cmd::REG_TEMP_LIMIT, cmd::TEMP_LIMIT_RESET, -25.0f,
       &INA228::INA228::setTemperatureOverlimitThreshold},
      {cmd::REG_PWR_LIMIT, cmd::PWR_LIMIT_RESET, 16.0f,
       &INA228::INA228::setPowerOverlimitThreshold},
  };

  for (const Case& c : cases) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    bus.reg16[c.reg] = c.resetValue;
    bus.writeError = Status::Error(Err::I2C_ERROR,
                                   "forced threshold write error", -81);
    queueWriteFailure(bus, c.reg);

    Status st = (dev.*(c.setter))(c.value);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(-81, st.detail);
    TEST_ASSERT_EQUAL_HEX16(c.resetValue, bus.reg16[c.reg]);
    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_TRUE(settings.thresholdsDirty);
  }
}

void test_invalid_threshold_values_do_not_touch_bus() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  const uint32_t writesBefore = bus.writeCalls;

  Status st = dev.setBusOvervoltageThreshold(90.0f);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));
  st = dev.setTemperatureOverlimitThreshold(std::numeric_limits<float>::infinity());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);
}

// ===========================================================================
// Device identity
// ===========================================================================

void test_read_manufacturer_id() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  uint16_t id = 0;
  Status st = dev.readManufacturerId(id);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(0x5449, id);
}

void test_read_device_id() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  uint16_t id = 0;
  Status st = dev.readDeviceId(id);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(0x2281, id);
}

void test_public_register_access_helpers() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  bus.reg16[cmd::REG_SHUNT_TEMPCO] = 0xA5C3u;
  bus.reg24[cmd::REG_POWER] = 0x123456u;
  bus.reg40[cmd::REG_ENERGY] = 0x0102030405ULL;

  uint16_t mfgId = 0;
  Status st = dev.readRegister16(cmd::REG_MANUFACTURER_ID, mfgId);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(0x5449, mfgId);

  uint16_t tempco = 0;
  st = dev.readRegister16(cmd::REG_SHUNT_TEMPCO, tempco);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(0xA5C3u, tempco);

  uint32_t power = 0xFFFFFFFFu;
  st = dev.readRegister24(cmd::REG_POWER, power);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX32(0x123456u, power);

  uint64_t energy = UINT64_MAX;
  st = dev.readRegister40(cmd::REG_ENERGY, energy);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT64(0x0102030405ULL, energy);

  st = dev.writeRegister16(cmd::REG_SHUNT_TEMPCO, 0xBEEFu);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_SHUNT_TEMPCO, bus.lastWriteReg);
  TEST_ASSERT_EQUAL_HEX16(0xBEEFu, bus.lastWrite16);
  TEST_ASSERT_EQUAL_HEX16(0xBEEFu, bus.reg16[cmd::REG_SHUNT_TEMPCO]);
}

void test_raw_software_reset_marks_every_reset_register_dirty() {
  const uint8_t resetRegisters[] = {
      cmd::REG_CONFIG,       cmd::REG_ADC_CONFIG, cmd::REG_SHUNT_CAL,
      cmd::REG_SHUNT_TEMPCO, cmd::REG_DIAG_ALRT,  cmd::REG_SOVL,
      cmd::REG_SUVL,         cmd::REG_BOVL,       cmd::REG_BUVL,
      cmd::REG_TEMP_LIMIT,   cmd::REG_PWR_LIMIT};

  for (uint8_t ambiguous = 0; ambiguous < 2; ++ambiguous) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
    TEST_ASSERT_TRUE(dev.triggerConversion(Mode::TRIG_ALL).inProgress());
    if (ambiguous != 0U) {
      bus.applyThenFailWriteReg = cmd::REG_CONFIG;
      bus.applyThenFailWriteMatch =
          static_cast<uint8_t>(bus.writeMatchCount[cmd::REG_CONFIG] + 1U);
    }

    const Status st = dev.writeRegister16(cmd::REG_CONFIG, cmd::CONFIG_RST);
    if (ambiguous != 0U) {
      TEST_ASSERT_TRUE(st.is(Err::I2C_ERROR));
    } else {
      TEST_ASSERT_TRUE(st.ok());
    }

    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_TRUE(settings.hardwareDirty);
    TEST_ASSERT_TRUE(settings.thresholdsDirty);
    TEST_ASSERT_FALSE(settings.triggeredConversionPending);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    for (size_t index = 0;
         index < sizeof(resetRegisters) / sizeof(resetRegisters[0]); ++index) {
      TEST_ASSERT_TRUE((settings.dirtyRegisterMask &
                        (uint64_t{1} << resetRegisters[index])) != 0U);
    }

    TEST_ASSERT_TRUE(dev.recover().ok());
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_TRUE(settings.thresholdsDirty);
    TEST_ASSERT_FALSE(settings.hardwareDirty);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }
}

void test_raw_accumulator_register_read_does_not_pre_preserve_diag_alert() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF;
  bus.reg40[cmd::REG_ENERGY] = 0x0102030405ULL;

  DiagAlertSnapshot before{};
  Status st = dev.getDiagAlertSnapshot(before);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(before.valid);
  TEST_ASSERT_FALSE(before.diag.energyOF);
  TEST_ASSERT_FALSE(before.diag.chargeOF);

  uint64_t energy = 0;
  st = dev.readRegister40(cmd::REG_ENERGY, energy);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT64(0x0102030405ULL, energy);
  TEST_ASSERT_FALSE((bus.diagAlrt & cmd::DIAG_ENERGYOF) != 0);
  TEST_ASSERT_TRUE((bus.diagAlrt & cmd::DIAG_CHARGEOF) != 0);

  DiagAlertSnapshot after{};
  st = dev.getDiagAlertSnapshot(after);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(before.raw, after.raw);
  TEST_ASSERT_FALSE(after.diag.energyOF);
  TEST_ASSERT_FALSE(after.diag.chargeOF);
}

void test_public_register_access_preserves_transport_errors() {
  const Err transportCodes[] = {
      Err::I2C_NACK_ADDR,
      Err::I2C_NACK_DATA,
      Err::I2C_TIMEOUT,
      Err::I2C_BUS,
      Err::I2C_ERROR,
  };

  for (Err err : transportCodes) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
    bus.readErrorRemaining = 1;
    bus.readError = Status::Error(err, "forced read16 error", -51);
    uint16_t reg16 = 0;
    Status st = dev.readRegister16(cmd::REG_MANUFACTURER_ID, reg16);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(err),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(-51, st.detail);
  }

  for (Err err : transportCodes) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
    bus.readErrorRemaining = 1;
    bus.readError = Status::Error(err, "forced read24 error", -52);
    uint32_t reg24 = 0;
    Status st = dev.readRegister24(cmd::REG_POWER, reg24);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(err),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(-52, st.detail);
  }

  for (Err err : transportCodes) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
    bus.readErrorRemaining = 1;
    bus.readError = Status::Error(err, "forced read40 error", -53);
    uint64_t reg40 = 0;
    Status st = dev.readRegister40(cmd::REG_ENERGY, reg40);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(err),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(-53, st.detail);
  }

  for (Err err : transportCodes) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
    bus.writeErrorRemaining = 1;
    bus.writeError = Status::Error(err, "forced write16 error", -54);
    Status st = dev.writeRegister16(cmd::REG_SHUNT_TEMPCO, 0x1234);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(err),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(-54, st.detail);
  }
}


void test_register_access_after_end_does_not_touch_bus() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  const uint32_t writesAfterBegin = bus.writeCalls;
  const uint32_t readsAfterBegin = bus.readCalls;

  dev.end();
  TEST_ASSERT_EQUAL_UINT32(writesAfterBegin, bus.writeCalls);
  TEST_ASSERT_EQUAL_UINT32(readsAfterBegin, bus.readCalls);

  uint16_t mfgId = 0;
  Status st = dev.readRegister16(cmd::REG_MANUFACTURER_ID, mfgId);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::NOT_INITIALIZED),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsAfterBegin, bus.readCalls);

  uint32_t power = 0;
  st = dev.readRegister24(cmd::REG_POWER, power);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::NOT_INITIALIZED),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsAfterBegin, bus.readCalls);

  st = dev.writeRegister16(cmd::REG_CONFIG, 0);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::NOT_INITIALIZED),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(writesAfterBegin, bus.writeCalls);
}

// ===========================================================================
// Cooperative production-contract tests
// ===========================================================================

void test_cooperative_bind_is_zero_i2c_and_validates_contract() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);

  const uint32_t readsBefore = bus.readCalls;
  const uint32_t writesBefore = bus.writeCalls;
  TEST_ASSERT_TRUE(dev.bind(cfg).ok());
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::UNKNOWN),
                          static_cast<uint8_t>(dev.hardwareState()));
  TEST_ASSERT_FALSE(dev.isInitialized());

  CalibrationPlan plan{};
  TEST_ASSERT_TRUE(dev.getCalibrationPlan(plan).ok());
  TEST_ASSERT_EQUAL_HEX16(0x0666u, plan.shuntCal);

  struct InvalidCase {
    uint8_t kind;
  };
  const InvalidCase cases[] = {{0}, {1}, {2}, {3}, {4}, {5}};
  for (const InvalidCase& c : cases) {
    FakeBus invalidBus;
    INA228::INA228 invalidDev;
    Config invalid = makeCooperativeConfig(invalidBus);
    switch (c.kind) {
      case 0: invalid.i2cWrite = nullptr; break;
      case 1: invalid.i2cTimeoutMs = 0; break;
      case 2: invalid.i2cAddress = 0x50; break;
      case 3: invalid.supportedRevisionMask = 0; break;
      case 4:
        invalid.shuntResistanceOhm = 0.025f;
        invalid.maxExpectedCurrentA = 2.5f;
        break;
      case 5: invalid.calibration.currentLsbNanoAmps = 0; break;
    }
    const Status st = invalidDev.bind(invalid);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT32(0u, invalidBus.readCalls);
    TEST_ASSERT_EQUAL_UINT32(0u, invalidBus.writeCalls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::UNBOUND),
                            static_cast<uint8_t>(invalidDev.hardwareState()));
  }
}

void test_cooperative_job_limits_are_exact_and_retry_free() {
  FakeBus bus;
  INA228::INA228 dev;
  JobLimits limits{};
  Status st = dev.getJobLimits(JobKind::INITIALIZE, limits);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::NOT_BOUND),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());

  struct LimitCase {
    JobKind kind;
    uint16_t transfers;
    OperationClass operationClass;
  };
  const LimitCase cases[] = {
      {JobKind::INITIALIZE, 14, OperationClass::MULTI_STEP_RUNTIME},
      {JobKind::REINITIALIZE, 14, OperationClass::MULTI_STEP_RUNTIME},
      {JobKind::VERIFY_CONFIGURATION, 8, OperationClass::MULTI_STEP_RUNTIME},
      {JobKind::INSTANTANEOUS_SAMPLE, 11, OperationClass::STEADY_STATE},
      {JobKind::RESET, 16, OperationClass::MAINTENANCE},
      {JobKind::ACCUMULATOR_RESET, 2, OperationClass::MULTI_STEP_RUNTIME},
  };
  for (const LimitCase& c : cases) {
    TEST_ASSERT_TRUE(dev.getJobLimits(c.kind, limits).ok());
    TEST_ASSERT_EQUAL_UINT16(c.transfers, limits.maxTransfers);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(c.operationClass),
                            static_cast<uint8_t>(limits.operationClass));
    TEST_ASSERT_EQUAL_UINT8(0u, limits.maxRetries);
    if (c.kind == JobKind::INSTANTANEOUS_SAMPLE || c.kind == JobKind::RESET) {
      TEST_ASSERT_GREATER_THAN_UINT32(0u, limits.maxWaitMicroseconds);
    } else {
      TEST_ASSERT_EQUAL_UINT32(0u, limits.maxWaitMicroseconds);
    }
  }
  st = dev.getJobLimits(JobKind::NONE, limits);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(0u, bus.readCalls + bus.writeCalls);
}

void test_instantaneous_sample_wait_includes_bounded_device_timing_margin() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeCooperativeConfig(bus);
    cfg.vbusConvTime = ConvTime::US_50;
    cfg.vshuntConvTime = ConvTime::US_50;
    cfg.vtempConvTime = ConvTime::US_50;
    cfg.averaging = Averaging::AVG_1;
    cfg.convDelayMs2 = 255U;
    TEST_ASSERT_TRUE(dev.bind(cfg).ok());
    JobLimits limits{};
    TEST_ASSERT_TRUE(dev.getJobLimits(JobKind::INSTANTANEOUS_SAMPLE, limits).ok());
    // ceil((150 us conversions + 510 ms delay + 60 us wakeup) * 65/64),
    // then rounded up to the public millisecond polling boundary.
    TEST_ASSERT_EQUAL_UINT32(519000U, limits.maxWaitMicroseconds);
    TEST_ASSERT_EQUAL_UINT16(11U, limits.maxTransfers);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeCooperativeConfig(bus);
    cfg.vbusConvTime = ConvTime::US_4120;
    cfg.vshuntConvTime = ConvTime::US_4120;
    cfg.vtempConvTime = ConvTime::US_4120;
    cfg.averaging = Averaging::AVG_1024;
    cfg.convDelayMs2 = 255U;
    TEST_ASSERT_TRUE(dev.bind(cfg).ok());
    JobLimits limits{};
    TEST_ASSERT_TRUE(dev.getJobLimits(JobKind::INSTANTANEOUS_SAMPLE, limits).ok());
    TEST_ASSERT_EQUAL_UINT32(13373000U, limits.maxWaitMicroseconds);
    TEST_ASSERT_EQUAL_UINT16(11U, limits.maxTransfers);
  }
}

void test_fixed_calibration_plans_cover_tunnelmonitor_profile_and_strict_limits() {
  CalibrationConfig explicitPlan{};
  explicitPlan.shuntMicroOhms = 25000;
  explicitPlan.mode = CalibrationMode::EXPLICIT_CURRENT_LSB;
  explicitPlan.maxCurrentMilliAmps = 2500;
  explicitPlan.currentLsbNanoAmps = 5000;

  CalibrationPlan plan{};
  TEST_ASSERT_TRUE(INA228::INA228::calculateCalibration(
      explicitPlan, AdcRange::MV_163_84, plan).ok());
  TEST_ASSERT_EQUAL_HEX16(0x0666u, plan.shuntCal);
  TEST_ASSERT_EQUAL_UINT32(5000u, plan.selectedCurrentLsbNanoAmps);
  TEST_ASSERT_EQUAL_UINT32(4999u, plan.effectiveCurrentLsbNanoAmps);
  TEST_ASSERT_EQUAL_UINT32(2620u, plan.representableCurrentMilliAmps);
  TEST_ASSERT_EQUAL_UINT32(163840u, plan.shuntFullScaleMicrovolts);
  TEST_ASSERT_TRUE(plan.quantized);
  TEST_ASSERT_FALSE(plan.clamped);
  TEST_ASSERT_FALSE(plan.maxCurrentExceedsShuntRange);
  TEST_ASSERT_FALSE(plan.maxCurrentExceedsCurrentRegister);

  CalibrationConfig exampleProfile{};
  exampleProfile.shuntMicroOhms = 15000;
  exampleProfile.mode = CalibrationMode::FROM_MAXIMUM_CURRENT;
  exampleProfile.maxCurrentMilliAmps = 10000;
  TEST_ASSERT_TRUE(INA228::INA228::calculateCalibration(
      exampleProfile, AdcRange::MV_163_84, plan).ok());
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(
      exampleProfile.maxCurrentMilliAmps, plan.representableCurrentMilliAmps);
  TEST_ASSERT_FALSE(plan.clamped);
  TEST_ASSERT_FALSE(plan.maxCurrentExceedsShuntRange);
  TEST_ASSERT_FALSE(plan.maxCurrentExceedsCurrentRegister);

  CalibrationConfig derived = explicitPlan;
  derived.mode = CalibrationMode::FROM_MAXIMUM_CURRENT;
  derived.currentLsbNanoAmps = 0;
  TEST_ASSERT_TRUE(INA228::INA228::calculateCalibration(
      derived, AdcRange::MV_163_84, plan).ok());
  TEST_ASSERT_EQUAL_HEX16(0x061Bu, plan.shuntCal);
  TEST_ASSERT_EQUAL_UINT32(4769u, plan.selectedCurrentLsbNanoAmps);
  TEST_ASSERT_EQUAL_UINT32(4770u, plan.effectiveCurrentLsbNanoAmps);
  TEST_ASSERT_EQUAL_UINT32(2500u, plan.representableCurrentMilliAmps);

  CalibrationConfig unsafe = explicitPlan;
  unsafe.maxCurrentMilliAmps = 7000;
  Status st = INA228::INA228::calculateCalibration(
      unsafe, AdcRange::MV_163_84, plan);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
  unsafe.allowUnsafePlan = true;
  TEST_ASSERT_TRUE(INA228::INA228::calculateCalibration(
      unsafe, AdcRange::MV_163_84, plan).ok());
  TEST_ASSERT_TRUE(plan.maxCurrentExceedsShuntRange);

  CalibrationConfig underflow = explicitPlan;
  underflow.shuntMicroOhms = 1;
  underflow.currentLsbNanoAmps = 1;
  st = INA228::INA228::calculateCalibration(
      underflow, AdcRange::MV_163_84, plan);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));

  CalibrationConfig none{};
  none.shuntMicroOhms = 1;
  st = INA228::INA228::calculateCalibration(
      none, AdcRange::MV_163_84, plan);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));
}

void test_identity_parser_splits_die_and_revision_strictly() {
  DeviceIdentity identity{};
  TEST_ASSERT_TRUE(INA228::INA228::parseDeviceIdentity(
      0x5449, 0x2281, identity).ok());
  TEST_ASSERT_EQUAL_HEX16(0x5449u, identity.manufacturerId);
  TEST_ASSERT_EQUAL_HEX16(0x0228u, identity.dieId);
  TEST_ASSERT_EQUAL_UINT8(1u, identity.revision);

  TEST_ASSERT_TRUE(INA228::INA228::parseDeviceIdentity(
      0x5449, 0x228Fu, identity).ok());
  TEST_ASSERT_EQUAL_UINT8(15u, identity.revision);

  Status st = INA228::INA228::parseDeviceIdentity(0x0000, 0x2281, identity);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_ID_MISMATCH),
                          static_cast<uint8_t>(st.code));
  st = INA228::INA228::parseDeviceIdentity(0x5449, 0xF228, identity);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_ID_MISMATCH),
                          static_cast<uint8_t>(st.code));
}

void test_cooperative_initialize_budget_order_forwarding_and_alert_determinism() {
  FakeBus bus;
  bus.diagAlrt = cmd::DIAG_CONFIG_MASK | cmd::DIAG_MEMSTAT;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);
  cfg.shuntTempCoeffPpmC = 321;
  cfg.alerts.slowAlert = true;
  cfg.alerts.activeHigh = true;
  TEST_ASSERT_TRUE(dev.bind(cfg).ok());

  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startInitialize(0xA5A55A5Au, operationId).ok());
  Status st{Err::IN_PROGRESS, 0, "pending"};
  for (uint8_t phase = 0; phase < 14; ++phase) {
    const uint32_t before = bus.readCalls + bus.writeCalls;
    st = dev.pollJob(bus.nowMs, 1);
    TEST_ASSERT_EQUAL_UINT32(1u, bus.readCalls + bus.writeCalls - before);
    if (phase < 13) {
      TEST_ASSERT_TRUE(st.inProgress());
    }
  }
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(14u, bus.transferHistoryCount);

  const TransferKind kinds[] = {
      TransferKind::WRITE_READ, TransferKind::WRITE_READ, TransferKind::WRITE_READ,
      TransferKind::WRITE, TransferKind::WRITE, TransferKind::WRITE,
      TransferKind::WRITE, TransferKind::WRITE, TransferKind::WRITE,
      TransferKind::WRITE_READ, TransferKind::WRITE_READ, TransferKind::WRITE_READ,
      TransferKind::WRITE_READ, TransferKind::WRITE_READ,
  };
  const uint8_t regs[] = {
      cmd::REG_MANUFACTURER_ID, cmd::REG_DEVICE_ID, cmd::REG_DIAG_ALRT,
      cmd::REG_ADC_CONFIG, cmd::REG_CONFIG, cmd::REG_DIAG_ALRT,
      cmd::REG_SHUNT_TEMPCO, cmd::REG_SHUNT_CAL, cmd::REG_ADC_CONFIG,
      cmd::REG_CONFIG, cmd::REG_ADC_CONFIG, cmd::REG_SHUNT_CAL,
      cmd::REG_DIAG_ALRT, cmd::REG_SHUNT_TEMPCO,
  };
  for (size_t i = 0; i < sizeof(regs); ++i) {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(kinds[i]),
                            static_cast<uint8_t>(bus.transferHistory[i].kind));
    TEST_ASSERT_EQUAL_HEX8(regs[i], bus.transferHistory[i].reg);
    TEST_ASSERT_EQUAL_HEX8(0x41u, bus.transferHistory[i].address);
    TEST_ASSERT_EQUAL_UINT32(20u, bus.transferHistory[i].timeoutMs);
  }
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_SLOWALERT | cmd::DIAG_APOL,
                          bus.reg16[cmd::REG_DIAG_ALRT]);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_SLOWALERT | cmd::DIAG_APOL |
                          cmd::DIAG_MEMSTAT, bus.diagAlrt);
  TEST_ASSERT_EQUAL_HEX16(321u, bus.reg16[cmd::REG_SHUNT_TEMPCO]);
  TEST_ASSERT_EQUAL_HEX16(0x0666u, bus.reg16[cmd::REG_SHUNT_CAL]);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                          static_cast<uint8_t>(dev.hardwareState()));

  DeviceIdentity identity{};
  TEST_ASSERT_TRUE(dev.getDeviceIdentity(identity).ok());
  TEST_ASSERT_EQUAL_HEX16(0x0228u, identity.dieId);
  TEST_ASSERT_EQUAL_UINT8(1u, identity.revision);

  JobSnapshot snapshot{};
  TEST_ASSERT_TRUE(dev.getJobState(snapshot).ok());
  TEST_ASSERT_EQUAL_UINT32(operationId, snapshot.operationId);
  TEST_ASSERT_EQUAL_UINT32(0xA5A55A5Au, snapshot.requestToken);
  TEST_ASSERT_EQUAL_UINT16(14u, snapshot.transfersCompleted);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobState::SUCCEEDED),
                          static_cast<uint8_t>(snapshot.state));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::CONFIRMED),
                          static_cast<uint8_t>(snapshot.effect));
  TEST_ASSERT_TRUE(snapshot.resultAvailable);
}

void test_initialize_failure_injection_covers_every_transfer_stage() {
  struct FailureCase {
    TransferKind kind;
    uint8_t reg;
    uint8_t occurrence;
    JobEffect effect;
    HardwareState hardwareState;
  };
  const FailureCase cases[] = {
      {TransferKind::WRITE_READ, cmd::REG_MANUFACTURER_ID, 1, JobEffect::NONE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE_READ, cmd::REG_DEVICE_ID, 1, JobEffect::NONE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE_READ, cmd::REG_DIAG_ALRT, 1, JobEffect::INDETERMINATE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE, cmd::REG_ADC_CONFIG, 1, JobEffect::INDETERMINATE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE, cmd::REG_CONFIG, 1, JobEffect::INDETERMINATE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE, cmd::REG_DIAG_ALRT, 1, JobEffect::INDETERMINATE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE, cmd::REG_SHUNT_TEMPCO, 1, JobEffect::INDETERMINATE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE, cmd::REG_SHUNT_CAL, 1, JobEffect::INDETERMINATE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE, cmd::REG_ADC_CONFIG, 2, JobEffect::INDETERMINATE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE_READ, cmd::REG_CONFIG, 1, JobEffect::PARTIAL, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE_READ, cmd::REG_ADC_CONFIG, 1, JobEffect::PARTIAL, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE_READ, cmd::REG_SHUNT_CAL, 1, JobEffect::PARTIAL, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE_READ, cmd::REG_DIAG_ALRT, 2, JobEffect::INDETERMINATE, HardwareState::RESYNC_REQUIRED},
      {TransferKind::WRITE_READ, cmd::REG_SHUNT_TEMPCO, 1, JobEffect::PARTIAL, HardwareState::RESYNC_REQUIRED},
  };

  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    const FailureCase& c = cases[index];
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    if (c.kind == TransferKind::WRITE) {
      queueNthWriteFailure(bus, c.reg, c.occurrence);
    } else {
      queueNthReadFailure(bus, c.reg, c.occurrence);
    }
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInitialize(static_cast<uint32_t>(index + 1),
                                         operationId).ok());
    const Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT32(index + 1u, bus.transferHistoryCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(c.hardwareState),
                            static_cast<uint8_t>(dev.hardwareState()));

    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobState::FAILED),
                            static_cast<uint8_t>(result.job.state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(c.effect),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(index + 1u),
                             result.job.transfersCompleted);
    TEST_ASSERT_FALSE(result.hasInstantaneousSample);
  }
}

void test_initialize_readback_mismatch_and_ambiguous_effects_are_observable() {
  {
    FakeBus bus;
    bus.configReadOverrideRemaining = 1;
    bus.configReadOverrideValue = cmd::CONFIG_ADCRANGE;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInitialize(10, operationId).ok());
    Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::CONFIG_MISMATCH),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::PARTIAL),
                            static_cast<uint8_t>(result.job.effect));
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    bus.applyThenFailWriteReg = cmd::REG_SHUNT_CAL;
    bus.applyThenFailWriteMatch = 1;
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInitialize(11, operationId).ok());
    Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_HEX16(0x0666u, bus.reg16[cmd::REG_SHUNT_CAL]);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::INDETERMINATE),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    TEST_ASSERT_EQUAL_UINT8(1u, bus.writeMatchCount[cmd::REG_SHUNT_CAL]);
  }

  {
    FakeBus bus;
    bus.clearDiagOnRead = true;
    bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_BUSOL;
    bus.consumeThenFailReadReg = cmd::REG_DIAG_ALRT;
    bus.consumeThenFailReadMatch = 1;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInitialize(12, operationId).ok());
    Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_HEX16(0u, bus.diagAlrt & cmd::DIAG_BUSOL);
    DiagnosticEvents events{};
    TEST_ASSERT_TRUE(dev.getDiagnosticEvents(events).ok());
    TEST_ASSERT_FALSE(events.valid);
    TEST_ASSERT_EQUAL_HEX16(0u, events.stickyEvents);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::INDETERMINATE),
                            static_cast<uint8_t>(result.job.effect));
  }
}

void test_initialize_semantic_identity_and_memstat_failures_are_terminal() {
  enum class SemanticFault : uint8_t {
    MANUFACTURER,
    DIE,
    MEMSTAT
  };
  struct Case {
    SemanticFault fault;
    Err expected;
    uint16_t transfers;
  };
  const Case cases[] = {
      {SemanticFault::MANUFACTURER, Err::DEVICE_ID_MISMATCH, 2},
      {SemanticFault::DIE, Err::DEVICE_ID_MISMATCH, 2},
      {SemanticFault::MEMSTAT, Err::MEMORY_ERROR, 3},
  };
  for (const Case& c : cases) {
    FakeBus bus;
    if (c.fault == SemanticFault::MANUFACTURER) {
      bus.manufacturerId = 0x0000;
    } else if (c.fault == SemanticFault::DIE) {
      bus.deviceId = 0xF228;
    } else {
      bus.diagAlrt = 0;
    }
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInitialize(0x1D00u + c.transfers,
                                         operationId).ok());
    const Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(c.expected),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT16(c.transfers,
                             static_cast<uint16_t>(bus.transferHistoryCount));
    TEST_ASSERT_EQUAL_UINT32(0u, bus.writeCalls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobState::FAILED),
                            static_cast<uint8_t>(result.job.state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::NONE),
                            static_cast<uint8_t>(result.job.effect));
  }
}

void test_reinitialize_identity_commit_cancel_and_invalidation_are_atomic() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus);

  DeviceIdentity identity{};
  TEST_ASSERT_TRUE(dev.getDeviceIdentity(identity).ok());
  TEST_ASSERT_EQUAL_HEX16(0x0228u, identity.dieId);
  TEST_ASSERT_EQUAL_UINT8(1u, identity.revision);

  clearTransferHistory(bus);
  clearReadHistory(bus);
  resetWriteTracking(bus);
  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startReinitialize(0xA100u, operationId).ok());
  TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 1).inProgress());
  TEST_ASSERT_EQUAL_UINT32(1u, bus.transferHistoryCount);
  TEST_ASSERT_TRUE(dev.cancelJob().is(Err::CANCELLED));
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::NONE),
                          static_cast<uint8_t>(result.job.effect));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                          static_cast<uint8_t>(dev.hardwareState()));
  TEST_ASSERT_FALSE(dev.isInitialized());
  TEST_ASSERT_TRUE(dev.getDeviceIdentity(identity).is(Err::HARDWARE_STATE_UNKNOWN));
  float busVoltage = 0.0f;
  TEST_ASSERT_TRUE(dev.readBusVoltage(busVoltage).is(Err::NOT_INITIALIZED));

  TEST_ASSERT_TRUE(dev.startReinitialize(0xA101u, operationId).ok());
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_TRUE(dev.getDeviceIdentity(identity).ok());

  bus.manufacturerId = 0x0000;
  TEST_ASSERT_TRUE(dev.startReinitialize(0xA102u, operationId).ok());
  Status st = pollCooperativeToTerminal(dev, bus);
  TEST_ASSERT_TRUE(st.is(Err::DEVICE_ID_MISMATCH));
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::NONE),
                          static_cast<uint8_t>(result.job.effect));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                          static_cast<uint8_t>(dev.hardwareState()));
  TEST_ASSERT_FALSE(dev.isInitialized());
  TEST_ASSERT_TRUE(dev.getDeviceIdentity(identity).is(Err::HARDWARE_STATE_UNKNOWN));

  bus.manufacturerId = cmd::MANUFACTURER_ID;
  TEST_ASSERT_TRUE(dev.startReinitialize(0xA103u, operationId).ok());
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_TRUE(dev.startInstantaneousSample(0xA104u, operationId).ok());
  const Status removal = Status::Error(Err::I2C_NACK_ADDR,
                                       "device removed during active job");
  TEST_ASSERT_TRUE(dev.invalidateHardwareState(removal).is(Err::I2C_NACK_ADDR));
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobState::FAILED),
                          static_cast<uint8_t>(result.job.state));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                          static_cast<uint8_t>(dev.hardwareState()));
  TEST_ASSERT_FALSE(dev.isInitialized());
  TEST_ASSERT_TRUE(dev.getDeviceIdentity(identity).is(Err::HARDWARE_STATE_UNKNOWN));
}

void test_reinitialize_semantic_failures_revoke_verified_identity() {
  enum class Fault : uint8_t { DIE, REVISION, MEMSTAT };
  struct Case {
    Fault fault;
    Err expected;
    uint16_t transfers;
  };
  const Case cases[] = {
      {Fault::DIE, Err::DEVICE_ID_MISMATCH, 2},
      {Fault::REVISION, Err::UNSUPPORTED_REVISION, 2},
      {Fault::MEMSTAT, Err::MEMORY_ERROR, 3},
  };

  for (const Case& c : cases) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    DeviceIdentity identity{};
    TEST_ASSERT_TRUE(dev.getDeviceIdentity(identity).ok());
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);
    if (c.fault == Fault::DIE) {
      bus.deviceId = 0xF228;
    } else if (c.fault == Fault::REVISION) {
      bus.deviceId = 0x2282;
    } else {
      bus.diagAlrt = 0;
    }
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startReinitialize(0xA200u + c.transfers,
                                           operationId).ok());
    const Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_TRUE(st.is(c.expected));
    TEST_ASSERT_EQUAL_UINT16(c.transfers,
                             static_cast<uint16_t>(bus.transferHistoryCount));
    TEST_ASSERT_EQUAL_UINT32(0u, bus.writeAttemptCount);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::NONE),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    TEST_ASSERT_FALSE(dev.isInitialized());
    TEST_ASSERT_TRUE(dev.getDeviceIdentity(identity).is(Err::HARDWARE_STATE_UNKNOWN));
  }
}

void test_verify_configuration_failure_injection_covers_every_transfer_stage() {
  struct FailureCase {
    uint8_t reg;
    uint8_t occurrence;
    JobEffect effect;
  };
  const FailureCase cases[] = {
      {cmd::REG_MANUFACTURER_ID, 1, JobEffect::NONE},
      {cmd::REG_DEVICE_ID, 1, JobEffect::NONE},
      {cmd::REG_DIAG_ALRT, 1, JobEffect::INDETERMINATE},
      {cmd::REG_CONFIG, 1, JobEffect::NONE},
      {cmd::REG_ADC_CONFIG, 1, JobEffect::NONE},
      {cmd::REG_SHUNT_CAL, 1, JobEffect::NONE},
      {cmd::REG_DIAG_ALRT, 2, JobEffect::INDETERMINATE},
      {cmd::REG_SHUNT_TEMPCO, 1, JobEffect::NONE},
  };

  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);
    queueNthReadFailure(bus, cases[index].reg, cases[index].occurrence);
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startVerifyConfiguration(
        static_cast<uint32_t>(0xB100u + index), operationId).ok());
    const Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_TRUE(st.is(Err::I2C_ERROR));
    TEST_ASSERT_EQUAL_UINT32(index + 1u, bus.transferHistoryCount);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(cases[index].effect),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                            static_cast<uint8_t>(dev.hardwareState()));
    TEST_ASSERT_TRUE(dev.isInitialized());
  }
}

void test_verify_configuration_is_read_only_bounded_and_generation_stable() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus);
  clearTransferHistory(bus);
  clearReadHistory(bus);
  resetWriteTracking(bus);

  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startVerifyConfiguration(0x0F00u, operationId).ok());
  const Status st = pollCooperativeToTerminal(dev, bus);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(8u, bus.transferHistoryCount);
  TEST_ASSERT_EQUAL_UINT32(0u, bus.writeAttemptCount);
  const uint8_t regs[] = {
      cmd::REG_MANUFACTURER_ID, cmd::REG_DEVICE_ID, cmd::REG_DIAG_ALRT,
      cmd::REG_CONFIG, cmd::REG_ADC_CONFIG, cmd::REG_SHUNT_CAL,
      cmd::REG_DIAG_ALRT, cmd::REG_SHUNT_TEMPCO,
  };
  for (size_t i = 0; i < sizeof(regs); ++i) {
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransferKind::WRITE_READ),
                            static_cast<uint8_t>(bus.transferHistory[i].kind));
    TEST_ASSERT_EQUAL_HEX8(regs[i], bus.transferHistory[i].reg);
  }
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT32(result.job.startConfigurationGeneration,
                           result.job.configurationGeneration);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::CONFIRMED),
                          static_cast<uint8_t>(result.job.effect));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                          static_cast<uint8_t>(dev.hardwareState()));
}

void test_verify_configuration_distinguishes_inconclusive_transport_from_disproof() {
  {
    FakeBus bus;
    bus.autoClearAccumulatorReset = true;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    TEST_ASSERT_TRUE(dev.resetAccumulators().ok());
    bus.reg40[cmd::REG_ENERGY] = 100U;
    bus.diagAlrt = cmd::DIAG_MEMSTAT;

    bus.readError = Status::Error(Err::I2C_TIMEOUT,
                                  "transient verification timeout", -55);
    queueNthReadFailure(bus, cmd::REG_MANUFACTURER_ID,
                        static_cast<uint8_t>(bus.readMatchCount[
                            cmd::REG_MANUFACTURER_ID] + 1U));
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startVerifyConfiguration(0x4401U, operationId).ok());
    TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).is(Err::I2C_TIMEOUT));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_TRUE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                            static_cast<uint8_t>(dev.hardwareState()));
    double energy = 0.0;
    TEST_ASSERT_TRUE(dev.readEnergy(energy).ok());
    TEST_ASSERT_TRUE(energy > 0.0);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    TEST_ASSERT_TRUE(dev.triggerConversion(Mode::TRIG_ALL).inProgress());
    bus.reg16[cmd::REG_CONFIG] ^=
        static_cast<uint16_t>(1U << cmd::BIT_CONFIG_CONVDLY);

    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startVerifyConfiguration(0x4402U, operationId).ok());
    TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).is(Err::CONFIG_MISMATCH));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_FALSE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_FALSE(settings.triggeredConversionPending);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    bus.readError = Status::Error(Err::I2C_NACK_ADDR,
                                  "device absent during verification", -56);
    queueNthReadFailure(bus, cmd::REG_MANUFACTURER_ID,
                        static_cast<uint8_t>(bus.readMatchCount[
                            cmd::REG_MANUFACTURER_ID] + 1U));
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startVerifyConfiguration(0x4403U, operationId).ok());
    TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).is(Err::DEVICE_NOT_FOUND));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_FALSE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    bus.readError = Status::Error(Err::I2C_NACK_ADDR,
                                  "device disappeared mid-verification", -57);
    queueNthReadFailure(bus, cmd::REG_DEVICE_ID,
                        static_cast<uint8_t>(bus.readMatchCount[
                            cmd::REG_DEVICE_ID] + 1U));
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startVerifyConfiguration(0x4404U, operationId).ok());
    TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).is(Err::I2C_NACK_ADDR));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_FALSE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    bus.readError = Status::Error(Err::I2C_NACK_UNKNOWN_PHASE,
                                  "phase unavailable mid-verification", -58);
    queueNthReadFailure(bus, cmd::REG_DEVICE_ID,
                        static_cast<uint8_t>(bus.readMatchCount[
                            cmd::REG_DEVICE_ID] + 1U));
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startVerifyConfiguration(0x4405U, operationId).ok());
    TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).is(
        Err::I2C_NACK_UNKNOWN_PHASE));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_TRUE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }
}

void test_job_identity_exactly_once_and_rebinding_do_not_leak_context() {
  FakeBus busA;
  FakeBus busB;
  INA228::INA228 dev;
  Config cfgA = makeCooperativeConfig(busA);
  Config cfgB = makeCooperativeConfig(busB);
  cfgB.i2cAddress = 0x4F;
  TEST_ASSERT_TRUE(dev.bind(cfgA).ok());

  uint32_t firstId = 0;
  TEST_ASSERT_TRUE(dev.startInitialize(0x11112222u, firstId).ok());
  TEST_ASSERT_NOT_EQUAL(0u, firstId);
  JobResult result = {};
  Status st = dev.takeJobResult(firstId, result);
  TEST_ASSERT_TRUE(st.inProgress());
  st = dev.takeJobResult(firstId + 1u, result);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::STALE_RESULT),
                          static_cast<uint8_t>(st.code));

  const uint32_t transfersBeforeBusyBind = busA.readCalls + busA.writeCalls;
  st = dev.bind(cfgB);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(transfersBeforeBusyBind,
                           busA.readCalls + busA.writeCalls);

  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, busA).ok());
  SettingsSnapshot settingsBeforeTerminalTake{};
  TEST_ASSERT_TRUE(dev.getSettings(settingsBeforeTerminalTake).ok());
  const HardwareState hardwareStateBeforeTerminalTake = dev.hardwareState();
  const uint32_t transfersBeforeTerminalTake = busA.readCalls + busA.writeCalls;
  float busVoltage = -1.0f;
  st = dev.readBusVoltage(busVoltage);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  st = dev.writeRegister16(cmd::REG_CONFIG, 0);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(transfersBeforeTerminalTake,
                           busA.readCalls + busA.writeCalls);
  SettingsSnapshot settingsAfterRejectedAccess{};
  TEST_ASSERT_TRUE(dev.getSettings(settingsAfterRejectedAccess).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(hardwareStateBeforeTerminalTake),
                          static_cast<uint8_t>(dev.hardwareState()));
  TEST_ASSERT_EQUAL(settingsBeforeTerminalTake.hardwareDirty,
                    settingsAfterRejectedAccess.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(settingsBeforeTerminalTake.dirtyRegisterMask,
                           settingsAfterRejectedAccess.dirtyRegisterMask);

  st = dev.bind(cfgB);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  st = dev.takeJobResult(firstId + 1u, result);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::STALE_RESULT),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE(dev.takeJobResult(firstId, result).ok());
  TEST_ASSERT_EQUAL_UINT32(firstId, result.job.operationId);
  TEST_ASSERT_EQUAL_UINT32(0x11112222u, result.job.requestToken);
  TEST_ASSERT_FALSE(result.hasInstantaneousSample);
  st = dev.takeJobResult(firstId, result);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::RESULT_NOT_AVAILABLE),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE(dev.readBusVoltage(busVoltage).ok());
  TEST_ASSERT_EQUAL_UINT32(transfersBeforeTerminalTake + 1u,
                           busA.readCalls + busA.writeCalls);

  const uint32_t busATransfers = busA.readCalls + busA.writeCalls;
  TEST_ASSERT_TRUE(dev.bind(cfgB).ok());
  TEST_ASSERT_EQUAL_UINT32(0u, busB.readCalls + busB.writeCalls);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::UNKNOWN),
                          static_cast<uint8_t>(dev.hardwareState()));

  uint32_t secondId = 0;
  TEST_ASSERT_TRUE(dev.startInitialize(0x33334444u, secondId).ok());
  TEST_ASSERT_NOT_EQUAL(firstId, secondId);
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, busB).ok());
  TEST_ASSERT_EQUAL_UINT32(busATransfers, busA.readCalls + busA.writeCalls);
  TEST_ASSERT_EQUAL_UINT32(14u, busB.transferHistoryCount);
  for (size_t i = 0; i < busB.transferHistoryCount; ++i) {
    TEST_ASSERT_EQUAL_HEX8(0x4Fu, busB.transferHistory[i].address);
    TEST_ASSERT_EQUAL_UINT32(20u, busB.transferHistory[i].timeoutMs);
  }
  TEST_ASSERT_TRUE(dev.takeJobResult(secondId, result).ok());
  TEST_ASSERT_EQUAL_UINT32(0x33334444u, result.job.requestToken);
}

void test_cancel_and_timeout_are_bus_silent_with_precise_effects() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInitialize(1, operationId).ok());
    const uint32_t before = bus.readCalls + bus.writeCalls;
    Status st = dev.cancelJob();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::CANCELLED),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT32(before, bus.readCalls + bus.writeCalls);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobState::CANCELLED),
                            static_cast<uint8_t>(result.job.state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::NONE),
                            static_cast<uint8_t>(result.job.effect));
    st = dev.cancelJob();
    TEST_ASSERT_TRUE(st.ok());
    TEST_ASSERT_TRUE(dev.timeoutJob().ok());
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInitialize(2, operationId).ok());
    TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 4).inProgress());
    const uint32_t before = bus.readCalls + bus.writeCalls;
    Status st = dev.cancelJob();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::CANCELLED),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT32(before, bus.readCalls + bus.writeCalls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::PARTIAL),
                            static_cast<uint8_t>(result.job.effect));
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInstantaneousSample(3, operationId).ok());
    TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 3).inProgress());
    const uint32_t before = bus.readCalls + bus.writeCalls;
    Status st = dev.timeoutJob();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::OPERATION_TIMEOUT),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT32(before, bus.readCalls + bus.writeCalls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobState::TIMED_OUT),
                            static_cast<uint8_t>(result.job.state));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::PARTIAL),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_FALSE(result.hasInstantaneousSample);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startVerifyConfiguration(4, operationId).ok());
    TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 2).inProgress());
    const uint32_t before = bus.readCalls + bus.writeCalls;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::CANCELLED),
                            static_cast<uint8_t>(dev.cancelJob().code));
    TEST_ASSERT_EQUAL_UINT32(before, bus.readCalls + bus.writeCalls);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                            static_cast<uint8_t>(dev.hardwareState()));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::NONE),
                            static_cast<uint8_t>(result.job.effect));
  }
}

void test_triggered_sample_sequence_wait_budget_and_atomic_result() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus, 100);
  loadPositiveMeasurementRegisters(bus);
  clearReadHistory(bus);
  resetWriteTracking(bus);
  clearTransferHistory(bus);

  JobLimits limits{};
  TEST_ASSERT_TRUE(dev.getJobLimits(JobKind::INSTANTANEOUS_SAMPLE, limits).ok());
  const uint32_t waitMs = (limits.maxWaitMicroseconds + 999U) / 1000U;
  bus.successfulWriteDurationReg = cmd::REG_ADC_CONFIG;
  bus.successfulWriteDurationMatch = 1U;
  bus.advanceNowMsOnWrite = 3U;
  const uint32_t preWriteMs = bus.nowMs;
  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startInstantaneousSample(0x12345678u, operationId).ok());
  TEST_ASSERT_TRUE(dev.pollJob(preWriteMs, 3).inProgress());
  const uint32_t postWriteMs = bus.nowMs;
  TEST_ASSERT_EQUAL_UINT32(preWriteMs + 3U, postWriteMs);
  TEST_ASSERT_EQUAL_UINT32(3u, bus.transferHistoryCount);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransferKind::WRITE),
                          static_cast<uint8_t>(bus.transferHistory[2].kind));
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG, bus.transferHistory[2].reg);

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  const uint32_t beforeWait = bus.readCalls + bus.writeCalls;
  TEST_ASSERT_TRUE(dev.pollJob(preWriteMs + waitMs, 1).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeWait, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_TRUE(dev.pollJob(postWriteMs + waitMs - 1U, 1).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeWait, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_TRUE(dev.pollJob(postWriteMs + waitMs, 0).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeWait, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_TRUE(dev.pollJob(postWriteMs + waitMs, 1).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeWait + 1u, bus.readCalls + bus.writeCalls);

  bus.nowMs = postWriteMs + waitMs;
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  TEST_ASSERT_EQUAL_UINT32(11u, bus.transferHistoryCount);
  const uint8_t regs[] = {
      cmd::REG_CONFIG, cmd::REG_SHUNT_CAL, cmd::REG_ADC_CONFIG,
      cmd::REG_DIAG_ALRT, cmd::REG_VSHUNT, cmd::REG_VBUS,
      cmd::REG_DIETEMP, cmd::REG_CURRENT, cmd::REG_POWER,
      cmd::REG_ADC_CONFIG, cmd::REG_ADC_CONFIG,
  };
  const TransferKind kinds[] = {
      TransferKind::WRITE_READ, TransferKind::WRITE_READ, TransferKind::WRITE,
      TransferKind::WRITE_READ, TransferKind::WRITE_READ, TransferKind::WRITE_READ,
      TransferKind::WRITE_READ, TransferKind::WRITE_READ, TransferKind::WRITE_READ,
      TransferKind::WRITE, TransferKind::WRITE_READ,
  };
  for (size_t i = 0; i < sizeof(regs); ++i) {
    TEST_ASSERT_EQUAL_HEX8(regs[i], bus.transferHistory[i].reg);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(kinds[i]),
                            static_cast<uint8_t>(bus.transferHistory[i].kind));
  }

  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_TRUE(result.hasInstantaneousSample);
  const InstantaneousSample& sample = result.instantaneousSample;
  TEST_ASSERT_EQUAL_UINT32(operationId, sample.operationId);
  TEST_ASSERT_EQUAL_UINT32(0x12345678u, sample.requestToken);
  TEST_ASSERT_EQUAL_UINT32(postWriteMs + waitMs, sample.capturedAtMs);
  TEST_ASSERT_EQUAL_HEX16(0x001Fu, sample.validChannels);
  TEST_ASSERT_EQUAL_INT32(0x4BF00, sample.raw.vshunt);
  TEST_ASSERT_EQUAL_UINT32(0x3C000u, sample.raw.vbus);
  TEST_ASSERT_EQUAL_INT16(0x0C80, sample.raw.dietemp);
  TEST_ASSERT_EQUAL_INT32(0x4CCCC, sample.raw.current);
  TEST_ASSERT_EQUAL_UINT32(0x48000Cu, sample.raw.power);
  TEST_ASSERT_FALSE(sample.raw.energyValid);
  TEST_ASSERT_FALSE(sample.raw.chargeValid);
  TEST_ASSERT_TRUE(sample.raw.diagAlertValid);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF,
                          sample.raw.diagAlertRaw);
  TEST_ASSERT_EQUAL_INT32(97200, sample.values.shuntMicrovolts);
  TEST_ASSERT_EQUAL_UINT32(48000u, sample.values.busMillivolts);
  TEST_ASSERT_EQUAL_INT32(25000, sample.values.dieTemperatureMilliC);
  TEST_ASSERT_EQUAL_INT32(1572, sample.values.currentMilliamps);
  TEST_ASSERT_EQUAL_UINT32(75479u, sample.values.powerMilliwatts);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                          static_cast<uint8_t>(dev.hardwareState()));
}

void test_triggered_sample_wait_is_uint32_wrap_safe() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);
  cfg.vbusConvTime = ConvTime::US_50;
  cfg.vshuntConvTime = ConvTime::US_50;
  cfg.vtempConvTime = ConvTime::US_50;
  TEST_ASSERT_TRUE(dev.bind(cfg).ok());
  (void)initializeCooperativeDevice(dev, bus);
  loadPositiveMeasurementRegisters(bus);
  clearTransferHistory(bus);
  clearReadHistory(bus);
  resetWriteTracking(bus);

  bus.nowMs = std::numeric_limits<uint32_t>::max();
  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startInstantaneousSample(55, operationId).ok());
  TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 3).inProgress());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  const uint32_t before = bus.readCalls + bus.writeCalls;
  TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 1).inProgress());
  TEST_ASSERT_EQUAL_UINT32(before, bus.readCalls + bus.writeCalls);
  bus.nowMs = 0;
  TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 1).inProgress());
  TEST_ASSERT_EQUAL_UINT32(before + 1u, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_TRUE(result.hasInstantaneousSample);
  TEST_ASSERT_EQUAL_UINT32(0u, result.instantaneousSample.capturedAtMs);
}

void test_triggered_sample_without_hook_anchors_on_later_bus_silent_poll() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);
  cfg.nowMs = nullptr;
  cfg.timeUser = nullptr;
  TEST_ASSERT_TRUE(dev.bind(cfg).ok());
  (void)initializeCooperativeDevice(dev, bus);
  loadPositiveMeasurementRegisters(bus);
  clearReadHistory(bus);
  resetWriteTracking(bus);
  clearTransferHistory(bus);

  JobLimits limits{};
  TEST_ASSERT_TRUE(dev.getJobLimits(JobKind::INSTANTANEOUS_SAMPLE, limits).ok());
  const uint32_t waitMs = (limits.maxWaitMicroseconds + 999U) / 1000U;
  bus.nowMs = 7000U;
  bus.successfulWriteDurationReg = cmd::REG_ADC_CONFIG;
  bus.successfulWriteDurationMatch = 1U;
  bus.advanceNowMsOnWrite = 3U;
  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startInstantaneousSample(0x3301U, operationId).ok());
  TEST_ASSERT_TRUE(dev.pollJob(7000U, 8U).inProgress());
  TEST_ASSERT_EQUAL_UINT32(7003U, bus.nowMs);
  TEST_ASSERT_EQUAL_UINT32(3U, bus.transferHistoryCount);

  const uint32_t beforeAnchor = bus.readCalls + bus.writeCalls;
  const uint32_t anchorMs = std::numeric_limits<uint32_t>::max() - 1U;
  TEST_ASSERT_TRUE(dev.pollJob(anchorMs, 0U).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeAnchor, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_TRUE(dev.pollJob(anchorMs + waitMs - 1U, 5U).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeAnchor, bus.readCalls + bus.writeCalls);

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  TEST_ASSERT_TRUE(dev.pollJob(anchorMs + waitMs, 1U).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeAnchor + 1U, bus.readCalls + bus.writeCalls);
  bus.nowMs = anchorMs + waitMs;
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  TEST_ASSERT_EQUAL_UINT32(11U, bus.transferHistoryCount);
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_TRUE(result.hasInstantaneousSample);
  TEST_ASSERT_EQUAL_HEX16(0x001FU,
                          result.instantaneousSample.validChannels);
}

void test_hookless_wait_deferral_is_cleared_by_cancel_and_timeout() {
  for (uint8_t jobKind = 0; jobKind < 2U; ++jobKind) {
    for (uint8_t timedOut = 0; timedOut < 2U; ++timedOut) {
      FakeBus bus;
      INA228::INA228 dev;
      Config cfg = makeCooperativeConfig(bus);
      cfg.nowMs = nullptr;
      cfg.timeUser = nullptr;
      TEST_ASSERT_TRUE(dev.bind(cfg).ok());
      (void)initializeCooperativeDevice(dev, bus);
      clearTransferHistory(bus);
      clearReadHistory(bus);
      resetWriteTracking(bus);

      uint32_t operationId = 0;
      const Status started = jobKind == 0U
          ? dev.startInstantaneousSample(0x3400U + timedOut, operationId)
          : dev.startReset(0x3410U + timedOut, operationId);
      TEST_ASSERT_TRUE(started.ok());
      TEST_ASSERT_TRUE(dev.pollJob(8000U, 8U).inProgress());
      TEST_ASSERT_EQUAL_UINT32(jobKind == 0U ? 3U : 1U,
                               bus.transferHistoryCount);
      const uint32_t transfersBeforeCancel = bus.readCalls + bus.writeCalls;
      const Status terminal = timedOut == 0U
          ? dev.cancelJob()
          : dev.timeoutJob();
      TEST_ASSERT_TRUE(terminal.is(timedOut == 0U
                                      ? Err::CANCELLED
                                      : Err::OPERATION_TIMEOUT));
      TEST_ASSERT_EQUAL_UINT32(transfersBeforeCancel,
                               bus.readCalls + bus.writeCalls);
      JobResult result{};
      TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
      TEST_ASSERT_EQUAL_UINT8(
          static_cast<uint8_t>(timedOut == 0U ? JobState::CANCELLED
                                              : JobState::TIMED_OUT),
          static_cast<uint8_t>(result.job.state));
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::PARTIAL),
                              static_cast<uint8_t>(result.job.effect));

      TEST_ASSERT_TRUE(dev.startReinitialize(
          0x3500U + static_cast<uint32_t>(jobKind) * 0x10U + timedOut,
          operationId).ok());
      const uint32_t beforeNextJob = bus.readCalls + bus.writeCalls;
      TEST_ASSERT_TRUE(dev.pollJob(9000U, 1U).inProgress());
      TEST_ASSERT_EQUAL_UINT32(beforeNextJob + 1U,
                               bus.readCalls + bus.writeCalls);
      TEST_ASSERT_TRUE(dev.cancelJob().is(Err::CANCELLED));
      TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    }
  }
}

void test_hookless_positive_budget_anchor_is_bus_silent() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);
  cfg.nowMs = nullptr;
  cfg.timeUser = nullptr;
  TEST_ASSERT_TRUE(dev.bind(cfg).ok());
  (void)initializeCooperativeDevice(dev, bus);
  clearTransferHistory(bus);
  clearReadHistory(bus);
  resetWriteTracking(bus);

  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startInstantaneousSample(0x3501U, operationId).ok());
  TEST_ASSERT_TRUE(dev.pollJob(10000U, 8U).inProgress());
  TEST_ASSERT_EQUAL_UINT32(3U, bus.transferHistoryCount);
  const uint32_t beforeAnchor = bus.readCalls + bus.writeCalls;
  TEST_ASSERT_TRUE(dev.pollJob(11000U, 8U).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeAnchor, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_TRUE(dev.cancelJob().is(Err::CANCELLED));
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
}

void test_sample_diagnostic_failures_restore_adc_and_preserve_correlated_evidence() {
  struct DiagCase {
    uint16_t diag;
    Err expected;
    HardwareState hardwareState;
  };
  const DiagCase cases[] = {
      {cmd::DIAG_MEMSTAT, Err::MEASUREMENT_NOT_READY, HardwareState::SYNCHRONIZED},
      {cmd::DIAG_CNVRF, Err::MEMORY_ERROR, HardwareState::RESYNC_REQUIRED},
      {cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF | cmd::DIAG_MATHOF,
       Err::MATH_OVERFLOW, HardwareState::SYNCHRONIZED},
  };

  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);

    uint32_t operationId = 0;
    const uint32_t token = static_cast<uint32_t>(0x6000u + index);
    TEST_ASSERT_TRUE(dev.startInstantaneousSample(token, operationId).ok());
    TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 3).inProgress());
    bus.diagAlrt = cases[index].diag;
    JobLimits limits{};
    TEST_ASSERT_TRUE(dev.getJobLimits(JobKind::INSTANTANEOUS_SAMPLE, limits).ok());
    bus.nowMs += (limits.maxWaitMicroseconds + 999U) / 1000U;
    const Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(cases[index].expected),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(cases[index].hardwareState),
                            static_cast<uint8_t>(dev.hardwareState()));
    TEST_ASSERT_EQUAL_UINT32(6u, bus.transferHistoryCount);
    TEST_ASSERT_EQUAL_HEX8(cmd::REG_DIAG_ALRT, bus.transferHistory[3].reg);
    TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG, bus.transferHistory[4].reg);
    TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG, bus.transferHistory[5].reg);

    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    if (cases[index].expected == Err::MEMORY_ERROR) {
      SettingsSnapshot settings{};
      TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
      TEST_ASSERT_TRUE(settings.hardwareDirty);
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MEMORY_ERROR),
                              static_cast<uint8_t>(settings.hardwareDirtyCause.code));
      TEST_ASSERT_FALSE(settings.initialized);
    }
    TEST_ASSERT_TRUE(result.hasInstantaneousSample);
    TEST_ASSERT_EQUAL_UINT32(operationId,
                             result.instantaneousSample.operationId);
    TEST_ASSERT_EQUAL_UINT32(token, result.instantaneousSample.requestToken);
    TEST_ASSERT_EQUAL_HEX16(cases[index].diag,
                            result.instantaneousSample.raw.diagAlertRaw);
    TEST_ASSERT_EQUAL_HEX16(0u, result.instantaneousSample.validChannels);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::CONFIRMED),
                            static_cast<uint8_t>(result.job.effect));
  }
}

void test_sample_failure_injection_covers_every_transfer_stage_without_retry() {
  struct FailureCase {
    TransferKind kind;
    uint8_t reg;
    uint8_t occurrence;
    JobEffect effect;
  };
  const FailureCase cases[] = {
      {TransferKind::WRITE_READ, cmd::REG_CONFIG, 1, JobEffect::NONE},
      {TransferKind::WRITE_READ, cmd::REG_SHUNT_CAL, 1, JobEffect::NONE},
      {TransferKind::WRITE, cmd::REG_ADC_CONFIG, 1, JobEffect::INDETERMINATE},
      {TransferKind::WRITE_READ, cmd::REG_DIAG_ALRT, 1, JobEffect::INDETERMINATE},
      {TransferKind::WRITE_READ, cmd::REG_VSHUNT, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_VBUS, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_DIETEMP, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_CURRENT, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_POWER, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE, cmd::REG_ADC_CONFIG, 2, JobEffect::INDETERMINATE},
      {TransferKind::WRITE_READ, cmd::REG_ADC_CONFIG, 1, JobEffect::PARTIAL},
  };

  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    loadPositiveMeasurementRegisters(bus);
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);
    const FailureCase& c = cases[index];
    if (c.kind == TransferKind::WRITE) {
      queueNthWriteFailure(bus, c.reg, c.occurrence);
    } else {
      queueNthReadFailure(bus, c.reg, c.occurrence);
    }

    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInstantaneousSample(
        static_cast<uint32_t>(0x7000u + index), operationId).ok());
    Status st{Err::IN_PROGRESS, 0, "pending"};
    for (uint8_t poll = 0; poll < 40 && st.inProgress(); ++poll) {
      const uint32_t before = bus.readCalls + bus.writeCalls;
      st = dev.pollJob(bus.nowMs, 1);
      const uint32_t used = bus.readCalls + bus.writeCalls - before;
      TEST_ASSERT_LESS_OR_EQUAL_UINT32(1u, used);
      if (bus.transferHistoryCount == 3u && st.inProgress()) {
        bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
      }
      if (st.inProgress() && used == 0) {
        ++bus.nowMs;
      }
    }
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT32(index + 1u, bus.transferHistoryCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(c.effect),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(index + 1u),
                             result.job.transfersCompleted);
    if (index >= 4u) {
      TEST_ASSERT_TRUE(result.hasInstantaneousSample);
      TEST_ASSERT_EQUAL_UINT32(operationId,
                               result.instantaneousSample.operationId);
      TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(0x7000u + index),
                               result.instantaneousSample.requestToken);
      TEST_ASSERT_EQUAL_HEX16(0u, result.instantaneousSample.validChannels);
    } else {
      TEST_ASSERT_FALSE(result.hasInstantaneousSample);
    }
  }
}

void test_active_job_excludes_other_hardware_apis_but_allows_cache_access() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus);

  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startVerifyConfiguration(77, operationId).ok());
  const uint32_t before = bus.readCalls + bus.writeCalls;
  float scalar = 0.0f;
  uint16_t raw16 = 0;
  DiagAlert diag{};
  uint32_t competingId = 0;

  Status st = dev.readBusVoltage(scalar);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_STRING("Cooperative job owns hardware access", st.msg);
  st = dev.setMode(Mode::CONT_ALL);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  st = dev.triggerConversion(Mode::TRIG_ALL);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  st = dev.readDiagAlert(diag);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  st = dev.readRegister16(cmd::REG_CONFIG, raw16);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  st = dev.writeRegister16(cmd::REG_CONFIG, 0);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  st = dev.probe();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  st = dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  st = dev.startReset(78, competingId);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(before, bus.readCalls + bus.writeCalls);

  JobSnapshot snapshot{};
  DiagnosticEvents events{};
  SettingsSnapshot settings{};
  TEST_ASSERT_TRUE(dev.getJobState(snapshot).ok());
  TEST_ASSERT_TRUE(dev.getDiagnosticEvents(events).ok());
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_EQUAL_UINT32(operationId, snapshot.operationId);
  TEST_ASSERT_EQUAL_UINT32(before, bus.readCalls + bus.writeCalls);

  TEST_ASSERT_TRUE(dev.cancelJob().is(Err::CANCELLED));
  JobResult result{};
  st = dev.readBusVoltage(scalar);
  TEST_ASSERT_TRUE(st.is(Err::BUSY));
  TEST_ASSERT_EQUAL_STRING("Unconsumed terminal result; call takeJobResult()", st.msg);
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
}

void test_invalidate_and_configuration_guard_require_verified_reappearance() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus);
  const uint32_t beforeInvalidate = bus.readCalls + bus.writeCalls;
  const Status removal = Status::Error(Err::I2C_NACK_ADDR, "sensor removed", 41);
  TEST_ASSERT_TRUE(dev.invalidateHardwareState(removal).ok());
  TEST_ASSERT_EQUAL_UINT32(beforeInvalidate, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                          static_cast<uint8_t>(dev.hardwareState()));
  TEST_ASSERT_FALSE(dev.isInitialized());

  uint32_t operationId = 0;
  Status st = dev.startInstantaneousSample(1, operationId);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_STATE_UNKNOWN),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(beforeInvalidate, bus.readCalls + bus.writeCalls);

  bus.reg16[cmd::REG_CONFIG] = cmd::CONFIG_ADCRANGE;
  bus.reg16[cmd::REG_ADC_CONFIG] = cmd::ADC_CONFIG_RESET;
  bus.reg16[cmd::REG_SHUNT_CAL] = cmd::SHUNT_CAL_RESET;
  TEST_ASSERT_TRUE(dev.startReinitialize(2, operationId).ok());
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                          static_cast<uint8_t>(dev.hardwareState()));
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_CONFIG] & cmd::CONFIG_ADCRANGE);
  TEST_ASSERT_EQUAL_HEX16(0x0666u, bus.reg16[cmd::REG_SHUNT_CAL]);

  bus.reg16[cmd::REG_SHUNT_CAL] = 0x0001;
  clearTransferHistory(bus);
  clearReadHistory(bus);
  resetWriteTracking(bus);
  TEST_ASSERT_TRUE(dev.startInstantaneousSample(3, operationId).ok());
  st = pollCooperativeToTerminal(dev, bus);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::CONFIG_MISMATCH),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(2u, bus.transferHistoryCount);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                          static_cast<uint8_t>(dev.hardwareState()));
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::NONE),
                          static_cast<uint8_t>(result.job.effect));
  TEST_ASSERT_FALSE(result.hasInstantaneousSample);
}

void test_diagnostic_event_acknowledgement_and_caller_timestamps_are_deterministic() {
  FakeBus bus;
  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_BUSOL;
  bus.nowMs = 1234;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus);

  DiagnosticEvents events{};
  TEST_ASSERT_TRUE(dev.getDiagnosticEvents(events).ok());
  TEST_ASSERT_TRUE(events.valid);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_MEMSTAT, events.latestRaw);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_BUSOL, events.stickyEvents);
  TEST_ASSERT_EQUAL_HEX16(0u, events.newlyObservedEvents);
  TEST_ASSERT_EQUAL_UINT32(1234u, events.firstObservedAtMs[4]);

  const uint32_t beforeAck = bus.readCalls + bus.writeCalls;
  TEST_ASSERT_TRUE(dev.acknowledgeDiagnosticEvents(cmd::DIAG_BUSOL).ok());
  TEST_ASSERT_EQUAL_UINT32(beforeAck, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_TRUE(dev.getDiagnosticEvents(events).ok());
  TEST_ASSERT_EQUAL_HEX16(0u, events.stickyEvents);
  TEST_ASSERT_EQUAL_UINT32(0u, events.firstObservedAtMs[4]);

  loadPositiveMeasurementRegisters(bus);
  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startInstantaneousSample(0xD1A6u, operationId).ok());
  const uint32_t sampleStart = 7000;
  TEST_ASSERT_TRUE(dev.pollJob(sampleStart, 3).inProgress());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF | cmd::DIAG_POL;
  JobLimits limits{};
  TEST_ASSERT_TRUE(dev.getJobLimits(JobKind::INSTANTANEOUS_SAMPLE, limits).ok());
  const uint32_t observedAt = sampleStart +
      (limits.maxWaitMicroseconds + 999U) / 1000U;
  TEST_ASSERT_TRUE(dev.pollJob(observedAt, 1).inProgress());
  bus.nowMs = observedAt;
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  TEST_ASSERT_TRUE(dev.getDiagnosticEvents(events).ok());
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_POL | cmd::DIAG_CNVRF,
                          events.stickyEvents);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_POL | cmd::DIAG_CNVRF,
                          events.newlyObservedEvents);
  TEST_ASSERT_EQUAL_UINT32(observedAt, events.observedAtMs);
  TEST_ASSERT_EQUAL_UINT32(observedAt, events.firstObservedAtMs[2]);

  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_TRUE(result.hasInstantaneousSample);
  TEST_ASSERT_EQUAL_UINT32(observedAt,
                           result.instantaneousSample.diagnostics.observedAtMs);
  TEST_ASSERT_TRUE(dev.acknowledgeDiagnosticEvents(0xFFFFu).ok());
  TEST_ASSERT_TRUE(dev.getDiagnosticEvents(events).ok());
  TEST_ASSERT_EQUAL_HEX16(0u, events.stickyEvents);
  TEST_ASSERT_EQUAL_HEX16(0u, events.newlyObservedEvents);
}

void test_accumulator_epoch_requires_verified_reset_after_each_generation() {
  FakeBus bus;
  bus.autoClearAccumulatorReset = true;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus);
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);

  double energy = -1.0;
  const uint32_t beforeInvalidRead = bus.readCalls;
  Status st = dev.readEnergy(energy);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_INVALID),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(beforeInvalidRead, bus.readCalls);

  uint32_t resetId = 0;
  TEST_ASSERT_TRUE(dev.startAccumulatorReset(0xACC0u, resetId).ok());
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  JobResult resetResult{};
  TEST_ASSERT_TRUE(dev.takeJobResult(resetId, resetResult).ok());
  TEST_ASSERT_EQUAL_UINT16(2u, resetResult.job.transfersCompleted);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::CONFIRMED),
                          static_cast<uint8_t>(resetResult.job.effect));

  bus.reg40[cmd::REG_ENERGY] = 100;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);
  TEST_ASSERT_TRUE(dev.readEnergy(energy).ok());
  TEST_ASSERT_TRUE(energy > 0.0);

  uint32_t reinitializeId = 0;
  TEST_ASSERT_TRUE(dev.startReinitialize(0xBEEFu, reinitializeId).ok());
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(reinitializeId, result).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);
  const uint32_t beforeSecondInvalidRead = bus.readCalls;
  st = dev.readEnergy(energy);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_INVALID),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(beforeSecondInvalidRead, bus.readCalls);
}

void test_accumulator_reset_clears_obsolete_snapshot_evidence() {
  FakeBus bus;
  bus.autoClearAccumulatorReset = true;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  constexpr uint16_t ACCUMULATOR_EVIDENCE =
      cmd::DIAG_CNVRF | cmd::DIAG_ENERGYOF |
      cmd::DIAG_CHARGEOF | cmd::DIAG_MATHOF;
  constexpr uint16_t RETAINED_EVIDENCE = cmd::DIAG_MEMSTAT | cmd::DIAG_BUSOL;
  bus.diagAlrt = RETAINED_EVIDENCE | ACCUMULATOR_EVIDENCE;
  uint16_t raw = 0;
  TEST_ASSERT_TRUE(dev.readDiagAlertRaw(raw).ok());
  TEST_ASSERT_EQUAL_HEX16(RETAINED_EVIDENCE | ACCUMULATOR_EVIDENCE, raw);

  DiagAlertSnapshot snapshot{};
  TEST_ASSERT_TRUE(dev.getDiagAlertSnapshot(snapshot).ok());
  TEST_ASSERT_EQUAL_HEX16(ACCUMULATOR_EVIDENCE,
                          snapshot.raw & ACCUMULATOR_EVIDENCE);
  TEST_ASSERT_EQUAL_HEX16(RETAINED_EVIDENCE,
                          snapshot.raw & RETAINED_EVIDENCE);
  TEST_ASSERT_TRUE(snapshot.valid);
  const uint32_t capturedMs = snapshot.capturedMs;
  DiagnosticEvents events{};
  TEST_ASSERT_TRUE(dev.getDiagnosticEvents(events).ok());
  TEST_ASSERT_EQUAL_HEX16(ACCUMULATOR_EVIDENCE,
                          events.stickyEvents & ACCUMULATOR_EVIDENCE);

  TEST_ASSERT_TRUE(dev.resetAccumulators().ok());
  TEST_ASSERT_TRUE(dev.getDiagAlertSnapshot(snapshot).ok());
  TEST_ASSERT_EQUAL_HEX16(0U, snapshot.raw & ACCUMULATOR_EVIDENCE);
  TEST_ASSERT_EQUAL_HEX16(RETAINED_EVIDENCE,
                          snapshot.raw & RETAINED_EVIDENCE);
  TEST_ASSERT_TRUE(snapshot.valid);
  TEST_ASSERT_EQUAL_UINT32(capturedMs, snapshot.capturedMs);
  TEST_ASSERT_TRUE(snapshot.diag.memstat);
  TEST_ASSERT_TRUE(snapshot.diag.busOL);
  TEST_ASSERT_TRUE(dev.getDiagnosticEvents(events).ok());
  TEST_ASSERT_EQUAL_HEX16(ACCUMULATOR_EVIDENCE,
                          events.stickyEvents & ACCUMULATOR_EVIDENCE);
}

void test_accumulator_reset_cancel_and_ambiguous_write_require_resync() {
  {
    FakeBus bus;
    bus.autoClearAccumulatorReset = true;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startAccumulatorReset(1, operationId).ok());
    TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 1).inProgress());
    const uint32_t before = bus.readCalls + bus.writeCalls;
    TEST_ASSERT_TRUE(dev.cancelJob().is(Err::CANCELLED));
    TEST_ASSERT_EQUAL_UINT32(before, bus.readCalls + bus.writeCalls);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::PARTIAL),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }

  {
    FakeBus bus;
    bus.autoClearAccumulatorReset = true;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    resetWriteTracking(bus);
    bus.applyThenFailWriteReg = cmd::REG_CONFIG;
    bus.applyThenFailWriteMatch = 1;
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startAccumulatorReset(2, operationId).ok());
    const Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT8(1u, bus.writeMatchCount[cmd::REG_CONFIG]);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::INDETERMINATE),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }
}

void test_reset_and_accumulator_failure_injection_cover_every_transfer_stage() {
  struct FailureCase {
    TransferKind kind;
    uint8_t reg;
    uint8_t occurrence;
    JobEffect effect;
  };
  const FailureCase resetCases[] = {
      {TransferKind::WRITE, cmd::REG_CONFIG, 1, JobEffect::INDETERMINATE},
      {TransferKind::WRITE_READ, cmd::REG_CONFIG, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_MANUFACTURER_ID, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_DEVICE_ID, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_DIAG_ALRT, 1, JobEffect::INDETERMINATE},
      {TransferKind::WRITE, cmd::REG_ADC_CONFIG, 1, JobEffect::INDETERMINATE},
      {TransferKind::WRITE, cmd::REG_CONFIG, 2, JobEffect::INDETERMINATE},
      {TransferKind::WRITE, cmd::REG_DIAG_ALRT, 1, JobEffect::INDETERMINATE},
      {TransferKind::WRITE, cmd::REG_SHUNT_TEMPCO, 1, JobEffect::INDETERMINATE},
      {TransferKind::WRITE, cmd::REG_SHUNT_CAL, 1, JobEffect::INDETERMINATE},
      {TransferKind::WRITE, cmd::REG_ADC_CONFIG, 2, JobEffect::INDETERMINATE},
      {TransferKind::WRITE_READ, cmd::REG_CONFIG, 2, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_ADC_CONFIG, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_SHUNT_CAL, 1, JobEffect::PARTIAL},
      {TransferKind::WRITE_READ, cmd::REG_DIAG_ALRT, 2, JobEffect::INDETERMINATE},
      {TransferKind::WRITE_READ, cmd::REG_SHUNT_TEMPCO, 1, JobEffect::PARTIAL},
  };

  for (size_t index = 0;
       index < sizeof(resetCases) / sizeof(resetCases[0]); ++index) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);
    const FailureCase& c = resetCases[index];
    if (c.kind == TransferKind::WRITE) {
      queueNthWriteFailure(bus, c.reg, c.occurrence);
    } else {
      queueNthReadFailure(bus, c.reg, c.occurrence);
    }
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startReset(static_cast<uint32_t>(0xC100u + index),
                                    operationId).ok());
    const Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_TRUE(st.is(Err::I2C_ERROR));
    TEST_ASSERT_EQUAL_UINT32(index + 1u, bus.transferHistoryCount);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(c.effect),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    TEST_ASSERT_FALSE(dev.isInitialized());
  }

  for (uint8_t stage = 0; stage < 2; ++stage) {
    FakeBus bus;
    bus.autoClearAccumulatorReset = true;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);
    if (stage == 0) {
      queueNthWriteFailure(bus, cmd::REG_CONFIG, 1);
    } else {
      queueNthReadFailure(bus, cmd::REG_CONFIG, 1);
    }
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startAccumulatorReset(0xCA00u + stage,
                                                operationId).ok());
    const Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_TRUE(st.is(Err::I2C_ERROR));
    TEST_ASSERT_EQUAL_UINT32(stage + 1u, bus.transferHistoryCount);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    const JobEffect expected = stage == 0 ? JobEffect::INDETERMINATE
                                          : JobEffect::PARTIAL;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(expected),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }
}

void test_reset_job_wait_and_zero_budget_are_wrap_safe_and_bus_silent() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus);
  clearTransferHistory(bus);
  clearReadHistory(bus);
  resetWriteTracking(bus);

  bus.nowMs = std::numeric_limits<uint32_t>::max();
  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startReset(0x5151u, operationId).ok());
  TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 1).inProgress());
  TEST_ASSERT_EQUAL_UINT32(1u, bus.transferHistoryCount);
  const uint32_t beforeWait = bus.readCalls + bus.writeCalls;
  TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 0).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeWait, bus.readCalls + bus.writeCalls);
  bus.nowMs = 0;
  TEST_ASSERT_TRUE(dev.pollJob(bus.nowMs, 0).inProgress());
  TEST_ASSERT_EQUAL_UINT32(beforeWait, bus.readCalls + bus.writeCalls);
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  TEST_ASSERT_EQUAL_UINT32(16u, bus.transferHistoryCount);
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_EQUAL_UINT16(16u, result.job.transfersCompleted);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::CONFIRMED),
                          static_cast<uint8_t>(result.job.effect));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                          static_cast<uint8_t>(dev.hardwareState()));
}

void test_reset_wait_origin_is_post_write_with_and_without_hook() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);

    bus.nowMs = 2000U;
    bus.successfulWriteDurationReg = cmd::REG_CONFIG;
    bus.successfulWriteDurationMatch = 1U;
    bus.advanceNowMsOnWrite = 3U;
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startReset(0x3601U, operationId).ok());
    TEST_ASSERT_TRUE(dev.pollJob(2000U, 1U).inProgress());
    const uint32_t postWriteMs = bus.nowMs;
    TEST_ASSERT_EQUAL_UINT32(2003U, postWriteMs);
    const uint32_t beforeWait = bus.readCalls + bus.writeCalls;
    TEST_ASSERT_TRUE(dev.pollJob(postWriteMs, 1U).inProgress());
    TEST_ASSERT_EQUAL_UINT32(beforeWait, bus.readCalls + bus.writeCalls);
    TEST_ASSERT_TRUE(dev.pollJob(postWriteMs + 1U, 1U).inProgress());
    TEST_ASSERT_EQUAL_UINT32(beforeWait + 1U, bus.readCalls + bus.writeCalls);
    bus.nowMs = postWriteMs + 1U;
    TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
    TEST_ASSERT_EQUAL_UINT32(16U, bus.transferHistoryCount);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT16(16U, result.job.transfersCompleted);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::CONFIRMED),
                            static_cast<uint8_t>(result.job.effect));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeCooperativeConfig(bus);
    cfg.nowMs = nullptr;
    cfg.timeUser = nullptr;
    TEST_ASSERT_TRUE(dev.bind(cfg).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);

    bus.nowMs = 3000U;
    bus.successfulWriteDurationReg = cmd::REG_CONFIG;
    bus.successfulWriteDurationMatch = 1U;
    bus.advanceNowMsOnWrite = 3U;
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startReset(0x3602U, operationId).ok());
    TEST_ASSERT_TRUE(dev.pollJob(3000U, 8U).inProgress());
    TEST_ASSERT_EQUAL_UINT32(3003U, bus.nowMs);
    TEST_ASSERT_EQUAL_UINT32(1U, bus.transferHistoryCount);
    const uint32_t beforeAnchor = bus.readCalls + bus.writeCalls;
    const uint32_t anchorMs = std::numeric_limits<uint32_t>::max();
    TEST_ASSERT_TRUE(dev.pollJob(anchorMs, 0U).inProgress());
    TEST_ASSERT_EQUAL_UINT32(beforeAnchor, bus.readCalls + bus.writeCalls);
    TEST_ASSERT_TRUE(dev.pollJob(anchorMs, 8U).inProgress());
    TEST_ASSERT_EQUAL_UINT32(beforeAnchor, bus.readCalls + bus.writeCalls);
    TEST_ASSERT_TRUE(dev.pollJob(0U, 1U).inProgress());
    TEST_ASSERT_EQUAL_UINT32(beforeAnchor + 1U, bus.readCalls + bus.writeCalls);
    bus.nowMs = 0U;
    TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
    TEST_ASSERT_EQUAL_UINT32(16U, bus.transferHistoryCount);
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT16(16U, result.job.transfersCompleted);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::CONFIRMED),
                            static_cast<uint8_t>(result.job.effect));
  }
}

void test_revision_policy_and_declared_alert_defaults_are_verified() {
  {
    FakeBus bus;
    bus.deviceId = 0x2282;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInitialize(1, operationId).ok());
    Status st = pollCooperativeToTerminal(dev, bus);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::UNSUPPORTED_REVISION),
                            static_cast<uint8_t>(st.code));
    JobResult result{};
    TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobEffect::NONE),
                            static_cast<uint8_t>(result.job.effect));
  }

  {
    FakeBus bus;
    bus.deviceId = 0x2282;
    bus.diagAlrt = cmd::DIAG_CONFIG_MASK | cmd::DIAG_MEMSTAT;
    INA228::INA228 dev;
    Config cfg = makeCooperativeConfig(bus);
    cfg.supportedRevisionMask = static_cast<uint16_t>(1U << 2);
    TEST_ASSERT_TRUE(dev.bind(cfg).ok());
    (void)initializeCooperativeDevice(dev, bus);
    DeviceIdentity identity{};
    TEST_ASSERT_TRUE(dev.getDeviceIdentity(identity).ok());
    TEST_ASSERT_EQUAL_UINT8(2u, identity.revision);
    TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_DIAG_ALRT]);
    TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_MEMSTAT, bus.diagAlrt);
  }
}

void test_retained_configuration_setters_preserve_success_and_failure_contracts() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeCooperativeConfig(bus);
    cfg.calibration.maxCurrentMilliAmps = 1000;
    TEST_ASSERT_TRUE(dev.bind(cfg).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);

    TEST_ASSERT_TRUE(dev.setVbusConvTime(ConvTime::US_50).ok());
    TEST_ASSERT_TRUE(dev.setVshuntConvTime(ConvTime::US_84).ok());
    TEST_ASSERT_TRUE(dev.setTempConvTime(ConvTime::US_150).ok());
    TEST_ASSERT_TRUE(dev.setAveraging(Averaging::AVG_4).ok());
    TEST_ASSERT_EQUAL_UINT32(4u, bus.transferHistoryCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_50),
                            static_cast<uint8_t>(dev.getConfig().vbusConvTime));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_84),
                            static_cast<uint8_t>(dev.getConfig().vshuntConvTime));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_150),
                            static_cast<uint8_t>(dev.getConfig().vtempConvTime));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Averaging::AVG_4),
                            static_cast<uint8_t>(dev.getConfig().averaging));

    TEST_ASSERT_TRUE(dev.setAdcRange(AdcRange::MV_40_96).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcRange::MV_40_96),
                            static_cast<uint8_t>(dev.getConfig().adcRange));
    CalibrationPlan plan{};
    TEST_ASSERT_TRUE(dev.getCalibrationPlan(plan).ok());
    TEST_ASSERT_EQUAL_HEX16(0x199Au, plan.shuntCal);
    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_FALSE(settings.hardwareDirty);
    TEST_ASSERT_TRUE(settings.thresholdsDirty);

    const uint32_t beforeInvalid = bus.readCalls + bus.writeCalls;
    TEST_ASSERT_TRUE(dev.setAveraging(static_cast<Averaging>(0xFF))
                         .is(Err::INVALID_PARAM));
    TEST_ASSERT_EQUAL_UINT32(beforeInvalid, bus.readCalls + bus.writeCalls);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);
    queueNthWriteFailure(bus, cmd::REG_ADC_CONFIG, 1);
    TEST_ASSERT_TRUE(dev.setAveraging(Averaging::AVG_4).is(Err::I2C_ERROR));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Averaging::AVG_1),
                            static_cast<uint8_t>(dev.getConfig().averaging));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_TRUE(settings.hardwareDirty);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeCooperativeConfig(bus);
    cfg.calibration.maxCurrentMilliAmps = 1000;
    TEST_ASSERT_TRUE(dev.bind(cfg).ok());
    (void)initializeCooperativeDevice(dev, bus);
    const uint16_t oldConfig = bus.reg16[cmd::REG_CONFIG];
    clearTransferHistory(bus);
    clearReadHistory(bus);
    resetWriteTracking(bus);
    queueNthWriteFailure(bus, cmd::REG_SHUNT_CAL, 1);
    TEST_ASSERT_TRUE(dev.setAdcRange(AdcRange::MV_40_96).is(Err::I2C_ERROR));
    TEST_ASSERT_EQUAL_UINT32(3u, bus.transferHistoryCount);
    TEST_ASSERT_EQUAL_HEX16(oldConfig, bus.reg16[cmd::REG_CONFIG]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcRange::MV_163_84),
                            static_cast<uint8_t>(dev.getConfig().adcRange));
    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_TRUE(settings.hardwareDirty);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::RESYNC_REQUIRED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }
}

void test_retained_reset_and_replay_wrappers_are_bounded_or_restricted() {
  {
    FakeBus bus;
    bus.autoClearAccumulatorReset = true;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    const uint32_t beforeSoftReset = bus.readCalls + bus.writeCalls;
    TEST_ASSERT_TRUE(dev.softReset().is(Err::INVALID_CONFIG));
    TEST_ASSERT_EQUAL_UINT32(beforeSoftReset, bus.readCalls + bus.writeCalls);

    TEST_ASSERT_TRUE(dev.resetAccumulators().ok());
    TEST_ASSERT_EQUAL_UINT32(2u, bus.transferHistoryCount);
    bus.reg40[cmd::REG_ENERGY] = 100;
    bus.diagAlrt = cmd::DIAG_MEMSTAT;
    double energy = 0.0;
    TEST_ASSERT_TRUE(dev.readEnergy(energy).ok());
    TEST_ASSERT_TRUE(energy > 0.0);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    TEST_ASSERT_TRUE(dev.startApplyCalibration().ok());
    Status st{Err::IN_PROGRESS, 0, "replay pending"};
    for (uint8_t poll = 0; poll < 20 && st.inProgress(); ++poll) {
      st = dev.pollApplyCalibration(bus.nowMs, 1);
    }
    TEST_ASSERT_TRUE(st.ok());
    TEST_ASSERT_EQUAL_UINT32(14u, bus.transferHistoryCount);
    JobSnapshot snapshot{};
    TEST_ASSERT_TRUE(dev.getJobState(snapshot).ok());
    TEST_ASSERT_FALSE(snapshot.resultAvailable);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);
    clearTransferHistory(bus);
    TEST_ASSERT_TRUE(dev.startResetJob().ok());
    Status st{Err::IN_PROGRESS, 0, "reset pending"};
    for (uint8_t poll = 0; poll < 30 && st.inProgress(); ++poll) {
      const uint32_t before = bus.readCalls + bus.writeCalls;
      st = dev.pollResetJob(bus.nowMs, 1);
      if (st.inProgress() && bus.readCalls + bus.writeCalls == before) {
        ++bus.nowMs;
      }
    }
    TEST_ASSERT_TRUE(st.ok());
    TEST_ASSERT_EQUAL_UINT32(16u, bus.transferHistoryCount);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(HardwareState::SYNCHRONIZED),
                            static_cast<uint8_t>(dev.hardwareState()));
  }
}

void test_latched_offline_policy_remains_explicit_legacy_opt_in() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);
  cfg.healthPolicy = HealthPolicy::LATCH_OFFLINE;
  cfg.offlineThreshold = 1;
  TEST_ASSERT_TRUE(dev.bind(cfg).ok());
  (void)initializeCooperativeDevice(dev, bus);

  bus.readError = Status::Error(Err::I2C_TIMEOUT, "forced timeout", -9);
  bus.readErrorRemaining = 1;
  float value = 0.0f;
  TEST_ASSERT_TRUE(dev.readBusVoltage(value).is(Err::I2C_TIMEOUT));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::OFFLINE),
                          static_cast<uint8_t>(dev.state()));
  const uint32_t beforeBlocked = bus.readCalls;
  TEST_ASSERT_TRUE(dev.readBusVoltage(value).is(Err::BUSY));
  TEST_ASSERT_EQUAL_UINT32(beforeBlocked, bus.readCalls);
  TEST_ASSERT_TRUE(dev.recover().ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::READY),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_TRUE(dev.readBusVoltage(value).ok());
}

void test_passive_health_never_suppresses_owner_requested_transport() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeCooperativeConfig(bus);
  cfg.offlineThreshold = 1;
  cfg.healthPolicy = HealthPolicy::PASSIVE;
  TEST_ASSERT_TRUE(dev.bind(cfg).ok());
  (void)initializeCooperativeDevice(dev, bus);

  bus.readError = Status::Error(Err::I2C_TIMEOUT, "forced timeout", -9);
  bus.readErrorRemaining = 3;
  for (uint8_t attempt = 0; attempt < 3; ++attempt) {
    float value = 123.0f;
    const Status st = dev.readBusVoltage(value);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_TIMEOUT),
                            static_cast<uint8_t>(st.code));
  }
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::DEGRADED),
                          static_cast<uint8_t>(dev.state()));
  const uint32_t beforeRecoveryRead = bus.readCalls;
  float value = -1.0f;
  TEST_ASSERT_TRUE(dev.readBusVoltage(value).ok());
  TEST_ASSERT_EQUAL_UINT32(beforeRecoveryRead + 1u, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::READY),
                          static_cast<uint8_t>(dev.state()));
}

// ===========================================================================
// Regression: wait origins newer than the caller timestamp
// ===========================================================================

// A blocking write can cross a millisecond boundary, so the hook-sampled wait
// origin can be newer than the timestamp the caller sampled before pollJob().
// The gate must treat that as "not elapsed" instead of underflowing.
void test_wait_origin_newer_than_caller_timestamp_does_not_skip_wait() {
  const uint32_t advanceMs[] = {0u, 1u, 5u};
  for (size_t index = 0; index < 3; ++index) {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
    (void)initializeCooperativeDevice(dev, bus);

    // Make every write advance the fake clock, then hold the caller's
    // timestamp at the pre-write value for the whole poll.
    bus.successfulWriteDurationReg = cmd::REG_ADC_CONFIG;
    bus.successfulWriteDurationMatch =
        static_cast<uint8_t>(bus.writeMatchCount[cmd::REG_ADC_CONFIG] + 1u);
    bus.advanceNowMsOnWrite = advanceMs[index];

    uint32_t operationId = 0;
    TEST_ASSERT_TRUE(dev.startInstantaneousSample(9u, operationId).ok());
    const uint32_t callerNowMs = bus.nowMs;
    const Status st = dev.pollJob(callerNowMs, 20u);
    TEST_ASSERT_TRUE(st.inProgress());

    JobSnapshot snapshot{};
    TEST_ASSERT_TRUE(dev.getJobState(snapshot).ok());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(JobState::ACTIVE),
                            static_cast<uint8_t>(snapshot.state));
    // Verify/verify/trigger only; the conversion wait must still be pending.
    TEST_ASSERT_EQUAL_UINT16(3u, snapshot.transfersCompleted);
    TEST_ASSERT_TRUE(dev.cancelJob().is(Err::CANCELLED));
    JobResult drained{};
    (void)dev.takeJobResult(operationId, drained);
  }
}

// ===========================================================================
// Regression: reset side effects the owner cannot see any other way
// ===========================================================================

void test_reset_marks_thresholds_dirty_and_reports_full_dirty_register_set() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus);

  SettingsSnapshot settings{};
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_FALSE(settings.thresholdsDirty);

  // A successful reset reverts SOVL/SUVL/BOVL/BUVL/TEMP_LIMIT/PWR_LIMIT to
  // datasheet defaults, and the driver never replays them.
  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startReset(11u, operationId).ok());
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).ok());
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_TRUE(settings.thresholdsDirty);

  // A reset that fails after the reset write must report every register the
  // reset restored, not just CONFIG.
  FakeBus failing;
  INA228::INA228 failedDev;
  TEST_ASSERT_TRUE(failedDev.bind(makeCooperativeConfig(failing)).ok());
  (void)initializeCooperativeDevice(failedDev, failing);
  failing.nthReadFailureRegs[0] = cmd::REG_CONFIG;
  failing.nthReadFailureMatches[0] =
      static_cast<uint8_t>(failing.readMatchCount[cmd::REG_CONFIG] + 1u);
  failing.nthReadFailureStatus[0] =
      Status::Error(Err::I2C_TIMEOUT, "forced reset verify failure", -7);
  failing.nthReadFailureCount = 1;

  uint32_t failedOperationId = 0;
  TEST_ASSERT_TRUE(failedDev.startReset(12u, failedOperationId).ok());
  TEST_ASSERT_FALSE(pollCooperativeToTerminal(failedDev, failing).ok());
  JobResult failedResult{};
  TEST_ASSERT_TRUE(
      failedDev.takeJobResult(failedOperationId, failedResult).ok());

  SettingsSnapshot failedSettings{};
  TEST_ASSERT_TRUE(failedDev.getSettings(failedSettings).ok());
  TEST_ASSERT_TRUE(failedSettings.hardwareDirty);
  const uint8_t resetRegisters[] = {
      cmd::REG_CONFIG,     cmd::REG_ADC_CONFIG, cmd::REG_SHUNT_CAL,
      cmd::REG_SHUNT_TEMPCO, cmd::REG_DIAG_ALRT, cmd::REG_SOVL,
      cmd::REG_SUVL,       cmd::REG_BOVL,       cmd::REG_BUVL,
      cmd::REG_TEMP_LIMIT, cmd::REG_PWR_LIMIT};
  for (size_t i = 0; i < sizeof(resetRegisters) / sizeof(resetRegisters[0]); ++i) {
    TEST_ASSERT_TRUE((failedSettings.dirtyRegisterMask &
                      (uint64_t{1} << resetRegisters[i])) != 0);
  }

  // Even when the reset write itself returns an error, the device may have
  // applied it. Mark the threshold advisory before issuing the write.
  FakeBus ambiguous;
  INA228::INA228 ambiguousDev;
  TEST_ASSERT_TRUE(ambiguousDev.bind(makeCooperativeConfig(ambiguous)).ok());
  (void)initializeCooperativeDevice(ambiguousDev, ambiguous);
  ambiguous.applyThenFailWriteReg = cmd::REG_CONFIG;
  ambiguous.applyThenFailWriteMatch =
      static_cast<uint8_t>(ambiguous.writeMatchCount[cmd::REG_CONFIG] + 1U);
  uint32_t ambiguousOperationId = 0;
  TEST_ASSERT_TRUE(ambiguousDev.startReset(13U, ambiguousOperationId).ok());
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(ambiguousDev, ambiguous)
                       .is(Err::I2C_ERROR));
  JobResult ambiguousResult{};
  TEST_ASSERT_TRUE(ambiguousDev.takeJobResult(
      ambiguousOperationId, ambiguousResult).ok());
  SettingsSnapshot ambiguousSettings{};
  TEST_ASSERT_TRUE(ambiguousDev.getSettings(ambiguousSettings).ok());
  TEST_ASSERT_TRUE(ambiguousSettings.thresholdsDirty);
}

// ===========================================================================
// Regression: invalidation preserves evidence; bind() clears advisories
// ===========================================================================

void test_invalidate_preserves_dirty_evidence_and_bind_clears_advisories() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  (void)initializeCooperativeDevice(dev, bus);

  bus.writeError = Status::Error(Err::I2C_ERROR,
                                 "forced threshold write error", -98);
  queueWriteFailure(bus, cmd::REG_SOVL);
  TEST_ASSERT_TRUE(dev.setShuntOvervoltageThreshold(0.010f).is(Err::I2C_ERROR));

  bus.writeFailureRegs[0] = cmd::REG_SHUNT_TEMPCO;
  bus.writeFailureCount = 1;
  TEST_ASSERT_FALSE(dev.setShuntTempCoeff(1234u).ok());
  bus.writeFailureCount = 0;

  SettingsSnapshot before{};
  TEST_ASSERT_TRUE(dev.getSettings(before).ok());
  TEST_ASSERT_TRUE(before.hardwareDirty);
  TEST_ASSERT_TRUE(before.thresholdsDirty);
  TEST_ASSERT_TRUE((before.dirtyRegisterMask &
                    (uint64_t{1} << cmd::REG_SHUNT_TEMPCO)) != 0);
  const Err firstCause = before.hardwareDirtyCause.code;
  const int32_t firstDetail = before.hardwareDirtyCause.detail;

  bus.readError = Status::Error(Err::I2C_TIMEOUT, "later job failure", -99);
  queueNthReadFailure(bus, cmd::REG_MANUFACTURER_ID,
                      static_cast<uint8_t>(bus.readMatchCount[
                          cmd::REG_MANUFACTURER_ID] + 1U));
  uint32_t operationId = 0;
  TEST_ASSERT_TRUE(dev.startReinitialize(0x9911U, operationId).ok());
  TEST_ASSERT_TRUE(pollCooperativeToTerminal(dev, bus).is(Err::I2C_TIMEOUT));
  JobResult result{};
  TEST_ASSERT_TRUE(dev.takeJobResult(operationId, result).ok());
  SettingsSnapshot afterJobFailure{};
  TEST_ASSERT_TRUE(dev.getSettings(afterJobFailure).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(firstCause),
                          static_cast<uint8_t>(afterJobFailure.hardwareDirtyCause.code));
  TEST_ASSERT_EQUAL_INT32(firstDetail, afterJobFailure.hardwareDirtyCause.detail);

  // Owner invalidation makes hardware less trusted; it must not erase which
  // registers are suspect, nor rewrite the first recorded cause.
  TEST_ASSERT_TRUE(dev.invalidateHardwareState(
      Status::Error(Err::HARDWARE_STATE_UNKNOWN, "owner invalidation")).ok());
  SettingsSnapshot after{};
  TEST_ASSERT_TRUE(dev.getSettings(after).ok());
  TEST_ASSERT_TRUE(after.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(before.dirtyRegisterMask, after.dirtyRegisterMask);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(firstCause),
                          static_cast<uint8_t>(after.hardwareDirtyCause.code));

  // A fresh binding starts from a clean advisory state.
  dev.invalidateHardwareState(Status::Ok());
  TEST_ASSERT_TRUE(dev.bind(makeCooperativeConfig(bus)).ok());
  SettingsSnapshot rebound{};
  TEST_ASSERT_TRUE(dev.getSettings(rebound).ok());
  TEST_ASSERT_FALSE(rebound.thresholdsDirty);
  TEST_ASSERT_EQUAL_UINT64(0u, rebound.dirtyRegisterMask);
}

// ===========================================================================
// Regression: uncalibrated ADC range change keeps the plan coherent
// ===========================================================================

void test_uncalibrated_adc_range_change_updates_plan_full_scale() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  CalibrationPlan plan{};
  TEST_ASSERT_TRUE(dev.getCalibrationPlan(plan).ok());
  TEST_ASSERT_EQUAL_UINT32(163840u, plan.shuntFullScaleMicrovolts);

  TEST_ASSERT_TRUE(dev.setAdcRange(AdcRange::MV_40_96).ok());
  TEST_ASSERT_TRUE(dev.getCalibrationPlan(plan).ok());
  TEST_ASSERT_EQUAL_UINT32(40960u, plan.shuntFullScaleMicrovolts);

  SettingsSnapshot settings{};
  TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcRange::MV_40_96),
                          static_cast<uint8_t>(settings.adcRange));
}

// ===========================================================================
// Entry point
// ===========================================================================

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_status_ok);
  RUN_TEST(test_status_error);
  RUN_TEST(test_status_in_progress);
  RUN_TEST(test_status_is_and_bool_conversion);
  RUN_TEST(test_err_name_contract_and_append_values);
  RUN_TEST(test_config_defaults);
  RUN_TEST(test_get_settings_before_begin_is_cache_only_uninitialized_snapshot);
  RUN_TEST(test_driver_state_alias_matches_state);
  RUN_TEST(test_get_settings_is_bus_silent_and_does_not_consume_diag);
  RUN_TEST(test_begin_rejects_missing_callbacks);
  RUN_TEST(test_begin_success_sets_ready_without_health_counts);
  RUN_TEST(test_configured_i2c_address_reaches_transport_callbacks);
  RUN_TEST(test_begin_rejects_invalid_address);
  RUN_TEST(test_begin_rejects_zero_timeout);
  RUN_TEST(test_begin_rejects_invalid_adc_range);
  RUN_TEST(test_begin_rejects_identity_and_memstat_mismatch_without_writes);
  RUN_TEST(test_begin_normalizes_offline_threshold_on_stored_copy);
  RUN_TEST(test_begin_programs_tempco_even_when_tempcomp_disabled);
  RUN_TEST(test_begin_rejects_non_finite_calibration);
  RUN_TEST(test_end_returns_to_uninit);
  RUN_TEST(test_missing_now_ms_uses_zero_for_health_timestamps);
  RUN_TEST(test_begin_without_now_ms_keeps_zero_health_timestamp);
  RUN_TEST(test_probe_failure_does_not_update_health);
  RUN_TEST(test_probe_preserves_transport_errors_without_health_tracking);
  RUN_TEST(test_recover_failure_updates_health_once);
  RUN_TEST(test_recover_replay_failures_mark_dirty_for_each_write_position);
  RUN_TEST(test_example_transport_maps_wire_errors_and_keeps_timeout_owned_by_init);
  RUN_TEST(test_example_transport_validates_params_and_handles_write_read);
  RUN_TEST(test_conversion_time_estimate);
  RUN_TEST(test_conversion_time_with_averaging);
  RUN_TEST(test_shutdown_conversion_time_ignores_configured_delay);
  RUN_TEST(test_conversion_time_defaults_and_maximum);
  RUN_TEST(test_conversion_time_table_vectors);
  RUN_TEST(test_config_register_encoding_vectors);
  RUN_TEST(test_conversion_ready_clears_completed_trigger_state);
  RUN_TEST(test_triggered_conversion_gates_reads_until_cnvrf);
  RUN_TEST(test_tick_timestamp_completes_trigger_without_now_hook);
  RUN_TEST(test_tick_deadline_is_wraparound_safe);
  RUN_TEST(test_tick_is_bus_silent_without_a_pending_trigger);
  RUN_TEST(test_configured_trigger_uses_hooked_post_write_origin_for_initialize_and_reinitialize);
  RUN_TEST(test_configured_trigger_without_hook_anchors_after_successful_terminal);
  RUN_TEST(test_hookless_internal_sync_jobs_do_not_consume_trigger_origin);
  RUN_TEST(test_hookless_configured_trigger_deferral_lifetime_is_bounded);
  RUN_TEST(test_write_duration_injection_is_consumed_only_by_success);
  RUN_TEST(test_ambiguous_adc_writes_invalidate_pending_trigger_timing);
  RUN_TEST(test_raw_conversion_invalidators_clear_pending_trigger_timing);
  RUN_TEST(test_legacy_range_change_uses_validated_calibration_plan_at_boundary);
  RUN_TEST(test_public_diag_read_consuming_cnvrf_does_not_strand_pending_trigger);
  RUN_TEST(test_tick_preserves_diag_alert_evidence_when_polling_cnvrf);
  RUN_TEST(test_readiness_path_preserves_diag_alert_evidence_for_measurement_gate);
  RUN_TEST(test_alert_config_setters_do_not_read_live_diag_alrt);
  RUN_TEST(test_alert_config_write_failure_preserves_cache_and_marks_diag_dirty);
  RUN_TEST(test_public_read_diag_alert_is_destructive_and_preserved);
  RUN_TEST(test_public_read_diag_alert_raw_is_destructive);
  RUN_TEST(test_read_bus_voltage_requires_init);
  RUN_TEST(test_read_bus_voltage_zero_on_default);
  RUN_TEST(test_uncalibrated_current_power_energy_charge_fail_without_i2c);
  RUN_TEST(test_diag_alert_surfaces_and_preserves_accumulator_overflow_flags);
  RUN_TEST(test_read_raw_sample_uses_unsigned_vbus_and_energy);
  RUN_TEST(test_read_integer_sample_uses_fixed_units_without_accumulators);
  RUN_TEST(test_convert_raw_sample_uses_fixed_units_without_i2c);
  RUN_TEST(test_integer_sample_requires_calibration_and_preserves_output);
  RUN_TEST(test_convert_raw_sample_reports_math_overflow_evidence);
  RUN_TEST(test_repeated_current_math_overflow_clears_after_accumulator_reset);
  RUN_TEST(test_20bit_edge_vectors_and_low_nibble_masking);
  RUN_TEST(test_temperature_negative_and_positive_vectors);
  RUN_TEST(test_read_raw_sample_failures_leave_output_unchanged);
  RUN_TEST(test_poll_measurement_ready_delay_gate_and_diag_budget);
  RUN_TEST(test_signed_raw_register_vectors_sign_extend);
  RUN_TEST(test_datasheet_table_8_4_measurement_vectors_and_negative_shunt);
  RUN_TEST(test_invalid_begin_calibration_configs_do_not_touch_i2c);
  RUN_TEST(test_set_calibration_invalid_params_do_not_touch_i2c_or_cache);
  RUN_TEST(test_set_calibration_write_failure_preserves_committed_scale);
  RUN_TEST(test_begin_uncalibrated_writes_shunt_cal_zero);
  RUN_TEST(test_cached_static_config_write_failures_mark_dirty_registers);
  RUN_TEST(test_threshold_setters_encode_exact_register_vectors);
  RUN_TEST(test_threshold_write_failures_preserve_registers);
  RUN_TEST(test_invalid_threshold_values_do_not_touch_bus);
  RUN_TEST(test_read_manufacturer_id);
  RUN_TEST(test_read_device_id);
  RUN_TEST(test_public_register_access_helpers);
  RUN_TEST(test_raw_software_reset_marks_every_reset_register_dirty);
  RUN_TEST(test_raw_accumulator_register_read_does_not_pre_preserve_diag_alert);
  RUN_TEST(test_public_register_access_preserves_transport_errors);
  RUN_TEST(test_register_access_after_end_does_not_touch_bus);

  RUN_TEST(test_cooperative_bind_is_zero_i2c_and_validates_contract);
  RUN_TEST(test_cooperative_job_limits_are_exact_and_retry_free);
  RUN_TEST(test_instantaneous_sample_wait_includes_bounded_device_timing_margin);
  RUN_TEST(test_fixed_calibration_plans_cover_tunnelmonitor_profile_and_strict_limits);
  RUN_TEST(test_identity_parser_splits_die_and_revision_strictly);
  RUN_TEST(test_cooperative_initialize_budget_order_forwarding_and_alert_determinism);
  RUN_TEST(test_initialize_failure_injection_covers_every_transfer_stage);
  RUN_TEST(test_initialize_readback_mismatch_and_ambiguous_effects_are_observable);
  RUN_TEST(test_initialize_semantic_identity_and_memstat_failures_are_terminal);
  RUN_TEST(test_reinitialize_identity_commit_cancel_and_invalidation_are_atomic);
  RUN_TEST(test_reinitialize_semantic_failures_revoke_verified_identity);
  RUN_TEST(test_verify_configuration_failure_injection_covers_every_transfer_stage);
  RUN_TEST(test_verify_configuration_is_read_only_bounded_and_generation_stable);
  RUN_TEST(test_verify_configuration_distinguishes_inconclusive_transport_from_disproof);
  RUN_TEST(test_job_identity_exactly_once_and_rebinding_do_not_leak_context);
  RUN_TEST(test_cancel_and_timeout_are_bus_silent_with_precise_effects);
  RUN_TEST(test_triggered_sample_sequence_wait_budget_and_atomic_result);
  RUN_TEST(test_triggered_sample_wait_is_uint32_wrap_safe);
  RUN_TEST(test_triggered_sample_without_hook_anchors_on_later_bus_silent_poll);
  RUN_TEST(test_hookless_wait_deferral_is_cleared_by_cancel_and_timeout);
  RUN_TEST(test_hookless_positive_budget_anchor_is_bus_silent);
  RUN_TEST(test_sample_diagnostic_failures_restore_adc_and_preserve_correlated_evidence);
  RUN_TEST(test_sample_failure_injection_covers_every_transfer_stage_without_retry);
  RUN_TEST(test_active_job_excludes_other_hardware_apis_but_allows_cache_access);
  RUN_TEST(test_invalidate_and_configuration_guard_require_verified_reappearance);
  RUN_TEST(test_diagnostic_event_acknowledgement_and_caller_timestamps_are_deterministic);
  RUN_TEST(test_accumulator_epoch_requires_verified_reset_after_each_generation);
  RUN_TEST(test_accumulator_reset_clears_obsolete_snapshot_evidence);
  RUN_TEST(test_accumulator_reset_cancel_and_ambiguous_write_require_resync);
  RUN_TEST(test_reset_and_accumulator_failure_injection_cover_every_transfer_stage);
  RUN_TEST(test_reset_job_wait_and_zero_budget_are_wrap_safe_and_bus_silent);
  RUN_TEST(test_reset_wait_origin_is_post_write_with_and_without_hook);
  RUN_TEST(test_revision_policy_and_declared_alert_defaults_are_verified);
  RUN_TEST(test_retained_configuration_setters_preserve_success_and_failure_contracts);
  RUN_TEST(test_retained_reset_and_replay_wrappers_are_bounded_or_restricted);
  RUN_TEST(test_latched_offline_policy_remains_explicit_legacy_opt_in);
  RUN_TEST(test_passive_health_never_suppresses_owner_requested_transport);
  RUN_TEST(test_wait_origin_newer_than_caller_timestamp_does_not_skip_wait);
  RUN_TEST(test_reset_marks_thresholds_dirty_and_reports_full_dirty_register_set);
  RUN_TEST(test_invalidate_preserves_dirty_evidence_and_bind_clears_advisories);
  RUN_TEST(test_uncalibrated_adc_range_change_updates_plan_full_scale);
  return UNITY_END();
}
