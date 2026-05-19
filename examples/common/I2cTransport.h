/**
 * @file I2cTransport.h
 * @brief Wire-based I2C transport adapter for INA228 examples.
 *
 * This file provides Wire-compatible I2C callbacks that can be
 * used with the INA228 driver. The library does not depend on Wire
 * directly; this adapter bridges them.
 *
 * NOT part of the library API. Example-only.
 */

#pragma once

#if defined(INA228_EXAMPLE_PLATFORM_IDF)
#include "Ina228IdfI2cTransport.h"
#else
#include <Arduino.h>
#include <Wire.h>
#endif

#include "INA228/Status.h"

namespace transport {

#if defined(INA228_EXAMPLE_PLATFORM_IDF)

inline INA228::Status wireWrite(uint8_t addr, const uint8_t* data, size_t len,
                                uint32_t timeoutMs, void* user) {
  return ina228IdfI2cWrite(addr, data, len, timeoutMs, user);
}

inline INA228::Status wireWriteRead(uint8_t addr, const uint8_t* tx, size_t txLen,
                                    uint8_t* rx, size_t rxLen, uint32_t timeoutMs,
                                    void* user) {
  return ina228IdfI2cWriteRead(addr, tx, txLen, rx, rxLen, timeoutMs, user);
}

inline INA228::Status wireWriteReadAt(uint8_t addr, const uint8_t* tx, size_t txLen,
                                      uint8_t* rx, size_t rxLen, uint32_t timeoutMs) {
  return ina228IdfI2cWriteReadAt(addr, tx, txLen, rx, rxLen, timeoutMs,
                                 &ina228IdfTransportContext());
}

inline INA228::Status probeAddress(uint8_t addr, uint16_t timeoutMs) {
  return ina228IdfProbeAddress(addr, timeoutMs);
}

inline uint32_t arduinoNowMs(void* user) {
  return ina228IdfNowMs(user);
}

inline bool initWire(int sda, int scl, uint32_t freq = 400000, uint16_t timeoutMs = 50,
                     uint8_t address = 0x40) {
  return ina228IdfInitI2c(sda, scl, freq, timeoutMs, address);
}

inline bool selectAddress(uint8_t address) {
  return ina228IdfSelectDeviceAddress(address);
}

inline void* configUser() {
  return &ina228IdfTransportContext();
}

#else

inline INA228::Status mapWireResult(uint8_t result, const char* context) {
  switch (result) {
    case 0:
      return INA228::Status::Ok();
    case 1:
      return INA228::Status::Error(INA228::Err::INVALID_PARAM, context, result);
    case 2:
      return INA228::Status::Error(INA228::Err::I2C_NACK_ADDR, context, result);
    case 3:
      return INA228::Status::Error(INA228::Err::I2C_NACK_DATA, context, result);
    case 4:
      return INA228::Status::Error(INA228::Err::I2C_BUS, context, result);
    case 5:
      return INA228::Status::Error(INA228::Err::I2C_TIMEOUT, context, result);
    default:
      return INA228::Status::Error(INA228::Err::I2C_ERROR, context, result);
  }
}

/**
 * @brief Wire-based I2C write implementation.
 *
 * Pass to Config::i2cWrite, and pass &Wire (or a custom TwoWire*) to i2cUser.
 * The timeout parameter is advisory; bus timeout ownership stays with initWire().
 *
 * @param addr I2C 7-bit address
 * @param data Data buffer to send
 * @param len Number of bytes
 * @param timeoutMs Timeout requested by the driver (advisory only)
 * @param user Pointer to TwoWire instance
 * @return Status OK on success, I2C error on failure
 */
inline INA228::Status wireWrite(uint8_t addr, const uint8_t* data, size_t len,
                                uint32_t timeoutMs, void* user) {
  (void)timeoutMs;

  TwoWire* wire = static_cast<TwoWire*>(user);
  if (wire == nullptr) {
    return INA228::Status::Error(INA228::Err::INVALID_CONFIG, "Wire instance is null");
  }
  if (!data || len == 0) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "Invalid I2C write params");
  }

  // Check for oversized writes (ESP32 Wire buffer is 128 bytes)
  if (len > 128) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "Write exceeds I2C buffer",
                                 static_cast<int32_t>(len));
  }

  wire->beginTransmission(addr);
  size_t written = wire->write(data, len);
  if (written != len) {
    return INA228::Status::Error(INA228::Err::I2C_ERROR, "I2C write incomplete",
                                  static_cast<int32_t>(written));
  }

  uint8_t result = wire->endTransmission(true);
  return mapWireResult(result, "I2C write failed");
}

