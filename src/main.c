
#include "os.h"
#include "platform.h"
#include "game.h"

typedef struct {
    void (GAME_API *init)(Allocator *a, World *world);
    void (GAME_API *update_and_render)(Allocator *a, World *world, PlayerInput p_input, float32 dt);
} GameAPI;
GameAPI game = {};

Allocator heap_allocator = {
    .make   = heap_make,
    .free   = heap_free,
    .resize = heap_resize,
    .ctx    = null
};

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

    bool32 running             = true;
    world.update_entity_split  = true;
    FanVector2 player_offset   = FanVector2Zero();
    FanVector2 player_index    = FanVector2Zero();

    FanRandomSeed(12398);

    ssize component_size  = kilobytes(1);

    ComponentStorageCreate(&world.c_transform,      &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_shape,          &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_movement,       &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_texture,        &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_behavior,       &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_animation,      &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_physics,        &arena_allocator, component_size);

    ComponentStorageCreate(&world.c_interaction,    &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_interactable,   &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_zone,           &arena_allocator, component_size);

    ComponentStorageCreate(&world.c_tag_background, &arena_allocator, component_size);
    ComponentStorageCreate(&world.c_tag_enemy,      &arena_allocator, component_size);

    FanCamera2D camera = { 0 };
    camera.zoom = 0.8f;
    PlayerInput p_input = { 0 };

    void *lib = LibOpen("bin/game.so");
    game.init = LibLoad(lib, "GameInit");
    game.update_and_render = LibLoad(lib, "GameUpdateAndRender");

    game.init(&arena_allocator, &world);
    while (running) {
        float dt = FanGetFrameTime();
        if (FanWindowShouldClose() || FanKeyPressed(FanKey_ESCAPE)) {
            running = false;
        }

        int32 player_animation_id = -1;
        p_input.direction = (FanVector2){ 0 };
        if (FanKeyDown(FanKey_W)) {
            p_input.direction.y -= 1;
            player_animation_id = 1;
        }
        if (FanKeyDown(FanKey_S)) {
            p_input.direction.y += 1;
            player_animation_id = 0;
        }
        if (FanKeyDown(FanKey_A)) {
            p_input.direction.x -= 1;
            player_animation_id = 2;
        }
        if (FanKeyDown(FanKey_D)) {
            p_input.direction.x += 1;
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

        if (FanKeyDown(FanKey_E)) {
            p_input.actions[0] = true;
        }
        else {
            p_input.actions[0] = false;
        }
        if (FanKeyPressed(FanKey_R)) {
            p_input.actions[1] = not p_input.actions[1];
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
            world.called_object_dump = true;
            printf("[[DEBUG INFO]]\n");
        }

        world.bounding_zone = (FanRect){ .width = FanWindowWidth(), .height = FanWindowHeight() };
        world.current_time = FanGetTime();
        int32 cam_move_idx = world.c_transform.sparse[world.spec_id.camera];
        CTransform *cam_transform = &world.c_transform.data[cam_move_idx];
        camera.target = FanVector2Add(cam_transform->position, FanVector2Scale(cam_transform->scale, 0.5f));
        camera.offset = (FanVector2){ FanWindowWidth() / 2.0f, FanWindowHeight() / 2.0f };

        if (world.called_object_dump) {
            printf("Total Allocations: %.2f / %.2f KB\n", (double)world.arena.size / 1000.0f, (double)world.arena.capacity / 1000.0f);
            printf("current_time: %.3f\n", world.current_time);

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
            game.update_and_render(&arena_allocator, &world, p_input, dt);
            FanCameraEnd();
            FanDrawFPS(2, 2);
        FanDrawEnd();
        world.called_object_dump = false;
        world.update_entity_split = false;
    }

    FanWindowClose();
    LibClose(lib);
    heap_allocator.free(null, world.arena.data, world.arena.capacity);
    return 0;
}
