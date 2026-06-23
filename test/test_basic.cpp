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

using namespace INA228;

namespace {

struct WriteEvent {
  uint8_t reg = 0;
  uint16_t value = 0;
  bool success = false;
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
  uint8_t lastReadReg = 0;
  uint8_t readHistory[256] = {};
  size_t readHistoryCount = 0;
  WriteEvent writeHistory[256] = {};
  size_t writeHistoryCount = 0;
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
  uint8_t configResetReadsRemaining = 0;
  bool configResetNeverClears = false;
  bool resetLeavesMemstatLow = false;
  uint8_t configReadOverrideRemaining = 0;
  uint16_t configReadOverrideValue = 0;

  int readErrorRemaining = 0;
  int writeErrorRemaining = 0;
  Status readError = Status::Error(Err::I2C_ERROR, "forced read error", -1);
  Status writeError = Status::Error(Err::I2C_ERROR, "forced write error", -2);
};

Status fakeWrite(uint8_t addr, const uint8_t* data, size_t len, uint32_t, void* user) {
  FakeBus* bus = static_cast<FakeBus*>(user);
  bus->writeCalls++;
  bus->lastWriteAddr = addr;
  if (data == nullptr || len == 0) {
    return Status::Error(Err::INVALID_PARAM, "invalid fake write args");
  }
  const uint8_t reg = data[0];
  const uint16_t value = (len == 3) ?
      static_cast<uint16_t>((static_cast<uint16_t>(data[1]) << 8) | data[2]) : 0;
  if (reg < 64) {
    bus->writeMatchCount[reg]++;
  }
  auto recordWrite = [&](bool success) {
    if (bus->writeHistoryCount < sizeof(bus->writeHistory) / sizeof(bus->writeHistory[0])) {
      bus->writeHistory[bus->writeHistoryCount++] = WriteEvent{reg, value, success};
    }
  };
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
      recordWrite(false);
      return status;
    }
  }
  if (bus->writeFailureCount > 0 && bus->writeFailureRegs[0] == reg) {
    for (size_t i = 1; i < bus->writeFailureCount; ++i) {
      bus->writeFailureRegs[i - 1] = bus->writeFailureRegs[i];
    }
    bus->writeFailureCount--;
    recordWrite(false);
    return bus->writeError;
  }
  if (bus->writeErrorRemaining > 0) {
    bus->writeErrorRemaining--;
    recordWrite(false);
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
      bus->diagAlrt = bus->resetLeavesMemstatLow ? 0 : cmd::DIAG_ALRT_RESET;
      bus->reg16[cmd::REG_CONFIG] = cmd::CONFIG_RST;
    }
    if (reg == cmd::REG_CONFIG && ((value & cmd::CONFIG_RSTACC) != 0)) {
      bus->reg40[cmd::REG_ENERGY] = 0;
      bus->reg40[cmd::REG_CHARGE] = 0;
      bus->diagAlrt &= ~(cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF |
                         cmd::DIAG_MATHOF | cmd::DIAG_CNVRF);
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
  recordWrite(true);
  return Status::Ok();
}

Status fakeWriteRead(uint8_t addr, const uint8_t* txData, size_t txLen, uint8_t* rxData,
                     size_t rxLen, uint32_t, void* user) {
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
  bus->lastReadReg = reg;
  if (reg < 64) {
    bus->readMatchCount[reg]++;
  }
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
      if (!bus->configResetNeverClears) {
        if (bus->configResetReadsRemaining > 0) {
          bus->configResetReadsRemaining--;
        } else {
          bus->reg16[reg] &= ~cmd::CONFIG_RST;
        }
      }
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

bool wasRegisterRead(const FakeBus& bus, uint8_t reg) {
  for (size_t i = 0; i < bus.readHistoryCount; ++i) {
    if (bus.readHistory[i] == reg) {
      return true;
    }
  }
  return false;
}

size_t firstReadIndex(const FakeBus& bus, uint8_t reg) {
  for (size_t i = 0; i < bus.readHistoryCount; ++i) {
    if (bus.readHistory[i] == reg) {
      return i;
    }
  }
  return bus.readHistoryCount;
}

void clearReadHistory(FakeBus& bus) {
  bus.readHistoryCount = 0;
  bus.lastReadReg = 0;
  for (size_t i = 0; i < 64; ++i) {
    bus.readMatchCount[i] = 0;
  }
}

void clearWriteHistory(FakeBus& bus) {
  bus.writeHistoryCount = 0;
  bus.lastWriteReg = 0;
  bus.lastWrite16 = 0;
  for (size_t i = 0; i < 64; ++i) {
    bus.writeMatchCount[i] = 0;
  }
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

size_t countRegisterWrites(const FakeBus& bus, uint8_t reg) {
  size_t count = 0;
  for (size_t i = 0; i < bus.writeHistoryCount; ++i) {
    if (bus.writeHistory[i].reg == reg) {
      count++;
    }
  }
  return count;
}

void forceOffline(INA228::INA228& dev, FakeBus& bus) {
  bus.readErrorRemaining = 3;
  bus.readError = Status::Error(Err::I2C_BUS, "forced offline", -90);
  for (uint8_t i = 0; i < 3; ++i) {
    float value = 0.0f;
    (void)dev.readBusVoltage(value);
  }
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::OFFLINE),
                          static_cast<uint8_t>(dev.state()));
  bus.readErrorRemaining = 0;
  clearReadHistory(bus);
  clearWriteHistory(bus);
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

Measurement sentinelMeasurement() {
  Measurement m{};
  m.shuntVoltageV = 1.0f;
  m.busVoltageV = 2.0f;
  m.temperatureC = 3.0f;
  m.currentA = 4.0f;
  m.powerW = 5.0f;
  m.energyJ = 6.0;
  m.chargeC = 7.0;
  m.energyValid = true;
  m.chargeValid = true;
  m.energyOverflow = true;
  m.chargeOverflow = true;
  m.mathOverflow = true;
  m.diagAlertValid = true;
  m.diagAlertRaw = 0xA55A;
  return m;
}

void assertSentinelMeasurement(const Measurement& m) {
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, m.shuntVoltageV);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.0f, m.busVoltageV);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.0f, m.temperatureC);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 4.0f, m.currentA);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 5.0f, m.powerW);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 6.0f, static_cast<float>(m.energyJ));
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 7.0f, static_cast<float>(m.chargeC));
  TEST_ASSERT_TRUE(m.energyValid);
  TEST_ASSERT_TRUE(m.chargeValid);
  TEST_ASSERT_TRUE(m.energyOverflow);
  TEST_ASSERT_TRUE(m.chargeOverflow);
  TEST_ASSERT_TRUE(m.mathOverflow);
  TEST_ASSERT_TRUE(m.diagAlertValid);
  TEST_ASSERT_EQUAL_HEX16(0xA55Au, m.diagAlertRaw);
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

IntegerSample sentinelIntegerSample() {
  IntegerSample sample{};
  sample.shuntMicrovolts = -11;
  sample.busMillivolts = 22;
  sample.dieTemperatureMilliC = -33;
  sample.currentMilliamps = -44;
  sample.powerMilliwatts = 55;
  sample.diagAlertValid = true;
  sample.diagAlertRaw = 0x33CC;
  return sample;
}

void assertSentinelIntegerSample(const IntegerSample& sample) {
  TEST_ASSERT_EQUAL_INT32(-11, sample.shuntMicrovolts);
  TEST_ASSERT_EQUAL_UINT32(22u, sample.busMillivolts);
  TEST_ASSERT_EQUAL_INT32(-33, sample.dieTemperatureMilliC);
  TEST_ASSERT_EQUAL_INT32(-44, sample.currentMilliamps);
  TEST_ASSERT_EQUAL_UINT32(55u, sample.powerMilliwatts);
  TEST_ASSERT_TRUE(sample.diagAlertValid);
  TEST_ASSERT_EQUAL_HEX16(0x33CCu, sample.diagAlertRaw);
}

void assertPositivePowerRawSample(const RawSample& raw) {
  TEST_ASSERT_EQUAL_INT32(0x4BF00, raw.vshunt);
  TEST_ASSERT_EQUAL_UINT32(0x3C000u, raw.vbus);
  TEST_ASSERT_EQUAL_INT16(0x0C80, raw.dietemp);
  TEST_ASSERT_EQUAL_INT32(0x4CCCC, raw.current);
  TEST_ASSERT_EQUAL_UINT32(0x48000Cu, raw.power);
  TEST_ASSERT_EQUAL_UINT64(0u, raw.energy);
  TEST_ASSERT_EQUAL_INT64(0, raw.charge);
  TEST_ASSERT_FALSE(raw.energyValid);
  TEST_ASSERT_FALSE(raw.chargeValid);
  TEST_ASSERT_FALSE(raw.energyOverflow);
  TEST_ASSERT_FALSE(raw.chargeOverflow);
  TEST_ASSERT_FALSE(raw.mathOverflow);
  TEST_ASSERT_FALSE(raw.diagAlertValid);
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

void test_get_settings_snapshot() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.i2cAddress = 0x4F;
  cfg.mode = Mode::CONT_TEMP_SHUNT;
  cfg.vbusConvTime = ConvTime::US_50;
  cfg.vshuntConvTime = ConvTime::US_84;
  cfg.vtempConvTime = ConvTime::US_150;
  cfg.averaging = Averaging::AVG_16;
  cfg.adcRange = AdcRange::MV_40_96;
  cfg.shuntResistanceOhm = 0.015f;
  cfg.maxExpectedCurrentA = 10.0f;
  cfg.tempCompEnabled = true;
  cfg.shuntTempCoeffPpmC = 3900;
  cfg.convDelayMs2 = 7;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  const uint32_t readsBefore = bus.readCalls;
  const uint32_t writesBefore = bus.writeCalls;
  SettingsSnapshot snap;
  Status st = dev.getSettings(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);
  TEST_ASSERT_TRUE(snap.initialized);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::READY),
                          static_cast<uint8_t>(snap.state));
  TEST_ASSERT_EQUAL_HEX8(0x4F, snap.i2cAddress);
  TEST_ASSERT_EQUAL_UINT32(10u, snap.i2cTimeoutMs);
  TEST_ASSERT_EQUAL_UINT8(3u, snap.offlineThreshold);
  TEST_ASSERT_TRUE(snap.hasNowMsHook);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::CONT_TEMP_SHUNT),
                          static_cast<uint8_t>(snap.mode));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_50),
                          static_cast<uint8_t>(snap.vbusConvTime));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_84),
                          static_cast<uint8_t>(snap.vshuntConvTime));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_150),
                          static_cast<uint8_t>(snap.vtempConvTime));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Averaging::AVG_16),
                          static_cast<uint8_t>(snap.averaging));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcRange::MV_40_96),
                          static_cast<uint8_t>(snap.adcRange));
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.015f, snap.shuntResistanceOhm);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.0f, snap.maxExpectedCurrentA);
  TEST_ASSERT_TRUE(snap.tempCompEnabled);
  TEST_ASSERT_EQUAL_UINT16(3900u, snap.shuntTempCoeffPpmC);
  TEST_ASSERT_EQUAL_UINT8(7u, snap.convDelayMs2);
  TEST_ASSERT_TRUE(snap.currentLsb > 0.0f);
  TEST_ASSERT_GREATER_THAN_UINT16(0u, snap.shuntCal);
  TEST_ASSERT_TRUE(snap.calibrated);
  TEST_ASSERT_FALSE(snap.calibrationClamped);
  TEST_ASSERT_TRUE(snap.maxCurrentExceedsShuntRange);
  TEST_ASSERT_FALSE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(0u, snap.dirtyRegisterMask);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::OK),
                          static_cast<uint8_t>(snap.hardwareDirtyCause.code));
  TEST_ASSERT_FALSE(snap.thresholdsDirty);
  TEST_ASSERT_FALSE(snap.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(0u, snap.triggeredConversionStartMs);
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
  clearWriteHistory(bus);
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

void test_invalid_begin_after_success_resets_default_runtime() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.i2cAddress = 0x4F;
  cfg.offlineThreshold = 1;
  cfg.mode = Mode::SHUTDOWN;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  const uint32_t readsBefore = bus.readCalls;
  const uint32_t writesBefore = bus.writeCalls;
  Config bad = makeConfig(bus);
  bad.i2cAddress = 0x50;
  Status st = dev.begin(bad);

  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_FALSE(dev.isInitialized());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_EQUAL_UINT32(0u, dev.totalSuccess());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.totalFailures());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.lastOkMs());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.lastErrorMs());
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);

  const Config& stored = dev.getConfig();
  TEST_ASSERT_NULL(stored.i2cWrite);
  TEST_ASSERT_NULL(stored.i2cWriteRead);
  TEST_ASSERT_EQUAL_HEX8(0x40, stored.i2cAddress);
  TEST_ASSERT_EQUAL_UINT32(50u, stored.i2cTimeoutMs);
  TEST_ASSERT_EQUAL_UINT8(5u, stored.offlineThreshold);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::CONT_ALL),
                          static_cast<uint8_t>(stored.mode));
}

