/// @file Arduino.h
/// @brief Minimal Arduino stub for native testing
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <cstdlib>

// Basic types
using byte = uint8_t;

// Timing stubs
inline uint32_t gMillisValue = 0;
inline void setMillis(uint32_t value) { gMillisValue = value; }
inline uint32_t millis() { return gMillisValue; }

// Native model of wired-AND GPIO levels during example startup bus clear.
inline uint32_t gMicrosValue = 0;
static constexpr int INPUT_PULLUP = 2;
static constexpr int OUTPUT = 3;
static constexpr int OUTPUT_OPEN_DRAIN = 0x13;
static constexpr int LOW = 0;
static constexpr int HIGH = 1;
struct StubPin {
  int mode = INPUT_PULLUP;
  int level = HIGH;
  bool heldLow = false;
  uint32_t lowReadsRemaining = 0;
};
inline StubPin gStubPins[64];
inline uint32_t gActiveHighWrites = 0;
inline void resetStubPins() {
  for (auto& pin : gStubPins) pin = StubPin{};
  gActiveHighWrites = 0;
}
inline void pinMode(int pin, int mode) { gStubPins[pin].mode = mode; }
inline void digitalWrite(int pin, int level) {
  gStubPins[pin].level = level;
  if (level == HIGH && gStubPins[pin].mode == OUTPUT) ++gActiveHighWrites;
}
inline int digitalRead(int pin) {
  auto& value = gStubPins[pin];
  if (value.heldLow) return LOW;
  if (value.lowReadsRemaining > 0) { --value.lowReadsRemaining; return LOW; }
  return value.level;
}

inline uint32_t micros() { return gMicrosValue; }
inline void delay(uint32_t ms) { gMicrosValue += ms * 1000U; }
inline void delayMicroseconds(uint32_t us) { gMicrosValue += us; }
inline void yield() {}

// Serial stub
class SerialClass {
public:
  void begin(uint32_t baud) { (void)baud; }
  void print(const char* s) { (void)s; }
  void println(const char* s = "") { (void)s; }
  void printf(const char* fmt, ...) { (void)fmt; }
  int available() { return 0; }
  int read() { return -1; }
  operator bool() { return true; }
};

extern SerialClass Serial;

// String class (minimal stub)
class String {
public:
  String() = default;
  String(const char* s) : _data(s ? s : "") {}
  const char* c_str() const { return _data.c_str(); }
  size_t length() const { return _data.length(); }
  void trim() {}
  bool startsWith(const char* prefix) const {
    return _data.find(prefix) == 0;
  }
  String substring(size_t start) const {
    return String(_data.substr(start).c_str());
  }
  int toInt() const { return std::stoi(_data); }
  String& operator+=(char c) { _data += c; return *this; }
private:
  std::string _data;
};
