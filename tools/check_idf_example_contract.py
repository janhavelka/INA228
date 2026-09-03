#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

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
    "verify_start",
    "verify_step",
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
]
REQUIRED_IDF_TARGETS = {"esp32s2", "esp32s3"}
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


def balanced_contents(text: str, opening_index: int,
                      opening: str, closing: str) -> str:
    """Return balanced delimiter contents while ignoring literals/comments."""
    depth = 0
    quote: str | None = None
    escaped = False
    line_comment = False
    block_comment = False
    index = opening_index
    while index < len(text):
        char = text[index]
        following = text[index + 1] if index + 1 < len(text) else ""
        if line_comment:
            if char == "\n":
                line_comment = False
        elif block_comment:
            if char == "*" and following == "/":
                block_comment = False
                index += 1
        elif quote is not None:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = None
        elif char == "/" and following == "/":
            line_comment = True
            index += 1
        elif char == "/" and following == "*":
            block_comment = True
            index += 1
        elif char in ('"', "'"):
            quote = char
        elif char == opening:
            depth += 1
        elif char == closing:
            depth -= 1
            if depth == 0:
                return text[opening_index + 1:index]
        index += 1
    fail(f"unbalanced '{opening}{closing}' delimiters")
    raise AssertionError("unreachable")


def cmake_command_args(text: str, command: str) -> str:
    match = re.search(rf"\b{re.escape(command)}\s*\(", text)
    if match is None:
        fail(f"CMake missing {command}()")
    opening_index = text.find("(", match.start())
    return balanced_contents(text, opening_index, "(", ")")


def command_dispatch_block(cli: str, command: str) -> str:
    patterns = (
        rf'\bif\s*\([^{{;]*\bcmd\s*==\s*"{re.escape(command)}"[^{{;]*\)',
        rf'\b(?:else\s+)?if\s*\([^{{;]*std::strcmp\(\s*cmd\s*,\s*'
        rf'"{re.escape(command)}"\s*\)\s*==\s*0[^{{;]*\)',
    )
    match = None
    for pattern in patterns:
        match = re.search(pattern, cli)
        if match is not None:
            break
    if match is None:
        fail(f"missing exact dispatch block for '{command}'")
    opening_index = cli.find("{", match.end())
    if opening_index < 0:
        fail(f"dispatch '{command}' has no body")
    return balanced_contents(cli, opening_index, "{", "}")


def function_block(source: str, function: str) -> str:
    match = re.search(rf"\b{re.escape(function)}\s*\(", source)
    if match is None:
        fail(f"missing function '{function}'")
    args_open = source.find("(", match.start())
    balanced_contents(source, args_open, "(", ")")
    body_open = source.find("{", args_open)
    if body_open < 0:
        fail(f"function '{function}' has no body")
    return balanced_contents(source, body_open, "{", "}")


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


def string_array(text: str, name: str) -> tuple[str, ...]:
    match = re.search(
        rf"\b{re.escape(name)}\s*\[\]\s*=\s*\{{(.*?)\}};", text, re.DOTALL
    )
    if match is None:
        fail(f"missing string array {name}")
    return tuple(re.findall(r'"([^"]+)"', match.group(1)))


def selftest_labels(text: str, function: str) -> tuple[str, ...]:
    labels: list[str] = []
    pattern = re.compile(
        rf'\b{re.escape(function)}\(\s*(?:"([^"]+)"|[^,]+,\s*"([^"]+)")'
    )
    for match in pattern.finditer(text):
        labels.append(match.group(1) or match.group(2))
    return tuple(labels)


def constexpr_value(text: str, name: str) -> int:
    match = re.search(
        rf"\b{re.escape(name)}\s*=\s*(0x[0-9A-Fa-f]+|[0-9]+)U?\s*;", text
    )
    if match is None:
        fail(f"missing constexpr value {name}")
    return int(match.group(1), 0)


def config_value(text: str, field: str) -> int:
    match = re.search(
        rf"\bcfg\.{re.escape(field)}\s*=\s*(0x[0-9A-Fa-f]+|[0-9]+)U?\s*;", text
    )
    if match is None:
        fail(f"missing example config field cfg.{field}")
    return int(match.group(1), 0)


def validate_parsers() -> None:
    nested = "idf_component_register(SRCS helper(foo) INCLUDE_DIRS include)"
    args = cmake_command_args(nested, "idf_component_register")
    if "INCLUDE_DIRS include" not in args:
        fail("balanced CMake parser truncated a nested argument")


