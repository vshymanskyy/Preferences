
/*
 * Preferences are stored in a simple append-only log inside a fixed region
 * of external SPI flash, accessed via `sfud`. There is no real filesystem:
 * put() appends a new entry and drops any previous entry for the same key;
 * clear()/remove() rewrite the region without the dropped entries.
 */

#ifndef SFUD_NVS_FLASH_OFFSET
  #define SFUD_NVS_FLASH_OFFSET    0
#endif
#ifndef SFUD_NVS_FLASH_SIZE
  #define SFUD_NVS_FLASH_SIZE      (8*1024)
#endif
#ifndef SFUD_NVS_MAX_NAME
  #define SFUD_NVS_MAX_NAME        31
#endif
#ifndef SFUD_NVS_MAX_VALUE
  #define SFUD_NVS_MAX_VALUE       1024
#endif
#ifndef SFUD_NVS_DEVICE_INDEX
  #define SFUD_NVS_DEVICE_INDEX    0
#endif

static const uint32_t SFUD_NVS_MAGIC   = 0x53465042; // "BPFS"

/*
 * Record layout (4-byte aligned):
 *   [magic:4][ns_len:1][key_len:1][val_len:2][ns:ns_len][key:key_len][val:val_len][pad]
 *
 * magic = SFUD_NVS_MAGIC  : active record
 * magic = 0x00000000      : deleted (written without erase, bits 1->0)
 * magic = 0xFFFFFFFF      : free (erased flash)
 */

struct _NvsHdr {
    uint32_t magic;
    uint8_t  ns_len;
    uint8_t  key_len;
    uint16_t val_len;
};

static sfud_flash* _sfud_dev;
static bool        _nvs_ready;

static uint32_t _rec_size(uint8_t ns_len, uint8_t key_len, uint16_t val_len) {
    return (sizeof(_NvsHdr) + ns_len + key_len + val_len + 3u) & ~3u;
}

static bool _hdr_valid(const _NvsHdr& h) {
    return h.ns_len  <= SFUD_NVS_MAX_NAME  &&
           h.key_len <= SFUD_NVS_MAX_NAME  &&
           h.val_len <= SFUD_NVS_MAX_VALUE;
}

// Returns offset past the last written record (= start of free space)
static uint32_t _nvs_end() {
    uint32_t off = 0;
    while (off + sizeof(_NvsHdr) <= (uint32_t)SFUD_NVS_FLASH_SIZE) {
        _NvsHdr h;
        sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off, sizeof(h), (uint8_t*)&h);
        if (h.magic == 0xFFFFFFFF) break;
        if (!_hdr_valid(h)) break; // corrupted
        off += _rec_size(h.ns_len, h.key_len, h.val_len);
    }
    return off;
}

// Returns offset of the last active record for (ns, key), or 0xFFFFFFFF if not found
static uint32_t _nvs_find(const char* ns, uint8_t ns_len, const char* key, uint8_t key_len) {
    uint32_t off = 0, result = 0xFFFFFFFF;
    uint8_t  nk[SFUD_NVS_MAX_NAME * 2 + 2];
    uint8_t  nk_len = ns_len + key_len;
    while (off + sizeof(_NvsHdr) <= (uint32_t)SFUD_NVS_FLASH_SIZE) {
        _NvsHdr h;
        sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off, sizeof(h), (uint8_t*)&h);
        if (h.magic == 0xFFFFFFFF) break;
        if (!_hdr_valid(h)) break;
        if (h.magic == SFUD_NVS_MAGIC && h.ns_len == ns_len && h.key_len == key_len && nk_len <= sizeof(nk)) {
            sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off + sizeof(_NvsHdr), nk_len, nk);
            if (memcmp(nk, ns, ns_len) == 0 && memcmp(nk + ns_len, key, key_len) == 0)
                result = off;
        }
        off += _rec_size(h.ns_len, h.key_len, h.val_len);
    }
    return result;
}

static void _nvs_invalidate(uint32_t off) {
    static const uint8_t zeros[4] = {0};
    sfud_write(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off, 4, zeros);
}

