
#include "platform.h"
#define FAN_PLATFORM_RAYLIB
#ifdef  FAN_PLATFORM_RAYLIB
# include "platform_raylib.c"
#endif

#if   defined(OS_WINDOWS)
# include "os_windows.c"
#elif defined(OS_LINUX)
# include "os_linux.c"
#endif

int FanRectIsEmpty(FanRect rect) {
    int result = 1;
    if (rect.x && rect.y && rect.width && rect.height) {
        result = 0;
    }
    return result;
}

float FanLerp(float a, float t, float b) {
    return  a + (b - a) * t;
}

int FanFloat32Equals(float x, float y) {
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
    if (!FanFloat32Equals(magnitude, 0.0f)) {
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

float FanVector2Cross(FanVector2 v1, FanVector2 v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

float FanVector2Dot(FanVector2 v1, FanVector2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

FanVector2 FanVector2Hadamard(FanVector2 v1, FanVector2 v2) {
    return (FanVector2){ v1.x * v2.x, v1.y * v2.y };
}

FanVector2 FanVector2Round(FanVector2 v) {
    return (FanVector2){ FanFloat32Round(v.x), FanFloat32Round(v.y) };
};

float FanFloat32Inf(void) {
    union { unsigned int i; float f; } u = { 0x7F800000 };
    return u.f;
}

float FanFloat32NegativeInf(void) {
    union { unsigned int i; float f; } u = { 0xFF800000 };
    return u.f;
}

float FanFloat32Round(float x) {
    return round(x);
}

float FanFloat32Exp(float x) {
    union { float f; int i; } u;
    u.i = (int)(12102203 * x) + 127 * (1 << 23);
    return u.f;
}
