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

#include <Arduino.h>
#include <Wire.h>

#include "INA228/Status.h"

namespace transport {

/// Apply the supplied callback timeout to the selected ESP32 Wire instance.
inline void applyWireTimeout(TwoWire* wire, uint32_t timeoutMs) {
#if defined(ARDUINO_ARCH_ESP32)
  const uint32_t clamped = timeoutMs > 0xFFFFU ? 0xFFFFU : timeoutMs;
  wire->setTimeOut(static_cast<uint16_t>(clamped));
#else
  (void)wire;
  (void)timeoutMs;
#endif
}

/// Largest single-transfer payload the underlying Wire buffer can hold.
#if defined(I2C_BUFFER_LENGTH)
static constexpr size_t WIRE_BUFFER_LIMIT = I2C_BUFFER_LENGTH;
#elif defined(BUFFER_LENGTH)
static constexpr size_t WIRE_BUFFER_LIMIT = BUFFER_LENGTH;
#else
static constexpr size_t WIRE_BUFFER_LIMIT = 32;
#endif

struct TransferStats {
  uint32_t read = 0;
  uint32_t write = 0;
};

inline TransferStats& transferStatsStorage() {
  static TransferStats stats{};
  return stats;
}

inline void resetTransferStats() {
  transferStatsStorage() = {};
}

inline TransferStats transferStats() {
  return transferStatsStorage();
}

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
 * The supplied timeout is applied to this callback on ESP32. Other platforms
 * retain their externally configured bus timeout.
 *
 * @param addr I2C 7-bit address
 * @param data Data buffer to send
 * @param len Number of bytes
 * @param timeoutMs Timeout requested by the driver
 * @param user Pointer to TwoWire instance
 * @return Status OK on success, I2C error on failure
 */
inline INA228::Status wireWrite(uint8_t addr, const uint8_t* data, size_t len,
                                uint32_t timeoutMs, void* user) {
  TwoWire* wire = static_cast<TwoWire*>(user);
  if (wire == nullptr) {
    return INA228::Status::Error(INA228::Err::INVALID_CONFIG, "Wire instance is null");
  }
  if (!data || len == 0) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "Invalid I2C write params");
  }

  if (len > WIRE_BUFFER_LIMIT) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "Write exceeds I2C buffer",
                                 static_cast<int32_t>(len));
  }

  transferStatsStorage().write++;
  applyWireTimeout(wire, timeoutMs);
  wire->beginTransmission(addr);
  size_t written = wire->write(data, len);
  if (written != len) {
    // Start a fresh empty transaction to discard the partial TX buffer before
    // closing it; do not deliberately put a truncated register write on-bus.
    transferStatsStorage().write++;
    wire->beginTransmission(addr);
    (void)wire->endTransmission(true);
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
 * The supplied timeout is applied to this callback on ESP32. Other platforms
 * retain their externally configured bus timeout.
 *
 * @param addr I2C 7-bit address
 * @param tx TX buffer to send
 * @param txLen TX length
 * @param rx RX buffer for readback
 * @param rxLen RX length
 * @param timeoutMs Timeout requested by the driver
 * @param user Pointer to TwoWire instance
 * @return Status OK on success, I2C error on failure
 */
inline INA228::Status wireWriteRead(uint8_t addr, const uint8_t* tx, size_t txLen,
                                    uint8_t* rx, size_t rxLen, uint32_t timeoutMs,
                                    void* user) {
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
  if (txLen > WIRE_BUFFER_LIMIT || rxLen > WIRE_BUFFER_LIMIT) {
    return INA228::Status::Error(INA228::Err::INVALID_PARAM, "I2C read exceeds buffer");
  }

  transferStatsStorage().read++;
  applyWireTimeout(wire, timeoutMs);
  wire->beginTransmission(addr);
  size_t written = wire->write(tx, txLen);
  if (written != txLen) {
    transferStatsStorage().write++;
    wire->beginTransmission(addr);
    (void)wire->endTransmission(true);
    return INA228::Status::Error(INA228::Err::I2C_ERROR, "I2C write incomplete",
                                 static_cast<int32_t>(written));
  }

  uint8_t result = wire->endTransmission(false);
  if (result != 0) {
    return mapWireResult(result, "I2C write phase failed");
  }

  size_t read = wire->requestFrom(addr, static_cast<uint8_t>(rxLen));
  if (read != rxLen) {
    // On arduino-esp32 endTransmission(false) only latches the repeated-start
    // flag; the whole write+read is issued by requestFrom(), so the phase
    // information is lost here. Re-probe the address so a removed or NACKing
    // device is still reported precisely instead of as a generic I2C error.
    transferStatsStorage().write++;
    wire->beginTransmission(addr);
    const uint8_t probe = wire->endTransmission(true);
    if (probe != 0) {
      return mapWireResult(probe, "I2C read failed");
    }
    return INA228::Status::Error(INA228::Err::I2C_ERROR,
                                 "I2C requestFrom failed; cause unavailable",
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
inline bool initWire(int sda, int scl, uint32_t freq = 400000, uint16_t timeoutMs = 50) {
#if defined(ARDUINO_ARCH_ESP32)
  // Application-owned bus clear (UM10204 3.1.16). Release, never drive HIGH
  // against a target holding SDA or stretching SCL. One deadline bounds all
  // clock waits; an uncleared bus must not be reported as initialized.
  const uint32_t startedUs = micros();
  const uint32_t timeoutUs = static_cast<uint32_t>(timeoutMs) * 1000U;
  pinMode(scl, OUTPUT_OPEN_DRAIN);
  pinMode(sda, OUTPUT_OPEN_DRAIN);
  digitalWrite(sda, HIGH);
  const auto releaseClock = [&]() {
    digitalWrite(scl, HIGH);
    while (digitalRead(scl) == LOW) {
      if (static_cast<uint32_t>(micros() - startedUs) >= timeoutUs) {
        digitalWrite(sda, HIGH);
        return false;
      }
      delay(1);
    }
    if (static_cast<uint32_t>(micros() - startedUs) >= timeoutUs) {
      digitalWrite(sda, HIGH);
      return false;
    }
    return true;
  };
  if (!releaseClock()) return false;
  for (int i = 0; i < 9; i++) {
    digitalWrite(scl, LOW);
    delayMicroseconds(5);
    if (!releaseClock()) return false;
    delayMicroseconds(5);
  }
  digitalWrite(scl, LOW);
  digitalWrite(sda, LOW);
  delayMicroseconds(5);
  if (!releaseClock()) return false;
  delayMicroseconds(5);
  digitalWrite(sda, HIGH);
  delayMicroseconds(5);
  if (digitalRead(sda) == LOW || digitalRead(scl) == LOW) return false;

#endif

#if defined(ARDUINO_ARCH_ESP32)
  // Set the desired clock during initialization. Arduino-ESP32 3.3.11's
  // separate setClock() reports failure before any device handle exists.
  if (!Wire.begin(sda, scl, freq)) {
    return false;
  }
#else
  if (!Wire.begin(sda, scl)) {
    return false;
  }
  if (!Wire.setClock(freq)) {
    return false;
  }
#endif
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

inline void* configUser() {
  return &Wire;
}

}  // namespace transport
