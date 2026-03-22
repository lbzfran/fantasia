
#include "os.h"
#include "platform.h"
#include "game.h"

typedef struct {
    void (*init)(fan_allocator *a, World *world, GameState *state);
    void (*update_and_render)(fan_allocator *a, World *world, GameState *state, float32 dt);
    void (*close)(fan_allocator *a, World *world, GameState *state);
    void (*on_reload)(World *world, GameState *state);

    void *library;
} GameAPI;

fan_allocator heap_allocator = {
    .make   = fan_heap_make,
    .free   = fan_heap_free,
    .resize = fan_heap_resize,
    .ctx    = null
};

GameAPI game = {};
GameState state = {};
World world = {};

void GameAPIClose(GameAPI *game) {
    fan_lib_close(game->library);
    fan_os_file_delete(GAME_LIB_TMP_PATH);
}

bool32 GameAPILoad(GameAPI *game) {
    if (game->library != nullptr) {
        GameAPIClose(game);
        game->library = nullptr;
    }

    if (!fan_os_file_copy(GAME_LIB_PATH, GAME_LIB_TMP_PATH)) {
        printf("ERROR: DEBUGGING Failed to copy game library!\n");
        return false;
    }

    void *library = fan_lib_open(GAME_LIB_TMP_PATH);
    if (library == nullptr) {
        printf("ERROR: Failed to load game library!\n");
        return false;
    }

    GameAPI new_game = {
        .library = library,
        .init = fan_lib_load(library, "GameInit"),
        .update_and_render = fan_lib_load(library, "GameUpdateAndRender"),
        .close = fan_lib_load(library, "GameClose"),
        .on_reload = fan_lib_load(library, "GameOnReload")
    };

    if (!new_game.init || !new_game.update_and_render || !new_game.close || !new_game.on_reload) {
        printf("ERROR: Failed to laod game symbols!\n");
        fan_lib_close(library);
        return false;
    }

    // void *old_library       = game->library;
    game->library           = new_game.library;
    game->init              = new_game.init;
    game->update_and_render = new_game.update_and_render;
    game->close             = new_game.close;
    game->on_reload         = new_game.on_reload;

    // if (old_library != nullptr) {
    //     fan_lib_close(old_library);
    // }

    return true;
}

int main(void) {
    if (!GameAPILoad(&game)) {
        return 1;
    }

    fan_window_config(FanWindow_WINDOW_RESIZABLE | FanWindow_VSYNC_HINT);
    fan_window_create(800, 600, "Fantasia");
    fan_dev_audio_create();

    world.arena = (Arena){
        .data     = heap_allocator.make(null, megabytes(1)),
        .size     = 0,
        .capacity = megabytes(1)
    };
    fan_allocator arena_allocator = {
        .make   = fan_arena_make,
        .free   = fan_arena_free,
        .resize = fan_arena_resize,
        .ctx    = &world.arena
    };

    bool32 running             = true;
    world.update_entity_split  = true;
    bool32 requested_reload    = false;
    uint64 last_mod_time       = 0;
    fan_vec2 player_offset   = fan_vec2_zero();
    fan_vec2 player_index    = fan_vec2_zero();

    fan_random_seed(12398);

    fan_camera2D camera = { 0 };
    camera.zoom = 0.8f;
    PlayerInput *p_input = &state.player_input;

    game.init(&arena_allocator, &world, &state);
    while (running) {
        float dt = fan_frametime_get();
        if (fan_window_shouldclose() || fan_key_pressed(FanKey_ESCAPE)) {
            running = false;
        }

        if (fan_window_resized()) {
            state.window_resized = true;
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

        // if (fan_key_down(FanKey_K)) {
        //     player_offset.y += 500.0f * dt;
        //     if (player_offset.y >= 200.0f) {
        //         player_offset.y = 200.0f;
        //     }
        // }
        // if (fan_key_down(FanKey_I)) {
        //     player_offset.y -= 500.0f * dt;
        //     if (player_offset.y <= -200.0f) {
        //         player_offset.y = -200.0f;
        //     }
        // }
        // if (fan_key_down(FanKey_L)) {
        //     player_offset.x += 500.0f * dt;
        //     if (player_offset.x >= 200.0f) {
        //         player_offset.x = 200.0f;
        //     }
        // }
        // if (fan_key_down(FanKey_J)) {
        //     player_offset.x -= 500.0f * dt;
        //     if (player_offset.x <= -200.0f) {
        //         player_offset.x = -200.0f;
        //     }
        // }

        p_input->actions[0] = fan_key_pressed(FanKey_E);
        p_input->actions[1] = fan_key_pressed(FanKey_R) ? not p_input->actions[1] : p_input->actions[1];
        p_input->actions[2] = fan_key_pressed(FanKey_J);
        p_input->actions[3] = fan_key_pressed(FanKey_K);
        p_input->actions[4] = fan_key_pressed(FanKey_L);
        p_input->actions[5] = fan_key_pressed(FanKey_U);
        p_input->actions[6] = fan_key_pressed(FanKey_I);
        p_input->actions[7] = fan_key_pressed(FanKey_O);

        // if (fan_key_down(FanKey_O)) {
        //     player_offset = fan_vec2_zero();
        // }

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
            state.player_called_object_dump = true;
            printf("[[DEBUG INFO]]\n");
        }

        if (fan_key_pressed(FanKey_T) ||
            fan_os_file_time_last_written(GAME_LIB_PATH,
                                         &last_mod_time) == 1) {
            requested_reload = true;
        }

        state.current_time = fan_time_get();
        // int32 cam_move_idx = world.c_transform.sparse[world.spec_id.camera];
        CTransform *cam_transform = ComponentGet(&world.c_transform, world.spec_id.camera);
        camera.target = fan_vec2_add(cam_transform->position, fan_vec2_scale(cam_transform->scale, 0.5f));
        // camera.offset = (fan_vec2){ FanWindowWidth() / 2.0f, FanWindowHeight() / 2.0f };

        if (state.player_called_object_dump) {
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

        // printf("DEBUG: MS IS: %zu\n", last_mod_time);
        if (requested_reload) {
            fan_os_wait(250);
            printf("DEBUG: Reloading!\n");
            if (GameAPILoad(&game)) {
                game.on_reload(&world, &state);
                requested_reload = false;
            }
            else {
                printf("Failed to Reload!");
                break;
            }
        }

        state.player_called_object_dump = false;
        world.update_entity_split = false;
    }

    game.close(&arena_allocator, &world, &state);
    heap_allocator.free(null, world.arena.data, world.arena.capacity);
    fan_dev_audio_close();
    fan_window_close();
    GameAPIClose(&game);
    return 0;
}
