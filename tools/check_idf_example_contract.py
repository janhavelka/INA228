#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

IDF_EXAMPLE_MACRO = "INA228_EXAMPLE_PLATFORM_IDF"
CLI_SOURCE_INCLUDE = '#include "examples/01_basic_bringup_cli/main.cpp"'
REQUIRED_COMPONENTS = [
    "INA228",
    "esp_driver_i2c",
    "esp_driver_gpio",
    "esp_timer",
    "freertos",
]
REQUIRED_FILES = [
    "CMakeLists.txt",
    "idf_component.yml",
    "examples/esp_idf/basic/CMakeLists.txt",
    "examples/esp_idf/basic/main/CMakeLists.txt",
    "examples/esp_idf/basic/main/GenerateGitProvenance.cmake",
    "examples/esp_idf/basic/main/main.cpp",
    "examples/esp_idf/basic/main/Ina228IdfI2cTransport.h",
    "examples/esp_idf/basic/main/Ina228IdfI2cTransport.cpp",
    "docs/integration/esp-idf.md",
]
MANDATORY_COMMANDS = [
    "help",
    "version",
    "scan",
    "scanina",
    "read",
    "raw",
    "timing",
    "vbus",
    "vshunt",
    "temp",
    "current",
    "power",
    "energy",
    "charge",
    "ready",
    "trigger",
    "mode",
    "convtime",
    "averaging",
    "adcrange",
    "cal",
    "tempco",
    "tempcomp",
    "delay",
    "cfg",
    "addr",
    "init",
    "end",
    "reset",
    "rstacc",
    "diag",
    "diagraw",
    "limits",
    "alatch",
    "cnvralert",
    "alslow",
    "apol",
    "sovl",
    "suvl",
    "bovl",
    "buvl",
    "tmplim",
    "pwrlim",
    "mfgid",
    "devid",
    "reg16",
    "reg24",
    "reg40",
    "wreg16",
    "drv",
    "probe",
    "recover",
    "verbose",
    "stress",
    "stress_mix",
    "hilrun",
    "xfer_reset",
    "xfer_stats",
    "xfer_assert",
    "selftest",
]
FORBIDDEN_IDF_TOKENS = [
    "Arduino.h",
    "Wire.h",
    "TwoWire",
    "String",
    "Serial",
    "ArduinoCompat",
    "IdfArduinoCompat",
    CLI_SOURCE_INCLUDE,
]
CI_REQUIRED_TOKENS = [
    "esp-idf-basic:",
    "uses: espressif/esp-idf-ci-action@v1",
    "esp_idf_version: v6.0.1",
    "target: ${{ matrix.target }}",
    "path: examples/esp_idf/basic",
    "command: idf.py set-target ${{ matrix.target }} build",
    "workflow_dispatch:",
    "Build ESP-IDF basic ${{ matrix.target }} with idf.py",
    "esp32s3",
    "esp32s2",
]
BUILD_DOC_REQUIRED_TOKENS = [
    "ESP-IDF v6.0.1",
    "idf.py --version",
    "idf.py -C examples/esp_idf/basic set-target esp32s3 build",
    "idf.py -C examples/esp_idf/basic set-target esp32s2 build",
    "examples/esp_idf/basic/build/",
    "static contract check",
    "not hardware validation",
]
CLI_WARNING_TOKENS = [
    "Safety:",
    "destructive/status-clearing",
    "continuous accumulation only",
    "validity flags",
]


def fail(msg: str) -> None:
    print(f"IDF example contract FAILED: {msg}")
    raise SystemExit(1)


def require_token(text: str, token: str, label: str) -> None:
    if token not in text:
        fail(f"{label} missing token '{token}'")


def command_has_dispatch(cli: str, command: str) -> bool:
    patterns = [
        rf'cmd\s*==\s*"{re.escape(command)}"',
        rf'cmd\.startsWith\("{re.escape(command)}\s',
        rf'cmd\.startsWith\("{re.escape(command)}"\)',
        rf'std::strcmp\(cmd,\s*"{re.escape(command)}"\)\s*==\s*0',
        rf'argAfter\(cmd,\s*"{re.escape(command)}\s',
        rf'startsWith\(cmd,\s*"{re.escape(command)}"\)',
    ]
    return any(re.search(pattern, cli) for pattern in patterns)


