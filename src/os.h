#ifndef FAN_OS_H
#define FAN_OS_H

#define _POSIX_C_SOURCE 199309L
#include <stddef.h>
#include <sys/types.h>
#include <uchar.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdalign.h>


#if defined(OS_WINDOWS)
 #if defined(DEBUG)
  #define GAME_LIB_PATH "bin/libgame.dll"
  #define GAME_LIB_TMP_PATH "bin/dbg_libgame.dll"
 #else
  #define GAME_LIB_PATH "libgame.dll"
  #define GAME_LIB_TMP_PATH "dbg_libgame.dll"
 #endif
 #if defined(PLATFORM_BUILD_SHARED)
  #define FAN_API __declspec(dllexport)
 #else
  #define FAN_API __declspec(dllimport)
 #endif
#else // OS_LINUX implied
 #if defined(DEBUG)
  #define GAME_LIB_PATH "bin/libgame.so"
  #define GAME_LIB_TMP_PATH "bin/dbg_libgame.so"
 #else
  #define GAME_LIB_PATH "libgame.so"
  #define GAME_LIB_TMP_PATH "dbg_libgame.so"
 #endif
 #if defined(PLATFORM_BUILD_SHARED)
  #define FAN_API __attribute((visibility("default")))
 #else
  #define FAN_API
 #endif
#endif

#ifndef FAN_API
    #define FAN_API extern
#endif

typedef unsigned char uchar8;
typedef char          char8;
typedef char16_t      char16;

typedef uint8_t       uint8;
typedef uint32_t      uint32;
typedef uint64_t      uint64;

#ifndef bool
typedef int32_t       bool32;
#else
typedef bool          bool32;
#endif
typedef int32_t       int32;

typedef float         float32;
typedef double        float64;

typedef size_t        usize;
typedef ptrdiff_t     ssize;
typedef uintptr_t     uintptr;

#ifdef DEBUG
# define assert(c) ((c) ? (void) (0) : fprintf(stderr, "%s failed in %s:%d:%s()\n", #c, __FILE__, __LINE__, __func__))
# define assume(c) assert(c)
#else
# define assert(c) ((void) (0))
static inline void assume(bool32 condition) {
    if (!condition) {
        __builtin_unreachable();
    }
}
#endif

#define sizeof(x)           (ssize)sizeof(x)
#ifndef alignas
#define alignas(x)          _Alignas(x)
#endif
#ifndef alignof
#define alignof(x)          _Alignof(x)
#endif
#define countof(a)          (sizeof(a) / sizeof(*(a)))
#define lengthof(s)         (countof(s) - 1)
#define signof(x)           ((x) > 0) ? 1 : (((x) < 0) ? -1 : 0)
#ifndef static_assert
#define static_assert _Static_assert
#endif
#ifdef nullptr_t
typedef nullptr_t nullptr;
#else
#define nullptr null
#endif

#if !defined(true) && !defined(false)
# define true    1
# define false   0
#endif

#define not     !
#define is      ==
#define isnt    !=
#define and     &&
#define or      ||
#define exists  != nullptr

#define local   static
#define global  static

// NOTE(liam): fake attribute used to denote if a function's parameter
// is effectively optional (as in a nullable parameter)
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

typedef struct fan_allocator {
    void *(*make)   (void *ctx, ssize size);
    void  (*free)   (void *ctx, void *ptr, ssize size);
    void *(*resize) (void *ctx, void *ptr, ssize old, ssize new);
    void *ctx;
} fan_allocator;

typedef struct fan_arena {
    uint8 *data;
    ssize  size;
    ssize  capacity;
} fan_arena;
#define FAN_ARENA_ALIGNMENT 16

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
    assume((allocator)->resize != null && "allocator 'resize' must be defined.");              \
    assume(typeof(*(arr).data) == typeof(x) && "array's data type must match.");        \
    if ((arr).size >= (arr).capacity) {                                                     \
        ssize new_capacity = max((arr).capacity * 2, FAN_ARRAY_INITIAL_CAPACITY);             \
        void *new_data = allocator->resize((allocator)->ctx, (arr).data, (arr).capacity, sizeof(*(arr).data) * new_capacity); \
        assume(new_data != nullptr); \
        (arr).data = new_data; \
        (arr).capacity = new_capacity;                                                        \
    }                                                                                       \
    (arr).data[(arr).size] = x;                                                                 \
    (arr).size++;                                                                             \
}while(0)

