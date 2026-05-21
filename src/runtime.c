
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

GameConsole console = {};
GameAPI game        = {};
GameState state     = {};
World world         = {};

// TODO(liam): implement these console functions
static void ConsoleOutputAdd(GameConsole *console, fan_str8 text) {
    int32 index = (console->output_start + console->output_count) % CONSOLE_MAX_OUTPUT;
    for (ssize i = 0; i < CONSOLE_MAX_INPUT; i++) {
        console->output[index][i] = text.data[i];
    }
    console->output[index][CONSOLE_MAX_INPUT - 1] = '\0';
    if (console->output_count < CONSOLE_MAX_OUTPUT) {
        console->output_count++;
    }
    else {
        console->output_start = (console->output_start + 1) % CONSOLE_MAX_OUTPUT;
    }
}

static void ConsoleHistoryAdd(GameConsole *console, fan_str8 cmd) {
    if (cmd.length == 0) return;
    if (console->history_count > 0 &&
        fan_str8_equals(fan_str8_cstr(console->history[(console->history_count - 1) % 256]), cmd)) {
        return;
    }
    int32 index = console->history_count % CONSOLE_MAX_INPUT;
    // strncpy(console->history[index], cmd, CONSOLE_MAX_INPUT - 1);
    // console->history[index][CONSOLE_MAX_INPUT - 1] = '\0';
    console->history_count++;
    console->history_position = console->history_count;
}

static void ConsoleExecute(GameConsole *console, fan_str8 cmd) {
    if (cmd.length == 0) return;

    if (fan_str8_equals(cmd, fan_str8_cstr("exit"))) {
        console->active = false;
        return;
    }
    if (fan_str8_equals(cmd, fan_str8_cstr("help"))) {
        ConsoleOutputAdd(console, fan_str8_cstr("Available commands: exit, help"));
        return;
    }
}

static void ConsoleUpdate(GameConsole *console, GameState *state) {
    // if (!console->active) return;
    int32 key = fan_key_current_char();
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && console->cursor_position < (CONSOLE_MAX_INPUT - 1)) {
            fan_memory_move(&console->input[console->cursor_position + 1],
                            &console->input[console->cursor_position],
                            console->input_size - console->cursor_position + 1);
            console->input[console->cursor_position] = (char8)key;
            // console->input[console->cursor_position + 1] = '\0';
            console->input_size++;
            console->cursor_position++;
        }
        key = fan_key_current_char();
    }

    int32 pressed = fan_key_current();
    while (pressed != 0) {
        switch (pressed) {
            case FanKey_ESCAPE: {
                console->input[0] = '\0';
                console->input_size = 0;
                console->cursor_position = 0;
                console->active = not console->active;
            } break;
            case FanKey_BACKSPACE: {
                if (console->cursor_position > 0 && console->input_size > 0) {
                    fan_memory_move(&console->input[console->cursor_position - 1],
                                    &console->input[console->cursor_position],
                                    console->input_size - console->cursor_position + 1);
                    console->input_size--;
                    console->cursor_position--;
                }
            } break;
            case FanKey_DELETE: {
                if (console->input[console->cursor_position] != '\0') {
                    fan_memory_move(&console->input[console->cursor_position],
                                    &console->input[console->cursor_position + 1],
                                    console->input_size - console->cursor_position);
                    console->input_size--;
                }
            } break;
            case FanKey_LEFT: {
                console->cursor_position = max(0, console->cursor_position - 1);
            } break;
            case FanKey_RIGHT: {
                console->cursor_position = min(console->cursor_position + 1, console->input_size);
            } break;
            case FanKey_HOME: {
                console->cursor_position = 0;
            } break;
            case FanKey_END: {
                console->cursor_position = console->input_size;
            } break;
            case FanKey_UP: {
                if (console->history_count > 0) {
                    if (console->history_position > 0) console->history_position--;
                    int32 index = console->history_position % CONSOLE_MAX_INPUT;
                    // strcpy(console->input, console->history[index]);
                    console->cursor_position = console->input_size;
                }
            } break;
            case FanKey_DOWN: {
                if (console->history_count > 0 && console->history_position < console->history_count) {
                    console->history_position++;
                    if (console->history_position == console->history_count) {
                        console->input[0] = '\0';
                    }
                    else {
                        int32 index = console->history_position % CONSOLE_MAX_INPUT;
                        // strcpy(console->input, console->history[index]);
                    }
                    console->cursor_position = console->input_size;
                }
            } break;
            case FanKey_ENTER: {
                if (console->input_size > 0) {
                    ConsoleOutputAdd(console, (fan_str8){ (uint8 *)console->input, console->input_size });
                    ConsoleExecute(console, (fan_str8){ (uint8 *)console->input, console->input_size });
                }
                console->input[0] = '\0';
                console->input_size = 0;
                console->cursor_position = 0;
                console->history_position = console->history_count;
            } break;
            default:
                break;
        }
        pressed = fan_key_current();
    }
}

