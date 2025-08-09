
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

void FanColorPrint_(FanColor c, const char *name) {
    printf("%s: (%d, %d, %d, %d)\n", name, c.r, c.g, c.b, c.a);
}
#define FanColorPrint(c) FanColorPrint_(c, #c)

void FanRectPrint_(FanRect r, const char *name) {
    printf("%s: (%d, %d, %d, %d)\n", name, r.x, r.y, r.width, r.height);
}
#define FanRectPrint(r) FanRectPrint_(r, #r)

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
    MovementFlag_Immovable     = (1 << 0),
    MovementFlag_NoCollision   = (1 << 1),
    MovementFlag_CollideSoftly = (1 << 2),
} MovementFlags;

// typedef struct CMovement {
//     FanVector2 position;
//     FanVector2 last_position;
//     FanVector2 direction;
//
//     float32 speed;
//     float32 friction;
//
//     int32 movement_flags;
//     bool32 initialized;
// } CMovement;
//
// typedef struct CBody {
//     FanVector2 scale;
//     FanVector2 offset;
//
//     int32 layer;
//
//     bool32 initialized;
// } CBody;

typedef struct CTexture {
    FanTexture texture;
    FanRect rect;
} CTexture;

typedef struct CTransform {
    FanVector2 position;
    FanVector2 scale;
    float32 rotation;

    bool32 initialized;
} CTransform;

typedef struct CPhysics {
    FanVector2 direction;
    FanVector2 last_position;

    float32 speed;
    float32 friction;

    int32 flags;
    bool32 initialized;
} CPhysics;

typedef struct CShape {
    FanColor color;
    FanVector2 offset;
    int32 layer;

    bool32 initialized;
} CShape;

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

// ComponentStorageDeclare(CMovement, CMovement);
// ComponentStorageDeclare(CBody, CBody);
ComponentStorageDeclare(CTransform, CTransform);
ComponentStorageDeclare(CShape, CShape);
ComponentStorageDeclare(CPhysics, CPhysics);

// ComponentStorageDeclare(CColor, FanColor);
ComponentStorageDeclare(CTexture, CTexture);

ComponentStorageDeclare(CBehavior, CBehavior);
ComponentStorageDeclare(CAnimation, CAnimation);

/*
 * type: System
 * component(s): CTransform
 */
void TransformUpdate(CTransform *t, FanVector2 position, FanVector2 scale, float dt) {
    if (not t->initialized) {
        init_if_null(t->scale.x, 96.0f);
        init_if_null(t->scale.y, 96.0f);

        t->initialized = true;
    }
    t->position = FanVector2Add(t->position, position);
    t->scale    = FanVector2Add(t->scale, scale);
}

void PhysicsUpdate(CPhysics *p, CTransform *t, FanVector2 direction, float32 dt) {
    if (not p->initialized) {
        init_if_null(p->last_position.x, t->position.x);
        init_if_null(p->last_position.y, t->position.y);

        init_if_null(p->direction.x,     1.0f);
        init_if_null(p->direction.y,     1.0f);

        init_if_null(p->speed,           500.0f);
        init_if_null(p->friction,        1.0f);

        p->initialized = true;
    }

    FanVector2 velocity = FanVector2Sub(t->position, p->last_position);
    FanVector2 acceleration = FanVector2Zero();

    FanVector2 screen_size = {
        FanWindowWidth(),
        FanWindowHeight()
    };
    if (FanVector2Length(t->scale) > 0) {
        screen_size.x = screen_size.x - t->scale.x;
        screen_size.y = screen_size.y - t->scale.y;
    }

    if (t->position.x < 0.0f) {
        t->position.x      = 0.0f;
        p->last_position.x = t->position.x + velocity.x;
    }
    else if (t->position.x > screen_size.x) {
        t->position.x      = screen_size.x;
        p->last_position.x = t->position.x + velocity.x;
    }
    if (t->position.y < 0.0f) {
        t->position.y      = 0.0f;
        p->last_position.y = t->position.y + velocity.y;
    }
    else if (t->position.y > screen_size.y) {
        t->position.y      = screen_size.y;
        p->last_position.y = t->position.y + velocity.y;
    }

    p->direction.x = coalesce(direction.x, p->direction.x);
    p->direction.y = coalesce(direction.y, p->direction.y);
    direction = FanVector2Normalize(direction);

    if (FanVector2Length(direction) > 0) {
        acceleration = FanVector2Add(acceleration, FanVector2Scale(direction, p->speed));
    }
    else if (FanVector2Length(velocity) > 0) {
        acceleration = FanVector2Sub(acceleration, FanVector2Scale(velocity, p->friction));
    }

    p->last_position = t->position;
    // NOTE: c->position += (velocity + acceleration * dt) * dt;
    t->position = FanVector2Add(t->position, FanVector2Scale(FanVector2Add(velocity, acceleration), dt));
}


