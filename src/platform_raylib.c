
#include "platform.h"

#define RAYLIB_IMPLEMENTATION
#include <raylib.h>

#define FanWindowInit           InitWindow
#define FanWindowClose          CloseWindow
#define FanWindowShouldClose    WindowShouldClose

#define FanGetFrameTime         GetFrameTime
#define FanGetTime              GetTime

#define FanRandomSeed           RandomSetSeed
#define FanRandomInt            GetRandomValue

#define FanKeyPressed           IsKeyPressed
#define FanKeyDown              IsKeyDown

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

#define FanDrawBegin            BeginDrawing
#define FanDrawClear            ClearBackground
#define FanDrawFPS              DrawFPS
#define FanDrawEnd              EndDrawing

#define FanDrawPixel            DrawPixel
#define FanDrawPixelV           DrawPixelV

#define FanDrawLine             DrawLine
#define FanDrawLineV            DrawLineV

#define FanDrawRect             DrawRectangle
// #define FanDrawRectV            DrawRectangleV
// #define FanDrawRectR            DrawRectangleRec

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
    Color rl_color = (Color){
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a
    };
    DrawTexturePro(rl_texture, rl_src, rl_dst, rl_origin, angle, rl_color);
}