// NOTE(liam): array definitions
#define fan_array_clear(allocator, arr) do{                                \
    assume(allocator.free != null && "allocator 'free' must be defined."); \
    allocator.free(allocator.ctx, arr.data, arr.capacity);                 \
}while(0)

#define fan_fbuf8_mem(buf, cap)    { buf, 0, cap, -1, 0 }
#define fan_fbuf8_fd(fd, buf, cap) { buf, 0, cap, fd, 0 }


FAN_API void fan_fbuf8_flush(fan_fbuf8 *);
FAN_API void fan_fbuf8_append(fan_fbuf8 *, uchar8 *, ssize);

FAN_API void fan_fbuf8_append_char(fan_fbuf8 *, uchar8);
FAN_API void fan_fbuf8_append_cstr(fan_fbuf8 *, const char8 *);
FAN_API void fan_fbuf8_append_str8(fan_fbuf8 *, fan_str8);
FAN_API void fan_fbuf8_append_ptr(fan_fbuf8  *, void *);

FAN_API void fan_fbuf8_append_long(fan_fbuf8   *, long);
FAN_API void fan_fbuf8_append_double(fan_fbuf8 *, double);

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

// NOTE(liam): string definitions
#define fan_str8_cstr(s) (fan_str8){ (uchar8 *)s, sizeof(s) - 1 }

FAN_API void fan_str8_print(fan_fbuf8 *, fan_str8);
FAN_API void fan_str8_printn(fan_fbuf8 *, fan_str8, uchar8);
FAN_API void fan_str8_println(fan_fbuf8 *, fan_str8);

FAN_API fan_str8 fan_str8_span(uchar8 *, uchar8 *);
FAN_API fan_str8 fan_str8_cstrv(const char8 *);
FAN_API int32 fan_str8_equals(fan_str8, fan_str8);
// trims spaces
FAN_API fan_str8 fan_str8_triml(fan_str8);
FAN_API fan_str8 fan_str8_trimr(fan_str8);
FAN_API fan_str8 fan_str8_substr(fan_str8, ssize);

FAN_API fan_cutstr8 fan_str8_cut(fan_str8, uchar8);


inline uintptr fan_align_forward(uintptr ptr, ssize alignment) {
    return (ptr + (alignment - 1)) & ~(alignment - 1);
}

FAN_API void *fan_heap_make(void *ctx, ssize size);
FAN_API void  fan_heap_free(void *ctx, void *ptr, ssize size);
FAN_API void *fan_heap_resize(void *ctx, void *ptr, ssize old, ssize new);

FAN_API void *fan_arena_make(void *ctx, ssize size);
FAN_API void  fan_arena_free(void *ctx, void *ptr, ssize size);
FAN_API void *fan_arena_resize(void *ctx, void *ptr, ssize old, ssize new);

FAN_API void  fan_arena_clear(fan_arena *a);

FAN_API void *fan_lib_open(const char *path);
FAN_API void *fan_lib_load(void *lib, const char *name);
FAN_API void  fan_lib_close(void *lib);

FAN_API bool32   fan_os_write(fan_pipe pipe, void *data, ssize length);
FAN_API fan_str8 fan_os_read(fan_allocator *mem, const char *path);
FAN_API void     fan_os_wait(uint32 ms);

FAN_API bool32 fan_file_copy(const char *src, const char *dst);
FAN_API bool32 fan_file_delete(const char *path);
FAN_API bool32 fan_file_time_last_written(const char *path, uint64 *last_ms);

// char* LibGetError(void);

// typedef void *(*ThreadFunc)(void *);
// typedef struct FanThread FanThread;
// FAN_API FanThread *FanThreadCreate(ThreadFunc, void *arg);
// FAN_API void FanThreadJoin(FanThread *);
// FAN_API void FanThreadClose(FanThread *);

#endif // FAN_OS_H
