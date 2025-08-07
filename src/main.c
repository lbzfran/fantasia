
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

#define true    1
#define false   0
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

    uintptr expected_size = offset - size;
    if (a->size == expected_size) {
        a->size -= size;
    }
}

void arena_clear(Arena *a) {
    a->size = 0;
}

void *arena_resize(void *ctx, void *ptr, ssize old, ssize new) {
    // TODO(liam): make this
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
        else if (a->size == offset - old) {
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

typedef struct CMovement {
    FanVector2 position;
    FanVector2 last_position;
    FanVector2 direction;

    float32 speed;
    float32 friction;

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
} CAnimation;

typedef struct {
    const char8 *name;
    FanRect *frames;
    float32 frame_time;
    int32 frame_count;
    bool32 loop;
} AnimationData;

FanRect player_idle_up_frames[1]    = { 0 };
FanRect player_idle_down_frames[3]  = { 0 };
FanRect player_idle_left_frames[3]  = { 0 };
FanRect player_idle_right_frames[3] = { 0 };

AnimationData anim_table[] = {
    { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.5f, .frame_count = 3, true  },
    { "player_idle_up",    player_idle_up_frames,    .frame_time = 0.0f, .frame_count = 1, false },
    { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.5f, .frame_count = 3, true  },
    { "player_idle_right", player_idle_right_frames, .frame_time = 0.5f, .frame_count = 3, true  },
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
        init_if_null(m->position.x, 0.0f);
        init_if_null(m->position.y, 0.0f);

        init_if_null(m->last_position.x, m->position.x);
        init_if_null(m->last_position.y, m->position.y);

        init_if_null(m->direction.x, 1.0f);
        init_if_null(m->direction.y, 1.0f);

        init_if_null(m->speed,    500.0f);
        init_if_null(m->friction, 1.0f);

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

/*
 * type: System
 * components: CAnimation, CTexture
 */
void AnimationUpdate(CAnimation *a, CTexture *t, int32 id, float dt) {
    AnimationData *data = &anim_table[a->id];
    if (a->id != id or a->name is null) {
        a->id = id;
        a->timer = 0.0f;
        a->current_frame = 0;
        a->finished = false;

        data = &anim_table[a->id];
        a->name = data->name;
    }

    if (not data->loop and a->finished) {
        return;
    }

    a->timer += dt;
    if (a->timer >= data->frame_time) {
        a->timer -= data->frame_time;

        a->current_frame++;
        if (a->current_frame >= data->frame_count) {
            if (data->loop) {
                a->current_frame = 0;
            }
            else {
                a->current_frame = data->frame_count - 1;
                a->finished = true;
            }
        }
    }
    // NOTE(liam): is it acceptable to keep this snippet here
    //             rather than outside?
    FanRect current_data = data->frames[a->current_frame];
    TextureUpdate(
        t,
        (FanVector2){
            current_data.x,
            current_data.y
        },
        (FanVector2){
            current_data.width,
            current_data.height
        },
        dt
    );
}

enum RenderFlags {
    RenderFlag_FlipX = (1 << 0),
    RenderFlag_FlipY = (1 << 1)
};
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


enum SpecialEntity {
    Entity_Player_One = 0,
    Entity_Background = 4,
};

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
    bool32 called_object_dump = false;
    FanVector2 player_offset  = FanVector2Zero();
    FanVector2 player_index   = FanVector2Zero();
    int32 player_animation_id = 0;

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
        .position = (FanVector2){ 200.0f, 300.0f }
    );
    ComponentStorageAddArgs(&world.c_color,    world.entity_count, 50, 255, 50, 255);
    world.entity_count++;

    ComponentStorageAddArgs(&world.c_body,  world.entity_count,
        .layer = 1,
        .scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() }
    );
    ComponentStorageAdd(&world.c_movement,  world.entity_count);
    ComponentStorageAddArgs(&world.c_color, world.entity_count, 175, 165, 175, 255);
    int32 background_id = world.entity_count;
    world.entity_count++;

    while (running) {
        float dt = FanGetFrameTime();
        if (FanWindowShouldClose() || FanKeyPressed(FanKey_ESCAPE)) {
            running = false;
        }

        FanVector2 player_direction = FanVector2Zero();
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
                    FanVector2 local_direction = FanVector2Zero();
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
                    MovementUpdate(local_movement, local_body, local_direction, dt);
                }

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

                FanVector2 new_scale  = FanVector2Zero();
                FanVector2 new_offset = FanVector2Zero();
                int32 new_layer = -1;
                if (local_id == Entity_Player_One) {
                    new_offset = player_offset;
                }
                else if (local_id == background_id) {
                    new_scale  = (FanVector2){ FanWindowWidth(), FanWindowHeight() };
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
                        AnimationUpdate(local_animation, local_texture, local_animation_id, dt);
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
                    printf("\tlocal_body_index: %zu\n", i);
                    printf("\tlocal_body->initialized: %s\n", local_body->initialized ? "true" : "false");
                    printf("\tlocal_body->layer: %d\n", local_body->layer);
                    printf("\t");
                    FanVector2Print(local_body->scale);
                    printf("\t");
                    FanVector2Print(local_body->offset);

                    if (local_animation isnt null) {
                        printf("\tlocal_animation_index: %d\n", local_animation_index);
                        printf("\tlocal_animation->id: %d\n", local_animation->id);
                    }
                }
            }

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
