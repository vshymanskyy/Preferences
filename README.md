# Preferences
Preferences library for **ESP8266** and **Particle Gen3** devices. **ESP32**-compatible API.

[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner-direct-single.svg)](https://stand-with-ukraine.pp.ua)

## How does it work?

Preferences are stored in the internal flash filesystem in a bunch of `/nvs/{namespace}/{property}` files.  
Filesystem should handle flash wearing, bad sectors and atomic `rename` file operation.  
`LittleFS` handles all that, so this is the default FS driver for ESP8266.  
Particle Gen3 devices also operate on `LittleFS` filesystem.

## Supported devices

- Arduino ESP8266 with LittleFS or SPIFFS
- Particle Argon, Boron, Xenon, Tracker, BSOM

Can be easily adjusted for any device with a reliable and POSIX-compatible filesystem

## API

Check out ESP32 [Preferences library](https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/preferences.html) API.
Differences:
- `partition_label` argument is not supported in `begin()`
- `getType()` and `freeEntries()` methods are not supported (returning dummy values)
