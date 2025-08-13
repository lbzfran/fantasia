
// #include <pthread.h>
#include <stdio.h>
#include <dlfcn.h>
#include "os.h"

void* LibOpen(const char *path) {
    void *res = dlopen(path, RTLD_LAZY);
    if (res == NULL) {
        printf("Failed to load DLL: %s.\n", dlerror());
    }
    return res;
}

void* LibLoad(void *lib, const char *name) {
    void *res = dlsym(lib, name);
    if (res == NULL) {
        printf("Failed to load function: '%s'.\n", dlerror());
    }
    return res;
}

void LibClose(void *lib) {
    dlclose(lib);
}


