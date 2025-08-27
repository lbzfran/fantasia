
// #include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
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


