# Release Checklist

`library.json` is the version source of truth. `include/INA228/Version.h` is
generated from it and committed.

## Version

- Update `library.json`.
- Run `python scripts/generate_version.py sync`.
- Run `python scripts/generate_version.py check`.
- Update `CHANGELOG.md`.
- Update README/docs/examples for API or behavior changes.
- Confirm `idf_component.yml` version is consistent.

## Local Checks

Run and record:

```powershell
Get-ChildItem tools/*.py | ForEach-Object { python -m py_compile $_.FullName }
python -m py_compile scripts/generate_version.py
python tools/run_i2c_hil.py --parser-self-test
python tools/run_i2c_hil.py --dry-run --suite exhaustive --include-not-run --benchmark-count 100
python tools/test_run_i2c_hil_parser.py
python tools/check_core_timing_guard.py
python tools/check_owner_contract.py
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python scripts/generate_version.py check
.\scripts\pio.cmd test -e native
.\scripts\pio.cmd run -e esp32s3dev
.\scripts\pio.cmd run -e esp32s2dev
.\scripts\pio.cmd pkg pack
doxygen Doxyfile
git diff --check
```

Use PlatformIO Core `6.1.19`. Inspect the package and compile its exported source
standalone under C++17. At minimum it must contain `library.json`,
`CMakeLists.txt`, `idf_component.yml`, `include/INA228/INA228.h`,
`include/INA228/Version.h`, and `src/INA228.cpp`; its README links must resolve
within the archive. Remove the generated `INA228-*.tar.gz` after validation.

Run pure ESP-IDF builds when ESP-IDF is installed:

```bash
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

## CI

Before merging or tagging, verify a completed CI run for the final branch or PR:

- Native tests pass.
- Guard scripts pass.
- Effective Arduino compile commands use GNU++17 and do not retain GNU++11.
- All Python tools byte-compile; the HIL parser self-test and exhaustive
  benchmark dry-run pass.
- Standalone HIL parser regressions pass, including fail-closed expected-error
  handling, frame identity/completion, stale/trailing-input isolation,
  failure-run accounting, and both firmware provenance profiles.
- PlatformIO ESP32-S3 build passes.
- PlatformIO ESP32-S2 build passes.
- Package validation passes.
- Doxygen completes with warnings treated as errors.
- Pure ESP-IDF `esp32s3` build passes.
- Pure ESP-IDF `esp32s2` build passes.
- Artifacts are present for native test results, static contract logs,
  PlatformIO package archive, and ESP-IDF build logs.

## Documentation And Claims

- README install and integration instructions are current.
- `docs/validation/validation-status.md` reflects current evidence.
- `docs/validation/hardware-evidence.md` lists checked-in HIL reports and
  marks partial evidence separately from release-grade validation.
- `docs/validation/hardware-validation-procedure.md` matches the CLI examples.
- The release HIL gate is one framed `exhaustive` run with no FAIL/UNKNOWN rows;
  `targeted` and `transfer` are diagnostic subsets, not additional gates.
- The release runner uses the matching `arduino` or `idf` profile and rejects a
  mismatched library version, exact 12-character commit identity, source
  status, or framework token.
- Wording separates implemented behavior, native tests, CI configuration,
  ESP-IDF build proof, and hardware validation.
- No production, field, 85 V safety, or hardware validation claims appear
  without dated logs.

## Tag

After merge and completed CI on the release commit:

```bash
git checkout main
git pull --ff-only
git tag -a vX.Y.Z -m "Release vX.Y.Z"
git push origin main vX.Y.Z
```
