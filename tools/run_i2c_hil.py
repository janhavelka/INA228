#!/usr/bin/env python3
"""Bounded INA228 example-CLI hardware-in-loop runner."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import platform
import pathlib
import re
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from typing import Iterable, Sequence


ROOT = pathlib.Path(__file__).resolve().parents[1]
ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")
PROMPT_RE = re.compile(r"(?m)^>\s*$")


@dataclass(frozen=True)
class Step:
    command: str
    expected: tuple[str, ...]
    label: str
    suite: str = "smoke"
    expect_failure: bool = False
    pause_after_s: float = 0.0


@dataclass
class Result:
    step: Step
    verdict: str
    elapsed_s: float
    output: str


@dataclass
class SoakSummary:
    total: int = 0
    pass_count: int = 0
    fail_count: int = 0
    unknown_count: int = 0
    total_latency_s: float = 0.0
    min_latency_s: float = 0.0
    max_latency_s: float = 0.0
    command_counts: dict[str, int] = field(default_factory=dict)
    command_failures: dict[str, int] = field(default_factory=dict)
    leading_failure_count: int = 0
    current_failure_run: int = 0
    max_failure_run: int = 0

    def record(self, result: Result) -> None:
        self.total += 1
        if result.verdict == "PASS":
            self.pass_count += 1
        elif result.verdict == "FAIL":
            self.fail_count += 1
        elif result.verdict == "UNKNOWN":
            self.unknown_count += 1
        if result.verdict == "FAIL":
            self.current_failure_run += 1
            self.max_failure_run = max(self.max_failure_run, self.current_failure_run)
            if self.leading_failure_count == self.total - 1:
                self.leading_failure_count += 1
        else:
            self.current_failure_run = 0
        self.command_counts[result.step.command] = (
            self.command_counts.get(result.step.command, 0) + 1
        )
        if result.verdict != "PASS":
            self.command_failures[result.step.command] = (
                self.command_failures.get(result.step.command, 0) + 1
            )
        self.total_latency_s += result.elapsed_s
        if self.total == 1:
            self.min_latency_s = result.elapsed_s
            self.max_latency_s = result.elapsed_s
        else:
            self.min_latency_s = min(self.min_latency_s, result.elapsed_s)
            self.max_latency_s = max(self.max_latency_s, result.elapsed_s)

    def counts(self) -> dict[str, int]:
        return {
            "PASS": self.pass_count,
            "FAIL": self.fail_count,
            "UNKNOWN": self.unknown_count,
            "NOT RUN": 0,
        }

    def mean_latency_s(self) -> float:
        if self.total == 0:
            return 0.0
        return self.total_latency_s / self.total


SMOKE_STEPS: tuple[Step, ...] = (
    Step("version", ("INA228 library version", "INA228 library commit"),
         "version, framework stack, and firmware provenance"),
    Step("scan", ("INA228 Address Probe", "Healthy INA228 devices"), "scan"),
    Step("init", ("initialize", "OK"), "initialize discovered INA228"),
    Step("probe", ("Status: OK",), "probe"),
    Step("settings", ("Active Settings", "State:", "Address:"), "settings"),
    Step("drv", ("Driver Health", "State:", "Online:"), "health"),
    Step("diagraw", ("DIAG_ALRT raw",), "DIAG_ALRT"),
    Step("raw", ("Raw Registers", "Vbus", "Temp"), "conversion raw read"),
)


FUNCTIONAL_STEPS: tuple[Step, ...] = (
    Step("help", ("INA228 CLI Help", "read", "raw"), "help", "functional"),
    Step("scanina", ("INA228 Address Probe", "Healthy INA228 devices"), "INA scan", "functional"),
    Step("mfgid", ("Manufacturer ID",), "manufacturer ID", "functional"),
    Step("devid", ("Device ID",), "device ID", "functional"),
    Step("timing", ("Conversion ready", "Estimated conversion time"), "timing", "functional"),
    Step("vbus", ("Vbus",), "bus voltage", "functional"),
    Step("vshunt", ("Vshunt",), "shunt voltage", "functional"),
    Step("temp", ("Temp",), "die temperature", "functional"),
    Step("current", ("Current",), "current", "functional"),
    Step("power", ("Power",), "power", "functional"),
    Step("rstacc", ("accumulator reset", "OK"),
         "establish valid accumulator epoch", "functional"),
    Step("energy", ("Energy",), "energy", "functional"),
    Step("charge", ("Charge",), "charge", "functional"),
    Step("read", ("Vbus", "Power", "Accum"), "aggregate read", "functional"),
    Step("diag", ("DIAG_ALRT Flags", "MEMSTAT"), "parsed diagnostics", "functional"),
    Step("limits", ("Alert Limits",), "alert limits", "functional"),
    Step("alatch", ("Alert latch",), "alert latch query", "functional"),
    Step("cnvralert", ("Conversion-ready alert",), "conversion alert query", "functional"),
    Step("alslow", ("Slow alert",), "slow alert query", "functional"),
    Step("apol", ("Alert polarity",), "alert polarity query", "functional"),
    Step("mode", ("Mode",), "mode query", "functional"),
    Step("convtime", ("Conversion times",), "conversion time query", "functional"),
    Step("averaging", ("Averaging",), "averaging query", "functional"),
    Step("adcrange", ("ADC range",), "ADC range query", "functional"),
    Step("cal", ("CURRENT_LSB",), "calibration query", "functional"),
    Step("tempco", ("Shunt temp coeff",), "temperature coefficient query", "functional"),
    Step("tempcomp", ("Temperature compensation",), "temperature compensation query", "functional"),
    Step("delay", ("Conversion delay",), "conversion delay query", "functional"),
    Step("ready", ("Conversion ready",), "readiness", "functional"),
    Step("reg16 0x3E", ("0x",), "manufacturer register raw16", "functional"),
    Step("reg16 0x3F", ("0x",), "device register raw16", "functional"),
    Step("reg24 0x05", ("0x",), "VBUS raw24", "functional"),
    Step("reg40 0x09", ("0x",), "ENERGY raw40", "functional"),
    Step("recover", ("Invalidating cached hardware state", "Status: OK"),
         "owner invalidation and verified reinitialization", "functional"),
    Step("selftest", ("INA228 selftest",), "self-test", "functional"),
    Step("stress 1000", ("Stress Summary", "Errors: 0"),
         "1000-sample measurement stress", "functional"),
    Step("stress_mix 1000", ("stress_mix summary", "fail=0"),
         "1000-operation mixed API stress", "functional"),
)


def feature_sweep_steps() -> tuple[Step, ...]:
    """Synchronous feature sweep compatible with the v3 owner contract."""
    steps: list[Step] = [
        Step("verbose 0", ("Verbose mode",), "reduce CLI chatter", "targeted"),
        Step("help", ("mode [0..15]", "sample_step <budget>", "limits"),
             "targeted CLI surface check", "targeted"),
        Step("drv", ("Driver Health", "State:", "Online:"),
             "initial health before mutation", "targeted"),
        Step("settings", ("Active Settings", "Mode:", "ADC range:"),
             "initial settings snapshot", "targeted"),
    ]

    steps.extend(
        Step(f"mode {mode}", ("setMode",), f"set operating mode {mode}", "targeted")
        for mode in range(16)
    )
    steps.extend([
        Step("mode", ("Mode:",), "query mode after full mode sweep", "targeted"),
        Step("mode 15", ("setMode", "OK"), "restore continuous-all mode", "targeted"),
    ])

    for channel in ("vbus", "vsh", "temp"):
        steps.extend(
            Step(f"convtime {channel} {index}", ("setConvTime",),
                 f"set {channel} conversion time index {index}", "targeted")
            for index in range(8)
        )
    steps.extend([
        Step("convtime", ("Conversion times",), "query conversion times", "targeted"),
        Step("convtime vbus 5", ("setConvTime", "OK"),
             "restore VBUS conversion time", "targeted"),
        Step("convtime vsh 5", ("setConvTime", "OK"),
             "restore VSHUNT conversion time", "targeted"),
        Step("convtime temp 5", ("setConvTime", "OK"),
             "restore TEMP conversion time", "targeted"),
    ])

    steps.extend(
        Step(f"averaging {index}", ("setAveraging",),
             f"set averaging index {index}", "targeted")
        for index in range(8)
    )
    steps.extend([
        Step("averaging", ("Averaging:",), "query averaging", "targeted"),
        Step("averaging 0", ("setAveraging", "OK"), "restore averaging 1", "targeted"),
    ])

    steps.extend([
        Step("adcrange 1", ("setAdcRange", "INVALID_CONFIG"),
             "reject low range incompatible with the bound 10 A profile", "targeted",
             expect_failure=True),
        Step("integer", ("Cooperative Instantaneous Sample", "Shunt:"),
             "sample after rejected range mutation", "targeted"),
        Step("adcrange 0", ("setAdcRange", "OK"), "restore default shunt range", "targeted"),
        Step("adcrange", ("ADC range:",), "query ADC range", "targeted"),
        Step("delay 0", ("setConversionDelay", "OK"), "conversion delay min", "targeted"),
        Step("delay 1", ("setConversionDelay", "OK"), "conversion delay small", "targeted"),
        Step("delay 127", ("setConversionDelay", "OK"), "conversion delay middle", "targeted"),
        Step("delay 255", ("setConversionDelay", "OK"), "conversion delay max", "targeted"),
        Step("delay", ("Conversion delay:",), "query conversion delay", "targeted"),
        Step("delay 0", ("setConversionDelay", "OK"), "restore zero conversion delay", "targeted"),
        Step("tempco 1", ("setShuntTempCoeff", "OK"), "set small tempco", "targeted"),
        Step("tempco 16383", ("setShuntTempCoeff", "OK"), "set max tempco", "targeted"),
        Step("tempco", ("Shunt temp coeff:",), "query tempco", "targeted"),
        Step("tempco 0", ("setShuntTempCoeff", "OK"), "restore tempco", "targeted"),
        Step("tempcomp 1", ("setTempCompensation", "OK"), "enable temp compensation", "targeted"),
        Step("tempcomp", ("Temperature compensation:",), "query temp compensation", "targeted"),
        Step("tempcomp 0", ("setTempCompensation", "OK"), "restore temp compensation", "targeted"),
        Step("cal 0.015 10", ("setCalibration", "INVALID_CONFIG"),
             "reject mutation of the bound fixed-unit calibration contract", "targeted",
             expect_failure=True),
        Step("cal", ("Calibration:", "CURRENT_LSB"), "query calibration", "targeted"),
    ])

    steps.extend([
        Step("alatch 1", ("setAlertLatch", "OK"), "enable alert latch", "targeted"),
        Step("alatch 0", ("setAlertLatch", "OK"), "restore alert latch", "targeted"),
        Step("cnvralert 1", ("setConversionReadyAlert", "OK"),
             "enable conversion-ready alert bit", "targeted"),
        Step("cnvralert 0", ("setConversionReadyAlert", "OK"),
             "restore conversion-ready alert bit", "targeted"),
        Step("alslow 1", ("setSlowAlert", "OK"), "enable slow alert", "targeted"),
        Step("alslow 0", ("setSlowAlert", "OK"), "restore slow alert", "targeted"),
        Step("apol 1", ("setAlertPolarity", "OK"), "set active-high alert", "targeted"),
        Step("apol 0", ("setAlertPolarity", "OK"), "restore active-low alert", "targeted"),
        Step("sovl 0.001", ("setShuntOvervoltageThreshold", "OK"),
             "safe shunt overvoltage threshold", "targeted"),
        Step("suvl -0.001", ("setShuntUndervoltageThreshold", "OK"),
             "safe shunt undervoltage threshold", "targeted"),
        Step("bovl 1.0", ("setBusOvervoltageThreshold", "OK"),
             "safe bus overvoltage threshold", "targeted"),
        Step("buvl 0.1", ("setBusUndervoltageThreshold", "OK"),
             "safe bus undervoltage threshold", "targeted"),
        Step("tmplim 100", ("setTemperatureOverlimitThreshold", "OK"),
             "safe temperature threshold", "targeted"),
        Step("pwrlim 0.01", ("setPowerOverlimitThreshold", "OK"),
             "safe power threshold", "targeted"),
        Step("limits", ("Alert Limits",), "query alert limits", "targeted"),
        Step("sovl 0.163835", ("setShuntOvervoltageThreshold", "OK"),
             "restore shunt overvoltage default", "targeted"),
        Step("suvl -0.16384", ("setShuntUndervoltageThreshold", "OK"),
             "restore shunt undervoltage default", "targeted"),
        Step("bovl 85", ("setBusOvervoltageThreshold", "OK"),
             "restore safe max bus overvoltage", "targeted"),
        Step("buvl 0", ("setBusUndervoltageThreshold", "OK"),
             "restore bus undervoltage default", "targeted"),
        Step("tmplim 255.99", ("setTemperatureOverlimitThreshold", "OK"),
             "restore safe max temperature threshold", "targeted"),
        Step("pwrlim 800", ("setPowerOverlimitThreshold", "OK"),
             "restore power threshold", "targeted"),
    ])

    steps.extend([
        Step("ready_step 0", ("INVALID_PARAM",),
             "ready zero-budget rejection", "targeted", expect_failure=True),
        Step("ready_step 1", ("pollMeasurementReady",),
             "ready single-transfer budget", "targeted"),
    ])

    for trigger_mode in range(1, 8):
        steps.extend([
            Step(f"trigger {trigger_mode}", ("triggerConversion",),
                 f"trigger mode {trigger_mode}", "targeted", pause_after_s=0.02),
            Step("ready", ("Conversion ready",),
                 f"ready poll after trigger {trigger_mode}", "targeted"),
        ])

    steps.extend([
        Step("mode 15", ("setMode", "OK"), "restore continuous mode after triggers", "targeted"),
        Step("rstacc", ("accumulator reset", "OK"), "reset accumulators", "targeted"),
        Step("energy", ("Energy",), "read energy after accumulator reset", "targeted"),
        Step("charge", ("Charge",), "read charge after accumulator reset", "targeted"),
        Step("diagraw", ("DIAG_ALRT raw",), "raw diagnostics destructive read", "targeted"),
        Step("diagsnap", ("DIAG_ALRT Snapshot",), "cache-only diagnostic snapshot", "targeted"),
        Step("reg16 0x00", ("0x",), "read CONFIG raw16", "targeted"),
        Step("reg16 0x01", ("0x",), "read ADC_CONFIG raw16", "targeted"),
        Step("reg16 0x02", ("0x",), "read SHUNT_CAL raw16", "targeted"),
        Step("reg24 0x04", ("0x",), "read VSHUNT raw24", "targeted"),
        Step("reg24 0x05", ("0x",), "read VBUS raw24", "targeted"),
        Step("reg24 0x06", ("0x",), "read DIETEMP raw24", "targeted"),
        Step("reg24 0x07", ("0x",), "read CURRENT raw24", "targeted"),
        Step("reg24 0x08", ("0x",), "read POWER raw24", "targeted"),
        Step("reg40 0x09", ("0x",), "read ENERGY raw40", "targeted"),
        Step("reg40 0x0A", ("0x",), "read CHARGE raw40", "targeted"),
    ])

    steps.extend([
        Step("mode -1", ("Invalid mode", "INVALID_PARAM"),
             "reject invalid negative mode", "targeted", expect_failure=True),
        Step("mode 16", ("Invalid mode", "INVALID_PARAM"),
             "reject invalid high mode", "targeted", expect_failure=True),
        Step("mode nonsense", ("Invalid mode", "INVALID_PARAM"),
             "reject malformed mode token", "targeted", expect_failure=True),
        Step("trigger 0", ("Invalid trigger mode", "INVALID_PARAM"),
             "reject shutdown as a trigger mode", "targeted", expect_failure=True),
        Step("trigger 8", ("Invalid trigger mode", "INVALID_PARAM"),
             "reject invalid trigger mode", "targeted", expect_failure=True),
        Step("trigger nonsense", ("Invalid trigger mode", "INVALID_PARAM"),
             "reject malformed trigger token", "targeted", expect_failure=True),
        Step("convtime vbus 8", ("Invalid conversion time", "INVALID_PARAM"),
             "reject invalid conversion index", "targeted", expect_failure=True),
        Step("convtime bogus 1", ("Invalid target", "INVALID_PARAM"),
             "reject invalid conversion target", "targeted", expect_failure=True),
        Step("averaging 8", ("Invalid averaging", "INVALID_PARAM"),
             "reject invalid averaging", "targeted", expect_failure=True),
        Step("averaging nonsense", ("Invalid averaging", "INVALID_PARAM"),
             "reject malformed averaging token", "targeted", expect_failure=True),
        Step("adcrange 2", ("Invalid ADC range", "INVALID_PARAM"),
             "reject invalid ADC range", "targeted", expect_failure=True),
        Step("delay 256", ("Usage: delay", "INVALID_PARAM"),
             "reject invalid conversion delay", "targeted", expect_failure=True),
        Step("tempco 16384", ("Usage: tempco", "INVALID_PARAM"),
             "reject invalid tempco", "targeted", expect_failure=True),
        Step("tempcomp 2", ("Usage: tempcomp", "INVALID_PARAM"),
             "reject invalid temp compensation", "targeted", expect_failure=True),
        Step("tempcomp nonsense", ("Usage: tempcomp", "INVALID_PARAM"),
             "reject malformed boolean token", "targeted", expect_failure=True),
        Step("alatch 2", ("Usage: alatch", "INVALID_PARAM"),
             "reject invalid latch", "targeted", expect_failure=True),
        Step("cnvralert 2", ("Usage: cnvralert", "INVALID_PARAM"),
             "reject invalid conversion alert", "targeted", expect_failure=True),
        Step("alslow 2", ("Usage: alslow", "INVALID_PARAM"),
             "reject invalid slow alert", "targeted", expect_failure=True),
        Step("apol 2", ("Usage: apol", "INVALID_PARAM"),
             "reject invalid alert polarity", "targeted", expect_failure=True),
        Step("cal 0 10", ("Usage: cal", "INVALID_PARAM"),
             "reject zero shunt calibration", "targeted", expect_failure=True),
        Step("reg16 0x100", ("Usage: reg16", "INVALID_PARAM"),
             "reject invalid raw16 register", "targeted", expect_failure=True),
        Step("reg24 0x100", ("Usage: reg24", "INVALID_PARAM"),
             "reject invalid raw24 register", "targeted", expect_failure=True),
        Step("reg40 0x100", ("Usage: reg40", "INVALID_PARAM"),
             "reject invalid raw40 register", "targeted", expect_failure=True),
        Step("verbose nonsense", ("Usage: verbose", "INVALID_PARAM"),
             "reject malformed verbosity token", "targeted", expect_failure=True),
        Step("wreg16 0x00 0", ("Confirmation required", "INVALID_PARAM"),
             "reject unconfirmed raw register write", "targeted", expect_failure=True),
        Step("unknown_hil_command", ("Unknown command", "INVALID_PARAM"),
             "reject unknown command with framed status", "targeted", expect_failure=True),
        Step("init 0x50", ("Invalid address", "INVALID_PARAM"),
             "reject invalid init address", "targeted", expect_failure=True),
        Step("end", ("Device shut down",), "end driver", "targeted"),
        Step("vbus", ("NOT_INITIALIZED",),
             "read after end must fail visibly", "targeted", expect_failure=True),
        Step("init", ("initialize", "OK"), "reinitialize configured device", "targeted"),
        Step("recover", ("Invalidating cached hardware state", "Status: OK"),
             "manual recovery after reinit", "targeted"),
        Step("settings", ("Active Settings", "State:", "READY"), "final settings", "targeted"),
        Step("drv", ("Driver Health", "State: READY", "Consecutive failures: 0"),
             "final health must be clean", "targeted"),
        Step("read", ("Vbus", "Power"), "final aggregate read", "targeted"),
    ])

    return tuple(steps)


FEATURE_SWEEP_STEPS: tuple[Step, ...] = feature_sweep_steps()

SOAK_STEPS: tuple[Step, ...] = (
    Step("vbus", ("Vbus",), "soak bus voltage", "soak"),
    Step("vshunt", ("Vshunt",), "soak shunt voltage", "soak"),
    Step("temp", ("Temp",), "soak temperature", "soak"),
    Step("current", ("Current",), "soak current", "soak"),
    Step("power", ("Power",), "soak power", "soak"),
    Step("integer", ("Cooperative Instantaneous Sample",),
         "soak atomic sample", "soak"),
    Step("raw", ("Raw Registers", "Vbus"), "soak raw", "soak"),
    Step("read", ("Vbus", "Power"), "soak aggregate", "soak"),
    Step("ready", ("Conversion ready",), "soak readiness", "soak"),
    Step("settings", ("Active Settings",), "soak settings", "soak"),
    Step("drv", ("Driver Health",), "soak health", "soak"),
    Step("diagsnap", ("DIAG_ALRT Snapshot",), "soak diagnostic snapshot", "soak"),
    Step("diagraw", ("DIAG_ALRT raw",), "soak raw diagnostics", "soak"),
    Step("probe", ("Status: OK",), "soak probe", "soak"),
    Step("recover", ("Invalidating cached hardware state", "Status: OK"),
         "soak verified reinitialization", "soak"),
    Step("stress 50", ("Stress Summary", "Errors:"), "soak stress", "soak"),
    Step("stress_mix 50", ("stress_mix summary", "fail="), "soak mixed stress", "soak"),
)


BENCHMARK_STEPS: tuple[Step, ...] = (
    Step("vbus", ("Vbus",), "benchmark bus voltage", "benchmark"),
    Step("vshunt", ("Vshunt",), "benchmark shunt voltage", "benchmark"),
    Step("temp", ("Temp",), "benchmark temperature", "benchmark"),
    Step("raw", ("Raw Registers", "Vbus"), "benchmark raw sample", "benchmark"),
    Step("integer", ("Cooperative Instantaneous Sample",),
         "benchmark atomic sample", "benchmark"),
    Step("read", ("Vbus", "Power"), "benchmark aggregate", "benchmark"),
)


V3_TARGETED_STEPS: tuple[Step, ...] = (
    Step("help", ("sample_step <budget>", "reset_start", "apply_start"),
         "v3 cooperative CLI surface", "targeted"),
    Step("integer", ("Cooperative Instantaneous Sample", "Operation:", "Current:"),
         "bounded atomic sample", "targeted"),
    Step("sample_step 0", ("pollJob(0)", "IN_PROGRESS"),
         "zero-budget sample start", "targeted"),
    Step("sample_step 3", ("pollJob(3)", "IN_PROGRESS"),
         "sample verify/trigger budget", "targeted", pause_after_s=0.05),
    Step("sample_step 8", ("Cooperative Sample Result",),
         "sample wait/read/restore completion", "targeted"),
    Step("apply_start", ("startReinitialize", "OK"),
         "verified reinitialization start", "targeted"),
    Step("apply_step 0", ("pollJob(0)", "IN_PROGRESS"),
         "reinitialization zero budget", "targeted"),
    Step("apply_step 14", ("pollJob(14)", "OK", "terminal result consumed"),
         "verified reinitialization completion", "targeted"),
    Step("reset_start", ("startReset", "OK"),
         "maintenance reset start", "targeted"),
    Step("reset_step 1", ("pollJob(1)", "IN_PROGRESS"),
         "reset write", "targeted", pause_after_s=0.05),
    Step("reset_step 0", ("pollJob(0)", "IN_PROGRESS"),
         "reset wait zero budget", "targeted"),
    Step("reset_step 15", ("pollJob(15)", "OK", "terminal result consumed"),
         "reset verification and initialization", "targeted"),
    Step("recover", ("Invalidating cached hardware state", "Status: OK"),
         "application-owned recovery boundary", "targeted"),
    Step("diagsnap", ("DIAG_ALRT Snapshot", "cache-only"),
         "cache-only diagnostic evidence", "targeted"),
    Step("selftest", ("Selftest result", "fail=0"),
         "device self-test", "targeted"),
)


V3_TRANSFER_STEPS: tuple[Step, ...] = (
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("sample_step 0", ("pollJob(0)", "IN_PROGRESS"),
         "sample zero budget", "transfer"),
    Step("xfer_assert 0 0 0", ("XFER_ASSERT PASS",),
         "sample start is bus-silent", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("sample_step 1", ("pollJob(1)", "IN_PROGRESS"),
         "sample budget one", "transfer"),
    Step("xfer_assert 1 0 1", ("XFER_ASSERT PASS",),
         "sample budget-one count", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("sample_step 2", ("pollJob(2)", "IN_PROGRESS"),
         "sample calibration/trigger", "transfer", pause_after_s=0.05),
    Step("xfer_assert 1 1 2", ("XFER_ASSERT PASS",),
         "sample trigger count", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("sample_step 8", ("Cooperative Sample Result",),
         "sample completion", "transfer"),
    Step("xfer_assert 7 1 8", ("XFER_ASSERT PASS",),
         "sample completion count", "transfer"),
    Step("apply_start", ("startReinitialize", "OK"),
         "reinitialization start", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("apply_step 0", ("pollJob(0)", "IN_PROGRESS"),
         "reinitialization zero budget", "transfer"),
    Step("xfer_assert 0 0 0", ("XFER_ASSERT PASS",),
         "reinitialization zero count", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("apply_step 1", ("pollJob(1)", "IN_PROGRESS"),
         "reinitialization budget one", "transfer"),
    Step("xfer_assert 1 0 1", ("XFER_ASSERT PASS",),
         "reinitialization first read", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("apply_step 13", ("pollJob(13)", "OK"),
         "reinitialization completion", "transfer"),
    Step("xfer_assert 7 6 13", ("XFER_ASSERT PASS",),
         "reinitialization remaining count", "transfer"),
    Step("reset_start", ("startReset", "OK"), "reset start", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("reset_step 0", ("pollJob(0)", "IN_PROGRESS"),
         "reset zero budget", "transfer"),
    Step("xfer_assert 0 0 0", ("XFER_ASSERT PASS",),
         "reset zero count", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("reset_step 1", ("pollJob(1)", "IN_PROGRESS"),
         "reset write", "transfer", pause_after_s=0.05),
    Step("xfer_assert 0 1 1", ("XFER_ASSERT PASS",),
         "reset write count", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset counters", "transfer"),
    Step("reset_step 15", ("pollJob(15)", "OK"),
         "reset completion", "transfer"),
    Step("xfer_assert 9 6 15", ("XFER_ASSERT PASS",),
         "reset remaining count", "transfer"),
)


STATIC_NOT_RUN_STEPS: tuple[Step, ...] = (
    Step("<fixture: disconnected target>", ("safe absent-device fixture",),
         "requires safe disconnect or switched fixture", "not-run"),
    Step("<fixture: bus fault injection>", ("safe fault-injection fixture",),
         "requires safe NACK/timeout/bus-error injection", "not-run"),
    Step("<fixture: alert pin capture>", ("alert pin instrumentation",),
         "requires alert-pin wiring and safe threshold stimulus", "not-run"),
    Step("<fixture: MCU reset or power cycle>", ("controlled reset/power fixture",),
         "requires explicit reset/power-cycle control", "not-run"),
)


FAILURE_TOKENS: tuple[str, ...] = (
    "I2C_NACK_ADDR",
    "I2C_NACK_DATA",
    "I2C_NACK_UNKNOWN_PHASE",
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
    re.compile(r"\bSTATUS\s*:\s*(?!OK\b|IN_PROGRESS\b)[A-Z0-9_]+", re.IGNORECASE),
)



# `drv` reports lifetime counters and the last recorded error. Those lines
# describe history that has already been recovered from, so scanning them for
# failure evidence would make every later `drv` step FAIL once any transient
# error has ever occurred -- including every iteration of a long soak. The live
# signal, "Consecutive failures: N", is deliberately left in place.
HISTORICAL_HEALTH_RE = re.compile(
    r"^[ \t]*(?:Total failures|Last error|Error code|Error detail|Error msg)[ \t]*:.*$",
    re.MULTILINE,
)


def clean_output(text: str) -> str:
    return ANSI_RE.sub("", text).replace("\r\n", "\n").replace("\r", "\n")


def has_failure(text: str) -> bool:
    scanned = HISTORICAL_HEALTH_RE.sub("", text)
    upper = scanned.upper()
    return any(token in upper for token in FAILURE_TOKENS) or any(
        pattern.search(scanned) for pattern in FAILURE_PATTERNS
    )


def classify_output(output: str, expected: Sequence[str]) -> str:
    text = clean_output(output)
    if has_failure(text):
        return "FAIL"
    if all(token in text for token in expected):
        return "PASS"
    return "UNKNOWN"


def classify_step(output: str, step: Step) -> str:
    text = clean_output(output)
    expected_seen = all(token in text for token in step.expected)
    if step.expect_failure and expected_seen:
        unexpected_text = text
        for token in step.expected:
            unexpected_text = unexpected_text.replace(token, "")
        return "FAIL" if has_failure(unexpected_text) else "PASS"
    if has_failure(text):
        return "FAIL"
    if expected_seen:
        return "PASS"
    return "UNKNOWN"


def git_text(args: Sequence[str]) -> str:
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
    except OSError:
        return "unavailable"
    text = result.stdout.strip()
    return text if text else "none"


def configured_library_version() -> str:
    try:
        data = json.loads((ROOT / "library.json").read_text(encoding="utf-8"))
        return str(data["version"])
    except (OSError, KeyError, TypeError, ValueError):
        return "unknown"


DEFAULT_FRAMEWORK_TOKENS: dict[str, tuple[str, ...]] = {
    "arduino": ("Arduino-ESP32: 3.3.11", "ESP-IDF: v5.5.5"),
    "idf": ("Runtime: native ESP-IDF v6.0.1",),
}
VERSION_PATTERN = re.compile(r"(?m)^\s*INA228 library version:\s*(\S+)\s*$")
COMMIT_PATTERN = re.compile(
    r"(?m)^\s*INA228 library commit:\s*([0-9a-f]{12}|unknown)\s*"
    r"\((clean|dirty|unknown)\)\s*$",
    re.IGNORECASE,
)


def version_contract_errors(output: str, profile: str, expected_version: str,
                            expected_commit: str, expected_git_status: str,
                            framework_tokens: Sequence[str] | None = None) -> list[str]:
    text = clean_output(output)
    errors: list[str] = []
    required_framework = tuple(framework_tokens or DEFAULT_FRAMEWORK_TOKENS[profile])
    for token in required_framework:
        if token not in text:
            errors.append(f"missing framework token: {token}")

    version_match = VERSION_PATTERN.search(text)
    if version_match is None:
        errors.append("missing INA228 library version field")
    elif expected_version != "any" and version_match.group(1) != expected_version:
        errors.append(
            f"library version mismatch: firmware={version_match.group(1)} "
            f"expected={expected_version}"
        )

    commit_match = COMMIT_PATTERN.search(text)
    if commit_match is None:
        errors.append("missing INA228 commit/status field")
        return errors

    actual_commit = commit_match.group(1).lower()
    actual_status = commit_match.group(2).lower()
    normalized_expected = expected_commit.lower()
    if normalized_expected != "any":
        commit_matches = (
            actual_commit != "unknown"
            and actual_commit == normalized_expected
        )
        if not commit_matches:
            errors.append(
                f"firmware commit mismatch: firmware={actual_commit} "
                f"expected={normalized_expected}"
            )
    if expected_git_status != "any" and actual_status != expected_git_status:
        errors.append(
            f"firmware status mismatch: firmware={actual_status} "
            f"expected={expected_git_status}"
        )
    return errors


def selected_steps(suite: str) -> tuple[Step, ...]:
    if suite == "smoke":
        return SMOKE_STEPS
    if suite == "functional":
        return SMOKE_STEPS + FUNCTIONAL_STEPS
    if suite == "exhaustive":
        return (SMOKE_STEPS + FUNCTIONAL_STEPS + FEATURE_SWEEP_STEPS
                + V3_TARGETED_STEPS + V3_TRANSFER_STEPS)
    if suite == "targeted":
        return SMOKE_STEPS + FEATURE_SWEEP_STEPS + V3_TARGETED_STEPS
    if suite == "transfer":
        return SMOKE_STEPS + V3_TRANSFER_STEPS
    raise ValueError(f"unsupported suite: {suite}")


def parser_self_test() -> int:
    cases = (
        ("  Status: OK\n=== Driver Health ===\n  State: READY\n  Online: true\n",
         ("Driver Health", "State:", "Online:"), "PASS"),
        ("=== Driver Health ===\n  State: READY\n  Online: true\n  Last error: never\n",
         ("Driver Health", "State:", "Online:"), "PASS"),
        ("  Status: I2C_TIMEOUT (code=14, detail=-1)\n", ("Status: OK",), "FAIL"),
        ("  Status: IN_PROGRESS (code=12, detail=0)\n", ("IN_PROGRESS",), "PASS"),
        ("[FAIL] probe responds\n", ("Status: OK",), "FAIL"),
        ("=== selftest ===\n  pass=4 fail=1 skip=0\n", ("selftest",), "FAIL"),
        ("=== Stress Summary ===\n  Success: 10\n  Errors: 3\n", ("Stress Summary",), "FAIL"),
        ("=== Stress Summary ===\n  Success: 10\n  Errors: 0\n",
         ("Stress Summary", "Errors:"), "PASS"),
        ("=== INA228 Address Probe (0x40-0x4F) ===\n  Healthy INA228 devices: 0\n",
         ("INA228 Address Probe", "Healthy INA228 devices"), "PASS"),
        ("boot banner only\n", ("Active Settings",), "UNKNOWN"),
    )
    for output, expected, want in cases:
        got = classify_output(output, expected)
        if got != want:
            print(f"parser self-test FAILED: expected {want}, got {got}")
            return 1
    payload, _, ok = strip_hilrun_frame(
        "noise\nHIL_BEGIN token=T1 seq=7\nStatus: OK\nold HILMARK text\n"
        "HIL_END token=T1 seq=7 status=OK elapsed_ms=3\n",
        "T1",
        "7",
    )
    if not ok or "old HILMARK text" not in payload:
        print("parser self-test FAILED: complete frame")
        return 1
    _, _, ok = strip_hilrun_frame("HIL_BEGIN token=T1 seq=7\n", "T1", "7")
    if ok:
        print("parser self-test FAILED: truncated frame accepted")
        return 1
    _, _, ok = strip_hilrun_frame(
        "HIL_BEGIN token=T1 seq=7\nHIL_END token=T2 seq=7 status=OK elapsed_ms=3\n",
        "T1",
        "7",
    )
    if ok:
        print("parser self-test FAILED: wrong token accepted")
        return 1
    _, _, ok = strip_hilrun_frame(
        "HIL_BEGIN token=T1 seq=7\nHIL_END token=T1 seq=8 status=OK elapsed_ms=3\n",
        "T1",
        "7",
    )
    if ok:
        print("parser self-test FAILED: wrong sequence accepted")
        return 1
    payload, _, ok = strip_hilrun_frame(
        "HIL_BEGIN token=T1 seq=7\nHIL_END token=T1 seq=7 status=INVALID_PARAM elapsed_ms=0\n",
        "T1",
        "7",
    )
    invalid_step = Step("hilrun T1 7 hilrun nested", ("INVALID_PARAM",),
                        "nested hilrun rejection", expect_failure=True)
    if not ok or classify_step(payload, invalid_step) != "PASS":
        print("parser self-test FAILED: nested/invalid frame status")
        return 1
    if classify_step(
        "Status: INVALID_PARAM\nStatus: I2C_TIMEOUT\n", invalid_step
    ) != "FAIL":
        print("parser self-test FAILED: expected rejection hid unrelated failure")
        return 1

    failure_gap = (
        Result(invalid_step, "FAIL", 0.0, ""),
        Result(invalid_step, "UNKNOWN", 0.0, ""),
        Result(invalid_step, "FAIL", 0.0, ""),
    )
    if max_consecutive_failures(failure_gap) != 1:
        print("parser self-test FAILED: UNKNOWN did not break failure run")
        return 1

    version_output = (
        "Arduino-ESP32: 3.3.11\nESP-IDF: v5.5.5\n"
        "INA228 library version: 9.9.9\n"
        "INA228 library commit: 0123456789ab (clean)\n"
    )
    if version_contract_errors(
        version_output, "arduino", "9.9.9", "0123456789ab", "clean"
    ):
        print("parser self-test FAILED: valid firmware provenance rejected")
        return 1
    if not version_contract_errors(
        version_output, "arduino", "9.9.9", "deadbeefcafe", "clean"
    ):
        print("parser self-test FAILED: stale firmware provenance accepted")
        return 1

    soak_step = Step("vbus", ("Vbus",), "report self-test", "soak")
    soak_result = Result(soak_step, "PASS", 0.125, "Vbus: 12.0 V")
    soak_summary = SoakSummary()
    soak_summary.record(soak_result)
    report_args = argparse.Namespace(
        port=None,
        baud=115200,
        profile="arduino",
        suite="smoke",
        expected_library_version="9.9.9",
        expected_commit="0123456789ab",
        expected_git_status="clean",
        operator=None,
        board=None,
        environment=None,
        fixture=None,
        safety=None,
        notes=None,
        soak_store_every=1,
    )
    timestamp = dt.datetime(2026, 1, 1, tzinfo=dt.timezone.utc)
    with tempfile.TemporaryDirectory(prefix="ina228_hil_self_test_") as temp_dir:
        report_path = pathlib.Path(temp_dir) / "report.md"
        write_report(report_path, report_args, (soak_result,), timestamp,
                     timestamp, None, 1.0, "", soak_summary)
        report = report_path.read_text(encoding="utf-8")
    if ("- Soak verdict counts: PASS=1, FAIL=0, UNKNOWN=0" not in report or
            "- Soak latency min/mean/max: 0.125 / 0.125 / 0.125 s" not in report):
        print("parser self-test FAILED: soak report generation")
        return 1
    print("parser self-test PASSED")
    return 0


def print_plan(steps: Iterable[Step], soak_seconds: float,
               benchmark_count: int, include_not_run: bool) -> None:
    print("INA228 HIL command plan:")
    for step in steps:
        expected = ", ".join(step.expected)
        print(f"  {step.command:<16} # {step.suite}/{step.label}; expect: {expected}")
        if step.pause_after_s > 0.0:
            print(f"  {'<pause>':<16} # {step.pause_after_s:.3f}s after {step.command}")
    if benchmark_count > 0:
        print(f"  {'<benchmark>':<16} # {benchmark_count} iterations each for "
              f"{len(BENCHMARK_STEPS)} read paths")
    if soak_seconds > 0.0:
        print(f"  {'<soak loop>':<16} # soak for {soak_seconds:.0f}s using "
              f"{len(SOAK_STEPS)} self-contained soak commands")
    if include_not_run:
        print(f"  {'<not-run rows>':<16} # {len(not_run_results(soak_seconds))} "
              "fixture/tooling limitations")


def read_response(serial_port, timeout_s: float, idle_s: float,
                  prompt_token: str | None = None) -> str:
    deadline = time.monotonic() + timeout_s
    last_rx = time.monotonic()
    chunks: list[bytes] = []
    line_prompt = prompt_token is not None and prompt_token.strip() == ">"
    prompt_bytes = prompt_token.encode("utf-8") if prompt_token and not line_prompt else None
    while time.monotonic() < deadline:
        waiting = getattr(serial_port, "in_waiting", 0)
        data = serial_port.read(waiting or 1)
        if data:
            chunks.append(data)
            last_rx = time.monotonic()
            joined = b"".join(chunks)
            if line_prompt:
                text = joined.decode("utf-8", errors="replace")
                if PROMPT_RE.search(clean_output(text)):
                    break
            elif prompt_bytes and prompt_bytes in joined:
                break
            continue
        if not prompt_token and chunks and (time.monotonic() - last_rx) >= idle_s:
            break
    return b"".join(chunks).decode("utf-8", errors="replace")


def read_until_text(serial_port, timeout_s: float, token: str, max_bytes: int | None = None) -> str:
    deadline = time.monotonic() + timeout_s
    chunks: list[bytes] = []
    token_bytes = token.encode("utf-8")
    while time.monotonic() < deadline:
        waiting = getattr(serial_port, "in_waiting", 0)
        data = serial_port.read(waiting or 1)
        if not data:
            continue
        chunks.append(data)
        joined = b"".join(chunks)
        if token_bytes in joined:
            break
        if max_bytes is not None and len(joined) >= max_bytes:
            break
    return b"".join(chunks).decode("utf-8", errors="replace")


def hilrun_end_re(token: str, seq: str) -> re.Pattern[str]:
    return re.compile(
        rf"(?m)^HIL_END token={re.escape(token)} seq={re.escape(seq)} "
        r"status=([A-Z0-9_]+) elapsed_ms=([0-9]+)\s*$"
    )


def read_until_hilrun_end(serial_port, timeout_s: float, token: str, seq: str,
                          max_bytes: int | None = None) -> str:
    deadline = time.monotonic() + timeout_s
    chunks: list[bytes] = []
    end_re = hilrun_end_re(token, seq)
    while time.monotonic() < deadline:
        waiting = getattr(serial_port, "in_waiting", 0)
        data = serial_port.read(waiting or 1)
        if not data:
            continue
        chunks.append(data)
        joined = b"".join(chunks)
        text = joined.decode("utf-8", errors="replace")
        if end_re.search(clean_output(text)):
            break
        if max_bytes is not None and len(joined) >= max_bytes:
            break
    return b"".join(chunks).decode("utf-8", errors="replace")


def strip_hil_marker(text: str, token: str) -> str:
    marker = f"HILMARK {token}"
    index = text.find(marker)
    if index < 0:
        return text
    return text[:index]


def strip_hilrun_frame(text: str, token: str, seq: str) -> tuple[str, str, bool]:
    clean = clean_output(text)
    begin_line = f"HIL_BEGIN token={token} seq={seq}"
    end_re = hilrun_end_re(token, seq)
    begin_index = clean.find(begin_line)
    # Search for the end marker after the begin marker so a stale end line can
    # never produce an empty payload that still looks like a complete frame.
    end_match = end_re.search(clean, max(begin_index, 0))
    if begin_index < 0 or end_match is None:
        return (
            clean + f"\n[runner] missing HIL frame token={token} seq={seq}",
            "",
            False,
        )
    payload_start = begin_index + len(begin_line)
    payload = clean[payload_start:end_match.start()].strip("\n")
    status = end_match.group(1)
    elapsed_ms = end_match.group(2)
    payload += f"\n[runner] frame_status={status} frame_elapsed_ms={elapsed_ms}"
    if status not in ("OK", "IN_PROGRESS"):
        payload += f"\nStatus: {status}"
    trailer = clean[end_match.end():].strip("\n")
    return payload, trailer, True


def drain_input(serial_port, drain_s: float) -> str:
    if drain_s <= 0.0:
        return ""
    deadline = time.monotonic() + drain_s
    chunks: list[bytes] = []
    while time.monotonic() < deadline:
        waiting = getattr(serial_port, "in_waiting", 0)
        if waiting <= 0:
            time.sleep(min(0.005, max(0.0, deadline - time.monotonic())))
            continue
        chunks.append(serial_port.read(waiting))
    return b"".join(chunks).decode("utf-8", errors="replace")


def run_step(serial_port, step: Step, args: argparse.Namespace) -> Result:
    start = time.monotonic()
    marker_missing = False
    trailer_verdict: str | None = None
    stale = drain_input(serial_port, args.drain_before_command_s)
    token = f"{args.frame_prefix}{time.monotonic_ns()}"
    seq = "0"
    if args.no_command_framing:
        serial_port.write((step.command + "\n").encode("ascii"))
        serial_port.flush()
        output = read_response(serial_port, args.timeout_s, args.idle_s, args.prompt_token)
    elif args.legacy_marker:
        marker = f"HILMARK {token}"
        serial_port.write((f"{step.command}\nhilmark {token}\n").encode("ascii"))
        serial_port.flush()
        marker_output = read_until_text(serial_port, args.timeout_s, marker,
                                        args.max_frame_bytes)
        marker_missing = marker not in marker_output
        output = strip_hil_marker(marker_output, token)
        if marker_missing:
            recovered = False
            for retry in range(args.marker_retries):
                retry_token = f"{args.frame_prefix}{time.monotonic_ns()}R{retry}"
                retry_marker = f"HILMARK {retry_token}"
                serial_port.write((f"hilmark {retry_token}\n").encode("ascii"))
                serial_port.flush()
                retry_output = read_until_text(serial_port, args.timeout_s, retry_marker,
                                              args.max_frame_bytes)
                if retry_marker in retry_output:
                    output += (
                        f"\n[runner] recovered missing HILMARK {token} "
                        f"with retry {retry + 1}"
                    )
                    marker_missing = False
                    recovered = True
                    break
                output += f"\n[runner] marker retry {retry + 1} failed:\n{retry_output}"
            if not recovered:
                output += f"\n[runner] missing HILMARK {token}"
    else:
        serial_port.write((f"hilrun {token} {seq} {step.command}\n").encode("ascii"))
        serial_port.flush()
        frame_output = read_until_hilrun_end(serial_port, args.timeout_s, token, seq,
                                             args.max_frame_bytes)
        output, inline_trailer, frame_ok = strip_hilrun_frame(frame_output, token, seq)
        marker_missing = not frame_ok
        classification_output = output
        if frame_ok:
            drained_trailer = drain_input(serial_port, args.post_frame_drain_s)
            trailer = "\n".join(
                part for part in (inline_trailer, drained_trailer) if part
            )
            cleaned_trailer = clean_output(trailer).strip()
            if cleaned_trailer and not PROMPT_RE.fullmatch(cleaned_trailer):
                trailer_verdict = "FAIL" if has_failure(cleaned_trailer) else "UNKNOWN"
                output += "\n[runner] drained trailing serial input after frame:\n"
                output += trailer
    if args.no_command_framing or args.legacy_marker:
        classification_output = output
    if stale.strip():
        output = "[runner] drained stale serial input before command:\n" + stale + "\n" + output
    elapsed = time.monotonic() - start
    if marker_missing:
        # --require-framed asserts that framed evidence exists. Without it a lost
        # frame is only inconclusive; with it, it is a failed release gate.
        verdict = "FAIL" if args.require_framed else "UNKNOWN"
    else:
        verdict = classify_step(classification_output, step)
    if trailer_verdict == "FAIL":
        verdict = "FAIL"
    elif trailer_verdict == "UNKNOWN" and verdict == "PASS":
        verdict = "UNKNOWN"
    if verdict == "PASS" and step.command == "version":
        provenance_errors = version_contract_errors(
            classification_output,
            args.profile,
            args.expected_library_version,
            args.expected_commit,
            args.expected_git_status,
            args.framework_token,
        )
        if provenance_errors:
            verdict = "FAIL"
            output += "\n[runner] provenance failure: " + "; ".join(provenance_errors)
    return Result(step=step, verdict=verdict, elapsed_s=elapsed, output=clean_output(output))


def write_transcript(path: pathlib.Path, results: Sequence[Result], boot_output: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as f:
        if boot_output.strip():
            f.write("### boot transcript\n")
            f.write(boot_output.rstrip())
            f.write("\n\n")
        for result in results:
            f.write(f"### {result.step.suite}: {result.step.command}\n")
            f.write(f"verdict={result.verdict} elapsed_s={result.elapsed_s:.3f}\n")
            f.write(result.output.rstrip())
            f.write("\n\n")


def markdown_escape(text: str) -> str:
    return text.replace("|", "\\|").replace("\n", "<br>")


def summarize_counts(results: Sequence[Result]) -> dict[str, int]:
    counts = {"PASS": 0, "FAIL": 0, "UNKNOWN": 0, "NOT RUN": 0}
    for result in results:
        counts[result.verdict] = counts.get(result.verdict, 0) + 1
    return counts


def elapsed_stats(results: Sequence[Result]) -> tuple[float, float, float]:
    timed = [result for result in results if result.verdict != "NOT RUN"]
    if not timed:
        return (0.0, 0.0, 0.0)
    values = [result.elapsed_s for result in timed]
    return (min(values), sum(values) / len(values), max(values))


def output_excerpt(output: str, limit: int = 160) -> str:
    lines = [line.strip() for line in clean_output(output).splitlines() if line.strip()]
    text = " / ".join(lines)
    if len(text) > limit:
        return text[: limit - 3] + "..."
    return text


def max_consecutive_failures(results: Sequence[Result]) -> int:
    max_seen = 0
    current = 0
    for result in results:
        if result.verdict == "FAIL":
            current += 1
            max_seen = max(max_seen, current)
        else:
            current = 0
    return max_seen


def actual_max_consecutive_failures(
    results: Sequence[Result], soak_summary: SoakSummary | None
) -> int:
    if soak_summary is None:
        return max_consecutive_failures(results)
    pre_soak = [
        result
        for result in results
        if result.step.suite not in ("soak", "not-run")
    ]
    trailing_pre_soak = 0
    for result in reversed(pre_soak):
        if result.verdict != "FAIL":
            break
        trailing_pre_soak += 1
    return max(
        max_consecutive_failures(pre_soak),
        soak_summary.max_failure_run,
        trailing_pre_soak + soak_summary.leading_failure_count,
    )


def not_run_results(soak_seconds: float) -> list[Result]:
    results = [
        Result(step=step, verdict="NOT RUN", elapsed_s=0.0, output=step.label)
        for step in STATIC_NOT_RUN_STEPS
    ]
    if soak_seconds <= 0.0:
        step = Step("<8-hour soak>", ("--soak-hours 8",),
                    "soak not requested for this run", "not-run")
        results.append(Result(step=step, verdict="NOT RUN", elapsed_s=0.0, output=step.label))
    return results


def actual_counts(results: Sequence[Result], soak_summary: SoakSummary | None) -> dict[str, int]:
    counts = summarize_counts(results)
    if soak_summary is None:
        return counts
    stored_soak_counts = summarize_counts(
        [result for result in results if result.step.suite == "soak"]
    )
    for verdict, count in stored_soak_counts.items():
        counts[verdict] = max(0, counts.get(verdict, 0) - count)
    for verdict, count in soak_summary.counts().items():
        counts[verdict] = counts.get(verdict, 0) + count
    return counts


def actual_command_count(results: Sequence[Result], soak_summary: SoakSummary | None) -> int:
    stored_soak_count = sum(1 for result in results if result.step.suite == "soak")
    regular_executed_count = sum(
        1
        for result in results
        if result.step.suite != "soak" and result.verdict != "NOT RUN"
    )
    actual_soak_count = soak_summary.total if soak_summary is not None else stored_soak_count
    return regular_executed_count + actual_soak_count


def write_report(path: pathlib.Path, args: argparse.Namespace, results: Sequence[Result],
                 started_at: dt.datetime, ended_at: dt.datetime,
                 transcript_path: pathlib.Path | None, soak_seconds: float,
                 boot_output: str, soak_summary: SoakSummary | None) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    counts = actual_counts(results, soak_summary)
    elapsed = (ended_at - started_at).total_seconds()
    min_elapsed, mean_elapsed, max_elapsed = elapsed_stats(results)
    stored_soak_count = sum(1 for result in results if result.step.suite == "soak")
    actual_soak_count = soak_summary.total if soak_summary is not None else stored_soak_count
    executed_command_count = actual_command_count(results, soak_summary)
    with path.open("w", encoding="utf-8", newline="\n") as f:
        f.write("# INA228 HIL Validation Report\n\n")
        f.write(f"- Date/time: {started_at.isoformat()} to {ended_at.isoformat()}\n")
        f.write(f"- Elapsed: {elapsed:.1f} s\n")
        f.write(f"- Port: {args.port or 'NOT RUN'}\n")
        f.write(f"- Baud: {args.baud}\n")
        f.write(f"- Firmware profile: {args.profile}\n")
        f.write(f"- Suite: {args.suite}\n")
        f.write(f"- Expected library version: {args.expected_library_version}\n")
        f.write(f"- Expected firmware commit: {args.expected_commit}\n")
        f.write(f"- Expected firmware status: {args.expected_git_status}\n")
        f.write(f"- Soak requested: {soak_seconds:.1f} s\n")
        f.write(f"- Operator: {args.operator or 'unspecified'}\n")
        f.write(f"- Board/environment: {args.board or 'unspecified'} / "
                f"{args.environment or 'unspecified'}\n")
        f.write(f"- Fixture: {args.fixture or 'unspecified'}\n")
        f.write(f"- Safety assumptions: {args.safety or 'unspecified'}\n")
        f.write(f"- OS: {platform.platform()}\n")
        f.write(f"- Python: {platform.python_version()}\n")
        f.write(f"- HIL command: `{' '.join(sys.argv)}`\n")
        f.write(f"- Branch: {git_text(['branch', '--show-current'])}\n")
        f.write(f"- Commit: {git_text(['rev-parse', 'HEAD'])}\n")
        f.write("- Dirty status:\n\n")
        f.write("```text\n")
        f.write(git_text(["status", "--short"]))
        f.write("\n```\n\n")
        if transcript_path is not None:
            f.write(f"- Transcript: `{transcript_path.as_posix()}`\n")
        if args.notes:
            f.write(f"- Notes: {args.notes}\n")
        if boot_output.strip():
            f.write("\n## Boot Transcript Excerpt\n\n")
            f.write("```text\n")
            f.write(output_excerpt(boot_output, 1200))
            f.write("\n```\n")
        f.write("\n## Summary\n\n")
        f.write("| PASS | FAIL | UNKNOWN | NOT RUN |\n")
        f.write("| ---: | ---: | ---: | ---: |\n")
        f.write(f"| {counts.get('PASS', 0)} | {counts.get('FAIL', 0)} | "
                f"{counts.get('UNKNOWN', 0)} | {counts.get('NOT RUN', 0)} |\n\n")
        f.write("## Timing Summary\n\n")
        f.write(f"- Commands executed: {executed_command_count}\n")
        f.write(f"- Commands recorded in detail: {len(results)}\n")
        f.write(f"- Soak commands executed: {actual_soak_count}\n")
        f.write(f"- Soak rows recorded in detail: {stored_soak_count}\n")
        f.write(f"- Recorded command latency min/mean/max: {min_elapsed:.3f} / "
                f"{mean_elapsed:.3f} / {max_elapsed:.3f} s\n\n")
        f.write("- Maximum consecutive FAIL verdicts: "
                f"{actual_max_consecutive_failures(results, soak_summary)}\n\n")
        f.write("## Steps\n\n")
        f.write("| ID | Suite | Command | Expected | Observed | Result | Elapsed s | Notes |\n")
        f.write("| --- | --- | --- | --- | --- | --- | ---: | --- |\n")
        for index, result in enumerate(results, 1):
            expected = ", ".join(result.step.expected)
            note = result.step.label
            f.write(f"| {index} | {result.step.suite} | `{markdown_escape(result.step.command)}` | "
                    f"{markdown_escape(expected)} | {markdown_escape(output_excerpt(result.output))} | "
                    f"{result.verdict} | "
                    f"{result.elapsed_s:.3f} | {markdown_escape(note)} |\n")
        if soak_seconds > 0.0:
            f.write("\n## Soak Summary\n\n")
            f.write(f"- Requested duration: {soak_seconds:.1f} s\n")
            f.write(f"- Executed soak commands: {actual_soak_count}\n")
            f.write(f"- Recorded soak rows: {stored_soak_count}\n")
            f.write(f"- Soak PASS row storage stride: each {args.soak_store_every}th "
                    "soak command when it passes, plus all FAIL/UNKNOWN rows\n")
            if soak_summary is not None:
                f.write(f"- Soak verdict counts: PASS={soak_summary.pass_count}, "
                        f"FAIL={soak_summary.fail_count}, "
                        f"UNKNOWN={soak_summary.unknown_count}\n")
                f.write("- Soak latency min/mean/max: "
                        f"{soak_summary.min_latency_s:.3f} / "
                        f"{soak_summary.mean_latency_s():.3f} / "
                        f"{soak_summary.max_latency_s:.3f} s\n")
            f.write("- Command mix:\n")
            command_counts: dict[str, int]
            if soak_summary is not None:
                command_counts = soak_summary.command_counts
            else:
                command_counts = {}
                for result in results:
                    if result.step.suite != "soak":
                        continue
                    command_counts[result.step.command] = (
                        command_counts.get(result.step.command, 0) + 1
                    )
            for command, count in sorted(command_counts.items()):
                f.write(f"  - `{command}`: {count}\n")
            if soak_summary is not None and soak_summary.command_failures:
                f.write("- Non-PASS soak command counts:\n")
                for command, count in sorted(soak_summary.command_failures.items()):
                    f.write(f"  - `{command}`: {count}\n")
        f.write("\n## Limitations\n\n")
        f.write("- Hardware safety and fixture details must be filled in by the operator.\n")
        f.write("- This runner records serial CLI evidence only; external instruments must be logged separately.\n")
        f.write("- Staged `maxInstructions` coverage is limited to the example CLI commands. "
                "The `transfer` suite records example callback counts, not logic-analyzer bus bytes. "
                "Example `tick()` calls between serial commands can add readiness reads; exact assertions "
                "are kept to deterministic paths and other paths record snapshots.\n")
        if soak_seconds <= 0.0:
            f.write("- Soak test was not requested in this run.\n")


def run_soak(serial_port, args: argparse.Namespace, results: list[Result],
             soak_seconds: float) -> SoakSummary:
    summary = SoakSummary()
    deadline = time.monotonic() + soak_seconds
    index = 0
    while time.monotonic() < deadline:
        step = SOAK_STEPS[index % len(SOAK_STEPS)]
        result = run_step(serial_port, step, args)
        summary.record(result)
        store_result = (
            result.verdict != "PASS"
            or args.soak_store_every <= 1
            or summary.total % args.soak_store_every == 0
        )
        if store_result:
            results.append(result)
        print_result = (
            result.verdict != "PASS"
            or args.soak_progress_every <= 1
            or summary.total % args.soak_progress_every == 0
        )
        if print_result:
            print(f"[{result.verdict}] soak#{summary.total} {step.command} "
                  f"({step.label}) {result.elapsed_s:.3f}s")
        if result.verdict != "PASS":
            print(result.output.rstrip())
        elif args.verbose and print_result:
            print(result.output.rstrip())
        if result.verdict == "FAIL" or (
                args.stop_on_non_pass and result.verdict != "PASS"):
            print("Stopping soak after non-PASS verdict")
            break
        index += 1
        pause_s = max(args.command_pause_s, result.step.pause_after_s)
        if pause_s > 0.0:
            time.sleep(pause_s)
    return summary


def run_benchmarks(serial_port, args: argparse.Namespace, results: list[Result]) -> None:
    if args.benchmark_count <= 0:
        return
    for step in BENCHMARK_STEPS:
        latencies: list[float] = []
        non_pass = 0
        for _ in range(args.benchmark_count):
            result = run_step(serial_port, step, args)
            results.append(result)
            latencies.append(result.elapsed_s)
            if result.verdict != "PASS":
                non_pass += 1
            if args.verbose:
                print(f"[{result.verdict}] {step.command} ({step.label}) "
                      f"{result.elapsed_s:.3f}s")
            pause_s = max(args.command_pause_s, result.step.pause_after_s)
            if pause_s > 0.0:
                time.sleep(pause_s)
        if latencies:
            mean = sum(latencies) / len(latencies)
            print(f"[BENCH] {step.command}: count={len(latencies)} non_pass={non_pass} "
                  f"min/mean/max={min(latencies):.3f}/{mean:.3f}/{max(latencies):.3f}s")


def run_serial(args: argparse.Namespace) -> int:
    try:
        import serial  # type: ignore
    except ImportError:
        print("pyserial is required for hardware runs; use --dry-run without it")
        return 2

    steps = selected_steps(args.suite)
    soak_seconds = args.soak_seconds
    if args.soak_hours > 0.0:
        soak_seconds = args.soak_hours * 3600.0

    started_at = dt.datetime.now().astimezone()
    results: list[Result] = []
    soak_summary: SoakSummary | None = None
    transcript_path = pathlib.Path(args.transcript) if args.transcript else None
    report_path = pathlib.Path(args.report) if args.report else None

    serial_port = serial.Serial()
    try:
        serial_port.dtr = False
        serial_port.rts = False
    except (AttributeError, OSError):
        pass
    serial_port.port = args.port
    serial_port.baudrate = args.baud
    serial_port.timeout = 0.05
    serial_port.write_timeout = 2.0
    try:
        serial_port.open()
    except (serial.SerialException, OSError) as exc:
        print(f"Unable to open {args.port}: {exc}")
        return 2

    boot_output = ""
    aborted: BaseException | None = None
    try:
        with serial_port:
            time.sleep(args.boot_settle_s)
            boot_output = read_response(serial_port, args.boot_capture_s, args.idle_s,
                                        args.prompt_token)
            for step in steps:
                result = run_step(serial_port, step, args)
                results.append(result)
                print(f"[{result.verdict}] {step.command} ({step.label}) {result.elapsed_s:.3f}s")
                if args.verbose:
                    print(result.output.rstrip())
                pause_s = max(args.command_pause_s, result.step.pause_after_s)
                if pause_s > 0.0:
                    time.sleep(pause_s)
            run_benchmarks(serial_port, args, results)
            if soak_seconds > 0.0:
                soak_summary = run_soak(serial_port, args, results, soak_seconds)
    except (serial.SerialException, OSError, KeyboardInterrupt) as exc:
        # Never discard the evidence collected so far; a long soak that dies
        # mid-run is exactly when the partial report matters most.
        aborted = exc
        print(f"Run aborted after {len(results)} steps: {exc!r}")
    finally:
        ended_at = dt.datetime.now().astimezone()
        if args.include_not_run or report_path is not None:
            results.extend(not_run_results(soak_seconds))
        if transcript_path is not None:
            write_transcript(transcript_path, results, boot_output)
        if report_path is not None:
            write_report(report_path, args, results, started_at, ended_at, transcript_path,
                         soak_seconds, boot_output, soak_summary)

    if aborted is not None:
        return 2

    if any(result.verdict == "FAIL" for result in results):
        return 1
    if soak_summary is not None and soak_summary.fail_count > 0:
        return 1
    if any(result.verdict == "UNKNOWN" for result in results) and args.fail_on_unknown:
        return 1
    if (soak_summary is not None and soak_summary.unknown_count > 0
            and args.fail_on_unknown):
        return 1
    return 0


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser(
        description="Run bounded INA228 example-CLI HIL validation."
    )
    parser.add_argument("--port", help="Serial port for hardware run")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--profile", choices=("arduino", "idf"), default="arduino",
                        help="Flashed diagnostic CLI profile")
    parser.add_argument("--expected-library-version", default=configured_library_version(),
                        help="Required firmware library version, or 'any'")
    parser.add_argument("--expected-commit",
                        default=git_text(["rev-parse", "--short=12", "HEAD"]),
                        help="Required 12-character firmware Git commit, or 'any'")
    parser.add_argument("--expected-git-status", choices=("clean", "dirty", "any"),
                        default="clean",
                        help="Required firmware source-tree status")
    parser.add_argument("--framework-token", action="append",
                        help="Override profile-default version token; repeat as needed")
    parser.add_argument("--suite", choices=("smoke", "functional", "exhaustive", "targeted",
                                            "transfer"),
                        default="smoke")
    parser.add_argument("--timeout-s", type=float, default=5.0,
                        help="Maximum seconds to wait per command")
    parser.add_argument("--idle-s", type=float, default=0.25,
                        help="Stop reading after this idle period once data arrived")
    parser.add_argument("--boot-settle-s", type=float, default=1.0,
                        help="Initial serial settle time before sending commands")
    parser.add_argument("--boot-capture-s", type=float, default=2.0,
                        help="Seconds to collect boot/banner output after opening serial")
    parser.add_argument("--command-pause-s", type=float, default=0.0,
                        help="Pause between commands")
    parser.add_argument("--drain-before-command-s", type=float, default=0.0,
                        help="Bounded stale-input drain before each command")
    parser.add_argument("--post-frame-drain-s", type=float, default=0.0,
                        help="Bounded drain after a complete hilrun frame, usually to consume prompt")
    parser.add_argument("--no-command-framing", action="store_true",
                        help="Send raw commands without hilrun or legacy hilmark framing")
    parser.add_argument("--legacy-marker", action="store_true",
                        help="Use the older command + hilmark framing instead of hilrun")
    parser.add_argument("--require-framed", action="store_true",
                        help="Treat a missing or mismatched hilrun frame as FAIL instead of UNKNOWN")
    parser.add_argument("--max-frame-bytes", type=int, default=8192,
                        help="Maximum bytes to read for one framed command response")
    parser.add_argument("--frame-prefix", default="HIL",
                        help="Prefix for generated HIL framing tokens")
    parser.add_argument("--marker-retries", type=int, default=1,
                        help="Bounded hilmark retry count when a command marker is not observed")
    parser.add_argument("--prompt-token",
                        help="Optional prompt token that ends response reads early")
    parser.add_argument("--soak-hours", type=float, default=0.0,
                        help="Run a soak loop for this many hours, e.g. 8")
    parser.add_argument("--soak-seconds", type=float, default=0.0,
                        help="Short soak duration for shakedown runs")
    parser.add_argument("--soak-store-every", type=int, default=1,
                        help="Store every Nth PASS soak row; FAIL/UNKNOWN rows are always stored")
    parser.add_argument("--soak-progress-every", type=int, default=1,
                        help="Print every Nth PASS soak row; FAIL/UNKNOWN rows are always printed")
    parser.add_argument("--benchmark-count", type=int, default=0,
                        help="Run this many iterations for each benchmark read path")
    parser.add_argument("--report", help="Write Markdown report to this path")
    parser.add_argument("--transcript", help="Write full command transcript to this path")
    parser.add_argument("--include-not-run", action="store_true",
                        help="Append NOT RUN rows for known fixture/tooling gaps")
    parser.add_argument("--operator", help="Operator name for the Markdown report")
    parser.add_argument("--board", help="Board or fixture board name for the report")
    parser.add_argument("--environment", help="Build/firmware environment for the report")
    parser.add_argument("--fixture", help="Fixture wiring and load summary for the report")
    parser.add_argument("--safety", help="Electrical/thermal safety assumptions for the report")
    parser.add_argument("--notes", help="Additional free-form report notes")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print the command plan without opening serial")
    parser.add_argument("--parser-self-test", action="store_true",
                        help="Run local output classifier tests")
    parser.add_argument("--fail-on-unknown", action="store_true",
                        help="Return nonzero when any step is UNKNOWN")
    parser.add_argument("--stop-on-non-pass", action="store_true",
                        help="Also stop soak after UNKNOWN; FAIL always stops it")
    parser.add_argument("--verbose", action="store_true",
                        help="Print command responses during hardware run")
    args = parser.parse_args(argv)

    if args.timeout_s <= 0.0 or args.idle_s <= 0.0 or args.boot_settle_s < 0.0:
        parser.error("timeouts must be positive and boot settle must be nonnegative")
    if (args.boot_capture_s < 0.0 or args.command_pause_s < 0.0
            or args.drain_before_command_s < 0.0 or args.post_frame_drain_s < 0.0):
        parser.error("boot capture, command pause, and drain times must be nonnegative")
    if args.marker_retries < 0 or args.benchmark_count < 0:
        parser.error("retry and benchmark counts must be nonnegative")
    if args.soak_store_every <= 0 or args.soak_progress_every <= 0:
        parser.error("soak store/progress strides must be positive")
    if args.soak_hours < 0.0 or args.soak_seconds < 0.0:
        parser.error("pauses and soak durations must be nonnegative")
    if not args.frame_prefix or any(ch.isspace() for ch in args.frame_prefix):
        parser.error("frame prefix must be nonempty and contain no whitespace")
    if args.max_frame_bytes <= 0:
        parser.error("max frame bytes must be positive")
    if args.require_framed and (args.no_command_framing or args.legacy_marker):
        parser.error("--require-framed cannot be combined with raw or legacy framing")
    if (args.expected_commit != "any" and
            re.fullmatch(r"[0-9a-fA-F]{12}", args.expected_commit) is None):
        parser.error("--expected-commit must be a 12-digit Git SHA or 'any'")
    if (args.expected_library_version != "any" and
            re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", args.expected_library_version) is None):
        parser.error("--expected-library-version must be SemVer or 'any'")

    soak_seconds = args.soak_seconds
    if args.soak_hours > 0.0:
        soak_seconds = args.soak_hours * 3600.0

    if args.parser_self_test:
        return parser_self_test()
    if args.dry_run:
        print_plan(selected_steps(args.suite), soak_seconds,
                   args.benchmark_count, args.include_not_run)
        return 0
    if not args.port:
        parser.error("--port is required unless --dry-run or --parser-self-test is used")
    return run_serial(args)


if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="backslashreplace")
    sys.exit(main(sys.argv[1:]))
