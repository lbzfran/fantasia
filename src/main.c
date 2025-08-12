
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

#define clamp(x, a, b)   min(max(x, a), b)

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

typedef struct CTransform {
    FanVector2 position;
    FanVector2 scale;
    float32    rotation;

    bool32     initialized;
} CTransform;

typedef struct CShape {
    FanColor   color;
    FanVector2 offset;
    int32      layer;

    bool32     visible;
    bool32     initialized;
} CShape;

typedef struct CMovement {
    FanVector2 velocity;

    FanVector2 direction;

    float32 speed;
    float32 max_speed;

    int32 flags;
    bool32 active;
    bool32 initialized;
} CMovement;

typedef struct CPhysics {
    FanVector2 direction;
    FanVector2 last_position;

    float32    speed;
    float32    friction;
    float32    mass;

    int32      flags;
    bool32     active;
    bool32     initialized;
} CPhysics;

typedef struct CTexture {
    FanTexture texture;
    FanRect    rect;
} CTexture;

typedef struct CBehavior {
    enum BehaviorType {
        BehaviorType_None   = 0,
        BehaviorType_Random = 1,
        BehaviorType_Follow = 2,
    } type;
    float64 start_time;
    float64 duration;
} CBehavior;

// state manager component
typedef struct CAnimation {
    const char8 *name;
    int32        id;
    float32      timer;
    int32        current_frame;
    bool32       finished;
    int32        animation_flags;
} CAnimation;

typedef struct {
    const char8 *name;
    FanRect     *frames;
    float32      frame_time;
    int32        frame_count;
    bool32       loop;

    int32        next_id;
} AnimationData;

typedef struct {
    FanRect zone;

    bool32 active;
    bool32 initialized;
} CInteract;

typedef struct {
    int32 player;
    int32 camera;
} SpecialEntityID;

#define istagged(t) (t >= 0 ? true : false)

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

ComponentStorageDeclare(CTransform, CTransform);
ComponentStorageDeclare(CShape,     CShape);
ComponentStorageDeclare(CMovement,  CMovement);
ComponentStorageDeclare(CTexture,   CTexture);

ComponentStorageDeclare(CBehavior,  CBehavior);
ComponentStorageDeclare(CAnimation, CAnimation);
ComponentStorageDeclare(CPhysics,   CPhysics);

ComponentStorageDeclare(CInteract,  CInteract);

// empty, query-only tags
ComponentStorageDeclare(CEnemyTag,      uint8);
ComponentStorageDeclare(CBackgroundTag, uint8);

typedef enum {
    TransformUpdate_SetPosition,
    TransformUpdate_SetScale,
    TransformUpdate_SetRotation,
} TransformUpdateFlags;
/*
 * type: System
 * component(s): CTransform
 */
void TransformUpdate(CTransform *t, FanVector2 position, FanVector2 scale, float32 angle, int32 flags, float dt) {
    if (not t->initialized) {
        init_if_null(t->scale.x, 96.0f);
        init_if_null(t->scale.y, 96.0f);

        t->initialized = true;
    }
    if (flags & TransformUpdate_SetPosition)
        t->position = position;
    else
        t->position = FanVector2Add(t->position, position);

    if (flags & TransformUpdate_SetScale)
        t->scale    = scale;
    else
        t->scale    = FanVector2Add(t->scale, scale);

    if (flags & TransformUpdate_SetRotation)
        t->rotation = angle;
    else
        t->rotation = t->rotation + angle;
}