void test_failed_begin_probe_resets_cached_config() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.i2cAddress = 0x4F;
  cfg.mode = Mode::SHUTDOWN;
  cfg.offlineThreshold = 1;
  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::TIMEOUT, "forced begin timeout", -10);

  Status st = dev.begin(cfg);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::TIMEOUT),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_FALSE(dev.isInitialized());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_NULL(dev.getConfig().i2cWrite);
  TEST_ASSERT_NULL(dev.getConfig().i2cWriteRead);
  TEST_ASSERT_EQUAL_HEX8(0x40, dev.getConfig().i2cAddress);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::CONT_ALL),
                          static_cast<uint8_t>(dev.getConfig().mode));
  TEST_ASSERT_EQUAL_UINT8(5u, dev.getConfig().offlineThreshold);
  TEST_ASSERT_EQUAL_UINT32(0u, dev.totalSuccess());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.totalFailures());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dev.currentLsb());
}

void test_begin_preserves_transport_errors_for_startup_reads() {
  const uint8_t regs[] = {
      cmd::REG_MANUFACTURER_ID,
      cmd::REG_DEVICE_ID,
      cmd::REG_DIAG_ALRT,
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
      Config cfg = makeConfig(bus);
      bus.readError = Status::Error(c.transport, "forced begin read error", -33);
      queueNthReadFailure(bus, reg, 1);

      Status st = dev.begin(cfg);
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(c.expected),
                              static_cast<uint8_t>(st.code));
      TEST_ASSERT_EQUAL_INT32(-33, st.detail);
      TEST_ASSERT_FALSE(dev.isInitialized());
      TEST_ASSERT_NULL(dev.getConfig().i2cWrite);
      TEST_ASSERT_NULL(dev.getConfig().i2cWriteRead);
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                              static_cast<uint8_t>(dev.state()));
    }
  }
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

void test_begin_apply_write_failures_reset_cache_without_health_counts() {
  struct Case {
    uint8_t reg;
    uint8_t nth;
  };
  const Case cases[] = {
      {cmd::REG_CONFIG, 1},
      {cmd::REG_DIAG_ALRT, 1},
      {cmd::REG_SHUNT_TEMPCO, 1},
      {cmd::REG_ADC_CONFIG, 1},
      {cmd::REG_SHUNT_CAL, 1},
  };

  for (const Case& c : cases) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    cfg.shuntTempCoeffPpmC = 3900;
    bus.writeError = Status::Error(Err::I2C_ERROR, "forced begin write error", -61);
    queueNthWriteFailure(bus, c.reg, c.nth);

    Status st = dev.begin(cfg);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(-61, st.detail);
    TEST_ASSERT_FALSE(dev.isInitialized());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                            static_cast<uint8_t>(dev.state()));
    TEST_ASSERT_NULL(dev.getConfig().i2cWrite);
    TEST_ASSERT_NULL(dev.getConfig().i2cWriteRead);
    TEST_ASSERT_EQUAL_HEX8(0x40, dev.getConfig().i2cAddress);
    TEST_ASSERT_EQUAL_UINT32(0u, dev.totalSuccess());
    TEST_ASSERT_EQUAL_UINT32(0u, dev.totalFailures());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dev.currentLsb());

    SettingsSnapshot settings{};
    TEST_ASSERT_TRUE(dev.getSettings(settings).ok());
    TEST_ASSERT_FALSE(settings.initialized);
    TEST_ASSERT_TRUE(settings.hardwareDirty);
    TEST_ASSERT_TRUE((settings.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
    TEST_ASSERT_TRUE((settings.dirtyRegisterMask & (uint64_t{1} << cmd::REG_ADC_CONFIG)) != 0);
    TEST_ASSERT_TRUE((settings.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);
    TEST_ASSERT_TRUE(settings.thresholdsDirty);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(settings.hardwareDirtyCause.code));

    DiagAlertSnapshot diag{};
    TEST_ASSERT_TRUE(dev.getDiagAlertSnapshot(diag).ok());
    TEST_ASSERT_FALSE(diag.valid);
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
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::DEGRADED),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(dev.lastError().code));
  TEST_ASSERT_EQUAL_UINT32(bus.nowMs, dev.lastErrorMs());
}

void test_recover_success_returns_ready() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_ALATCH |
                 cmd::DIAG_MATHOF | cmd::DIAG_BUSOL;
  uint16_t raw = 0;
  TEST_ASSERT_TRUE(dev.readDiagAlertRaw(raw).ok());
  DiagAlertSnapshot diag{};
  TEST_ASSERT_TRUE(dev.getDiagAlertSnapshot(diag).ok());
  TEST_ASSERT_TRUE(diag.diag.mathOF);
  TEST_ASSERT_TRUE(diag.diag.busOL);
  bus.diagAlrt = cmd::DIAG_MEMSTAT;

  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::I2C_ERROR, "forced recover error", -9);
  (void)dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::DEGRADED),
                          static_cast<uint8_t>(dev.state()));

  bus.nowMs = 4321;
  Status st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::READY),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_EQUAL_UINT8(0u, dev.consecutiveFailures());
  TEST_ASSERT_EQUAL_UINT32(4321u, dev.lastOkMs());

  TEST_ASSERT_TRUE(dev.getDiagAlertSnapshot(diag).ok());
  TEST_ASSERT_TRUE(diag.valid);
  TEST_ASSERT_FALSE(diag.diag.mathOF);
  TEST_ASSERT_FALSE(diag.diag.busOL);
}

void test_recover_preserves_transport_error_code() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::I2C_NACK_ADDR, "forced recover nack", 7);
  Status st = dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_NACK_ADDR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_NACK_ADDR),
                          static_cast<uint8_t>(dev.lastError().code));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::DEGRADED),
                          static_cast<uint8_t>(dev.state()));
}

void test_recover_preserves_transport_errors_for_identity_memstat_reads() {
  const uint8_t regs[] = {
      cmd::REG_MANUFACTURER_ID,
      cmd::REG_DEVICE_ID,
      cmd::REG_DIAG_ALRT,
  };
  const Err codes[] = {
      Err::I2C_NACK_ADDR,
      Err::I2C_NACK_DATA,
      Err::I2C_TIMEOUT,
      Err::I2C_BUS,
      Err::I2C_ERROR,
      Err::TIMEOUT,
  };

  for (uint8_t reg : regs) {
    for (Err code : codes) {
      FakeBus bus;
      INA228::INA228 dev;
      TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
      clearReadHistory(bus);
      bus.readError = Status::Error(code, "forced recover read error", -62);
      queueNthReadFailure(bus, reg, 1);

      Status st = dev.recover();
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(code),
                              static_cast<uint8_t>(st.code));
      TEST_ASSERT_EQUAL_INT32(-62, st.detail);
      TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(code),
                              static_cast<uint8_t>(dev.lastError().code));
    }
  }
}

void test_recover_identity_mismatch_updates_health() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.manufacturerId = 0x1234;
  Status st = dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_ID_MISMATCH),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(1u, dev.totalFailures());
  TEST_ASSERT_EQUAL_UINT8(1u, dev.consecutiveFailures());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::DEGRADED),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_ID_MISMATCH),
                          static_cast<uint8_t>(dev.lastError().code));
}

void test_recover_memstat_failure_updates_health() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.diagAlrt = 0;
  Status st = dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MEMORY_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(1u, dev.totalFailures());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::DEGRADED),
                          static_cast<uint8_t>(dev.state()));
}

void test_recover_reaches_offline_when_threshold_is_one() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.offlineThreshold = 1;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::I2C_ERROR, "forced timeout", -10);
  Status st = dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::OFFLINE),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_FALSE(dev.isOnline());
}

void test_offline_read_bus_voltage_returns_busy_without_i2c() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.offlineThreshold = 1;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::I2C_ERROR, "forced timeout", -10);
  (void)dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::OFFLINE),
                          static_cast<uint8_t>(dev.state()));

  const uint32_t readsBefore = bus.readCalls;
  const uint32_t writesBefore = bus.writeCalls;
  float volts = 0.0f;
  Status st = dev.readBusVoltage(volts);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_STRING("Driver is offline; call recover()", st.msg);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);
}

void test_failed_recover_from_offline_keeps_latch_after_intermediate_success() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.offlineThreshold = 3;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  for (uint8_t i = 0; i < cfg.offlineThreshold; ++i) {
    bus.readErrorRemaining = 1;
    bus.readError = Status::Error(Err::I2C_ERROR, "forced recover error", -20);
    TEST_ASSERT_FALSE(dev.recover().ok());
  }
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::OFFLINE),
                          static_cast<uint8_t>(dev.state()));

  bus.deviceId = 0x1234;
  const Status st = dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_ID_MISMATCH),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::OFFLINE),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_TRUE(dev.consecutiveFailures() >= cfg.offlineThreshold);
}

void test_offline_reset_accumulators_returns_busy_without_i2c() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.offlineThreshold = 1;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::I2C_ERROR, "forced timeout", -10);
  (void)dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::OFFLINE),
                          static_cast<uint8_t>(dev.state()));

  const uint32_t readsBefore = bus.readCalls;
  const uint32_t writesBefore = bus.writeCalls;
  Status st = dev.resetAccumulators();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_STRING("Driver is offline; call recover()", st.msg);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);
}

