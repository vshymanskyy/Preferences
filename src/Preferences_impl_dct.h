
/*
 * Place the DCT at the end of Flash memory by default.
 *
 * Module size is 4k
 *  max values in each moudle = 4024 / (16 + 128+4) = 27
 * Max module number is 6
 *  if backup enabled, the total module number is 6 + 1*6 = 12, the size is 48k
 *  if wear leveling enabled, the total module number is 6 + 2*6 + 3*6 = 36, the size is 144k"
 */

#ifndef DCT_FLASH_SIZE
  #define DCT_FLASH_SIZE              0x200000
#endif
#ifndef DCT_MODULE_NUM
  #define DCT_MODULE_NUM              6
#endif
#ifndef DCT_VARIABLE_NAME_SIZE
  #define DCT_VARIABLE_NAME_SIZE      16
#endif
#ifndef DCT_VARIABLE_VALUE_SIZE
  #define DCT_VARIABLE_VALUE_SIZE     (128+4)
#endif
#ifndef DCT_WEAR_LEVELING
  #define DCT_WEAR_LEVELING           0
#endif
#ifndef DCT_BACKUP
  #define DCT_BACKUP                  1
#endif

static bool gPrefsDctInit;

bool Preferences::begin(const char * name, bool readOnly){
    if(_started || !name || !strlen(name)){
        return false;
    }
    _readOnly = readOnly;

    int32_t ret;
    if (!gPrefsDctInit) {
#ifdef DCT_BEGIN_ADDR
        size_t addr = DCT_BEGIN_ADDR;
#else
        size_t dct_size = DCT_MODULE_NUM;
#if DCT_WEAR_LEVELING
        dct_size *= 6;
#elif DCT_BACKUP
        dct_size *= 2;
#endif
        size_t addr = DCT_FLASH_SIZE - (dct_size * 4096);
#endif

        ret = dct_init(addr, DCT_MODULE_NUM,
                       DCT_VARIABLE_NAME_SIZE, DCT_VARIABLE_VALUE_SIZE,
                       DCT_BACKUP, DCT_WEAR_LEVELING);
        if (DCT_SUCCESS != ret) {
            LOG_E("DCT not initialized");
            return false;
        }
        gPrefsDctInit = true;
    }

    ret = dct_register_module((char*)name);
    if (DCT_SUCCESS != ret) {
        LOG_E("Cannot register module");
        return false;
    }
    ret = dct_open_module(&_handle, (char*)name);
    _started = (DCT_SUCCESS == ret);
    if (!_started) {
        LOG_E("Cannot open module");
        return false;
    }
    return _started;
}

void Preferences::end(){
    if(!_started){
        return;
    }
    if (DCT_SUCCESS != dct_close_module(&_handle)) {
        LOG_E("Cannot close module");
    }
    _started = false;
}

/*
 * Clear all keys in opened preferences
 *
 * NOTE: DCT library does not provide API to clear all values in an open module.
 *       So we do this: close, unregister, register, open
 * */

bool Preferences::clear(){
    if(!_started || _readOnly){
        return false;
    }

    char name[MODULE_NAME_SIZE+1];
    memcpy(name, _handle.module_name, sizeof(name));

    if (DCT_SUCCESS != dct_close_module(&_handle)) {
        LOG_E("Cannot close module");
        return false;
    }
    int32_t ret = dct_unregister_module(name);
    if (DCT_SUCCESS != ret) {
        LOG_E("Cannot unregister module");
        return false;
    }
    ret = dct_register_module(name);
    if (DCT_SUCCESS != ret) {
        LOG_E("Cannot re-register module");
        return false;
    }
    ret = dct_open_module(&_handle, name);
    _started = (DCT_SUCCESS == ret);
    if (!_started) {
        LOG_E("Cannot re-open module");
        return false;
    }
    return _started;
}

/*
 * Remove a key
 * */

bool Preferences::remove(const char * key){
    if(!_started || !key || _readOnly){
        return false;
    }
    int32_t ret = dct_delete_variable_new(&_handle, (char*)key);
    return (DCT_SUCCESS == ret);
}

/*
 * Put a key value
 * */

size_t Preferences::putBytes(const char* key, const void* buf, size_t len){
    if(!_started || !key || !buf || _readOnly){
        return 0;
    }

    if (DCT_SUCCESS == dct_set_variable_new(&_handle, (char*)key, (char*)buf, len)) {
        return len;
    }

    return 0;
}

bool Preferences::isKey(const char* key) {
    if(!_started || !key){
        return false;
    }

    uint16_t num = dct_get_variable_num(&_handle);
    for (int i=0; i<num; i++) {
        char name[DCT_VARIABLE_NAME_SIZE+1];
        if (DCT_SUCCESS == dct_get_variable_name(&_handle, i, (uint8_t*)name)) {
            name[DCT_VARIABLE_NAME_SIZE] = '\0';
            if (!strncmp(key, name, sizeof(name))) {
                return true;
            }
        }
    }

    return false;
}

/*
 * Get a key value
 * */

String Preferences::getString(const char* key, const String defaultValue){
    if(!_started || !key){
        return defaultValue;
    }

    char buff[DCT_VARIABLE_VALUE_SIZE+1];
    uint16_t len = sizeof(buff);
    int32_t ret = dct_get_variable_new(&_handle, (char*)key, buff, &len);
    if (DCT_SUCCESS == ret) {
        buff[len] = '\0';
        return String(buff);
    }
    //LOG_I("Get value %s error: %d", key, ret);
    return defaultValue;
}

size_t Preferences::getBytesLength(const char* key){
    if(!_started || !key){
        return -1;
    }

    char buff[DCT_VARIABLE_VALUE_SIZE+1];
    uint16_t len = sizeof(buff);
    int32_t ret = dct_get_variable_new(&_handle, (char*)key, buff, &len);
    if (DCT_SUCCESS == ret) {
        return len;
    }
    //LOG_I("Get value length %s error: %d", key, ret);
    return 0;
}

size_t Preferences::getBytes(const char* key, void * buf, size_t maxLen){
    if(!key){
        return 0;
    }

    uint16_t len = maxLen;
    int32_t ret = dct_get_variable_new(&_handle, (char*)key, (char*)buf, &len);
    if (DCT_SUCCESS == ret) {
        return len;
    }
    //LOG_I("Get value %s error: %d", key, ret);
    return 0;
}

size_t Preferences::freeEntries() {
    if(!_started){
        return 0;
    }
    return dct_remain_variable(&_handle);
}
