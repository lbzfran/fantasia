#ifndef FAN_GAME_H
#define FAN_GAME_H

#include "os.h"
#include "platform.h"

#define istagged(t) (t >= 0 ? true : false)

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

typedef struct {
    FanVector2 position;
    FanVector2 scale;
    float32    rotation;

    bool32     initialized;
} CTransform;

typedef struct {
    FanColor   color;
    FanVector2 offset;
    int32      layer;

    bool32     visible;
    bool32     initialized;
} CShape;

typedef struct {
    FanVector2 velocity_input;
    FanVector2 velocity_force;

    FanVector2 direction;

    float32    speed;
    float32    max_speed;

    float32    lock_time;

    int32      flags;
    bool32     active;
    bool32     initialized;
} CMovement;

typedef struct {
    FanVector2 last_position;

    float32    speed;
    float32    friction;
    float32    mass;

    int32      flags;
    bool32     active;
    bool32     initialized;
} CPhysics;

typedef struct {
    FanTexture texture;
    FanRectInt32    rect;
} CTexture;

typedef struct CBehavior {
    enum BehaviorType {
        BehaviorType_None   = 0,
        BehaviorType_Random = 1,
        BehaviorType_Follow = 2,
    } type;
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
    FanRectInt32     *frames;
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
    FanSound sound;
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
    FanVector2 direction;
    float32    radius;

    FanColor   color;
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

typedef enum {
    SystemMode_Overworld = 0,
    SystemMode_Menu,
    SystemMode_Battle
} SystemMode;

typedef struct {
    FanVector2 direction;
    int32      actions[4];
} PlayerInput;

typedef struct {
    SystemMode   mode;
    FanMusic     music;

    PlayerInput  p_input;

    FanRectInt32 bound_zone;
    float64      current_time;
    float32      camera_zoom;

    FanRTexture  lightmap;

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
    ssize rows;
    ssize cols;
    int32 *V;
} MatrixInt32;

typedef struct {
    MatrixInt32 tiles; // 1D repr 2D plane

    FanVector2 origin; // top-left, relative to screen
    int32 tile_size;
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
ComponentDeclare(CZone,         FanRectFloat32);
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

	TileMap                map;
    int32                  pixels_per_unit;

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
