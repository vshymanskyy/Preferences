
#include "Preferences.h"

#ifndef NVS_PATH
  #define NVS_PATH "/nvs"
#endif

#if defined(PARTICLE) && (PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_XENON)
  #define NVS_USE_POSIX
#elif defined(PARTICLE) && (PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_TRACKER)
  #define NVS_USE_POSIX
#elif defined(ARDUINO) && defined(ESP8266)
  #if !defined(NVS_USE_LITTLEFS) && !defined(NVS_USE_SPIFFS)
    #define NVS_USE_LITTLEFS    // Use LittleFS by default
    //#define NVS_USE_SPIFFS
  #endif
#elif defined(ARDUINO) && defined(ESP32)
  #error "For ESP32 devices, please use native Preferences library"
#else
  #error "FS API not implemented for target platform"
#endif

//#define NVS_LOG

#define NVS_LOG_NAME "prefs"

#if defined(NVS_LOG) && defined(PARTICLE)
  static Logger prefsLog(NVS_LOG_NAME);

  #define LOG_I(...)        prefsLog.info(__VA_ARGS__)
  #define LOG_W(...)        prefsLog.warn(__VA_ARGS__)
  #define LOG_E(...)        prefsLog.error(__VA_ARGS__)
#elif defined(NVS_LOG) && defined(ARDUINO)
  #define LOG_I(fmt, ...)   { Serial.printf("%010d [%s] INFO: ", millis(),  NVS_LOG_NAME); Serial.printf(fmt "\n", ##__VA_ARGS__); }
  #define LOG_W(fmt, ...)   { Serial.printf("%010d [%s] WARN: ", millis(),  NVS_LOG_NAME); Serial.printf(fmt "\n", ##__VA_ARGS__); }
  #define LOG_E(fmt, ...)   { Serial.printf("%010d [%s] ERROR: ", millis(),  NVS_LOG_NAME); Serial.printf(fmt "\n", ##__VA_ARGS__); }
#else
  #define LOG_I(fmt, ...)
  #define LOG_W(fmt, ...)
  #define LOG_E(fmt, ...)
#endif

#if defined(NVS_USE_POSIX)

#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

static bool _fs_init() {
    return true;
}

static bool _fs_mkdir(const char *path) {
    struct stat statbuf;

    if (stat(path, &statbuf) == 0) {
        if ((statbuf.st_mode & S_IFDIR) != 0) {
            return true;
        } else {
            LOG_E("mkdir failed, file exists %s", path);
            return false;
        }
    } else {
        if (errno != ENOENT) {
            // Error other than file does not exist
            LOG_E("stat filed errno=%d", errno);
            return false;
        }
    }

    // File does not exist (errno == 2)
    if (mkdir(path, 0777) == 0) {
        return true;
    } else {
        LOG_E("mkdir failed errno=%d", errno);
        return false;
    }
}

static bool _fs_verify(const char* path, const void* buf, int bufsize) {
    int fd = open(path, O_RDONLY);
    if (fd >= 0) {
        struct stat st;
        stat(path, &st);
        if (st.st_size == bufsize && bufsize <= 1024) {
            // Check if content is the same
            uint8_t tmp[bufsize];
            if (read(fd, tmp, bufsize) == bufsize) {
                if (!memcmp(buf, tmp, bufsize)) {
                    return true;
                }
            }
        }
    }
    return false;
}

static int _fs_create(const char* path, const void* buf, int bufsize) {
    int fd = open(path, O_WRONLY | O_CREAT);
    if (fd == -1) {
        return -1;
    }
    int len = write(fd, buf, bufsize);
    close(fd);
    return len;
}

static int _fs_read(const char* path, void* buf, int bufsize) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return -1;
    }
    int len = read(fd, buf, bufsize);
    close(fd);
    return len;
}

static int _fs_get_size(const char* path) {
    struct stat st;
    if (0 == stat(path, &st)) {
        return st.st_size;
    }
    return -1;
}

static bool _fs_exists(const char* path) {
    struct stat st;
    return (0 == stat(path, &st));
}

static bool _fs_rename(const char* from, const char* to) {
    return (0 == rename(from, to));
}

static bool _fs_unlink(const char* path) {
    return (0 == unlink(path));
}

static bool _fs_clean_dir(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return false;

    struct dirent entry;
    struct dirent *result;

    for (int ret = readdir_r(dir, &entry, &result);
         result != NULL && ret == 0;
         ret = readdir_r(dir, &entry, &result))
    {
        const char* name = entry.d_name;
        if (!strcmp(name, ".") || !strcmp(name, "..")) {
            continue;
        }
        String p = String(path) + name;
        if (0 != unlink(p)) {
            closedir(dir);
            return false;
        } else {
            LOG_I("erased %s", p.c_str());
        }
    }
    closedir(dir);
    return true;
}

#elif defined(NVS_USE_LITTLEFS) || defined(NVS_USE_SPIFFS)

#if defined(NVS_USE_LITTLEFS)
#include "LittleFS.h"
#define FS LittleFS
#else
#include "FS.h"
#define FS SPIFFS
#endif

static bool _fs_init() {
    bool res = FS.begin();
#ifdef NVS_USE_SPIFFS
    // Increase reliability for SPIFFS
    FS.check();
#endif
    return res;
}

static bool _fs_mkdir(const char *path) {
    // Path are automatically created as needed
    return true;
}

