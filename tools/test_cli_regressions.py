#!/usr/bin/env python3
"""Execute extracted CLI reporting/parsing code on the host; no hardware claims."""
from __future__ import annotations

import pathlib
import re
import subprocess
import tempfile
import unittest

from check_idf_example_contract import command_dispatch_block, function_block

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCES = {
    "arduino": ROOT / "examples/01_basic_bringup_cli/main.cpp",
    "idf": ROOT / "examples/esp_idf/basic/main/main.cpp",
}
QUERIES = ("convtime", "averaging", "adcrange", "cal", "tempco", "tempcomp", "delay")

PREAMBLE = r'''
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include "INA228/INA228.h"
using namespace INA228;
struct String {
  std::string value;
  String(const char* text): value(text) {}
  size_t length() const { return value.length(); }
  char charAt(size_t i) const { return value[i]; }
  const char* c_str() const { return value.c_str(); }
};
struct Console {
  template<class... T> void printf(const char* fmt, T... args) { std::printf(fmt, args...); }
  void println(const char* text = "") { std::puts(text); }
} Serial;
#define LOGI(...) std::printf(__VA_ARGS__)
#define LOG_COLOR_YELLOW ""
#define LOG_COLOR_RESET ""
#define LOG_COLOR_RESULT(ok) ""
struct Device {
  Config cfg{};
  bool initialized = false;
  unsigned configReads = 0;
  bool isInitialized() const { return initialized; }
  const Config& getConfig() { ++configReads; return cfg; }
  float currentLsb() const { return 0.000005f; }
} device;
Err hilCommandStatus = Err::OK;
void printStatus(Status st) { hilCommandStatus = st.code; }
const char* convTimeToStr(ConvTime) { return "1052"; }
const char* avgToStr(Averaging) { return "1"; }
const char* adcRangeToStr(AdcRange) { return "163.84"; }
const char* boolStr(bool value) { return value ? "yes" : "no"; }
const char* log_bool_str(bool value) { return boolStr(value); }
'''


def host_program(source: str, platform: str) -> str:
    parts = [PREAMBLE]
    signature = "const String&" if platform == "arduino" else "const char*"
    parts.append(f"bool parseU32({signature} token, uint32_t& out) {{"
                 + function_block(source, "parseU32") + "}")
    for query in QUERIES:
        parts.append(f"void query_{query}() {{" + command_dispatch_block(source, query) + "}")

    settings = function_block(source, "printSettings")
    match = re.search(r'(?:Serial\.printf|std::printf)\("  Calibration:', settings)
    assert match is not None
    statement = settings[match.start():settings.index(";", match.start()) + 1]
    parts.append("void settingsCalibration() { SettingsSnapshot snap{}; "
                 "const Config& cfg = device.getConfig(); " + statement + "}")

    if platform == "arduino":
        reporter = function_block(source, "runSelfTest").split(
            'Serial.println("=== INA228 selftest', 1)[0]
        parts.append("void failingSelftest() {" + reporter
                     + 'reportCheck("injected failure", false, ""); }')
    else:
        parts.append("struct SelftestStats { unsigned pass=0, fail=0, skip=0; };")
        parts.append("void reportSelftest(SelftestStats& stats, const char* name, "
                     "bool passed, const char* note) {"
                     + function_block(source, "reportSelftest") + "}")
        parts.append('void failingSelftest() { SelftestStats stats; '
                     'reportSelftest(stats, "injected failure", false, ""); }')

    parts.append("int main() {")
    for query in QUERIES:
        parts.append(f"hilCommandStatus=Err::OK; device.configReads=0; query_{query}();"
                     f' std::printf("\\nQUERY {query} %d %u\\n", '
                     "hilCommandStatus == Err::NOT_INITIALIZED, device.configReads);")
    parts.append(r'''
      device.initialized=true;
      device.cfg.calibration.mode=CalibrationMode::EXPLICIT_CURRENT_LSB;
      device.cfg.calibration.shuntMicroOhms=25000;
      device.cfg.calibration.maxCurrentMilliAmps=2500;
      query_cal(); settingsCalibration();
      device.cfg.calibration.mode=CalibrationMode::NONE;
      device.cfg.shuntResistanceOhm=0.01f;
      device.cfg.maxExpectedCurrentA=3.0f;
      query_cal(); settingsCalibration();
      failingSelftest();
      std::printf("SELFTEST %d\n", hilCommandStatus != Err::OK);
      const char* invalid[] = {" -1", "\t-0", "\n-1", "", "4294967296", "2garbage"};
      for (const char* text : invalid) {
        uint32_t value=123;
        const bool accepted=parseU32(text, value);
        std::printf("PARSE_INVALID %d %u\n", accepted, value);
      }
      uint32_t value=0;
      const bool accepted=parseU32(" 0xFF", value);
      std::printf("PARSE_VALID %d %u\n", accepted, value);
    }
    ''')
    return "\n".join(parts)


