
#ifndef _PREFERENCES_SETUP_H_
#define _PREFERENCES_SETUP_H_

#if defined(NVS_USE_POSIX) || defined(NVS_USE_LITTLEFS) || defined(NVS_USE_SPIFFS) || defined(NVS_USE_DCT) || defined(NVS_USE_SFUD)
  // OK, use it.
#elif defined(NVS_USE_DUMMY)
  // OK, warn about it.
  #warning "Dummy implementation is used (won't store any values)"
#elif defined(PARTICLE) && (PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_P2 || PLATFORM_ID == PLATFORM_MSOM)
  #define NVS_USE_POSIX
#elif defined(PARTICLE) && (PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_TRACKER)
  #define NVS_USE_POSIX
#elif defined(ARDUINO) && (defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_MKRVIDOR4000))
  #error "Please use the native WiFiNINA library (WiFiPreferences.h)"
#elif defined(ARDUINO) && (defined(ESP8266) || defined(ARDUINO_ARCH_RP2040))
  #define NVS_USE_LITTLEFS    // Use LittleFS by default
#elif defined(ARDUINO) && defined(ARDUINO_ARCH_AMEBAD)
  #define NVS_USE_DCT
#elif defined(ARDUINO) && (defined(ARDUINO_SEEED_WIO_TERMINAL) || defined(SEEED_WIO_TERMINAL) || defined(WIO_TERMINAL))
  #define NVS_USE_SFUD
#elif defined(ARDUINO) && defined(ESP32)
  #error "For ESP32 devices, please use the native Preferences library"
#else
  #error "FS API not implemented for the target platform"
#endif

#if defined(NVS_USE_DCT)
    extern "C" {
      #include <dct.h>
    }
#elif defined(NVS_USE_SFUD)
    #include <sfud.h>
#endif

#endif
