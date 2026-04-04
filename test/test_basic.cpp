/// @file test_basic.cpp
/// @brief Native contract tests for INA228 lifecycle and health behavior.

#include <unity.h>

#include "Arduino.h"
#include "Wire.h"

SerialClass Serial;
TwoWire Wire;

#include "INA228/INA228.h"
#include "common/I2cTransport.h"

using namespace INA228;

namespace {

struct FakeBus {
  uint32_t nowMs = 1000;
  uint32_t writeCalls = 0;
  uint32_t readCalls = 0;

  int readErrorRemaining = 0;
  int writeErrorRemaining = 0;
  Status readError = Status::Error(Err::I2C_ERROR, "forced read error", -1);
  Status writeError = Status::Error(Err::I2C_ERROR, "forced write error", -2);
};

Status fakeWrite(uint8_t, const uint8_t* data, size_t len, uint32_t, void* user) {
  FakeBus* bus = static_cast<FakeBus*>(user);
  bus->writeCalls++;
  if (data == nullptr || len == 0) {
    return Status::Error(Err::INVALID_PARAM, "invalid fake write args");
  }
  if (bus->writeErrorRemaining > 0) {
    bus->writeErrorRemaining--;
    return bus->writeError;
  }
  return Status::Ok();
}

Status fakeWriteRead(uint8_t, const uint8_t* txData, size_t txLen, uint8_t* rxData,
                     size_t rxLen, uint32_t, void* user) {
  FakeBus* bus = static_cast<FakeBus*>(user);
  bus->readCalls++;
  if (txData == nullptr || txLen == 0 || (rxLen > 0 && rxData == nullptr)) {
    return Status::Error(Err::INVALID_PARAM, "invalid fake write-read args");
  }
  if (bus->readErrorRemaining > 0) {
    bus->readErrorRemaining--;
    return bus->readError;
  }

  const uint8_t reg = txData[0];
  for (size_t i = 0; i < rxLen; ++i) {
    rxData[i] = 0;
  }

  // Manufacturer ID: 0x5449
  if (reg == cmd::REG_MANUFACTURER_ID && rxLen >= 2) {
    rxData[0] = 0x54;
    rxData[1] = 0x49;
  }
  // Device ID: 0x2281
  else if (reg == cmd::REG_DEVICE_ID && rxLen >= 2) {
    rxData[0] = 0x22;
    rxData[1] = 0x81;
  }
  // DIAG_ALRT: MEMSTAT=1 (bit 0)
  else if (reg == cmd::REG_DIAG_ALRT && rxLen >= 2) {
    rxData[0] = 0x00;
    rxData[1] = 0x01;
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

}  // namespace

void setUp() {
  setMillis(0);
  Wire._clearEndTransmissionResult();
  Wire._clearRequestFromOverride();
}

void tearDown() {}

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

void test_begin_success_sets_ready_and_health() {
  FakeBus bus;
  INA228::INA228 dev;
  Status st = dev.begin(makeConfig(bus));
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::READY),
                          static_cast<uint8_t>(dev.state()));
  TEST_ASSERT_TRUE(dev.isOnline());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.totalSuccess());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.totalFailures());
  TEST_ASSERT_EQUAL_UINT8(0u, dev.consecutiveFailures());
  TEST_ASSERT_EQUAL_UINT32(0u, dev.lastOkMs());
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

void test_end_returns_to_uninit() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());
  dev.end();
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(DriverState::UNINIT),
                          static_cast<uint8_t>(dev.state()));
}

// ===========================================================================
// NowMs fallback
// ===========================================================================

void test_now_ms_fallback_uses_millis_when_callback_missing() {
  FakeBus bus;
  INA228::INA228 dev;
  Config cfg = makeConfig(bus);
  cfg.nowMs = nullptr;
  cfg.timeUser = nullptr;
  TEST_ASSERT_TRUE(dev.begin(cfg).ok());

  setMillis(4321);
  Status st = dev.recover();
  TEST_ASSERT_TRUE(st.ok());
  TEST_ASSERT_EQUAL_UINT32(4321u, dev.lastOkMs());
}

void test_begin_without_now_ms_uses_millis_fallback() {
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
  TEST_ASSERT_EQUAL_UINT32(4242u, dev.lastOkMs());
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
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Err::DEVICE_NOT_FOUND),
                          static_cast<uint8_t>(st.code));
  TEST_ASSERT_EQUAL_UINT32(beforeSuccess, dev.totalSuccess());
  TEST_ASSERT_EQUAL_UINT32(beforeFailures, dev.totalFailures());
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(beforeState),
                          static_cast<uint8_t>(dev.state()));
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

void test_read_measurement_all_zero() {
  FakeBus bus;
  INA228::INA228 dev;
  TEST_ASSERT_TRUE(dev.begin(makeConfig(bus)).ok());

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

// ===========================================================================
// Entry point
// ===========================================================================

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_status_ok);
  RUN_TEST(test_status_error);
  RUN_TEST(test_status_in_progress);
  RUN_TEST(test_config_defaults);
  RUN_TEST(test_begin_rejects_missing_callbacks);
  RUN_TEST(test_begin_success_sets_ready_and_health);
  RUN_TEST(test_begin_rejects_invalid_address);
  RUN_TEST(test_begin_rejects_zero_timeout);
  RUN_TEST(test_end_returns_to_uninit);
  RUN_TEST(test_now_ms_fallback_uses_millis_when_callback_missing);
  RUN_TEST(test_begin_without_now_ms_uses_millis_fallback);
  RUN_TEST(test_probe_failure_does_not_update_health);
  RUN_TEST(test_recover_failure_updates_health_once);
  RUN_TEST(test_recover_success_returns_ready);
  RUN_TEST(test_recover_preserves_transport_error_code);
  RUN_TEST(test_recover_reaches_offline_when_threshold_is_one);
  RUN_TEST(test_example_transport_maps_wire_errors_and_keeps_timeout_owned_by_init);
  RUN_TEST(test_example_transport_validates_params_and_handles_write_read);
  RUN_TEST(test_conversion_time_estimate);
  RUN_TEST(test_conversion_time_with_averaging);
  RUN_TEST(test_read_bus_voltage_requires_init);
  RUN_TEST(test_read_bus_voltage_zero_on_default);
  RUN_TEST(test_read_measurement_all_zero);
  RUN_TEST(test_read_manufacturer_id);
  RUN_TEST(test_read_device_id);
  return UNITY_END();
}
