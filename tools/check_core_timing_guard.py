#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCAN_DIRS = ("src", "include")
VALID_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp"}

FORBIDDEN_CALLS = {
    "delay": re.compile(r"\bdelay\s*\("),
    "millis": re.compile(r"\bmillis\s*\("),
    "micros": re.compile(r"\bmicros\s*\("),
    "delayMicroseconds": re.compile(r"\bdelayMicroseconds\s*\("),
    "yield": re.compile(r"\byield\s*\("),
    "vTaskDelay": re.compile(r"\bvTaskDelay\s*\("),
    "xTaskGetTickCount": re.compile(r"\bxTaskGetTickCount\s*\("),
    "esp_timer_get_time": re.compile(r"\besp_timer_get_time\s*\("),
}

FORBIDDEN_CODE_TOKENS = {
    "Wire": re.compile(r"\bWire\b"),
    "TwoWire": re.compile(r"\bTwoWire\b"),
    "Serial": re.compile(r"\bSerial\b"),
    "String": re.compile(r"\bString\b"),
    "esp_err_t": re.compile(r"\besp_err_t\b"),
    "i2c_master": re.compile(r"\bi2c_master_\w+"),
    "FreeRTOS": re.compile(r"\bFreeRTOS\b"),
}

FORBIDDEN_INCLUDE_RE = re.compile(
    r'^\s*#\s*include\s*[<\"]'
    r'(Arduino\.h|Wire\.h|driver/[^>\"]+|esp_[^>\"]+|freertos/[^>\"]+|FreeRTOS\.h|examples/[^>\"]+)'
    r'[>\"]',
    re.MULTILINE,
)
NON_CODE_RE = re.compile(
    r'(?:u8|u|U|L)?R"(?P<raw_delim>[^ ()\\\t\r\n]{0,16})\(.*?\)'
    r'(?P=raw_delim)"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|'
    r'/\*.*?\*/|//[^\n]*',
    re.DOTALL,
)

def strip_non_code(text: str) -> str:
    return NON_CODE_RE.sub("", text)


def validate_lexer(errors: list[str]) -> None:
    executable = 'auto text = R"tag(text" // not a comment)tag"; vTaskDelay(1);'
    hidden = 'auto text = R"tag(vTaskDelay(1);)tag";'
    if "vTaskDelay(1)" not in strip_non_code(executable):
        errors.append("internal lexer hid executable code following a raw string")
    if "vTaskDelay(1)" in strip_non_code(hidden):
        errors.append("internal lexer retained code-like text inside a raw string")


def collect_sources() -> list[pathlib.Path]:
    files: list[pathlib.Path] = []
    for dirname in SCAN_DIRS:
        root = ROOT / dirname
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if path.is_file() and path.suffix.lower() in VALID_SUFFIXES:
                files.append(path)
    return files


def main() -> int:
    errors: list[str] = []
    validate_lexer(errors)

    for path in collect_sources():
        rel = path.relative_to(ROOT).as_posix()
        raw = path.read_text(encoding="utf-8", errors="replace")
        code = strip_non_code(raw)

        for call_name, pattern in FORBIDDEN_CALLS.items():
            count = len(pattern.findall(code))
            if count > 0:
                errors.append(f"forbidden timing call in {rel}: {call_name} count={count}")

        for token_name, pattern in FORBIDDEN_CODE_TOKENS.items():
            count = len(pattern.findall(code))
            if count > 0:
                errors.append(
                    f"forbidden framework/core-boundary token in {rel}: "
                    f"{token_name} count={count}"
                )

        include_count = len(FORBIDDEN_INCLUDE_RE.findall(raw))
        if include_count > 0:
            errors.append(
                f"forbidden framework include in {rel}: count={include_count}"
            )

    if errors:
        print("Core timing guard FAILED:")
        for err in errors:
            print(f"- {err}")
        return 1

    print("Core timing guard PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
