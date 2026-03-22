
#include <windows.h>
#include <stdio.h>
#include "os.h"

void os_error_get_(void) {
    DWORD err = GetLastError();
    char *msg = NULL;

    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   err,
                   0,
                   (LPSTR)&msg,
                   0,
                   NULL);

    printf("OS ERROR %lu: %s\n", err, msg);
    LocalFree(msg);
}

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

HANDLE *fan_os_pipe_get(fan_pipe pipe) {
    HANDLE *h = GetStdHandle((DWORD)pipe);
    return h;
}

bool32 fan_os_write(fan_pipe pipe, void *data, ssize length) {
    HANDLE *ctx_handle = fan_os_pipe_get(pipe);
    return (bool32)WriteFile(ctx_handle, data, (DWORD)length, null, null);
}

bool32 fan_os_file_copy(const char *src, const char *dst) {
    if (CopyFile(src, dst, FALSE)) {
        return true;
    }
    os_error_get_();
    return false;
}

bool32 fan_os_file_delete(const char *path) {
    return (bool32)DeleteFile(path);
}

bool32 fan_os_file_time_last_written(const char *path, uint64 *last_ms) {
    WIN32_FILE_ATTRIBUTE_DATA data;

    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
        os_error_get_();
        return 0;
    }

    ULARGE_INTEGER t;
    t.LowPart  = data.ftLastWriteTime.dwLowDateTime;
    t.HighPart = data.ftLastWriteTime.dwHighDateTime;
    uint64 ms = t.QuadPart / 10000ULL;

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

void fan_os_wait(uint64 ms) {
    Sleep(ms);
}
