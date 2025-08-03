
#include <string.h>
#define RAYLIB_IMPLEMENTATION
#include <raylib.h>
#include <raymath.h>

#include <stdint.h>
#include <stddef.h>
#include <uchar.h>

#include <stdio.h>
#include <stdlib.h>

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

#define assert(c)               while (!(c)) __builtin_unreachable()

#define sizeof(x)               (ssize)sizeof(x)
#define alignof(x)              (_Alignof(x))
#define countof(a)              (sizeof(a) / sizeof(*(a)))
#define lengthof(s)             (countof(s) - 1)
#define signof(x)               ((x) > 0) ? 1 : (((x) < 0) ? -1 : 0)

#define coalesce(a, b)          ((a) ? (a) : (b))
#define init_if_null(a, x)      ((a) = coalesce((a), (x)))

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

// #define min(x,y)        ((x) < (y) ? (x) : (y))
// #define max(x,y)        ((x) > (y) ? (x) : (y))

void Vector2Print_(Vector2 v, const char *name) {
    printf("%s:(%f, %f)\n", name, v.x, v.y);
}
#define Vector2Print(v) Vector2Print_(v, #v)

typedef struct allocator {
    void *(*make)   (void *ctx, ssize);
    void  (*free)   (void *ctx, void *, ssize);
    void *(*resize) (void *ctx, void *, ssize, ssize);
    void *ctx;
} Allocator;

global void *heap_allocator_make(void *ctx, ssize size) {
    (void)ctx;
    void *result = malloc(size);
    assert(result && "ERROR: Reached Out-Of-Memory state.");

    return result;
}

global void heap_allocator_free(void *ctx, void *ptr, ssize size) {
    (void)ctx;
    (void)size;

    free(ptr);
    ptr = null;
}

global void *heap_allocator_resize(void *ctx, void *ptr, ssize old, ssize new) {
    (void)ctx;
    void *result = heap_allocator_make(ctx, new);

    if (ptr isnt null) {
        if (new > old) {
            memcpy(result, ptr, old);
        }
        else {
            memmove(result, ptr, old);
        }
        heap_allocator_free(ctx, ptr, old);
    }

    return result;
}

Allocator heap_allocator = {
    .make   = heap_allocator_make,
    .free   = heap_allocator_free,
    .resize = heap_allocator_resize,
    .ctx    = null
};

typedef struct Mob {
    Vector2 position;
    Vector2 scale;

    float32 speed;
    float32 friction;

    Vector2 direction;
    Vector2 last_position;

    Vector2 offset;

    bool32 initialized;
} Mob;

#define ComponentStorage(name, T)  typedef struct name##Storage { \
                                   uint32 *sparse;                \
                                   uint32 *dense;                 \
                                   T      *data;                  \
                                   uint32  size;                  \
                                   uint32  capacity;              \
                              } name##Storage

// WARN: no bounds check
#define ComponentStorageAdd(storage, id) do{                \
        (storage)->sparse[id] = (storage)->size;            \
        (storage)->dense[(storage)->size++] = (uint32)id;   \
    }while(0);

// WARN: no bounds check
#define ComponentStorageDelete(storage, id, count_ptr) do{             \
        (storage)->dense[(storage)->sparse[id]] = (uint32)0;           \
        (storage)->parse[id] = (uint32)0;                              \
        (storage)->data[(storage)->size--] = typeof((storage)->data)0; \
    }while(0);

typedef struct CMovement {
    Vector2 position;
    Vector2 last_position;

    Vector2 direction;

    float32 speed;
    float32 friction;

    bool32 initialized;
} CMovement;

ComponentStorage(CMovement, CMovement);

typedef Vector2 CScale;
ComponentStorage(CScale, Vector2);

/*
 * type: System
 */
void MovementUpdate(CMovement *m, Vector2 scale, Vector2 direction, float dt) {
    if (not m->initialized) {
        init_if_null(m->position.x, (float)GetScreenWidth()/2);
        init_if_null(m->position.y, (float)GetScreenHeight()/2);

        init_if_null(m->last_position.x, m->position.x);
        init_if_null(m->last_position.y, m->position.y);

        init_if_null(m->direction.x, 1.0f);
        init_if_null(m->direction.y, 1.0f);

        init_if_null(m->speed, 500.0f);
        init_if_null(m->friction, 1.0f);

        m->initialized = true;
    }

    Vector2 velocity = Vector2Subtract(m->position, m->last_position);
    Vector2 acceleration = Vector2Zero();

    if (m->position.x < 0.0f) {
        m->position.x = 0.0f;
        m->last_position.x = m->position.x + velocity.x;
    }
    else if (m->position.x > GetScreenWidth() - scale.x) {
        m->position.x = GetScreenWidth() - scale.x;
        m->last_position.x = m->position.x + velocity.x;
    }
    if (m->position.y < 0.0f) {
        m->position.y = 0.0f;
        m->last_position.y = m->position.y + velocity.y;
    }
    else if (m->position.y > GetScreenHeight() - scale.y) {
        m->position.y = GetScreenHeight() - scale.y;
        m->last_position.y = m->position.y + velocity.y;
    }

    m->direction.x = coalesce(direction.x, m->direction.x);
    m->direction.y = coalesce(direction.y, m->direction.y);
    direction = Vector2Normalize(direction);

    if (Vector2Length(direction) > 0) {
        acceleration = Vector2Add(acceleration, Vector2Scale(direction, m->speed));
    }
    else if (Vector2Length(velocity) > 0) {
        acceleration = Vector2Subtract(acceleration, Vector2Scale(velocity, m->friction));
    }

    m->last_position = m->position;
    // NOTE: c->position += (velocity + acceleration * dt) * dt;
    m->position = Vector2Add(m->position, Vector2Scale(Vector2Add(velocity, acceleration), dt));
}