void test_offline_poll_measurement_ready_returns_busy_without_i2c() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.offlineThreshold = 1;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  bus.readErrorRemaining = 1;
  bus.readError = Status::Error(Err::I2C_ERROR, "forced timeout", -10);
  (void)dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::OFFLINE),
                          static_cast<uint8_t>(dev.state()));

  const uint32_t readsBefore = bus.readCalls;
  const uint32_t writesBefore = bus.writeCalls;
  bool ready = true;
  Status st = dev.pollMeasurementReady(bus.nowMs, 1, ready);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::BUSY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_FALSE(ready);
  TEST_ASSERT_EQUAL_STRING("Driver is offline; call recover()", st.msg);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT32(writesBefore, bus.writeCalls);
}

void test_soft_reset_verifies_identity_memstat_and_replays_cache() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  cfg.shuntTempCoeffPpmC = 3900;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  TEST_ASSERT_TRUE(dev.setAlertLatch(true).ok());

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_ALATCH |
                 cmd::DIAG_MATHOF | cmd::DIAG_BUSOL;
  uint16_t raw = 0;
  TEST_ASSERT_TRUE(dev.readDiagAlertRaw(raw).ok());
  DiagAlertSnapshot diag{};
  TEST_ASSERT_TRUE(dev.getDiagAlertSnapshot(diag).ok());
  TEST_ASSERT_TRUE(diag.diag.mathOF);
  TEST_ASSERT_TRUE(diag.diag.busOL);

  clearReadHistory(bus);
  clearWriteHistory(bus);
  bus.configResetReadsRemaining = 1;
  Status st = dev.softReset();
  TEST_ASSERT_TRUE(st.ok());

  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(7u, bus.writeHistoryCount);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_CONFIG, bus.writeHistory[0].reg);
  TEST_ASSERT_EQUAL_HEX16(cmd::CONFIG_RST, bus.writeHistory[0].value);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG, bus.writeHistory[1].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_CONFIG, bus.writeHistory[2].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_DIAG_ALRT, bus.writeHistory[3].reg);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_ALATCH, bus.writeHistory[3].value);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_SHUNT_TEMPCO, bus.writeHistory[4].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_SHUNT_CAL, bus.writeHistory[5].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG, bus.writeHistory[6].reg);

  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_CONFIG));
  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_MANUFACTURER_ID));
  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_DEVICE_ID));
  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_DIAG_ALRT));
  TEST_ASSERT_EQUAL_HEX16(0x0FD2u, bus.reg16[cmd::REG_SHUNT_CAL]);
  TEST_ASSERT_EQUAL_HEX16(3900u, bus.reg16[cmd::REG_SHUNT_TEMPCO]);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_ALATCH,
                          bus.diagAlrt & cmd::DIAG_CONFIG_MASK);
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_CONFIG] &
                              (cmd::CONFIG_RST | cmd::CONFIG_RSTACC));

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(0u, snap.dirtyRegisterMask);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::OK),
                          static_cast<uint8_t>(snap.hardwareDirtyCause.code));
  TEST_ASSERT_TRUE(snap.thresholdsDirty);

  TEST_ASSERT_TRUE(dev.getDiagAlertSnapshot(diag).ok());
  TEST_ASSERT_TRUE(diag.valid);
  TEST_ASSERT_FALSE(diag.diag.mathOF);
  TEST_ASSERT_FALSE(diag.diag.busOL);
}

void test_soft_reset_write_failure_marks_reset_domain_dirty() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  clearWriteHistory(bus);
  queueNthWriteFailure(bus, cmd::REG_CONFIG, 1);
  Status st = dev.softReset();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(1u, bus.writeHistoryCount);
  TEST_ASSERT_EQUAL_UINT32(0u, countRegisterWrites(bus, cmd::REG_SHUNT_CAL));

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_ADC_CONFIG)) != 0);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(snap.hardwareDirtyCause.code));
  TEST_ASSERT_TRUE(snap.thresholdsDirty);
}

void test_soft_reset_timeout_marks_dirty_without_replay() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  clearWriteHistory(bus);
  bus.configResetNeverClears = true;
  Status st = dev.softReset();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::TIMEOUT),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(1u, bus.writeHistoryCount);
  TEST_ASSERT_EQUAL_UINT32(0u, countRegisterWrites(bus, cmd::REG_SHUNT_CAL));

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_ADC_CONFIG)) != 0);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_DIAG_ALRT)) != 0);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_TEMPCO)) != 0);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(snap.hardwareDirtyCause.code));
}

void test_soft_reset_read_failures_mark_dirty_without_replay() {
  const uint8_t regs[] = {
      cmd::REG_CONFIG,
      cmd::REG_MANUFACTURER_ID,
      cmd::REG_DEVICE_ID,
      cmd::REG_DIAG_ALRT,
  };

  for (uint8_t reg : regs) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());

    clearReadHistory(bus);
    clearWriteHistory(bus);
    bus.readError = Status::Error(Err::I2C_ERROR, "forced soft reset read error", -71);
    queueNthReadFailure(bus, reg, 1);

    Status st = dev.softReset();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(-71, st.detail);
    TEST_ASSERT_EQUAL_UINT32(1u, bus.writeHistoryCount);
    TEST_ASSERT_EQUAL_UINT32(0u, countRegisterWrites(bus, cmd::REG_SHUNT_CAL));

    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_ADC_CONFIG)) != 0);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);
  }
}

void test_soft_reset_identity_mismatch_marks_dirty_without_replay() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    bus.manufacturerId = 0x1234;
    clearWriteHistory(bus);

    Status st = dev.softReset();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_ID_MISMATCH),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(0x1234, st.detail);
    TEST_ASSERT_EQUAL_UINT32(1u, bus.writeHistoryCount);
    TEST_ASSERT_EQUAL_UINT32(0u, countRegisterWrites(bus, cmd::REG_SHUNT_CAL));

    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    bus.deviceId = 0x4321;
    clearWriteHistory(bus);

    Status st = dev.softReset();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_ID_MISMATCH),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_INT32(0x4321, st.detail);
    TEST_ASSERT_EQUAL_UINT32(1u, bus.writeHistoryCount);
    TEST_ASSERT_EQUAL_UINT32(0u, countRegisterWrites(bus, cmd::REG_SHUNT_CAL));

    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);
  }
}

void test_soft_reset_memstat_failure_marks_dirty_without_replay() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.resetLeavesMemstatLow = true;
  clearWriteHistory(bus);

  Status st = dev.softReset();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MEMORY_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(1u, bus.writeHistoryCount);
  TEST_ASSERT_EQUAL_UINT32(0u, countRegisterWrites(bus, cmd::REG_SHUNT_CAL));

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);
}

void test_reset_accumulators_clears_rstacc_and_invalidates_until_cnvrf() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);
  bus.reg40[cmd::REG_ENERGY] = 123;
  bus.reg40[cmd::REG_CHARGE] = 456;
  bus.diagAlrt |= cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF | cmd::DIAG_MATHOF;
  uint16_t raw = 0;
  TEST_ASSERT_TRUE(dev.readDiagAlertRaw(raw).ok());
  DiagAlertSnapshot diag{};
  TEST_ASSERT_TRUE(dev.getDiagAlertSnapshot(diag).ok());
  TEST_ASSERT_TRUE(diag.diag.energyOF);
  TEST_ASSERT_TRUE(diag.diag.chargeOF);
  TEST_ASSERT_TRUE(diag.diag.mathOF);

  Status st = dev.resetAccumulators();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT64(0u, bus.reg40[cmd::REG_ENERGY]);
  TEST_ASSERT_EQUAL_UINT64(0u, bus.reg40[cmd::REG_CHARGE]);
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_CONFIG] & cmd::CONFIG_RSTACC);
  TEST_ASSERT_EQUAL_HEX16(0u, bus.diagAlrt &
                              (cmd::DIAG_ENERGYOF | cmd::DIAG_CHARGEOF | cmd::DIAG_MATHOF));
  TEST_ASSERT_TRUE(dev.getDiagAlertSnapshot(diag).ok());
  TEST_ASSERT_TRUE(diag.valid);
  TEST_ASSERT_FALSE(diag.diag.cnvrf);
  TEST_ASSERT_FALSE(diag.diag.energyOF);
  TEST_ASSERT_FALSE(diag.diag.chargeOF);
  TEST_ASSERT_FALSE(diag.diag.mathOF);

  double energy = 1.0;
  st = dev.readEnergy(energy);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_INVALID),
                          static_cast<uint8_t>(st.code));

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs + 1u);
  st = dev.readEnergy(energy);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, static_cast<float>(energy));
}

void test_reset_accumulators_write_failure_marks_dirty_and_invalidates() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);
  clearWriteHistory(bus);
  queueNthWriteFailure(bus, cmd::REG_CONFIG, 1);

  Status st = dev.resetAccumulators();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);

  double energy = 1.0;
  st = dev.readEnergy(energy);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(st.code));
}

void test_reset_accumulators_later_failures_mark_dirty_and_invalidate() {
  struct Case {
    uint8_t failSecondWrite;
    uint8_t failReadback;
    uint8_t stuckReadback;
    Err expected;
  };
  const Case cases[] = {
      {1, 0, 0, Err::I2C_ERROR},
      {0, 1, 0, Err::I2C_ERROR},
      {0, 0, 1, Err::TIMEOUT},
  };

  for (const Case& c : cases) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
    dev.tick(bus.nowMs);
    clearReadHistory(bus);
    clearWriteHistory(bus);

    if (c.failSecondWrite != 0) {
      queueNthWriteFailure(bus, cmd::REG_CONFIG, 2);
    }
    if (c.failReadback != 0) {
      bus.readError = Status::Error(Err::I2C_ERROR,
                                    "forced reset accumulator read error", -72);
      queueNthReadFailure(bus, cmd::REG_CONFIG, 1);
    }
    if (c.stuckReadback != 0) {
      bus.configReadOverrideRemaining = 1;
      bus.configReadOverrideValue = cmd::CONFIG_RSTACC;
    }

    Status st = dev.resetAccumulators();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(c.expected),
                            static_cast<uint8_t>(st.code));
    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);

    double energy = 1.0;
    st = dev.readEnergy(energy);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                            static_cast<uint8_t>(st.code));
  }
}

void test_soft_reset_replay_failures_mark_dirty_for_each_write_position() {
  struct Case {
    uint8_t reg;
    uint8_t nth;
  };
  const Case cases[] = {
      {cmd::REG_ADC_CONFIG, 1},
      {cmd::REG_CONFIG, 2},
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
    cfg.shuntTempCoeffPpmC = 3900;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    clearWriteHistory(bus);
    queueNthWriteFailure(bus, c.reg, c.nth);

    Status st = dev.softReset();
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_ADC_CONFIG)) != 0);
    TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);

    const uint32_t readsBefore = bus.readCalls;
    float current = 0.0f;
    st = dev.readCurrent(current);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  }
}

