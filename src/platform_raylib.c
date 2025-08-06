
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

float FanLerp(float a, float t, float b) {
    return  a + (b - a) * t;
}

int FanFloatEquals(float x, float y) {
    float epsilon = 0.000001f;
    int result = (fabsf(x - y)) <= (epsilon*fmaxf(1.0f, fmaxf(fabsf(x), fabsf(y))));
    return result;
}

FanVector2 FanVector2Zero(void) {
    return (FanVector2){ 0.0f, 0.0f };
}
FanVector2 FanVector2One(void) {
    return (FanVector2){ 1.0f, 1.0f };
}

FanVector2 FanVector2Add(FanVector2 v1, FanVector2 v2) {
    return (FanVector2){ v1.x + v2.x, v1.y + v2.y };
}

FanVector2 FanVector2AddValue(FanVector2 v, float x) {
    return (FanVector2){ v.x + x, v.y + x };
}

FanVector2 FanVector2Sub(FanVector2 v1, FanVector2 v2) {
    return (FanVector2){ v1.x - v2.x, v1.y - v2.y };
}
FanVector2 FanVector2SubValue(FanVector2 v, float x) {
    return (FanVector2){ v.x - x, v.y - x };
}

FanVector2 FanVector2Normalize(FanVector2 v) {
    float magnitude = FanVector2Length(v);

    FanVector2 result = FanVector2Zero();
    if (!FanFloatEquals(magnitude, 0.0f)) {
        result = (FanVector2){ v.x / magnitude, v.y / magnitude };
    }
    return result;
}

float FanVector2Length(FanVector2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}
float FanVector2LengthSqr(FanVector2 v) {
    return v.x * v.x + v.y * v.y;
}

FanVector2 FanVector2Scale(FanVector2 v, float scale) {
    return (FanVector2){ v.x * scale, v.y * scale };
}
FanVector2 FanVector2Negate(FanVector2 v) {
    return (FanVector2){ -v.x, -v.y };
}