void MobUpdate(Mob *mob, Vector2 direction, Vector2 offset, float32 dt) {
    if (not mob->initialized) {
        init_if_null(mob->position.x, (float)GetScreenWidth()/2);
        init_if_null(mob->position.y, (float)GetScreenHeight()/2);

        init_if_null(mob->last_position.x, mob->position.x);
        init_if_null(mob->last_position.y, mob->position.y);

        init_if_null(mob->scale.x, 100.0f);
        init_if_null(mob->scale.y, 100.0f);

        init_if_null(mob->direction.x, 1.0f);
        init_if_null(mob->direction.y, 1.0f);

        init_if_null(mob->speed, 500.0f);
        init_if_null(mob->friction, 1.0f);

        mob->initialized = true;
    }

    Vector2 velocity = Vector2Subtract(mob->position, mob->last_position);
    Vector2 acceleration = Vector2Zero();

    if (mob->position.x < 0.0f) {
        mob->position.x = 0.0f;
        mob->last_position.x = mob->position.x + velocity.x;
    }
    else if (mob->position.x > GetScreenWidth() - mob->scale.x) {
        mob->position.x = GetScreenWidth() - mob->scale.x;
        mob->last_position.x = mob->position.x + velocity.x;
    }
    if (mob->position.y < 0.0f) {
        mob->position.y = 0.0f;
        mob->last_position.y = mob->position.y + velocity.y;
    }
    else if (mob->position.y > GetScreenHeight() - mob->scale.y) {
        mob->position.y = GetScreenHeight() - mob->scale.y;
        mob->last_position.y = mob->position.y + velocity.y;
    }

    mob->direction.x = coalesce(direction.x, mob->direction.x);
    mob->direction.y = coalesce(direction.y, mob->direction.y);
    direction = Vector2Normalize(direction);

    if (Vector2Length(direction) > 0) {
        acceleration = Vector2Add(acceleration, Vector2Scale(direction, mob->speed));
    }
    else if (Vector2Length(velocity) > 0) {
        acceleration = Vector2Subtract(acceleration, Vector2Scale(velocity, mob->friction));
    }

    mob->last_position = mob->position;
    // NOTE: mob->position += (velocity + acceleration * dt) * dt;
    mob->position = Vector2Add(mob->position, Vector2Scale(Vector2Add(velocity, acceleration), dt));

    mob->offset = offset;
}

void MobRender(Mob *mob) {
    DrawRectangleV(Vector2Add(mob->position, mob->offset), mob->scale, GRAY);
    DrawRectangleV(mob->position, mob->scale, BLACK);
}

typedef struct World {
    Mob mobs[255];
    uint8 mob_count;

    uint8 entities[255];
} World;
World world = {};


int main(void) {
    InitWindow(800, 600, "Fantasia");
    bool running = true;
    world.mobs[world.mob_count++] = (Mob){ 0 };
    Vector2 player_offset = Vector2Zero();

    while (running) {
        float dt = GetFrameTime();
        if (WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
            running = false;
        }

        Vector2 player_direction = Vector2Zero();
        if (IsKeyDown(KEY_W)) {
            player_direction.y -= 1;
        }
        if (IsKeyDown(KEY_S)) {
            player_direction.y += 1;
        }
        if (IsKeyDown(KEY_A)) {
            player_direction.x -= 1;
        }
        if (IsKeyDown(KEY_D)) {
            player_direction.x += 1;
        }

        if (IsKeyDown(KEY_I)) {
            player_offset.y += 1000.0f * dt;
            if (player_offset.y >= 200.0f) {
                player_offset.y = 200.0f;
            }
        }
        if (IsKeyDown(KEY_K)) {
            player_offset.y -= 1000.0f * dt;
            if (player_offset.y <= 0.0f) {
                player_offset.y = 0.0f;
            }
        }
        if (IsKeyDown(KEY_J)) {
            player_offset.x += 1000.0f * dt;
            if (player_offset.x >= 200.0f) {
                player_offset.x = 200.0f;
            }
        }
        if (IsKeyDown(KEY_L)) {
            player_offset.x -= 1000.0f * dt;
            if (player_offset.x <= 0.0f) {
                player_offset.x = 0.0f;
            }
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            for (uint8 i = 0; i < world.mob_count; i++) {
                if (i == 0) {
                    MobUpdate(&world.mobs[i], player_direction, player_offset, dt);
                }
                else {
                    MobUpdate(&world.mobs[i], Vector2One(), Vector2Zero(), dt);
                }
                MobRender(&world.mobs[i]);
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