void test_recover_replay_failure_preserves_dirty_until_full_recover() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  TEST_ASSERT_TRUE(dev.setAlertLatch(true).ok());
  TEST_ASSERT_TRUE(dev.writeRegister16(cmd::REG_SHUNT_TEMPCO, 0x1234).ok());

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(snap.hardwareDirtyCause.code));

  clearWriteHistory(bus);
  queueNthWriteFailure(bus, cmd::REG_DIAG_ALRT, 1);
  Status st = dev.recover();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(snap.hardwareDirtyCause.code));
  TEST_ASSERT_EQUAL_HEX16(0x1234u, bus.reg16[cmd::REG_SHUNT_TEMPCO]);

  const uint32_t readsBefore = bus.readCalls;
  float busVoltage = 0.0f;
  st = dev.readBusVoltage(busVoltage);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);

  st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_SHUNT_TEMPCO]);
  TEST_ASSERT_EQUAL_HEX16(cmd::DIAG_ALATCH,
                          bus.diagAlrt & cmd::DIAG_CONFIG_MASK);
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.hardwareDirty);
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

    clearWriteHistory(bus);
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

  Wire._setEndTransmissionResult(0);
  Wire._setRequestFromResult(1);
  st = transport::wireWriteRead(0x40, &tx, 1, &rx, 1, 50, &Wire);
  TEST_ASSERT_TRUE(st.ok());

  Wire._setRequestFromResult(0);
  st = transport::wireWriteRead(0x40, &tx, 1, &rx, 1, 50, &Wire);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
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
  cfg.mode = Mode::TRIG_ALL;
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
    cfg.mode = Mode::TRIG_ALL;
    cfg.vbusConvTime = ConvTime::US_4120;
    cfg.vshuntConvTime = ConvTime::US_4120;
    cfg.vtempConvTime = ConvTime::US_4120;
    cfg.averaging = Averaging::AVG_1024;
    cfg.convDelayMs2 = 255;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_EQUAL_HEX16(0x7FFFu, bus.reg16[cmd::REG_ADC_CONFIG]);
    TEST_ASSERT_EQUAL_HEX16(0x3FC0u, bus.reg16[cmd::REG_CONFIG]);
  }
}

void test_begin_marks_each_triggered_mode_pending() {
  const Mode modes[] = {
      Mode::TRIG_BUS,        Mode::TRIG_SHUNT,      Mode::TRIG_SHUNT_BUS,
      Mode::TRIG_TEMP,       Mode::TRIG_TEMP_BUS,   Mode::TRIG_TEMP_SHUNT,
      Mode::TRIG_ALL,
  };

  for (Mode mode : modes) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.mode = mode;
    bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF | cmd::DIAG_BUSOL;

    Status st = dev.begin(cfg);
    TEST_ASSERT_TRUE(st.ok());

    SettingsSnapshot snap{};
    st = dev.getSettings(snap);
    TEST_ASSERT_TRUE(st.ok());
    TEST_ASSERT_TRUE(snap.triggeredConversionPending);
    TEST_ASSERT_EQUAL_UINT32(bus.nowMs, snap.triggeredConversionStartMs);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(mode), static_cast<uint8_t>(snap.mode));

    const uint16_t modeBits =
        static_cast<uint16_t>(mode) << cmd::BIT_ADC_MODE;
    TEST_ASSERT_EQUAL_UINT16(modeBits,
                             bus.reg16[cmd::REG_ADC_CONFIG] & cmd::MASK_ADC_MODE);
    TEST_ASSERT_FALSE((bus.diagAlrt & cmd::DIAG_CNVRF) != 0);
  }
}

void test_begin_triggered_reads_do_not_return_stale_registers_before_cnvrf() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.mode = Mode::TRIG_BUS;
  bus.reg24[cmd::REG_VBUS] = 0x001000;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  const uint32_t readsBefore = bus.readCalls;
  float volts = 123.0f;
  Status st = dev.readBusVoltage(volts);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MEASUREMENT_NOT_READY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_FLOAT(123.0f, volts);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);

  RawSample raw{};
  raw.vbus = 0x12345u;
  st = dev.readRawSample(raw);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MEASUREMENT_NOT_READY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(0x12345u, raw.vbus);
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
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
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::SHUTDOWN),
                          static_cast<uint8_t>(snap.mode));
  TEST_ASSERT_EQUAL_UINT32(0u, dev.estimateConversionTimeUs());
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

  Status st = dev.triggerConversion(Mode::TRIG_ALL);
  TEST_ASSERT_TRUE(st.inProgress());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;

  const uint32_t dueMs = dev.estimateConversionTimeMs();
  dev.tick(dueMs);

  SettingsSnapshot settings{};
  st = dev.getSettings(settings);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(settings.triggeredConversionPending);
  TEST_ASSERT_EQUAL_UINT32(0u, settings.triggeredConversionStartMs);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::SHUTDOWN),
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
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::SHUTDOWN),
                          static_cast<uint8_t>(settings.mode));
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
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Mode::SHUTDOWN),
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

void test_tick_does_not_overwrite_diag_alert_snapshot_when_no_state_pending() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.clearDiagOnRead = true;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF | cmd::DIAG_SHNTOL;
  const uint32_t readsBefore = bus.readCalls;

  dev.tick(bus.nowMs);
  TEST_ASSERT_TRUE(bus.readCalls > readsBefore);

  DiagAlertSnapshot snap{};
  Status st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.diag.cnvrf);
  TEST_ASSERT_TRUE(snap.diag.shntOL);

  const uint32_t readsAfterReady = bus.readCalls;
  bus.diagAlrt = cmd::DIAG_MEMSTAT;
  dev.tick(bus.nowMs + 1u);
  TEST_ASSERT_EQUAL_UINT32(readsAfterReady, bus.readCalls);

  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.diag.cnvrf);
  TEST_ASSERT_TRUE(snap.diag.shntOL);

  bool ready = true;
  st = dev.pollConversionReady(bus.nowMs + 2u, ready);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE(ready);

  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.diag.cnvrf);
  TEST_ASSERT_TRUE(snap.diag.shntOL);
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

void test_read_measurement_all_zero_when_calibrated() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.1f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  Measurement m{};
  m.busVoltageV = 99.9f;
  Status st = dev.readMeasurement(m);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.busVoltageV);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.shuntVoltageV);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.temperatureC);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.currentA);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, m.powerW);
}

void test_continuous_mode_allows_energy_and_charge_after_cnvrf_when_calibrated() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.1f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  bus.reg40[cmd::REG_ENERGY] = 10;
  bus.reg40[cmd::REG_CHARGE] = 20;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);
  clearReadHistory(bus);

  const double expectedEnergy =
      cmd::ENERGY_COEFF * cmd::POWER_COEFF *
      static_cast<double>(dev.currentLsb()) * 10.0;
  const double expectedCharge = static_cast<double>(dev.currentLsb()) * 20.0;

  double value = -1.0;
  Status st = dev.readEnergy(value);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(static_cast<float>(expectedEnergy * 0.0001),
                           static_cast<float>(expectedEnergy),
                           static_cast<float>(value));

  value = -1.0;
  st = dev.readCharge(value);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(static_cast<float>(expectedCharge * 0.0001),
                           static_cast<float>(expectedCharge),
                           static_cast<float>(value));

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.valid);
  TEST_ASSERT_FALSE(snap.diag.energyOF);
  TEST_ASSERT_FALSE(snap.diag.chargeOF);
  TEST_ASSERT_FALSE(snap.diag.mathOF);
}

void test_triggered_modes_reject_energy_charge_without_accumulator_i2c() {
  const Mode modes[] = {
      Mode::TRIG_BUS,        Mode::TRIG_SHUNT,      Mode::TRIG_SHUNT_BUS,
      Mode::TRIG_TEMP,       Mode::TRIG_TEMP_BUS,   Mode::TRIG_TEMP_SHUNT,
      Mode::TRIG_ALL,
  };

  for (Mode mode : modes) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.mode = mode;
    cfg.shuntResistanceOhm = 0.1f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());

    bus.nowMs += dev.estimateConversionTimeMs();
    bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
    dev.tick(bus.nowMs);
    bus.reg40[cmd::REG_ENERGY] = 10;
    bus.reg40[cmd::REG_CHARGE] = 20;
    clearReadHistory(bus);

    double value = 99.0;
    Status st = dev.readEnergy(value);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_INVALID),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 99.0f, static_cast<float>(value));

    value = 88.0;
    st = dev.readCharge(value);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_INVALID),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 88.0f, static_cast<float>(value));
    TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_ENERGY));
    TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_CHARGE));
  }
}

void test_shutdown_modes_reject_energy_charge_without_accumulator_i2c() {
  const Mode modes[] = {Mode::SHUTDOWN, Mode::SHUTDOWN2};

  for (Mode mode : modes) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.mode = mode;
    cfg.shuntResistanceOhm = 0.1f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    bus.reg40[cmd::REG_ENERGY] = 10;
    bus.reg40[cmd::REG_CHARGE] = 20;
    clearReadHistory(bus);

    double value = 99.0;
    Status st = dev.readEnergy(value);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_INVALID),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 99.0f, static_cast<float>(value));

    value = 88.0;
    st = dev.readCharge(value);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_INVALID),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FLOAT_WITHIN(0.000001f, 88.0f, static_cast<float>(value));
    TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_ENERGY));
    TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_CHARGE));
  }
}

void test_read_measurement_marks_invalid_accumulation_without_accumulator_i2c() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.mode = Mode::SHUTDOWN;
  cfg.shuntResistanceOhm = 0.1f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.reg24[cmd::REG_VBUS] = 0x001000;
  bus.reg40[cmd::REG_ENERGY] = 10;
  bus.reg40[cmd::REG_CHARGE] = 20;
  clearReadHistory(bus);

  Measurement m{};
  m.energyJ = 123.0;
  m.chargeC = 456.0;
  Status st = dev.readMeasurement(m);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(m.busVoltageV > 0.0f);
  TEST_ASSERT_FALSE(m.energyValid);
  TEST_ASSERT_FALSE(m.chargeValid);
  TEST_ASSERT_FALSE(m.diagAlertValid);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.0f, static_cast<float>(m.energyJ));
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 0.0f, static_cast<float>(m.chargeC));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_ENERGY));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_CHARGE));
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

void test_read_energy_preserves_energy_overflow_and_reports_status() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.1f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);

  bus.reg40[cmd::REG_ENERGY] = 10;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_ENERGYOF;
  clearReadHistory(bus);

  double value = 99.0;
  Status st = dev.readEnergy(value);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE((st.detail & cmd::DIAG_ENERGYOF) != 0);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 99.0f, static_cast<float>(value));
  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_DIAG_ALRT));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_ENERGY));

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.diag.energyOF);
}

void test_read_charge_preserves_charge_overflow_and_reports_status() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.1f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);

  bus.reg40[cmd::REG_CHARGE] = 20;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CHARGEOF;
  clearReadHistory(bus);

  double value = 88.0;
  Status st = dev.readCharge(value);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE((st.detail & cmd::DIAG_CHARGEOF) != 0);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 88.0f, static_cast<float>(value));
  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_DIAG_ALRT));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_CHARGE));

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.diag.chargeOF);
}

