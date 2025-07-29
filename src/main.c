
#define RAYLIB_IMPLEMENTATION
#include <raylib.h>
#include <raymath.h>

#include <stdint.h>
#include <stddef.h>
#include <uchar.h>

typedef char        byte;
typedef char        char8;
typedef char16_t    char16;

typedef uint8_t     uint8;
typedef uint32_t    uint32;
typedef uint64_t    uint64;

typedef int32_t     bool32;
typedef int32_t     int32;

typedef float       float32;
typedef double      float64;

typedef size_t      usize;
typedef ptrdiff_t   ssize;
typedef uintptr_t   uintptr;

#define assert(c)   while (!(c)) __builtin_unreachable()

#define sizeof(x)       (ssize)sizeof(x)
#define alignof(x)      (_Alignof(x))
#define countof(a)      (sizeof(a) / sizeof(*(a)))
#define lengthof(s)     (countof(s) - 1)
#define signof(x)       ((x) > 0) ? 1 : (((x) < 0) ? -1 : 0)

#define coalesce(a, b)  ((a) ? (a) : (b))

#define init_if_null(a, x)    ((a) = coalesce((a), (x)))

#define not     !
#define is      ==
#define isnt    !=
#define and     &&
#define or      ||

#define null    0
#define kilobytes(x)    ((x)*1024LL)
#define megabytes(x)    (kilobytes(x)*1024LL)
#define gigabytes(x)    (megabytes(x)*1024LL)

#define min(x,y)        ((x) < (y) ? (x) : (y))
#define max(x,y)        ((x) > (y) ? (x) : (y))

typedef struct Mob {
    Vector2 position;
    Vector2 scale;

    float32 speed;
    float32 friction;

    Vector2 direction;
    Vector2 last_position;

    bool32 initialized;
} Mob;

void MobUpdate(Mob *mob, Vector2 direction, float32 dt) {
    if (not mob->initialized) {
        init_if_null(mob->scale.x, 1.0f);
        init_if_null(mob->scale.y, 1.0f);

        init_if_null(mob->direction.x, 1.0f);
        init_if_null(mob->direction.y, 1.0f);

        init_if_null(mob->speed, 1200.0f);
        init_if_null(mob->friction, 1.0f);

        mob->initialized = true;
    }

    Vector2 velocity = Vector2Subtract(mob->position, mob->last_position);
    Vector2 acceleration = (Vector2){ 0.0f, 0.0f };

    if (mob->position.x < 0.0f) {
        mob->position.x = 0.0f;
        mob->last_position.x = mob->position.x + velocity.x;
    }
    // else if ...
    if (mob->position.y < 0.0f) {
        mob->position.y = 0.0f;
        mob->last_position.y = mob->position.y + velocity.y;
    }
    // else if ...

    mob->direction.x = coalesce(direction.x, mob->direction.x);
    mob->direction.y = coalesce(direction.y, mob->direction.y);
    direction = Vector2Normalize(direction);
}

typedef struct World {
    Mob mobs[255];
} World;

int main(void) {
    InitWindow(800, 600, "Fantasia");
    bool running = true;

    while (running) {
        if (WindowShouldClose() || IsKeyPressed(KEY_Q)) {
            running = false;
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