void MovementUpdate(CMovement *m, CTransform *t, FanVector2 direction, float32 dt) {
    if (not m->initialized) {
        init_if_null(m->speed,       400.0f);
        init_if_null(m->max_speed,   500.0f);

        init_if_null(m->direction.x, 1.0f);
        init_if_null(m->direction.y, 1.0f);

        m->active      = true;
        m->initialized = true;
    }

    FanVector2 velocity = FanVector2Zero();
    if (FanVector2Length(direction) > 0.0f) {
        m->direction = direction;
        direction    = FanVector2Normalize(direction);
        velocity     = FanVector2Scale(direction, m->speed);
    }
    m->velocity = velocity;

    t->position = FanVector2Add(t->position, FanVector2Scale(m->velocity, dt));

    float32 softness = 0.005f;
    if (not (m->flags & MovementFlag_NoCollision)) {
        FanRect bounding_zone = {
            .x = 0.0f,
            .y = 0.0f,
            .width = FanWindowWidth(),
            .height = FanWindowHeight()
        };
        if (FanVector2Length(t->scale) > 0) {
            bounding_zone.width  -= t->scale.x;
            bounding_zone.height -= t->scale.y;
        }
        // t->position.x = clamp(t->position.x, bounding_zone.x, bounding_zone.width);
        // t->position.y = clamp(t->position.y, bounding_zone.y, bounding_zone.height);

        float32 overlap;
        if (t->position.x < bounding_zone.x) {
            overlap = bounding_zone.x - t->position.x;
            t->position.x += overlap * softness;
        }
        else if (t->position.x > bounding_zone.width) {
            overlap = t->position.x - bounding_zone.width;
            t->position.x -= overlap * softness;
        }

        if (t->position.y < bounding_zone.y) {
            overlap = bounding_zone.y - t->position.y;
            t->position.y += overlap * softness;
        }
        else if (t->position.y > bounding_zone.height) {
            overlap = t->position.y - bounding_zone.height;
            t->position.y -= overlap * softness;
        }
    }
}

void PhysicsUpdate(CPhysics *p, CTransform *t, FanVector2 force, float32 dt) {
    if (not p->initialized) {
        init_if_null(p->last_position.x, t->position.x);
        init_if_null(p->last_position.y, t->position.y);

        init_if_null(p->direction.x,     1.0f);
        init_if_null(p->direction.y,     1.0f);

        init_if_null(p->speed,           500.0f);
        init_if_null(p->friction,        0.2f);
        init_if_null(p->mass,            1.0f);

        p->active = true;
        p->initialized = true;
    }

    FanVector2 velocity = FanVector2Sub(t->position, p->last_position);
    FanVector2 acceleration = FanVector2Zero();

    FanVector2 screen_size = {
        FanWindowWidth(),
        FanWindowHeight()
    };
    if (FanVector2Length(t->scale) > 0) {
        screen_size.x -= t->scale.x;
        screen_size.y -= t->scale.y;
    }

    velocity = FanVector2Scale(velocity, 1.0f - p->friction * dt);

    float32 safe_mass = max(p->mass, 0.0001f);
    acceleration = FanVector2Scale(force, p->speed / safe_mass);

    FanVector2 new_position = FanVector2Add(
        t->position,
        FanVector2Add(velocity, FanVector2Scale(acceleration, dt * dt))
    );

    new_position.x = clamp(new_position.x, 0.0f, screen_size.x);
    new_position.y = clamp(new_position.y, 0.0f, screen_size.y);

    p->last_position = t->position;
    t->position = new_position;
}


bool32 CollisionCheck(FanVector2 aPos, FanVector2 aSize, FanVector2 bPos, FanVector2 bSize) {
    bool32 result = false;

    // AABB
    result = not (aPos.x + aSize.x < bPos.x or bPos.x + bSize.x < aPos.x or
                  aPos.y + aSize.y < bPos.y or bPos.y + bSize.y < aPos.y);

    return result;
}