void test_math_overflow_blocks_accumulation_reads_and_is_preserved() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.1f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);

  bus.reg40[cmd::REG_ENERGY] = 10;
  bus.reg40[cmd::REG_CHARGE] = 20;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_MATHOF;
  clearReadHistory(bus);

  double value = 77.0;
  Status st = dev.readEnergy(value);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MATH_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE((st.detail & cmd::DIAG_MATHOF) != 0);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 77.0f, static_cast<float>(value));

  value = 66.0;
  st = dev.readCharge(value);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MATH_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE((st.detail & cmd::DIAG_MATHOF) != 0);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 66.0f, static_cast<float>(value));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_ENERGY));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_CHARGE));

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.diag.mathOF);
}

void test_math_overflow_blocks_current_power_and_measurement() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.1f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_MATHOF;
  bus.reg24[cmd::REG_CURRENT] = 0x100000;
  bus.reg24[cmd::REG_POWER] = 0x100000;
  clearReadHistory(bus);

  float current = 123.0f;
  Status st = dev.readCurrent(current);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MATH_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE((st.detail & cmd::DIAG_MATHOF) != 0);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 123.0f, current);
  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_DIAG_ALRT));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_CURRENT));

  clearReadHistory(bus);
  float power = 456.0f;
  st = dev.readPower(power);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MATH_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE((st.detail & cmd::DIAG_MATHOF) != 0);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 456.0f, power);
  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_DIAG_ALRT));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_POWER));

  clearReadHistory(bus);
  Measurement m{};
  m.currentA = 789.0f;
  st = dev.readMeasurement(m);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MATH_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE((st.detail & cmd::DIAG_MATHOF) != 0);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 789.0f, m.currentA);
  TEST_ASSERT_TRUE(wasRegisterRead(bus, cmd::REG_DIAG_ALRT));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_VSHUNT));

  DiagAlertSnapshot snap{};
  st = dev.getDiagAlertSnapshot(snap);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(snap.diag.mathOF);
}

void test_reset_accumulators_invalidates_until_next_continuous_cnvrf() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.1f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);

  bus.reg40[cmd::REG_ENERGY] = 10;
  double value = -1.0;
  Status st = dev.readEnergy(value);
  TEST_ASSERT_TRUE(st.ok());

  st = dev.resetAccumulators();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT64(0u, bus.reg40[cmd::REG_ENERGY]);
  TEST_ASSERT_EQUAL_UINT64(0u, bus.reg40[cmd::REG_CHARGE]);
  clearReadHistory(bus);

  value = 99.0;
  st = dev.readEnergy(value);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_INVALID),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, 99.0f, static_cast<float>(value));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_ENERGY));

  bus.reg40[cmd::REG_ENERGY] = 4;
  bus.reg40[cmd::REG_CHARGE] = 5;
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);

  const double expectedEnergy =
      cmd::ENERGY_COEFF * cmd::POWER_COEFF *
      static_cast<double>(dev.currentLsb()) * 4.0;
  const double expectedCharge = static_cast<double>(dev.currentLsb()) * 5.0;

  st = dev.readEnergy(value);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(static_cast<float>(expectedEnergy * 0.0001),
                           static_cast<float>(expectedEnergy),
                           static_cast<float>(value));

  value = -1.0;
  st = dev.readCharge(value);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(static_cast<float>(expectedCharge * 0.0001),
                           static_cast<float>(expectedCharge),
                           static_cast<float>(value));
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

void test_scalar_power_energy_charge_lsb_vectors() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);

  bus.reg24[cmd::REG_POWER] = 0x000001u;
  float power = 0.0f;
  Status st = dev.readPower(power);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.0000001f, 0.00006103515625f, power);

  bus.reg24[cmd::REG_POWER] = 0xFFFFFFu;
  st = dev.readPower(power);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1023.99993896484375f, power);

  bus.reg40[cmd::REG_ENERGY] = 0x0000000001ULL;
  double energy = 0.0;
  st = dev.readEnergy(energy);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.0000001f, 0.0009765625f,
                           static_cast<float>(energy));

  bus.reg40[cmd::REG_CHARGE] = 0x0000000001ULL;
  double charge = 0.0;
  st = dev.readCharge(charge);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.0000001f, 0.000019073486328125f,
                           static_cast<float>(charge));
}

void test_read_measurement_failures_leave_output_unchanged() {
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

    Measurement m = sentinelMeasurement();
    Status st = dev.readMeasurement(m);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    assertSentinelMeasurement(m);
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

void test_power_sample_raw_step_respects_budget_one() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  loadPositiveMeasurementRegisters(bus);
  clearReadHistory(bus);

  RawSample raw = sentinelRawSample();
  IntegerSample integer = sentinelIntegerSample();
  const uint8_t expected[] = {
      cmd::REG_VSHUNT,
      cmd::REG_VBUS,
      cmd::REG_DIETEMP,
      cmd::REG_CURRENT,
      cmd::REG_POWER,
  };

  for (size_t i = 0; i < sizeof(expected); ++i) {
    Status st = dev.readPowerSampleRawStep(raw, integer, 1);
    if (i + 1U < sizeof(expected)) {
      TEST_ASSERT_TRUE(st.inProgress());
      assertSentinelRawSample(raw);
      assertSentinelIntegerSample(integer);
    } else {
      TEST_ASSERT_TRUE(st.ok());
    }
    TEST_ASSERT_EQUAL_UINT32(i + 1U, bus.readHistoryCount);
    TEST_ASSERT_EQUAL_HEX8(expected[i], bus.readHistory[i]);
  }

  assertPositivePowerRawSample(raw);
  assertPositiveIntegerSample(integer);
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_DIAG_ALRT));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_ENERGY));
  TEST_ASSERT_FALSE(wasRegisterRead(bus, cmd::REG_CHARGE));
}

void test_power_sample_raw_step_respects_budget_two() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  loadPositiveMeasurementRegisters(bus);
  clearReadHistory(bus);

  RawSample raw = sentinelRawSample();
  IntegerSample integer = sentinelIntegerSample();

  Status st = dev.readPowerSampleRawStep(raw, integer, 2);
  TEST_ASSERT_TRUE(st.inProgress());
  TEST_ASSERT_EQUAL_UINT32(2u, bus.readHistoryCount);
  assertSentinelRawSample(raw);
  assertSentinelIntegerSample(integer);

  st = dev.readPowerSampleRawStep(raw, integer, 2);
  TEST_ASSERT_TRUE(st.inProgress());
  TEST_ASSERT_EQUAL_UINT32(4u, bus.readHistoryCount);
  assertSentinelRawSample(raw);
  assertSentinelIntegerSample(integer);

  st = dev.readPowerSampleRawStep(raw, integer, 2);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(5u, bus.readHistoryCount);
  assertPositivePowerRawSample(raw);
  assertPositiveIntegerSample(integer);
}

void test_power_sample_raw_step_full_budget_outputs_integer_units() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  loadPositiveMeasurementRegisters(bus);
  clearReadHistory(bus);

  RawSample raw = sentinelRawSample();
  IntegerSample integer = sentinelIntegerSample();
  Status st = dev.readPowerSampleRawStep(raw, integer, 5);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(5u, bus.readHistoryCount);
  assertPositivePowerRawSample(raw);
  assertPositiveIntegerSample(integer);
}

void test_power_sample_step_failure_each_register_clears_job_preserves_outputs() {
  const uint8_t regs[] = {
      cmd::REG_VSHUNT,
      cmd::REG_VBUS,
      cmd::REG_DIETEMP,
      cmd::REG_CURRENT,
      cmd::REG_POWER,
  };

  for (uint8_t reg : regs) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    loadPositiveMeasurementRegisters(bus);
    clearReadHistory(bus);
    queueNthReadFailure(bus, reg, 1);

    RawSample raw = sentinelRawSample();
    IntegerSample integer = sentinelIntegerSample();
    Status st = dev.readPowerSampleRawStep(raw, integer, 5);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));
    assertSentinelRawSample(raw);
    assertSentinelIntegerSample(integer);

    clearReadHistory(bus);
    st = dev.readPowerSampleRawStep(raw, integer, 5);
    TEST_ASSERT_TRUE(st.ok());
    TEST_ASSERT_EQUAL_UINT32(5u, bus.readHistoryCount);
    assertPositivePowerRawSample(raw);
    assertPositiveIntegerSample(integer);
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

void test_zero_budget_fixed_step_calls_are_bus_silent() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  RawSample raw = sentinelRawSample();
  IntegerSample integer = sentinelIntegerSample();
  bool ready = true;

  clearReadHistory(bus);
  clearWriteHistory(bus);
  const uint32_t readsBefore = bus.readCalls;
  const uint32_t writesBefore = bus.writeCalls;

  Status st = dev.pollMeasurementReady(bus.nowMs, 0, ready);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));
  st = dev.readPowerSampleRawStep(raw, integer, 0);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE(dev.startConfigReplayJob().inProgress());
  st = dev.pollConfigReplayJob(bus.nowMs, 0);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_TRUE(dev.pollConfigReplayJob(bus.nowMs, 6).ok());
  TEST_ASSERT_TRUE(dev.startResetJob().inProgress());
  st = dev.pollResetJob(bus.nowMs, 0);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_PARAM),
                          static_cast<uint8_t>(st.code));

  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);
  TEST_ASSERT_EQUAL_UINT32(writesBefore + 6u, bus.writeCalls);
  TEST_ASSERT_EQUAL_UINT32(6u, bus.writeHistoryCount);
  assertSentinelRawSample(raw);
  assertSentinelIntegerSample(integer);
}

void test_apply_calibration_job_marks_adc_dirty_when_later_write_fails() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  cfg.shuntTempCoeffPpmC = 3900;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  clearWriteHistory(bus);
  queueNthWriteFailure(bus, cmd::REG_SHUNT_CAL, 1);

  Status st = dev.startApplyCalibration();
  TEST_ASSERT_TRUE(st.inProgress());
  st = dev.pollApplyCalibration(bus.nowMs, 6);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(5u, bus.writeHistoryCount);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG, bus.writeHistory[0].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_CONFIG, bus.writeHistory[1].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_DIAG_ALRT, bus.writeHistory[2].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_SHUNT_TEMPCO, bus.writeHistory[3].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_SHUNT_CAL, bus.writeHistory[4].reg);
  TEST_ASSERT_FALSE(bus.writeHistory[4].success);

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_ADC_CONFIG)) != 0);
}

void test_config_replay_job_aliases_share_apply_calibration_job() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_TRUE(dev.startConfigReplayJob().inProgress());
    TEST_ASSERT_TRUE(dev.pollApplyCalibration(bus.nowMs, 6).ok());
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_TRUE(dev.startApplyCalibration().inProgress());
    TEST_ASSERT_TRUE(dev.pollConfigReplayJob(bus.nowMs, 6).ok());
  }
}

