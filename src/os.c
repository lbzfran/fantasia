
#include "os.h"
#if   defined(OS_WINDOWS)
# include "os_windows.c"
#elif defined(OS_LINUX)
# include "os_linux.c"
#endif

void *heap_make(void *ctx, ssize size) {
    (void)ctx;
    void *result = malloc(size);
    assert(result && "ERROR: Reached Out-Of-Memory state.");

    return result;
}

void heap_free(void *ctx, void *ptr, ssize size) {
    (void)ctx;
    (void)size;

    free(ptr);
    ptr = null;
}

void *heap_resize(void *ctx, void *ptr, ssize old, ssize new) {
    (void)ctx;
    void *result = heap_make(ctx, new);

    if (ptr isnt null) {
        if (new > old) {
            memcpy(result, ptr, old);
        }
        else {
            memmove(result, ptr, old);
        }
        heap_free(ctx, ptr, old);
    }

    return result;
}

void *arena_make(void *ctx, ssize size) {
    Arena *a = (Arena *)ctx;

    uintptr base = (uintptr)(a->data + a->size);
    uintptr alignment = align_forward(base, ARENA_ALIGNMENT);
    ssize offset = alignment - (uintptr)a->data;

    assert(size + offset <= a->capacity && "ERROR: Reached Out-Of-Memory state.");

    void *result = a->data + offset;
    a->size = offset + size;

    return result;
}

void arena_free(void *ctx, void *ptr, ssize size) {
    Arena *a = (Arena *)ctx;

    uintptr ptr_val = (uintptr)ptr;
    uintptr base_val = (uintptr)a->data;

    ssize offset = ptr_val - base_val;

    ssize expected_size = offset + size;
    if (a->size == expected_size) {
        a->size -= size;
    }
}

void arena_clear(Arena *a) {
    a->size = 0;
}

void *arena_resize(void *ctx, void *ptr, ssize old, ssize new) {
    Arena *a = (Arena *)ctx;

    if (new == old) {
        return ptr;
    }

    void *result = null;
    if (ptr is null) {
        result = arena_make(ctx, new);
    }
    else {
        uintptr ptr_val  = (uintptr)ptr;
        uintptr base_val = (uintptr)a->data;

        ssize offset = ptr_val - base_val;

        if (new > old) {
            result = arena_make(ctx, new);
            memcpy(result, ptr, old);
        }
        else if (a->size == offset + old) {
            a->size -= (offset + old - new);
            result = ptr;
        }
    }

    return result;
}

