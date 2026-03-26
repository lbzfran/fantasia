/*
 *
 * purpose: this file focuses on functions that combine components together.
 * this could also
 *
 */
#include "game.h"
#include "os.h"
#include "platform.h"

static inline int32 SpawnPlayer(World *world, fan_vec2 position, fan_vec2 direction) {
    fan_texture tex_girl_01 = fan_texture_load("./resources/Citizens/Female/Hana/Hana.png");
    fan_vec2 citizen_size = (fan_vec2){ (float32)16.0f, (float32)16.0f };

    fan_component_add(&world->c_transform, world->entity_count,
                     .position = position);
    fan_component_add(&world->c_shape,         world->entity_count);
    fan_component_add(&world->c_movement,  world->entity_count,
        .direction = direction
        // .flags = MovementFlag_CollideSoftly
    );
    fan_component_add(&world->c_texture,   world->entity_count,
        .texture = tex_girl_01,
        // .rect = player_idle_down_frames[0],
        // .rect = (fan_rect_i32){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
        .rect = { .x = 0, .y = 0, .w = citizen_size.x, .h = citizen_size.y },
        // .rect = { .width = 36, .height = 36 },
    );
    fan_component_add(&world->c_animation, world->entity_count);
    fan_component_add(&world->c_interaction,  world->entity_count);
    fan_component_add(&world->c_interactable, world->entity_count);
    fan_component_add(&world->c_attack,   world->entity_count,
        .arc_angle    = fan_f32_rad(45.0f),
        .swing_time   = 0.2f,
        .knockback    = 1.5f,
        .attack_range = 2.0f,
    );
    fan_component_add(&world->c_light,     world->entity_count,
        .color  = (fan_color){ 170, 170, 170, 170 },
        .radius = 200.0f,
    );
    fan_component_add(&world->c_magic, world->entity_count);

    world->spec_id.player = world->entity_count;
    world->entity_count++;
    world->update_entity_split = true;

    return world->spec_id.player;
}
// TODO(liam):
// - Transform scales less than 1 have scaling issues with their hitbox.
// - Having less components causes issues with interactivity (likely a bounds error during iterations).
static inline void SpawnBullet(World *world, fan_vec2 position, fan_vec2 direction) {
    // TODO(liam): dynamically added entity not properly initializing.
    fan_component_add(&world->c_transform, world->entity_count,
        .position = position,
        .scale = fan_vec2_one()
    );
    fan_component_add(&world->c_shape, world->entity_count,
        .color = (fan_color){ 50, 50, 50, 255 },
    );
    fan_component_add(&world->c_movement,  world->entity_count,
        .flags = MovementFlag_Ghost
    );
    fan_component_add(&world->c_behavior, world->entity_count,
        .target_id = world->spec_id.player,
        .type      = BehaviorType_Follow
    );
    fan_component_add(&world->c_interaction,  world->entity_count);
    fan_component_add(&world->c_interactable, world->entity_count);
    // TODO(liam): add a lifetime component
    // fan_component_add(&world->c_life, world->entity_count,
    //     .time = 5.0f
    // );

    world->entity_count++;

    // TODO(liam): call to update entity split not respected.
    world->update_entity_split = true;
}