// Erase region, rewrite only the latest active record for each (ns, key)
static bool _nvs_compact() {
    uint8_t* buf = (uint8_t*)malloc(SFUD_NVS_FLASH_SIZE);
    if (!buf) return false;

    uint32_t write_off = 0;
    uint32_t read_off  = 0;
    while (read_off + sizeof(_NvsHdr) <= (uint32_t)SFUD_NVS_FLASH_SIZE) {
        _NvsHdr h;
        sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + read_off, sizeof(h), (uint8_t*)&h);
        if (h.magic == 0xFFFFFFFF) break;
        if (!_hdr_valid(h)) break;
        uint32_t sz = _rec_size(h.ns_len, h.key_len, h.val_len);
        if (h.magic == SFUD_NVS_MAGIC) {
            uint8_t nk_len = h.ns_len + h.key_len;
            uint8_t nk[SFUD_NVS_MAX_NAME * 2 + 2];
            if (nk_len <= sizeof(nk) &&
                sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + read_off + sizeof(_NvsHdr), nk_len, nk) == SFUD_SUCCESS &&
                _nvs_find((char*)nk, h.ns_len, (char*)nk + h.ns_len, h.key_len) == read_off) {
                if (write_off + sz <= (uint32_t)SFUD_NVS_FLASH_SIZE) {
                    sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + read_off, sz, buf + write_off);
                    write_off += sz;
                }
            }
        }
        read_off += sz;
    }

    sfud_erase(_sfud_dev, SFUD_NVS_FLASH_OFFSET, SFUD_NVS_FLASH_SIZE);
    if (write_off > 0)
        sfud_write(_sfud_dev, SFUD_NVS_FLASH_OFFSET, write_off, buf);
    free(buf);
    return true;
}

static bool _nvs_append(const char* ns, uint8_t ns_len, const char* key, uint8_t key_len, const void* val, uint16_t val_len) {
    uint32_t end = _nvs_end();
    uint32_t sz  = _rec_size(ns_len, key_len, val_len);
    if (end + sz > (uint32_t)SFUD_NVS_FLASH_SIZE) {
        LOG_I("compacting flash log");
        if (!_nvs_compact()) return false;
        end = _nvs_end();
        if (end + sz > (uint32_t)SFUD_NVS_FLASH_SIZE) { LOG_E("flash full"); return false; }
    }
    uint32_t base = SFUD_NVS_FLASH_OFFSET + end;
    _NvsHdr h = { SFUD_NVS_MAGIC, ns_len, key_len, val_len };
    if (sfud_write(_sfud_dev, base,                                  sizeof(h), (const uint8_t*)&h)  != SFUD_SUCCESS ||
        sfud_write(_sfud_dev, base + sizeof(h),                      ns_len,   (const uint8_t*)ns)   != SFUD_SUCCESS ||
        sfud_write(_sfud_dev, base + sizeof(h) + ns_len,             key_len,  (const uint8_t*)key)  != SFUD_SUCCESS ||
        (val_len > 0 &&
         sfud_write(_sfud_dev, base + sizeof(h) + ns_len + key_len, val_len,  (const uint8_t*)val)  != SFUD_SUCCESS)) {
        LOG_E("sfud_write failed at 0x%08X", base);
        return false;
    }
    return true;
}

// Scan the region; erase it if the content looks corrupt (not erased-flash and not valid records).
// This handles the case where the region was never erased (factory state, SPIFFS remnants, etc.)
// and sfud_write would silently fail to program 1-bits over existing 0-bits.
static void _nvs_check_region() {
    uint32_t off = 0;
    while (off + sizeof(_NvsHdr) <= (uint32_t)SFUD_NVS_FLASH_SIZE) {
        _NvsHdr h;
        sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off, sizeof(h), (uint8_t*)&h);
        if (h.magic == 0xFFFFFFFF) return; // erased flash, all good
        if ((h.magic == SFUD_NVS_MAGIC || h.magic == 0x00000000) && _hdr_valid(h)) {
            off += _rec_size(h.ns_len, h.key_len, h.val_len);
            continue;
        }
        LOG_W("NVS region corrupt, erasing");
        sfud_erase(_sfud_dev, SFUD_NVS_FLASH_OFFSET, SFUD_NVS_FLASH_SIZE);
        return;
    }
}

// --- Preferences member functions ---

bool Preferences::begin(const char* name, bool readOnly) {
    if (_started || !name || !strlen(name)) return false;
    _readOnly = readOnly;
    if (!_sfud_dev) {
        // Use an already-probed device (e.g. initialized by the BSP) if available,
        // otherwise initialize sfud ourselves.
        sfud_flash* dev = sfud_get_device(SFUD_NVS_DEVICE_INDEX);
        if (!dev || !dev->chip.capacity) {
            if (sfud_init() != SFUD_SUCCESS) { LOG_E("sfud_init failed"); return false; }
            #ifdef SFUD_USING_QSPI
            sfud_qspi_fast_read_enable(sfud_get_device(SFUD_W25Q32_DEVICE_INDEX), 2);
            #endif
            dev = sfud_get_device(SFUD_NVS_DEVICE_INDEX);
        }
        if (!dev || !dev->chip.capacity) { LOG_E("sfud device not ready"); return false; }
        _sfud_dev = dev;
        if (!_nvs_ready) {
            _nvs_check_region();
            _nvs_ready = true;
        }
    }
    _path    = name;
    _started = true;
    return true;
}

void Preferences::end() {
    if (!_started) return;
    _path    = "";
    _started = false;
}

