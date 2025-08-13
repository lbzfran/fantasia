
#include <string.h>
#include "platform.h"

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>
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


#define ComponentDeclare(name, T) \
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
#define ComponentAdd(storage, id) do{                                   \
        (storage)->sparse[id] = (storage)->size;                        \
        (storage)->dense[(storage)->size % (storage)->capacity] = (id); \
        (storage)->size++;                                              \
        if ((storage)->size >= (storage)->capacity)                     \
            assert(false && "Out of Memory!");                          \
    }while(0);


#define ComponentArgs(storage, id, ...) do{                                                             \
        assert((storage)->sparse[id] != -1 && "Attempted to pass component args to unassigned entity"); \
        (storage)->data[(storage)->sparse[id]] = (typeof(*(storage)->data)){__VA_ARGS__};               \
    }while(0);

#define ComponentAddArgs(storage, id, ...) do{ \
    ComponentAdd(storage, id);                 \
    ComponentArgs(storage, id, __VA_ARGS__);   \
}while(0);

// WARN: no bounds check
#define ComponentDelete(storage, id, count_ptr) do{                             \
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

typedef struct {
    int32 id;
    int32 flags;
} AnimationRequest;

// state manager component
typedef struct CAnimation {
    const char8       *name;
    int32              id;
    float32            timer;
    int32              current_frame;
    bool32             finished;
    int32              flags;

    AnimationRequest   request;
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
    int32 player;
    int32 camera;
} SpecialEntityID;
#define istagged(t) (t >= 0 ? true : false)

FanRect player_idle_up_frames[1]    = { 0 };
FanRect player_idle_down_frames[3]  = { 0 };
FanRect player_idle_left_frames[3]  = { 0 };
FanRect player_idle_right_frames[3] = { 0 };

AnimationData anim_table[] = {
    { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.5f, .frame_count = 3, true,  -1 },
    { "player_idle_up",    player_idle_up_frames,    .frame_time = 1.5f, .frame_count = 1, false,  0 },
    { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.5f, .frame_count = 3, false,  0 },
    { "player_idle_right", player_idle_right_frames, .frame_time = 0.5f, .frame_count = 3, false,  0 },
};

ComponentDeclare(CTransform, CTransform);
ComponentDeclare(CShape,     CShape);
ComponentDeclare(CMovement,  CMovement);
ComponentDeclare(CTexture,   CTexture);

ComponentDeclare(CBehavior,  CBehavior);
ComponentDeclare(CAnimation, CAnimation);
ComponentDeclare(CPhysics,   CPhysics);

ComponentDeclare(CInteraction,  bool32);
ComponentDeclare(CInteractable, bool32);
ComponentDeclare(CZone,         FanRect);

ComponentDeclare(CEnemyTag,      uint8);
ComponentDeclare(CBackgroundTag, uint8);

