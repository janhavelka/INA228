#!/usr/bin/env python3
"""Regression tests for the INA228 serial HIL parser and verdict contract."""

from __future__ import annotations

import importlib.util
import io
import pathlib
import sys
import types
from contextlib import redirect_stdout


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
        self.writes: list[str] = []

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
        self.writes.append(command)
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


class FakeUnframedSerial(FakeFramedSerial):
    def write(self, data: bytes) -> int:
        self.writes.append(data.decode("ascii"))
        self.buffer.extend((self.payload.rstrip() + "\n").encode("ascii"))
        return len(data)


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
        "INA228 library version: 9.9.9\n"
        "INA228 library commit: 0123456789ab (clean)\n"
    )
    assert_equal(
        runner.version_contract_errors(
            arduino, "arduino", "9.9.9", "0123456789ab", "clean"
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
                arduino, "arduino", "9.9.9", "deadbeefcafe", "clean"
            )
        ),
        "wrong firmware commit must fail",
    )
    dirty = arduino.replace("(clean)", "(dirty)")
    assert_true(
        bool(
            runner.version_contract_errors(
                dirty, "arduino", "9.9.9", "0123456789ab", "clean"
            )
        ),
        "dirty release firmware must fail",
    )

    idf = (
        "Runtime: native ESP-IDF v6.0.1\n"
        "INA228 library version: 9.9.9\n"
        "INA228 library commit: 0123456789ab (clean)\n"
    )
    assert_equal(
        runner.version_contract_errors(
            idf, "idf", "9.9.9", "0123456789ab", "clean"
        ),
        [],
        "native IDF provenance",
    )
    short_commit = arduino.replace("0123456789ab", "0123456")
    assert_true(
        bool(
            runner.version_contract_errors(
                short_commit, "arduino", "9.9.9", "0123456789ab", "clean"
            )
        ),
        "short firmware commit must fail",
    )