def main() -> int:
    validate_parsers()
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
    for token in FORBIDDEN_IDF_TOKENS:
        if token in idf_main:
            fail(f"ESP-IDF main contains forbidden Arduino/facade token '{token}'")

    cmake = (
        ROOT / "examples" / "esp_idf" / "basic" / "main" / "CMakeLists.txt"
    ).read_text(encoding="utf-8", errors="replace")
    cmake_code = re.sub(r"(?m)#.*$", "", cmake)
    component_args = cmake_command_args(cmake_code, "idf_component_register")
    include_dirs = re.search(
        r"\bINCLUDE_DIRS\b(.*?)(?=\b(?:REQUIRES|PRIV_REQUIRES|EMBED_FILES|"
        r"EMBED_TXTFILES|WHOLE_ARCHIVE)\b|$)",
        component_args,
        re.DOTALL,
    )
    if include_dirs is None:
        fail("ESP-IDF main CMake missing INCLUDE_DIRS")
    if (include_dirs is not None and "../" in include_dirs.group(1)) or "examples/common" in cmake:
        fail("ESP-IDF main CMake must not expose repo root or examples/common include paths")
    for component in REQUIRED_COMPONENTS:
        if re.search(rf"\b{re.escape(component)}\b", cmake) is None:
            fail(f"ESP-IDF CMake missing required component '{component}'")

    if (ROOT / "examples" / "common" / "IdfArduinoCompat.h").exists():
        fail("examples/common/IdfArduinoCompat.h must not exist")

    transport = "\n".join(
        (ROOT / "examples" / "esp_idf" / "basic" / "main" / filename)
        .read_text(encoding="utf-8", errors="replace")
        for filename in ("Ina228IdfI2cTransport.cpp", "Ina228IdfI2cTransport.h")
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
        "INA228::Status ina228IdfSelectDeviceAddress",
        "INA228::Status ina228IdfDeinitI2c",
        "I2C temporary device removal failed",
        "temporaryDevPendingRemoval",
        "removePendingTemporaryDevice",
        "single-owner",
    ):
        require_token(transport, token, "ESP-IDF transport")
    for discarded_cleanup in (
        "(void)i2c_master_bus_rm_device",
        "(void)i2c_del_master_bus",
    ):
        if discarded_cleanup in transport:
            fail(f"ESP-IDF transport discards cleanup result '{discarded_cleanup}'")
    for token in FORBIDDEN_IDF_TOKENS:
        if token in transport:
            fail(f"ESP-IDF transport contains forbidden Arduino/facade token '{token}'")
    idf_source_root = ROOT / "examples" / "esp_idf" / "basic" / "main"
    idf_sources = "\n".join(
        path.read_text(encoding="utf-8", errors="replace")
        for path in sorted(idf_source_root.rglob("*"))
        if path.is_file()
        and path.suffix.lower() in {".c", ".cc", ".cpp", ".h", ".hpp"}
    )
    if re.search(r'#\s*include\s*[<"][^">]*\.cpp[>"]', idf_sources):
        fail("native ESP-IDF sources must not include implementation .cpp files")
    if "examples/01_basic_bringup_cli" in idf_sources or "examples/common/" in idf_sources:
        fail("native ESP-IDF sources must not include Arduino example paths")
    for token in FORBIDDEN_IDF_TOKENS:
        if token in idf_sources:
            fail(
                "native ESP-IDF sources contain forbidden Arduino/facade "
                f"token '{token}'"
            )

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
        require_token(
            text,
            "Finish the cooperative operation before accessing the bus",
            f"{label} cooperative bus-owner guard",
        )
        guarded_helpers = {
            "probe": "probeAddressRaw",
            "selftest": "runSelfTest",
        }
        for command in ("scan", "scanina", "probe", "recover", "selftest"):
            dispatch = command_dispatch_block(text, command)
            helper = guarded_helpers.get(command)
            if helper is None:
                require_token(
                    dispatch,
                    "diagnosticBusAccessStatus()",
                    f"{label} '{command}' owner guard",
                )
            else:
                require_token(dispatch, f"{helper}(",
                              f"{label} '{command}' guarded route")
                require_token(
                    function_block(text, helper),
                    "diagnosticBusAccessStatus()",
                    f"{label} '{command}' helper owner guard",
                )
        for function in ("printBusAlertLimit", "printTemperatureAlertLimit"):
            require_token(
                function_block(text, function),
                "printThresholdReapplyWarning(settings)",
                f"{label} {function} threshold advisory",
            )
    if "st.code == INA228::Err::I2C_NACK_ADDR || st.code == INA228::Err::I2C_ERROR" in idf_main:
        fail("native ESP-IDF scan must not hide generic I2C_ERROR as an empty address")
    arduino_help = help_items(cli)
    idf_help = help_items(idf_main)
    arduino_aliases = aliases_from_help(arduino_help)
    idf_aliases = aliases_from_help(idf_help)
    for command in MANDATORY_COMMANDS:
        if command not in arduino_aliases:
            fail(f"Arduino CLI missing help item '{command}'")
        if not command_has_dispatch(cli, command):
            fail(f"Arduino CLI missing dispatch '{command}'")
        if command not in idf_aliases:
            fail(f"native ESP-IDF CLI missing help item '{command}'")
        if not command_has_dispatch(idf_main, command):
            fail(f"native ESP-IDF CLI missing dispatch '{command}'")

    if arduino_help != idf_help:
        missing = [item for item in arduino_help if item not in idf_help]
        extra = [item for item in idf_help if item not in arduino_help]
        if missing or extra:
            fail(f"Arduino/native help rows differ: missing={missing}, extra={extra}")
        index = next(
            i for i, (a, b) in enumerate(zip(arduino_help, idf_help)) if a != b
        )
        fail(
            f"Arduino/native help rows are ordered differently at row {index}: "
            f"arduino={arduino_help[index]!r} idf={idf_help[index]!r}"
        )
    arduino_stress = string_array(cli, "STRESS_MIX_OPERATIONS")
    idf_stress = string_array(idf_main, "STRESS_MIX_OPERATIONS")
    if arduino_stress != idf_stress:
        fail(f"stress_mix operations differ: arduino={arduino_stress} idf={idf_stress}")
    for text, label in ((cli, "Arduino CLI"), (idf_main, "native ESP-IDF CLI")):
        stress_body = function_block(text, "runStress")
        if re.search(
            r"if\s*\(verboseMode\)\s*\{[^{}]*failed:",
            stress_body,
            re.DOTALL,
        ) is None:
            fail(f"{label} verbose stress output must report failed samples")
    for text, label in ((cli, "Arduino CLI"), (idf_main, "native ESP-IDF CLI")):
        match = re.search(
            r"bool\s+stressMixStatusAccepted\([^)]*\)\s*\{(.*?)\}", text, re.DOTALL
        )
        if match is None or "return st.ok();" not in match.group(1):
            fail(f"{label} stress_mix must accept only Status::ok()")
    arduino_selftest = selftest_labels(cli, "reportCheck")
    idf_selftest = selftest_labels(idf_main, "reportSelftest")
    if arduino_selftest != idf_selftest:
        fail(
            f"selftest labels differ: arduino={arduino_selftest} idf={idf_selftest}"
        )
    for command in sorted(arduino_aliases):
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
    board = (ROOT / "examples" / "common" / "BoardConfig.h").read_text(
        encoding="utf-8", errors="replace"
    )
    for board_name, idf_name in (
        ("I2C_SDA", "I2C_SDA"),
        ("I2C_SCL", "I2C_SCL"),
        ("I2C_FREQ_HZ", "I2C_FREQ_HZ"),
        ("I2C_TIMEOUT_MS", "I2C_TIMEOUT_MS"),
        ("INA228_I2C_ADDR", "DEFAULT_I2C_ADDRESS"),
    ):
        if constexpr_value(board, board_name) != constexpr_value(idf_main, idf_name):
            fail(f"board default {board_name} differs from native IDF {idf_name}")
    for field in ("calibration.shuntMicroOhms", "calibration.maxCurrentMilliAmps"):
        if config_value(cli, field) != config_value(idf_main, field):
            fail(f"demo calibration field {field} differs between examples")
    for token in (
        "cfg.mode = INA228::Mode::CONT_ALL",
        "cfg.calibration.mode = INA228::CalibrationMode::FROM_MAXIMUM_CURRENT",
    ):
        require_token(cli, token, "Arduino demo profile")
        require_token(idf_main, token, "native ESP-IDF demo profile")
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
    exact_list_items = set(
        re.findall(r"(?m)^\s*-\s*(esp32s[23])\s*(?:#.*)?$", ci)
    )
    missing_targets = REQUIRED_IDF_TARGETS - exact_list_items
    if missing_targets:
        fail(f"CI ESP-IDF build matrix missing exact targets {sorted(missing_targets)}")

    print("IDF example contract PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
