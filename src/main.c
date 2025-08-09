
#include <string.h>
#include "platform.h"

#include <inttypes.h>
#include <stddef.h>
#include <uchar.h>

#include <stdio.h>
#include <stdlib.h>

typedef char       byte;
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

void FanVector2Print_(FanVector2 v, const char *name) {
    printf("%s: (%f, %f)\n", name, v.x, v.y);
}
#define FanVector2Print(v) FanVector2Print_(v, #v)

typedef struct allocator {
    void *(*make)   (void *ctx, ssize);
    void  (*free)   (void *ctx, void *, ssize);
    void *(*resize) (void *ctx, void *, ssize, ssize);
    void *ctx;
} Allocator;

global void *heap_make(void *ctx, ssize size) {
    (void)ctx;
    void *result = malloc(size);
    assert(result && "ERROR: Reached Out-Of-Memory state.");

    return result;
}

global void heap_free(void *ctx, void *ptr, ssize size) {
    (void)ctx;
    (void)size;

    free(ptr);
    ptr = null;
}

global void *heap_resize(void *ctx, void *ptr, ssize old, ssize new) {
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

Allocator heap_allocator = {
    .make   = heap_make,
    .free   = heap_free,
    .resize = heap_resize,
    .ctx    = null
};

typedef struct Arena {
    uint8 *data;
    ssize  size;
    ssize  capacity;
} Arena;

inline uintptr align_forward(uintptr ptr, ssize alignment) {
    return (ptr + (alignment - 1)) & ~(alignment - 1);
}

#define ARENA_ALIGNMENT 16

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


#define ComponentStorageArgs(storage, id, ...) do{                                                      \
        assert((storage)->sparse[id] != -1 && "Attempted to pass component args to unassigned entity"); \
        (storage)->data[(storage)->sparse[id]] = (typeof(*(storage)->data)){__VA_ARGS__};               \
    }while(0);

#define ComponentStorageAddArgs(storage, id, ...) do{ \
    ComponentStorageAdd(storage, id);                 \
    ComponentStorageArgs(storage, id, __VA_ARGS__);   \
}while(0);

// WARN: no bounds check
#define ComponentStorageDelete(storage, id, count_ptr) do{                      \
        (storage)->dense[(storage)->sparse[id]] = (typeof(*(storage)->dense))0; \
        (storage)->parse[id] = (typeof(*(storage)->parse))-1;                   \
        (storage)->data[(storage)->size] = typeof(*(storage)->data) {0};        \
        if ((storage)->size > 0)                                                \
            (storage)->size--;                                                  \
    }while(0);

typedef enum {
    MovementFlag_Immovable   = (1 << 0),
    MovementFlag_NoCollision = (1 << 1)
} MovementFlags;

typedef struct CMovement {
    FanVector2 position;
    FanVector2 last_position;
    FanVector2 direction;

    float32 speed;
    float32 friction;

    int32 movement_flags;
    bool32 initialized;
} CMovement;

typedef struct CBody {
    FanVector2 scale;
    FanVector2 offset;

    int32 layer;

    bool32 initialized;
} CBody;

typedef struct CTexture {
    FanTexture texture;

    FanRect rect;
} CTexture;

typedef struct CBehavior {
    enum BehaviorType {
        BehaviorType_None   = 0,
        BehaviorType_Random = 1
    } type;
    float64 start_time;
    float64 duration;
} CBehavior;

// state manager component
typedef struct CAnimation {
    const char8 *name;
    int32 id;
    float32 timer;
    int32 current_frame;
    bool32 finished;
    int32 animation_flags;
} CAnimation;

typedef struct {
    const char8 *name;
    FanRect *frames;
    float32 frame_time;
    int32 frame_count;
    bool32 loop;

    int32 next_id;
} AnimationData;

#define FanRect_EMPTY (FanRect){ 0 }