static bool verifyContent(File& f, const void* buf, int bufsize) {
    // TODO: read in chunks, remove this limitation
    if (f.size() == bufsize && bufsize <= 1024) {
        // Check if content is the same
        uint8_t tmp[bufsize];
        if (f.read((uint8_t*)tmp, bufsize) == bufsize) {
            if (!memcmp(buf, tmp, bufsize)) {
                return true;
            }
        }
    }
    return false;
}

static bool _fs_verify(const char* path, const void* buf, int bufsize) {
    if (File f = FS.open(path, "r")) {
        return verifyContent(f, buf, bufsize);
    }
    return false;
}

static int _fs_create(const char* path, const void* buf, int bufsize) {
    if (File f = FS.open(path, "w")) {
        return f.write((const uint8_t*)buf, bufsize);
    }
    return -1;
}

static int _fs_update(const char* path, const void* buf, int bufsize) {
    if (File f = FS.open(path, "r+")) {
        if (verifyContent(f, buf, bufsize)) {
            LOG_I("data matches, skip writing to %s", path);
            return bufsize;
        }
        if (f.size() <= bufsize) {
            f.seek(0, SeekSet);
            return f.write((const uint8_t*)buf, bufsize);
        }
    }
    return _fs_create(path, buf, bufsize);
}

static int _fs_read(const char* path, void* buf, int bufsize) {
    if (File f = FS.open(path, "r")) {
        return f.read((uint8_t*)buf, bufsize);
    }
    return -1;
}

static int _fs_get_size(const char* path) {
    if (File f = FS.open(path, "r")) {
        return f.size();
    }
    return -1;
}

static bool _fs_exists(const char* path) {
    return FS.exists(path);
}

static bool _fs_rename(const char* from, const char* to) {
    return FS.rename(from, to);
}

static bool _fs_unlink(const char* path) {
    return FS.remove(path);
}

static bool _fs_clean_dir(const char* path) {
    Dir dir = FS.openDir(path);
    while (dir.next()) {
#ifdef NVS_USE_SPIFFS
        String p = dir.fileName();
#else
        String p = String(path) + dir.fileName();
#endif
        FS.remove(p.c_str());
        LOG_I("erased %s", p.c_str());
    }
    return true;
}

#endif

Preferences::Preferences()
    : _started(false)
    , _readOnly(false)
{}

Preferences::~Preferences(){
    end();
}

bool Preferences::begin(const char * name, bool readOnly){
    if(_started || !name || !strlen(name)){
        return false;
    }
    _readOnly = readOnly;

    if (!_fs_init()) {
        return false;
    }

    if (_fs_mkdir(NVS_PATH)) {
        String p = String(NVS_PATH) + String("/") + name;
        if (_fs_mkdir(p.c_str())) {
            _started = true;
            _path = String(NVS_PATH) + String("/") + name + String("/");
        }
    }

    return _started;
}

void Preferences::end(){
    if(!_started){
        return;
    }
    _path = "";
    _started = false;
}

/*
 * Clear all keys in opened preferences
 * */

bool Preferences::clear(){
    if(!_started || _readOnly){
        return false;
    }

    return _fs_clean_dir(_path.c_str());
}

/*
 * Remove a key
 * */

bool Preferences::remove(const char * key){
    if(!_started || !key || _readOnly){
        return false;
    }
    String path = _path + key;
    return _fs_unlink(path.c_str());
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

size_t Preferences::putBytes(const char* key, const void* buf, size_t len){
    if(!_started || !key || !buf || !len || _readOnly){
        return 0;
    }

    String path = _path + key;

    if (_fs_exists(path.c_str())) {
#ifdef NVS_USE_SPIFFS
        return _fs_update(path.c_str(), buf, len);
#else
        if (_fs_verify(path.c_str(), buf, len)) {
            LOG_I("data matches, skip writing to %s", path.c_str());
            return len;
        }

        String next = _path + "\a_?\a_?";

        int written = _fs_create(next.c_str(), buf, len);

        if (_fs_rename(next.c_str(), path.c_str())) {
            return written;
        } else {
            return 0;
        }
#endif
    } else {
        return _fs_create(path.c_str(), buf, len);
    }
}

PreferenceType Preferences::getType(const char* key) {
    return PT_INVALID;
}

bool Preferences::isKey(const char* key) {
    if(!_started || !key){
        return false;
    }
    String path = _path + key;

    return _fs_exists(path.c_str());
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

String Preferences::getString(const char* key, const String defaultValue){
    if(!_started || !key){
        return defaultValue;
    }

    String path = _path + key;

    int len = _fs_get_size(path.c_str());

    // TODO: allocate on heap, remove this limitation
    if (len >= 0 && len <= 1024) {
        char buff[len+1];
        getBytes(key, buff, len);
        buff[len] = '\0';
        return String(buff);
    }
    return defaultValue;
}

size_t Preferences::getBytesLength(const char* key){
    if(!_started || !key){
        return -1;
    }

    String path = _path + key;

    int len = _fs_get_size(path.c_str());
    return (len >= 0) ? len : 0;
}

size_t Preferences::getBytes(const char* key, void * buf, size_t maxLen){
    if(!key){
        return 0;
    }
    String path = _path + key;

    int len = _fs_get_size(path.c_str());
    if(!len || !buf || !maxLen){
        return len;
    }

    if(len == -1){
        LOG_I("value not found: %s", key);
        return 0;
    }
    if(len > maxLen){
        LOG_W("not enough space in buffer: %u < %u", maxLen, len);
        return 0;
    }

    return _fs_read(path.c_str(), buf, len);
}

size_t Preferences::freeEntries() {
    return 1000;
}