static void ConsoleDraw(GameConsole *console, int32 width, int32 height) {
    if (!console->active) return;

    int32 console_height = height / 2;
    int32 line_height = 20;
    int32 margin = 10;
    int32 input_y = height - margin - line_height;

    fan_draw_rect(0, 0, width, console_height, (fan_color){ 0, 0, 0, 100 });
    // fan_draw_rect(0, input_y - line_height - margin,
    //               width, line_height + margin * 2, (fan_color){ 0, 0, 0, 150 });
    // fan_draw_rect(0, 0,
    //               width, line_height + margin * 2, (fan_color){ 0, 0, 0, 150 });

    for (int32 i = 0; i < console->output_count && i < CONSOLE_MAX_OUTPUT; i++) {
        int32 index = (console->output_start + i) % CONSOLE_MAX_OUTPUT;
        fan_draw_text(console->output[index], (fan_vec2){ (float32)margin, (float32)margin + (float32)i * line_height }, line_height, fan_color_WHITE);
    }

    fan_draw_text(console->input, (fan_vec2){ (float32)margin, (float32)input_y }, line_height, fan_color_BLACK);

    // char8 prompt[20 + 16];

    int32 cursor_x = margin + fan_text_measure(console->input, line_height) -
        fan_text_measure(console->input + console->cursor_position, line_height);

    fan_draw_rect(cursor_x, input_y, 2, line_height, fan_color_WHITE);
}

static void GameAPIClose(GameAPI *game) {
    fan_lib_close(game->library);
    fan_file_delete(GAME_LIB_TMP_PATH);
    game->library = nullptr;
}

static bool32 GameAPILoad(GameAPI *game) {
    if (game->library != nullptr) {
        GameAPIClose(game);
    }

    if (!fan_file_copy(GAME_LIB_PATH, GAME_LIB_TMP_PATH)) {
        fan_log_error("DEBUGGING Failed to copy game library!\n");
        return false;
    }

    void *library = fan_lib_open(GAME_LIB_TMP_PATH);
    if (library == nullptr) {
        fan_log_error("Failed to load game library!\n");
        return false;
    }

    GameAPI new_game = {
        .library           = library,
        .init              = fan_lib_load(library, "GameInit"),
        .update_and_render = fan_lib_load(library, "GameUpdateAndRender"),
        .close             = fan_lib_load(library, "GameClose"),
        .on_reload         = fan_lib_load(library, "GameOnReload")
    };

    if (!new_game.init || !new_game.update_and_render || !new_game.close || !new_game.on_reload) {
        fan_log_error(": Failed to laod game symbols!\n");
        fan_lib_close(library);
        return false;
    }

    game->library           = new_game.library;
    game->init              = new_game.init;
    game->update_and_render = new_game.update_and_render;
    game->close             = new_game.close;
    game->on_reload         = new_game.on_reload;

    return true;
}

