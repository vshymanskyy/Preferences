#if defined(NVS_LFS_TEENSY)
  #include <LittleFS.h>
  #if !defined(NVS_TEENSY_FS_SIZE)
    // Reserved at the end of the internal program flash
    #define NVS_TEENSY_FS_SIZE (64 * 1024)
  #endif
  // Teensy doesn't provide a global LittleFS instance.
  LittleFS_Program NVS_FS;
  #define FS NVS_FS
  #define _FS_MODE_READ  FILE_READ
  #define _FS_MODE_WRITE FILE_WRITE_BEGIN
#elif defined(NVS_LFS_NRF52)
  #include <InternalFileSystem.h>
  using namespace Adafruit_LittleFS_Namespace;
  #define FS InternalFS
  #define _FS_MODE_READ  FILE_O_READ
  #define _FS_MODE_WRITE FILE_O_WRITE
#else
  #include "LittleFS.h"
  #define FS LittleFS
  #define _FS_MODE_READ  "r"
  #define _FS_MODE_WRITE "w"
#endif

static bool _fs_init() {
#if defined(NVS_LFS_TEENSY)
    return FS.begin(NVS_TEENSY_FS_SIZE);
#else
    return FS.begin();
#endif
}

#ifdef NVS_FORMAT_ENABLE

static bool _fs_format() {
    return FS.format();
}

#endif

static bool _fs_mkdir(const char *path) {
#if defined(NVS_LFS_TEENSY) || defined(NVS_LFS_NRF52)
    // Unlike ESP8266/RP2040, paths aren't auto-created here
    if (FS.exists(path)) {
        return true;
    }
    return FS.mkdir(path);
#else
    // Paths are automatically created as needed
    (void)path;
    return true;
#endif
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

static bool _fs_verify(const char* path, const void* buf, size_t bufsize) {
    LOG_D("%s %s (%d bytes)", __FUNCTION__, path, bufsize);
    if (File f = FS.open(path, _FS_MODE_READ)) {
        return verifyContent(f, buf, bufsize);
    }
    return false;
}

static int _fs_create(const char* path, const void* buf, size_t bufsize) {
    LOG_D("%s %s (%d bytes)", __FUNCTION__, path, bufsize);
    if (File f = FS.open(path, _FS_MODE_WRITE)) {
#if defined(NVS_LFS_TEENSY) || defined(NVS_LFS_NRF52)
        // FILE_O_WRITE always seeks to the end; truncate() alone would cut at
        // that (stale) position, so reset to 0 first, then drop old content
        f.truncate(0);
        f.seek(0);
#endif
        return f.write((const uint8_t*)buf, bufsize);
    }
    return -1;
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

static bool _fs_rename(const char* from, const char* to) {
    LOG_D("%s %s => %s", __FUNCTION__, from, to);
    return FS.rename(from, to);
}

static bool _fs_unlink(const char* path) {
    LOG_D("%s %s", __FUNCTION__, path);
    return FS.remove(path);
}

static bool _fs_clean_dir(const char* path) {
    LOG_D("%s %s", __FUNCTION__, path);
#if defined(NVS_LFS_TEENSY) || defined(NVS_LFS_NRF52)
    // No Dir/openDir() here: directory entries are walked via File::openNextFile()
    if (File dir = FS.open(path, _FS_MODE_READ)) {
        while (File f = dir.openNextFile()) {
            String p = String(path) + f.name();  // TODO: "/"?
            f.close();
            FS.remove(p.c_str());
            LOG_I("erased %s", p.c_str());
        }
    }
#else
    Dir dir = FS.openDir(path);
    while (dir.next()) {
        String p = String(path) + dir.fileName();
        FS.remove(p.c_str());
        LOG_I("erased %s", p.c_str());
    }
#endif
    FS.remove(path);
    return true;
}