def test_stale_input_cannot_supply_framed_verdict() -> None:
    args = types.SimpleNamespace(
        drain_before_command_s=0.001,
        frame_prefix="TEST",
        no_command_framing=False,
        timeout_s=0.05,
        max_frame_bytes=4096,
        post_frame_drain_s=0.001,
        profile="arduino",
        expected_library_version="9.9.9",
        expected_commit="0123456789ab",
        expected_git_status="clean",
        framework_token=None,
    )
    step = runner.Step("version", ("INA228 library version:",), "provenance")
    stale = (
        b"Arduino-ESP32: 3.3.11\n"
        b"INA228 library version: 9.9.9\n"
        b"INA228 library commit: 0123456789ab (clean)\n"
    )
    result = runner.run_step(
        FakeFramedSerial(stale, "INA228 library version: 9.9.9"), step, args
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
        timeout_s=0.05,
        max_frame_bytes=4096,
        post_frame_drain_s=0.001,
        profile="arduino",
        expected_library_version="9.9.9",
        expected_commit="0123456789ab",
        expected_git_status="clean",
        framework_token=None,
    )
    payload = (
        "Arduino-ESP32: 3.3.11\n"
        "ESP-IDF: v5.5.5\n"
        "INA228 library version: 9.9.9\n"
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
    _, _, complete = runner.strip_hilrun_frame(
        "HIL_END token=T1 seq=7 status=OK elapsed_ms=3\n"
        "HIL_BEGIN token=T1 seq=7\n",
        "T1",
        "7",
    )
    assert_true(not complete, "stale end before begin accepted")


def test_require_framed_reaches_run_step_missing_frame_verdict() -> None:
    args = types.SimpleNamespace(
        drain_before_command_s=0.0,
        frame_prefix="TEST",
        no_command_framing=False,
        timeout_s=0.01,
        max_frame_bytes=4096,
        post_frame_drain_s=0.0,
        profile="arduino",
        expected_library_version="any",
        expected_commit="any",
        expected_git_status="any",
        framework_token=None,
        require_framed=False,
    )
    step = runner.Step("vbus", ("Vbus",), "sample")
    optional = runner.run_step(FakeUnframedSerial(b"", "Vbus: 12.0 V"), step, args)
    assert_equal(optional.verdict, "UNKNOWN", "optional missing frame")

    args.require_framed = True
    required = runner.run_step(FakeUnframedSerial(b"", "Vbus: 12.0 V"), step, args)
    assert_equal(required.verdict, "FAIL", "required missing frame")


def test_local_modes_skip_git_provenance_and_keep_report_plan_parity() -> None:
    original_git_text = runner.git_text
    runner.git_text = lambda _args: "unavailable"
    try:
        output = io.StringIO()
        with redirect_stdout(output):
            assert_equal(
                runner.main(["--dry-run", "--suite", "smoke", "--report", "report.md"]),
                0,
                "source-export dry run",
            )
        assert_true("<not-run rows>" in output.getvalue(), "report NOT RUN plan row")

        with redirect_stdout(io.StringIO()):
            assert_equal(runner.main(["--parser-self-test"]), 0,
                         "source-export parser self-test")
    finally:
        runner.git_text = original_git_text


def test_repeated_clock_tokens_still_get_unique_sequences() -> None:
    args = types.SimpleNamespace(
        drain_before_command_s=0.0,
        frame_prefix="TEST",
        no_command_framing=False,
        timeout_s=0.05,
        max_frame_bytes=4096,
        post_frame_drain_s=0.0,
        profile="arduino",
        expected_library_version="any",
        expected_commit="any",
        expected_git_status="any",
        framework_token=None,
    )
    serial = FakeFramedSerial(b"", "Vbus: 12.0 V")
    step = runner.Step("vbus", ("Vbus",), "sample")
    original_monotonic_ns = runner.time.monotonic_ns
    runner.time.monotonic_ns = lambda: 123456789
    try:
        assert_equal(runner.run_step(serial, step, args).verdict, "PASS", "first frame")
        assert_equal(runner.run_step(serial, step, args).verdict, "PASS", "second frame")
    finally:
        runner.time.monotonic_ns = original_monotonic_ns
    first = serial.writes[0].split(" ", 3)
    second = serial.writes[1].split(" ", 3)
    assert_equal(first[1], second[1], "frozen clock should repeat token")
    assert_equal(first[2], "0", "first sequence")
    assert_equal(second[2], "1", "second sequence")


def test_interrupted_compressed_soak_preserves_unstored_summary() -> None:
    args = types.SimpleNamespace(
        soak_store_every=10,
        soak_progress_every=100,
        verbose=False,
        stop_on_non_pass=False,
        command_pause_s=0.0,
    )
    results = []
    summary = runner.SoakSummary()
    calls = 0
    original_run_step = runner.run_step
    original_monotonic = runner.time.monotonic

    def fake_run_step(serial_port, step, run_args):
        nonlocal calls
        del serial_port, run_args
        calls += 1
        if calls == 4:
            raise OSError("simulated serial loss")
        return runner.Result(step, "PASS", 0.01, "ok")

    runner.run_step = fake_run_step
    runner.time.monotonic = lambda: 0.0
    try:
        try:
            runner.run_soak(None, args, results, 10.0, summary)
            raise AssertionError("interrupted soak did not propagate serial failure")
        except OSError:
            pass
    finally:
        runner.run_step = original_run_step
        runner.time.monotonic = original_monotonic

    assert_equal(summary.total, 3, "completed unstored soak commands")
    assert_equal(summary.pass_count, 3, "unstored soak PASS count")
    assert_equal(len(results), 0, "compressed result rows")


def test_health_capture_retains_errors_and_requires_complete_evidence() -> None:
    args = types.SimpleNamespace(
        drain_before_command_s=0.0, frame_prefix="TEST", no_command_framing=False,
        timeout_s=0.01, max_frame_bytes=4096, post_frame_drain_s=0.0,
        require_framed=True, health_after_command=True,
    )
    health = (
        "=== Driver Health ===\nState: DEGRADED\nOnline: true\n"
        "Consecutive failures: 1\nTotal success: 4\nTotal failures: 1\n"
        "Error code: I2C_NACK_ADDR\nError detail: 2\n"
    )

    class HealthSerial(FakeFramedSerial):
        def __init__(self, response: str, snapshot: str | None):
            super().__init__(b"", response)
            self.response = response
            self.snapshot = snapshot

        def write(self, data: bytes) -> int:
            is_health = data.decode("ascii").split(" ", 3)[3].strip() == "drv"
            if is_health and self.snapshot is None:
                raise OSError("test serial loss during health capture")
            self.payload = self.snapshot if is_health else self.response
            return super().write(data)

    for response, snapshot, expected in (
        ("Vbus: 12 V", health, "PASS"),
        ("Status: I2C_TIMEOUT", health, "FAIL"),
        ("Vbus: 12 V", "=== Driver Health ===\nState: READY", "FAIL"),
        ("Status: I2C_TIMEOUT", None, "FAIL"),
    ):
        serial = HealthSerial(response, snapshot)
        result = runner.run_step(serial, runner.Step("vbus", ("Vbus",), "sample"), args)
        assert_equal(result.verdict, expected, "health capture verdict")
        assert_equal(len(serial.writes), 2 if snapshot is not None else 1,
                     "one bounded health capture")
        assert_true(response in result.output, "original command retained")
        assert_true("command_utc=" in result.output, "UTC timestamp retained")
        if snapshot is not None:
            assert_true(snapshot.strip() in result.output, "cached error history retained")


def test_health_capture_does_not_follow_lost_framing_or_reset() -> None:
    args = types.SimpleNamespace(
        drain_before_command_s=0.0, frame_prefix="TEST", no_command_framing=False,
        timeout_s=0.01, max_frame_bytes=4096, post_frame_drain_s=0.0,
        require_framed=True, health_after_command=True,
    )
    for serial in (
        FakeUnframedSerial(b"", "Vbus: 12 V"),
        FakeFramedSerial(b"", "Vbus: 12 V\nESP-ROM:esp32s3"),
    ):
        result = runner.run_step(serial, runner.Step("vbus", ("Vbus",), "sample"), args)
        assert_equal(result.verdict, "FAIL", "lost framing/reset fails command")
        assert_true(result.framing_lost, "framing loss must stop the caller")
        assert_equal(len(serial.writes), 1, "no health command after framing loss/reset")
        assert_true("health NOT RUN" in result.output, "skipped capture reason retained")


def test_aborted_fixed_plan_preserves_failure_and_skips_dependent_phases() -> None:
    class FakePort:
        def open(self):
            pass

        def __enter__(self):
            return self

        def __exit__(self, *_args):
            pass

    args = types.SimpleNamespace(
        suite="smoke", soak_seconds=1.0, soak_hours=0.0, transcript=None,
        report=None, port="TEST", baud=115200, boot_settle_s=0.0,
        boot_capture_s=0.0, idle_s=0.001, prompt_token=None, verbose=False,
        command_pause_s=0.0, benchmark_count=1, include_not_run=False,
        fail_on_unknown=True,
    )
    original_serial = sys.modules.get("serial")
    originals = {name: getattr(runner, name) for name in
                 ("selected_steps", "read_response", "run_step", "run_benchmarks", "run_soak")}
    sys.modules["serial"] = types.SimpleNamespace(Serial=FakePort, SerialException=OSError)
    steps = (runner.Step("vbus", ("Vbus",), "first"),
             runner.Step("current", ("Current",), "second"))
    runner.selected_steps = lambda _suite: steps
    runner.read_response = lambda *_args: ""

    def forbidden_phase(*_args):
        raise AssertionError("dependent hardware phase executed after abort")

    runner.run_benchmarks = forbidden_phase
    runner.run_soak = forbidden_phase
    try:
        for stop_flag, framing_lost in ((True, False), (False, True)):
            args.stop_on_non_pass = stop_flag
            calls = []

            def failed_step(_serial, step, _args):
                calls.append(step.command)
                return runner.Result(step, "FAIL", 0.01,
                                     "Status: I2C_TIMEOUT original evidence", framing_lost)

            runner.run_step = failed_step
            output = io.StringIO()
            with redirect_stdout(output):
                assert_equal(runner.run_serial(args), 1, "failed run exit")
            assert_equal(calls, ["vbus"], "remaining fixed commands skipped")
            assert_true("I2C_TIMEOUT original evidence" in output.getvalue(),
                        "root failure retained in live output without verbose")
            assert_true("dependent phases NOT RUN" in output.getvalue(),
                        "aborted dependencies are explicit")
    finally:
        for name, value in originals.items():
            setattr(runner, name, value)
        if original_serial is None:
            del sys.modules["serial"]
        else:
            sys.modules["serial"] = original_serial


def main() -> int:
    tests = (
        test_expected_rejection_is_fail_closed,
        test_failure_runs_reset_on_every_non_failure,
        test_executed_command_count_excludes_not_run_rows,
        test_framework_and_provenance_contract,
        test_stale_input_cannot_supply_framed_verdict,
        test_post_frame_trailer_cannot_hide_failure,
        test_frame_identity_and_completion_are_exact,
        test_require_framed_reaches_run_step_missing_frame_verdict,
        test_local_modes_skip_git_provenance_and_keep_report_plan_parity,
        test_repeated_clock_tokens_still_get_unique_sequences,
        test_interrupted_compressed_soak_preserves_unstored_summary,
        test_health_capture_retains_errors_and_requires_complete_evidence,
        test_health_capture_does_not_follow_lost_framing_or_reset,
        test_aborted_fixed_plan_preserves_failure_and_skips_dependent_phases,
    )
    for test in tests:
        test()
    print(f"HIL parser regression tests PASSED ({len(tests)} groups)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
