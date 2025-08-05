
#include <string.h>
#define RAYLIB_IMPLEMENTATION
#include <raylib.h>
#include <raymath.h>

#include <inttypes.h>
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
    printf("%s: (%f, %f)\n", name, v.x, v.y);
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

#define ComponentStorageDeclare(name, T) \
    typedef struct name##Storage {       \
         int32 *sparse;                  \
         int32 *dense;                   \
             T *data;                    \
         ssize  size;                    \
         ssize  capacity;                \
    } name##Storage

#define ComponentStorageCreate(storage, mem, size) do{                                 \
    (storage)->sparse   = (mem)->make((mem)->ctx, sizeof(*(storage)->sparse) * size);  \
    (storage)->dense    = (mem)->make((mem)->ctx, sizeof(*(storage)->dense)  * size);  \
    (storage)->data     = (mem)->make((mem)->ctx, sizeof(*(storage)->data)   * size);  \
    (storage)->capacity = size;                                                        \
    memset((storage)->sparse, -1, sizeof(*(storage)->sparse) * size);                  \
    memset((storage)->dense,  -1, sizeof(*(storage)->dense)  * size);                  \
    memset((storage)->data,    0, sizeof(*(storage)->data)   * size);                  \
    }while(0);

// WARN: assert on fail
#define ComponentStorageAdd(storage, id) do{                            \
        (storage)->sparse[id] = (storage)->size;                        \
        (storage)->dense[(storage)->size % (storage)->capacity] = (id); \
        (storage)->size++;                                              \
        if ((storage)->size >= (storage)->capacity)                     \
            assert(false && "Out of Memory!");                          \
    }while(0);

// WARN: no bounds check
#define ComponentStorageDelete(storage, id, count_ptr) do{                      \
        (storage)->dense[(storage)->sparse[id]] = (typeof(*(storage)->dense))0; \
        (storage)->parse[id] = (typeof(*(storage)->parse))-1;                   \
        (storage)->data[(storage)->size] = typeof(*(storage)->data) {0};        \
        if ((storage)->size > 0)                                                \
            (storage)->size--;                                                  \
    }while(0);

typedef struct CMovement {
    Vector2 position;
    Vector2 last_position;

    Vector2 direction;

    float32 speed;
    float32 friction;

    bool32  initialized;
} CMovement;

typedef struct CBody {
    Vector2 scale;
    Vector2 offset;

    int32   layer;

    bool32  initialized;
} CBody;

typedef struct CTexture {
    Texture2D texture;

    Vector2   index;
    Vector2   size;
} CTexture;

typedef struct CBehavior {
    enum BehaviorType {
        BehaviorType_None   = 0,
        BehaviorType_Random = 1
    } type;
    float64 start_time;
    float64 duration;
} CBehavior;

ComponentStorageDeclare(CMovement, CMovement);
ComponentStorageDeclare(CBody, CBody);

ComponentStorageDeclare(CColor, Color);
ComponentStorageDeclare(CTexture, CTexture);

ComponentStorageDeclare(CBehavior, CBehavior);

/*
 * type: System
 * component(s): CMovement, CBody (optional)
 */
