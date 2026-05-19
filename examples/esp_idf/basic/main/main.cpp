#include <cstdint>

#include "INA228/INA228.h"
#include "Ina228IdfI2cTransport.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr char TAG[] = "ina228_basic";
constexpr gpio_num_t I2C_SDA = GPIO_NUM_8;
constexpr gpio_num_t I2C_SCL = GPIO_NUM_9;
constexpr uint32_t I2C_FREQ_HZ = 400000;
constexpr uint8_t INA228_ADDRESS = 0x40;
constexpr float SHUNT_OHM = 0.015f;
constexpr float MAX_CURRENT_A = 10.0f;

}  // namespace

extern "C" void app_main(void) {
  Ina228IdfI2c transport{};
  transport.address = INA228_ADDRESS;

  i2c_master_bus_config_t busConfig{};
  busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
  busConfig.i2c_port = I2C_NUM_0;
  busConfig.sda_io_num = I2C_SDA;
  busConfig.scl_io_num = I2C_SCL;
  busConfig.glitch_ignore_cnt = 7;
  busConfig.flags.enable_internal_pullup = true;

  ESP_ERROR_CHECK(i2c_new_master_bus(&busConfig, &transport.bus));

  i2c_device_config_t devConfig{};
  devConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  devConfig.device_address = INA228_ADDRESS;
  devConfig.scl_speed_hz = I2C_FREQ_HZ;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(transport.bus, &devConfig, &transport.dev));

  INA228::INA228 device;
  INA228::Config cfg{};
  cfg.i2cWrite = ina228IdfI2cWrite;
  cfg.i2cWriteRead = ina228IdfI2cWriteRead;
  cfg.i2cUser = &transport;
  cfg.nowMs = ina228IdfNowMs;
  cfg.i2cAddress = INA228_ADDRESS;
  cfg.i2cTimeoutMs = 50;
  cfg.mode = INA228::Mode::CONT_ALL;
  cfg.shuntResistanceOhm = SHUNT_OHM;
  cfg.maxExpectedCurrentA = MAX_CURRENT_A;

  INA228::Status st = device.begin(cfg);
  if (!st.ok()) {
    ESP_LOGE(TAG, "begin failed: %s (%d detail=%ld)", st.msg, static_cast<int>(st.code),
             static_cast<long>(st.detail));
    return;
  }

  uint16_t manufacturer = 0;
  uint16_t deviceId = 0;
  uint16_t diagAlert = 0;
  (void)device.readManufacturerId(manufacturer);
  (void)device.readDeviceId(deviceId);
  (void)device.readDiagAlertRaw(diagAlert);
  ESP_LOGI(TAG, "manufacturer=0x%04X device=0x%04X diag=0x%04X",
           manufacturer, deviceId, diagAlert);

  vTaskDelay(pdMS_TO_TICKS(device.estimateConversionTimeMs() + 1U));

  INA228::Measurement measurement{};
  st = device.readMeasurement(measurement);
  if (st.ok()) {
    ESP_LOGI(TAG, "bus=%.3f V shunt=%.6f V current=%.6f A power=%.6f W",
             static_cast<double>(measurement.busVoltageV),
             static_cast<double>(measurement.shuntVoltageV),
             static_cast<double>(measurement.currentA),
             static_cast<double>(measurement.powerW));
  } else {
    ESP_LOGW(TAG, "measurement unavailable: %s (%d detail=%ld)", st.msg,
             static_cast<int>(st.code), static_cast<long>(st.detail));
  }

  ESP_LOGI(TAG, "state=%u successes=%lu failures=%lu", static_cast<unsigned>(device.state()),
           static_cast<unsigned long>(device.totalSuccess()),
           static_cast<unsigned long>(device.totalFailures()));
}
