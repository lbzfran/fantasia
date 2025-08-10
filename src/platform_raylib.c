
#include "platform.h"

#define RAYLIB_IMPLEMENTATION
#include <raylib.h>
#include <math.h>

Color FanColorToRL(FanColor color) {
    Color rl_color = (Color){
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a
    };

    return rl_color;
}

void FanWindowCreate(int width, int height, const char *title) {
    InitWindow(width, height, title);
}

void FanWindowClose(void) {
    CloseWindow();
}

int FanWindowShouldClose(void) {
    int result = WindowShouldClose();
    return result;
}

void FanSetLogLevel(int level) {
    SetTraceLogLevel(level);
}

int FanWindowWidth(void) {
    int result = GetScreenWidth();
    return result;
}
int FanWindowHeight(void) {
    int result = GetScreenHeight();
    return result;
}

double FanGetFrameTime(void) {
    double result = GetFrameTime();
    return result;
}

double FanGetTime(void) {
    double result = GetTime();
    return result;
}

void FanRandomSeed(int seed) {
    SetRandomSeed(seed);
}

int FanRandomInt(int min, int max) {
    int result = GetRandomValue(min, max);
    return result;
}

int FanKeyPressed(FanKey key) {
    int result = IsKeyPressed(key);
    return result;
}

int FanKeyDown(FanKey key) {
    int result = IsKeyDown(key);
    return result;
}

FanTexture FanTextureLoad(const char *filepath) {
    Texture2D rl_texture = LoadTexture(filepath);
    FanTexture texture = (FanTexture){
        .id = rl_texture.id,
        .width = rl_texture.width,
        .height = rl_texture.height,
        .mipmaps = rl_texture.mipmaps,
        .format = rl_texture.format
    };
    return texture;
}
void FanTextureUnload(FanTexture texture) {
    Texture2D rl_texture = (Texture2D){
        .id = texture.id,
        .width = texture.width,
        .height = texture.height,
        .mipmaps = texture.mipmaps,
        .format = texture.format
    };
    UnloadTexture(rl_texture);
}

void FanDrawBegin(void) {
    BeginDrawing();
}

void FanDrawClear(FanColor color) {
    Color rl_color = FanColorToRL(color);
    ClearBackground(rl_color);
}

void FanDrawFPS(int x, int y) {
    DrawFPS(x, y);
}

void FanDrawEnd(void) {
    EndDrawing();
}

void FanDrawRect(int x, int y, int w, int h, FanColor color) {
    Color rl_color = FanColorToRL(color);

    DrawRectangle(x, y, w, h, rl_color);
}

void FanDrawRectV(FanVector2 pos, FanVector2 scale, FanColor color) {
    FanDrawRect(pos.x, pos.y, scale.x, scale.y, color);
}

void FanDrawRectR(FanRect rect, FanColor color) {
    FanDrawRect(rect.x, rect.y, rect.width, rect.height, color);
}

void FanDrawTexture(FanTexture texture, FanRect src, FanRect dst, FanVector2 origin, float angle, FanColor color) {
    Texture2D rl_texture = (Texture2D){
        .id = texture.id,
        .width = texture.width,
        .height = texture.height,
        .mipmaps = texture.mipmaps,
        .format = texture.format
    };
    Rectangle rl_src = (Rectangle){
        .x = src.x,
        .y = src.y,
        .width = src.width,
        .height = src.height
    };
    Rectangle rl_dst = (Rectangle){
        .x = dst.x,
        .y = dst.y,
        .width = dst.width,
        .height = dst.height
    };
    Vector2 rl_origin = (Vector2){
        .x = origin.x,
        .y = origin.y
    };
    Color rl_color = FanColorToRL(color);
    DrawTexturePro(rl_texture, rl_src, rl_dst, rl_origin, angle, rl_color);
}

void FanCameraBegin(FanCamera2D camera) {
    Camera2D rl_camera = (Camera2D){
        .target = (Vector2){ camera.target.x, camera.target.y },
        .offset = (Vector2){ camera.offset.x, camera.offset.y },
        .rotation = camera.rotation,
        .zoom = camera.zoom
    };

    BeginMode2D(rl_camera);
};

void FanCameraEnd(void) {
    EndMode2D();
}
