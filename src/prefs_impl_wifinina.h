#include <WiFiNINA.h>

static bool _fs_init() {
    return WiFi.status() != WL_NO_SHIELD;
}

static bool _fs_mkdir(const char *path) {
    LOG_D("%s %s", __FUNCTION__, path);
    // Paths are automatically created as needed
    (void)path;
    return true;
}

static int _fs_get_size(const char* path) {
    uint32_t len = 0;
    if (WiFiStorage.exists(path, &len)) {
        if (len >= 4) {
            return len - 4;
        } else {
            return 0;
        }
    }
    return -1;
}

static int _fs_create(const char* path, const void* buf, int bufsize) {
    LOG_D("%s %s (%d bytes)", __FUNCTION__, path, bufsize);
    if (bufsize < 0 || bufsize > 1024) { return -1; }

    uint8_t tmp[bufsize+4];
    memcpy(tmp, buf, bufsize);
    memset(tmp+bufsize, '\n', 4);
    if (WiFiStorage.write(path, 0, tmp, bufsize+4)) {
        return bufsize;
    }
    return -1;
}

static int _fs_read(const char* path, void* buf, int bufsize) {
    LOG_D("%s %s (%d bytes)", __FUNCTION__, path, bufsize);
    int fsize = _fs_get_size(path);
    if (fsize < 0) { return -1; }

    memset(buf, 0, bufsize);
    bufsize = min(bufsize, fsize);

    if (bufsize < 0 || bufsize > 1024) { return -1; }
    uint8_t tmp[bufsize+4];
    if (WiFiStorage.read(path, 0, tmp, bufsize+4)) {
        memcpy(buf, tmp, bufsize);
        return bufsize;
    }
    return -1;
}

static int _fs_update(const char* path, const void* buf, int bufsize) {
    int fsize = _fs_get_size(path);

    LOG_D("%s %s (old: %d, new %d bytes)", __FUNCTION__, path, fsize, bufsize);
    if (fsize < 0) { return -1; }

    // TODO: read in chunks, remove this limitation
    if (fsize == bufsize && bufsize <= 1024) {
        // Check if content is the same
        uint8_t tmp[bufsize+4];
        memset(tmp, 0, bufsize+4);
        if (WiFiStorage.read(path, 0, tmp, bufsize+4)) {
            if (!memcmp(buf, tmp, bufsize)) {
                return bufsize;
            }
        }
    }
    //if (fsize > bufsize) {
        // TODO: this is not atomic
        WiFiStorage.remove(path);
    //}
    return _fs_create(path, buf, bufsize);
}


static bool _fs_exists(const char* path) {
    return WiFiStorage.exists(path);
}

static bool _fs_rename(const char* from, const char* to) {
    LOG_D("%s %s => %s", __FUNCTION__, from, to);
    return WiFiStorage.rename(from, to);
}

static bool _fs_unlink(const char* path) {
    LOG_D("%s %s", __FUNCTION__, path);
    return WiFiStorage.remove(path);
}

static bool _fs_clean_dir(const char* path) {
    LOG_D("%s %s", __FUNCTION__, path);
    return false; // TODO: no API for this
}
