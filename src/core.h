#ifndef FAN_CORE_H
#define FAN_CORE_H

#include "os.h"

#define coalesce(a, b)      ((a) ? (a) : (b))
#define init_if_null(a, x)  ((a) = coalesce((a), (x)))

typedef struct {
    float32 x;
    float32 y;
} fan_vec2, fan_vec2_f32;

typedef struct {
    int32 r;
    int32 g;
    int32 b;
    int32 a;
} fan_color;

typedef struct {
    int32 x, y, w, h;
} fan_rect_int32, fan_rect_i32;

typedef struct {
    float32 x, y, w, h;
} fan_rect, fan_rect_float32, fan_rect_f32;

typedef struct {
    ssize rows;
    ssize cols;
    int32 *V;
} fan_matrix, fan_matrix_i32;

typedef struct {
    fan_vec2 target;
    fan_vec2 offset;
    float32 rotation;
    float32 zoom;
} fan_camera2D;

typedef struct {
    uint32 id;
    int32 width;
    int32 height;
    int32 mipmaps;
    int32 format;
} fan_texture;

typedef struct {
    int32 id;
    fan_texture texture;
    fan_texture depth;
} fan_rtexture;

#define fan_color_WHITE   (fan_color){ 210, 210, 210, 255 }
#define fan_color_GRAY    (fan_color){  80,  80,  80, 255 }
#define fan_color_BLACK   (fan_color){   0,   0,   0, 255 }
#define fan_color_RED     (fan_color){ 255,   0,   0, 255 }
#define fan_color_ORANGE  (fan_color){ 255, 165,   0, 255 }
#define fan_color_YELLOW  (fan_color){ 255, 255,   0, 255 }
#define fan_color_GREEN   (fan_color){   0, 255,   0, 255 }
#define fan_color_CYAN    (fan_color){   0, 255, 255, 255 }
#define fan_color_BLUE    (fan_color){   0,   0, 255, 255 }
#define fan_color_MAGENTA (fan_color){ 255,   0, 255, 255 }

#define PI 3.14159265358979323846f

FAN_API float32 fan_f32_clamp(float32 value, float32 min, float32 max);
FAN_API float32 fan_f32_lerp(float32 a, float32 x, float32 b);
FAN_API int32 fan_f32_equals(float32 x, float32 y);
FAN_API float32 fan_inf(void);
FAN_API float32 fan_neg_inf(void);
FAN_API float32 fan_f32_exp(float32);
FAN_API bool32 fan_f32_isvalid(float32);

FAN_API float32 fan_f32_round(float32);
FAN_API int32 fan_f32_truncate(float32);
FAN_API float32 fan_f32_abs(float32);

FAN_API float32 fan_f32_sin(float32);
FAN_API float32 fan_f32_cos(float32);
FAN_API float32 fan_f32_sqrt(float32);
FAN_API float32 fan_f32_atan2(float32, float32);

FAN_API float32 fan_f32_rsqrt(float32); // NOTE(liam): inverse sqrt

FAN_API float32 fan_f32_rad(float32);
FAN_API float32 fan_f32_deg(float32);

FAN_API void fan_vec2_print_(fan_vec2, const char8 *);
FAN_API void fan_color_print_(fan_color, const char8 *);
FAN_API void fan_rect_i32_print_(fan_rect_i32, const char8 *);
FAN_API void fan_rect_f32_print_(fan_rect_f32, const char8 *);
#define fan_vec2_print(v) fan_vec2_print_(v, #v)
#define fan_color_print(c) fan_color_print_(c, #c)
#define fan_rect_print(r) _Generic((r), fan_rect_i32: fan_rect_i32_print_, fan_rect_f32: fan_rect_f32_print_)(r, #r)

FAN_API int32 fan_i32_clamp(int32 v, int32 min, int32 max);
FAN_API int32 fan_rect_i32_isempty(fan_rect_i32 rect);
FAN_API int32 fan_rect_f32_isempty(fan_rect_f32 rect);

FAN_API fan_vec2 fan_vec2_zero(void);
FAN_API fan_vec2 fan_vec2_one(void);

FAN_API fan_vec2 fan_vec2_add(fan_vec2 v1, fan_vec2 v2);
FAN_API fan_vec2 fan_vec2_addv(fan_vec2 v, float32 x);

FAN_API fan_vec2 fan_vec2_sub(fan_vec2 v1, fan_vec2 v2);
FAN_API fan_vec2 fan_vec2_subv(fan_vec2 v, float32 x);

FAN_API fan_vec2 fan_vec2_normalize(fan_vec2 v);

FAN_API float32 fan_vec2_length(fan_vec2 v);
FAN_API float32 fan_vec2_lengthsqr(fan_vec2 v);

FAN_API fan_vec2 fan_vec2_scale(fan_vec2 v, float32 scale);
FAN_API fan_vec2 fan_vec2_negate(fan_vec2 v);

FAN_API float32 fan_vec2_dot(fan_vec2 v1, fan_vec2 v2);
FAN_API float32 fan_vec2_cross(fan_vec2 v1, fan_vec2 v2);
FAN_API fan_vec2 fan_vec2_hadamard(fan_vec2 v1, fan_vec2 v2);

FAN_API fan_vec2 fan_vec2_round(fan_vec2 v);
FAN_API fan_vec2 fan_vec2_rotate(fan_vec2 v, float32 angle);

FAN_API fan_vec2 fan_vec2_lerp(fan_vec2 v1, float32 t, fan_vec2 v2);

FAN_API fan_matrix fan_matrix_create_(ssize, ssize, int32 *);
#define fan_matrix_create(a, row, col) fan_matrix_create_(row, col, (a)->make((a)->ctx,sizeof(int32) * row * col))
#define fan_matrix_at(mat, i, j) ((mat).V[(int32)((ssize)(i) * (mat).cols + (ssize)(j))])

FAN_API void fan_matrix_fill(fan_matrix, int32);
FAN_API void fan_matrix_randomize(fan_matrix, int32, int32);

#endif // FAN_CORE_H
