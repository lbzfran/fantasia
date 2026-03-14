
#include "os.h"
#include "platform.h"

#define FAN_PLATFORM_RAYLIB
#ifdef  FAN_PLATFORM_RAYLIB
# include "platform_raylib.c"
#endif

#include <math.h>

int32 fan_i32_clamp(int32 v, int32 min, int32 max) {
    return v < min ? min : ((v > max) ? max : v);
}

bool32 fan_rect_i32_isempty(fan_rect_i32 rect) {
    bool32 result = true;
    if (rect.x || rect.y || rect.w || rect.h) {
        result = false;
    }
    return result;
}

bool32 fan_rect_f32_isempty(fan_rect_f32 rect) {
    bool32 result = false;
    if (fan_f32_equals(rect.x,      0.0f) &&
        fan_f32_equals(rect.y,      0.0f) &&
        fan_f32_equals(rect.w,      0.0f) &&
        fan_f32_equals(rect.h,      0.0f)) {
        result = true;
    }
    return result;
}

float32 fan_f32_lerp(float32 a, float32 t, float32 b) {
    return  a + (b - a) * t;
}

bool32 fan_f32_equals(float32 x, float32 y) {
    float32 epsilon = 0.000001f;
    int result = (fabsf(x - y)) <= (epsilon*fmaxf(1.0f, fmaxf(fabsf(x), fabsf(y))));
    return result;
}

fan_vec2 fan_vec2_zero(void) {
    return (fan_vec2){ 0.0f, 0.0f };
}

fan_vec2 fan_vec2_one(void) {
    return (fan_vec2){ 1.0f, 1.0f };
}

fan_vec2 fan_vec2_add(fan_vec2 v1, fan_vec2 v2) {
    return (fan_vec2){ v1.x + v2.x, v1.y + v2.y };
}

fan_vec2 fan_vec2_addf(fan_vec2 v, float32 x) {
    return (fan_vec2){ v.x + x, v.y + x };
}

fan_vec2 fan_vec2_sub(fan_vec2 v1, fan_vec2 v2) {
    return (fan_vec2){ v1.x - v2.x, v1.y - v2.y };
}
fan_vec2 fan_vec2_subf(fan_vec2 v, float32 x) {
    return (fan_vec2){ v.x - x, v.y - x };
}

fan_vec2 fan_vec2_normalize(fan_vec2 v) {
    float32 magnitude = fan_vec2_length(v);

    fan_vec2 result = fan_vec2_zero();
    if (!fan_f32_equals(magnitude, 0.0f)) {
        result = (fan_vec2){ v.x / magnitude, v.y / magnitude };
    }
    return result;
}

float32 fan_vec2_length(fan_vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}
float32 fan_vec2_lengthsqr(fan_vec2 v) {
    return v.x * v.x + v.y * v.y;
}

fan_vec2 fan_vec2_scale(fan_vec2 v, float32 scale) {
    return (fan_vec2){ v.x * scale, v.y * scale };
}
fan_vec2 fan_vec2_negate(fan_vec2 v) {
    return (fan_vec2){ -v.x, -v.y };
}

