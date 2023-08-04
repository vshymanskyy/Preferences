
#if defined(NVS_PATH)
  // OK, use it.
#elif defined(NVS_USE_WIFININA)
  #define NVS_PATH "/fs/nvs"
#else
  #define NVS_PATH "/nvs"
#endif

#if defined(NVS_USE_LITTLEFS)
  #define NVS_ATOMIC_CLEAR
#endif

#define NVS_STAGING_FN  "\a_new?"
#define NVS_DELETED_FN  "\a_del?"

#if defined(NVS_USE_POSIX)
  #include "prefs_impl_posix.h"
#elif defined(NVS_USE_LITTLEFS) || defined(NVS_USE_SPIFFS)
  #include "prefs_impl_arduino.h"
#elif defined(NVS_USE_WIFININA)
  #include "prefs_impl_wifinina.h"
#elif defined(NVS_USE_DUMMY)
  #include "prefs_impl_dummy.h"
#endif

static bool gPrefsFsInit;

bool Preferences::begin(const char * name, bool readOnly){
    if(_started || !name || !strlen(name)){
        return false;
    }
    _readOnly = readOnly;

    if (!gPrefsFsInit) {
        if (!_fs_init()) {
            LOG_E("FS not initialized");
            return false;
        }
        if (!_fs_mkdir(NVS_PATH)) {
            LOG_E("Cannot create NVS_PATH");
            return false;
        }
#if defined(NVS_ATOMIC_CLEAR)
        String deleted = String(NVS_PATH) + String("/" NVS_DELETED_FN);
        if (_fs_exists(deleted.c_str())) {
            if (!_fs_clean_dir((deleted + "/").c_str())) {
                LOG_E("Cannot cleanup a deleted namespace");
            }
        }
#endif
        gPrefsFsInit = true;
    }

    String p = String(NVS_PATH) + String("/") + name;
    if (_fs_mkdir(p.c_str())) {
        _started = true;
        _path = String(NVS_PATH) + String("/") + name + String("/");
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

#if defined(NVS_ATOMIC_CLEAR)
    String path = _path.substring(0, _path.length()-1);
    String deleted = String(NVS_PATH) + String("/" NVS_DELETED_FN);
    if (_fs_rename(path.c_str(), deleted.c_str())) {
        return _fs_clean_dir((deleted + "/").c_str());
    } else {
        LOG_W("Cannot rename directory");
        return _fs_clean_dir(path.c_str());
    }
#else
    if (_fs_clean_dir(_path.c_str())) {
        String p = _path.substring(0, _path.length()-1);
        return _fs_mkdir(p.c_str());
    }
    return false;
#endif
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

size_t Preferences::putBytes(const char* key, const void* buf, size_t len){
    if(!_started || !key || !buf || _readOnly){
        return 0;
    }

    String path = _path + key;

    if (_fs_exists(path.c_str())) {
#if defined(NVS_USE_SPIFFS) || defined(NVS_USE_WIFININA)
        return _fs_update(path.c_str(), buf, len);
#else
        if (_fs_verify(path.c_str(), buf, len)) {
            LOG_I("data matches, skip writing to %s", path.c_str());
            return len;
        }

        String next = _path + NVS_STAGING_FN;

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

    if(len < 0){
        LOG_I("value not found: %s", key);
        return 0;
    }
    if((size_t)len > maxLen){
        LOG_W("not enough space in buffer: %u < %u", maxLen, len);
        return 0;
    }

    return _fs_read(path.c_str(), buf, len);
}

size_t Preferences::freeEntries() {
    return 1000;
}
