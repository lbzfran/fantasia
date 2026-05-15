
// #include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "os.h"

void* fan_lib_open(const char *path) {
    void *res = dlopen(path, RTLD_LAZY);
    if (res == NULL) {
        fan_log_error("Failed to load DLL: %s.\n", dlerror());
    }
    return res;
}

void* fan_lib_load(void *lib, const char *name) {
    void *res = dlsym(lib, name);
    if (res == NULL) {
        fan_log_error("Failed to load function: '%s'.\n", dlerror());
    }
    return res;
}

void fan_lib_close(void *lib) {
    dlclose(lib);
}

bool32 fan_os_write(fan_pipe pipe, void *data, ssize length) {
    return (bool32)write(pipe, data, length);
}

bool32 fan_file_copy(const char *src, const char *dst) {
    int32 fd_src = open(src, O_RDONLY);
    if (fd_src == -1) {
        return false;
    }

    int32 fd_dst = open(dst, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_dst == -1) {
        close(fd_src);
        return false;
    }

    char buf[4096];
    ssize bytes_read;

    while ((bytes_read = read(fd_src, buf, sizeof(buf))) > 0) {
        char *buf_ptr = buf;
        ssize remaining = bytes_read;

        while (remaining > 0) {
            ssize bytes_written = write(fd_dst, buf_ptr, remaining);
            if (bytes_written < 0) {
                close(fd_dst);
                close(fd_src);
                return false;
            }

            remaining -= bytes_written;
            buf_ptr += bytes_written;
        }
    }

    close(fd_dst);
    close(fd_src);

    return bytes_read == 0; // success if EOF, failure if -1
}

bool32 fan_file_delete(const char *path) {
    if (remove(path)) {
        return true;
    }
    return false;
}

bool32 fan_file_time_last_written(const char *path, uint64 *last_ms) {
    struct stat st;

    if (stat(path, &st) != 0) {
        return 0;
    }

    uint64 ms = (uint64)st.st_mtime * 1000ULL;

    if (*last_ms == 0) {
        *last_ms = ms;
        return 0;
    }

    if (ms != *last_ms) {
        *last_ms = ms;
        return 1;
    }

    return 0;
}
