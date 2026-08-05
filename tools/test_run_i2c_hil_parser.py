#!/usr/bin/env python3
"""Regression tests for the INA228 serial HIL parser and verdict contract."""

from __future__ import annotations

import importlib.util
import pathlib
import sys
import types


ROOT = pathlib.Path(__file__).resolve().parents[1]
RUNNER_PATH = ROOT / "tools" / "run_i2c_hil.py"


def load_runner():
    spec = importlib.util.spec_from_file_location("ina228_hil_runner", RUNNER_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError("unable to load HIL runner")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


runner = load_runner()


class FakeFramedSerial:
    def __init__(
        self, stale: bytes, payload: str, trailer: bytes = b"", inline_trailer: bool = False
    ) -> None:
        self.buffer = bytearray(stale)
        self.payload = payload
        self.trailer = trailer
        self.inline_trailer = inline_trailer
        self.pending_trailer = b""

    @property
    def in_waiting(self) -> int:
        return len(self.buffer)

    def read(self, size: int) -> bytes:
        size = min(size, len(self.buffer))
        result = bytes(self.buffer[:size])
        del self.buffer[:size]
        if not self.buffer and self.pending_trailer:
            self.buffer.extend(self.pending_trailer)
            self.pending_trailer = b""
        return result

    def write(self, data: bytes) -> int:
        command = data.decode("ascii")
        fields = command.split(" ", 3)
        token, seq = fields[1], fields[2]
        response = (
            f"HIL_BEGIN token={token} seq={seq}\n"
            f"{self.payload.rstrip()}\n"
            f"HIL_END token={token} seq={seq} status=OK elapsed_ms=1\n"
        )
        if self.inline_trailer:
            response += self.trailer.decode("ascii")
        self.buffer.extend(response.encode("ascii"))
        self.pending_trailer = b"" if self.inline_trailer else self.trailer
        return len(data)

    def flush(self) -> None:
        return None


def assert_equal(actual, expected, label: str) -> None:
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected!r}, got {actual!r}")


def assert_true(value: bool, label: str) -> None:
    if not value:
        raise AssertionError(label)


def test_expected_rejection_is_fail_closed() -> None:
    step = runner.Step(
        "mode bad", ("INVALID_PARAM",), "negative command", expect_failure=True
    )
    assert_equal(
        runner.classify_step("Status: INVALID_PARAM\n", step),
        "PASS",
        "expected rejection",
    )
    assert_equal(
        runner.classify_step(
            "Status: INVALID_PARAM\nStatus: I2C_TIMEOUT\n", step
        ),
        "FAIL",
        "expected rejection plus transport failure",
    )
    assert_equal(
        runner.classify_step("Status: INVALID_PARAM\n[FAIL] unrelated\n", step),
        "FAIL",
        "expected rejection plus explicit failure",
    )


def test_failure_runs_reset_on_every_non_failure() -> None:
    step = runner.Step("vbus", ("Vbus",), "sample")
    results = (
        runner.Result(step, "FAIL", 0.0, ""),
        runner.Result(step, "UNKNOWN", 0.0, ""),
        runner.Result(step, "FAIL", 0.0, ""),
        runner.Result(step, "FAIL", 0.0, ""),
        runner.Result(step, "NOT RUN", 0.0, ""),
        runner.Result(step, "FAIL", 0.0, ""),
    )
    assert_equal(runner.max_consecutive_failures(results), 2, "failure run")

    summary = runner.SoakSummary()
    soak_step = runner.Step("vbus", ("Vbus",), "sample", "soak")
    for verdict in ("FAIL", "PASS", "FAIL", "FAIL"):
        summary.record(runner.Result(soak_step, verdict, 0.0, ""))
    stored = (
        runner.Result(soak_step, "FAIL", 0.0, ""),
        runner.Result(soak_step, "FAIL", 0.0, ""),
        runner.Result(soak_step, "FAIL", 0.0, ""),
    )
    assert_equal(
        runner.actual_max_consecutive_failures(stored, summary),
        2,
        "unstored soak PASS resets failure run",
    )


def test_executed_command_count_excludes_not_run_rows() -> None:
    regular_step = runner.Step("vbus", ("Vbus",), "sample")
    soak_step = runner.Step("vbus", ("Vbus",), "sample", "soak")
    skipped_step = runner.Step("<fault injection>", (), "not available", "not-run")
    results = (
        runner.Result(regular_step, "PASS", 0.0, ""),
        runner.Result(soak_step, "PASS", 0.0, ""),
        runner.Result(skipped_step, "NOT RUN", 0.0, ""),
    )
    assert_equal(runner.actual_command_count(results, None), 2, "stored command count")

    summary = runner.SoakSummary()
    for _ in range(4):
        summary.record(runner.Result(soak_step, "PASS", 0.0, ""))
    assert_equal(runner.actual_command_count(results, summary), 5, "expanded soak count")