bool Preferences::clear() {
    if (!_started || _readOnly) return false;
    const char* ns     = _path.c_str();
    uint8_t     ns_len = (uint8_t)_path.length();
    uint32_t    off    = 0;
    while (off + sizeof(_NvsHdr) <= (uint32_t)SFUD_NVS_FLASH_SIZE) {
        _NvsHdr h;
        sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off, sizeof(h), (uint8_t*)&h);
        if (h.magic == 0xFFFFFFFF) break;
        if (!_hdr_valid(h)) break;
        if (h.magic == SFUD_NVS_MAGIC && h.ns_len == ns_len) {
            uint8_t ns_buf[SFUD_NVS_MAX_NAME];
            sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off + sizeof(_NvsHdr), ns_len, ns_buf);
            if (memcmp(ns_buf, ns, ns_len) == 0)
                _nvs_invalidate(off);
        }
        off += _rec_size(h.ns_len, h.key_len, h.val_len);
    }
    return true;
}

bool Preferences::remove(const char* key) {
    if (!_started || !key || _readOnly) return false;
    uint32_t off = _nvs_find(_path.c_str(), _path.length(), key, strlen(key));
    if (off == 0xFFFFFFFF) return false;
    _nvs_invalidate(off);
    return true;
}

size_t Preferences::putBytes(const char* key, const void* buf, size_t len) {
    if (!_started || !key || _readOnly) return 0;
    if (!buf && len > 0) return 0;
    if (len > SFUD_NVS_MAX_VALUE) return 0;
    const char* ns      = _path.c_str();
    uint8_t     ns_len  = (uint8_t)_path.length();
    uint8_t     key_len = (uint8_t)strlen(key);
    uint32_t    old     = _nvs_find(ns, ns_len, key, key_len);
    if (old != 0xFFFFFFFF) {
        _NvsHdr h;
        sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + old, sizeof(h), (uint8_t*)&h);
        if (h.val_len == len) {
            uint8_t tmp[SFUD_NVS_MAX_VALUE];
            sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + old + sizeof(h) + ns_len + key_len, len, tmp);
            if (len == 0 || memcmp(tmp, buf, len) == 0) return len; // unchanged, skip write
        }
    }
    if (!_nvs_append(ns, ns_len, key, key_len, buf, (uint16_t)len)) return 0;
    if (old != 0xFFFFFFFF) _nvs_invalidate(old);
    return len;
}

bool Preferences::isKey(const char* key) {
    if (!_started || !key) return false;
    return _nvs_find(_path.c_str(), _path.length(), key, strlen(key)) != 0xFFFFFFFF;
}

size_t Preferences::getBytesLength(const char* key) {
    if (!_started || !key) return 0;
    uint32_t off = _nvs_find(_path.c_str(), _path.length(), key, strlen(key));
    if (off == 0xFFFFFFFF) return 0;
    _NvsHdr h;
    sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off, sizeof(h), (uint8_t*)&h);
    return h.val_len;
}

size_t Preferences::getBytes(const char* key, void* dst, size_t maxLen) {
    if (!key) return 0;
    uint32_t off = _nvs_find(_path.c_str(), _path.length(), key, strlen(key));
    if (off == 0xFFFFFFFF) return 0;
    _NvsHdr h;
    sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off, sizeof(h), (uint8_t*)&h);
    if (!dst || !maxLen) return h.val_len;
    if (h.val_len > maxLen) { LOG_W("buffer too small: %u < %u", maxLen, h.val_len); return 0; }
    if (h.val_len > 0)
        sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off + sizeof(h) + h.ns_len + h.key_len, h.val_len, (uint8_t*)dst);
    return h.val_len;
}

String Preferences::getString(const char* key, const String defaultValue) {
    if (!_started || !key) return defaultValue;
    uint32_t off = _nvs_find(_path.c_str(), _path.length(), key, strlen(key));
    if (off == 0xFFFFFFFF) return defaultValue;
    _NvsHdr h;
    sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off, sizeof(h), (uint8_t*)&h);
    if (h.val_len == 0) return String("");
    char buf[SFUD_NVS_MAX_VALUE + 1];
    sfud_read(_sfud_dev, SFUD_NVS_FLASH_OFFSET + off + sizeof(h) + h.ns_len + h.key_len, h.val_len, (uint8_t*)buf);
    buf[h.val_len] = '\0';
    return String(buf);
}

size_t Preferences::freeEntries() {
    if (!_started) return 0;
    uint32_t used = _nvs_end();
    uint32_t free_bytes = (used < (uint32_t)SFUD_NVS_FLASH_SIZE) ? (SFUD_NVS_FLASH_SIZE - used) : 0;
    return free_bytes / (sizeof(_NvsHdr) + 4); // rough estimate
}
