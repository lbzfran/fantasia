
#include <windows.h>
#include <stdio.h>
#include "os.h"

void* LibOpen(const char *path) {
    void *res = (void *)LoadLibraryA((TEXT(path)));
    if (res == NULL) {
        printf("Failed to load DLL: '%lu'.\n", GetLastError());
    }
    return res;
}

void* LibLoad(void *lib, const char *name) {
    void *res = (void *)GetProcAddress((HMODULE)lib, name);
    if (res == NULL) {
        printf("Failed to load function: '%lu'.\n", GetLastError());
    }
    return res;
}

void LibClose(void *lib) {
    FreeLibrary((HMODULE)lib);
    lib = NULL;
}