void CollisionResolve(CTransform *a, CMovement *a_m, CTransform *b, CMovement *b_m, float32 dt) {
    // NOTE(liam): this check is prob unnecessary
    if (a_m->flags & MovementFlag_NoCollision or b_m->flags & MovementFlag_NoCollision)
        return;

    if (CollisionCheck(a->position, a->scale, b->position, b->scale)) {
        FanVector2 aMax = (FanVector2){
            a->position.x + a->scale.x,
            a->position.y + a->scale.y
        };
        FanVector2 bMax = (FanVector2){
            b->position.x + b->scale.x,
            b->position.y + b->scale.y
        };

        FanVector2 overlap = (FanVector2){
            min(aMax.x, bMax.x) - max(a->position.x, b->position.x),
            min(aMax.y, bMax.y) - max(a->position.y, b->position.y)
        };

        const float32 tolerance = 0.0f;
        if (overlap.x <= tolerance || overlap.y <= tolerance)
            return;

        float32 correction;
        float32 aMove = (a_m->flags & MovementFlag_Immovable) ? 0.0f : 1.0f;
        float32 bMove = (b_m->flags & MovementFlag_Immovable) ? 0.0f : 1.0f;
        if (a_m->flags & MovementFlag_CollideSoftly) {
            aMove *= dt;
        }
        if (b_m->flags & MovementFlag_CollideSoftly) {
            bMove *= dt;
        }

        float32 totalMove = aMove + bMove;
        float32 aFactor = (totalMove > 0.0f) ? (aMove / totalMove) : 0.0f;
        float32 bFactor = (totalMove > 0.0f) ? (bMove / totalMove) : 0.0f;

        const float32 softness = 0.005f;
        if (overlap.x < overlap.y) {
            correction = overlap.x * softness;
            if (a->position.x < b->position.x) {
                a->position.x -= correction * aFactor;
                b->position.x += correction * bFactor;
            } else {
                a->position.x += correction * aFactor;
                b->position.x -= correction * bFactor;
            }
        } else {
            correction = overlap.y * softness;
            if (a->position.y < b->position.y) {
                a->position.y -= correction * aFactor;
                b->position.y += correction * bFactor;
            } else {
                a->position.y += correction * aFactor;
                b->position.y -= correction * bFactor;
            }
        }
    }
    // NOTE(liam): potentially handle 'tunneling' if needed
    // likely solution: https://blog.hamaluik.ca/posts/swept-aabb-collision-using-minkowski-difference/
}

typedef enum {
    ShapeUpdate_SetColor,
    ShapeUpdate_SetOffset,
    ShapeUpdate_SetLayer,
} ShapeUpdateFlags;
/*
 * type: System
 * component(s): Shape
 */
void ShapeUpdate(CShape *s, FanColor color, FanVector2 offset, int32 layer, int32 flags, float dt) {
    (void)dt;
    if (not s->initialized) {
        if (s->color.a == 0) {
            s->color = FanColor_WHITE;
        }
        init_if_null(s->layer, 2);

        s->visible = true;
        s->initialized = true;
    }

    if (flags & ShapeUpdate_SetColor)
        s->color = color;

    if (flags & ShapeUpdate_SetOffset)
        s->offset = offset;

    if (flags & ShapeUpdate_SetLayer)
        s->layer = layer;
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
    InteractFlag_SetPosition = 1,
    InteractFlag_SetScale,
} InteractUpdateFlags;

void InteractUpdate(CInteract *r, CTransform *t, CMovement *m, FanVector2 position, FanVector2 scale, int32 flags) {
    if (not r->initialized) {

        r->active = true;
        r->initialized = true;
    }

    if (flags & InteractFlag_SetPosition) {
        r->zone.x = position.x;
        r->zone.y = position.y;
    }

    if (flags & InteractFlag_SetScale) {
        r->zone.width  = scale.x;
        r->zone.height = scale.y;
    }
}

typedef enum {
    RenderFlag_FlipX        = (1 << 0),
    RenderFlag_FlipY        = (1 << 1),
    RenderFlag_ShowInteract = (1 << 2)
} RenderFlags;
/*
 * type: System
 * component(s): Transform, Shape, Texture (opt), Physics (opt)
 */
