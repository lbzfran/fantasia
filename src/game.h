#ifndef FAN_GAME_H
#define FAN_GAME_H

#include "os.h"
#include "platform.h"

#define istagged(t) (t >= 0 ? true : false)
#define TILE_SIZE 64

#define ComponentDeclare(name, T) \
    typedef struct name##Storage {       \
         int32 *sparse;                  \
         int32 *dense;                   \
             T *data;                    \
         ssize  size;                    \
         ssize  capacity;                \
    } name##Storage

#define ComponentCreate(storage, mem, size) do{                                        \
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
        (storage)->sparse[id] = (int32)(storage)->size;                 \
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

typedef struct {
    fan_vec2   position;
    fan_vec2   scale;
    float32    rotation;

    bool32     initialized;
} CTransform;

typedef struct {
    fan_color  color;
    fan_vec2   offset;
    int32      layer;

    bool32     visible;
    bool32     initialized;
} CShape;

typedef struct {
    fan_vec2   velocity_input;
    fan_vec2   velocity_force;

    fan_vec2   direction;

    float32    speed;
    float32    max_speed;

    float32    lock_time;

    int32      flags;
    bool32     active;
    bool32     initialized;
} CMovement;

typedef struct {
    fan_vec2   last_position;

    float32    speed;
    float32    friction;
    float32    mass;

    int32      flags;
    bool32     active;
    bool32     initialized;
} CPhysics;

typedef struct {
    fan_texture texture;
    fan_rect    rect;
} CTexture;

typedef struct CBehavior {
    enum BehaviorType {
        BehaviorType_None   = 0,
        BehaviorType_Random = 1,
        BehaviorType_Follow = 2,
    } type;
    ssize   target_id;
    float32 timer;
    float32 update_time;
    bool32  updating;
} CBehavior;

typedef struct {
    int32 id;
    int32 flags;
} AnimationRequest;

typedef struct {
    const char8      *name;
    fan_rect         *frames;
    float32           frame_time;
    int32             frame_count;
    bool32            loop;

    int32             next_id;
} AnimationData;

typedef struct {
    const char8       *name;
    int32              id;
    float32            timer;
    int32              current_frame;
    bool32             finished;
    int32              flags;

    AnimationRequest   request;
} CAnimation;

typedef struct {
    fan_sound sound;
    bool32   playing;
    bool32   looping;
    float32  volume;
    float32  pitch;
} CSound;

/*
 *
 * directional, point light, spotlight
 *
 */

typedef struct {
    fan_vec2   direction;
    float32    radius;

    fan_color  color;
    float32    intensity;
} CLight;

// typedef struct {
//     int32 id;
//     const char *name;
// } CItem;
//
// typedef struct {
//     float32 value;
// } CEquipment;
//
// #define MAX_INVENTORY 8
//
// typedef struct {
//     int32 slots[MAX_INVENTORY];
//     int32 count;
// } CInventory;

typedef struct {
    float32 attack_range;
    float32 knockback;

    float32 arc_angle;
    float32 swing_time;
    float32 timer;

    float32 cooldown_time;
    float32 cast_timer;
    bool32  attacking;
} CAttack;


typedef struct {
    int32 player;
    int32 camera;
} SpecialEntityID;

/*
 * Tilemaps will have a strict 16-size array
 * of rectangles that point to the exact location on a texture.
 * If this is standardized correctly across the textures,
 * this tilemap will only need to be initialized and set once.
 * TILESETS NECESSARY
 * all corners
 * outer bottom-right
 * outer bottom-left
 * outer top-right
 * outer top-left
 * right edge
 * left edge
 * bottom edge
 * top edge
 * outer bottom-right
 * outer bottom-left
 * outer top-right
 * outer top-left
 * bottom-left top-right
 * top-left bottom-right
 * no corners
 */
typedef struct {
    int32 sprite_id;
    fan_rect rect;
} TileVisual;

typedef struct {
    bool32 solid;
} TileLogic;

typedef struct {
    fan_vec2 tl, tr, bl, br;
} TileOffsets;

typedef struct {
    float32 sizes[4];
    fan_vec2 coordinates[16];
} GridAtlas;

typedef enum {
    SystemMode_Overworld = 0,
    SystemMode_Menu,
    SystemMode_Battle
} SystemMode;

typedef struct {
    fan_vec2  direction;
    int32     actions[4];
} PlayerInput;

typedef struct {
    SystemMode   mode;
    fan_music    music;

    PlayerInput  p_input;

    fan_rect_i32 bound_zone;
    float64      current_time;
    float32      camera_zoom;

    fan_rtexture rendermap;
    fan_rtexture lightmap;

    // TODO(liam): support dynamic window resizing
    // also support minimum window size in platform layer.
    bool32       resized;
    int32        window_width;
    int32        window_height;

    fan_vec2     render_size;
    fan_vec2     world_offset;
    int32        world_scale; // pixels_per_unit

    bool32       called_object_dump;
} GameState;

typedef struct {
    int32 *dynamic_entities;
    ssize  dynamic_count;
    ssize  dynamic_capacity;

    int32 *static_entities;
    ssize  static_count;
    ssize  static_capacity;
} EntitySplit;

typedef struct {
    fan_matrix  tiles;  // 1D repr 2D plane
    fan_vec2    origin;
    int32       tile_size;
} TileMap;

ComponentDeclare(CTransform, CTransform);
ComponentDeclare(CShape,     CShape);
ComponentDeclare(CMovement,  CMovement);
ComponentDeclare(CTexture,   CTexture);

ComponentDeclare(CBehavior,  CBehavior);
ComponentDeclare(CAnimation, CAnimation);
ComponentDeclare(CPhysics,   CPhysics);
ComponentDeclare(CSound,     CSound);
ComponentDeclare(CLight,     CLight);

ComponentDeclare(CInteraction,  bool32);
ComponentDeclare(CInteractable, bool32);
ComponentDeclare(CZone,         fan_rect_f32);
ComponentDeclare(CAttack,       CAttack);

ComponentDeclare(CEnemyTag,      uint8);
ComponentDeclare(CBackgroundTag, uint8);


typedef struct {
    Arena                  arena;

    uint8                  entity_count;
    SpecialEntityID        spec_id;

    AnimationData         *anim_table;
    ssize                  anim_table_size;

    EntitySplit            split;
	bool32                 update_entity_split;

    GridAtlas              tile_atlas;
    fan_texture           *tilesets;
	TileMap                map;

    CTransformStorage      c_transform;
    CShapeStorage          c_shape;
    CMovementStorage       c_movement;
    CTextureStorage        c_texture;

    CBehaviorStorage       c_behavior;
    CAnimationStorage      c_animation;
    CPhysicsStorage        c_physics;
    CSoundStorage          c_sound;
    CLightStorage          c_light;

    CInteractionStorage    c_interaction;
    CInteractableStorage   c_interactable;
    CZoneStorage           c_zone;
    CAttackStorage         c_attack;

    CEnemyTagStorage       c_tag_enemy;
    CBackgroundTagStorage  c_tag_background;
} World;

void GAME_API GameInit(Allocator *a, World *world, GameState *state);
void GAME_API GameUpdateAndRender(Allocator *a, World *world, GameState *state, float32 dt);
void GAME_API GameClose(Allocator *a, World *world, GameState *state);

#endif // FAN_GAME_H