void MovementSystem(CMovement *m, CTransform *t, FanVector2 direction, float32 dt) {
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

void PhysicsSystem(CPhysics *p, CTransform *t, FanVector2 force, float32 dt) {
    if (not p->initialized) {
        init_if_null(p->last_position.x, t->position.x);
        init_if_null(p->last_position.y, t->position.y);

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

inline bool32 CollisionCheckR(FanRect a, FanRect b) {
    bool32 result = false;

    result = not (a.x + a.width  < b.x or b.x + b.width  < a.x or
                  a.y + a.height < b.y or b.y + b.height < a.y);

    return result;
}

bool32 CollisionCheckV(FanVector2 aPos, FanVector2 aSize, FanVector2 bPos, FanVector2 bSize) {
    bool32 result = false;

    // AABB
    result = not (aPos.x + aSize.x < bPos.x or bPos.x + bSize.x < aPos.x or
                  aPos.y + aSize.y < bPos.y or bPos.y + bSize.y < aPos.y);

    return result;
}

bool32 CollisionSystem(
        CTransform *a,
	    CMovement *a_m,
	    CTransform *b,
	    CMovement *b_m,
	    float32 dt
    ) {
    // NOTE(liam): this check is prob unnecessary
    if (a_m->flags & MovementFlag_NoCollision or b_m->flags & MovementFlag_NoCollision)
        return false;

    if (CollisionCheckV(a->position, a->scale, b->position, b->scale)) {
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
            return false;

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

        return true;
    }
    // NOTE(liam): potentially handle 'tunneling' if needed
    // likely solution: https://blog.hamaluik.ca/posts/swept-aabb-collision-using-minkowski-difference/
    return false;
}

void TextureUpdate(CTexture *t, FanVector2 pos, FanVector2 size, float dt) {
    (void)dt;

    t->rect = (FanRect){
        .x      = pos.x,
        .y      = pos.y,
        .width  = coalesce(size.x, t->rect.width),
        .height = coalesce(size.y, t->rect.height)
    };
}

inline CAnimation AnimationApply_(int32 new_id, int32 flags, AnimationRequest ar) {
    assert(new_id != -1 && "Out of Bounds Access!");
    CAnimation new_state = (CAnimation) {
        .id = new_id,
        .name = anim_table[new_id].name,
        .flags = flags,
        .request = ar
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
void AnimationSystem(CAnimation *a, CTexture *t, float dt) {
    if (a->finished or
        (a->request.id != -1 and (a->flags & AnimationFlag_NotInterruptible) == false)) {
        *a = AnimationApply_(a->request.id, a->request.flags, (AnimationRequest){ -1, 0 });
    }

    AnimationData *data = &anim_table[a->id];

    a->timer += dt;
    if (a->timer >= data->frame_time) {
        a->timer -= data->frame_time;
        a->current_frame++;

        if (a->current_frame >= data->frame_count) {
            if (data->loop and (a->flags & AnimationFlag_DisableLoop) == false) {
                a->current_frame = 0;
            }
            else {
                if (data->next_id != -1) {
                    *a = AnimationApply_(data->next_id, a->flags, a->request);
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
    RenderFlag_FlipX        = (1 << 0),
    RenderFlag_FlipY        = (1 << 1),
    RenderFlag_ShowInteract = (1 << 2)
} RenderFlags;
/*
 * type: System
 * component(s): Transform, Shape, Texture (opt), Physics (opt)
 */
void RenderSystem(
        CShape *s,
	    CTransform *t,
	    CTexture *tx,
	    CMovement *m,
	    bool32 interacting,
	    FanRect zone,
	    int32 flags
    ) {
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

        if (flags & RenderFlag_ShowInteract) {
            FanColor zone_color = interacting ?
                (FanColor){ 255, 0, 0, 75 } : (FanColor){ 0, 255, 0, 75 };

            FanDrawRectR(zone, zone_color);
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

    CInteractionStorage    c_interaction;
    CInteractableStorage   c_interactable;
    CZoneStorage           c_zone;

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

typedef struct {
    FanVector2 direction;
    int32 actions[4];
} PlayerInput;

void RenderEntities(PlayerInput p_input, float32 dt) {
    RenderEntry render_array[128] = { { -1, 0.0f, 0 } };
    ssize render_entry_count = 0;

    for (ssize i = 0; i < world.c_shape.size; i++) {
        int32 id = world.c_shape.dense[i];
        if (id == -1)
            continue;
        CShape *shape = &world.c_shape.data[i];

        int32 transform_idx = world.c_transform.sparse[id];
        if (transform_idx == -1)
            continue;
        CTransform *transform = &world.c_transform.data[transform_idx];

        if (not shape->initialized) {
            if (shape->color.a == 0) {
                shape->color = FanColor_WHITE;
            }
            init_if_null(shape->layer, 2);

            shape->visible = true;
            shape->initialized = true;
        }

        render_array[render_entry_count++] = (RenderEntry) {
            id,
            transform->position.y + transform->scale.y,
            shape->layer
        };
    }

    SortRender(render_array, 0, world.c_shape.size - 1);

    for (ssize i = 0; i < render_entry_count; i++) {
        int32 id = render_array[i].id;
        if (id == -1)
            continue;

        int32 shape_idx        = world.c_shape.sparse[id];
        int32 transform_idx    = world.c_transform.sparse[id];
        int32 move_idx         = world.c_movement.sparse[id];
        int32 animation_idx    = world.c_animation.sparse[id];
        int32 texture_idx      = world.c_texture.sparse[id];

        int32 tag_bg           = world.c_tag_background.sparse[id];

        int32 interaction_idx  = world.c_interaction.sparse[id];
        int32 interactable_idx = world.c_interactable.sparse[id];
        int32 zone_idx         = world.c_zone.sparse[id];

        CShape       *shape       = &world.c_shape.data[shape_idx];
        CTransform   *transform   = &world.c_transform.data[transform_idx];
        CMovement    *move        = &world.c_movement.data[move_idx];
        CTexture     *texture     = null;
        CAnimation   *animation   = null;
        bool32        interacting = false;
        bool32        interacted  = false;

        if (not shape->visible) {
            printf("skipping %d!\n", id);
            continue;
        }

        FanRect zone       = { 0 };
        int32 render_flags = 0;

        if (zone_idx != -1) {
            zone = world.c_zone.data[zone_idx];
        }
        else {
            zone = (FanRect) {
                transform->position.x,
                transform->position.y,
                transform->scale.x,
                transform->scale.y,
            };
        }

        if (interaction_idx != -1) {
            interacting = world.c_interaction.data[interaction_idx];
        }

        if (interactable_idx != -1) {
            interacted  = world.c_interactable.data[interactable_idx];
        }

        if (texture_idx != -1) {
            texture       = &world.c_texture.data[texture_idx];
            if (animation_idx != -1) {
                animation = &world.c_animation.data[animation_idx];
            }
        }

        if (move_idx != -1) {
            move = &world.c_movement.data[move_idx];
        }

        render_flags |= RenderFlag_FlipX;

        if (p_input.actions[1])
            render_flags |= RenderFlag_ShowInteract;

        RenderSystem(shape, transform, texture, move, interacting, zone, render_flags);

        if (called_object_dump) {
            printf("id: %d\n", id);
            printf("interacting: %s\n", interacting ? "true" : "false");
            printf("interacted: %s\n", interacted ? "true" : "false");
            FanRectPrint(zone);
        }
    }
}


void UpdateEntities(
        PlayerInput p_input,
        bool32 update_entity_split,
        float32 dt
    ) {
    if (update_entity_split) {
        UpdateEntitySplit();
    }

    for (ssize i = 0; i < world.c_transform.size; i++) {
        int32 id = world.c_transform.dense[i];
        if (id == -1)
            continue;

        CTransform *transform = &world.c_transform.data[i];
        if (not transform->initialized) {
            init_if_null(transform->scale.x, 96.0f);
            init_if_null(transform->scale.y, 96.0f);

            transform->initialized = true;
        }
    }

    for (ssize i = 0; i < world.c_movement.size; i++) {
        int32 id = world.c_movement.dense[i];
        if (id == -1)
            continue;

        int32 transform_idx   = world.c_transform.sparse[id];
        int32 behavior_idx    = world.c_behavior.sparse[id];
        int32 interact_idx    = world.c_interaction.sparse[id];
        int32 interacted_idx  = world.c_interactable.sparse[id];

        CMovement  *move      = &world.c_movement.data[i];
        CTransform *transform = &world.c_transform.data[transform_idx];
        CBehavior  *behavior  = null;

        FanVector2 direction = FanVector2Zero();

        if (interact_idx != -1) {
            world.c_interaction.data[interact_idx] = false;
        }

        if (interacted_idx != -1) {
            world.c_interactable.data[interacted_idx] = false;
        }

        if (id == world.spec_id.player) {
            direction = p_input.direction;
        }
        else if (behavior_idx != -1) {
            behavior = &world.c_behavior.data[behavior_idx];
            switch (behavior->type) {
                case BehaviorType_Random: {
                    if (world.current_time - behavior->start_time > behavior->duration) {
                        direction = (FanVector2) {
                            FanRandomInt(-1, 1),
                            FanRandomInt(-1, 1)
                        };
                        behavior->start_time = world.current_time;
                    }
                    else {
                        // keeps entity moving rather than staying still
                        direction = move->direction;
                    }
                } break;
                case BehaviorType_Follow: {
                    CTransform *target_transform = &world.c_transform.data[0];
                    FanVector2 target_face = target_transform->position;
                       if (id == world.spec_id.camera) {
                        CShape *target_shape = &world.c_shape.data[0];
                        target_face = FanVector2Add(target_face, target_shape->offset);
                    }
                    FanVector2 face = FanVector2Normalize(FanVector2Sub(target_face, transform->position));

                    direction = (FanVector2){ signof(face.x), signof(face.y) };
                } break;
                case BehaviorType_None:
                default: {
                } break;
            }
        }

        MovementSystem(move, transform, direction, dt);


        if (called_object_dump) {
            printf("\tid: %d\n", id);

            if (id == world.spec_id.player) {
                FanVector2Print(transform->position);
                FanVector2Print(transform->scale);

                if (move) {
                    printf("\t");
                    FanVector2Print(move->velocity);
                    FanVector2Print(move->direction);
                    printf("\tmove->speed: %f\n", move->speed);
                }
            }
        }
    }

    for (ssize i = 0; i < world.c_animation.size; i++) {
        int32 id = world.c_animation.dense[i];
        if (id == -1)
            continue;

        int32 texture_idx = world.c_texture.sparse[id];

        CAnimation *anim  = &world.c_animation.data[i];
        CTexture *texture = &world.c_texture.data[texture_idx];

        AnimationSystem(anim, texture, dt);
    }

    for (ssize i = 0; i < dynamic_entity_count; i++) {
        int32 id = dynamic_entities[i];


        int32 move_idx        = world.c_movement.sparse[id];
        int32 transform_idx   = world.c_transform.sparse[id];
        int32 interact_idx    = world.c_interaction.sparse[id];
        int32 zone_idx        = world.c_zone.sparse[id];

        int32 tag_enemy       = world.c_tag_enemy.sparse[id];

        CMovement  *move      = &world.c_movement.data[move_idx];
        CTransform *transform = &world.c_transform.data[transform_idx];
        bool32     *interact  = null;
        FanRect     zone      = (FanRect) { 0 };

        if (interact_idx != -1) {
            interact = &world.c_interaction.data[interact_idx];
        }

        if (zone_idx != -1) {
            zone = world.c_zone.data[zone_idx];
        }
        else {
            zone = (FanRect) {
                transform->position.x,
                transform->position.y,
                transform->scale.x,
                transform->scale.y,
            };
        }

        if (move->active and not (move->flags & MovementFlag_NoCollision)) {
            for (ssize j = i + 1; j < dynamic_entity_count; j++) {
                int32 other_id = dynamic_entities[j];

                int32 other_transform_idx    = world.c_transform.sparse[other_id];
                int32 other_move_idx         = world.c_movement.sparse[other_id];
                assert(other_move_idx != -1);
                int32 other_interacted_idx   = world.c_interactable.sparse[other_id];
                int32 other_zone_idx         = world.c_zone.sparse[id];

                int32 other_tag_enemy        = world.c_tag_enemy.sparse[id];

                CTransform *other_transform  = &world.c_transform.data[other_transform_idx];
                CMovement  *other_move       = &world.c_movement.data[other_move_idx];
                bool32     *other_interacted = null;
                FanRect     other_zone       = (FanRect) { 0 };

                if (other_interacted_idx != -1) {
                    other_interacted = &world.c_interactable.data[other_interacted_idx];
                }

                if (other_zone_idx != -1) {
                    other_zone = world.c_zone.data[other_zone_idx];
                }
                else {
                    other_zone = (FanRect) {
                        other_transform->position.x,
                        other_transform->position.y,
                        other_transform->scale.x,
                        other_transform->scale.y,
                    };
                }

                CollisionSystem(transform, move, other_transform, other_move, dt);
                if (interact and other_interacted and
                    not (istagged(tag_enemy) and istagged(other_tag_enemy)) and
                    CollisionCheckR(zone, other_zone)) {
                    *interact = true;
                    *other_interacted = true;
                }
            }
            for (ssize i = 0; i < static_entity_count; i++) {
                int32 other_id = static_entities[i];

                int32 other_transform_idx    = world.c_transform.sparse[other_id];
                int32 other_move_idx         = world.c_movement.sparse[other_id];
                assert(other_move_idx != -1);

                CTransform *other_transform  = &world.c_transform.data[other_transform_idx];
                CMovement  *other_move       = &world.c_movement.data[other_move_idx];

                CollisionSystem(transform, move, other_transform, other_move, dt);
            }
        }

        if (called_object_dump) {
            printf("\tid: %d\n", id);

            if (id == world.spec_id.player) {
                printf("\t");
                FanVector2Print(transform->position);
                printf("\t");
                FanVector2Print(transform->scale);

                if (move) {
                    printf("\t");
                    FanVector2Print(move->velocity);
                    printf("\t");
                    FanVector2Print(move->direction);
                    printf("\tmove->speed: %f\n", move->speed);
                }
                printf("\t");
                FanRectPrint(zone);
            }
        }
    }

    RenderEntities(p_input, dt);
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

    ComponentAdd(&world.c_transform,     world.entity_count);
    ComponentAdd(&world.c_shape,         world.entity_count);
    ComponentAddArgs(&world.c_movement,  world.entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentAddArgs(&world.c_texture,   world.entity_count,
        .texture = tex_link,
        .rect = (FanRect){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    ComponentAddArgs(&world.c_animation, world.entity_count);
    world.spec_id.player = world.entity_count;
    world.entity_count++;

    ComponentAddArgs(&world.c_transform,  world.entity_count,
        .scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() },
    );
    ComponentAddArgs(&world.c_shape,      world.entity_count,
        .layer = 1,
        .color = (FanColor){ 155, 155, 155, 255 },
    );
    ComponentAddArgs(&world.c_movement,   world.entity_count,
        .flags = MovementFlag_NoCollision,
    );
    ComponentAdd(&world.c_tag_background, world.entity_count);
    world.entity_count++;
}

global void SceneMain(void) {
    FanTexture tex_sprite = FanTextureLoad("./resources/Sprite-0001.png");

    ComponentAdd(&world.c_transform,     world.entity_count);
    ComponentAdd(&world.c_shape,         world.entity_count);
    ComponentAddArgs(&world.c_movement,  world.entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentAddArgs(&world.c_texture,   world.entity_count,
        .texture = tex_sprite,
        .rect = { .x = 64, .y = 0, .width = 14, .height = 16 },
        // .rect = (FanRect){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    // ComponentAddArgs(&world.c_animation, world.entity_count);
    ComponentAdd(&world.c_interaction, world.entity_count);
    ComponentAdd(&world.c_interactable, world.entity_count);
    world.spec_id.player = world.entity_count;
    world.entity_count++;

    // FanTexture tex_mewee = FanTextureLoad("./resources/mewee.png");
    ComponentAdd(&world.c_transform,    world.entity_count);
    ComponentAddArgs(&world.c_shape,    world.entity_count,
        .color = (FanColor){ 50, 255, 255, 255 }
    );
    ComponentAddArgs(&world.c_movement, world.entity_count, .speed = 400.0f);
    ComponentAddArgs(&world.c_texture,  world.entity_count,
        .texture = tex_sprite,
        .rect = { .x = 0, .y = 0, .width = 36, .height = 36 },
    );
    ComponentAddArgs(&world.c_behavior, world.entity_count,
        .type = BehaviorType_Random,
        .duration = 0.2f
    );
    ComponentAdd(&world.c_interaction, world.entity_count);
    ComponentAdd(&world.c_interactable, world.entity_count);
    ComponentAdd(&world.c_tag_enemy, world.entity_count);
    world.entity_count++;

    ComponentAdd(&world.c_transform,    world.entity_count);
    ComponentAddArgs(&world.c_shape,    world.entity_count,
        .color = (FanColor){ 255, 50, 255, 255 },
    );
    ComponentAddArgs(&world.c_movement, world.entity_count, .speed = 150.0f);
    ComponentAddArgs(&world.c_texture,  world.entity_count,
        .texture = tex_sprite,
        .rect = { .x = 64, .y = 0, .width = 14, .height = 16 },
    );
    ComponentAddArgs(&world.c_behavior, world.entity_count,
        .type = BehaviorType_Follow,
    );
    ComponentAdd(&world.c_interaction, world.entity_count);
    ComponentAdd(&world.c_interactable, world.entity_count);
    ComponentAdd(&world.c_tag_enemy, world.entity_count);
    world.entity_count++;

    ComponentAddArgs(&world.c_transform,  world.entity_count,
        .scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() },
    );
    ComponentAddArgs(&world.c_shape,      world.entity_count,
        .layer = 1,
        .color = (FanColor){ 155, 155, 155, 255 },
    );
    ComponentAddArgs(&world.c_movement,   world.entity_count,
        .flags = MovementFlag_NoCollision,
    );
    ComponentAdd(&world.c_tag_background, world.entity_count);
    world.entity_count++;

    ComponentAddArgs(&world.c_transform, world.entity_count,
        .position = (FanVector2){ 200.0f, 300.0f },
        .scale = (FanVector2){ 400.0f, 150.0f }
    );
    ComponentAddArgs(&world.c_shape,     world.entity_count,
        .color = (FanColor){ 200, 165, 175, 255 },
        .layer = 3,
    );
    ComponentAddArgs(&world.c_movement,  world.entity_count,
        .flags = MovementFlag_NoCollision,
    );
    world.entity_count++;

    ComponentAddArgs(&world.c_transform, world.entity_count,
        .position = (FanVector2){ 200, 100 },
    );
    ComponentAddArgs(&world.c_shape,     world.entity_count,
        .color = (FanColor){ 50, 255, 255, 255 },
    );
    ComponentAddArgs(&world.c_movement,  world.entity_count,
        .flags = MovementFlag_Immovable,
    );
    world.entity_count++;

    ComponentAddArgs(&world.c_transform, world.entity_count,
        .position = (FanVector2){ 296, 100 },
    );
    ComponentAddArgs(&world.c_shape,     world.entity_count,
        .color = (FanColor){ 50, 255, 255, 255 },
    );
    ComponentAddArgs(&world.c_movement,  world.entity_count,
        .flags = MovementFlag_Immovable,
    );
    world.entity_count++;

    ComponentAdd(&world.c_transform,    world.entity_count);
    ComponentAddArgs(&world.c_movement, world.entity_count,
        .speed = 380.0f,
        .flags = MovementFlag_NoCollision,
    );
    ComponentAddArgs(&world.c_behavior, world.entity_count,
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

    ComponentStorageCreate(&world.c_interaction,    &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_interactable,   &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_zone,           &arena_allocator, component_size);

    ComponentStorageCreate(&world.c_tag_background, &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_tag_enemy,      &arena_allocator, component_size);

    SceneMain();

    FanCamera2D camera = { 0 };
    camera.zoom = 0.8f;
    PlayerInput p_input = { 0 };

    while (running) {
        float dt = FanGetFrameTime();
        if (FanWindowShouldClose() || FanKeyPressed(FanKey_ESCAPE)) {
            running = false;
        }

        int32 player_animation_id = -1;
        p_input.direction = (FanVector2){ 0 };
        if (FanKeyDown(FanKey_W)) {
            p_input.direction.y -= 1;
            player_animation_id = 1;
        }
        if (FanKeyDown(FanKey_S)) {
            p_input.direction.y += 1;
            player_animation_id = 0;
        }
        if (FanKeyDown(FanKey_A)) {
            p_input.direction.x -= 1;
            player_animation_id = 2;
        }
        if (FanKeyDown(FanKey_D)) {
            p_input.direction.x += 1;
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

        if (FanKeyDown(FanKey_E)) {
            p_input.actions[0] = true;
        }
        else {
            p_input.actions[0] = false;
        }
        if (FanKeyPressed(FanKey_R)) {
            p_input.actions[1] = not p_input.actions[1];
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
            UpdateEntities(p_input, update_entity_split, dt);
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
