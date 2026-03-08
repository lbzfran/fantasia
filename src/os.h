#ifndef FAN_OS_H
#define FAN_OS_H

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>
#include <uchar.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if defined(OS_WINDOWS)
    #define GAME_LIB_PATH "libgame.dll"
    #if defined(BUILD_SHARED)
        #define GAME_API __declspec(dllexport)
    #elif defined(USE_SHARED)
        #define GAME_API __declspec(dllimport)
    #endif
#else
    #define GAME_LIB_PATH "libgame.so"
    #if defined(BUILD_SHARED)
        #define GAME_API __attribute((visibility("default")))
    #endif
#endif

#ifndef GAME_API
    #define GAME_API
#endif

typedef unsigned char uchar8;
typedef char          char8;
typedef char16_t      char16;

typedef uint8_t       uint8;
typedef uint32_t      uint32;
typedef uint64_t      uint64;

typedef int32_t       bool32;
typedef int32_t       int32;

typedef float         float32;
typedef double        float64;

typedef size_t        usize;
typedef ptrdiff_t     ssize;
typedef uintptr_t     uintptr;

#define DEBUG
#ifdef DEBUG
# define assert(c) ((c) ? (void) (0) : fprintf(stderr, "%s failed in %s:%d:%s()\n", #c, __FILE__, __LINE__, __func__))
# define assume(c) assert(c)
#else
# define assert(c) ((void) (0))
# define assume(c) if (!(c)) __builtin_unreachable()
#endif

#define sizeof(x)           (ssize)sizeof(x)
#define alignof(x)          (_Alignof(x))
#define countof(a)          (sizeof(a) / sizeof(*(a)))
#define lengthof(s)         (countof(s) - 1)
#define signof(x)           ((x) > 0) ? 1 : (((x) < 0) ? -1 : 0)

#define coalesce(a, b)      ((a) ? (a) : (b))
#define init_if_null(a, x)  ((a) = coalesce((a), (x)))

#if !defined(true) && !defined(false)
# define true    1
# define false   0
#endif
#define not     !
#define is      ==
#define isnt    !=
#define and     &&
#define or      ||

#define local   static
#define global  static

#define optional_

#define null            0
#define kilobytes(x)    ((x)*1024LL)
#define megabytes(x)    (kilobytes(x)*1024LL)
#define gigabytes(x)    (megabytes(x)*1024LL)

#if !defined(min) && !defined(max)
# define min(x,y)        ((x) < (y) ? (x) : (y))
# define max(x,y)        ((x) > (y) ? (x) : (y))
#endif

#define clamp(x, a, b)   min(max(x, a), b)

typedef struct allocator {
    void *(*make)   (void *ctx, ssize size);
    void  (*free)   (void *ctx, void *ptr, ssize size);
    void *(*resize) (void *ctx, void *ptr, ssize old, ssize new);
    void *ctx;
} Allocator;

typedef struct Arena {
    uint8 *data;
    ssize  size;
    ssize  capacity;
} Arena;
#define ARENA_ALIGNMENT 16

typedef enum {
    fan_pipe_stdout = 0,
    fan_pipe_stdin,
    fan_pipe_stderr,
} fan_pipe;

typedef struct {
    uchar8        *buf;
    ssize          length;
    ssize          capacity;
    fan_pipe       pipe;
    int32          error;
} fan_fbuf8;

typedef struct {
    uchar8 *data;
    ssize   length;
} fan_str8;

typedef struct {
    fan_str8 head;
    fan_str8 tail;
    int32    ok;
} fan_cutstr8;

#define FAN_ARRAY_INITIAL_CAPACITY 32
#define fan_array_append(allocator, arr, x) do{                                             \
    assume(allocator.resize != null && "allocator 'resize' must be defined.");              \
    if (arr.size + 1 >= arr.capacity) {                                                     \
        ssize new_capacity = max(arr.capacity * 2, FAN_ARRAY_INITIAL_CAPACITY);             \
        arr.data = allocator.resize(allocator.ctx, arr.capacity, sizeof(x) * new_capacity); \
        arr.capacity = new_capacity;                                                        \
    }                                                                                       \
    assume(typeof(*allocator.data) == typeof(x) && "array's data type must match.");        \
    arr.data[arr.size] = x;                                                                 \
    arr.size++;                                                                             \
}while(0)