void ShapeRender(CShape *s, CTransform *t, CTexture *tx, CMovement *m, CInteract *r, int32 flags) {
    if (tx is null) {
        if (FanVector2Length(s->offset) > 0.0f) {
            FanDrawRectV(FanVector2Add(t->position, s->offset), t->scale, (FanColor){ 50, 50, 50, 255 });
        }
        FanDrawRectV(t->position, t->scale, s->color);
    }
    else {
        float width  = (tx->rect.width)  ? tx->rect.width  : tx->texture.width;
        float height = (tx->rect.height) ? tx->rect.height : tx->texture.height;

        if (m) {
            if (flags & RenderFlag_FlipX) {
                width  *= m->direction.x ? m->direction.x : 1.0f;
            }
            if (flags & RenderFlag_FlipY) {
                height *= m->direction.y ? m->direction.y : 1.0f;
            }
        }

        FanRect src = (FanRect) {
            tx->rect.x,
            tx->rect.y,
            width,
            height
        };

        FanRect dst = (FanRect) {
            t->position.x + s->offset.x,
            t->position.y + s->offset.y,
            t->scale.x,
            t->scale.y
        };

        if (flags & RenderFlag_ShowInteract and r isnt null) {
            FanRect dst_interact = r->zone;
            // FanDrawTexture(
            //     tx->texture,
            //     src,
            //     dst_interact,
            //     (FanVector2) { 0.0f, 0.0f },
            //     0.0f,
            //     s->color
            // );
            FanDrawRectR(dst_interact, (FanColor){ 255, 0, 125, 75 });
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
    SystemMode_Overworld = 0,
    SystemMode_Menu,
    SystemMode_Battle
} SystemMode;

typedef struct World {
    SystemMode             current_mode;

    uint8                  entity_count;
    SpecialEntityID        spec_id;

    Arena                  arena;

    CTransformStorage      c_transform;
    CShapeStorage          c_shape;
    CMovementStorage       c_movement;
    CTextureStorage        c_texture;

    CBehaviorStorage       c_behavior;
    CAnimationStorage      c_animation;
    CPhysicsStorage        c_physics;

    CInteractStorage        c_interact;

    CEnemyTagStorage       c_tag_enemy;
    CBackgroundTagStorage  c_tag_background;

    float64                current_time;
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

        int32 move_index = world.c_movement.sparse[id];
        int32 bg_tag = world.c_tag_background.sparse[id];

        bool32 is_static = true;
        if (bg_tag < 0 or move_index != -1) {
            CMovement *movement = &world.c_movement.data[move_index];
            if (movement->flags & MovementFlag_NoCollision) continue;

            if (not (movement->flags & MovementFlag_Immovable)) {
                is_static = false;
            }
        }

        if (is_static)
            static_entities[static_entity_count++]   = id;
        else
            dynamic_entities[dynamic_entity_count++] = id;
    }
}

void UpdateAndRender(
        FanVector2 player_direction,
        FanVector2 player_offset,
        int32 player_animation_id,
        bool32 update_entity_split,
        float32 dt
    ) {
    if (called_object_dump)
        printf("[CTransform]\n");

    if (update_entity_split)
        UpdateEntitySplit();

    for (ssize i = 0; i < world.c_transform.size; i++) {
        // int32 self_id = dynamic_entities[i];
        int32 self_id = world.c_transform.dense[i];

        int32 self_transform_idx = world.c_transform.sparse[self_id];
        // int32 self_physics_idx   = world.c_physics.sparse[self_id];
        int32 self_move_idx      = world.c_movement.sparse[self_id];
        int32 self_behavior_idx  = world.c_behavior.sparse[self_id];

        CTransform *self_transform = &world.c_transform.data[self_transform_idx];
        // CPhysics   *self_physics   = null;
        CMovement  *self_move      = null;
        CBehavior  *self_behavior  = null;

        FanVector2 self_position = FanVector2Zero();
        FanVector2 self_scale    = FanVector2Zero();
        float32 rotation = 0.0f;
        int32 transform_flags = 0;
        TransformUpdate(self_transform, self_position, self_scale, rotation, transform_flags, dt);
        // if (self_id == world.spec_id.camera) {
        //     self_transform->scale = (FanVector2){ FanWindowWidth() / 2.0f, FanWindowHeight() / 2.0f };
        // }

        if (self_move_idx != -1) {
            self_move = &world.c_movement.data[self_move_idx];

            if (self_move->active and not (self_move->flags & MovementFlag_NoCollision)) {
                for (ssize j = i + 1; j < dynamic_entity_count; j++) {
                    int32 other_id = dynamic_entities[j];

                    int32 other_transform_idx = world.c_transform.sparse[other_id];
                    int32 other_move_idx      = world.c_movement.sparse[other_id];
                    assert(other_move_idx != -1);

                    CTransform *other_transform = &world.c_transform.data[other_transform_idx];
                    CMovement  *other_move      = &world.c_movement.data[other_move_idx];

                    CollisionResolve(self_transform, self_move, other_transform, other_move, dt);
                }
                for (ssize s = 0; s < static_entity_count; s++) {
                    int32 other_id = static_entities[s];

                    int32 other_transform_idx = world.c_transform.sparse[other_id];
                    int32 other_move_idx      = world.c_movement.sparse[other_id];
                    assert(other_move_idx != -1);

                    CTransform *other_transform = &world.c_transform.data[other_transform_idx];
                    CMovement  *other_move      = &world.c_movement.data[other_move_idx];

                    CollisionResolve(self_transform, self_move, other_transform, other_move, dt);
                }
            }

            FanVector2 self_direction = FanVector2Zero();
            if (self_id == world.spec_id.player) {
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
                                self_direction = self_move->direction;
                            }
                        } break;
                        case BehaviorType_Follow: {
                            CTransform *target_transform = &world.c_transform.data[0];
                            FanVector2 target_face = target_transform->position;
                            if (self_id == world.spec_id.camera) {
                                CShape *target_shape = &world.c_shape.data[0];
                                target_face = FanVector2Add(target_face, target_shape->offset);
                            }
                            FanVector2 face = FanVector2Normalize(FanVector2Sub(target_face, self_transform->position));
                            self_direction = (FanVector2){ signof(face.x), signof(face.y) };

                        } break;
                        case BehaviorType_None:
                        default: break;
                    }
                }
            }

            MovementUpdate(self_move, self_transform, self_direction, dt);
        }

        if (called_object_dump) {
            printf("\tid: %d\n", self_id);

            if (self_id == world.spec_id.player) {
                // printf("\tself_transform: %d\n", self_transform_idx);
                // printf("\t");
                FanVector2Print(self_transform->position);
                // printf("\t");
                // FanVector2Print(self_transform->scale);
                // printf("\tself_transform->rotation: %f\n", self_transform->rotation);

                if (self_move) {
                    printf("\t");
                    FanVector2Print(self_move->velocity);
                    FanVector2Print(self_move->direction);
                    printf("\tself_move->speed: %f\n", self_move->speed);
                }

                // if (self_physics) {
                // printf("\tself_physics: %d\n", self_physics_idx);
                //     printf("\tself_physics->initialized: %s\n", self_physics->initialized ? "true" : "false");
                //     printf("\tself_physics->speed: %f\n", self_physics->speed);
                //     printf("\tself_physics->friction: %f\n", self_physics->friction);
                //     printf("\t");
                //     FanVector2Print(self_physics->last_position);
                //     printf("\t");
                //     FanVector2Print(self_physics->direction);
                // }
                //
                // if (self_behavior) {
                //     printf("\tself_behavior: %d\n", self_behavior_idx);
                //     printf("\tself_behavior->type: %d\n", self_behavior->type);
                //     printf("\tself_behavior->start_time: %f\n", self_behavior->start_time);
                //     printf("\tself_behavior->duration: %f\n", self_behavior->duration);
                // }
            }
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
        int32 id = render_array[i].id;
        if (id == -1) {
            continue;
        }

        int32 shape_idx     = world.c_shape.sparse[id];
        int32 transform_idx = world.c_transform.sparse[id];
        int32 move_idx      = world.c_movement.sparse[id];
        // int32 physics_idx   = world.c_physics.sparse[id];
        int32 animation_idx = world.c_animation.sparse[id];
        int32 texture_idx   = world.c_texture.sparse[id];
        int32 interact_idx  = world.c_interact.sparse[id];
        int32 tag_bg        = world.c_tag_background.sparse[id];


        CShape     *shape     = &world.c_shape.data[shape_idx];
        CTransform *transform = &world.c_transform.data[transform_idx];
        CMovement  *move      = &world.c_movement.data[move_idx];
        // CPhysics   *physics   = null;
        CTexture   *texture   = null;
        CAnimation *animation = null;
        CInteract  *interact  = null;

        FanColor   color  = (FanColor){ 0, 0, 0, 0 };
        FanVector2 offset = FanVector2Zero();
        int32 layer = 0;

        FanVector2 interact_pos = FanVector2Zero();
        FanVector2 interact_scale = FanVector2Zero();

        int32 shape_flags    = 0;
        int32 interact_flags = 0;
        int32 render_flags   = 0;
        if (id == world.spec_id.player) {
            offset = player_offset;
            shape_flags |= ShapeUpdate_SetOffset;


            interact = &world.c_interact.data[interact_idx];

            interact_pos = (FanVector2) {
                transform->position.x + transform->scale.x * move->direction.x,
                transform->position.y + transform->scale.y * move->direction.y
            };
            interact_scale = (FanVector2) {
                transform->scale.x,
                transform->scale.y
            };
            interact_flags |= InteractFlag_SetPosition;
            interact_flags |= InteractFlag_SetScale;

            render_flags   |= RenderFlag_ShowInteract;
            // offset = FanVector2Hadamard(transform->scale, move->direction);
        }
        else if (istagged(tag_bg)) {
            transform->scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() };
        }
        ShapeUpdate(shape, color, offset, layer, shape_flags, dt);

        if (interact)
            InteractUpdate(interact, transform, move, interact_pos, interact_scale, interact_flags);

        if (texture_idx != -1) {
            texture = &world.c_texture.data[texture_idx];
            if (animation_idx != -1) {
                animation = &world.c_animation.data[animation_idx];

                int32 animation_id = 0;
                if (id == world.spec_id.player) {
                    animation_id = player_animation_id;
                }
                AnimationUpdate(animation, texture, animation_id, 0, dt);
            }
        }

        if (move_idx != -1) {
            move = &world.c_movement.data[move_idx];
        }

        if (shape->visible) {
            // if (id != world.spec_id.player) {
            render_flags |= RenderFlag_FlipX;
            // }
            ShapeRender(shape, transform, texture, move, interact, render_flags);
        }


        if (called_object_dump) {
            printf("\tid: %d\n", id);

            if (id == world.spec_id.player) {
                printf("\tshape: %d\n", shape_idx);
                printf("\tshape->initialized: %s\n", shape->initialized ? "true" : "false");
                printf("\tshape->layer: %d\n", shape->layer);
                printf("\t");
                FanColorPrint(shape->color);
                printf("\t");
                FanVector2Print(shape->offset);

                if (animation) {
                    printf("\tanimation: %s (%d)\n", animation->name, animation_idx);
                    printf("\tanimation->id: %d\n", animation->id);
                    printf("\tanimation->timer: %f\n", animation->timer);
                    printf("\tanimation->current_frame: %d\n", animation->current_frame);
                    printf("\tanimation->finished: %d\n", animation->finished);
                }
            }
        }
    }
}