/**
 * @brief Wire-based I2C write-read implementation.
 *
 * Pass to Config::i2cWriteRead, and pass &Wire (or a custom TwoWire*) to i2cUser.
 * The timeout parameter is advisory; bus timeout ownership stays with initWire().
 *
 * @param addr I2C 7-bit address
 * @param tx TX buffer to send
 * @param txLen TX length
 * @param rx RX buffer for readback
 * @param rxLen RX length
 * @param timeoutMs Timeout requested by the driver (advisory only)
 * @param user Pointer to TwoWire instance
 * @return Status OK on success, I2C error on failure
 */
inline INA228::Status wireWriteRead(uint8_t addr, const uint8_t* tx, size_t txLen,
                                    uint8_t* rx, size_t rxLen, uint32_t timeoutMs,
                                    void* user) {
  (void)timeoutMs;

  TwoWire* wire = static_cast<TwoWire*>(user);
  if (wire == nullptr) {
    return INA228::Status::Error(INA228::Err::INVALID_CONFIG, "Wire instance is null");
  }
  if ((txLen > 0 && tx == nullptr) || (rxLen > 0 && rx == nullptr)) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "Invalid I2C read params");
  }
  if (txLen == 0 || rxLen == 0) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "I2C read length invalid");
  }
  // Check for oversized transfers (ESP32 Wire buffer is 128 bytes)
  if (txLen > 128 || rxLen > 128) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "I2C read exceeds buffer");
  }

  wire->beginTransmission(addr);
  size_t written = wire->write(tx, txLen);
  if (written != txLen) {
    return INA228::Status::Error(INA228::Err::I2C_ERROR, "I2C write incomplete",
                                 static_cast<int32_t>(written));
  }

  uint8_t result = wire->endTransmission(false);
  if (result != 0) {
    return mapWireResult(result, "I2C write phase failed");
  }

  size_t read = wire->requestFrom(addr, static_cast<uint8_t>(rxLen));
  if (read != rxLen) {
    return INA228::Status::Error(INA228::Err::I2C_ERROR, "I2C read length mismatch",
                                  static_cast<int32_t>(read));
  }

  for (size_t i = 0; i < rxLen; ++i) {
    if (wire->available()) {
      rx[i] = static_cast<uint8_t>(wire->read());
    } else {
      return INA228::Status::Error(INA228::Err::I2C_ERROR, "I2C data not available");
    }
  }

  return INA228::Status::Ok();
}

/**
 * @brief Initialize Wire with default pins and frequency.
 *
 * @param sda SDA pin number
 * @param scl SCL pin number
 * @param freq I2C clock frequency in Hz (default 400kHz)
 * @param timeoutMs I2C timeout in milliseconds (default 50ms)
 * @return true on success
 */
inline bool initWire(int sda, int scl, uint32_t freq = 400000, uint16_t timeoutMs = 50,
                     uint8_t address = 0x40) {
  (void)address;
#if defined(ARDUINO_ARCH_ESP32)
  // Toggle SCL to release any stuck slave
  pinMode(scl, OUTPUT);
  pinMode(sda, INPUT_PULLUP);
  for (int i = 0; i < 9; i++) {
    digitalWrite(scl, LOW);
    delayMicroseconds(5);
    digitalWrite(scl, HIGH);
    delayMicroseconds(5);
  }
  // Generate STOP condition
  pinMode(sda, OUTPUT);
  digitalWrite(sda, LOW);
  delayMicroseconds(5);
  digitalWrite(scl, HIGH);
  delayMicroseconds(5);
  digitalWrite(sda, HIGH);
  delayMicroseconds(5);
#endif

  Wire.begin(sda, scl);
  Wire.setClock(freq);
  Wire.setTimeOut(timeoutMs);
  return true;
}

inline INA228::Status wireWriteReadAt(uint8_t addr, const uint8_t* tx, size_t txLen,
                                      uint8_t* rx, size_t rxLen, uint32_t timeoutMs) {
  return wireWriteRead(addr, tx, txLen, rx, rxLen, timeoutMs, &Wire);
}

inline INA228::Status probeAddress(uint8_t addr, uint16_t timeoutMs) {
#if defined(ARDUINO_ARCH_ESP32)
  Wire.setTimeOut(timeoutMs);
#else
  (void)timeoutMs;
#endif
  Wire.beginTransmission(addr);
  return mapWireResult(Wire.endTransmission(true), "I2C address probe failed");
}

inline uint32_t arduinoNowMs(void*) {
  return millis();
}

inline bool selectAddress(uint8_t) {
  return true;
}

inline void* configUser() {
  return &Wire;
}

#endif

}  // namespace transport
