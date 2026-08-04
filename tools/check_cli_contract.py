#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED_COMMON = [
    "BoardConfig.h",
    "BuildConfig.h",
    "Log.h",
    "I2cTransport.h",
    "I2cScanner.h",
    "CliStyle.h",
]

MANDATORY_COMMANDS = [
    "help",
    "?",
    "version",
    "ver",
    "scan",
    "probe",
    "recover",
    "drv",
    "read",
    "verbose",
    "stress",
    "hilrun",
    "xfer_reset",
    "xfer_stats",
    "xfer_assert",
]


def help_items(text: str) -> list[tuple[str, str]]:
    return re.findall(r'cli::printHelpItem\("([^"]+)",\s*"([^"]+)"\)', text)


def aliases_from_help(items: list[tuple[str, str]]) -> set[str]:
    aliases: set[str] = set()
    for command_spec, _ in items:
        for alternative in command_spec.split(" / "):
            alias = alternative.strip().split(" ", 1)[0]
            if alias:
                aliases.add(alias)
    return aliases


def command_has_dispatch(text: str, command: str) -> bool:
    quoted = re.escape(f'"{command}"')
    prefix_quoted = re.escape(f'"{command} "')
    patterns = (
        rf"cmd\s*==\s*{quoted}",
        rf"cmd\.startsWith\(\s*{prefix_quoted}\s*\)",
    )
    return any(re.search(pattern, text) for pattern in patterns)


def fail(msg: str) -> None:
    print(f"CLI contract FAILED: {msg}")
    raise SystemExit(1)


def ensure_exists(path: pathlib.Path, label: str) -> None:
    if not path.exists():
        fail(f"missing {label}: {path.as_posix()}")


def ensure_missing(path: pathlib.Path, label: str) -> None:
    if path.exists():
        fail(f"forbidden {label} still present: {path.as_posix()}")


def main() -> int:
    common_dir = ROOT / "examples" / "common"
    bringup_main = ROOT / "examples" / "01_basic_bringup_cli" / "main.cpp"

    ensure_exists(common_dir, "common example directory")
    ensure_exists(bringup_main, "bringup CLI example")

    ensure_missing(ROOT / "examples" / "00_smoke_boot", "deprecated example 00_smoke_boot")
    ensure_missing(
        ROOT / "examples" / "03_feature_walkthrough",
        "deprecated example 03_feature_walkthrough",
    )

    for name in REQUIRED_COMMON:
        ensure_exists(common_dir / name, f"common helper {name}")

    ensure_missing(common_dir / "IdfArduinoCompat.h", "Arduino compatibility facade")

    text = bringup_main.read_text(encoding="utf-8", errors="replace")

    items = help_items(text)
    if not items:
        fail("no CLI help items found")
    aliases = aliases_from_help(items)
    for cmd in MANDATORY_COMMANDS:
        if cmd not in aliases:
            fail(f"mandatory command '{cmd}' missing from CLI help")
    for cmd in sorted(aliases):
        if not command_has_dispatch(text, cmd):
            fail(f"help command or alias '{cmd}' has no visible dispatch")

    if re.search(r"\bcfg\b", text) is None and re.search(r"\bsettings\b", text) is None:
        fail("either 'cfg' or 'settings' command must be present")
    if re.search(r"\bstatic\s+String\b", text):
        fail("bringup CLI must not use a static String input buffer")
    for token in ("CLI_MAX_LINE_LEN", "CLI_MAX_BYTES_PER_LOOP", "MAX_STRESS_COUNT"):
        if token not in text:
            fail(f"bringup CLI missing bounded console/stress token '{token}'")
    if ".toInt()" in text:
        fail("bringup CLI must use strict full-token numeric parsing, not String::toInt()")
    for token in (
        "parseI32",
        "parseBool01",
        "rejectInvalidCommand",
        "wreg16 <addr> <val> confirm",
        "Confirmation required: wreg16",
    ):
        if token not in text:
            fail(f"bringup CLI missing strict parsing/safety token '{token}'")

    print("CLI contract PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
