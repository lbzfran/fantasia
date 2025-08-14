
#include <windows.h>
#include <stdio.h>
#include "os.h"

void* LibOpen(const char *path) {
    void *res = (void *)LoadLibraryA(path);
    if (res == NULL) {
        printf("Failed to load DLL '%s': '%lu'.\n", path, GetLastError());
    }
    return res;
}

void* LibLoad(void *lib, const char *name) {
    void *res = (void *)GetProcAddress((HMODULE)lib, name);
    if (res == NULL) {
        printf("Failed to load function '%s': '%lu'.\n", name, GetLastError());
    }
    return res;
}

void LibClose(void *lib) {
    FreeLibrary((HMODULE)lib);
    lib = NULL;
}