void test_apply_replay_failure_each_step_marks_exact_dirty_register() {
  struct Case {
    uint8_t failReg;
    uint8_t failMatch;
    uint64_t expectedMask;
  };
  const Case cases[] = {
      {cmd::REG_ADC_CONFIG, 1, uint64_t{1} << cmd::REG_ADC_CONFIG},
      {cmd::REG_CONFIG, 1,
       (uint64_t{1} << cmd::REG_CONFIG) | (uint64_t{1} << cmd::REG_ADC_CONFIG)},
      {cmd::REG_DIAG_ALRT, 1,
       (uint64_t{1} << cmd::REG_DIAG_ALRT) | (uint64_t{1} << cmd::REG_ADC_CONFIG)},
      {cmd::REG_SHUNT_TEMPCO, 1,
       (uint64_t{1} << cmd::REG_SHUNT_TEMPCO) | (uint64_t{1} << cmd::REG_ADC_CONFIG)},
      {cmd::REG_SHUNT_CAL, 1,
       (uint64_t{1} << cmd::REG_SHUNT_CAL) | (uint64_t{1} << cmd::REG_ADC_CONFIG)},
      {cmd::REG_ADC_CONFIG, 2, uint64_t{1} << cmd::REG_ADC_CONFIG},
  };

  for (const Case& c : cases) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    cfg.shuntTempCoeffPpmC = 3900;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    clearWriteHistory(bus);
    queueNthWriteFailure(bus, c.failReg, c.failMatch);

    TEST_ASSERT_TRUE(dev.startConfigReplayJob().inProgress());
    Status st = dev.pollConfigReplayJob(bus.nowMs, 6);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(st.code));

    SettingsSnapshot snap{};
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_TRUE(snap.hardwareDirty);
    TEST_ASSERT_EQUAL_UINT64(c.expectedMask, snap.dirtyRegisterMask);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                            static_cast<uint8_t>(snap.hardwareDirtyCause.code));

    st = dev.startConfigReplayJob();
    TEST_ASSERT_TRUE(st.inProgress());
    st = dev.pollConfigReplayJob(bus.nowMs, 6);
    TEST_ASSERT_TRUE(st.ok());
    TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
    TEST_ASSERT_FALSE(snap.hardwareDirty);
    TEST_ASSERT_EQUAL_UINT64(0u, snap.dirtyRegisterMask);
  }
}

void test_reset_job_budget_one_delay_and_reidentification() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  cfg.shuntTempCoeffPpmC = 3900;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  TEST_ASSERT_TRUE(dev.setAlertLatch(true).ok());
  clearReadHistory(bus);
  clearWriteHistory(bus);
  bus.configResetReadsRemaining = 1;

  Status st = dev.startResetJob();
  TEST_ASSERT_TRUE(st.inProgress());

  uint32_t lastReads = bus.readCalls;
  uint32_t lastWrites = bus.writeCalls;
  st = dev.pollResetJob(bus.nowMs, 1);
  TEST_ASSERT_TRUE(st.inProgress());
  TEST_ASSERT_EQUAL_UINT32(0u, bus.readCalls - lastReads);
  TEST_ASSERT_EQUAL_UINT32(1u, bus.writeCalls - lastWrites);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_CONFIG, bus.writeHistory[0].reg);
  TEST_ASSERT_EQUAL_HEX16(cmd::CONFIG_RST, bus.writeHistory[0].value);

  lastReads = bus.readCalls;
  lastWrites = bus.writeCalls;
  st = dev.pollResetJob(bus.nowMs, 1);
  TEST_ASSERT_TRUE(st.inProgress());
  TEST_ASSERT_EQUAL_UINT32(0u, bus.readCalls - lastReads);
  TEST_ASSERT_EQUAL_UINT32(0u, bus.writeCalls - lastWrites);

  bus.nowMs += 1;
  for (uint8_t i = 0; i < 20 && st.inProgress(); ++i) {
    lastReads = bus.readCalls;
    lastWrites = bus.writeCalls;
    st = dev.pollResetJob(bus.nowMs, 1);
    const uint32_t delta = (bus.readCalls - lastReads) +
                           (bus.writeCalls - lastWrites);
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(1u, delta);
  }

  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(5u, bus.readHistoryCount);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_CONFIG, bus.readHistory[0]);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_CONFIG, bus.readHistory[1]);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_MANUFACTURER_ID, bus.readHistory[2]);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_DEVICE_ID, bus.readHistory[3]);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_DIAG_ALRT, bus.readHistory[4]);

  TEST_ASSERT_EQUAL_UINT32(7u, bus.writeHistoryCount);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_CONFIG, bus.writeHistory[0].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG, bus.writeHistory[1].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_CONFIG, bus.writeHistory[2].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_DIAG_ALRT, bus.writeHistory[3].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_SHUNT_TEMPCO, bus.writeHistory[4].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_SHUNT_CAL, bus.writeHistory[5].reg);
  TEST_ASSERT_EQUAL_HEX8(cmd::REG_ADC_CONFIG, bus.writeHistory[6].reg);

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(0u, snap.dirtyRegisterMask);
  TEST_ASSERT_TRUE(snap.thresholdsDirty);
}

void test_reset_job_failure_each_step_reasserts_offline_when_started_offline() {
  struct Case {
    uint8_t readReg;
    uint8_t readMatch;
    uint8_t writeReg;
    uint8_t writeMatch;
  };
  const Case cases[] = {
      {0, 0, cmd::REG_CONFIG, 1},
      {cmd::REG_CONFIG, 1, 0, 0},
      {cmd::REG_MANUFACTURER_ID, 1, 0, 0},
      {cmd::REG_DEVICE_ID, 1, 0, 0},
      {cmd::REG_DIAG_ALRT, 1, 0, 0},
      {0, 0, cmd::REG_ADC_CONFIG, 1},
      {0, 0, cmd::REG_CONFIG, 2},
      {0, 0, cmd::REG_DIAG_ALRT, 1},
      {0, 0, cmd::REG_SHUNT_TEMPCO, 1},
      {0, 0, cmd::REG_SHUNT_CAL, 1},
      {0, 0, cmd::REG_ADC_CONFIG, 2},
  };

  for (const Case& c : cases) {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    cfg.shuntTempCoeffPpmC = 3900;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    forceOffline(dev, bus);

    if (c.readMatch != 0) {
      queueNthReadFailure(bus, c.readReg, c.readMatch);
    }
    if (c.writeMatch != 0) {
      queueNthWriteFailure(bus, c.writeReg, c.writeMatch);
    }

    TEST_ASSERT_TRUE(dev.startResetJob().inProgress());
    Status st = dev.pollResetJob(bus.nowMs, 1);
    if (c.writeReg == cmd::REG_CONFIG && c.writeMatch == 1) {
      TEST_ASSERT_FALSE(st.ok());
      TEST_ASSERT_FALSE(st.inProgress());
    } else {
      TEST_ASSERT_TRUE(st.inProgress());
      bus.nowMs += 1;
      st = dev.pollResetJob(bus.nowMs, 16);
      TEST_ASSERT_FALSE(st.ok());
      TEST_ASSERT_FALSE(st.inProgress());
    }
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::OFFLINE),
                            static_cast<uint8_t>(dev.state()));
  }
}

void test_calibration_vectors_program_exact_shunt_cal_and_lsb() {
  FakeBus bus0;
  INA228::INA228 dev0;
  Config cfg0 = makeConfig(bus0);
  cfg0.shuntResistanceOhm = 0.0162f;
  cfg0.maxExpectedCurrentA = 10.0f;
  cfg0.adcRange = AdcRange::MV_163_84;
  TEST_ASSERT_TRUE(dev0.begin(cfg0).ok());

  SettingsSnapshot snap0{};
  TEST_ASSERT_TRUE(dev0.getSettings(snap0).ok());
  TEST_ASSERT_TRUE(snap0.calibrated);
  TEST_ASSERT_EQUAL_HEX16(0x0FD2u, snap0.shuntCal);
  TEST_ASSERT_EQUAL_HEX16(0x0FD2u, bus0.reg16[cmd::REG_SHUNT_CAL]);
  TEST_ASSERT_FLOAT_WITHIN(0.000000001f, 0.000019073486328125f, snap0.currentLsb);
  TEST_ASSERT_FALSE(snap0.calibrationClamped);
  TEST_ASSERT_FALSE(snap0.maxCurrentExceedsShuntRange);

  FakeBus bus1;
  INA228::INA228 dev1;
  Config cfg1 = cfg0;
  cfg1.i2cUser = &bus1;
  cfg1.timeUser = &bus1;
  cfg1.adcRange = AdcRange::MV_40_96;
  TEST_ASSERT_TRUE(dev1.begin(cfg1).ok());

  SettingsSnapshot snap1{};
  TEST_ASSERT_TRUE(dev1.getSettings(snap1).ok());
  TEST_ASSERT_TRUE(snap1.calibrated);
  TEST_ASSERT_EQUAL_HEX16(0x3F48u, snap1.shuntCal);
  TEST_ASSERT_EQUAL_HEX16(0x3F48u, bus1.reg16[cmd::REG_SHUNT_CAL]);
  TEST_ASSERT_TRUE((bus1.reg16[cmd::REG_CONFIG] & cmd::CONFIG_ADCRANGE) != 0);
  TEST_ASSERT_FLOAT_WITHIN(0.000000001f, 0.000019073486328125f, snap1.currentLsb);
  TEST_ASSERT_FALSE(snap1.calibrationClamped);
  TEST_ASSERT_TRUE(snap1.maxCurrentExceedsShuntRange);
}

void test_set_adc_range_recomputes_shunt_cal_with_multiplier() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  Status st = dev.setAdcRange(AdcRange::MV_40_96);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE((bus.reg16[cmd::REG_CONFIG] & cmd::CONFIG_ADCRANGE) != 0);
  TEST_ASSERT_EQUAL_HEX16(0x3F48u, bus.reg16[cmd::REG_SHUNT_CAL]);
  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_EQUAL_HEX16(0x3F48u, snap.shuntCal);
  TEST_ASSERT_FLOAT_WITHIN(0.000000001f, 0.000019073486328125f, snap.currentLsb);
  TEST_ASSERT_TRUE(snap.maxCurrentExceedsShuntRange);
  TEST_ASSERT_FALSE(snap.hardwareDirty);

  st = dev.setAdcRange(AdcRange::MV_163_84);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FALSE((bus.reg16[cmd::REG_CONFIG] & cmd::CONFIG_ADCRANGE) != 0);
  TEST_ASSERT_EQUAL_HEX16(0x0FD2u, bus.reg16[cmd::REG_SHUNT_CAL]);
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_EQUAL_HEX16(0x0FD2u, snap.shuntCal);
  TEST_ASSERT_FALSE(snap.maxCurrentExceedsShuntRange);
}

void test_set_adc_range_config_write_failure_marks_config_dirty() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  SettingsSnapshot before{};
  TEST_ASSERT_TRUE(dev.getSettings(before).ok());
  clearWriteHistory(bus);
  queueNthWriteFailure(bus, cmd::REG_CONFIG, 1);

  Status st = dev.setAdcRange(AdcRange::MV_40_96);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  SettingsSnapshot after{};
  TEST_ASSERT_TRUE(dev.getSettings(after).ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcRange::MV_163_84),
                          static_cast<uint8_t>(after.adcRange));
  TEST_ASSERT_EQUAL_HEX16(before.shuntCal, after.shuntCal);
  TEST_ASSERT_FLOAT_WITHIN(0.000000001f, before.currentLsb, after.currentLsb);
  TEST_ASSERT_TRUE(after.hardwareDirty);
  TEST_ASSERT_TRUE((after.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
  TEST_ASSERT_EQUAL_UINT32(1u, bus.writeHistoryCount);
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_CONFIG] & cmd::CONFIG_ADCRANGE);

  float current = 1.0f;
  st = dev.readCurrent(current);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(st.code));

  st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(dev.getSettings(after).ok());
  TEST_ASSERT_FALSE(after.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(0u, after.dirtyRegisterMask);
}

