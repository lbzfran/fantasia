
#include "os.h"
#include "platform.h"

#define FAN_PLATFORM_RAYLIB
#ifdef  FAN_PLATFORM_RAYLIB
# include "platform_raylib.c"
#endif

bool32 FanRectIsEmpty(FanRect rect) {
    int result = 1;
    if (rect.x && rect.y && rect.width && rect.height) {
        result = 0;
    }
    return result;
}

float32 FanLerp(float32 a, float32 t, float32 b) {
    return  a + (b - a) * t;
}

bool32 FanFloat32Equals(float32 x, float32 y) {
    float32 epsilon = 0.000001f;
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

FanVector2 FanVector2AddValue(FanVector2 v, float32 x) {
    return (FanVector2){ v.x + x, v.y + x };
}

FanVector2 FanVector2Sub(FanVector2 v1, FanVector2 v2) {
    return (FanVector2){ v1.x - v2.x, v1.y - v2.y };
}
FanVector2 FanVector2SubValue(FanVector2 v, float32 x) {
    return (FanVector2){ v.x - x, v.y - x };
}

FanVector2 FanVector2Normalize(FanVector2 v) {
    float32 magnitude = FanVector2Length(v);

    FanVector2 result = FanVector2Zero();
    if (!FanFloat32Equals(magnitude, 0.0f)) {
        result = (FanVector2){ v.x / magnitude, v.y / magnitude };
    }
    return result;
}

float32 FanVector2Length(FanVector2 v) {
    return sqrt(v.x * v.x + v.y * v.y);
}
float32 FanVector2LengthSqr(FanVector2 v) {
    return v.x * v.x + v.y * v.y;
}

FanVector2 FanVector2Scale(FanVector2 v, float32 scale) {
    return (FanVector2){ v.x * scale, v.y * scale };
}
FanVector2 FanVector2Negate(FanVector2 v) {
    return (FanVector2){ -v.x, -v.y };
}

float32 FanVector2Cross(FanVector2 v1, FanVector2 v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

float32 FanVector2Dot(FanVector2 v1, FanVector2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

FanVector2 FanVector2Hadamard(FanVector2 v1, FanVector2 v2) {
    return (FanVector2){ v1.x * v2.x, v1.y * v2.y };
}

FanVector2 FanVector2Round(FanVector2 v) {
    return (FanVector2){ FanFloat32Round(v.x), FanFloat32Round(v.y) };
};

float32 FanFloat32Inf(void) {
    union { uint32 i; float32 f; } u = { 0x7F800000 };
    return u.f;
}

float32 FanFloat32NegativeInf(void) {
    union { uint32 i; float32 f; } u = { 0xFF800000 };
    return u.f;
}

float32 FanFloat32Round(float32 x) {
    return round(x);
}

float32 FanFloat32Exp(float32 x) {
    union { float32 f; int32 i; } u;
    u.i = (int32)(12102203 * x) + 127 * (1 << 23);
    return u.f;
}

void FanVector2Print_(FanVector2 v, const char *name) {
    printf("%s: (%f, %f)\n", name, v.x, v.y);
}

void FanColorPrint_(FanColor c, const char *name) {
    printf("%s: (%d, %d, %d, %d)\n", name, c.r, c.g, c.b, c.a);
}

void FanRectPrint_(FanRect r, const char *name) {
    printf("%s: (%d, %d, %d, %d)\n", name, r.x, r.y, r.width, r.height);
}