class CliRegressions(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.source = {key: path.read_text(encoding="utf-8") for key, path in SOURCES.items()}
        cls.output = {}
        with tempfile.TemporaryDirectory(prefix="ina228-cli-") as directory:
            for platform, source in cls.source.items():
                cpp = pathlib.Path(directory) / f"{platform}.cpp"
                exe = pathlib.Path(directory) / f"{platform}.exe"
                cpp.write_text(host_program(source, platform), encoding="utf-8")
                subprocess.run(["g++", "-std=c++17", "-I", str(ROOT / "include"),
                                str(cpp), "-o", str(exe)], check=True, timeout=60)
                cls.output[platform] = subprocess.check_output([str(exe)], text=True, timeout=10)

    def test_uninitialized_queries_reject_before_reading_defaults(self) -> None:
        for platform, output in self.output.items():
            for query in QUERIES:
                with self.subTest(platform=platform, query=query):
                    self.assertIn(f"QUERY {query} 1 0", output)

    def test_calibration_reports_fixed_and_legacy_units(self) -> None:
        for platform, output in self.output.items():
            with self.subTest(platform=platform):
                self.assertEqual(output.count("Rshunt=0.025000 ohm  MaxCurrent=2.500000 A"), 2)
                self.assertEqual(output.count("Rshunt=0.010000 ohm  MaxCurrent=3.000000 A"), 2)

    def test_selftest_failure_reaches_hil_status(self) -> None:
        for platform, output in self.output.items():
            with self.subTest(platform=platform):
                self.assertIn("SELFTEST 1", output)

    def test_unsigned_parser_rejects_whitespace_hidden_minus(self) -> None:
        for platform, output in self.output.items():
            with self.subTest(platform=platform):
                self.assertEqual(output.count("PARSE_INVALID 0 123"), 6)
                self.assertIn("PARSE_VALID 1 255", output)

    def test_stress_loops_allow_lower_priority_tasks_to_run(self) -> None:
        for platform, source in self.source.items():
            for function in ("runStress", "runStressMix"):
                with self.subTest(platform=platform, function=function):
                    body = function_block(source, function)
                    self.assertRegex(body, r"\b(?:delay|sleepMs)\(1\)")
                    self.assertNotRegex(body, r"\b(?:yield|taskYIELD)\(\)")

    def test_failed_arduino_bus_setup_still_prints_prompt(self) -> None:
        setup = function_block(self.source["arduino"], "setup")
        # Every early return must already have exposed the console prompt.
        for prefix in setup.split("return;")[:-1]:
            self.assertIn("printHelp();", prefix)
            self.assertIn("cli::printPrompt();", prefix)

    def test_ready_step_usage_matches_nonzero_budget(self) -> None:
        for platform, source in self.source.items():
            with self.subTest(platform=platform):
                self.assertIn("Usage: ready_step <1..255>", source)
                self.assertNotIn("Usage: ready_step <0..255>", source)

    def test_mode_locals_are_initialized(self) -> None:
        for platform, source in self.source.items():
            with self.subTest(platform=platform):
                self.assertNotRegex(source, r"\bMode\s+mode\s*;")


if __name__ == "__main__":
    unittest.main()
