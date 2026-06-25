#!/usr/bin/env python3
"""Bounded INA228 example-CLI hardware-in-loop runner."""

from __future__ import annotations

import argparse
import datetime as dt
import platform
import pathlib
import re
import subprocess
import sys
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
    accept_busy_after_empty_retry: bool = False
    accept_no_job_after_empty_retry: bool = False


@dataclass
class Result:
    step: Step
    verdict: str
    elapsed_s: float
    output: str


@dataclass
class SoakSummary:
    requested_s: float
    total: int = 0
    stored: int = 0
    pass_count: int = 0
    fail_count: int = 0
    unknown_count: int = 0
    empty_response_retries: int = 0
    total_latency_s: float = 0.0
    min_latency_s: float = 0.0
    max_latency_s: float = 0.0
    command_counts: dict[str, int] = field(default_factory=dict)
    command_failures: dict[str, int] = field(default_factory=dict)
    command_empty_retries: dict[str, int] = field(default_factory=dict)

    def record(self, result: Result) -> None:
        self.total += 1
        if result.verdict == "PASS":
            self.pass_count += 1
        elif result.verdict == "FAIL":
            self.fail_count += 1
        elif result.verdict == "UNKNOWN":
            self.unknown_count += 1
        self.command_counts[result.step.command] = (
            self.command_counts.get(result.step.command, 0) + 1
        )
        if result.verdict != "PASS":
            self.command_failures[result.step.command] = (
                self.command_failures.get(result.step.command, 0) + 1
            )
        retry_count = result.output.count("[runner] empty framed response attempt")
        if retry_count > 0:
            self.empty_response_retries += retry_count
            self.command_empty_retries[result.step.command] = (
                self.command_empty_retries.get(result.step.command, 0) + retry_count
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
    Step("version", ("INA228 library version",), "version"),
    Step("scan", ("INA228 Address Probe", "Healthy INA228 devices"), "scan"),
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
    Step("rstacc", ("reset",), "accumulator reset", "functional"),
    Step("recover", ("Attempting recovery", "frame_status=OK"), "manual recover", "functional"),
    Step("selftest", ("INA228 selftest",), "self-test", "functional"),
    Step("stress 50", ("Stress Summary", "Errors:"), "short stress", "functional"),
    Step("stress_mix 50", ("stress_mix summary", "fail="), "short mixed stress", "functional"),
)


EXHAUSTIVE_STEPS: tuple[Step, ...] = (
    Step("integer", ("Integer Sample", "Bus:", "Current:", "Power:"),
         "fixed-unit integer sample", "exhaustive"),
    Step("diagsnap", ("DIAG_ALRT Snapshot", "cache-only"),
         "cache-only diagnostic snapshot", "exhaustive"),
    Step("ready_step 0", ("INVALID_PARAM",),
         "readiness zero-budget rejection", "exhaustive", expect_failure=True),
    Step("ready_step 1", ("pollMeasurementReady",),
         "readiness single-instruction poll", "exhaustive"),
    Step("sample_step 0", ("INVALID_PARAM",),
         "power sample zero-budget rejection", "exhaustive", expect_failure=True),
    Step("sample_step 1", ("readPowerSampleRawStep", "IN_PROGRESS"),
         "power sample budget one", "exhaustive"),
    Step("sample_step 2", ("readPowerSampleRawStep",),
         "power sample budget two", "exhaustive"),
    Step("sample_step 5", ("Power Sample Step Result",),
         "power sample full budget", "exhaustive"),
    Step("apply_start", ("startConfigReplayJob", "IN_PROGRESS"),
         "calibration job start", "exhaustive", accept_busy_after_empty_retry=True),
    Step("apply_step 0", ("INVALID_PARAM",),
         "calibration job zero-budget rejection", "exhaustive", expect_failure=True),
    Step("apply_step 1", ("pollConfigReplayJob", "IN_PROGRESS"),
         "calibration job budget one", "exhaustive"),
    Step("apply_step 6", ("pollConfigReplayJob", "OK"),
         "calibration job full budget", "exhaustive",
         accept_no_job_after_empty_retry=True),
    Step("reset_start", ("startResetJob", "IN_PROGRESS"),
         "reset job start", "exhaustive", accept_busy_after_empty_retry=True),
    Step("reset_step 0", ("INVALID_PARAM",),
         "reset job zero-budget rejection", "exhaustive", expect_failure=True),
    Step("reset_step 1", ("pollResetJob", "IN_PROGRESS"),
         "reset job budget one", "exhaustive", pause_after_s=0.05),
    Step("reset_step 16", ("pollResetJob", "OK"),
         "reset job completion budget", "exhaustive",
         accept_no_job_after_empty_retry=True),
    Step("mode 15", ("setMode", "OK"),
         "restore continuous-all mode after reset job", "exhaustive"),
)