bool32 CollisionCheck(FanVector2 aPos, FanVector2 aSize, FanVector2 bPos, FanVector2 bSize) {
    bool32 result = false;

    // AABB
    result = not (aPos.x + aSize.x < bPos.x or bPos.x + bSize.x < aPos.x or
                  aPos.y + aSize.y < bPos.y or bPos.y + bSize.y < aPos.y);

    return result;
}

void CollisionResolve(CTransform *a, CPhysics *a_p, CTransform *b, CPhysics *b_p, float32 dt) {
    if (CollisionCheck(a->position, a->scale, b->position, b->scale)) {
        FanVector2 aMax = (FanVector2){
            a->position.x + a->scale.x,
                a->position.y + a->scale.y
        };
        FanVector2 bMax = (FanVector2){
            a->position.x + a->scale.x,
                a->position.y + a->scale.y
        };

        FanVector2 overlap = (FanVector2){
            min(aMax.x, bMax.x) - max(a->position.x, b->position.x),
                min(aMax.y, bMax.y) - max(a->position.y, b->position.y)
        };

        if (overlap.x <= 0.0f || overlap.y <= 0.0f)
            return;

        float32 correction;
        int32 aMovable = (a_p->flags & MovementFlag_Immovable) ? 0 : 1;
        int32 bMovable = (b_p->flags & MovementFlag_Immovable) ? 0 : 1;
        if (a_p->flags & MovementFlag_CollideSoftly) {
            aMovable *= dt;
        }
        if (b_p->flags & MovementFlag_CollideSoftly) {
            bMovable *= dt;
        }
        if (overlap.x < overlap.y) {
            correction = overlap.x;
            if (aMovable and bMovable) {
                correction *= 0.5f;
            }
            if (a->position.x < b->position.x) {
                a->position.x -= correction * aMovable;
                b->position.x += correction * bMovable;
            } else {
                a->position.x += correction * aMovable;
                b->position.x -= correction * bMovable;
            }
        } else {
            correction = overlap.y;
            if (aMovable and bMovable) {
                correction *= 0.5f;
            }
            if (a->position.y < b->position.y) {
                a->position.y -= correction * aMovable;
                b->position.y += correction * bMovable;
            } else {
                a->position.y += correction * aMovable;
                b->position.y -= correction * bMovable;
            }
        }
    }
    // NOTE(liam): potentially handle 'tunneling' if needed
    // likely solution: https://blog.hamaluik.ca/posts/swept-aabb-collision-using-minkowski-difference/
}

/*
 * type: System
 * component(s): Shape
 */
