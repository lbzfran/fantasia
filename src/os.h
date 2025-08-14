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

typedef char       char8;
typedef char16_t   char16;

typedef uint8_t    uint8;
typedef uint32_t   uint32;
typedef uint64_t   uint64;

typedef int32_t    bool32;
typedef int32_t    int32;

typedef float      float32;
typedef double     float64;

typedef size_t     usize;
typedef ptrdiff_t  ssize;
typedef uintptr_t  uintptr;

#define assert(c)           while (!(c)) __builtin_unreachable()

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

#define null    0
#define kilobytes(x)    ((x)*1024LL)
#define megabytes(x)    (kilobytes(x)*1024LL)
#define gigabytes(x)    (megabytes(x)*1024LL)

#if !defined(min) && !defined(max)
# define min(x,y)        ((x) < (y) ? (x) : (y))
# define max(x,y)        ((x) > (y) ? (x) : (y))
#endif

#define clamp(x, a, b)   min(max(x, a), b)

typedef struct allocator {
    void *(*make)   (void *ctx, ssize);
    void  (*free)   (void *ctx, void *, ssize);
    void *(*resize) (void *ctx, void *, ssize, ssize);
    void *ctx;
} Allocator;

typedef struct Arena {
    uint8 *data;
    ssize  size;
    ssize  capacity;
} Arena;
#define ARENA_ALIGNMENT 16

inline uintptr align_forward(uintptr ptr, ssize alignment) {
    return (ptr + (alignment - 1)) & ~(alignment - 1);
}

void *heap_make(void *ctx, ssize size);
void  heap_free(void *ctx, void *ptr, ssize size);
void *heap_resize(void *ctx, void *ptr, ssize old, ssize new);

void *arena_make(void *ctx, ssize size);
void arena_free(void *ctx, void *ptr, ssize size);
void *arena_resize(void *ctx, void *ptr, ssize old, ssize new);

void arena_clear(Arena *a);

void* LibOpen(const char* path);
void* LibLoad(void *lib, const char *name);
void LibClose(void *lib);

// char* LibGetError(void);

// typedef void *(*ThreadFunc)(void *);
// typedef struct FanThread FanThread;
// FAN_API FanThread *FanThreadCreate(ThreadFunc, void *arg);
// FAN_API void FanThreadJoin(FanThread *);
// FAN_API void FanThreadClose(FanThread *);

#endif // FAN_OS_H
