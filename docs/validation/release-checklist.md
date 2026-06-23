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

```bash
python -m py_compile tools/run_i2c_hil.py
python tools/run_i2c_hil.py --parser-self-test
python tools/run_i2c_hil.py --dry-run --suite targeted
python tools/run_i2c_hil.py --dry-run --suite transfer
python tools/check_core_timing_guard.py
python tools/check_cli_contract.py
python tools/check_idf_example_contract.py
python scripts/generate_version.py check
python -m platformio test -e native
python -m platformio run -e esp32s3dev
python -m platformio run -e esp32s2dev
python -m platformio pkg pack
```

Remove any generated `INA228-*.tar.gz` artifact after package validation.

Run pure ESP-IDF builds when ESP-IDF is installed:

```bash
idf.py -C examples/esp_idf/basic set-target esp32s3 build
idf.py -C examples/esp_idf/basic set-target esp32s2 build
```

## CI

Before merging or tagging, verify a completed CI run for the final branch or PR:

- Native tests pass.
- Guard scripts pass.
- HIL runner `py_compile` and parser self-test pass.
- PlatformIO ESP32-S3 build passes.
- PlatformIO ESP32-S2 build passes.
- Package validation passes.
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