void MovementUpdate(CMovement *m, CBody *b, Vector2 direction, float dt) {
    if (not m->initialized) {
        init_if_null(m->position.x, 0.0f);
        init_if_null(m->position.y, 0.0f);

        init_if_null(m->last_position.x, m->position.x);
        init_if_null(m->last_position.y, m->position.y);

        init_if_null(m->direction.x, 1.0f);
        init_if_null(m->direction.y, 1.0f);

        init_if_null(   m->speed,  500.0f);
        init_if_null(m->friction,    1.0f);

        m->initialized = true;
    }

    Vector2 velocity = Vector2Subtract(m->position, m->last_position);
    Vector2 acceleration = Vector2Zero();

    Vector2 screen_size = {
        GetScreenWidth(),
        GetScreenHeight()
    };
    if (b isnt null) {
        screen_size.x = screen_size.x - b->scale.x;
        screen_size.y = screen_size.y - b->scale.y;
    }

    if (m->position.x < 0.0f) {
        m->position.x      = 0.0f;
        m->last_position.x = m->position.x + velocity.x;
    }
    else if (m->position.x > screen_size.x) {
        m->position.x      = screen_size.x;
        m->last_position.x = m->position.x + velocity.x;
    }
    if (m->position.y < 0.0f) {
        m->position.y      = 0.0f;
        m->last_position.y = m->position.y + velocity.y;
    }
    else if (m->position.y > screen_size.y) {
        m->position.y      = screen_size.y;
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

/*
 * type: System
 * component(s): CBody
 */
void BodyUpdate(CBody *b, Vector2 scale, Vector2 offset, float dt) {
    (void)dt;
    if (not b->initialized) {
        init_if_null( b->scale.x, 100.0f);
        init_if_null( b->scale.y, 100.0f);

        b->initialized = true;
    }

    b->scale.x  = coalesce( scale.x, b->scale.x );
    b->scale.y  = coalesce( scale.y, b->scale.y );

    b->offset.x = coalesce(offset.x, b->offset.x);
    b->offset.y = coalesce(offset.y, b->offset.y);
}

/*
 * type: System
 * component(s): CTexture
 */
void TextureUpdate(CTexture *t, Vector2 index, Vector2 size, float dt) {
    (void)dt;

    t->index.x = coalesce(index.x, t->index.x);
    t->index.y = coalesce(index.y, t->index.y);

    t->size.x = coalesce(size.x, t->size.x);
    t->size.y = coalesce(size.y, t->size.y);
}

enum RenderFlags {
    RenderFlag_FlipX = (1 << 0),
    RenderFlag_FlipY = (1 << 1)
};
/*
 * type: System
 * component(s): CBody, CMovement
 */
void BodyRender(CBody *b, CMovement *m, Color color, CTexture *t, int32 flags) {

    if (t is null) {
        if (Vector2Length(b->offset) > 0.0f) {
            DrawRectangleV(Vector2Add(m->position, b->offset), b->scale, GRAY);
        }
        DrawRectangleV(m->position, b->scale, color);
    }
    else {

        Vector2 texture_size = {
            (t->size.x) ? t->size.x : t->texture.width,
            (t->size.y) ? t->size.y : t->texture.height
        };

        Rectangle src = (Rectangle) {
            t->index.x * texture_size.x,
            t->index.y * texture_size.y,
            (flags & RenderFlag_FlipX) ? m->direction.x * texture_size.x : texture_size.x,
            (flags & RenderFlag_FlipY) ? m->direction.y * texture_size.y : texture_size.y
        };
        Rectangle dst = (Rectangle) {
            m->position.x,
            m->position.y,
            b->scale.x,
            b->scale.y
        };

        Rectangle dst_shadow = (Rectangle) {
            m->position.x + b->offset.x,
            m->position.y + b->offset.y,
            b->scale.x,
            b->scale.y
        };

        if (Vector2Length(b->offset) > 0.0f) {
            DrawTexturePro(
                t->texture,
                src,
                dst_shadow,
                (Vector2) { 0.0f, 0.0f },
                0.0f,
                GRAY);
        }

        DrawTexturePro(
            t->texture,
            src,
            dst,
            (Vector2) { 0.0f, 0.0f },
            0.0f,
            color
        );
    }
}

typedef struct RenderEntry {
    int32   id;
    float32 height;
    int32   layer;
} RenderEntry;

int32 SortRenderPartition_(RenderEntry *entries, int32 low, int32 high) {
    RenderEntry pivot = entries[high];
    RenderEntry temp;

    int32 i = low - 1;

    for (int32 j = low; j <= high - 1; j++) {
        if ((entries[j].layer < pivot.layer) or \
            (entries[j].layer == pivot.layer and entries[j].height < pivot.height)) {
            i++;
            temp = entries[j];
            entries[j] = entries[i];
            entries[i] = temp;
        }
    }

    temp = entries[i + 1];
    entries[i + 1] = entries[high];
    entries[high] = temp;

    return i + 1;
}

void SortRender(RenderEntry *entries, int32 low, int32 high) {
    // qsort in-place
    if (low < high) {
        int32 pi = SortRenderPartition_(entries, low, high);

        SortRender(entries, low, pi - 1);
        SortRender(entries, pi + 1, high);
    }
}


enum SpecialEntity {
    Entity_Player_One = 0,
};

typedef struct World {
    uint8             entity_count;
    CBodyStorage      c_body;
    CMovementStorage  c_movement;
    CColorStorage     c_color;
    CTextureStorage   c_texture;
    CBehaviorStorage  c_behavior;

    float64           current_time;
} World;
World world = {};


int main(void) {
    InitWindow(800, 600, "Fantasia");

    bool32 running = true;
    bool32 called_object_dump = false;
    Vector2 player_offset = Vector2Zero();

    SetRandomSeed(12398);

    ssize component_size  = kilobytes(1);

    ComponentStorageCreate(&world.c_body,     &heap_allocator, component_size);
    ComponentStorageCreate(&world.c_movement, &heap_allocator, component_size);
    ComponentStorageCreate(&world.c_color,    &heap_allocator, component_size);
    ComponentStorageCreate(&world.c_texture,  &heap_allocator, component_size);
    ComponentStorageCreate(&world.c_behavior, &heap_allocator, component_size);

    int32 local_behavior_index, local_color_index, local_body_index,
          local_texture_index, local_movement_index;

    // world.c_body.sparse[world.entity_count]
    ComponentStorageAdd(    &world.c_body, world.entity_count);
    ComponentStorageAdd(&world.c_movement, world.entity_count);
    ComponentStorageAdd( &world.c_texture, world.entity_count);

    // local_body_index = world.c_body.sparse[world.entity_count];
    // world.c_body.data[local_body_index].layer = 1;

    local_texture_index = world.c_texture.sparse[world.entity_count];
    world.c_texture.data[local_texture_index].texture = LoadTexture("./resources/mewee.png");
    world.c_texture.data[local_texture_index + 1].texture = world.c_texture.data[local_texture_index].texture;
    world.c_texture.data[local_texture_index + 2].texture = world.c_texture.data[local_texture_index].texture;
    world.entity_count++;

    ComponentStorageAdd(    &world.c_body, world.entity_count);
    ComponentStorageAdd(&world.c_movement, world.entity_count);
    ComponentStorageAdd(   &world.c_color, world.entity_count);
    ComponentStorageAdd( &world.c_texture, world.entity_count);
    ComponentStorageAdd(&world.c_behavior, world.entity_count);

    local_movement_index    = world.c_movement.sparse[world.entity_count];
    local_color_index = world.c_color.sparse[world.entity_count];
    local_behavior_index = world.c_behavior.sparse[world.entity_count];

    world.c_movement.data[local_movement_index].speed = 400.0f;
    world.c_color.data[local_color_index] = (Color){ 50, 255, 255, 255 };
    world.c_behavior.data[local_behavior_index].type = BehaviorType_Random;
    world.c_behavior.data[local_behavior_index].duration = 0.2f;
    world.entity_count++;

    ComponentStorageAdd(    &world.c_body, world.entity_count);
    ComponentStorageAdd(&world.c_movement, world.entity_count);
    ComponentStorageAdd(   &world.c_color, world.entity_count);
    ComponentStorageAdd( &world.c_texture, world.entity_count);
    ComponentStorageAdd(&world.c_behavior, world.entity_count);

    local_movement_index    = world.c_movement.sparse[world.entity_count];
    local_color_index    = world.c_color.sparse[world.entity_count];
    local_behavior_index = world.c_behavior.sparse[world.entity_count];

    world.c_movement.data[local_movement_index].speed = 300.0f;
    world.c_color.data[local_color_index] = (Color){ 255, 50, 255, 255 };
    world.c_behavior.data[local_behavior_index].type = BehaviorType_Random;
    world.c_behavior.data[local_behavior_index].duration = 0.5f;

    world.entity_count++;

    ComponentStorageAdd(    &world.c_body, world.entity_count);
    ComponentStorageAdd(&world.c_movement, world.entity_count);
    ComponentStorageAdd(   &world.c_color, world.entity_count);

    local_movement_index    = world.c_movement.sparse[world.entity_count];
    local_color_index = world.c_color.sparse[world.entity_count];
    local_behavior_index = world.c_behavior.sparse[world.entity_count];

    world.c_movement.data[local_movement_index].position = (Vector2){ 200.0f, 300.0f };
    world.c_color.data[local_color_index] = (Color){ 50, 255, 50, 255 };
    world.entity_count++;

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

        if (IsKeyDown(KEY_K)) {
            player_offset.y += 500.0f * dt;
            if (player_offset.y >= 200.0f) {
                player_offset.y = 200.0f;
            }
        }
        if (IsKeyDown(KEY_I)) {
            player_offset.y -= 500.0f * dt;
            if (player_offset.y <= 0.0f) {
                player_offset.y = 0.0f;
            }
        }
        if (IsKeyDown(KEY_L)) {
            player_offset.x += 500.0f * dt;
            if (player_offset.x >= 200.0f) {
                player_offset.x = 200.0f;
            }
        }
        if (IsKeyDown(KEY_J)) {
            player_offset.x -= 500.0f * dt;
            if (player_offset.x <= 0.0f) {
                player_offset.x = 0.0f;
            }
        }

        if (IsKeyPressed(KEY_P)) {
            called_object_dump = true;
            printf("[[DEBUG INFO]]\n");
        }

        world.current_time = GetTime();

        if (called_object_dump) {
            printf("current_time: %.3f\n", world.current_time);

            printf("Total Component 'Body' size/capacity:\t%zu/%zu\n", world.c_body.size, world.c_body.capacity);
            printf("Total Component 'Movement' size/capacity:\t%zu/%zu\n", world.c_movement.size, world.c_movement.capacity);
            printf("Total Component 'Color' size/capacity:\t%zu/%zu\n", world.c_color.size, world.c_color.capacity);
            printf("Total Component 'Texture' size/capacity:\t%zu/%zu\n", world.c_texture.size, world.c_texture.capacity);
            printf("Total Component 'Behavior' size/capacity:\t%zu/%zu\n", world.c_behavior.size, world.c_behavior.capacity);
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            if (called_object_dump) {
                printf("[CMovement]\n");
            }
            for (ssize i = 0; i < world.c_movement.size; i++) {
                int32 local_id = world.c_movement.dense[i];
                if (local_id == -1) {
                    continue;
                }

                CMovement *local_movement = &world.c_movement.data[i];
                CBehavior *local_behavior = null;

                int32 local_body_index = world.c_body.sparse[local_id];
                CBody *local_body = null;
                if (local_body_index != -1) {
                    local_body = &world.c_body.data[local_body_index];
                }

                if (local_id == Entity_Player_One) {
                    MovementUpdate(local_movement, local_body, player_direction, dt);
                }
                else {
                    int32 local_behavior_index = world.c_behavior.sparse[local_id];
                    Vector2 local_direction = Vector2Zero();
                    if (local_behavior_index != -1) {
                        local_behavior = &world.c_behavior.data[local_behavior_index];

                        switch (local_behavior->type) {
                            case BehaviorType_Random: {
                                if (world.current_time - local_behavior->start_time > local_behavior->duration) {
                                    local_direction = (Vector2) {
                                        GetRandomValue(-1, 1),
                                        GetRandomValue(-1, 1)
                                    };
                                    local_behavior->start_time = world.current_time;
                                }
                                else {
                                    // keeps entity moving rather than staying still
                                    local_direction = local_movement->direction;
                                }
                            } break;
                            case BehaviorType_None:
                            default: {
                                printf("Failing\n");
                            } break;
                        }
                    }
                    MovementUpdate(local_movement, local_body, local_direction, dt);
                }

                if (called_object_dump) {
                    printf("\tlocal_id: %d\n", local_id);
                    printf("\tlocal_movement_index: %zu\n", i);
                    printf("\tlocal_movement->initialized: %s\n", local_movement->initialized ? "true" : "false");
                    printf("\tlocal_movement->speed: %f\n", local_movement->speed);
                    printf("\tlocal_movement->friction: %f\n", local_movement->friction);
                    printf("\t");
                    Vector2Print(local_movement->position);
                    printf("\t");
                    Vector2Print(local_movement->last_position);
                    printf("\t");
                    Vector2Print(local_movement->direction);

                    if (local_behavior isnt null) {
                        printf("\tlocal_behavior->type: %d\n", local_behavior->type);
                        printf("\tlocal_behavior->start_time: %f\n", local_behavior->start_time);
                        printf("\tlocal_behavior->duration: %f\n", local_behavior->duration);
                    }
                }
            }

            if (called_object_dump) {
                printf("[CBody]\n");
            }

            RenderEntry temp_array[128] = { { -1, 0.0f, 0 } };
            for (ssize i = 0; i < world.c_body.size; i++) {
                int32 local_id = world.c_body.dense[i];
                if (local_id == -1) {
                    continue;
                }
                CBody *local_body = &world.c_body.data[i];

                int32 local_movement_index = world.c_movement.sparse[local_id];
                if (local_movement_index == -1) {
                    // won't render anyways
                    continue;
                }
                CMovement *local_movement = &world.c_movement.data[local_movement_index];

                temp_array[i] = (RenderEntry) {
                    local_id,
                    local_movement->position.y + local_body->scale.y,
                    local_body->layer
                };
            }

            SortRender(temp_array, 0, world.c_body.size - 1);

            for (ssize i = 0; i < world.c_body.size; i++) {
                // int32 local_id = world.c_body.dense[i];
                int32 local_id = temp_array[i].id;
                if (local_id == -1) {
                    continue;
                }

                int32 local_body_index = world.c_body.sparse[local_id];
                CBody *local_body = &world.c_body.data[local_body_index];

                if (local_id == Entity_Player_One) {
                    BodyUpdate(local_body, Vector2Zero(), player_offset, dt);
                }
                else {
                    BodyUpdate(local_body, Vector2Zero(), Vector2Zero(), dt);
                }

                int32 local_movement_index = world.c_movement.sparse[local_id];
                if (local_movement_index == -1) {
                    // NOTE(liam): skip render if not found
                    continue;
                }
                CMovement *local_movement = &world.c_movement.data[local_movement_index];

                int32 local_texture_index = world.c_texture.sparse[local_id];
                CTexture *local_texture = null;
                if (local_texture_index != -1) {
                    local_texture = &world.c_texture.data[local_texture_index];

                    TextureUpdate(local_texture, Vector2Zero(), Vector2Zero(), dt);
                }

                int32 local_color_index = world.c_color.sparse[local_id];
                Color local_color = (Color){ 255, 255, 255, 255 };
                if (local_color_index != -1) {
                    local_color = world.c_color.data[local_color_index];
                }

                BodyRender(local_body, local_movement, local_color, local_texture, RenderFlag_FlipX);

                if (called_object_dump) {
                    printf("\tlocal_id: %d\n", local_id);
                    printf("\tlocal_body_index: %zu\n", i);
                    printf("\tlocal_body->initialized: %s\n", local_body->initialized ? "true" : "false");
                    printf("\t");
                    Vector2Print(local_body->scale);
                    printf("\t");
                    Vector2Print(local_body->offset);
                }
            }

            DrawFPS(2, 2);
        EndDrawing();
        called_object_dump = false;
    }

    for (ssize i = 0; i < world.c_texture.size; i++) {
        if (world.c_texture.dense[i] == -1) {
            continue;
        }
        UnloadTexture(world.c_texture.data[i].texture);
    }

    CloseWindow();
    return 0;
}
