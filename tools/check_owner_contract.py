#!/usr/bin/env python3
"""Static guard for the bounded external-I2C-owner contract."""

from __future__ import annotations

import json
import pathlib
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"Owner contract FAILED: {message}")
    raise SystemExit(1)


def require(text: str, token: str, label: str) -> None:
    if token not in text:
        fail(f"{label} missing token {token!r}")


def forbid(text: str, token: str, label: str) -> None:
    if token in text:
        fail(f"{label} contains forbidden token {token!r}")


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8", errors="replace")


def main() -> int:
    public = read("include/INA228/INA228.h")
    config = read("include/INA228/Config.h")
    implementation = read("src/INA228.cpp")
    arduino = read("examples/01_basic_bringup_cli/main.cpp")
    idf = read("examples/esp_idf/basic/main/main.cpp")
    platformio = read("platformio.ini")

    for token in (
        "Status bind(const Config& config)",
        "Status startInitialize(uint32_t requestToken, uint32_t& operationId)",
        "Status startReinitialize(uint32_t requestToken, uint32_t& operationId)",
        "Status startVerifyConfiguration(uint32_t requestToken, uint32_t& operationId)",
        "Status startInstantaneousSample(uint32_t requestToken, uint32_t& operationId)",
        "Status startReset(uint32_t requestToken, uint32_t& operationId)",
        "Status startAccumulatorReset(uint32_t requestToken, uint32_t& operationId)",
        "Status pollJob(uint32_t nowMs, uint8_t maxTransfers)",
        "Status cancelJob()",
        "Status timeoutJob()",
        "Status takeJobResult(uint32_t expectedOperationId, JobResult& out)",
        "Status invalidateHardwareState(const Status& cause)",
        "enum class JobEffect",
        "INDETERMINATE",
        "uint32_t requestToken",
        "bool resultAvailable",
        "uint8_t maxRetries = 0",
    ):
        require(public, token, "public API")

    for token in (
        "enum class HealthPolicy",
        "PASSIVE = 0",
        "HealthPolicy healthPolicy = HealthPolicy::PASSIVE",
        "CalibrationConfig calibration{}",
        "AlertConfig alerts{}",
        "uint16_t supportedRevisionMask",
    ):
        require(config, token, "configuration")

    for token in (
        "if (maxTransfers == 0)",
        "Job in progress; no transfer budget",
        "_terminalResultAvailable",
        "Err::STALE_RESULT",
        "JobEffect::INDETERMINATE",
        "HardwareState::RESYNC_REQUIRED",
    ):
        require(implementation, token, "implementation")

    core = public + config + implementation
    for token in (
        "Arduino.h",
        "Wire.h",
        "driver/i2c_master.h",
        "freertos/",
        "FreeRTOS",
        "vTaskDelay",
    ):
        forbid(core, token, "core")

    for example, label in ((arduino, "Arduino example"), (idf, "ESP-IDF example")):
        for token in (
            "device.bind(",
            "device.startInitialize(",
            "device.startInstantaneousSample(",
            "device.startReinitialize(",
            "device.pollJob(",
            "device.timeoutJob(",
            "device.takeJobResult(",
            "CalibrationMode::FROM_MAXIMUM_CURRENT",
            "HealthPolicy::PASSIVE",
        ):
            require(example, token, label)
        for token in (
            "device.begin(",
            "device.recover(",
            "device.readPowerSampleRawStep(",
            "device.startResetJob(",
            "device.pollResetJob(",
            "device.startConfigReplayJob(",
            "device.pollConfigReplayJob(",
        ):
            forbid(example, token, label)

    require(platformio, "build_unflags =\n  -std=gnu++11", "PlatformIO")
    require(platformio, "-std=gnu++17", "PlatformIO")

    version = str(json.loads(read("library.json"))["version"])
    require(read("include/INA228/Version.h"),
            f'#define INA228_VERSION_STRING "{version}"', "generated version")
    require(read("idf_component.yml"), f'version: "{version}"', "IDF metadata")
    require(read("Doxyfile"), f'PROJECT_NUMBER         = "{version}"',
            "Doxygen metadata")
    require(read("CHANGELOG.md"), f"## [{version}]", "changelog")

    print("Owner contract PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
