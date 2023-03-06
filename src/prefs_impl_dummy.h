
static bool _fs_init() {
    return true;
}

static bool _fs_mkdir(const char *path) {
    (void)path;
    return true;
}

static bool _fs_verify(const char* path, const void* buf, int bufsize) {
    (void)path; (void)buf; (void)bufsize;
    return true;
}

static int _fs_create(const char* path, const void* buf, int bufsize) {
    (void)path; (void)buf; (void)bufsize;
    return bufsize;
}

static int _fs_read(const char* path, void* buf, int bufsize) {
    (void)path; (void)buf; (void)bufsize;
    return -1;
}

static int _fs_get_size(const char* path) {
    (void)path;
    return -1;
}

static bool _fs_exists(const char* path) {
    (void)path;
    return false;
}

static bool _fs_rename(const char* from, const char* to) {
    (void)from; (void)to;
    return true;
}

static bool _fs_unlink(const char* path) {
    (void)path;
    return true;
}

static bool _fs_clean_dir(const char* path) {
    (void)path;
    return true;
}
