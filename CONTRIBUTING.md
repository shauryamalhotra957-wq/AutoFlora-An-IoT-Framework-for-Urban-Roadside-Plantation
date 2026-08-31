# Contributing

## Before changing firmware

- Keep pump power disconnected while uploading or testing sensor logic.
- Preserve low-tank and sensor-failure protections; a bad reading must not silently enable irrigation.
- Update the pin map and calibration notes with wiring or threshold changes.

## Verification

Compile for an Arduino Uno in the Arduino IDE, then follow [docs/COMMISSIONING_CHECKLIST.md](docs/COMMISSIONING_CHECKLIST.md). Record calibration values and controlled-pump observations in the pull request.

Never commit credentials, deployment coordinates, or private telemetry.
