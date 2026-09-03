#pragma once

#include <cstddef>
#include <cstdint>

#include "INA228/INA228.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

// Diagnostic single-owner ESP-IDF example glue. Production shared-bus or
// multitask applications should wrap the bus/device handles in their own bus
// manager and lock before calling into one or more INA228 instances.
struct Ina228IdfI2c {
  i2c_master_bus_handle_t bus = nullptr;
  i2c_master_dev_handle_t dev = nullptr;
  // Retained only after a bounded temporary-device cleanup failure.
  i2c_master_dev_handle_t temporaryDevPendingRemoval = nullptr;
  uint8_t address = 0x40;
  uint32_t frequencyHz = 400000;
  esp_err_t lastError = ESP_OK;
};

struct Ina228IdfTransferStats {
  uint32_t read = 0;
  uint32_t write = 0;
};

Ina228IdfI2c& ina228IdfTransportContext();
void ina228IdfResetTransferStats();
Ina228IdfTransferStats ina228IdfTransferStats();
bool ina228IdfInitI2c(int sda, int scl, uint32_t freqHz, uint16_t timeoutMs,
                      uint8_t address);
INA228::Status ina228IdfDeinitI2c();
INA228::Status ina228IdfSelectDeviceAddress(uint8_t address);
INA228::Status ina228IdfProbeAddress(uint8_t address, uint16_t timeoutMs);
esp_err_t ina228IdfLastError();

INA228::Status ina228IdfI2cWrite(uint8_t addr, const uint8_t* data, size_t len,
                                 uint32_t timeoutMs, void* user);
INA228::Status ina228IdfI2cWriteRead(uint8_t addr, const uint8_t* txData, size_t txLen,
                                     uint8_t* rxData, size_t rxLen,
                                     uint32_t timeoutMs, void* user);
INA228::Status ina228IdfI2cWriteReadAt(uint8_t addr, const uint8_t* txData,
                                       size_t txLen, uint8_t* rxData, size_t rxLen,
                                       uint32_t timeoutMs, void* user);
uint32_t ina228IdfNowMs(void* user);
