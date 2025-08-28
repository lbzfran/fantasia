
#include <windows.h>
#include <stdio.h>
#include "os.h"

void* fan_lib_open(const char *path) {
    void *res = (void *)LoadLibraryA(path);
    if (res == NULL) {
        printf("Failed to load DLL '%s': '%lu'.\n", path, GetLastError());
    }
    return res;
}

void* fan_lib_load(void *lib, const char *name) {
    void *res = (void *)GetProcAddress((HMODULE)lib, name);
    if (res == NULL) {
        printf("Failed to load function '%s': '%lu'.\n", name, GetLastError());
    }
    return res;
}

void fan_lib_close(void *lib) {
    FreeLibrary((HMODULE)lib);
    lib = NULL;
}

bool32 fan_os_write(void *ctx, void *data, ssize length) {
    HANDLE *ctx_handle = (HANDLE *)ctx;
    return (bool32)WriteFile(ctx, data, length, null, null);
}
