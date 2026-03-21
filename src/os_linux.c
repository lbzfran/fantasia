
// #include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include "os.h"

void* fan_lib_open(const char *path) {
    void *res = dlopen(path, RTLD_LAZY);
    if (res == NULL) {
        printf("Failed to load DLL: %s.\n", dlerror());
    }
    return res;
}

void* fan_lib_load(void *lib, const char *name) {
    void *res = dlsym(lib, name);
    if (res == NULL) {
        printf("Failed to load function: '%s'.\n", dlerror());
    }
    return res;
}

void fan_lib_close(void *lib) {
    dlclose(lib);
}

bool32 fan_os_write(fan_pipe pipe, void *data, ssize length) {
    return (bool32)write(pipe, data, length);
}

bool32 fan_os_file_copy(const char *src, const char *dst) {
}