def help_items(text: str) -> list[tuple[str, str]]:
    return re.findall(r'printHelpItem\("([^"]+)",\s*"([^"]+)"\)', text)


def aliases_from_help(items: list[tuple[str, str]]) -> set[str]:
    aliases: set[str] = set()
    for command_spec, _ in items:
        for alternative in command_spec.split(" / "):
            alias = alternative.strip().split(" ", 1)[0]
            if alias:
                aliases.add(alias)
    return aliases


def main() -> int:
    for rel in REQUIRED_FILES:
        if not (ROOT / rel).exists():
            fail(f"missing {rel}")

    idf_main = (ROOT / "examples" / "esp_idf" / "basic" / "main" / "main.cpp").read_text(
        encoding="utf-8", errors="replace"
    )
    for token in (
        'extern "C" void app_main(void)',
        '#include "driver/i2c_master.h"',
        "esp_timer_get_time",
        "vTaskDelay",
        "select(",
        "read(STDIN_FILENO",
        "char line[MAX_LINE_LEN]",
        "ina228IdfProbeAddress",
        "ina228IdfI2cWriteReadAt",
    ):
        require_token(idf_main, token, "ESP-IDF main")
    if "std::fgets" in idf_main:
        fail("ESP-IDF main must not block driver tick progress in std::fgets")
    if IDF_EXAMPLE_MACRO in idf_main:
        fail("ESP-IDF main must not enable the old shared Arduino CLI path")
    for token in FORBIDDEN_IDF_TOKENS:
        if token in idf_main:
            fail(f"ESP-IDF main contains forbidden Arduino/facade token '{token}'")

    cmake = (
        ROOT / "examples" / "esp_idf" / "basic" / "main" / "CMakeLists.txt"
    ).read_text(encoding="utf-8", errors="replace")
    if re.search(r"INCLUDE_DIRS[^\r\n]*\.\./", cmake) or "examples/common" in cmake:
        fail("ESP-IDF main CMake must not expose repo root or examples/common include paths")
    for component in REQUIRED_COMPONENTS:
        if re.search(rf"\b{re.escape(component)}\b", cmake) is None:
            fail(f"ESP-IDF CMake missing required component '{component}'")

    if (ROOT / "examples" / "common" / "IdfArduinoCompat.h").exists():
        fail("examples/common/IdfArduinoCompat.h must not exist")

    transport = (
        (ROOT / "examples" / "esp_idf" / "basic" / "main" / "Ina228IdfI2cTransport.cpp")
        .read_text(encoding="utf-8", errors="replace")
        + (ROOT / "examples" / "esp_idf" / "basic" / "main" / "Ina228IdfI2cTransport.h")
        .read_text(encoding="utf-8", errors="replace")
    )
    for token in (
        "driver/i2c_master.h",
        "i2c_new_master_bus",
        "i2c_master_probe",
        "i2c_master_transmit",
        "i2c_master_transmit_receive",
        "mapEspProbeErr",
        "ESP_ERR_NOT_FOUND",
        "I2C_NACK_ADDR",
        "I2C_NACK_UNKNOWN_PHASE",
        "I2C NACK, ESP-IDF phase unavailable",
        "single-owner",
    ):
        require_token(transport, token, "ESP-IDF transport")
    for token in FORBIDDEN_IDF_TOKENS:
        if token in transport:
            fail(f"ESP-IDF transport contains forbidden Arduino/facade token '{token}'")

    cli = (ROOT / "examples" / "01_basic_bringup_cli" / "main.cpp").read_text(
        encoding="utf-8", errors="replace"
    )
    for text, label in ((cli, "Arduino CLI"), (idf_main, "native ESP-IDF CLI")):
        for token in CLI_WARNING_TOKENS:
            require_token(text, token, label)
        require_token(
            text,
            "if (!st.ok() && st.msg != nullptr && st.msg[0] != '\\0')",
            f"{label} non-OK message gate",
        )
    if "st.code == INA228::Err::I2C_NACK_ADDR || st.code == INA228::Err::I2C_ERROR" in idf_main:
        fail("native ESP-IDF scan must not hide generic I2C_ERROR as an empty address")
    for command in MANDATORY_COMMANDS:
        if f'printHelpItem("{command}' not in cli:
            fail(f"Arduino CLI missing help item '{command}'")
        if not command_has_dispatch(cli, command):
            fail(f"Arduino CLI missing dispatch '{command}'")
        if f'printHelpItem("{command}' not in idf_main:
            fail(f"native ESP-IDF CLI missing help item '{command}'")
        if not command_has_dispatch(idf_main, command):
            fail(f"native ESP-IDF CLI missing dispatch '{command}'")

    arduino_help = help_items(cli)
    idf_help = help_items(idf_main)
    if arduino_help != idf_help:
        missing = [item for item in arduino_help if item not in idf_help]
        extra = [item for item in idf_help if item not in arduino_help]
        fail(f"Arduino/native help rows differ: missing={missing}, extra={extra}")
    aliases = aliases_from_help(arduino_help)
    for command in sorted(aliases):
        if not command_has_dispatch(cli, command):
            fail(f"Arduino help alias '{command}' has no dispatch")
        if not command_has_dispatch(idf_main, command):
            fail(f"native ESP-IDF help alias '{command}' has no dispatch")
    if ".toInt()" in cli:
        fail("Arduino CLI must use strict full-token numeric parsing")
    for text, label in ((cli, "Arduino CLI"), (idf_main, "native ESP-IDF CLI")):
        for token in (
            "rejectInvalidCommand",
            "parseBool01",
            "wreg16 <addr> <val> confirm",
            "Confirmation required: wreg16",
        ):
            require_token(text, token, label)
    for token in (
        "firstSourceLen >= firstLen",
        "secondSourceLen >= secondLen",
    ):
        require_token(idf_main, token, "native ESP-IDF full-token parser")
    if "std::strncpy(first" in idf_main or "std::strncpy(second" in idf_main:
        fail("native ESP-IDF argument splitting must reject instead of truncate")

    manifest = (ROOT / "idf_component.yml").read_text(encoding="utf-8", errors="replace")
    for token in ("esp32s2", "esp32s3", "idf:"):
        require_token(manifest, token, "idf_component.yml")

    build_doc = (ROOT / "docs" / "integration" / "esp-idf.md").read_text(
        encoding="utf-8", errors="replace"
    )
    for token in BUILD_DOC_REQUIRED_TOKENS:
        require_token(build_doc, token, "ESP-IDF build guide")

    provenance = (
        ROOT
        / "examples"
        / "esp_idf"
        / "basic"
        / "main"
        / "GenerateGitProvenance.cmake"
    ).read_text(encoding="utf-8", errors="replace")
    for token in (
        "add_custom_target(ina228_example_git_provenance ALL",
        "add_dependencies(${COMPONENT_LIB} ina228_example_git_provenance)",
        "ina228_git_provenance.h",
    ):
        require_token(cmake, token, "ESP-IDF firmware provenance build hook")
    require_token(
        idf_main,
        '#include "ina228_git_provenance.h"',
        "ESP-IDF firmware provenance include",
    )
    for token in (
        "rev-parse --short=12 HEAD",
        "status --porcelain --untracked-files=normal",
        "INA228_GIT_COMMIT",
        "INA228_GIT_STATUS",
        "copy_if_different",
    ):
        require_token(provenance, token, "ESP-IDF firmware provenance generator")

    gitignore = (ROOT / ".gitignore").read_text(encoding="utf-8", errors="replace")
    for token in (
        "/examples/esp_idf/basic/build/",
        "/examples/esp_idf/basic/sdkconfig",
    ):
        require_token(gitignore, token, "ESP-IDF generated-file ignore policy")

    ci = (ROOT / ".github" / "workflows" / "ci.yml").read_text(
        encoding="utf-8", errors="replace"
    )
    for token in CI_REQUIRED_TOKENS:
        require_token(ci, token, "CI ESP-IDF build matrix")

    print("IDF example contract PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
