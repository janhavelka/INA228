#include "Ina228IdfI2cTransport.h"

#include <limits>

#include "esp_err.h"
#include "esp_timer.h"

namespace {

int clampTimeoutMs(uint32_t timeoutMs) {
  const uint32_t maxTimeout = static_cast<uint32_t>(std::numeric_limits<int>::max());
  return static_cast<int>(timeoutMs > maxTimeout ? maxTimeout : timeoutMs);
}

INA228::Status mapEspErr(esp_err_t err, const char* context) {
  if (err == ESP_OK) {
    return INA228::Status::Ok();
  }
  if (err == ESP_ERR_TIMEOUT) {
    return INA228::Status::Error(INA228::Err::I2C_TIMEOUT, "I2C timeout",
                                 static_cast<int32_t>(err));
  }
  if (err == ESP_ERR_INVALID_RESPONSE) {
    return INA228::Status::Error(INA228::Err::I2C_ERROR, "I2C invalid response/NACK",
                                 static_cast<int32_t>(err));
  }
  return INA228::Status::Error(INA228::Err::I2C_BUS, context, static_cast<int32_t>(err));
}

INA228::Status validateContext(uint8_t addr, const void* user, const Ina228IdfI2c*& ctx) {
  ctx = static_cast<const Ina228IdfI2c*>(user);
  if (ctx == nullptr || ctx->dev == nullptr) {
    return INA228::Status::Error(INA228::Err::I2C_BUS, "IDF I2C device not configured");
  }
  if (addr != ctx->address) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "Unexpected I2C address",
                                 static_cast<int32_t>(addr));
  }
  return INA228::Status::Ok();
}

}  // namespace

INA228::Status ina228IdfI2cWrite(uint8_t addr, const uint8_t* data, size_t len,
                                 uint32_t timeoutMs, void* user) {
  const Ina228IdfI2c* ctx = nullptr;
  INA228::Status st = validateContext(addr, user, ctx);
  if (!st.ok()) {
    return st;
  }
  if (data == nullptr || len == 0) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "Invalid I2C write buffer");
  }

  const esp_err_t err =
      i2c_master_transmit(ctx->dev, data, len, clampTimeoutMs(timeoutMs));
  return mapEspErr(err, "I2C write failed");
}

INA228::Status ina228IdfI2cWriteRead(uint8_t addr, const uint8_t* txData, size_t txLen,
                                     uint8_t* rxData, size_t rxLen,
                                     uint32_t timeoutMs, void* user) {
  const Ina228IdfI2c* ctx = nullptr;
  INA228::Status st = validateContext(addr, user, ctx);
  if (!st.ok()) {
    return st;
  }
  if (txData == nullptr || txLen == 0 || (rxLen > 0 && rxData == nullptr)) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "Invalid I2C read buffer");
  }

  const esp_err_t err = i2c_master_transmit_receive(
      ctx->dev, txData, txLen, rxData, rxLen, clampTimeoutMs(timeoutMs));
  return mapEspErr(err, "I2C write-read failed");
}

uint32_t ina228IdfNowMs(void*) {
  return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}
