
#include "os.h"
#if   defined(OS_WINDOWS)
# include "os_windows.c"
#elif defined(OS_LINUX)
# include "os_linux.c"
#endif
#include <string.h>

void *fan_heap_make(void *ctx, ssize size) {
    (void)ctx;
    void *result = malloc(size);
    assert(result && "ERROR: Reached Out-Of-Memory state.");

    return result;
}

void fan_heap_free(void *ctx, void *ptr, ssize size) {
    (void)ctx;
    (void)size;

    free(ptr);
    ptr = null;
}

void *fan_heap_resize(void *ctx, void *ptr, ssize old, ssize new) {
    (void)ctx;
    void *result = fan_heap_make(ctx, new);

    if (ptr isnt null) {
        if (new > old) {
            memcpy(result, ptr, old);
        }
        else {
            memmove(result, ptr, old);
        }
        fan_heap_free(ctx, ptr, old);
    }

    return result;
}

void *fan_arena_make(void *ctx, ssize size) {
    fan_arena *a = (fan_arena *)ctx;

    uintptr base = (uintptr)(a->data + a->size);
    uintptr alignment = fan_align_forward(base, FAN_ARENA_ALIGNMENT);
    ssize offset = alignment - (uintptr)a->data;

    assert(size + offset <= a->capacity && "ERROR: Reached Out-Of-Memory state.");

    void *result = a->data + offset;
    a->size = offset + size;

    return result;
}

void fan_arena_free(void *ctx, void *ptr, ssize size) {
    fan_arena *a = (fan_arena *)ctx;

    uintptr ptr_val = (uintptr)ptr;
    uintptr base_val = (uintptr)a->data;

    ssize offset = ptr_val - base_val;

    ssize expected_size = offset + size;
    if (a->size == expected_size) {
        a->size -= size;
    }
}

void fan_arena_clear(fan_arena *a) {
    a->size = 0;
}

void *fan_arena_resize(void *ctx, void *ptr, ssize old, ssize new) {
    fan_arena *a = (fan_arena *)ctx;

    if (new == old) {
        return ptr;
    }

    void *result = null;
    if (ptr is null) {
        result = fan_arena_make(ctx, new);
    }
    else {
        uintptr ptr_val  = (uintptr)ptr;
        uintptr base_val = (uintptr)a->data;

        ssize offset = ptr_val - base_val;

        if (new > old) {
            result = fan_arena_make(ctx, new);
            memcpy(result, ptr, old);
        }
        else if (a->size == offset + old) {
            a->size -= (offset + old - new);
            result = ptr;
        }
    }

    return result;
}


void fan_fbuf8_flush(fan_fbuf8 *b) {
    b->error |= b->pipe < 0;
    if (!b->error && b->length) {
        b->error |= !fan_os_write(b->pipe, b->buf, b->length);
        b->length = 0;
    }
}

void fan_fbuf8_append(fan_fbuf8 *b, uchar8 *src, ssize length) {
    uchar8 *end = src + length;
    while (!b->error && src<end) {
        ssize left = end - src;
        ssize available = b->capacity - b->length;
        ssize amount = available < left ? available : left;

        for (ssize i = 0; i < amount; i++) {
            b->buf[b->length+i] = src[i];
        }
        b->length += amount;
        src += amount;

        if (amount < left) {
            fan_fbuf8_flush(b);
        }
    }
}

void fan_fbuf8_append_char(fan_fbuf8 *b, uchar8 c) {
    fan_fbuf8_append(b, &c, 1);
}

void fan_fbuf8_append_str8(fan_fbuf8 *b, fan_str8 s) {
    fan_fbuf8_append(b, s.data, s.length);
}

void fan_fbuf8_append_cstr(fan_fbuf8 *b, const char8 *s) {
    fan_fbuf8_append(b, (uchar8 *)s, sizeof(s) - 1);
}

void fan_fbuf8_append_long(fan_fbuf8 *b, long x) {
    uchar8  tmp[64];
    uchar8 *end = tmp + sizeof(tmp);
    uchar8 *beg = end;
    long t = x > 0 ? -x : x;
    do {
        *--beg = (uchar8)('0' - t % 10);
    }
    while (t /= 10);

    if (x < 0) {
        *--beg = (uchar8)('-');
    }
    fan_fbuf8_append(b, beg, end - beg);
}

void fan_fbuf8_append_double(fan_fbuf8 *b, double x) {
    long prec = 1000000;  // i.e. 6 decimals

    if (x < 0) {
        fan_fbuf8_append_char(b, '-');
        x = -x;
    }

    x += 0.5 / (double)prec;
    if (x >= (double)(-1UL>>1)) {
        fan_fbuf8_append_cstr(b, "inf");
    } else {
        long integral = (long)x;
        long fractional = (long)((long)(x - (double)integral) * prec);
        fan_fbuf8_append_long(b, integral);
        fan_fbuf8_append_char(b, '.');
        for (long i = prec/10; i > 1; i /= 10) {
            if (i > fractional) {
                fan_fbuf8_append_char(b, '0');
            }
        }
        fan_fbuf8_append_long(b, fractional);
    }
}

void fan_fbuf8_append_ptr(fan_fbuf8 *b, void *ptr) {
    fan_fbuf8_append_cstr(b, "0x");
    uintptr_t u = (uintptr_t)ptr;
    for (int i = 2*sizeof(u) - 1; i >= 0; i--) {
        fan_fbuf8_append_char(b, "0123456789abcdef"[(u>>(4 * i)) & 15]);
    }
}

void fan_str8_print(fan_fbuf8 *b, fan_str8 s) {
    fan_fbuf8_append_str8(b, s);
    fan_fbuf8_flush(b);
}

void fan_str8_printn(fan_fbuf8 *b, fan_str8 s, uchar8 end) {
    fan_fbuf8_append_str8(b, s);
    fan_fbuf8_append_char(b, end);
    fan_fbuf8_flush(b);
}

void fan_str8_println(fan_fbuf8 *b, fan_str8 s) {
    fan_str8_printn(b, s, '\n');
}

fan_str8 fan_str8_span(uchar8 *beg, uchar8 *end) {
    fan_str8 r = {0};
    r.data = beg;
    r.length  = beg ? end-beg : 0;
    return r;
}

int fan_str8_equals(fan_str8 a, fan_str8 b) {
    return a.length==b.length && (!a.length || !memcmp(a.data, b.data, a.length));
}

fan_str8 fan_str8_triml(fan_str8 s) {
    for (; s.length && *s.data <= ' '; s.data++, s.length--) {}
    return s;
}

fan_str8 fan_str8_trimr(fan_str8 s) {
    for (; s.length && s.data[s.length-1] <= ' '; s.length--) {}
    return s;
}

fan_str8 fan_str8_substr(fan_str8 s, ssize i) {
    if (i) {
        s.data   += i;
        s.length -= i;
    }
    return s;
}

fan_cutstr8 fan_str8_cut(fan_str8 s, uchar8 c) {
    fan_cutstr8 r = { 0 };
    if (!s.length) return r;  // null pointer special case
    uchar8 *beg = s.data;
    uchar8 *end = s.data + s.length;
    uchar8 *cut = beg;
    for (; cut<end && *cut != c; cut++) {}
    r.ok   = cut < end;
    r.head = fan_str8_span(beg, cut);
    r.tail = fan_str8_span(cut+r.ok, end);
    return r;
}

