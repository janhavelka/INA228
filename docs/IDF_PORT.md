# INA228 ESP-IDF Portability Status

Last audited: 2026-04-03

## Current Reality
- Primary runtime remains PlatformIO + Arduino.
- Core transport is callback-based (`Config.i2cWrite`, `Config.i2cWriteRead`).
- Optional timing hook is available (`Config.nowMs`, `Config.timeUser`).
- Core measurement/recovery logic uses `_nowMs()` wrapper.
- Arduino timing is used only as fallback in one place:
  - `INA228::_nowMs()` -> `millis()` when `Config.nowMs == nullptr`

## ESP-IDF Adapter Requirements
To run under pure ESP-IDF, provide:
1. I2C write callback.
2. I2C write-read callback.
3. Optional `nowMs(user)` callback.

## Minimal Adapter Pattern
```cpp
static uint32_t idfNowMs(void*) {
  return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

INA228::Config cfg{};
cfg.i2cWrite = myI2cWrite;
cfg.i2cWriteRead = myI2cWriteRead;
cfg.nowMs = idfNowMs;
```