def test_framework_and_provenance_contract() -> None:
    arduino = (
        "Arduino-ESP32: 3.3.11\n"
        "ESP-IDF: v5.5.5\n"
        "INA228 library version: 3.0.2\n"
        "INA228 library commit: 0123456789ab (clean)\n"
    )
    assert_equal(
        runner.version_contract_errors(
            arduino, "arduino", "3.0.2", "0123456789ab", "clean"
        ),
        [],
        "Arduino provenance",
    )
    assert_true(
        bool(
            runner.version_contract_errors(
                arduino, "arduino", "3.0.1", "0123456789ab", "clean"
            )
        ),
        "wrong library version must fail",
    )
    assert_true(
        bool(
            runner.version_contract_errors(
                arduino, "arduino", "3.0.2", "deadbeefcafe", "clean"
            )
        ),
        "wrong firmware commit must fail",
    )
    dirty = arduino.replace("(clean)", "(dirty)")
    assert_true(
        bool(
            runner.version_contract_errors(
                dirty, "arduino", "3.0.2", "0123456789ab", "clean"
            )
        ),
        "dirty release firmware must fail",
    )

    idf = (
        "Runtime: native ESP-IDF v6.0.1\n"
        "INA228 library version: 3.0.2\n"
        "INA228 library commit: 0123456789ab (clean)\n"
    )
    assert_equal(
        runner.version_contract_errors(
            idf, "idf", "3.0.2", "0123456789ab", "clean"
        ),
        [],
        "native IDF provenance",
    )
    short_commit = arduino.replace("0123456789ab", "0123456")
    assert_true(
        bool(
            runner.version_contract_errors(
                short_commit, "arduino", "3.0.2", "0123456789ab", "clean"
            )
        ),
        "short firmware commit must fail",
    )


def test_stale_input_cannot_supply_framed_verdict() -> None:
    args = types.SimpleNamespace(
        drain_before_command_s=0.001,
        frame_prefix="TEST",
        no_command_framing=False,
        legacy_marker=False,
        timeout_s=0.05,
        max_frame_bytes=4096,
        post_frame_drain_s=0.001,
        profile="arduino",
        expected_library_version="3.0.2",
        expected_commit="0123456789ab",
        expected_git_status="clean",
        framework_token=None,
    )
    step = runner.Step("version", ("INA228 library version:",), "provenance")
    stale = (
        b"Arduino-ESP32: 3.3.11\n"
        b"INA228 library version: 3.0.2\n"
        b"INA228 library commit: 0123456789ab (clean)\n"
    )
    result = runner.run_step(
        FakeFramedSerial(stale, "INA228 library version: 3.0.2"), step, args
    )
    if result.verdict != "FAIL":
        raise AssertionError(
            f"stale framed classification: {result.verdict!r}\n{result.output}"
        )
    assert_true("drained stale serial input" in result.output, "stale transcript retained")


def test_post_frame_trailer_cannot_hide_failure() -> None:
    args = types.SimpleNamespace(
        drain_before_command_s=0.0,
        frame_prefix="TEST",
        no_command_framing=False,
        legacy_marker=False,
        timeout_s=0.05,
        max_frame_bytes=4096,
        post_frame_drain_s=0.001,
        profile="arduino",
        expected_library_version="3.0.2",
        expected_commit="0123456789ab",
        expected_git_status="clean",
        framework_token=None,
    )
    payload = (
        "Arduino-ESP32: 3.3.11\n"
        "ESP-IDF: v5.5.5\n"
        "INA228 library version: 3.0.2\n"
        "INA228 library commit: 0123456789ab (clean)"
    )
    step = runner.Step("version", ("INA228 library version:",), "provenance")
    failed = runner.run_step(
        FakeFramedSerial(
            b"", payload, b"Status: I2C_TIMEOUT\n", inline_trailer=True
        ),
        step,
        args,
    )
    assert_equal(failed.verdict, "FAIL", "post-frame failure")
    assert_true("drained trailing serial input" in failed.output, "trailer retained")

    unexpected = runner.run_step(
        FakeFramedSerial(b"", payload, b"unexpected reset banner\n"), step, args
    )
    assert_equal(unexpected.verdict, "UNKNOWN", "unexpected post-frame text")


def test_frame_identity_and_completion_are_exact() -> None:
    text = (
        "noise\nHIL_BEGIN token=T1 seq=7\nStatus: OK\n"
        "HIL_END token=T1 seq=7 status=OK elapsed_ms=3\n"
    )
    payload, trailer, complete = runner.strip_hilrun_frame(text, "T1", "7")
    assert_true(complete, "complete frame rejected")
    assert_true("frame_status=OK" in payload, "frame status missing")
    assert_equal(trailer, "", "unexpected complete-frame trailer")
    _, _, complete = runner.strip_hilrun_frame(text, "T2", "7")
    assert_true(not complete, "wrong token accepted")
    _, _, complete = runner.strip_hilrun_frame(text, "T1", "8")
    assert_true(not complete, "wrong sequence accepted")
    _, _, complete = runner.strip_hilrun_frame(
        "HIL_BEGIN token=T1 seq=7\n", "T1", "7"
    )
    assert_true(not complete, "truncated frame accepted")


def main() -> int:
    tests = (
        test_expected_rejection_is_fail_closed,
        test_failure_runs_reset_on_every_non_failure,
        test_executed_command_count_excludes_not_run_rows,
        test_framework_and_provenance_contract,
        test_stale_input_cannot_supply_framed_verdict,
        test_post_frame_trailer_cannot_hide_failure,
        test_frame_identity_and_completion_are_exact,
    )
    for test in tests:
        test()
    print(f"HIL parser regression tests PASSED ({len(tests)} groups)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
