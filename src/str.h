#ifndef FAN_STR_H_
#define FAN_STR_H_

#include <stddef.h>

int os_write(int fd, void *data, ptrdiff_t length);

typedef struct {
    unsigned char *buf;
    ptrdiff_t      length;
    ptrdiff_t      capacity;
    int            fd;
    int            error;
} fbuf8;

typedef struct {
    unsigned char *data;
    ptrdiff_t      length;
} str8;

typedef struct {
    str8  head;
    str8  tail;
    int   ok;
} cutstr8;

#define fbuf8_mem(buf, cap)    { buf, 0, cap, -1, 0 }
#define fbuf8_fd(fd, buf, cap) { buf, 0, cap, fd, 0 }

#define STR8(s)    (str8){ (unsigned char *)s, sizeof(s) - 1 }

void fbuf8_flush(fbuf8 *);
void fbuf8_append(fbuf8 *, unsigned char *, ptrdiff_t);

void fbuf8_append_char(fbuf8   *, unsigned char);
void fbuf8_append_cstr(fbuf8   *, const char *);
void fbuf8_append_str8(fbuf8   *, str8);
void fbuf8_append_ptr(fbuf8    *, void *);

void fbuf8_append_long(fbuf8   *, long);
void fbuf8_append_double(fbuf8 *, double);

#define fbuf8_append_derive_(b, x) _Generic((x),  \
        int:                fbuf8_append_long,    \
        long:               fbuf8_append_long,    \
        float:              fbuf8_append_double,  \
        double:             fbuf8_append_double,  \
        char:               fbuf8_append_char,    \
        unsigned char:      fbuf8_append_char,    \
        char *:             fbuf8_append_cstr,    \
        const char *:       fbuf8_append_cstr,    \
        str8:               fbuf8_append_str8,    \
        default:            (void)0               \
)(b, x)

void str8_print(fbuf8 *, str8);
void str8_printn(fbuf8 *, str8, unsigned char);
void str8_println(fbuf8 *, str8);

str8 str8_span(unsigned char *, unsigned char *);
int str8_equals(str8, str8);
// trims spaces
str8 str8_triml(str8);
str8 str8_trimr(str8);
str8 str8_substr(str8, ptrdiff_t);

cutstr8 str8_cut(str8, unsigned char);

#endif // FAN_STR_H_
#ifdef FAN_STR_IMPLEMENTATION

#include <string.h>

void fbuf8_flush(fbuf8 *b) {
    b->error |= b->fd < 0;
    if (!b->error && b->length) {
        b->error |= !os_write(b->fd, b->buf, b->length);
        b->length = 0;
    }
}

void fbuf8_append(fbuf8 *b, unsigned char *src, ptrdiff_t length) {
    unsigned char *end = src + length;
    while (!b->error && src<end) {
        ptrdiff_t left = end - src;
        ptrdiff_t available = b->capacity - b->length;
        ptrdiff_t amount = available < left ? available : left;

        for (ptrdiff_t i = 0; i < amount; i++) {
            b->buf[b->length+i] = src[i];
        }
        b->length += amount;
        src += amount;

        if (amount < left) {
            fbuf8_flush(b);
        }
    }
}

void fbuf8_append_char(fbuf8 *b, unsigned char c) {
    fbuf8_append(b, &c, 1);
}

void fbuf8_append_str8(fbuf8 *b, str8 s) {
    fbuf8_append(b, s.data, s.length);
}

#define STR8(s)    (str8){ (unsigned char *)s, sizeof(s) - 1 }

void fbuf8_append_cstr(fbuf8 *b, const char *s) {
    fbuf8_append(b, (unsigned char *)s, sizeof(s) - 1);
}

void fbuf8_append_long(fbuf8 *b, long x) {
    unsigned char  tmp[64];
    unsigned char *end = tmp + sizeof(tmp);
    unsigned char *beg = end;
    long t = x > 0 ? -x : x;
    do {
        *--beg = (unsigned char)('0' - t % 10);
    }
    while (t /= 10);

    if (x < 0) {
        *--beg = (unsigned char)('-');
    }
    fbuf8_append(b, beg, end - beg);
}

void fbuf8_append_double(fbuf8 *b, double x) {
    long prec = 1000000;  // i.e. 6 decimals

    if (x < 0) {
        fbuf8_append_char(b, '-');
        x = -x;
    }

    x += 0.5 / prec;
    if (x >= (double)(-1UL>>1)) {
        fbuf8_append_cstr(b, "inf");
    } else {
        long integral = (long)x;
        long fractional = (long)((x - integral) * prec);
        fbuf8_append_long(b, integral);
        fbuf8_append_char(b, '.');
        for (long i = prec/10; i > 1; i /= 10) {
            if (i > fractional) {
                fbuf8_append_char(b, '0');
            }
        }
        fbuf8_append_long(b, fractional);
    }
}

void fbuf8_append_ptr(fbuf8 *b, void *ptr) {
    fbuf8_append_cstr(b, "0x");
    uintptr_t u = (uintptr_t)ptr;
    for (int i = 2*sizeof(u) - 1; i >= 0; i--) {
        fbuf8_append_char(b, "0123456789abcdef"[(u>>(4 * i)) & 15]);
    }
}

void str8_print(fbuf8 *b, str8 s) {
    fbuf8_append_str8(b, s);
    fbuf8_flush(b);
}

void str8_printn(fbuf8 *b, str8 s, unsigned char end) {
    fbuf8_append_str8(b, s);
    fbuf8_append_char(b, end);
    fbuf8_flush(b);
}

void str8_println(fbuf8 *b, str8 s) {
    str8_printn(b, s, '\n');
}

str8 str8_span(unsigned char *beg, unsigned char *end)
{
    str8 r = {0};
    r.data = beg;
    r.length  = beg ? end-beg : 0;
    return r;
}

int str8_equals(str8 a, str8 b)
{
    return a.length==b.length && (!a.length || !memcmp(a.data, b.data, a.length));
}

str8 str8_triml(str8 s)
{
    for (; s.length && *s.data<=' '; s.data++, s.length--) {}
    return s;
}

str8 str8_trimr(str8 s)
{
    for (; s.length && s.data[s.length-1]<=' '; s.length--) {}
    return s;
}

str8 str8_substr(str8 s, ptrdiff_t i)
{
    if (i) {
        s.data += i;
        s.length  -= i;
    }
    return s;
}

cutstr8 str8_cut(str8 s, unsigned char c) {
    cutstr8 r = { 0 };
    if (!s.length) return r;  // null pointer special case
    unsigned char *beg = s.data;
    unsigned char *end = s.data + s.length;
    unsigned char *cut = beg;
    for (; cut<end && *cut!=c; cut++) {}
    r.ok   = cut < end;
    r.head = str8_span(beg, cut);
    r.tail = str8_span(cut+r.ok, end);
    return r;
}

#endif // FAN_STR_IMPLEMENTATION
