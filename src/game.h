#ifndef FAN_GAME_H
#define FAN_GAME_H

#include "os.h"
#include "platform.h"

#define istagged(t) (t >= 0 ? true : false)
#define TILE_SIZE 64
#define MAX_ENTITY_CAP kilobytes(1)

typedef struct {
    bool32 active;
    uint8 input[256];
    ssize input_size;
    int32 cursor_position;

    uint8 output[20][256];
    int32 output_start;
    int32 output_count;

    uint8 history[10][256];
    int32 history_position;
    int32 history_count;

    // fan_cvar cvars[32];
} GameConsole;

typedef enum {
    MovementFlag_Immovable     = (1 << 0),
    MovementFlag_NoCollision   = (1 << 1),
    MovementFlag_CollideSoftly = (1 << 2),
    MovementFlag_Ghost         = (1 << 3),
} MovementFlags;

typedef struct {
    fan_vec2   position;
    fan_vec2   scale;
    float32    rotation;

    fan_vec2   default_scale;
    float32    default_rotation;

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
    fan_vec2   knockback_force;

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
    fan_vec2 direction;
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

typedef struct {
    fan_str8 buf;
    fan_vec2 origin; // center of draw
    bool32   visible;

    fan_color text_color;
    fan_color background_color;
} CText;

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
    int32 tilemap;
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
    int32     actions[8];
} PlayerInput;

// typedef int32 CQuestion;
typedef struct {
    int32 id;
    bool32 answered;

    float32 timer;
    float32 cooldown_time;
} CQuestion;
/*
 * Basic plan is that when player triggers
 * the 'answer' button, their next directional
 * input is processed as an answer.
 * Depending on the context, this answer affects
 * the surroundings.
 */
typedef enum {
    CAnswer_NONE = 0,
    CAnswer_A,
    CAnswer_B,
    CAnswer_C,
    CAnswer_D,
} CAnswer;

typedef enum {
    CollisionFlag_Immovable   = (1 << 0),
    CollisionFlag_NoCollision = (1 << 1),
    CollisionFlag_MirrorScale = (1 << 2),
    CollisionFlag_Ghost       = (1 << 3),
} CollisionFlags;

typedef struct {
    bool32         active;
    CollisionFlags flags;

    fan_rect       boundary; // NOTE(liam): this is positioned relative to the
                             // entity's transform position.
} CCollision;

typedef struct {
    SystemMode   mode;
    fan_music    music;

    PlayerInput  player_input;
    bool32       player_called_object_dump;

    float64      current_time;
    float32      camera_zoom;

    fan_rtexture rendermap;
    fan_rtexture lightmap;
    fan_rtexture tilemap;

    // TODO(liam): support dynamic window resizing
    // also support minimum window size in platform layer.
    bool32       window_resized;
    int32        window_width;
    int32        window_height;

    fan_vec2     render_size;
    fan_vec2     render_offset;
    float32      render_scale;

    int32        world_scale; // pixels_per_unit
    fan_rect_i32 world_bound_zone;

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
    fan_matrix  logic_tiles;
    fan_matrix  visual_tiles;
    fan_vec2    origin;
    int32       tile_size;
} TileMap;

typedef enum {
    TileID_None = 0,
    TileID_Ground,
} TileID;

typedef struct RenderEntry {
    int32   id;
    float32 height;
    int32   layer;
} RenderEntry;

typedef enum {
    RenderFlag_FlipX        = (1 << 0),
    RenderFlag_FlipY        = (1 << 1),
    RenderFlag_ShowInteract = (1 << 2)
} RenderFlags;

typedef enum {
    AnimationFlag_NotInterruptible = (1 << 0),
    AnimationFlag_DisableLoop      = (1 << 1),
} AnimationFlags;

fan_component_declare(CTransform, CTransform);
fan_component_declare(CShape,     CShape);
fan_component_declare(CMovement,  CMovement);
fan_component_declare(CTexture,   CTexture);

fan_component_declare(CBehavior,  CBehavior);
fan_component_declare(CAnimation, CAnimation);
fan_component_declare(CPhysics,   CPhysics);
fan_component_declare(CSound,     CSound);
fan_component_declare(CLight,     CLight);
fan_component_declare(CText,      CText);

fan_component_declare(CInteraction,  bool32);
fan_component_declare(CInteractable, bool32);
fan_component_declare(CCollision,    CCollision);
fan_component_declare(CAttack,       CAttack);

fan_component_declare(CEnemyTag,      uint8);
fan_component_declare(CBackgroundTag, uint8);

fan_component_declare(CQuestion, CQuestion);
fan_component_declare(CAnswerTag,   CAnswer);

typedef struct {
    fan_arena              arena;
    // fan_freelist           freelist;

    uint8                  entity_count;
    SpecialEntityID        spec_id;

    AnimationData         *anim_table;
    ssize                  anim_table_size;

    fan_asset              assets;

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
    CTextStorage           c_text;

    CInteractionStorage    c_interaction;
    CInteractableStorage   c_interactable;
    CCollisionStorage      c_collision;
    CAttackStorage         c_attack;
    CQuestionStorage       c_question;

    CEnemyTagStorage       c_tag_enemy;
    CBackgroundTagStorage  c_tag_background;
    CAnswerTagStorage      c_tag_answer;
} World;

void GameInit(fan_allocator *a, World *world, GameState *state);
void GameUpdateAndRender(fan_allocator *a, World *world, GameState *state, float32 dt);
void GameClose(fan_allocator *a, World *world, GameState *state);
void GameOnReload(World *world, GameState *state);

#endif // FAN_GAME_H
