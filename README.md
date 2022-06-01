# Preferences
Preferences library for **ESP8266** and **Particle Gen3** devices. **ESP32**-compatible.

## Supported devices

- Arduino ESP8266 with LittleFS or SPIFFS
- Particle Argon, Boron, Xenon, Tracker, BSOM
- Any device with a reliable and POSIX-compatible filesystem

## API

Check out ESP32 [Preferences library](https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/preferences.html) API.
Differences:
- `partition_label` argument is not supported in `begin()`
- `getType()` and `freeEntries()` methods are not supported