int GameMain(void) {
    if (!GameAPILoad(&game)) {
        return 1;
    }

    fan_window_config(FanWindow_WINDOW_RESIZABLE | FanWindow_VSYNC_HINT);
    fan_window_create(800, 600, "Fantasia");
    fan_dev_audio_create();

    world.arena = (fan_arena){
        .data     = heap_allocator.make(null, megabytes(2)),
        .size     = 0,
        .capacity = megabytes(2)
    };
    fan_allocator arena_allocator = {
        .make   = fan_arena_make,
        .free   = fan_arena_free,
        .resize = fan_arena_resize,
        .ctx    = &world.arena
    };

    // NOTE(liam): testing DSL here!
    // fan_str8 dsl_buf = fan_os_read(&arena_allocator, "./resources/test.dsl");
    // (void)fan_dsl_tokenize(&arena_allocator, dsl_buf);

    bool32 running           = true;
    world.update_entity_split = true;
#ifdef DEBUG
    bool32 requested_reload   = false;
    uint64 last_mod_time      = 0;
#endif

    fan_random_seed(12398);

    fan_camera2D camera = { 0 };
    camera.zoom = 0.8f;
    PlayerInput *p_input = &state.player_input;

    ConsoleOutputAdd(&console, fan_str8_cstr("Test!"));

    game.init(&arena_allocator, &world, &state);
    while (running) {
        float dt = fan_frametime_get();
        if (fan_window_shouldclose()) {
            running = false;
        }

        if (fan_window_resized()) {
            state.window_resized = true;
        }


        p_input->direction = (fan_vec2){ 0 };

        if (console.active) {
            ConsoleUpdate(&console, &state);
        }
        else {
            if (fan_key_pressed(FanKey_ESCAPE)) {
                running = false;
            }

            if (fan_key_pressed(FanKey_GRAVE)) {
                console.active = not console.active;
            }

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

            p_input->actions[0] = fan_key_pressed(FanKey_E);
            p_input->actions[1] = fan_key_pressed(FanKey_R) ? not p_input->actions[1] : p_input->actions[1];
            p_input->actions[2] = fan_key_pressed(FanKey_F) ? not p_input->actions[2] : p_input->actions[2];

            p_input->actions[3] = fan_key_pressed(FanKey_H);
            if (!p_input->actions[3]) p_input->actions[4] = fan_key_pressed(FanKey_J);
            if (!p_input->actions[4]) p_input->actions[5] = fan_key_pressed(FanKey_K);
            if (!p_input->actions[5]) p_input->actions[6] = fan_key_pressed(FanKey_L);

#ifdef DEBUG
            if (fan_key_pressed(FanKey_P)) {
                state.player_called_object_dump = true;
                fan_log_debug("[[START DUMP]]\n");
            }
            if (fan_key_pressed(FanKey_T) ||
                fan_file_time_last_written(GAME_LIB_PATH,
                                           &last_mod_time) == 1) {
                requested_reload = true;
            }
#endif
        }


        state.current_time = fan_time_get();
        CTransform *cam_transform = fan_component_get(&world.c_transform, world.spec_id.camera);
        camera.target = fan_vec2_add(cam_transform->position, fan_vec2_scale(cam_transform->scale, 0.5f));

        fan_draw_begin();
            fan_draw_clear(fan_color_WHITE);
            fan_camera_begin(camera);
            game.update_and_render(&arena_allocator, &world, &state, dt);
            fan_camera_end();
            ConsoleDraw(&console, state.window_width, state.window_height);
            fan_draw_fps(state.window_width - 100, 2);
        fan_draw_end();

#ifdef DEBUG
        if (state.player_called_object_dump) {
            fan_log_debug("Total Allocations: %.2f / %.2f KB\n", (float64)world.arena.size / 1000.0, (float64)world.arena.capacity / 1000.0);
            fan_log_debug("current_time: %.3f\n", state.current_time);

            fan_log_debug("Total Component 'Transform' size/capacity: \t%td/%td\n", world.c_transform.size, world.c_transform.capacity);
            fan_log_debug("Total Component 'Shape' size/capacity:     \t%td/%td\n", world.c_shape.size,     world.c_shape.capacity);
            fan_log_debug("Total Component 'Physics' size/capacity:   \t%td/%td\n", world.c_physics.size,   world.c_physics.capacity);
            fan_log_debug("Total Component 'Texture' size/capacity:   \t%td/%td\n", world.c_texture.size,   world.c_texture.capacity);
            fan_log_debug("Total Component 'Behavior' size/capacity:  \t%td/%td\n", world.c_behavior.size,  world.c_behavior.capacity);
            fan_log_debug("Total Component 'Animation' size/capacity: \t%td/%td\n", world.c_animation.size, world.c_animation.capacity);
            fan_log_debug("Total Component 'Question' size/capacity: \t%td/%td\n", world.c_question.size,   world.c_question.capacity);
            state.player_called_object_dump = false;
        }

        if (requested_reload) {
            fan_os_wait(250);
            fan_log_debug("DEBUG: Reloading!\n");
            if (GameAPILoad(&game)) {
                game.on_reload(&world, &state);
                requested_reload = false;
            }
            else {
                fan_log_error("Failed to Reload!");
                break;
            }
        }
#endif
    }

    game.close(&arena_allocator, &world, &state);
    heap_allocator.free(null, world.arena.data, world.arena.capacity);
    fan_dev_audio_close();
    fan_window_close();
    GameAPIClose(&game);
    return 0;
}