def targeted_steps() -> tuple[Step, ...]:
    """Boundary-heavy HIL pass for chip/library features without a long soak."""
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
        Step("adcrange 1", ("setAdcRange", "OK"), "switch low shunt range", "targeted"),
        Step("integer", ("Integer Sample", "Shunt:"), "integer sample after low range", "targeted"),
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
        Step("cal 0.015 10", ("setCalibration", "OK"),
             "reapply nominal calibration", "targeted"),
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
             "ready budget one", "targeted"),
        Step("ready_step 2", ("pollMeasurementReady",),
             "ready budget two", "targeted"),
        Step("ready_step 255", ("pollMeasurementReady",),
             "ready max budget", "targeted"),
        Step("sample_step 0", ("INVALID_PARAM",),
             "sample zero-budget rejection", "targeted", expect_failure=True),
        Step("sample_step 1", ("readPowerSampleRawStep",),
             "sample budget one", "targeted"),
        Step("sample_step 2", ("readPowerSampleRawStep",),
             "sample budget two", "targeted"),
        Step("sample_step 3", ("readPowerSampleRawStep",),
             "sample budget three", "targeted"),
        Step("sample_step 4", ("readPowerSampleRawStep",),
             "sample budget four", "targeted"),
        Step("sample_step 5", ("Power Sample Step Result",),
             "sample full budget", "targeted"),
        Step("sample_step 255", ("Power Sample Step Result",),
             "sample max budget", "targeted"),
        Step("apply_start", ("startConfigReplayJob", "IN_PROGRESS"),
             "calibration job start", "targeted", accept_busy_after_empty_retry=True),
        Step("apply_step 0", ("INVALID_PARAM",),
             "calibration zero-budget rejection", "targeted", expect_failure=True),
        Step("apply_step 1", ("pollConfigReplayJob",),
             "calibration budget one", "targeted"),
        Step("apply_step 2", ("pollConfigReplayJob",),
             "calibration budget two", "targeted"),
        Step("apply_step 3", ("pollConfigReplayJob",),
             "calibration budget three", "targeted"),
        Step("apply_step 6", ("BUSY",),
             "post-completion calibration poll reports BUSY", "targeted",
             expect_failure=True),
        Step("apply_start", ("startConfigReplayJob", "IN_PROGRESS"),
             "calibration full-budget restart", "targeted",
             accept_busy_after_empty_retry=True),
        Step("apply_step 6", ("pollConfigReplayJob", "OK"),
             "calibration full-budget completion", "targeted",
             accept_no_job_after_empty_retry=True),
        Step("reset_start", ("startResetJob", "IN_PROGRESS"),
             "reset job start", "targeted", accept_busy_after_empty_retry=True),
        Step("reset_step 0", ("INVALID_PARAM",),
             "reset zero-budget rejection", "targeted", expect_failure=True),
        Step("reset_step 1", ("pollResetJob", "IN_PROGRESS"),
             "reset budget one", "targeted", pause_after_s=0.05),
        Step("reset_step 1", ("pollResetJob",),
             "reset budget one repeated", "targeted", pause_after_s=0.05),
        Step("reset_step 2", ("pollResetJob",),
             "reset budget two", "targeted", pause_after_s=0.05),
        Step("reset_step 16", ("pollResetJob", "OK"),
             "reset completion budget", "targeted",
             accept_no_job_after_empty_retry=True),
    ])

    for trigger_mode in range(1, 8):
        steps.extend([
            Step(f"trigger {trigger_mode}", ("triggerConversion",),
                 f"trigger mode {trigger_mode}", "targeted", pause_after_s=0.02),
            Step("ready_step 1", ("pollMeasurementReady",),
                 f"ready poll after trigger {trigger_mode}", "targeted"),
            Step("sample_step 5", ("readPowerSampleRawStep",),
                 f"sample after trigger {trigger_mode}", "targeted"),
        ])

    steps.extend([
        Step("mode 15", ("setMode", "OK"), "restore continuous mode after triggers", "targeted"),
        Step("rstacc", ("resetAccumulators", "OK"), "reset accumulators", "targeted"),
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
        Step("mode -1", ("Invalid mode",), "reject invalid negative mode", "targeted"),
        Step("mode 16", ("Invalid mode",), "reject invalid high mode", "targeted"),
        Step("trigger 8", ("Invalid trigger mode",), "reject invalid trigger mode", "targeted"),
        Step("convtime vbus 8", ("Invalid conversion time",), "reject invalid conversion index", "targeted"),
        Step("convtime bogus 1", ("Invalid target",), "reject invalid conversion target", "targeted"),
        Step("averaging 8", ("Invalid averaging",), "reject invalid averaging", "targeted"),
        Step("adcrange 2", ("Invalid ADC range",), "reject invalid ADC range", "targeted"),
        Step("delay 256", ("Usage: delay",), "reject invalid conversion delay", "targeted"),
        Step("tempco 16384", ("Usage: tempco",), "reject invalid tempco", "targeted"),
        Step("tempcomp 2", ("Usage: tempcomp",), "reject invalid temp compensation", "targeted"),
        Step("alatch 2", ("Usage: alatch",), "reject invalid latch", "targeted"),
        Step("cnvralert 2", ("Usage: cnvralert",), "reject invalid conversion alert", "targeted"),
        Step("alslow 2", ("Usage: alslow",), "reject invalid slow alert", "targeted"),
        Step("apol 2", ("Usage: apol",), "reject invalid alert polarity", "targeted"),
        Step("cal 0 10", ("Usage: cal",), "reject zero shunt calibration", "targeted"),
        Step("reg16 0x100", ("Usage: reg16",), "reject invalid raw16 register", "targeted"),
        Step("reg24 0x100", ("Usage: reg24",), "reject invalid raw24 register", "targeted"),
        Step("reg40 0x100", ("Usage: reg40",), "reject invalid raw40 register", "targeted"),
        Step("unknown_hil_command", ("Unknown command", "INVALID_PARAM"),
             "reject unknown command with framed status", "targeted", expect_failure=True),
        Step("init 0x50", ("Invalid address",), "reject invalid init address", "targeted"),
        Step("end", ("Device shut down",), "end driver", "targeted"),
        Step("vbus", ("NOT_INITIALIZED",),
             "read after end must fail visibly", "targeted", expect_failure=True),
        Step("init 0x41", ("begin", "OK"), "reinitialize known device", "targeted"),
        Step("recover", ("Attempting recovery", "frame_status=OK"),
             "manual recovery after reinit", "targeted"),
        Step("settings", ("Active Settings", "State:", "READY"), "final settings", "targeted"),
        Step("drv", ("Driver Health", "State: READY", "Consecutive failures: 0"),
             "final health must be clean", "targeted"),
        Step("read", ("Vbus", "Power"), "final aggregate read", "targeted"),
    ])

    return tuple(steps)