void ShapeUpdate(CShape *s, FanColor color, FanVector2 offset, int32 layer, float dt) {
    (void)dt;
    if (not s->initialized) {
        if (s->color.a == 0) {
            s->color = FanColor_WHITE;
        }
        init_if_null(s->layer,   2);

        s->initialized = true;
    }

    if (color.a > 0) {
        s->color = color;
    }

    s->offset.x = coalesce(offset.x, s->offset.x);
    s->offset.y = coalesce(offset.y, s->offset.y);

    if (layer != -1) {
        s->layer = layer;
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
        (FanVector2){ current_data.x,     current_data.y },
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
 * component(s): Transform, Shape, Texture (opt), Physics (opt)
 */
void ShapeRender(CShape *s, CTransform *t, CTexture *tx, CPhysics *p, int32 flags) {
    if (tx is null) {
        if (FanVector2Length(s->offset) > 0.0f) {
            FanDrawRectV(FanVector2Add(t->position, s->offset), t->scale, (FanColor){ 50, 50, 50, 255 });
        }
        FanDrawRectV(t->position, t->scale, s->color);
    }
    else {
        float width  = (tx->rect.width)  ? tx->rect.width  : tx->texture.width;
        float height = (tx->rect.height) ? tx->rect.height : tx->texture.height;

        if (p isnt null) {
            if (flags & RenderFlag_FlipX) {
                width  *= p->direction.x;
            }
            if (flags & RenderFlag_FlipY) {
                height *= p->direction.y;
            }
        }

        FanRect src = (FanRect) {
            tx->rect.x,
            tx->rect.y,
            width,
            height
        };
        FanRect dst = (FanRect) {
            t->position.x,
            t->position.y,
            t->scale.x,
            t->scale.y
        };

        FanRect dst_shadow = (FanRect) {
            t->position.x + s->offset.x,
            t->position.y + s->offset.y,
            t->scale.x,
            t->scale.y
        };

        if (FanVector2Length(s->offset) > 0.0f) {
            FanDrawTexture(
                tx->texture,
                src,
                dst_shadow,
                (FanVector2) { 0.0f, 0.0f },
                0.0f,
                s->color
            );
        }

        FanDrawTexture(
            tx->texture,
            src,
            dst,
            (FanVector2) { 0.0f, 0.0f },
            0.0f,
            s->color
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

typedef enum {
    SystemMode_Overworld = 0,
    SystemMode_Menu,
    SystemMode_Battle
} SystemMode;

typedef struct World {
    SystemMode        current_mode;

    uint8             entity_count;

    Arena             arena;

    // CBodyStorage      c_body;
    // CMovementStorage  c_movement;
    // CColorStorage     c_color;
    CTransformStorage c_transform;
    CShapeStorage     c_shape;
    CPhysicsStorage   c_physics;
    CTextureStorage   c_texture;
    CBehaviorStorage  c_behavior;
    CAnimationStorage c_animation;

    float64           current_time;
} World;
World world = {};

bool32 called_object_dump = false;

int32 dynamic_entities[128] = { -1 };
int32 static_entities[128]  = { -1 };
ssize dynamic_entity_count  = 0;
ssize static_entity_count   = 0;

// NOTE(liam): must call whenever entities are added/removed
global void UpdateEntitySplit(void) {
    dynamic_entity_count = 0;
    static_entity_count  = 0;
    for (ssize i = 0; i < world.c_transform.size; i++) {
        int32 id = world.c_transform.dense[i];
        if (id == -1) continue;

        int32 physics_index = world.c_physics.sparse[id];
        bool32 is_static = true;

        if (physics_index != -1) {
            CPhysics *physics = &world.c_physics.data[i];
            if (physics->flags & MovementFlag_NoCollision) continue;

            if (not (physics->flags & MovementFlag_Immovable)) {
                is_static = false;
            }
        }

        if (is_static)
            static_entities[static_entity_count++]   = id;
        else
            dynamic_entities[dynamic_entity_count++] = id;

    }
}

void UpdateAndRender(FanVector2 player_direction, FanVector2 player_offset, int32 player_animation_id, float32 dt) {
    if (called_object_dump) {
        printf("[CTransform]\n");
    }

    for (ssize i = 0; i < dynamic_entity_count; i++) {
        int32 self_id = dynamic_entities[i];


        int32 self_transform_idx = world.c_transform.sparse[self_id];
        int32 self_physics_idx = world.c_physics.sparse[self_id];
        int32 self_behavior_idx = world.c_behavior.sparse[self_id];

        CTransform *self_transform = &world.c_transform.data[self_transform_idx];
        CPhysics *self_physics = null;
        CBehavior *self_behavior = null;

        FanVector2 self_position = FanVector2Zero();
        FanVector2 self_scale = FanVector2Zero();
        TransformUpdate(self_transform, self_position, self_scale, dt);

        if (self_physics_idx != -1) {
            self_physics = &world.c_physics.data[self_physics_idx];
            // COLLISION CHECK HERE
            for (ssize j = i + 1; j < dynamic_entity_count; j++) {
                int32 other_id = dynamic_entities[j];
                int32 other_transform_idx = world.c_transform.sparse[other_id];
                CTransform *other_transform = &world.c_transform.data[other_transform_idx];

                int32 other_physics_idx = world.c_physics.sparse[other_id];
                assert(other_physics_idx != -1);
                CPhysics *other_physics = &world.c_physics.data[other_physics_idx];

                CollisionResolve(self_transform, self_physics, other_transform, other_physics, dt);
            }
            for (ssize s = 0; s < static_entity_count; s++) {
                int32 other_id = static_entities[s];
                int32 other_transform_idx = world.c_transform.sparse[other_id];
                CTransform *other_transform = &world.c_transform.data[other_transform_idx];

                int32 other_physics_idx = world.c_physics.sparse[other_id];
                assert(other_physics_idx != -1);
                CPhysics *other_physics = &world.c_physics.data[other_physics_idx];

                CollisionResolve(self_transform, self_physics, other_transform, other_physics, dt);
            }

            FanVector2 self_direction = FanVector2Zero();
            if (self_id == Entity_Player_One) {
                self_direction = player_direction;
            }
            else {
                if (self_behavior_idx != -1) {
                    self_behavior = &world.c_behavior.data[self_behavior_idx];

                    switch (self_behavior->type) {
                        case BehaviorType_Random: {
                            if (world.current_time - self_behavior->start_time > self_behavior->duration) {
                                self_direction = (FanVector2) {
                                    FanRandomInt(-1, 1),
                                    FanRandomInt(-1, 1)
                                };
                                self_behavior->start_time = world.current_time;
                            }
                            else {
                                // keeps entity moving rather than staying still
                                self_direction = self_physics->direction;
                            }
                        } break;
                        case BehaviorType_None:
                        default: break;
                    }
                }
            }

            PhysicsUpdate(self_physics, self_transform, self_direction, dt);
        }

        if (called_object_dump) {
            printf("\tid: %d\n", self_id);

            // printf("\tself_transform: %d\n", self_transform_idx);
            // printf("\t");
            FanVector2Print(self_transform->position);
            // printf("\t");
            // FanVector2Print(self_transform->scale);
            // printf("\tself_transform->rotation: %f\n", self_transform->rotation);

            if (self_physics isnt null) {
                printf("\tself_physics: %d\n", self_physics_idx);
            //     printf("\tself_physics->initialized: %s\n", self_physics->initialized ? "true" : "false");
            //     printf("\tself_physics->speed: %f\n", self_physics->speed);
            //     printf("\tself_physics->friction: %f\n", self_physics->friction);
            //     printf("\t");
            //     FanVector2Print(self_physics->last_position);
            //     printf("\t");
            //     FanVector2Print(self_physics->direction);
            }
            //
            // if (self_behavior isnt null) {
            //     printf("\tself_behavior: %d\n", self_behavior_idx);
            //     printf("\tself_behavior->type: %d\n", self_behavior->type);
            //     printf("\tself_behavior->start_time: %f\n", self_behavior->start_time);
            //     printf("\tself_behavior->duration: %f\n", self_behavior->duration);
            // }
        }
    }

    if (called_object_dump) {
        printf("[CBody : Y-axis ordered]\n");
    }

    RenderEntry render_array[128] = { { -1, 0.0f, 0 } };
    ssize render_entry_count = 0;
    for (ssize i = 0; i < world.c_shape.size; i++) {
        int32 id = world.c_shape.dense[i];
        if (id == -1) {
            continue;
        }
        CShape *shape = &world.c_shape.data[i];

        int32 transform_idx = world.c_transform.sparse[id];
        if (transform_idx == -1)
            continue;
        CTransform *transform = &world.c_transform.data[transform_idx];

        render_array[render_entry_count++] = (RenderEntry) {
            id,
            transform->position.y + transform->scale.y,
            shape->layer
        };
    }

    SortRender(render_array, 0, world.c_shape.size - 1);

    for (ssize i = 0; i < render_entry_count; i++) {
        // int32 local_id = world.c_body.dense[i];
        int32 id = render_array[i].id;
        if (id == -1) {
            continue;
        }

        int32 shape_idx     = world.c_shape.sparse[id];
        int32 transform_idx = world.c_transform.sparse[id];
        int32 physics_idx   = world.c_physics.sparse[id];
        int32 animation_idx = world.c_animation.sparse[id];
        int32 texture_idx   = world.c_texture.sparse[id];

        CShape     *shape     = &world.c_shape.data[shape_idx];
        CTransform *transform = &world.c_transform.data[transform_idx];
        CPhysics   *physics   = null;
        CTexture   *texture   = null;
        CAnimation *animation = null;

        FanColor   color  = (FanColor){ 0, 0, 0, 0 };
        FanVector2 offset = FanVector2Zero();
        int32 layer = -1;
        if (id == Entity_Player_One) {
            offset = player_offset;
        }
        else if (id == Entity_Background) {
            // scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() };
        }
        ShapeUpdate(shape, color, offset, layer, dt);

        if (texture_idx != -1) {
            texture = &world.c_texture.data[texture_idx];
            if (animation_idx != -1) {
                animation = &world.c_animation.data[animation_idx];

                int32 animation_id = 0;
                if (id == Entity_Player_One) {
                    animation_id = player_animation_id;
                }
                AnimationUpdate(animation, texture, animation_id, 0, dt);
            }
            else {
                TextureUpdate(texture, FanVector2Zero(), FanVector2Zero(), dt);
            }
        }

        if (physics_idx != -1) {
            physics = &world.c_physics.data[physics_idx];
        }

        if (id == Entity_Player_One) {
            ShapeRender(shape, transform, texture, physics, null);
        }
        else {
            ShapeRender(shape, transform, texture, physics, RenderFlag_FlipX);
        }


        if (called_object_dump) {
            printf("\tid: %d\n", id);

            printf("\tshape: %d\n", shape_idx);
            printf("\tshape->initialized: %s\n", shape->initialized ? "true" : "false");
            printf("\tshape->layer: %d\n", shape->layer);
            printf("\t");
            FanColorPrint(shape->color);
            printf("\t");
            FanVector2Print(shape->offset);

            if (animation isnt null) {
                printf("\tanimation: %s (%d)\n", animation->name, animation_idx);
                printf("\tanimation->id: %d\n", animation->id);
                printf("\tanimation->timer: %f\n", animation->timer);
                printf("\tanimation->current_frame: %d\n", animation->current_frame);
                printf("\tanimation->finished: %d\n", animation->finished);
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

    ComponentStorageCreate(&world.c_transform, &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_shape,     &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_physics,   &arena_allocator, component_size);
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
    ComponentStorageAdd(&world.c_transform,     world.entity_count);
    ComponentStorageAdd(&world.c_shape,         world.entity_count);
    ComponentStorageAddArgs(&world.c_physics,       world.entity_count,
        .flags = MovementFlag_CollideSoftly
    );
    ComponentStorageAddArgs(&world.c_texture,   world.entity_count,
        .texture = tex_link,
        .rect = (FanRect){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    ComponentStorageAddArgs(&world.c_animation, world.entity_count);
    world.entity_count++;

    FanTexture tex_mewee = FanTextureLoad("./resources/mewee.png");
    ComponentStorageAdd(&world.c_transform,    world.entity_count);
    ComponentStorageAddArgs(&world.c_shape,    world.entity_count,
        .color = (FanColor){ 50, 255, 255, 255 }
    );
    ComponentStorageAddArgs(&world.c_physics,  world.entity_count, .speed = 400.0f);
    ComponentStorageAddArgs(&world.c_texture,  world.entity_count,
        .texture = tex_mewee);
    ComponentStorageAddArgs(&world.c_behavior, world.entity_count,
        .type = BehaviorType_Random,
        .duration = 0.2f
    );
    world.entity_count++;

    ComponentStorageAdd(&world.c_transform,    world.entity_count);
    ComponentStorageAddArgs(&world.c_shape,    world.entity_count,
        .color = (FanColor){ 255, 50, 255, 255 }
    );
    ComponentStorageAddArgs(&world.c_physics,  world.entity_count, .speed = 300.0f);
    ComponentStorageAddArgs(&world.c_texture,  world.entity_count,
        .texture = tex_mewee);
    ComponentStorageAddArgs(&world.c_behavior, world.entity_count,
        .type = BehaviorType_Random,
        .duration = 0.5f
    );
    world.entity_count++;

    ComponentStorageAddArgs(&world.c_transform, world.entity_count,
        .scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() },
    );
    ComponentStorageAddArgs(&world.c_shape,     world.entity_count,
        .layer = 1,
        .color = (FanColor){ 155, 155, 155, 255 },
    );
    ComponentStorageAddArgs(&world.c_physics,   world.entity_count,
        .flags = MovementFlag_NoCollision);
    world.entity_count++;

    ComponentStorageAddArgs(&world.c_transform, world.entity_count,
        .position = (FanVector2){ 200.0f, 300.0f },
    );
    ComponentStorageAddArgs(&world.c_shape,     world.entity_count,
        .color = (FanColor){ 200, 165, 175, 255 },
    );
    ComponentStorageAdd(&world.c_physics,  world.entity_count);
    world.entity_count++;

    ComponentStorageAdd(&world.c_transform, world.entity_count);
    ComponentStorageAddArgs(&world.c_shape, world.entity_count,
        .color = (FanColor){ 50, 255, 255, 255 },
    );
    ComponentStorageAddArgs(&world.c_physics,  world.entity_count);
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

            printf("Total Component 'Transform' size/capacity:   \t%zu/%zu\n", world.c_transform.size, world.c_transform.capacity);
            printf("Total Component 'Shape' size/capacity:  \t%zu/%zu\n",      world.c_shape.size,     world.c_shape.capacity);
            printf("Total Component 'Physics' size/capacity: \t%zu/%zu\n",     world.c_physics.size,   world.c_physics.capacity);
            printf("Total Component 'Texture' size/capacity:   \t%zu/%zu\n",   world.c_texture.size,   world.c_texture.capacity);
            printf("Total Component 'Behavior' size/capacity:  \t%zu/%zu\n",   world.c_behavior.size,  world.c_behavior.capacity);
            printf("Total Component 'Animation' size/capacity: \t%zu/%zu\n",   world.c_animation.size, world.c_animation.capacity);
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