void test_prompt_positive_raw_vector_converts_all_outputs() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);

  bus.reg24[cmd::REG_VSHUNT] = 0x4BF000;
  bus.reg24[cmd::REG_VBUS] = 0x3C0000;
  bus.reg16[cmd::REG_DIETEMP] = 0x0C80;
  bus.reg24[cmd::REG_CURRENT] = 0x4CCCC0;
  bus.reg24[cmd::REG_POWER] = 0x48000C;
  bus.reg40[cmd::REG_ENERGY] = 0x003F480000ULL;
  bus.reg40[cmd::REG_CHARGE] = 0x0043800000ULL;

  RawSample raw{};
  Status st = dev.readRawSample(raw);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_INT32(311040, raw.vshunt);
  TEST_ASSERT_EQUAL_UINT32(245760u, raw.vbus);
  TEST_ASSERT_EQUAL_INT16(3200, raw.dietemp);
  TEST_ASSERT_EQUAL_INT32(314572, raw.current);
  TEST_ASSERT_EQUAL_UINT32(0x48000Cu, raw.power);
  TEST_ASSERT_EQUAL_UINT64(0x003F480000ULL, raw.energy);
  TEST_ASSERT_EQUAL_INT64(1132462080LL, raw.charge);

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  Measurement m{};
  st = dev.readMeasurement(m);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0972f, m.shuntVoltageV);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 48.0f, m.busVoltageV);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 25.0f, m.temperatureC);
  TEST_ASSERT_FLOAT_WITHIN(0.00001f, 5.999984741f, m.currentA);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 288.000732f, m.powerW);
  TEST_ASSERT_TRUE(m.energyValid);
  TEST_ASSERT_TRUE(m.chargeValid);
  TEST_ASSERT_FLOAT_WITHIN(1.0f, 1036800.0f, static_cast<float>(m.energyJ));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 21600.0f, static_cast<float>(m.chargeC));
}

void test_prompt_positive_raw_vector_uses_range1_shunt_lsb() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  cfg.adcRange = AdcRange::MV_40_96;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.reg24[cmd::REG_VSHUNT] = 0x4BF000;

  float shunt = 0.0f;
  Status st = dev.readShuntVoltage(shunt);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.00001f, 0.0243f, shunt);
}

void test_prompt_negative_raw_vectors_sign_extend() {
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

void test_prompt_negative_scalar_conversions() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  dev.tick(bus.nowMs);

  bus.reg24[cmd::REG_VSHUNT] = 0xB41000;
  float shunt = 0.0f;
  Status st = dev.readShuntVoltage(shunt);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.00001f, -0.0972f, shunt);

  bus.reg24[cmd::REG_CURRENT] = 0x800000;
  float current = 0.0f;
  st = dev.readCurrent(current);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.00001f, -10.0f, current);

  bus.reg24[cmd::REG_CURRENT] = 0xFFFFF0;
  st = dev.readCurrent(current);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.0000001f, -0.000019073486328125f, current);

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  bus.reg40[cmd::REG_CHARGE] = 0xFFFFFFFFFFULL;
  double charge = 0.0;
  st = dev.readCharge(charge);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(0.0000001f, -dev.currentLsb(), static_cast<float>(charge));

  bus.diagAlrt = cmd::DIAG_MEMSTAT | cmd::DIAG_CNVRF;
  bus.reg40[cmd::REG_CHARGE] = 0x8000000000ULL;
  st = dev.readCharge(charge);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_FLOAT_WITHIN(1.0f, -10485760.0f, static_cast<float>(charge));
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

void test_read_power_overflow_returns_math_overflow_without_inf_output() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = std::numeric_limits<float>::min();
  cfg.maxExpectedCurrentA = std::numeric_limits<float>::max();
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.reg24[cmd::REG_POWER] = 0xFFFFFF;

  float power = 123.0f;
  Status st = dev.readPower(power);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MATH_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 123.0f, power);
}

void test_read_measurement_overflow_leaves_output_unchanged() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = std::numeric_limits<float>::min();
  cfg.maxExpectedCurrentA = std::numeric_limits<float>::max();
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  bus.reg24[cmd::REG_POWER] = 0xFFFFFF;

  Measurement m{};
  m.busVoltageV = 99.0f;
  Status st = dev.readMeasurement(m);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::MATH_OVERFLOW),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 99.0f, m.busVoltageV);
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

void test_set_adc_range_failure_rolls_back_config_when_shunt_cal_write_fails() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  const uint16_t oldConfigReg = bus.reg16[cmd::REG_CONFIG];
  const uint16_t oldShuntCal = bus.reg16[cmd::REG_SHUNT_CAL];
  queueWriteFailure(bus, cmd::REG_SHUNT_CAL);

  Status st = dev.setAdcRange(AdcRange::MV_40_96);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_HEX16(oldConfigReg, bus.reg16[cmd::REG_CONFIG]);
  TEST_ASSERT_EQUAL_HEX16(oldShuntCal, bus.reg16[cmd::REG_SHUNT_CAL]);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcRange::MV_163_84),
                          static_cast<uint8_t>(dev.getConfig().adcRange));

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);

  float current = 1.0f;
  st = dev.readCurrent(current);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(st.code));

  st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(0u, snap.dirtyRegisterMask);
}

void test_set_adc_range_rollback_failure_marks_dirty_and_recover_resyncs() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  queueWriteFailure(bus, cmd::REG_SHUNT_CAL);
  queueWriteFailure(bus, cmd::REG_CONFIG);

  Status st = dev.setAdcRange(AdcRange::MV_40_96);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_CONFIG)) != 0);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);

  float current = 1.0f;
  const uint32_t readsBefore = bus.readCalls;
  st = dev.readCurrent(current);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);

  st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_CONFIG] & cmd::CONFIG_ADCRANGE);
  TEST_ASSERT_EQUAL_HEX16(0x0FD2u, bus.reg16[cmd::REG_SHUNT_CAL]);
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(0u, snap.dirtyRegisterMask);
}

void test_scale_changes_mark_thresholds_dirty() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.shuntResistanceOhm = 0.0162f;
  cfg.maxExpectedCurrentA = 10.0f;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());
  TEST_ASSERT_TRUE(dev.setShuntOvervoltageThreshold(0.020f).ok());

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.thresholdsDirty);

  TEST_ASSERT_TRUE(dev.setAdcRange(AdcRange::MV_40_96).ok());
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.thresholdsDirty);
}

void test_calibration_clamp_updates_current_lsb_to_actual_register_value() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  Status st = dev.setCalibration(0.1f, 100000.0f);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(cmd::MASK_SHUNT_CAL, bus.reg16[cmd::REG_SHUNT_CAL]);

  const float expected = static_cast<float>(
      static_cast<double>(cmd::MASK_SHUNT_CAL) /
      (cmd::SHUNT_CAL_FACTOR * 0.1));
  TEST_ASSERT_FLOAT_WITHIN(expected * 0.0001f, expected, dev.currentLsb());
  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.calibrationClamped);
}

void test_calibration_does_not_commit_cache_on_write_failure() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  TEST_ASSERT_TRUE(dev.setCalibration(0.1f, 10.0f).ok());
  const Config oldConfig = dev.getConfig();
  const float oldLsb = dev.currentLsb();
  const uint16_t oldReg = bus.reg16[cmd::REG_SHUNT_CAL];

  bus.writeErrorRemaining = 1;
  Status st = dev.setCalibration(0.2f, 20.0f);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, oldConfig.shuntResistanceOhm,
                           dev.getConfig().shuntResistanceOhm);
  TEST_ASSERT_FLOAT_WITHIN(0.000001f, oldConfig.maxExpectedCurrentA,
                           dev.getConfig().maxExpectedCurrentA);
  TEST_ASSERT_FLOAT_WITHIN(oldLsb * 0.0001f, oldLsb, dev.currentLsb());
  TEST_ASSERT_EQUAL_HEX16(oldReg, bus.reg16[cmd::REG_SHUNT_CAL]);

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask & (uint64_t{1} << cmd::REG_SHUNT_CAL)) != 0);

  float current = 0.0f;
  st = dev.readCurrent(current);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(st.code));

  st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_HEX16(oldReg, bus.reg16[cmd::REG_SHUNT_CAL]);
}

void test_config_setter_does_not_commit_cache_on_write_failure_and_marks_dirty() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  bus.writeErrorRemaining = 1;
  Status st = dev.setVbusConvTime(ConvTime::US_50);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::I2C_ERROR),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ConvTime::US_1052),
                          static_cast<uint8_t>(dev.getConfig().vbusConvTime));

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask &
                    (uint64_t{1} << cmd::REG_ADC_CONFIG)) != 0);

  float volts = 0.0f;
  st = dev.readBusVoltage(volts);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(st.code));

  st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(0u, snap.dirtyRegisterMask);
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

void test_raw_config_write_marks_dirty_until_recover_replays_cache() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

  Status st = dev.writeRegister16(cmd::REG_SHUNT_TEMPCO, 0x1234);
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(0x1234u, bus.reg16[cmd::REG_SHUNT_TEMPCO]);

  SettingsSnapshot snap{};
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_TRUE(snap.hardwareDirty);
  TEST_ASSERT_TRUE((snap.dirtyRegisterMask &
                    (uint64_t{1} << cmd::REG_SHUNT_TEMPCO)) != 0);

  float busVoltage = 1.0f;
  const uint32_t readsBefore = bus.readCalls;
  st = dev.readBusVoltage(busVoltage);
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(readsBefore, bus.readCalls);

  st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_HEX16(0u, bus.reg16[cmd::REG_SHUNT_TEMPCO]);
  TEST_ASSERT_TRUE(dev.getSettings(snap).ok());
  TEST_ASSERT_FALSE(snap.hardwareDirty);
  TEST_ASSERT_EQUAL_UINT64(0u, snap.dirtyRegisterMask);
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