global void SceneSolo(void) {
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
    ComponentStorageAddArgs(&world.c_movement,  world.entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentStorageAddArgs(&world.c_texture,   world.entity_count,
        .texture = tex_link,
        .rect = (FanRect){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    ComponentStorageAddArgs(&world.c_animation, world.entity_count);
    world.spec_id.player = world.entity_count;
    world.entity_count++;

    ComponentStorageAddArgs(&world.c_transform,  world.entity_count,
        .scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() },
    );
    ComponentStorageAddArgs(&world.c_shape,      world.entity_count,
        .layer = 1,
        .color = (FanColor){ 155, 155, 155, 255 },
    );
    ComponentStorageAddArgs(&world.c_movement,   world.entity_count,
        .flags = MovementFlag_NoCollision,
    );
    ComponentStorageAdd(&world.c_tag_background, world.entity_count);
    world.entity_count++;
}

global void SceneMain(void) {
    FanTexture tex_sprite = FanTextureLoad("./resources/Sprite-0001.png");

    ComponentStorageAdd(&world.c_transform,     world.entity_count);
    ComponentStorageAdd(&world.c_shape,         world.entity_count);
    ComponentStorageAddArgs(&world.c_movement,  world.entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentStorageAddArgs(&world.c_texture,   world.entity_count,
        .texture = tex_sprite,
        .rect = { .x = 64, .y = 0, .width = 14, .height = 16 },
        // .rect = (FanRect){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    // ComponentStorageAddArgs(&world.c_animation, world.entity_count);
    ComponentStorageAddArgs(&world.c_interact,  world.entity_count,
        .zone = (FanRect){ 0 });
    world.spec_id.player = world.entity_count;
    world.entity_count++;

    // FanTexture tex_mewee = FanTextureLoad("./resources/mewee.png");
    ComponentStorageAdd(&world.c_transform,    world.entity_count);
    ComponentStorageAddArgs(&world.c_shape,    world.entity_count,
        .color = (FanColor){ 50, 255, 255, 255 }
    );
    ComponentStorageAddArgs(&world.c_movement, world.entity_count, .speed = 400.0f);
    ComponentStorageAddArgs(&world.c_texture,  world.entity_count,
        .texture = tex_sprite,
        .rect = { .x = 64, .y = 0, .width = 14, .height = 16 },
    );
    ComponentStorageAddArgs(&world.c_behavior, world.entity_count,
        .type = BehaviorType_Random,
        .duration = 0.2f
    );
    world.entity_count++;

    ComponentStorageAdd(&world.c_transform,    world.entity_count);
    ComponentStorageAddArgs(&world.c_shape,    world.entity_count,
        .color = (FanColor){ 255, 50, 255, 255 },
    );
    ComponentStorageAddArgs(&world.c_movement, world.entity_count, .speed = 150.0f);
    ComponentStorageAddArgs(&world.c_texture,  world.entity_count,
        .texture = tex_sprite,
        .rect = { .x = 64, .y = 0, .width = 14, .height = 16 },
    );
    ComponentStorageAddArgs(&world.c_behavior, world.entity_count,
        .type = BehaviorType_Follow,
    );
    world.entity_count++;

    ComponentStorageAddArgs(&world.c_transform,  world.entity_count,
        .scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() },
    );
    ComponentStorageAddArgs(&world.c_shape,      world.entity_count,
        .layer = 1,
        .color = (FanColor){ 155, 155, 155, 255 },
    );
    ComponentStorageAddArgs(&world.c_movement,   world.entity_count,
        .flags = MovementFlag_NoCollision,
    );
    ComponentStorageAdd(&world.c_tag_background, world.entity_count);
    world.entity_count++;

    ComponentStorageAddArgs(&world.c_transform, world.entity_count,
        .position = (FanVector2){ 200.0f, 300.0f },
        .scale = (FanVector2){ 400.0f, 150.0f }
    );
    ComponentStorageAddArgs(&world.c_shape,     world.entity_count,
        .color = (FanColor){ 200, 165, 175, 255 },
        .layer = 3,
    );
    ComponentStorageAddArgs(&world.c_movement,  world.entity_count,
        .flags = MovementFlag_NoCollision,
    );
    world.entity_count++;

    ComponentStorageAddArgs(&world.c_transform, world.entity_count,
        .position = (FanVector2){ 200, 100 }
    );
    ComponentStorageAddArgs(&world.c_shape,     world.entity_count,
        .color = (FanColor){ 50, 255, 255, 255 },
    );
    ComponentStorageAddArgs(&world.c_movement,  world.entity_count,
        .flags = MovementFlag_Immovable
    );
    world.entity_count++;

    ComponentStorageAddArgs(&world.c_transform, world.entity_count,
        .position = (FanVector2){ 296, 100 }
    );
    ComponentStorageAddArgs(&world.c_shape,     world.entity_count,
        .color = (FanColor){ 50, 255, 255, 255 },
    );
    ComponentStorageAddArgs(&world.c_movement,  world.entity_count,
        .flags = MovementFlag_Immovable
    );
    world.entity_count++;

    ComponentStorageAdd(&world.c_transform,    world.entity_count);
    ComponentStorageAddArgs(&world.c_movement, world.entity_count,
        .speed = 380.0f,
        .flags = MovementFlag_NoCollision,
    );
    ComponentStorageAddArgs(&world.c_behavior, world.entity_count,
        .type = BehaviorType_Follow,
    );
    world.spec_id.camera = world.entity_count;
    world.entity_count++;
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

    bool32 running             = true;
    bool32 update_entity_split = true;
    FanVector2 player_offset   = FanVector2Zero();
    FanVector2 player_index    = FanVector2Zero();

    FanRandomSeed(12398);

    ssize component_size  = kilobytes(1);

    ComponentStorageCreate(&world.c_transform,      &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_shape,          &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_movement,       &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_texture,        &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_behavior,       &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_animation,      &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_physics,        &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_interact,       &arena_allocator, component_size);

    ComponentStorageCreate(&world.c_tag_background, &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_tag_enemy,      &arena_allocator, component_size);

    SceneMain();

    FanCamera2D camera = { 0 };
    camera.zoom = 0.8f;

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
            if (player_offset.y <= -200.0f) {
                player_offset.y = -200.0f;
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
            if (player_offset.x <= -200.0f) {
                player_offset.x = -200.0f;
            }
        }

        if (FanKeyDown(FanKey_O)) {
            player_offset = FanVector2Zero();
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
        int32 cam_move_idx = world.c_transform.sparse[world.spec_id.camera];
        CTransform *cam_transform = &world.c_transform.data[cam_move_idx];
        camera.target = FanVector2Add(cam_transform->position, FanVector2Scale(cam_transform->scale, 0.5f));
        camera.offset = (FanVector2){ FanWindowWidth() / 2.0f, FanWindowHeight() / 2.0f };

        if (called_object_dump) {
            printf("Total Allocations: %.2f / %.2f KB\n", (double)world.arena.size / 1000.0f, (double)world.arena.capacity / 1000.0f);
            printf("current_time: %.3f\n", world.current_time);

            printf("Total Component 'Transform' size/capacity: \t%zu/%zu\n", world.c_transform.size, world.c_transform.capacity);
            printf("Total Component 'Shape' size/capacity:     \t%zu/%zu\n", world.c_shape.size,     world.c_shape.capacity);
            printf("Total Component 'Physics' size/capacity:   \t%zu/%zu\n", world.c_physics.size,   world.c_physics.capacity);
            printf("Total Component 'Texture' size/capacity:   \t%zu/%zu\n", world.c_texture.size,   world.c_texture.capacity);
            printf("Total Component 'Behavior' size/capacity:  \t%zu/%zu\n", world.c_behavior.size,  world.c_behavior.capacity);
            printf("Total Component 'Animation' size/capacity: \t%zu/%zu\n", world.c_animation.size, world.c_animation.capacity);
        }

        FanDrawBegin();
            FanDrawClear(FanColor_WHITE);
            FanCameraBegin(camera);
            UpdateAndRender(player_direction, player_offset, player_animation_id, update_entity_split, dt);
            FanCameraEnd();
            FanDrawFPS(2, 2);
        FanDrawEnd();
        called_object_dump = false;
        update_entity_split = false;
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
