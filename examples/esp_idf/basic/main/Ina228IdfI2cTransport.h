#pragma once

#include <cstddef>
#include <cstdint>

#include "INA228/INA228.h"
#include "driver/i2c_master.h"

struct Ina228IdfI2c {
  i2c_master_bus_handle_t bus = nullptr;
  i2c_master_dev_handle_t dev = nullptr;
  uint8_t address = 0x40;
};

INA228::Status ina228IdfI2cWrite(uint8_t addr, const uint8_t* data, size_t len,
                                 uint32_t timeoutMs, void* user);
INA228::Status ina228IdfI2cWriteRead(uint8_t addr, const uint8_t* txData, size_t txLen,
                                     uint8_t* rxData, size_t rxLen,
                                     uint32_t timeoutMs, void* user);
uint32_t ina228IdfNowMs(void* user);
