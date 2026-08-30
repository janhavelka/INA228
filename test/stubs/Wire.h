/// @file Wire.h
/// @brief Minimal Wire stub for native testing
#pragma once

#include <cstdint>
#include <cstddef>

class TwoWire {
public:
  // arduino-esp32's TwoWire::begin(sda, scl) returns bool; match it so the
  // example transport's error handling compiles the same way under test.
  bool begin(int sda = -1, int scl = -1) { (void)sda; (void)scl; return _beginResult; }
  void setClock(uint32_t freq) { (void)freq; }
  void setTimeOut(uint32_t timeoutMs) { _timeoutMs = timeoutMs; }
  uint32_t getTimeOut() const { return _timeoutMs; }
  
  void beginTransmission(uint8_t addr) { _addr = addr; _txLen = 0; ++_beginTransmissionCalls; }
  size_t write(uint8_t data) { _txBuf[_txLen++] = data; return 1; }
  size_t write(const uint8_t* data, size_t len) {
    size_t accepted = _writeOverrideEnabled ? _writeOverride : len;
    if (accepted > len) accepted = len;
    size_t written = 0;
    for (size_t i = 0; i < accepted && _txLen < sizeof(_txBuf); i++) {
      _txBuf[_txLen++] = data[i];
      ++written;
    }
    return written;
  }
  uint8_t endTransmission(bool stop = true) {
    (void)stop;
    _lastEndTransmissionTxLen = _txLen;
    ++_endTransmissionCalls;
    if (_queuedEndTransmissionIndex < _queuedEndTransmissionCount) {
      return _queuedEndTransmissionResults[_queuedEndTransmissionIndex++];
    }
    return _endTransmissionResult;
  }
  
  size_t requestFrom(uint8_t addr, size_t len) { 
    (void)addr;
    if (_requestFromOverrideEnabled) {
      _rxLen = _requestFromOverride;
    } else {
      _rxLen = len;
    }
    _rxIdx = 0;
    return _rxLen;
  }
  
  int available() { return _rxLen - _rxIdx; }
  int read() { 
    if (_rxIdx < _rxLen) {
      return _rxBuf[_rxIdx++];
    }
    return -1;
  }

  // Test helper: set data to return on next read
  void _setReadData(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len && i < sizeof(_rxBuf); i++) {
      _rxBuf[i] = data[i];
    }
  }

  void _setEndTransmissionResult(uint8_t result) { _endTransmissionResult = result; }
  void _queueEndTransmissionResult(uint8_t result) {
    if (_queuedEndTransmissionCount < sizeof(_queuedEndTransmissionResults)) {
      _queuedEndTransmissionResults[_queuedEndTransmissionCount++] = result;
    }
  }
  void _clearEndTransmissionResult() {
    _endTransmissionResult = 0;
    _queuedEndTransmissionCount = 0;
    _queuedEndTransmissionIndex = 0;
  }
  void _setBeginResult(bool result) { _beginResult = result; }
  void _setWriteResult(size_t result) {
    _writeOverrideEnabled = true;
    _writeOverride = result;
  }
  void _clearWriteOverride() { _writeOverrideEnabled = false; }
  void _resetTransmissionObservations() {
    _beginTransmissionCalls = 0;
    _endTransmissionCalls = 0;
    _lastEndTransmissionTxLen = 0;
  }
  size_t _beginTransmissionCallCount() const { return _beginTransmissionCalls; }
  size_t _endTransmissionCallCount() const { return _endTransmissionCalls; }
  size_t _lastEndTransmissionLength() const { return _lastEndTransmissionTxLen; }
  void _setRequestFromResult(size_t len) {
    _requestFromOverrideEnabled = true;
    _requestFromOverride = len;
  }
  void _clearRequestFromOverride() { _requestFromOverrideEnabled = false; }

private:
  uint8_t _addr = 0;
  uint8_t _txBuf[64] = {};
  size_t _txLen = 0;
  uint8_t _rxBuf[64] = {};
  size_t _rxLen = 0;
  size_t _rxIdx = 0;
  uint32_t _timeoutMs = 0;
  uint8_t _endTransmissionResult = 0;
  uint8_t _queuedEndTransmissionResults[8] = {};
  size_t _queuedEndTransmissionCount = 0;
  size_t _queuedEndTransmissionIndex = 0;
  bool _requestFromOverrideEnabled = false;
  size_t _requestFromOverride = 0;
  bool _beginResult = true;
  bool _writeOverrideEnabled = false;
  size_t _writeOverride = 0;
  size_t _beginTransmissionCalls = 0;
  size_t _endTransmissionCalls = 0;
  size_t _lastEndTransmissionTxLen = 0;
};

extern TwoWire Wire;