#define fan_array_clear(allocator, arr) do{                                \
    assume(allocator.free != null && "allocator 'free' must be defined."); \
    allocator.free(allocator.ctx, arr.data, arr.capacity);                 \
}while(0)

#define fan_fbuf8_mem(buf, cap)    { buf, 0, cap, -1, 0 }
#define fan_fbuf8_fd(fd, buf, cap) { buf, 0, cap, fd, 0 }

#define fan_str8_cstr(s)    (fan_str8){ (uchar8 *)s, sizeof(s) - 1 }

void fan_fbuf8_flush(fan_fbuf8 *);
void fan_fbuf8_append(fan_fbuf8 *, uchar8 *, ssize);

void fan_fbuf8_append_char(fan_fbuf8 *, uchar8);
void fan_fbuf8_append_cstr(fan_fbuf8 *, const char8 *);
void fan_fbuf8_append_str8(fan_fbuf8 *, fan_str8);
void fan_fbuf8_append_ptr(fan_fbuf8  *, void *);

void fan_fbuf8_append_long(fan_fbuf8   *, long);
void fan_fbuf8_append_double(fan_fbuf8 *, double);

#define fan_fbuf8_append_derive_(b, x) _Generic((x),  \
        int32:              fan_fbuf8_append_long,    \
        int64:              fan_fbuf8_append_long,    \
        float32:            fan_fbuf8_append_double,  \
        float64:            fan_fbuf8_append_double,  \
        char8:              fan_fbuf8_append_char,    \
        uchar8:             fan_fbuf8_append_char,    \
        char8 *:            fan_fbuf8_append_cstr,    \
        const char8 *:      fan_fbuf8_append_cstr,    \
        fan_str8:           fan_fbuf8_append_str8,    \
        default:            (void)0                   \
)(b, x)

void fan_str8_print(fan_fbuf8 *, fan_str8);
void fan_str8_printn(fan_fbuf8 *, fan_str8, uchar8);
void fan_str8_println(fan_fbuf8 *, fan_str8);

fan_str8 fan_str8_span(uchar8 *, uchar8 *);
int32 fan_str8_equals(fan_str8, fan_str8);
// trims spaces
fan_str8 fan_str8_triml(fan_str8);
fan_str8 fan_str8_trimr(fan_str8);
fan_str8 fan_str8_substr(fan_str8, ssize);

fan_cutstr8 fan_str8_cut(fan_str8, uchar8);


inline uintptr fan_align_forward(uintptr ptr, ssize alignment) {
    return (ptr + (alignment - 1)) & ~(alignment - 1);
}

void *fan_heap_make(void *ctx, ssize size);
void  fan_heap_free(void *ctx, void *ptr, ssize size);
void *fan_heap_resize(void *ctx, void *ptr, ssize old, ssize new);

void *fan_arena_make(void *ctx, ssize size);
void  fan_arena_free(void *ctx, void *ptr, ssize size);
void *fan_arena_resize(void *ctx, void *ptr, ssize old, ssize new);

void  fan_arena_clear(Arena *a);

void *fan_lib_open(const char* path);
void *fan_lib_load(void *lib, const char *name);
void  fan_lib_close(void *lib);

bool32 fan_os_write(fan_pipe pipe, void *data, ssize length);

// char* LibGetError(void);

// typedef void *(*ThreadFunc)(void *);
// typedef struct FanThread FanThread;
// FAN_API FanThread *FanThreadCreate(ThreadFunc, void *arg);
// FAN_API void FanThreadJoin(FanThread *);
// FAN_API void FanThreadClose(FanThread *);

#endif // FAN_OS_H
