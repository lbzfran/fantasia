
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

#define FanTextureLoad          LoadTexture
#define FanTextureUnload        UnloadTexture

#define FanDrawBegin            BeginDrawing
#define FanDrawClear            ClearBackground
#define FanDrawFPS              DrawFPS
#define FanDrawEnd              EndDrawing

#define FanDrawPixel            DrawPixel
#define FanDrawPixelV           DrawPixelV

#define FanDrawLine             DrawLine
#define FanDrawLineV            DrawLineV

#define FanDrawRect             DrawRectangle
#define FanDrawRectV            DrawRectangleV
#define FanDrawRectR            DrawRectangleRec

#define FanDrawTexture          DrawTexturePro
