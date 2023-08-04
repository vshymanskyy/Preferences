
#include "Preferences.h"

//#define NVS_LOG

#define NVS_LOG_NAME "prefs"

#if defined(NVS_LOG) && defined(PARTICLE)
  static Logger prefsLog(NVS_LOG_NAME);

  #define LOG_D(...)        prefsLog.trace(__VA_ARGS__)
  #define LOG_I(...)        prefsLog.info(__VA_ARGS__)
  #define LOG_W(...)        prefsLog.warn(__VA_ARGS__)
  #define LOG_E(...)        prefsLog.error(__VA_ARGS__)
#elif defined(NVS_LOG) && defined(ARDUINO)
  #define LOG_D(fmt, ...)   { Serial.printf("%6lu [%s] DEBUG: ", millis(),  NVS_LOG_NAME); Serial.printf(fmt "\n", ##__VA_ARGS__); }
  #define LOG_I(fmt, ...)   { Serial.printf("%6lu [%s] INFO: ",  millis(),  NVS_LOG_NAME); Serial.printf(fmt "\n", ##__VA_ARGS__); }
  #define LOG_W(fmt, ...)   { Serial.printf("%6lu [%s] WARN: ",  millis(),  NVS_LOG_NAME); Serial.printf(fmt "\n", ##__VA_ARGS__); }
  #define LOG_E(fmt, ...)   { Serial.printf("%6lu [%s] ERROR: ", millis(),  NVS_LOG_NAME); Serial.printf(fmt "\n", ##__VA_ARGS__); }
#else
  #define LOG_D(fmt, ...)
  #define LOG_I(fmt, ...)
  #define LOG_W(fmt, ...)
  #define LOG_E(fmt, ...)
#endif

#if defined(NVS_USE_DCT)
  #include "Preferences_impl_dct.h"
#else
  #include "Preferences_impl_fs.h"
#endif

Preferences::Preferences()
    : _started(false)
    , _readOnly(false)
{}

Preferences::~Preferences(){
    end();
}

/*
 * Put a key value
 * */

size_t Preferences::putChar(const char* key, int8_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putUChar(const char* key, uint8_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putShort(const char* key, int16_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putUShort(const char* key, uint16_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putInt(const char* key, int32_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putUInt(const char* key, uint32_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putLong(const char* key, int32_t value){
    return putInt(key, value);
}

size_t Preferences::putULong(const char* key, uint32_t value){
    return putUInt(key, value);
}

size_t Preferences::putLong64(const char* key, int64_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putULong64(const char* key, uint64_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putFloat(const char* key, const float_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putDouble(const char* key, const double_t value){
    return putBytes(key, (void*)&value, sizeof(value));
}

size_t Preferences::putBool(const char* key, const bool value){
    return putUChar(key, (uint8_t) (value ? 1 : 0));
}

size_t Preferences::putString(const char* key, const char* value){
    return putBytes(key, (void*)value, strlen(value));
}

size_t Preferences::putString(const char* key, const String value){
    return putBytes(key, value.c_str(), value.length());
}

PreferenceType Preferences::getType(const char* key) {
    (void)key;
    return PT_INVALID;
}

/*
 * Get a key value
 * */

int8_t Preferences::getChar(const char* key, const int8_t defaultValue){
    int8_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

uint8_t Preferences::getUChar(const char* key, const uint8_t defaultValue){
    uint8_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

int16_t Preferences::getShort(const char* key, const int16_t defaultValue){
    int16_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

uint16_t Preferences::getUShort(const char* key, const uint16_t defaultValue){
    uint16_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

int32_t Preferences::getInt(const char* key, const int32_t defaultValue){
    int32_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

uint32_t Preferences::getUInt(const char* key, const uint32_t defaultValue){
    uint32_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

int32_t Preferences::getLong(const char* key, const int32_t defaultValue){
    return getInt(key, defaultValue);
}

uint32_t Preferences::getULong(const char* key, const uint32_t defaultValue){
    return getUInt(key, defaultValue);
}

int64_t Preferences::getLong64(const char* key, const int64_t defaultValue){
    int64_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

uint64_t Preferences::getULong64(const char* key, const uint64_t defaultValue){
    uint64_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

float_t Preferences::getFloat(const char* key, const float_t defaultValue) {
    float_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

double_t Preferences::getDouble(const char* key, const double_t defaultValue) {
    double_t value = defaultValue;
    getBytes(key, (void*) &value, sizeof(value));
    return value;
}

bool Preferences::getBool(const char* key, const bool defaultValue) {
    return getUChar(key, defaultValue ? 1 : 0) == 1;
}

size_t Preferences::getString(const char* key, char* value, const size_t maxLen){
    if(!_started || !key || !value || !maxLen){
        return 0;
    }
    size_t len = getBytes(key, value, maxLen-1);
    value[len] = '\0';
    return len;
}