float32 fan_vec2_cross(fan_vec2 v1, fan_vec2 v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

float32 fan_vec2_dot(fan_vec2 v1, fan_vec2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

fan_vec2 fan_vec2_hadamard(fan_vec2 v1, fan_vec2 v2) {
    return (fan_vec2){ v1.x * v2.x, v1.y * v2.y };
}

fan_vec2 fan_vec2_round(fan_vec2 v) {
    return (fan_vec2){ fan_f32_round(v.x), fan_f32_round(v.y) };
};

float32 fan_inf(void) {
    union { uint32 i; float32 f; } u = { 0x7F800000 };
    return u.f;
}

float32 fan_neg_inf(void) {
    union { uint32 i; float32 f; } u = { 0xFF800000 };
    return u.f;
}

float32 fan_f32_round(float32 x) {
    return roundf(x);
}

int32 fan_f32_truncate(float32 x) {
    return (int32)x;
}

float32 fan_f32_abs(float32 x) {
    return fabsf(x);
}


float32 fan_f32_exp(float32 x) {
    union { float32 f; int32 i; } u;
    u.i = (int32)(12102203 * x) + 127 * (1 << 23);
    return u.f;
}

float32 fan_f32_sin(float32 x) {
    return sinf(x);
}

float32 fan_f32_cos(float32 x) {
    return cosf(x);
}

float32 fan_f32_sqrt(float32 x) {
    return sqrtf(x);
}

float32 fan_f32_rad(float32 deg) {
    return deg * (PI / 180.0f);
}
float32 fan_f32_deg(float32 rad) {
    return rad * (180.0f / PI);
}

fan_vec2 fan_vec2_rotate(fan_vec2 v, float32 angle) {
    float32 cos_a = fan_f32_cos(angle);
    float32 sin_a = fan_f32_sin(angle);

    return (fan_vec2) {
        v.x * cos_a - v.y * sin_a,
        v.x * sin_a + v.y * cos_a
    };
}

fan_vec2 fan_vec2_lerp(fan_vec2 v1, float32 t, fan_vec2 v2) {
    return (fan_vec2) {
        fan_f32_lerp(v1.x, t, v2.x),
        fan_f32_lerp(v1.y, t, v2.y)
    };
}

void fan_vec2_print_(fan_vec2 v, const char8 *name) {
    printf("%s: (%f, %f)\n", name, (float64)v.x, (float64)v.y);
}

void fan_color_print_(fan_color c, const char8 *name) {
    printf("%s: (%d, %d, %d, %d)\n", name, c.r, c.g, c.b, c.a);
}

void fan_rect_i32_print_(fan_rect_i32 r, const char8 *name) {
    printf("%s: (%d, %d, %d, %d)\n", name, r.x, r.y, r.w, r.h);
}

void fan_rect_f32_print_(fan_rect_f32 r, const char8 *name) {
    printf("%s: (%f, %f, %f, %f)\n", name, (float64)r.x, (float64)r.y, (float64)r.w, (float64)r.h);
}

fan_matrix fan_matrix_create_(ssize rows, ssize cols, int32 *data) {
    fan_matrix mat = { .cols = cols, .rows = rows, .V = data };
    return mat;
}

void fan_matrix_fill(fan_matrix mat, int32 x) {
    for (ssize i = 0; i < mat.rows; i++) {
        for (ssize j = 0; j < mat.cols; j++) {
            fan_matrix_at(mat, i, j) = x;
        }
    }
}

void fan_matrix_randomize(fan_matrix mat, int32 start, int32 end) {
    srand(start);
    for (ssize i = 0; i < mat.rows; i++) {
        for (ssize j = 0; j < mat.cols; j++) {
            int32 value = (int32)((rand() % (end + 1)) + start);
            fan_matrix_at(mat, i, j) = value;
        }
    }
}

inline bool32 fan_rect_i32_valid(fan_rect_i32 rect) {
    return rect.x >= 0 && rect.y >= 0 && rect.w >= 0 && rect.h >= 0;
}

fan_rect_i32 fan_rect_i32_intersection(fan_rect_i32 a, fan_rect_i32 b) {
    int32_t x1 = (a.x > b.x) ? a.x : b.x;
    int32_t y1 = (a.y > b.y) ? a.y : b.y;

    int32_t x2 = ((a.x + a.w) < (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
    int32_t y2 = ((a.y + a.h) < (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);

    if (x2 <= x1 || y2 <= y1) {
        return (fan_rect_i32) {};
    }
    return (fan_rect_i32) {
        .x = x1,
        .y = y1,
        .w = x2 - x1,
        .h = y2 - y1
    };
}

fan_rect_i32 fan_rect_i32_bounding(fan_rect_i32 a, fan_rect_i32 b) {
    int32_t x1 = (a.x < b.x) ? a.x : b.x;
    int32_t y1 = (a.y < b.y) ? a.y : b.y;

    int32_t x2 = ((a.x + a.w) > (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
    int32_t y2 = ((a.y + a.h) > (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);

    return (fan_rect_i32){
        .x = x1,
        .y = y1,
        .w = x2 - x1,
        .h = y2 - y1
    };
}

inline bool32 fan_rect_i32_equals(fan_rect_i32 a, fan_rect_i32 b) {
    return a.x == b.x &&
           a.y == b.y &&
           a.w == b.w &&
           a.h == b.h;
}

inline bool32 fan_rect_i32_contains(fan_rect_i32 r, int32 x, int32 y) {
    return x >= r.x &&
           y >= r.y &&
           x <  r.x + r.w &&
           y <  r.y + r.h;
}
