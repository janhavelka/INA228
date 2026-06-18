#!/usr/bin/env python3
"""Bounded INA228 CLI hardware-in-loop smoke runner."""

from __future__ import annotations

import argparse
import re
import sys
import time
from dataclasses import dataclass
from typing import Iterable, Sequence


ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")


@dataclass(frozen=True)
class Step:
    command: str
    expected: tuple[str, ...]
    label: str


STEPS: tuple[Step, ...] = (
    Step("version", ("INA228 library version",), "version"),
    Step("scan", ("INA228 Address Probe", "Healthy INA228 devices"), "scan"),
    Step("probe", ("Status: OK",), "probe"),
    Step("settings", ("Active Settings", "State:", "Address:"), "settings"),
    Step("drv", ("Driver Health", "State:", "Online:"), "health"),
    Step("diagraw", ("DIAG_ALRT raw",), "DIAG_ALRT"),
    Step("raw", ("Raw Registers", "Vbus", "Temp"), "conversion raw read"),
)


FAILURE_TOKENS: tuple[str, ...] = (
    "I2C_NACK_ADDR",
    "I2C_NACK_DATA",
    "I2C_TIMEOUT",
    "I2C_BUS",
    "DEVICE_NOT_FOUND",
    "DEVICE_ID_MISMATCH",
    "MEMORY_ERROR",
    "HARDWARE_DIRTY",
)


FAILURE_PATTERNS: tuple[re.Pattern[str], ...] = (
    re.compile(r"^\s*\[FAIL\]", re.IGNORECASE | re.MULTILINE),
    re.compile(r"\bFAIL(?:ED|URES?)?\s*[:=]\s*[1-9][0-9]*\b", re.IGNORECASE),
    re.compile(r"\bERRORS?\s*[:=]\s*[1-9][0-9]*\b", re.IGNORECASE),
    re.compile(r"\bSTATUS\s*:\s*(?!OK\b)[A-Z0-9_]+", re.IGNORECASE),
)


def clean_output(text: str) -> str:
    return ANSI_RE.sub("", text).replace("\r\n", "\n").replace("\r", "\n")


def has_failure(text: str) -> bool:
    upper = text.upper()
    return any(token in upper for token in FAILURE_TOKENS) or any(
        pattern.search(text) for pattern in FAILURE_PATTERNS
    )


def classify_output(output: str, expected: Sequence[str]) -> str:
    text = clean_output(output)
    if has_failure(text):
        return "FAIL"
    if all(token in text for token in expected):
        return "PASS"
    return "UNKNOWN"


def parser_self_test() -> int:
    cases = (
        ("  Status: OK\n=== Driver Health ===\n  State: READY\n  Online: true\n",
         ("Driver Health", "State:", "Online:"), "PASS"),
        ("=== Driver Health ===\n  State: READY\n  Online: true\n  Last error: never\n",
         ("Driver Health", "State:", "Online:"), "PASS"),
        ("  Status: I2C_TIMEOUT (code=14, detail=-1)\n", ("Status: OK",), "FAIL"),
        ("[FAIL] probe responds\n", ("Status: OK",), "FAIL"),
        ("=== selftest ===\n  pass=4 fail=1 skip=0\n", ("selftest",), "FAIL"),
        ("=== Stress Summary ===\n  Success: 10\n  Errors: 3\n", ("Stress Summary",), "FAIL"),
        ("=== INA228 Address Probe (0x40-0x4F) ===\n  Healthy INA228 devices: 0\n",
         ("INA228 Address Probe", "Healthy INA228 devices"), "PASS"),
        ("boot banner only\n", ("Active Settings",), "UNKNOWN"),
    )
    for output, expected, want in cases:
        got = classify_output(output, expected)
        if got != want:
            print(f"parser self-test FAILED: expected {want}, got {got}")
            return 1
    print("parser self-test PASSED")
    return 0


def print_plan(steps: Iterable[Step]) -> None:
    print("INA228 HIL command plan:")
    for step in steps:
        expected = ", ".join(step.expected)
        print(f"  {step.command:<10} # {step.label}; expect: {expected}")


def read_response(serial_port, timeout_s: float, idle_s: float) -> str:
    deadline = time.monotonic() + timeout_s
    last_rx = time.monotonic()
    chunks: list[bytes] = []
    while time.monotonic() < deadline:
        waiting = getattr(serial_port, "in_waiting", 0)
        data = serial_port.read(waiting or 1)
        if data:
            chunks.append(data)
            last_rx = time.monotonic()
            continue
        if chunks and (time.monotonic() - last_rx) >= idle_s:
            break
    return b"".join(chunks).decode("utf-8", errors="replace")


def run_serial(args: argparse.Namespace) -> int:
    try:
        import serial  # type: ignore
    except ImportError:
        print("pyserial is required for hardware runs; use --dry-run without it")
        return 2

    with serial.Serial(args.port, args.baud, timeout=0.05) as serial_port:
        time.sleep(args.boot_settle_s)
        serial_port.reset_input_buffer()
        failures = 0
        for step in STEPS:
            serial_port.write((step.command + "\n").encode("ascii"))
            serial_port.flush()
            output = read_response(serial_port, args.timeout_s, args.idle_s)
            verdict = classify_output(output, step.expected)
            print(f"[{verdict}] {step.command} ({step.label})")
            if args.verbose:
                print(clean_output(output).rstrip())
            if verdict != "PASS":
                failures += 1
        return 1 if failures else 0


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Run a bounded INA228 example-CLI HIL smoke sequence."
    )
    parser.add_argument("--port", help="Serial port for hardware run")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout-s", type=float, default=5.0,
                        help="Maximum seconds to wait per command")
    parser.add_argument("--idle-s", type=float, default=0.25,
                        help="Stop reading after this idle period once data arrived")
    parser.add_argument("--boot-settle-s", type=float, default=1.0,
                        help="Initial serial settle time before sending commands")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print the command plan without opening serial")
    parser.add_argument("--parser-self-test", action="store_true",
                        help="Run local output classifier tests")
    parser.add_argument("--verbose", action="store_true",
                        help="Print command responses during hardware run")
    args = parser.parse_args(argv)

    if args.timeout_s <= 0.0 or args.idle_s <= 0.0 or args.boot_settle_s < 0.0:
        parser.error("timeouts must be positive and boot settle must be nonnegative")

    if args.parser_self_test:
        return parser_self_test()
    if args.dry_run:
        print_plan(STEPS)
        return 0
    if not args.port:
        parser.error("--port is required unless --dry-run or --parser-self-test is used")
    return run_serial(args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