FanRect player_idle_up_frames[1]    = { FanRect_EMPTY };
FanRect player_idle_down_frames[3]  = { FanRect_EMPTY };
FanRect player_idle_left_frames[3]  = { FanRect_EMPTY };
FanRect player_idle_right_frames[3] = { FanRect_EMPTY };

AnimationData anim_table[] = {
    { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.5f, .frame_count = 3, true,  -1 },
    { "player_idle_up",    player_idle_up_frames,    .frame_time = 1.5f, .frame_count = 1, false,  0 },
    { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.5f, .frame_count = 3, false,  0 },
    { "player_idle_right", player_idle_right_frames, .frame_time = 0.5f, .frame_count = 3, false,  0 },
};

ComponentStorageDeclare(CMovement, CMovement);
ComponentStorageDeclare(CBody, CBody);

ComponentStorageDeclare(CColor, FanColor);
ComponentStorageDeclare(CTexture, CTexture);

ComponentStorageDeclare(CBehavior, CBehavior);
ComponentStorageDeclare(CAnimation, CAnimation);

/*
 * type: System
 * component(s): CMovement, CBody (optional)
 */
void MovementUpdate(CMovement *m, CBody *b, FanVector2 direction, float dt) {
    if (not m->initialized) {
        init_if_null(m->position.x,      0.0f);
        init_if_null(m->position.y,      0.0f);

        init_if_null(m->last_position.x, m->position.x);
        init_if_null(m->last_position.y, m->position.y);

        init_if_null(m->direction.x,     1.0f);
        init_if_null(m->direction.y,     1.0f);

        init_if_null(m->speed,           500.0f);
        init_if_null(m->friction,        1.0f);

        m->initialized = true;
    }

    FanVector2 velocity = FanVector2Sub(m->position, m->last_position);
    FanVector2 acceleration = FanVector2Zero();

    FanVector2 screen_size = {
        FanWindowWidth(),
        FanWindowHeight()
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
    direction = FanVector2Normalize(direction);

    if (FanVector2Length(direction) > 0) {
        acceleration = FanVector2Add(acceleration, FanVector2Scale(direction, m->speed));
    }
    else if (FanVector2Length(velocity) > 0) {
        acceleration = FanVector2Sub(acceleration, FanVector2Scale(velocity, m->friction));
    }

    m->last_position = m->position;
    // NOTE: c->position += (velocity + acceleration * dt) * dt;
    m->position = FanVector2Add(m->position, FanVector2Scale(FanVector2Add(velocity, acceleration), dt));
}

bool32 CollisionCheck(FanVector2 aPos, FanVector2 aSize, FanVector2 bPos, FanVector2 bSize) {
    bool32 result = false;

    // AABB
    result = not (aPos.x + aSize.x < bPos.x or bPos.x + bSize.x < aPos.x or
                  aPos.y + aSize.y < bPos.y or bPos.y + bSize.y < aPos.y);

    return result;
}

void CollisionResolve(CMovement *aMove, CBody aBody, CMovement *bMove, CBody bBody, float32 dt) {
    if (CollisionCheck(aMove->position, aBody.scale, bMove->position, bBody.scale)) {
        FanVector2 aMax = (FanVector2){
            aMove->position.x + aBody.scale.x,
                aMove->position.y + aBody.scale.y
        };
        FanVector2 bMax = (FanVector2){
            aMove->position.x + aBody.scale.x,
                aMove->position.y + aBody.scale.y
        };

        FanVector2 overlap = (FanVector2){
            min(aMax.x, bMax.x) - max(aMove->position.x, bMove->position.x),
                min(aMax.y, bMax.y) - max(aMove->position.y, bMove->position.y)
        };

        if (overlap.x <= 0.0f || overlap.y <= 0.0f)
            return;

        float32 correction;
        int32 aMovable = (aMove->movement_flags & MovementFlag_Immovable) ? 0 : 1;
        int32 bMovable = (bMove->movement_flags & MovementFlag_Immovable) ? 0 : 1;
        if (overlap.x < overlap.y) {
            correction = overlap.x;
            if (aMovable and bMovable) {
                correction *= 0.5f;
            }
            if (aMove->position.x < bMove->position.x) {
                aMove->position.x -= correction * aMovable;
                bMove->position.x += correction * bMovable;
            } else {
                aMove->position.x += correction * aMovable;
                bMove->position.x -= correction * bMovable;
            }
        } else {
            correction = overlap.y;
            if (aMovable and bMovable) {
                correction *= 0.5f;
            }
            if (aMove->position.y < bMove->position.y) {
                aMove->position.y -= correction * aMovable;
                bMove->position.y += correction * bMovable;
            } else {
                aMove->position.y += correction * aMovable;
                bMove->position.y -= correction * bMovable;
            }
        }
    }
    // NOTE(liam): potentially handle 'tunneling' if needed
    // likely solution: https://blog.hamaluik.ca/posts/swept-aabb-collision-using-minkowski-difference/
}

/*
 * type: System
 * component(s): CBody
 */
void BodyUpdate(CBody *b, FanVector2 scale, FanVector2 offset, int32 layer, float dt) {
    (void)dt;
    if (not b->initialized) {
        init_if_null(b->scale.x, 96.0f);
        init_if_null(b->scale.y, 96.0f);

        init_if_null(b->layer,   2);

        b->initialized = true;
    }

    b->scale.x  = coalesce(scale.x,  b->scale.x);
    b->scale.y  = coalesce(scale.y,  b->scale.y);

    b->offset.x = coalesce(offset.x, b->offset.x);
    b->offset.y = coalesce(offset.y, b->offset.y);

    if (layer != -1) {
        b->layer = layer;
    }
}

/*
 * type: System
 * component(s): CTexture
 */
void TextureUpdate(CTexture *t, FanVector2 pos, FanVector2 size, float dt) {
    (void)dt;

    t->rect = (FanRect){
        .x      = pos.x,
        .y      = pos.y,
        .width  = coalesce(size.x, t->rect.width),
        .height = coalesce(size.y, t->rect.height)
    };
}

inline CAnimation AnimationApply(int32 new_id, int32 flags) {
    assert(new_id != -1 && "Out of Bounds Access!");
    CAnimation new_state = (CAnimation) {
        .id = new_id,
        .name = anim_table[new_id].name,
        .animation_flags = flags
    };
    return new_state;
}

typedef enum {
    AnimationFlag_NotInterruptible = (1 << 0),
    AnimationFlag_DisableLoop      = (1 << 1),
} AnimationFlags;
/*
 * type: System
 * components: CAnimation, CTexture
 */
void AnimationUpdate(CAnimation *a, CTexture *t, int32 request_id, int32 flags, float dt) {
    if (request_id != -1 and (a->animation_flags & AnimationFlag_NotInterruptible) == false) {
        *a = AnimationApply(request_id, flags);
    }

    AnimationData *data = &anim_table[a->id];

    a->timer += dt;
    if (a->timer >= data->frame_time) {
        a->timer -= data->frame_time;
        a->current_frame++;

        if (a->current_frame >= data->frame_count) {
            if (data->loop and (a->animation_flags & AnimationFlag_DisableLoop) == false) {
                a->current_frame = 0;
            }
            else {
                if (data->next_id != -1) {
                    *a = AnimationApply(data->next_id, flags);
                }
                else {
                    a->finished = true;
                    a->current_frame = data->frame_count - 1;
                }
            }
        }
    }

    if (a->current_frame >= data->frame_count) {
        a->current_frame = data->frame_count > 0 ? data->frame_count - 1 : 0;
    }

    FanRect current_data = data->frames[a->current_frame];
    TextureUpdate(
        t,
        (FanVector2){ current_data.x, current_data.y },
        (FanVector2){ current_data.width, current_data.height },
        dt
    );
}

typedef enum {
    RenderFlag_FlipX = (1 << 0),
    RenderFlag_FlipY = (1 << 1)
} RenderFlags;
/*
 * type: System
 * component(s): CBody, CMovement
 */
void BodyRender(CBody *b, CMovement *m, FanColor color, CTexture *t, int32 flags) {
    if (t is null) {
        if (FanVector2Length(b->offset) > 0.0f) {
            FanDrawRectV(FanVector2Add(m->position, b->offset), b->scale, (FanColor){ 50, 50, 50, 255 });
        }
        FanDrawRectV(m->position, b->scale, color);
    }
    else {
        float width  = (t->rect.width)  ? t->rect.width  : t->texture.width;
        float height = (t->rect.height) ? t->rect.height : t->texture.height;
        if (flags & RenderFlag_FlipX) {
            width  *= m->direction.x;
        }
        if (flags & RenderFlag_FlipY) {
            height *= m->direction.y;
        }

        FanRect src = (FanRect) {
            t->rect.x,
            t->rect.y,
            width,
            height
        };
        FanRect dst = (FanRect) {
            m->position.x,
            m->position.y,
            b->scale.x,
            b->scale.y
        };

        FanRect dst_shadow = (FanRect) {
            m->position.x + b->offset.x,
            m->position.y + b->offset.y,
            b->scale.x,
            b->scale.y
        };

        if (FanVector2Length(b->offset) > 0.0f) {
            FanDrawTexture(
                t->texture,
                src,
                dst_shadow,
                (FanVector2) { 0.0f, 0.0f },
                0.0f,
                FanColor_GRAY
            );
        }

        FanDrawTexture(
            t->texture,
            src,
            dst,
            (FanVector2) { 0.0f, 0.0f },
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


typedef enum {
    Entity_Player_One = 0,
    Entity_Background = 4,
} SpecialEntity;

typedef struct World {
    uint8             entity_count;

    Arena             arena;

    CBodyStorage      c_body;
    CMovementStorage  c_movement;
    CColorStorage     c_color;
    CTextureStorage   c_texture;
    CBehaviorStorage  c_behavior;
    CAnimationStorage c_animation;

    float64           current_time;
} World;
World world = {};
bool32 called_object_dump = false;

int32 dynamic_entities[128] = { -1 };
int32 static_entities[128]  = { -1 };
ssize dynamic_entity_count = 0;
ssize static_entity_count  = 0;

// NOTE(liam): must call whenever entities are added/removed
global void UpdateEntitySplit(void) {
    dynamic_entity_count = 0;
    static_entity_count  = 0;
    for (ssize local_movement_index = 0; local_movement_index < world.c_movement.size; local_movement_index++) {
        int32 local_id = world.c_movement.dense[local_movement_index];
        if (local_id == -1) continue;

        CMovement *local_movement = &world.c_movement.data[local_movement_index];
        if (local_movement->movement_flags & MovementFlag_NoCollision) continue;

        if (local_movement->movement_flags & MovementFlag_Immovable) {
            static_entities[static_entity_count++] = local_id;
        }
        else {
            dynamic_entities[dynamic_entity_count++] = local_id;
        }
    }
}

void UpdateAndRender(FanVector2 player_direction, FanVector2 player_offset, int32 player_animation_id, float32 dt) {
    if (called_object_dump) {
        printf("[CMovement]\n");
    }

    for (ssize i = 0; i < dynamic_entity_count; i++) {
        int32 local_id = dynamic_entities[i];

        int32 local_movement_index = world.c_movement.sparse[local_id];
        CMovement *local_movement = &world.c_movement.data[local_movement_index];

        CBody *local_body = null;
        int32 local_body_index = world.c_body.sparse[local_id];
        assert(local_body_index != -1);
        local_body = &world.c_body.data[local_body_index];

        // COLLISION CHECK HERE
        for (ssize j = i + 1; j < dynamic_entity_count; j++) {
            int32 other_id = dynamic_entities[j];
            int32 other_movement_index = world.c_movement.sparse[other_id];
            CMovement *other_movement = &world.c_movement.data[other_movement_index];

            int32 other_body_index = world.c_body.sparse[other_id];
            assert(other_body_index != -1);
            CBody *other_body = &world.c_body.data[other_body_index];

            CollisionResolve(local_movement, *local_body, other_movement, *other_body, dt);
        }
        for (ssize s = 0; s < static_entity_count; s++) {
            int32 other_id = static_entities[s];
            int32 other_movement_index = world.c_movement.sparse[other_id];
            CMovement *other_movement = &world.c_movement.data[other_movement_index];

            int32 other_body_index = world.c_body.sparse[other_id];
            assert(other_body_index != -1);
            CBody *other_body = &world.c_body.data[other_body_index];

            CollisionResolve(local_movement, *local_body, other_movement, *other_body, dt);
        }

        CBehavior *local_behavior = null;
        FanVector2 local_direction = FanVector2Zero();
        if (local_id == Entity_Player_One) {
            local_direction = player_direction;
        }
        else {
            int32 local_behavior_index = world.c_behavior.sparse[local_id];
            if (local_behavior_index != -1) {
                local_behavior = &world.c_behavior.data[local_behavior_index];

                switch (local_behavior->type) {
                    case BehaviorType_Random: {
                        if (world.current_time - local_behavior->start_time > local_behavior->duration) {
                            local_direction = (FanVector2) {
                                FanRandomInt(-1, 1),
                                FanRandomInt(-1, 1)
                            };
                            local_behavior->start_time = world.current_time;
                        }
                        else {
                            // keeps entity moving rather than staying still
                            local_direction = local_movement->direction;
                        }
                    } break;
                    case BehaviorType_None:
                    default: break;
                }
            }
        }
        MovementUpdate(local_movement, local_body, local_direction, dt);

        if (called_object_dump) {
            printf("\tlocal_id: %d\n", local_id);
            printf("\tlocal_movement_index: %zu\n", i);
            printf("\tlocal_movement->initialized: %s\n", local_movement->initialized ? "true" : "false");
            printf("\tlocal_movement->speed: %f\n", local_movement->speed);
            printf("\tlocal_movement->friction: %f\n", local_movement->friction);
            printf("\t");
            FanVector2Print(local_movement->position);
            printf("\t");
            FanVector2Print(local_movement->last_position);
            printf("\t");
            FanVector2Print(local_movement->direction);

            if (local_behavior isnt null) {
                printf("\tlocal_behavior->type: %d\n", local_behavior->type);
                printf("\tlocal_behavior->start_time: %f\n", local_behavior->start_time);
                printf("\tlocal_behavior->duration: %f\n", local_behavior->duration);
            }
        }
    }

    if (called_object_dump) {
        printf("[CBody : Y-axis ordered]\n");
    }

    RenderEntry render_array[128] = { { -1, 0.0f, 0 } };
    for (ssize local_body_index = 0; local_body_index < world.c_body.size; local_body_index++) {
        int32 local_id = world.c_body.dense[local_body_index];
        if (local_id == -1) {
            continue;
        }
        CBody *local_body = &world.c_body.data[local_body_index];

        int32 local_movement_index = world.c_movement.sparse[local_id];
        if (local_movement_index == -1) {
            // won't render anyways
            continue;
        }
        CMovement *local_movement = &world.c_movement.data[local_movement_index];

        render_array[local_body_index] = (RenderEntry) {
            local_id,
                local_movement->position.y + local_body->scale.y,
                local_body->layer
        };
    }

    SortRender(render_array, 0, world.c_body.size - 1);

    for (ssize i = 0; i < world.c_body.size; i++) {
        // int32 local_id = world.c_body.dense[i];
        int32 local_id = render_array[i].id;
        if (local_id == -1) {
            continue;
        }

        int32 local_body_index = world.c_body.sparse[local_id];
        assert(local_body_index != -1);

        CBody *local_body = &world.c_body.data[local_body_index];

        FanVector2 new_scale  = FanVector2Zero();
        FanVector2 new_offset = FanVector2Zero();
        int32 new_layer = -1;
        if (local_id == Entity_Player_One) {
            new_offset = player_offset;
        }
        else if (local_id == Entity_Background) {
            new_scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() };
        }
        BodyUpdate(local_body, new_scale, new_offset, new_layer, dt);

        int32 local_movement_index = world.c_movement.sparse[local_id];
        if (local_movement_index == -1) {
            // NOTE(liam): skip render if not found
            continue;
        }
        CMovement *local_movement = &world.c_movement.data[local_movement_index];

        int32 local_animation_index = world.c_animation.sparse[local_id];
        int32 local_texture_index = world.c_texture.sparse[local_id];
        CTexture *local_texture = null;
        CAnimation *local_animation = null;
        if (local_texture_index != -1) {
            local_texture = &world.c_texture.data[local_texture_index];
            if (local_animation_index != -1) {
                local_animation = &world.c_animation.data[local_animation_index];

                int32 local_animation_id = 0;
                if (local_id == Entity_Player_One) {
                    local_animation_id = player_animation_id;
                }
                AnimationUpdate(local_animation, local_texture, local_animation_id, 0, dt);
            }
            else {
                TextureUpdate(local_texture, FanVector2Zero(), FanVector2Zero(), dt);
            }
        }

        int32 local_color_index = world.c_color.sparse[local_id];
        FanColor local_color = (FanColor){ 255, 255, 255, 255 };
        if (local_color_index != -1) {
            local_color = world.c_color.data[local_color_index];
        }

        if (local_id == Entity_Player_One) {
            BodyRender(local_body, local_movement, local_color, local_texture, null);
        }
        else {
            BodyRender(local_body, local_movement, local_color, local_texture, RenderFlag_FlipX);
        }


        if (called_object_dump) {
            printf("\tlocal_id: %d\n", local_id);
            printf("\tlocal_body_index: %d\n", local_body_index);
            printf("\tlocal_body->initialized: %s\n", local_body->initialized ? "true" : "false");
            printf("\tlocal_body->layer: %d\n", local_body->layer);
            printf("\t");
            FanVector2Print(local_body->scale);
            printf("\t");
            FanVector2Print(local_body->offset);

            if (local_animation isnt null) {
                printf("\tlocal_animation_index: %d\n", local_animation_index);
                printf("\tlocal_animation: %s\n", local_animation->name);
                printf("\tlocal_animation->id: %d\n", local_animation->id);
                printf("\tlocal_animation->timer: %f\n", local_animation->timer);
                printf("\tlocal_animation->current_frame: %d\n", local_animation->current_frame);
                printf("\tlocal_animation->finished: %d\n", local_animation->finished);
            }
        }
    }
}

int main(void) {
    FanWindowCreate(800, 600, "Fantasia");

    world.arena = (Arena){
        .data     = heap_allocator.make(null, megabytes(1)),
        .size     = 0,
        .capacity = megabytes(1)
    };
    Allocator arena_allocator = {
        .make   = arena_make,
        .free   = arena_free,
        .resize = arena_resize,
        .ctx    = &world.arena
    };

    bool32 running            = true;
    FanVector2 player_offset  = FanVector2Zero();
    FanVector2 player_index   = FanVector2Zero();

    FanRandomSeed(12398);

    ssize component_size  = kilobytes(1);

    ComponentStorageCreate(&world.c_body,      &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_movement,  &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_color,     &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_texture,   &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_behavior,  &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_animation, &arena_allocator, component_size);


    FanTexture tex_link = FanTextureLoad("./resources/link.png");
    FanVector2 sprite_link_size = (FanVector2){ tex_link.width / 10.0f, tex_link.height / 8.0f };
    player_idle_down_frames[0]  = (FanRect){ 0,                         0,                         0, 0 };
    player_idle_down_frames[1]  = (FanRect){ sprite_link_size.x,        0,                         0, 0 };
    player_idle_down_frames[2]  = (FanRect){ 2.0f * sprite_link_size.x, 0,                         0, 0 };

    player_idle_up_frames[0]    = (FanRect){ 0,                         2.0f * sprite_link_size.y, 0, 0 };

    player_idle_left_frames[0]  = (FanRect){ 0,                         sprite_link_size.y,        0, 0 };
    player_idle_left_frames[1]  = (FanRect){ sprite_link_size.x,        sprite_link_size.y,        0, 0 };
    player_idle_left_frames[2]  = (FanRect){ 2.0f * sprite_link_size.x, sprite_link_size.y,        0, 0 };

    player_idle_right_frames[0] = (FanRect){ 0,                         3.0f * sprite_link_size.y, 0, 0 };
    player_idle_right_frames[1] = (FanRect){ sprite_link_size.x,        3.0f * sprite_link_size.y, 0, 0 };
    player_idle_right_frames[2] = (FanRect){ 2.0f * sprite_link_size.x, 3.0f * sprite_link_size.y, 0, 0 };
    ComponentStorageAdd(&world.c_body,          world.entity_count);
    ComponentStorageAdd(&world.c_movement,      world.entity_count);
    ComponentStorageAddArgs(&world.c_texture,   world.entity_count,
        .texture = tex_link,
        .rect = (FanRect){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    ComponentStorageAddArgs(&world.c_animation, world.entity_count);
    world.entity_count++;

    FanTexture tex_mewee = FanTextureLoad("./resources/mewee.png");
    ComponentStorageAdd(&world.c_body,         world.entity_count);
    ComponentStorageAddArgs(&world.c_texture,  world.entity_count,
        .texture = tex_mewee);
    ComponentStorageAddArgs(&world.c_movement, world.entity_count, .speed = 400.0f);
    ComponentStorageAddArgs(&world.c_color,    world.entity_count, 50, 255, 255, 255);
    ComponentStorageAddArgs(&world.c_behavior, world.entity_count,
        .type = BehaviorType_Random,
        .duration = 0.2f
    );
    world.entity_count++;

    ComponentStorageAdd(&world.c_body,         world.entity_count);
    ComponentStorageAddArgs(&world.c_movement, world.entity_count, .speed = 300.0f);
    ComponentStorageAddArgs(&world.c_color,    world.entity_count, 255, 50, 255, 255);
    ComponentStorageAddArgs(&world.c_texture,  world.entity_count,
        .texture = tex_mewee);
    ComponentStorageAddArgs(&world.c_behavior, world.entity_count,
        .type = BehaviorType_Random,
        .duration = 0.5f
    );
    world.entity_count++;

    ComponentStorageAdd(&world.c_body,         world.entity_count);
    ComponentStorageAddArgs(&world.c_movement, world.entity_count,
        .position = (FanVector2){ 200.0f, 300.0f },
        // .movement_flags = MovementFlag_NoCollision
    );
    ComponentStorageAddArgs(&world.c_color,    world.entity_count, 50, 255, 50, 255);
    world.entity_count++;

    ComponentStorageAddArgs(&world.c_body,  world.entity_count,
        .layer = 1,
        .scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() }
    );
    ComponentStorageAddArgs(&world.c_movement,  world.entity_count,
        .movement_flags = MovementFlag_NoCollision);
    ComponentStorageAddArgs(&world.c_color, world.entity_count, 175, 165, 175, 255);
    world.entity_count++;

    ComponentStorageAdd(&world.c_body,         world.entity_count);
    ComponentStorageAddArgs(&world.c_movement, world.entity_count,
        .position = (FanVector2){ 400.0f, 400.0f }
    );
    ComponentStorageAddArgs(&world.c_color,    world.entity_count, 50, 255, 255, 255);
    // ComponentStorageAddArgs(&world.c_behavior, world.entity_count,
    //     .type = BehaviorType_Random,
    //     .duration = 0.5f
    // );
    world.entity_count++;

    UpdateEntitySplit();
    while (running) {
        float dt = FanGetFrameTime();
        if (FanWindowShouldClose() || FanKeyPressed(FanKey_ESCAPE)) {
            running = false;
        }

        FanVector2 player_direction = FanVector2Zero();
        int32 player_animation_id = -1;
        if (FanKeyDown(FanKey_W)) {
            player_direction.y -= 1;
            player_animation_id = 1;
        }
        if (FanKeyDown(FanKey_S)) {
            player_direction.y += 1;
            player_animation_id = 0;
        }
        if (FanKeyDown(FanKey_A)) {
            player_direction.x -= 1;
            player_animation_id = 2;
        }
        if (FanKeyDown(FanKey_D)) {
            player_direction.x += 1;
            player_animation_id = 3;
        }

        if (FanKeyDown(FanKey_K)) {
            player_offset.y += 500.0f * dt;
            if (player_offset.y >= 200.0f) {
                player_offset.y = 200.0f;
            }
        }
        if (FanKeyDown(FanKey_I)) {
            player_offset.y -= 500.0f * dt;
            if (player_offset.y <= 0.0f) {
                player_offset.y = 0.0f;
            }
        }
        if (FanKeyDown(FanKey_L)) {
            player_offset.x += 500.0f * dt;
            if (player_offset.x >= 200.0f) {
                player_offset.x = 200.0f;
            }
        }
        if (FanKeyDown(FanKey_J)) {
            player_offset.x -= 500.0f * dt;
            if (player_offset.x <= 0.0f) {
                player_offset.x = 0.0f;
            }
        }

        if (FanKeyPressed(FanKey_V)) {
            player_index.x -= 1;
        }
        if (FanKeyPressed(FanKey_B)) {
            player_index.x += 1;
        }
        if (FanKeyPressed(FanKey_N)) {
            player_index.y -= 1;
        }
        if (FanKeyPressed(FanKey_M)) {
            player_index.y += 1;
        }

        if (FanKeyPressed(FanKey_P)) {
            called_object_dump = true;
            printf("[[DEBUG INFO]]\n");
        }

        world.current_time = FanGetTime();

        if (called_object_dump) {
            printf("Total Allocations: %.2f / %.2f KB\n", (double)world.arena.size / 1000.0f, (double)world.arena.capacity / 1000.0f);
            printf("current_time: %.3f\n", world.current_time);

            printf("Total Component 'Body' size/capacity:      \t%zu/%zu\n", world.c_body.size, world.c_body.capacity);
            printf("Total Component 'Movement' size/capacity:  \t%zu/%zu\n", world.c_movement.size, world.c_movement.capacity);
            printf("Total Component 'Color' size/capacity:     \t%zu/%zu\n", world.c_color.size, world.c_color.capacity);
            printf("Total Component 'Texture' size/capacity:   \t%zu/%zu\n", world.c_texture.size, world.c_texture.capacity);
            printf("Total Component 'Behavior' size/capacity:  \t%zu/%zu\n", world.c_behavior.size, world.c_behavior.capacity);
            printf("Total Component 'Animation' size/capacity: \t%zu/%zu\n", world.c_animation.size, world.c_animation.capacity);
        }

        FanDrawBegin();
            FanDrawClear(FanColor_WHITE);
            UpdateAndRender(player_direction, player_offset, player_animation_id, dt);
            FanDrawFPS(2, 2);
        FanDrawEnd();
        called_object_dump = false;
    }

    for (ssize i = 0; i < world.c_texture.size; i++) {
        if (world.c_texture.dense[i] == -1) {
            continue;
        }
        FanTextureUnload(world.c_texture.data[i].texture);
    }

    FanWindowClose();
    heap_allocator.free(null, world.arena.data, world.arena.capacity);
    return 0;
}
