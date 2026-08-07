#include "FS.h"

#define FS SPIFFS
#define _FS_MODE_READ  "r"
#define _FS_MODE_WRITE "w"

static bool _fs_init() {
    bool res = FS.begin();
    // Increase reliability for SPIFFS
    FS.check();
    return res;
}

#ifdef NVS_FORMAT_ENABLE

static bool _fs_format() {
    return FS.format();
}

#endif

static bool _fs_mkdir(const char *path) {
    // Paths are automatically created as needed
    (void)path;
    return true;
}

static bool verifyContent(File& f, const void* buf, size_t bufsize) {
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

static int _fs_create(const char* path, const void* buf, size_t bufsize) {
    LOG_D("%s %s (%d bytes)", __FUNCTION__, path, bufsize);
    if (File f = FS.open(path, _FS_MODE_WRITE)) {
        return f.write((const uint8_t*)buf, bufsize);
    }
    return -1;
}

static int _fs_update(const char* path, const void* buf, size_t bufsize) {
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

static int _fs_read(const char* path, void* buf, size_t bufsize) {
    LOG_D("%s %s (%d bytes)", __FUNCTION__, path, bufsize);
    if (File f = FS.open(path, _FS_MODE_READ)) {
        return f.read((uint8_t*)buf, bufsize);
    }
    return -1;
}

static int _fs_get_size(const char* path) {
    if (File f = FS.open(path, _FS_MODE_READ)) {
        return f.size();
    }
    return -1;
}

static bool _fs_exists(const char* path) {
    return FS.exists(path);
}

static bool _fs_unlink(const char* path) {
    LOG_D("%s %s", __FUNCTION__, path);
    return FS.remove(path);
}

static bool _fs_clean_dir(const char* path) {
    LOG_D("%s %s", __FUNCTION__, path);
    Dir dir = FS.openDir(path);
    while (dir.next()) {
        String p = dir.fileName();
        FS.remove(p.c_str());
        LOG_I("erased %s", p.c_str());
    }
    return true;
}