void test_status_contracts_for_calibration_accumulation_and_dirty_state() {
  {
    FakeBus bus;
    INA228::INA228 dev;
    TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
    float current = 123.0f;
    Status st = dev.readCurrent(current);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::INVALID_CONFIG),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 123.0f, current);
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    cfg.mode = Mode::SHUTDOWN;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    double energy = 456.0;
    Status st = dev.readEnergy(energy);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::ACCUMULATION_INVALID),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 456.0f, static_cast<float>(energy));
  }

  {
    FakeBus bus;
    INA228::INA228 dev;
    Config cfg = makeConfig(bus);
    cfg.shuntResistanceOhm = 0.0162f;
    cfg.maxExpectedCurrentA = 10.0f;
    TEST_ASSERT_TRUE(dev.begin(cfg).ok());
    TEST_ASSERT_TRUE(dev.writeRegister16(cmd::REG_SHUNT_TEMPCO, 0x1234).ok());
    float power = 789.0f;
    Status st = dev.readPower(power);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::HARDWARE_DIRTY),
                            static_cast<uint8_t>(st.code));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 789.0f, power);
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
  RUN_TEST(test_get_settings_snapshot);
  RUN_TEST(test_get_settings_before_begin_is_cache_only_uninitialized_snapshot);
  RUN_TEST(test_driver_state_alias_matches_state);
  RUN_TEST(test_get_settings_is_bus_silent_and_does_not_consume_diag);
  RUN_TEST(test_begin_rejects_missing_callbacks);
  RUN_TEST(test_begin_success_sets_ready_without_health_counts);
  RUN_TEST(test_configured_i2c_address_reaches_transport_callbacks);
  RUN_TEST(test_begin_rejects_invalid_address);
  RUN_TEST(test_begin_rejects_zero_timeout);
  RUN_TEST(test_begin_rejects_invalid_adc_range);
  RUN_TEST(test_invalid_begin_after_success_resets_default_runtime);
  RUN_TEST(test_failed_begin_probe_resets_cached_config);
  RUN_TEST(test_begin_preserves_transport_errors_for_startup_reads);
  RUN_TEST(test_begin_rejects_identity_and_memstat_mismatch_without_writes);
  RUN_TEST(test_begin_apply_write_failures_reset_cache_without_health_counts);
  RUN_TEST(test_begin_normalizes_offline_threshold_on_stored_copy);
  RUN_TEST(test_begin_programs_tempco_even_when_tempcomp_disabled);
  RUN_TEST(test_begin_rejects_non_finite_calibration);
  RUN_TEST(test_end_returns_to_uninit);
  RUN_TEST(test_missing_now_ms_uses_zero_for_health_timestamps);
  RUN_TEST(test_begin_without_now_ms_keeps_zero_health_timestamp);
  RUN_TEST(test_probe_failure_does_not_update_health);
  RUN_TEST(test_probe_preserves_transport_errors_without_health_tracking);
  RUN_TEST(test_recover_failure_updates_health_once);
  RUN_TEST(test_recover_success_returns_ready);
  RUN_TEST(test_recover_preserves_transport_error_code);
  RUN_TEST(test_recover_preserves_transport_errors_for_identity_memstat_reads);
  RUN_TEST(test_recover_identity_mismatch_updates_health);
  RUN_TEST(test_recover_memstat_failure_updates_health);
  RUN_TEST(test_recover_reaches_offline_when_threshold_is_one);
  RUN_TEST(test_offline_read_bus_voltage_returns_busy_without_i2c);
  RUN_TEST(test_failed_recover_from_offline_keeps_latch_after_intermediate_success);
  RUN_TEST(test_offline_reset_accumulators_returns_busy_without_i2c);
  RUN_TEST(test_offline_poll_measurement_ready_returns_busy_without_i2c);
  RUN_TEST(test_soft_reset_verifies_identity_memstat_and_replays_cache);
  RUN_TEST(test_soft_reset_write_failure_marks_reset_domain_dirty);
  RUN_TEST(test_soft_reset_timeout_marks_dirty_without_replay);
  RUN_TEST(test_soft_reset_read_failures_mark_dirty_without_replay);
  RUN_TEST(test_soft_reset_identity_mismatch_marks_dirty_without_replay);
  RUN_TEST(test_soft_reset_memstat_failure_marks_dirty_without_replay);
  RUN_TEST(test_reset_accumulators_clears_rstacc_and_invalidates_until_cnvrf);
  RUN_TEST(test_reset_accumulators_write_failure_marks_dirty_and_invalidates);
  RUN_TEST(test_reset_accumulators_later_failures_mark_dirty_and_invalidate);
  RUN_TEST(test_soft_reset_replay_failures_mark_dirty_for_each_write_position);
  RUN_TEST(test_recover_replay_failure_preserves_dirty_until_full_recover);
  RUN_TEST(test_recover_replay_failures_mark_dirty_for_each_write_position);
  RUN_TEST(test_example_transport_maps_wire_errors_and_keeps_timeout_owned_by_init);
  RUN_TEST(test_example_transport_validates_params_and_handles_write_read);
  RUN_TEST(test_conversion_time_estimate);
  RUN_TEST(test_conversion_time_with_averaging);
  RUN_TEST(test_shutdown_conversion_time_ignores_configured_delay);
  RUN_TEST(test_conversion_time_defaults_and_maximum);
  RUN_TEST(test_conversion_time_table_vectors);
  RUN_TEST(test_config_register_encoding_vectors);
  RUN_TEST(test_begin_marks_each_triggered_mode_pending);
  RUN_TEST(test_begin_triggered_reads_do_not_return_stale_registers_before_cnvrf);
  RUN_TEST(test_conversion_ready_clears_completed_trigger_state);
  RUN_TEST(test_triggered_conversion_gates_reads_until_cnvrf);
  RUN_TEST(test_tick_timestamp_completes_trigger_without_now_hook);
  RUN_TEST(test_tick_deadline_is_wraparound_safe);
  RUN_TEST(test_public_diag_read_consuming_cnvrf_does_not_strand_pending_trigger);
  RUN_TEST(test_tick_preserves_diag_alert_evidence_when_polling_cnvrf);
  RUN_TEST(test_tick_does_not_overwrite_diag_alert_snapshot_when_no_state_pending);
  RUN_TEST(test_readiness_path_preserves_diag_alert_evidence_for_measurement_gate);
  RUN_TEST(test_alert_config_setters_do_not_read_live_diag_alrt);
  RUN_TEST(test_public_read_diag_alert_is_destructive_and_preserved);
  RUN_TEST(test_public_read_diag_alert_raw_is_destructive);
  RUN_TEST(test_read_bus_voltage_requires_init);
  RUN_TEST(test_read_bus_voltage_zero_on_default);
  RUN_TEST(test_uncalibrated_current_power_energy_charge_fail_without_i2c);
  RUN_TEST(test_read_measurement_all_zero_when_calibrated);
  RUN_TEST(test_continuous_mode_allows_energy_and_charge_after_cnvrf_when_calibrated);
  RUN_TEST(test_triggered_modes_reject_energy_charge_without_accumulator_i2c);
  RUN_TEST(test_shutdown_modes_reject_energy_charge_without_accumulator_i2c);
  RUN_TEST(test_read_measurement_marks_invalid_accumulation_without_accumulator_i2c);
  RUN_TEST(test_diag_alert_surfaces_and_preserves_accumulator_overflow_flags);
  RUN_TEST(test_read_energy_preserves_energy_overflow_and_reports_status);
  RUN_TEST(test_read_charge_preserves_charge_overflow_and_reports_status);
  RUN_TEST(test_math_overflow_blocks_accumulation_reads_and_is_preserved);
  RUN_TEST(test_math_overflow_blocks_current_power_and_measurement);
  RUN_TEST(test_reset_accumulators_invalidates_until_next_continuous_cnvrf);
  RUN_TEST(test_read_raw_sample_uses_unsigned_vbus_and_energy);
  RUN_TEST(test_read_integer_sample_uses_fixed_units_without_accumulators);
  RUN_TEST(test_convert_raw_sample_uses_fixed_units_without_i2c);
  RUN_TEST(test_integer_sample_requires_calibration_and_preserves_output);
  RUN_TEST(test_convert_raw_sample_reports_math_overflow_evidence);
  RUN_TEST(test_20bit_edge_vectors_and_low_nibble_masking);
  RUN_TEST(test_temperature_negative_and_positive_vectors);
  RUN_TEST(test_scalar_power_energy_charge_lsb_vectors);
  RUN_TEST(test_read_measurement_failures_leave_output_unchanged);
  RUN_TEST(test_read_raw_sample_failures_leave_output_unchanged);
  RUN_TEST(test_power_sample_raw_step_respects_budget_one);
  RUN_TEST(test_power_sample_raw_step_respects_budget_two);
  RUN_TEST(test_power_sample_raw_step_full_budget_outputs_integer_units);
  RUN_TEST(test_power_sample_step_failure_each_register_clears_job_preserves_outputs);
  RUN_TEST(test_poll_measurement_ready_delay_gate_and_diag_budget);
  RUN_TEST(test_zero_budget_fixed_step_calls_are_bus_silent);
  RUN_TEST(test_apply_calibration_job_marks_adc_dirty_when_later_write_fails);
  RUN_TEST(test_config_replay_job_aliases_share_apply_calibration_job);
  RUN_TEST(test_apply_replay_failure_each_step_marks_exact_dirty_register);
  RUN_TEST(test_reset_job_budget_one_delay_and_reidentification);
  RUN_TEST(test_reset_job_failure_each_step_reasserts_offline_when_started_offline);
  RUN_TEST(test_calibration_vectors_program_exact_shunt_cal_and_lsb);
  RUN_TEST(test_set_adc_range_recomputes_shunt_cal_with_multiplier);
  RUN_TEST(test_set_adc_range_config_write_failure_marks_config_dirty);
  RUN_TEST(test_prompt_positive_raw_vector_converts_all_outputs);
  RUN_TEST(test_prompt_positive_raw_vector_uses_range1_shunt_lsb);
  RUN_TEST(test_prompt_negative_raw_vectors_sign_extend);
  RUN_TEST(test_prompt_negative_scalar_conversions);
  RUN_TEST(test_invalid_begin_calibration_configs_do_not_touch_i2c);
  RUN_TEST(test_set_calibration_invalid_params_do_not_touch_i2c_or_cache);
  RUN_TEST(test_read_power_overflow_returns_math_overflow_without_inf_output);
  RUN_TEST(test_read_measurement_overflow_leaves_output_unchanged);
  RUN_TEST(test_begin_uncalibrated_writes_shunt_cal_zero);
  RUN_TEST(test_set_adc_range_failure_rolls_back_config_when_shunt_cal_write_fails);
  RUN_TEST(test_set_adc_range_rollback_failure_marks_dirty_and_recover_resyncs);
  RUN_TEST(test_scale_changes_mark_thresholds_dirty);
  RUN_TEST(test_calibration_clamp_updates_current_lsb_to_actual_register_value);
  RUN_TEST(test_calibration_does_not_commit_cache_on_write_failure);
  RUN_TEST(test_config_setter_does_not_commit_cache_on_write_failure_and_marks_dirty);
  RUN_TEST(test_cached_static_config_write_failures_mark_dirty_registers);
  RUN_TEST(test_raw_config_write_marks_dirty_until_recover_replays_cache);
  RUN_TEST(test_threshold_setters_encode_exact_register_vectors);
  RUN_TEST(test_threshold_write_failures_preserve_registers);
  RUN_TEST(test_invalid_threshold_values_do_not_touch_bus);
  RUN_TEST(test_read_manufacturer_id);
  RUN_TEST(test_read_device_id);
  RUN_TEST(test_public_register_access_helpers);
  RUN_TEST(test_raw_accumulator_register_read_does_not_pre_preserve_diag_alert);
  RUN_TEST(test_public_register_access_preserves_transport_errors);
  RUN_TEST(test_status_contracts_for_calibration_accumulation_and_dirty_state);
  RUN_TEST(test_register_access_after_end_does_not_touch_bus);
  return UNITY_END();
}
