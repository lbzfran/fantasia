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

    float32    speed;
    float32    max_speed;

    int32      flags;
    bool32     active;
    bool32     initialized;
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
    FanRectInt32    rect;
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

    PlayerInput  p_input;

    FanRectInt32 bound_zone;
    float64      current_time;
    float32      camera_zoom;

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

ComponentDeclare(CInteraction,  bool32);
ComponentDeclare(CInteractable, bool32);
ComponentDeclare(CZone,         FanRectFloat32);

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

    CInteractionStorage    c_interaction;
    CInteractableStorage   c_interactable;
    CZoneStorage           c_zone;

    CEnemyTagStorage       c_tag_enemy;
    CBackgroundTagStorage  c_tag_background;
} World;

void GAME_API GameInit(Allocator *a, World *world, GameState *state);
void GAME_API GameUpdateAndRender(Allocator *a, World *world, GameState *state, float32 dt);
void GAME_API GameClose(Allocator *a, World *world, GameState *state);

#endif // FAN_GAME_H
