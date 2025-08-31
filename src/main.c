
#include "os.h"
#include "platform.h"
#include "game.h"

typedef struct {
    void (*init)(Allocator *a, World *world, GameState *state);
    void (*update_and_render)(Allocator *a, World *world, GameState *state, float32 dt);
    void (*close)(Allocator *a, World *world, GameState *state);
} GameAPI;
GameAPI game = {};

Allocator heap_allocator = {
    .make   = fan_heap_make,
    .free   = fan_heap_free,
    .resize = fan_heap_resize,
    .ctx    = null
};

GameState state = {};
World world = {};

int main(void) {
    void *lib = fan_lib_open("bin/libgame.so");
    if ((uintptr)lib == null) {
        printf("ERROR: Failed to load game library!\n");
        return 1;
    }
    game.init = fan_lib_load(lib, "GameInit");
    game.update_and_render = fan_lib_load(lib, "GameUpdateAndRender");
    game.close = fan_lib_load(lib, "GameClose");

    fan_window_create(800, 600, "Fantasia");
    fan_dev_audio_create();

    world.arena = (Arena){
        .data     = heap_allocator.make(null, megabytes(1)),
        .size     = 0,
        .capacity = megabytes(1)
    };
    Allocator arena_allocator = {
        .make   = fan_arena_make,
        .free   = fan_arena_free,
        .resize = fan_arena_resize,
        .ctx    = &world.arena
    };

    bool32 running             = true;
    world.update_entity_split  = true;
    fan_vec2 player_offset   = fan_vec2_zero();
    fan_vec2 player_index    = fan_vec2_zero();

    fan_random_seed(12398);

    fan_camera2D camera = { 0 };
    camera.zoom = 0.8f;
    PlayerInput *p_input = &state.p_input;

    game.init(&arena_allocator, &world, &state);
    while (running) {
        float dt = fan_frametime_get();
        if (fan_window_shouldclose() || fan_key_pressed(FanKey_ESCAPE)) {
            running = false;
        }

        p_input->direction = (fan_vec2){ 0 };
        if (fan_key_down(FanKey_W)) {
            p_input->direction.y += 1;
        }
        if (fan_key_down(FanKey_S)) {
            p_input->direction.y -= 1;
        }
        if (fan_key_down(FanKey_A)) {
            p_input->direction.x -= 1;
        }
        if (fan_key_down(FanKey_D)) {
            p_input->direction.x += 1;
        }

        if (fan_key_down(FanKey_K)) {
            player_offset.y += 500.0f * dt;
            if (player_offset.y >= 200.0f) {
                player_offset.y = 200.0f;
            }
        }
        if (fan_key_down(FanKey_I)) {
            player_offset.y -= 500.0f * dt;
            if (player_offset.y <= -200.0f) {
                player_offset.y = -200.0f;
            }
        }
        if (fan_key_down(FanKey_L)) {
            player_offset.x += 500.0f * dt;
            if (player_offset.x >= 200.0f) {
                player_offset.x = 200.0f;
            }
        }
        if (fan_key_down(FanKey_J)) {
            player_offset.x -= 500.0f * dt;
            if (player_offset.x <= -200.0f) {
                player_offset.x = -200.0f;
            }
        }

        if (fan_key_pressed(FanKey_E)) {
            p_input->actions[0] = true;
        }
        else {
            p_input->actions[0] = false;
        }
        if (fan_key_pressed(FanKey_R)) {
            p_input->actions[1] = not p_input->actions[1];
        }

        if (fan_key_down(FanKey_O)) {
            player_offset = fan_vec2_zero();
        }

        if (fan_key_pressed(FanKey_V)) {
            player_index.x -= 1;
        }
        if (fan_key_pressed(FanKey_B)) {
            player_index.x += 1;
        }
        if (fan_key_pressed(FanKey_N)) {
            player_index.y -= 1;
        }
        if (fan_key_pressed(FanKey_M)) {
            player_index.y += 1;
        }

        if (fan_key_pressed(FanKey_P)) {
            state.called_object_dump = true;
            printf("[[DEBUG INFO]]\n");
        }

        state.current_time = fan_time_get();
        int32 cam_move_idx = world.c_transform.sparse[world.spec_id.camera];
        CTransform *cam_transform = &world.c_transform.data[cam_move_idx];
        camera.target = fan_vec2_add(cam_transform->position, fan_vec2_scale(cam_transform->scale, 0.5f));
        // camera.offset = (fan_vec2){ FanWindowWidth() / 2.0f, FanWindowHeight() / 2.0f };

        if (state.called_object_dump) {
            printf("Total Allocations: %.2f / %.2f KB\n", (float64)world.arena.size / 1000.0, (float64)world.arena.capacity / 1000.0);
            printf("current_time: %.3f\n", state.current_time);

            printf("Total Component 'Transform' size/capacity: \t%td/%td\n", world.c_transform.size, world.c_transform.capacity);
            printf("Total Component 'Shape' size/capacity:     \t%td/%td\n", world.c_shape.size,     world.c_shape.capacity);
            printf("Total Component 'Physics' size/capacity:   \t%td/%td\n", world.c_physics.size,   world.c_physics.capacity);
            printf("Total Component 'Texture' size/capacity:   \t%td/%td\n", world.c_texture.size,   world.c_texture.capacity);
            printf("Total Component 'Behavior' size/capacity:  \t%td/%td\n", world.c_behavior.size,  world.c_behavior.capacity);
            printf("Total Component 'Animation' size/capacity: \t%td/%td\n", world.c_animation.size, world.c_animation.capacity);
        }

        fan_draw_begin();
            fan_draw_clear(fan_color_WHITE);
            fan_camera_begin(camera);
            game.update_and_render(&arena_allocator, &world, &state, dt);
            fan_camera_end();
            fan_draw_fps(2, 2);
        fan_draw_end();
        state.called_object_dump = false;
        world.update_entity_split = false;
    }

    game.close(&arena_allocator, &world, &state);
    heap_allocator.free(null, world.arena.data, world.arena.capacity);
    fan_dev_audio_close();
    fan_window_close();
    fan_lib_close(lib);
    return 0;
}
