# Preferences

[![Arduino Library Manager](https://img.shields.io/static/v1?label=Arduino&message=v2.1.0&logo=arduino&logoColor=white&color=blue)](https://www.ardu-badge.com/Preferences)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/vshymanskyy/library/Preferences.svg)](https://registry.platformio.org/packages/libraries/vshymanskyy/Preferences) 

Provides **ESP32**-compatible **Preferences** API for a wider variety of platforms:
- **ESP8266** using LittleFS
- **RP2040** boards with [Pico core](https://github.com/earlephilhower/arduino-pico)
- Arduino **Nano 33 IoT, MKR1010, MKR VIDOR** using WiFiNINA storage
- Particle Gen3 devices: **Argon, Boron, Xenon, Tracker, BSOM**

Available from: [`Arduino Library Manager`](https://www.arduino.cc/reference/en/libraries/preferences), [`PlatformIO`](https://registry.platformio.org/libraries/vshymanskyy/Preferences), [`Particle Build`](https://build.particle.io/libs/Preferences)

[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner-direct-single.svg)](https://stand-with-ukraine.pp.ua)

## How does it work?

```cpp
#include <Preferences.h>
Preferences prefs;

void setup() {
  Serial.begin(115200);
  prefs.begin("my-app");

  int counter = prefs.getInt("counter", 1); // default to 1
  Serial.printf("Reboot count: %d\n", counter);
  counter++;
  prefs.putInt("counter", counter);
}

void loop() {}
```

Preferences are stored in the internal flash filesystem in a bunch of `/nvs/{namespace}/{property}` files.  
Filesystem should handle flash wearing, bad sectors and atomic `rename` file operation.
- `LittleFS` handles all that, so this is the default FS driver for ESP8266. `SPIFFS` use is possible, but it is discouraged.
- Particle Gen3 devices also operate on a built-in `LittleFS` filesystem.
- Arduino Nano and MKR devices use the storage of the U-blox NINA module.

## API

Check out ESP32 [Preferences library](https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/preferences.html) API.
Differences:
- `partition_label` argument is not supported in `begin()`
- `getType()` and `freeEntries()` methods are not supported (returning dummy values)
- `putBytes()` and `putString()` allow writing empty values (length = 0)
- `put*()` and `get*()` operations **don't fail** if the existing value has a different type

## Known issues

- `clear()` is not working on Arduino Nano 33 IoT, MKR1010, MKR VIDOR. This cannot be implemented, as WiFiNINA storage doesn't provide any API to remove or enumerate directories ([along with other bugs](https://github.com/arduino-libraries/WiFiNINA/issues/created_by/vshymanskyy)). If you need to clear a namespace on these devices, you'll have to erase each key individually using `remove()`.