TARGETED_STEPS: tuple[Step, ...] = targeted_steps()

TRANSFER_STEPS: tuple[Step, ...] = (
    Step("verbose 0", ("Verbose mode",), "reduce CLI chatter", "transfer"),
    Step("mode 15", ("setMode", "OK"), "continuous-all mode for stable reads", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("ready_step 0", ("INVALID_PARAM",),
         "zero-budget readiness is bus-silent", "transfer", expect_failure=True),
    Step("xfer_assert 0 0 0", ("XFER_ASSERT PASS",),
         "ready_step 0 transfer count", "transfer"),
    Step("trigger 7", ("triggerConversion",), "start triggered conversion for readiness poll",
         "transfer", pause_after_s=0.05),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("ready_step 1", ("pollMeasurementReady",),
         "single-instruction readiness poll", "transfer"),
    Step("xfer_stats", ("XFER_STATS",),
         "ready_step 1 transfer count snapshot", "transfer"),
    Step("mode 15", ("setMode", "OK"), "restore continuous mode", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("sample_step 0", ("INVALID_PARAM",),
         "zero-budget sample is bus-silent", "transfer", expect_failure=True),
    Step("xfer_assert 0 0 0", ("XFER_ASSERT PASS",),
         "sample_step 0 transfer count", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("sample_step 1", ("readPowerSampleRawStep", "IN_PROGRESS"),
         "fresh sample job budget one", "transfer"),
    Step("xfer_assert 1 0 1", ("XFER_ASSERT PASS",),
         "sample_step 1 transfer count", "transfer"),
    Step("sample_step 255", ("Power Sample Step Result",),
         "finish partial sample job", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("sample_step 2", ("readPowerSampleRawStep",),
         "fresh sample job budget two", "transfer"),
    Step("xfer_assert 2 0 2", ("XFER_ASSERT PASS",),
         "sample_step 2 transfer count", "transfer"),
    Step("sample_step 255", ("Power Sample Step Result",),
         "finish partial sample job", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("sample_step 5", ("Power Sample Step Result",),
         "fresh sample job full budget", "transfer"),
    Step("xfer_assert 5 0 5", ("XFER_ASSERT PASS",),
         "sample_step 5 transfer count", "transfer"),
    Step("apply_start", ("startConfigReplayJob", "IN_PROGRESS"),
         "config replay job start", "transfer", accept_busy_after_empty_retry=True),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("apply_step 0", ("INVALID_PARAM",),
         "zero-budget config replay is bus-silent", "transfer", expect_failure=True),
    Step("xfer_assert 0 0 0", ("XFER_ASSERT PASS",),
         "apply_step 0 transfer count", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("apply_step 1", ("pollConfigReplayJob", "IN_PROGRESS"),
         "config replay budget one", "transfer"),
    Step("xfer_assert 0 1 1", ("XFER_ASSERT PASS",),
         "apply_step 1 transfer count", "transfer"),
    Step("apply_step 6", ("pollConfigReplayJob", "OK"),
         "finish partial config replay", "transfer", accept_no_job_after_empty_retry=True),
    Step("apply_start", ("startConfigReplayJob", "IN_PROGRESS"),
         "config replay full-budget restart", "transfer",
         accept_busy_after_empty_retry=True),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("apply_step 6", ("pollConfigReplayJob", "OK"),
         "config replay full budget", "transfer", accept_no_job_after_empty_retry=True),
    Step("xfer_stats", ("XFER_STATS",),
         "apply_step 6 transfer count snapshot", "transfer"),
    Step("reset_start", ("startResetJob", "IN_PROGRESS"),
         "reset job start", "transfer", accept_busy_after_empty_retry=True),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("reset_step 0", ("INVALID_PARAM",),
         "zero-budget reset is bus-silent", "transfer", expect_failure=True),
    Step("xfer_assert 0 0 0", ("XFER_ASSERT PASS",),
         "reset_step 0 transfer count", "transfer"),
    Step("xfer_reset", ("XFER_RESET",), "reset transfer counters", "transfer"),
    Step("reset_step 1", ("pollResetJob", "IN_PROGRESS"),
         "reset job budget one", "transfer", pause_after_s=0.05),
    Step("xfer_stats", ("XFER_STATS",),
         "reset_step 1 transfer count snapshot", "transfer"),
    Step("reset_step 16", ("pollResetJob", "OK"),
         "finish reset job", "transfer", accept_no_job_after_empty_retry=True),
    Step("mode 15", ("setMode", "OK"), "restore continuous mode after reset", "transfer"),
    Step("drv", ("Driver Health", "State: READY"), "final health", "transfer"),
)


TARGETED_SOAK_STEPS: tuple[Step, ...] = (
    Step("ready_step 0", ("INVALID_PARAM",),
         "targeted soak ready zero-budget rejection", "soak", expect_failure=True),
    Step("ready_step 1", ("pollMeasurementReady",),
         "targeted soak ready budget one", "soak"),
    Step("ready_step 2", ("pollMeasurementReady",),
         "targeted soak ready budget two", "soak"),
    Step("ready_step 255", ("pollMeasurementReady",),
         "targeted soak ready max budget", "soak"),
    Step("sample_step 0", ("INVALID_PARAM",),
         "targeted soak sample zero-budget rejection", "soak", expect_failure=True),
    Step("sample_step 1", ("readPowerSampleRawStep",),
         "targeted soak sample budget one", "soak"),
    Step("sample_step 2", ("readPowerSampleRawStep",),
         "targeted soak sample budget two", "soak"),
    Step("sample_step 3", ("readPowerSampleRawStep",),
         "targeted soak sample budget three", "soak"),
    Step("sample_step 4", ("readPowerSampleRawStep",),
         "targeted soak sample budget four", "soak"),
    Step("sample_step 5", ("Power Sample Step Result",),
         "targeted soak sample full budget", "soak"),
    Step("sample_step 255", ("Power Sample Step Result",),
         "targeted soak sample max budget", "soak"),
    Step("apply_start", ("startConfigReplayJob", "IN_PROGRESS"),
         "targeted soak apply start", "soak", accept_busy_after_empty_retry=True),
    Step("apply_step 0", ("INVALID_PARAM",),
         "targeted soak apply zero-budget rejection", "soak", expect_failure=True),
    Step("apply_step 1", ("pollConfigReplayJob",),
         "targeted soak apply budget one", "soak"),
    Step("apply_step 2", ("pollConfigReplayJob",),
         "targeted soak apply budget two", "soak"),
    Step("apply_step 6", ("pollConfigReplayJob", "OK"),
         "targeted soak apply completion", "soak", accept_no_job_after_empty_retry=True),
    Step("reset_start", ("startResetJob", "IN_PROGRESS"),
         "targeted soak reset start", "soak", accept_busy_after_empty_retry=True),
    Step("reset_step 0", ("INVALID_PARAM",),
         "targeted soak reset zero-budget rejection", "soak", expect_failure=True),
    Step("reset_step 1", ("pollResetJob", "IN_PROGRESS"),
         "targeted soak reset budget one", "soak", pause_after_s=0.05),
    Step("reset_step 1", ("pollResetJob",),
         "targeted soak reset budget one repeated", "soak", pause_after_s=0.05),
    Step("reset_step 16", ("pollResetJob", "OK"),
         "targeted soak reset completion", "soak", accept_no_job_after_empty_retry=True),
    Step("mode 15", ("setMode", "OK"),
         "targeted soak restore continuous mode", "soak"),
    Step("trigger 1", ("triggerConversion",),
         "targeted soak trigger bus", "soak", pause_after_s=0.02),
    Step("ready_step 1", ("pollMeasurementReady",),
         "targeted soak ready after trigger", "soak"),
    Step("sample_step 5", ("readPowerSampleRawStep",),
         "targeted soak sample after trigger", "soak"),
    Step("trigger 7", ("triggerConversion",),
         "targeted soak trigger all", "soak", pause_after_s=0.02),
    Step("ready_step 2", ("pollMeasurementReady",),
         "targeted soak ready after all trigger", "soak"),
    Step("sample_step 5", ("readPowerSampleRawStep",),
         "targeted soak sample after all trigger", "soak"),
    Step("mode 15", ("setMode", "OK"),
         "targeted soak restore continuous mode after trigger", "soak"),
    Step("adcrange 1", ("setAdcRange", "OK"),
         "targeted soak switch low range", "soak"),
    Step("integer", ("Integer Sample",),
         "targeted soak integer sample low range", "soak"),
    Step("adcrange 0", ("setAdcRange", "OK"),
         "targeted soak restore range", "soak"),
    Step("diagraw", ("DIAG_ALRT raw",),
         "targeted soak raw diagnostics", "soak"),
    Step("diagsnap", ("DIAG_ALRT Snapshot",),
         "targeted soak diagnostic snapshot", "soak"),
    Step("drv", ("Driver Health", "State: READY"),
         "targeted soak health check", "soak"),
    Step("recover", ("Attempting recovery", "frame_status=OK"),
         "targeted soak manual recovery", "soak"),
    Step("read", ("Vbus", "Power"),
         "targeted soak aggregate read", "soak"),
)


SOAK_STEPS: tuple[Step, ...] = (
    Step("vbus", ("Vbus",), "soak bus voltage", "soak"),
    Step("vshunt", ("Vshunt",), "soak shunt voltage", "soak"),
    Step("temp", ("Temp",), "soak temperature", "soak"),
    Step("current", ("Current",), "soak current", "soak"),
    Step("power", ("Power",), "soak power", "soak"),
    Step("integer", ("Integer Sample",), "soak integer sample", "soak"),
    Step("raw", ("Raw Registers", "Vbus"), "soak raw", "soak"),
    Step("sample_step 5", ("Power Sample Step Result",), "soak fixed-step sample", "soak"),
    Step("read", ("Vbus", "Power"), "soak aggregate", "soak"),
    Step("ready", ("Conversion ready",), "soak readiness", "soak"),
    Step("settings", ("Active Settings",), "soak settings", "soak"),
    Step("drv", ("Driver Health",), "soak health", "soak"),
    Step("diagsnap", ("DIAG_ALRT Snapshot",), "soak diagnostic snapshot", "soak"),
    Step("diagraw", ("DIAG_ALRT raw",), "soak raw diagnostics", "soak"),
    Step("probe", ("Status: OK",), "soak probe", "soak"),
    Step("recover", ("Attempting recovery", "frame_status=OK"), "soak recover", "soak"),
    Step("stress 50", ("Stress Summary", "Errors:"), "soak stress", "soak"),
    Step("stress_mix 50", ("stress_mix summary", "fail="), "soak mixed stress", "soak"),
)


def soak_steps_for_suite(suite: str) -> tuple[Step, ...]:
    if suite == "targeted":
        return TARGETED_SOAK_STEPS
    return SOAK_STEPS


BENCHMARK_STEPS: tuple[Step, ...] = (
    Step("vbus", ("Vbus",), "benchmark bus voltage", "benchmark"),
    Step("vshunt", ("Vshunt",), "benchmark shunt voltage", "benchmark"),
    Step("temp", ("Temp",), "benchmark temperature", "benchmark"),
    Step("raw", ("Raw Registers", "Vbus"), "benchmark raw sample", "benchmark"),
    Step("integer", ("Integer Sample",), "benchmark integer sample", "benchmark"),
    Step("read", ("Vbus", "Power"), "benchmark aggregate", "benchmark"),
    Step("sample_step 5", ("Power Sample Step Result",), "benchmark fixed-step sample", "benchmark"),
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


def clean_output(text: str) -> str:
    return ANSI_RE.sub("", text).replace("\r\n", "\n").replace("\r", "\n")


def has_line_prompt(text: str) -> bool:
    return bool(PROMPT_RE.search(clean_output(text)))


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


def classify_step(output: str, step: Step) -> str:
    text = clean_output(output)
    expected_seen = all(token in text for token in step.expected)
    if (step.accept_busy_after_empty_retry
            and "[runner] empty framed response attempt" in text
            and "Status: BUSY" in text
            and "Another fixed-step job is active" in text):
        return "PASS"
    if (step.accept_no_job_after_empty_retry
            and "[runner] empty framed response attempt" in text
            and "Status: BUSY" in text
            and ("No apply calibration job active" in text
                 or "No reset job active" in text)):
        return "PASS"
    if step.expect_failure and expected_seen:
        return "PASS"
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


def selected_steps(suite: str) -> tuple[Step, ...]:
    if suite == "smoke":
        return SMOKE_STEPS
    if suite == "functional":
        return SMOKE_STEPS + FUNCTIONAL_STEPS
    if suite == "exhaustive":
        return SMOKE_STEPS + FUNCTIONAL_STEPS + EXHAUSTIVE_STEPS
    if suite == "targeted":
        return SMOKE_STEPS + TARGETED_STEPS
    if suite == "transfer":
        return SMOKE_STEPS + TRANSFER_STEPS
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
    payload, ok = strip_hilrun_frame(
        "noise\nHIL_BEGIN token=T1 seq=7\nStatus: OK\nold HILMARK text\n"
        "HIL_END token=T1 seq=7 status=OK elapsed_ms=3\n",
        "T1",
        "7",
    )
    if not ok or "old HILMARK text" not in payload:
        print("parser self-test FAILED: complete frame")
        return 1
    _, ok = strip_hilrun_frame("HIL_BEGIN token=T1 seq=7\n", "T1", "7")
    if ok:
        print("parser self-test FAILED: truncated frame accepted")
        return 1
    _, ok = strip_hilrun_frame(
        "HIL_BEGIN token=T1 seq=7\nHIL_END token=T2 seq=7 status=OK elapsed_ms=3\n",
        "T1",
        "7",
    )
    if ok:
        print("parser self-test FAILED: wrong token accepted")
        return 1
    _, ok = strip_hilrun_frame(
        "HIL_BEGIN token=T1 seq=7\nHIL_END token=T1 seq=8 status=OK elapsed_ms=3\n",
        "T1",
        "7",
    )
    if ok:
        print("parser self-test FAILED: wrong sequence accepted")
        return 1
    payload, ok = strip_hilrun_frame(
        "HIL_BEGIN token=T1 seq=7\nHIL_END token=T1 seq=7 status=INVALID_PARAM elapsed_ms=0\n",
        "T1",
        "7",
    )
    invalid_step = Step("hilrun T1 7 hilrun nested", ("INVALID_PARAM",),
                        "nested hilrun rejection", expect_failure=True)
    if not ok or classify_step(payload, invalid_step) != "PASS":
        print("parser self-test FAILED: nested/invalid frame status")
        return 1
    recovered_start = (
        "[runner] empty framed response attempt 1; retrying\n"
        "[I] startResetJob(): BUSY\n"
        "  Status: BUSY (code=11, detail=0)\n"
        "  Message: Another fixed-step job is active\n"
        "[runner] frame_status=BUSY frame_elapsed_ms=1\n"
        "Status: BUSY\n"
    )
    recovered_step = Step(
        "reset_start",
        ("startResetJob", "IN_PROGRESS"),
        "recovered reset start",
        accept_busy_after_empty_retry=True,
    )
    strict_step = Step("reset_start", ("startResetJob", "IN_PROGRESS"), "strict reset start")
    if classify_step(recovered_start, recovered_step) != "PASS":
        print("parser self-test FAILED: recovered start BUSY was not accepted")
        return 1
    if classify_step(recovered_start, strict_step) != "FAIL":
        print("parser self-test FAILED: strict start accepted recovered BUSY")
        return 1
    recovered_done = (
        "[runner] empty framed response attempt 1; retrying\n"
        "[I] pollConfigReplayJob(6): BUSY\n"
        "  Status: BUSY (code=11, detail=0)\n"
        "  Message: No apply calibration job active\n"
        "[runner] frame_status=BUSY frame_elapsed_ms=1\n"
        "Status: BUSY\n"
    )
    recovered_done_step = Step(
        "apply_step 6",
        ("pollConfigReplayJob", "OK"),
        "recovered apply completion",
        accept_no_job_after_empty_retry=True,
    )
    strict_done_step = Step("apply_step 6", ("pollConfigReplayJob", "OK"),
                            "strict apply completion")
    if classify_step(recovered_done, recovered_done_step) != "PASS":
        print("parser self-test FAILED: recovered completion BUSY was not accepted")
        return 1
    if classify_step(recovered_done, strict_done_step) != "FAIL":
        print("parser self-test FAILED: strict completion accepted recovered BUSY")
        return 1
    print("parser self-test PASSED")
    return 0


def print_plan(steps: Iterable[Step], soak_seconds: float,
               benchmark_count: int, include_not_run: bool,
               suite: str = "smoke") -> None:
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
        soak_steps = soak_steps_for_suite(suite)
        print(f"  {'<soak loop>':<16} # soak for {soak_seconds:.0f}s using "
              f"{len(soak_steps)} {suite} soak commands")
    if include_not_run:
        print(f"  {'<not-run rows>':<16} # {len(STATIC_NOT_RUN_STEPS)} fixture/tooling limitations")


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


def strip_hilrun_frame(text: str, token: str, seq: str) -> tuple[str, bool]:
    clean = clean_output(text)
    begin_line = f"HIL_BEGIN token={token} seq={seq}"
    end_re = hilrun_end_re(token, seq)
    begin_index = clean.find(begin_line)
    end_match = end_re.search(clean)
    if begin_index < 0 or end_match is None:
        return clean + f"\n[runner] missing HIL frame token={token} seq={seq}", False
    payload_start = begin_index + len(begin_line)
    payload = clean[payload_start:end_match.start()].strip("\n")
    status = end_match.group(1)
    elapsed_ms = end_match.group(2)
    payload += f"\n[runner] frame_status={status} frame_elapsed_ms={elapsed_ms}"
    if status not in ("OK", "IN_PROGRESS"):
        payload += f"\nStatus: {status}"
    return payload, True


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
    output = ""
    retry_notes: list[str] = []
    marker_missing = False
    attempts = max(0, args.empty_retries) + 1
    for attempt in range(attempts):
        stale = drain_input(serial_port, args.drain_before_command_s)
        token = f"{args.frame_prefix}{time.monotonic_ns()}{attempt}"
        seq = str(attempt)
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
            if not frame_output.strip() and attempt < attempts - 1:
                retry_notes.append(
                    f"[runner] empty framed response attempt {attempt + 1}; retrying"
                )
                output = ""
                marker_missing = False
                if args.command_pause_s > 0.0:
                    time.sleep(args.command_pause_s)
                continue
            output, frame_ok = strip_hilrun_frame(frame_output, token, seq)
            marker_missing = not frame_ok
            if frame_ok:
                trailer = drain_input(serial_port, args.post_frame_drain_s)
                if clean_output(trailer).strip() and not PROMPT_RE.fullmatch(
                    clean_output(trailer).strip()
                ):
                    output += "\n[runner] drained trailing serial input after frame:\n"
                    output += trailer
        if stale.strip():
            output = "[runner] drained stale serial input before command:\n" + stale + "\n" + output
        if output.strip() or attempt == attempts - 1:
            break
        if args.command_pause_s > 0.0:
            time.sleep(args.command_pause_s)
    elapsed = time.monotonic() - start
    if retry_notes:
        output = "\n".join(retry_notes + [output])
    verdict = "UNKNOWN" if marker_missing else classify_step(output, step)
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
        elif result.verdict == "PASS":
            current = 0
    return max_seen


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
    actual_command_count = len(results) - stored_soak_count + actual_soak_count
    with path.open("w", encoding="utf-8", newline="\n") as f:
        f.write("# INA228 HIL Validation Report\n\n")
        f.write(f"- Date/time: {started_at.isoformat()} to {ended_at.isoformat()}\n")
        f.write(f"- Elapsed: {elapsed:.1f} s\n")
        f.write(f"- Port: {args.port or 'NOT RUN'}\n")
        f.write(f"- Baud: {args.baud}\n")
        f.write(f"- Suite: {args.suite}\n")
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
        f.write(f"- Commands executed: {actual_command_count}\n")
        f.write(f"- Commands recorded in detail: {len(results)}\n")
        f.write(f"- Soak commands executed: {actual_soak_count}\n")
        f.write(f"- Soak rows recorded in detail: {stored_soak_count}\n")
        f.write(f"- Recorded command latency min/mean/max: {min_elapsed:.3f} / "
                f"{mean_elapsed:.3f} / {max_elapsed:.3f} s\n\n")
        f.write(f"- Maximum consecutive FAIL verdicts: {max_consecutive_failures(results)}\n\n")
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
            f.write(f"- Soak PASS row storage stride: every {args.soak_store_every} "
                    "PASS row(s), plus all FAIL/UNKNOWN rows\n")
            if soak_summary is not None:
                f.write(f"- Soak verdict counts: PASS={soak_summary.pass_count}, "
                        f"FAIL={soak_summary.fail_count}, "
                        f"UNKNOWN={soak_summary.unknown_count}\n")
                f.write("- Empty framed response retries: "
                        f"{soak_summary.empty_response_retries}\n")
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
            if soak_summary is not None and soak_summary.command_empty_retries:
                f.write("- Empty framed response retry counts:\n")
                for command, count in sorted(soak_summary.command_empty_retries.items()):
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
    summary = SoakSummary(requested_s=soak_seconds)
    deadline = time.monotonic() + soak_seconds
    index = 0
    soak_steps = soak_steps_for_suite(args.suite)
    while time.monotonic() < deadline:
        step = soak_steps[index % len(soak_steps)]
        result = run_step(serial_port, step, args)
        summary.record(result)
        store_result = (
            result.verdict != "PASS"
            or args.soak_store_every <= 1
            or summary.total % args.soak_store_every == 0
        )
        if store_result:
            results.append(result)
            summary.stored += 1
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
        failures = 0
        for _ in range(args.benchmark_count):
            result = run_step(serial_port, step, args)
            results.append(result)
            latencies.append(result.elapsed_s)
            if result.verdict == "FAIL":
                failures += 1
            if args.verbose:
                print(f"[{result.verdict}] {step.command} ({step.label}) "
                      f"{result.elapsed_s:.3f}s")
            pause_s = max(args.command_pause_s, result.step.pause_after_s)
            if pause_s > 0.0:
                time.sleep(pause_s)
        if latencies:
            mean = sum(latencies) / len(latencies)
            print(f"[BENCH] {step.command}: count={len(latencies)} failures={failures} "
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

    with serial.Serial(args.port, args.baud, timeout=0.05) as serial_port:
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

    ended_at = dt.datetime.now().astimezone()
    if args.include_not_run or report_path is not None:
        results.extend(not_run_results(soak_seconds))
    if transcript_path is not None:
        write_transcript(transcript_path, results, boot_output)
    if report_path is not None:
        write_report(report_path, args, results, started_at, ended_at, transcript_path,
                     soak_seconds, boot_output, soak_summary)

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
                        help="Require the hilrun framed CLI path")
    parser.add_argument("--max-frame-bytes", type=int, default=8192,
                        help="Maximum bytes to read for one framed command response")
    parser.add_argument("--frame-prefix", default="HIL",
                        help="Prefix for generated HIL framing tokens")
    parser.add_argument("--marker-retries", type=int, default=1,
                        help="Bounded hilmark retry count when a command marker is not observed")
    parser.add_argument("--prompt-token",
                        help="Optional prompt token that ends response reads early")
    parser.add_argument("--empty-retries", type=int, default=0,
                        help="Bounded retries when a command returns no serial output")
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
                        help="Stop soak after the first FAIL or UNKNOWN row")
    parser.add_argument("--verbose", action="store_true",
                        help="Print command responses during hardware run")
    args = parser.parse_args(argv)

    if args.timeout_s <= 0.0 or args.idle_s <= 0.0 or args.boot_settle_s < 0.0:
        parser.error("timeouts must be positive and boot settle must be nonnegative")
    if (args.boot_capture_s < 0.0 or args.command_pause_s < 0.0
            or args.drain_before_command_s < 0.0 or args.post_frame_drain_s < 0.0):
        parser.error("boot capture, command pause, and drain times must be nonnegative")
    if args.empty_retries < 0 or args.marker_retries < 0 or args.benchmark_count < 0:
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

    soak_seconds = args.soak_seconds
    if args.soak_hours > 0.0:
        soak_seconds = args.soak_hours * 3600.0

    if args.parser_self_test:
        return parser_self_test()
    if args.dry_run:
        print_plan(selected_steps(args.suite), soak_seconds,
                   args.benchmark_count, args.include_not_run, args.suite)
        return 0
    if not args.port:
        parser.error("--port is required unless --dry-run or --parser-self-test is used")
    return run_serial(args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