void SceneSolo(World *world) {
    fan_texture tex_link = fan_texture_load("./resources/link.png");
    // fan_vec2 sprite_link_size = (fan_vec2){ tex_link.width / 10.0f, tex_link.height / 8.0f };
    // player_idle_down_frames[0]  = (fan_rect_i32){ 0,                         0,                         0, 0 };
    // player_idle_down_frames[1]  = (fan_rect_i32){ sprite_link_size.x,        0,                         0, 0 };
    // player_idle_down_frames[2]  = (fan_rect_i32){ 2.0f * sprite_link_size.x, 0,                         0, 0 };
    //
    // player_idle_up_frames[0]    = (fan_rect_i32){ 0,                         2.0f * sprite_link_size.y, 0, 0 };
    //
    // player_idle_left_frames[0]  = (fan_rect_i32){ 0,                         sprite_link_size.y,        0, 0 };
    // player_idle_left_frames[1]  = (fan_rect_i32){ sprite_link_size.x,        sprite_link_size.y,        0, 0 };
    // player_idle_left_frames[2]  = (fan_rect_i32){ 2.0f * sprite_link_size.x, sprite_link_size.y,        0, 0 };
    //
    // player_idle_right_frames[0] = (fan_rect_i32){ 0,                         3.0f * sprite_link_size.y, 0, 0 };
    // player_idle_right_frames[1] = (fan_rect_i32){ sprite_link_size.x,        3.0f * sprite_link_size.y, 0, 0 };
    // player_idle_right_frames[2] = (fan_rect_i32){ 2.0f * sprite_link_size.x, 3.0f * sprite_link_size.y, 0, 0 };


    fan_component_add(&world->c_transform,     world->entity_count);
    fan_component_add(&world->c_shape,         world->entity_count);
    fan_component_add(&world->c_movement,  world->entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    fan_component_add(&world->c_texture,   world->entity_count,
        .texture = tex_link,
        .rect    = { 0, 0, (float32)tex_link.width / 10.0f, (float32)tex_link.height / 8.0f }
    );
    // fan_component_add(&world->c_animation, world->entity_count);
    world->spec_id.player = world->entity_count;
    world->entity_count++;

    // fan_component_add(&world->c_transform,  world->entity_count,
    //     .scale = (fan_vec2){ (float32)render_width, (float32)render_height },
    // );
    fan_component_add(&world->c_shape,      world->entity_count,
        .layer = 1,
        .color = (fan_color){ 155, 155, 155, 255 },
    );
    fan_component_add(&world->c_movement,   world->entity_count,
        .flags = MovementFlag_NoCollision,
    );
    fan_component_add(&world->c_tag_background, world->entity_count);
    world->entity_count++;
}

global void SceneMain(World *world) {
    fan_texture tex_sprite = fan_texture_load("./resources/Sprite-0001.png");

    fan_texture tex_girl_01 = fan_texture_load("./resources/Citizens/Female/Hana/Hana.png");
    fan_texture tex_girl_02 = fan_texture_load("./resources/Citizens/Female/Khali/Khali.png");
    fan_texture tex_girl_03 = fan_texture_load("./resources/Citizens/Male/Artun/Artun.png");

    world->tilesets[0] = fan_texture_load("./resources/tileset_01.png");
    // TODO(liam): initialize a basic tilemap.
    // world->map.tiles = ...;
    // fan_matrix_fill(world->map.logic_tiles, 0);
    fan_matrix_randomize(world->map.logic_tiles, 0, 1);
    fan_matrix_fill(world->map.visual_tiles, 0);

    global fan_rect player_idle_down_frames[4]  = { 0 };
    global fan_rect player_idle_up_frames[4]    = { 0 };
    global fan_rect player_idle_left_frames[4]  = { 0 };
    global fan_rect player_idle_right_frames[4] = { 0 };

    global fan_rect player_walk_down_frames[4]  = { 0 };
    global fan_rect player_walk_up_frames[4]    = { 0 };
    global fan_rect player_walk_left_frames[4]  = { 0 };
    global fan_rect player_walk_right_frames[4] = { 0 };

    fan_vec2 citizen_size = (fan_vec2){ (float32)16.0f, (float32)16.0f };

    player_idle_left_frames[0]  = (fan_rect){
        0,                           0, citizen_size.x, citizen_size.y
    };
    player_idle_left_frames[1]  = (fan_rect){
        citizen_size.x,          0, citizen_size.x, citizen_size.y
    };
    player_idle_left_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), 0, citizen_size.x, citizen_size.y
    };
    player_idle_left_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), 0, citizen_size.x, citizen_size.y
    };

    player_idle_down_frames[0]  = (fan_rect){
        0,                           (2.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_down_frames[1]  = (fan_rect){
        citizen_size.x,          (2.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_down_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), (2.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_down_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), (2.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    player_idle_right_frames[0]  = (fan_rect){
        0,                           citizen_size.y, citizen_size.x, citizen_size.y
    };
    player_idle_right_frames[1]  = (fan_rect){
        citizen_size.x,          citizen_size.y, citizen_size.x, citizen_size.y
    };
    player_idle_right_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), citizen_size.y, citizen_size.x, citizen_size.y
    };
    player_idle_right_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), citizen_size.y, citizen_size.x, citizen_size.y
    };

    player_idle_up_frames[0] = (fan_rect){
        0,                           (3.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_up_frames[1] = (fan_rect){
        citizen_size.x,          (3.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_up_frames[2] = (fan_rect){
        (2.0f * citizen_size.x), (3.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_up_frames[3] = (fan_rect){
        (3.0f * citizen_size.x), (3.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    // WALK

    player_walk_left_frames[0]  = (fan_rect){
        0,                           (4.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_left_frames[1]  = (fan_rect){
        citizen_size.x,          (4.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_left_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), (4.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_left_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), (4.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    player_walk_down_frames[0]  = (fan_rect){
        0,                           (6.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_down_frames[1]  = (fan_rect){
        citizen_size.x,          (6.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_down_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), (6.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_down_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), (6.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    player_walk_right_frames[0]  = (fan_rect){
        0,                           (5.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_right_frames[1]  = (fan_rect){
        citizen_size.x,          (5.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_right_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), (5.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_right_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), (5.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    player_walk_up_frames[0] = (fan_rect){
        0,                           (7.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_up_frames[1] = (fan_rect){
        citizen_size.x,          (7.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_up_frames[2] = (fan_rect){
        (2.0f * citizen_size.x), (7.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_up_frames[3] = (fan_rect){
        (3.0f * citizen_size.x), (7.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };


    global AnimationData anim_table[] = {
        { "player_idle_up",    player_idle_up_frames,    .frame_time = 0.4f, .frame_count = 4, true, -1 },
        { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.4f, .frame_count = 4, true, -1 },
        { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.4f, .frame_count = 4, true, -1 },
        { "player_idle_right", player_idle_right_frames, .frame_time = 0.4f, .frame_count = 4, true, -1 },
        { "player_walk_up",    player_walk_up_frames,    .frame_time = 0.2f, .frame_count = 4, true, -1 },
        { "player_walk_down",  player_walk_down_frames,  .frame_time = 0.2f, .frame_count = 4, true, -1 },
        { "player_walk_left",  player_walk_left_frames,  .frame_time = 0.2f, .frame_count = 4, true, -1 },
        { "player_walk_right", player_walk_right_frames, .frame_time = 0.2f, .frame_count = 4, true, -1 },
    };

    world->anim_table = anim_table;

    // non-visual that keeps track of what goes where
    // global TileVisual world_grid[50] = { 0 };
    // visual that is offset by half a tile and determines
    // what tile to render based on its four neighbors from
    // the world grid.

    SpawnPlayer(world, fan_vec2_zero(), fan_vec2_one());

    fan_component_add(&world->c_transform,    world->entity_count);
    fan_component_add(&world->c_movement, world->entity_count,
        .speed = 5.0f,
        .flags = MovementFlag_NoCollision,
    );
    fan_component_add(&world->c_behavior, world->entity_count,
        .type = BehaviorType_Follow,
        .target_id = world->spec_id.player,
    );
    world->spec_id.camera = world->entity_count;
    world->entity_count++;

    // fan_texture tex_mewee = fan_texture_load("./resources/mewee.png");
    fan_component_add(&world->c_transform,    world->entity_count);
    fan_component_add(&world->c_shape,    world->entity_count,
        .color = (fan_color){ 50, 255, 255, 255 },
    );
    fan_component_add(&world->c_movement, world->entity_count, .speed = 1.0f);
    fan_component_add(&world->c_texture,  world->entity_count,
        .texture = tex_girl_02,
        .rect = { .x = 0, .y = 0, .w = citizen_size.x, .h = citizen_size.y },
    );
    fan_component_add(&world->c_animation, world->entity_count);
    fan_component_add(&world->c_behavior, world->entity_count,
        .type = BehaviorType_Random,
        .update_time = 60.0f,
    );
    fan_component_add(&world->c_interaction,  world->entity_count);
    fan_component_add(&world->c_interactable, world->entity_count);
    // fan_component_add(&world->c_zone,     world->entity_count,
    //     .x = 0, .y = 0, .width = 1, .height = 1,
    // );
    fan_component_add(&world->c_tag_enemy,    world->entity_count);
    world->entity_count++;

    // fan_component_add(&world->c_transform,    world->entity_count);
    // fan_component_add(&world->c_shape,    world->entity_count,
        // .color = (fan_color){ 255, 50, 255, 255 },
    // );
    // fan_component_add(&world->c_movement, world->entity_count, .speed = 2.0f);
    // fan_component_add(&world->c_texture,  world->entity_count,
    //     .texture = tex_girl_03,
    //     .rect = { .x = 0, .y = 0, .width = citizen_size.x, .height = citizen_size.y },
    // );
    // fan_component_add(&world->c_animation, world->entity_count);
    // fan_component_add(&world->c_behavior, world->entity_count,
    //     .type = BehaviorType_Follow,
    //     .target_id = world->spec_id.player,
    // );
    // fan_component_add(&world->c_interaction,  world->entity_count);
    // fan_component_add(&world->c_interactable, world->entity_count);
    // fan_component_add(&world->c_tag_enemy,    world->entity_count);
    // world->entity_count++;
    //
    fan_component_add(&world->c_transform,  world->entity_count,
        .position = (fan_vec2){  0, 5 },
        .scale    = (fan_vec2){ 10, 6 },
    );
    fan_component_add(&world->c_shape,      world->entity_count,
        .layer = 1,
        .color = (fan_color){ 155, 155, 155, 255 },
    );
    fan_component_add(&world->c_movement,   world->entity_count,
        .flags = MovementFlag_NoCollision,
    );
    fan_component_add(&world->c_tag_background, world->entity_count);
    world->spec_id.tilemap = world->entity_count;
    world->entity_count++;
    //
    // fan_component_add(&world->c_transform, world->entity_count,
    //     .position = (fan_vec2){ 200.0f, 300.0f },
    //     .scale = (fan_vec2){ 400.0f, 150.0f }
    // );
    // fan_component_add(&world->c_shape,     world->entity_count,
    //     .color = (fan_color){ 200, 165, 175, 255 },
    //     .layer = 3,
    // );
    // fan_component_add(&world->c_movement,  world->entity_count,
    //     .flags = MovementFlag_NoCollision,
    // );
    // world->entity_count++;

    // fan_component_add(&world->c_transform, world->entity_count,
    //     .position = (fan_vec2){ 200, 100 },
    // );
    // fan_component_add(&world->c_shape,     world->entity_count,
    //     .color = (fan_color){ 50, 255, 255, 255 },
    // );
    // fan_component_add(&world->c_movement,  world->entity_count,
    //     .flags = MovementFlag_Immovable,
    // );
    // world->entity_count++;

    // fan_component_add(&world->c_transform, world->entity_count,
    //     .position = (fan_vec2){ 5, 2 },
    // );
    // fan_component_add(&world->c_shape,     world->entity_count,
    //     .color = (fan_color){ 50, 255, 255, 255 },
    // );
    // fan_component_add(&world->c_movement,  world->entity_count,
    //     .flags = MovementFlag_Immovable,
    // );
    // world->entity_count++;
    //
    // fan_component_add(&world->c_transform, world->entity_count,
    //     .position = (fan_vec2){ 5, 3 },
    // );
    // fan_component_add(&world->c_movement,  world->entity_count);
    // fan_component_add(&world->c_shape, world->entity_count,
    //     .color = (fan_color){ 50, 255, 50, 255 },
    // );
    // world->entity_count++;

    // SpawnBullet(world, fan_vec2_zero(), fan_vec2_one());
}
