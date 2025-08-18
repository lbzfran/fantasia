
#include "os.h"
#include "platform.h"
#include "game.h"

typedef struct {
    void (GAME_API *init)(Allocator *a, World *world, GameState *state);
    void (GAME_API *update_and_render)(Allocator *a, World *world, GameState *state, float32 dt);
    void (GAME_API *close)(Allocator *a, World *world, GameState *state);
} GameAPI;
GameAPI game = {};

Allocator heap_allocator = {
    .make   = heap_make,
    .free   = heap_free,
    .resize = heap_resize,
    .ctx    = null
};

GameState state = {};
World world = {};

int main(void) {
    FanWindowCreate(800, 600, "Fantasia");
    FanAudioDevCreate();

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
    world.update_entity_split  = true;
    FanVector2 player_offset   = FanVector2Zero();
    FanVector2 player_index    = FanVector2Zero();

    FanRandomSeed(12398);

    FanCamera2D camera = { 0 };
    camera.zoom = 0.8f;
    PlayerInput *p_input = &state.p_input;

    void *lib = LibOpen(GAME_LIB_PATH);
    game.init = LibLoad(lib, "GameInit");
    game.update_and_render = LibLoad(lib, "GameUpdateAndRender");
    game.close = LibLoad(lib, "GameClose");

    game.init(&arena_allocator, &world, &state);
    while (running) {
        float dt = FanGetFrameTime();
        if (FanWindowShouldClose() || FanKeyPressed(FanKey_ESCAPE)) {
            running = false;
        }

        p_input->direction = (FanVector2){ 0 };
        if (FanKeyDown(FanKey_W)) {
            p_input->direction.y += 1;
        }
        if (FanKeyDown(FanKey_S)) {
            p_input->direction.y -= 1;
        }
        if (FanKeyDown(FanKey_A)) {
            p_input->direction.x -= 1;
        }
        if (FanKeyDown(FanKey_D)) {
            p_input->direction.x += 1;
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

        if (FanKeyPressed(FanKey_E)) {
            p_input->actions[0] = true;
        }
        else {
            p_input->actions[0] = false;
        }
        if (FanKeyPressed(FanKey_R)) {
            p_input->actions[1] = not p_input->actions[1];
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
            state.called_object_dump = true;
            printf("[[DEBUG INFO]]\n");
        }

        state.current_time = FanGetTime();
        int32 cam_move_idx = world.c_transform.sparse[world.spec_id.camera];
        CTransform *cam_transform = &world.c_transform.data[cam_move_idx];
        camera.target = FanVector2Add(cam_transform->position, FanVector2Scale(cam_transform->scale, 0.5f));
        // camera.offset = (FanVector2){ FanWindowWidth() / 2.0f, FanWindowHeight() / 2.0f };

        if (state.called_object_dump) {
            printf("Total Allocations: %.2f / %.2f KB\n", (double)world.arena.size / 1000.0f, (double)world.arena.capacity / 1000.0f);
            printf("current_time: %.3f\n", state.current_time);

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
            game.update_and_render(&arena_allocator, &world, &state, dt);
            FanCameraEnd();
            FanDrawFPS(2, 2);
        FanDrawEnd();
        state.called_object_dump = false;
        world.update_entity_split = false;
    }

    FanAudioDevClose();
    FanWindowClose();
    LibClose(lib);
    game.close(&arena_allocator, &world, &state);
    heap_allocator.free(null, world.arena.data, world.arena.capacity);
    return 0;
}
