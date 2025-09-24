#ifndef FAN_PLATFORM_H
#define FAN_PLATFORM_H

#include "os.h"

#if defined(PLATFORM_BUILD_SHARED)
    #if defined(OS_WINDOWS)
        #define FAN_API __declspec(dllexport)
    #elif defined(OS_LINUX)
        #define FAN_API __attribute__ ((visibility ("default")))
    #else
        #define FAN_API
    #endif
#else
    #if defined(OS_WINDOWS)
        #define FAN_API __declspec(dllimport)
    #else
        #define FAN_API
    #endif
#endif

# ifndef FAN_API
    #define FAN_API extern
# endif

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
    int32 x;
    int32 y;
    int32 width;
    int32 height;
} fan_rect_int32, fan_rect_i32;

typedef struct {
    float32 x;
    float32 y;
    float32 width;
    float32 height;
} fan_rect, fan_rect_f32;

typedef struct {
    ssize  rows;
    ssize  cols;
    int32 *V;
} fan_matrix, fan_matrix_i32;

typedef struct {
    uint32 id;
    int32  width;
    int32  height;
    int32  mipmaps;
    int32  format;
} fan_texture;

typedef struct {
    int32       id;
    fan_texture texture;
    fan_texture depth;
} fan_rtexture;

typedef struct {
    fan_vec2 target;
    fan_vec2 offset;
    float32  rotation;
    float32  zoom;
} fan_camera2D;

typedef struct {
    void *buffer;
    void *processor;

    uint32 sample_rate;
    uint32 sample_size;
    uint32 channels;
} fan_audio_stream;

typedef struct {
    fan_audio_stream stream;
    uint32           frame_count;
} fan_sound;

typedef struct {
    fan_audio_stream  stream;
    uint32            frame_count;
    int32             loop;
    int32             ctx_type;
    void             *ctx_data;
} fan_music;

typedef enum {
    FanKey_NULL            = 0,        // Key: NULL, used for no key pressed
    // Alphanumeric keys
    FanKey_APOSTROPHE      = 39,       // Key: '
    FanKey_COMMA           = 44,       // Key: ,
    FanKey_MINUS           = 45,       // Key: -
    FanKey_PERIOD          = 46,       // Key: .
    FanKey_SLASH           = 47,       // Key: /
    FanKey_ZERO            = 48,       // Key: 0
    FanKey_ONE             = 49,       // Key: 1
    FanKey_TWO             = 50,       // Key: 2
    FanKey_THREE           = 51,       // Key: 3
    FanKey_FOUR            = 52,       // Key: 4
    FanKey_FIVE            = 53,       // Key: 5
    FanKey_SIX             = 54,       // Key: 6
    FanKey_SEVEN           = 55,       // Key: 7
    FanKey_EIGHT           = 56,       // Key: 8
    FanKey_NINE            = 57,       // Key: 9
    FanKey_SEMICOLON       = 59,       // Key: ;
    FanKey_EQUAL           = 61,       // Key: =
    FanKey_A               = 65,       // Key: A | a
    FanKey_B               = 66,       // Key: B | b
    FanKey_C               = 67,       // Key: C | c
    FanKey_D               = 68,       // Key: D | d
    FanKey_E               = 69,       // Key: E | e
    FanKey_F               = 70,       // Key: F | f
    FanKey_G               = 71,       // Key: G | g
    FanKey_H               = 72,       // Key: H | h
    FanKey_I               = 73,       // Key: I | i
    FanKey_J               = 74,       // Key: J | j
    FanKey_K               = 75,       // Key: K | k
    FanKey_L               = 76,       // Key: L | l
    FanKey_M               = 77,       // Key: M | m
    FanKey_N               = 78,       // Key: N | n
    FanKey_O               = 79,       // Key: O | o
    FanKey_P               = 80,       // Key: P | p
    FanKey_Q               = 81,       // Key: Q | q
    FanKey_R               = 82,       // Key: R | r
    FanKey_S               = 83,       // Key: S | s
    FanKey_T               = 84,       // Key: T | t
    FanKey_U               = 85,       // Key: U | u
    FanKey_V               = 86,       // Key: V | v
    FanKey_W               = 87,       // Key: W | w
    FanKey_X               = 88,       // Key: X | x
    FanKey_Y               = 89,       // Key: Y | y
    FanKey_Z               = 90,       // Key: Z | z
    FanKey_LEFT_BRACKET    = 91,       // Key: [
    FanKey_BACKSLASH       = 92,       // Key: '\'
    FanKey_RIGHT_BRACKET   = 93,       // Key: ]
    FanKey_GRAVE           = 96,       // Key: `
    // Function keys
    FanKey_SPACE           = 32,       // Key: Space
    FanKey_ESCAPE          = 256,      // Key: Esc
    FanKey_ENTER           = 257,      // Key: Enter
    FanKey_TAB             = 258,      // Key: Tab
    FanKey_BACKSPACE       = 259,      // Key: Backspace
    FanKey_INSERT          = 260,      // Key: Ins
    FanKey_DELETE          = 261,      // Key: Del
    FanKey_RIGHT           = 262,      // Key: Cursor right
    FanKey_LEFT            = 263,      // Key: Cursor left
    FanKey_DOWN            = 264,      // Key: Cursor down
    FanKey_UP              = 265,      // Key: Cursor up
    FanKey_PAGE_UP         = 266,      // Key: Page up
    FanKey_PAGE_DOWN       = 267,      // Key: Page down
    FanKey_HOME            = 268,      // Key: Home
    FanKey_END             = 269,      // Key: End
    FanKey_CAPS_LOCK       = 280,      // Key: Caps lock
    FanKey_SCROLL_LOCK     = 281,      // Key: Scroll down
    FanKey_NUM_LOCK        = 282,      // Key: Num lock
    FanKey_PRINT_SCREEN    = 283,      // Key: Print screen
    FanKey_PAUSE           = 284,      // Key: Pause
    FanKey_F1              = 290,      // Key: F1
    FanKey_F2              = 291,      // Key: F2
    FanKey_F3              = 292,      // Key: F3
    FanKey_F4              = 293,      // Key: F4
    FanKey_F5              = 294,      // Key: F5
    FanKey_F6              = 295,      // Key: F6
    FanKey_F7              = 296,      // Key: F7
    FanKey_F8              = 297,      // Key: F8
    FanKey_F9              = 298,      // Key: F9
    FanKey_F10             = 299,      // Key: F10
    FanKey_F11             = 300,      // Key: F11
    FanKey_F12             = 301,      // Key: F12
    FanKey_LEFT_SHIFT      = 340,      // Key: Shift left
    FanKey_LEFT_CONTROL    = 341,      // Key: Control left
    FanKey_LEFT_ALT        = 342,      // Key: Alt left
    FanKey_LEFT_SUPER      = 343,      // Key: Super left
    FanKey_RIGHT_SHIFT     = 344,      // Key: Shift right
    FanKey_RIGHT_CONTROL   = 345,      // Key: Control right
    FanKey_RIGHT_ALT       = 346,      // Key: Alt right
    FanKey_RIGHT_SUPER     = 347,      // Key: Super right
    FanKey_KB_MENU         = 348,      // Key: KB menu
    // Keypad keys
    FanKey_KP_0            = 320,      // Key: Keypad 0
    FanKey_KP_1            = 321,      // Key: Keypad 1
    FanKey_KP_2            = 322,      // Key: Keypad 2
    FanKey_KP_3            = 323,      // Key: Keypad 3
    FanKey_KP_4            = 324,      // Key: Keypad 4
    FanKey_KP_5            = 325,      // Key: Keypad 5
    FanKey_KP_6            = 326,      // Key: Keypad 6
    FanKey_KP_7            = 327,      // Key: Keypad 7
    FanKey_KP_8            = 328,      // Key: Keypad 8
    FanKey_KP_9            = 329,      // Key: Keypad 9
    FanKey_KP_DECIMAL      = 330,      // Key: Keypad .
    FanKey_KP_DIVIDE       = 331,      // Key: Keypad /
    FanKey_KP_MULTIPLY     = 332,      // Key: Keypad *
    FanKey_KP_SUBTRACT     = 333,      // Key: Keypad -
    FanKey_KP_ADD          = 334,      // Key: Keypad +
    FanKey_KP_ENTER        = 335,      // Key: Keypad Enter
    FanKey_KP_EQUAL        = 336,      // Key: Keypad =
} fan_key;

typedef enum {
    FanLog_ALL     = 0,
    FanLog_TRACE   = 1,
    FanLog_DEBUG   = 2,
    FanLog_INFO    = 3,
    FanLog_WARNING = 4,
    FanLog_ERROR   = 5,
    FanLog_FATAL   = 6,
    FanLog_NONE    = 7
} fan_loglevel;

typedef enum {
    FanWindow_VSYNC_HINT         = 0x00000040,   // Set to try enabling V-Sync on GPU
    FanWindow_FULLSCREEN_MODE    = 0x00000002,   // Set to run program in fullscreen
    FanWindow_WINDOW_RESIZABLE   = 0x00000004,   // Set to allow resizable window
    FanWindow_WINDOW_UNDECORATED = 0x00000008,   // Set to disable window decoration (frame and buttons)
    FanWindow_WINDOW_HIDDEN      = 0x00000080,   // Set to hide window
    FanWindow_WINDOW_MINIMIZED   = 0x00000200,   // Set to minimize window (iconify)
    FanWindow_WINDOW_MAXIMIZED   = 0x00000400,   // Set to maximize window (expanded to monitor)
    FanWindow_WINDOW_UNFOCUSED   = 0x00000800,   // Set to window non focused
    FanWindow_WINDOW_TOPMOST     = 0x00001000,   // Set to window always on top
    FanWindow_WINDOW_ALWAYS_RUN  = 0x00000100,   // Set to allow windows running while minimized
    FanWindow_WINDOW_TRANSPARENT = 0x00000010,   // Set to allow transparent framebuffer
    FanWindow_WINDOW_HIGHDPI     = 0x00002000,   // Set to support HighDPI
    FanWindow_WINDOW_MOUSE_PASSTHROUGH = 0x00004000, // Set to support mouse passthrough, only supported when FLAG_WINDOW_UNDECORATED
    FanWindow_BORDERLESS_WINDOWED_MODE = 0x00008000, // Set to run program in borderless windowed mode
    FanWindow_MSAA_4X_HINT       = 0x00000020,   // Set to try enabling MSAA 4X
    FanWindow_INTERLACED_HINT    = 0x00010000    // Set to try enabling interlaced video format (for V3D)
} fan_flag_window;

typedef enum {
    FanBlend_ALPHA = 0,                // Blend textures considering alpha (default)
    FanBlend_ADDITIVE,                 // Blend textures adding colors
    FanBlend_MULTIPLIED,               // Blend textures multiplying colors
    FanBlend_ADD_COLORS,               // Blend textures adding colors (alternative)
    FanBlend_SUBTRACT_COLORS,          // Blend textures subtracting colors (alternative)
    FanBlend_ALPHA_PREMULTIPLY,        // Blend premultiplied textures considering alpha
    FanBlend_CUSTOM,                   // Blend textures using custom src/dst factors (use rlSetBlendFactors())
    FanBlend_CUSTOM_SEPARATE           // Blend textures using custom rgb/alpha separate src/dst factors (use rlSetBlendFactorsSeparate())
} fan_flag_blend;

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

FAN_API void  fan_window_create(int32 width, int32 height, const char8 *title);
FAN_API void  fan_window_close(void);
FAN_API int32 fan_window_shouldclose(void);

FAN_API void fan_window_config(int32);
FAN_API bool32 fan_window_resized(void);
FAN_API void fan_log_set(int32);

FAN_API int32 fan_window_width(void);
FAN_API int32 fan_window_height(void);

FAN_API void fan_fps_target(int32 fps);

FAN_API void fan_dev_audio_create(void);
FAN_API void fan_dev_audio_close(void);

FAN_API fan_sound fan_sound_load(const char8 *filepath);
FAN_API void      fan_sound_unload(fan_sound);
FAN_API void      fan_sound_play(fan_sound);
FAN_API void      fan_sound_stop(fan_sound);
FAN_API void      fan_sound_pause(fan_sound sound);
FAN_API void      fan_sound_resume(fan_sound sound);

FAN_API void     fan_sound_volume_set(fan_sound, float32);
FAN_API void     fan_sound_pitch_set(fan_sound, float32);
FAN_API void     fan_sound_pan_set(fan_sound, float32);

FAN_API fan_music fan_music_load(const char8 *filepath);
FAN_API void      fan_music_unload(fan_music);
FAN_API void      fan_music_update(fan_music);
FAN_API void      fan_music_play(fan_music);
FAN_API void      fan_music_stop(fan_music);
FAN_API void      fan_music_pause(fan_music music);
FAN_API void      fan_music_resume(fan_music music);
FAN_API void      fan_music_seek(fan_music, float32);
FAN_API void      fan_music_volume_set(fan_music, float32);
FAN_API void      fan_music_pitch_set(fan_music, float32);
FAN_API void      fan_music_pan_set(fan_music, float32);

FAN_API float32  fan_music_time_played(fan_music music);
FAN_API float32  fan_music_time_length(fan_music music);

FAN_API float32 fan_frametime_get(void);
FAN_API float32 fan_time_get(void);
FAN_API int32   fan_fps_get(void);

FAN_API void fan_random_seed(int32 seed);
FAN_API int32  fan_random_int(int32 min, int32 max);

FAN_API int32 fan_key_pressed(fan_key key);
FAN_API int32 fan_key_down(fan_key key);

FAN_API fan_texture fan_texture_load(const char8 *filepath);
FAN_API void        fan_texture_unload(fan_texture texture);

FAN_API void fan_draw_begin(void);
FAN_API void fan_draw_clear(fan_color color);
FAN_API void fan_draw_fps(int32 x, int32 y);
FAN_API void fan_draw_end(void);

FAN_API void fan_draw_pixel(int32, int32, fan_color color);
FAN_API void fan_draw_line(int32, int32, int32, int32, fan_color color);
FAN_API void fan_draw_linev(fan_vec2 start, fan_vec2 end, fan_color color);
FAN_API void fan_draw_rect(int32 x, int32 y, int32 w, int32 h, fan_color color);
FAN_API void fan_draw_rectv(fan_vec2 pos, fan_vec2 scale, fan_color color);
FAN_API void fan_draw_rectr(fan_rect rect, fan_color color);
FAN_API void fan_draw_circle(int32 x, int32 y, float32 r, fan_color color);
FAN_API void fan_draw_circle_grad(int32 x, int32 y, float32 r, fan_color in, fan_color out);

FAN_API void fan_draw_texture(fan_texture texture, fan_rect src, fan_rect dst, fan_vec2 origin, float32 angle, fan_color);

FAN_API void fan_camera_begin(fan_camera2D);
FAN_API void fan_camera_end(void);

FAN_API fan_rtexture fan_rtexture_load(int32, int32);
FAN_API void fan_rtexture_unload(fan_rtexture);
FAN_API void fan_mode_texture_begin(fan_rtexture);
FAN_API void fan_mode_texture_end(void);
FAN_API void fan_mode_blend_begin(int32);
FAN_API void fan_mode_blend_end(void);

// quick maths

FAN_API float32 fan_f32_clamp(float32 value, float32 min, float32 max);
FAN_API float32 fan_f32_lerp(float32 a, float32 x, float32 b);
FAN_API int32   fan_f32_equals(float32 x, float32 y);
FAN_API float32 fan_inf(void);
FAN_API float32 fan_neg_inf(void);
FAN_API float32 fan_f32_exp(float32);

FAN_API float32 fan_f32_round(float32);
FAN_API int32   fan_f32_truncate(float32);
FAN_API float32 fan_f32_abs(float32);

FAN_API float32 fan_f32_sin(float32);
FAN_API float32 fan_f32_cos(float32);
FAN_API float32 fan_f32_sqrt(float32);

FAN_API float32 fan_f32_rad(float32);
FAN_API float32 fan_f32_deg(float32);

FAN_API void fan_vec2_print_(fan_vec2, const char8 *);
FAN_API void fan_color_print_(fan_color, const char8 *);
FAN_API void fan_rect_i32_print_(fan_rect_i32, const char8 *);
FAN_API void fan_rect_f32_print_(fan_rect_f32, const char8 *);
#define fan_vec2_print(v) fan_vec2_print_(v, #v)
#define fan_color_print(c) fan_color_print_(c, #c)
#define fan_rect_print(r) _Generic((r),        \
    fan_rect_i32: fan_rect_i32_print_,         \
    fan_rect_f32: fan_rect_f32_print_)(r, #r)


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

FAN_API float32    fan_vec2_dot(fan_vec2 v1, fan_vec2 v2);
FAN_API float32    fan_vec2_cross(fan_vec2 v1, fan_vec2 v2);
FAN_API fan_vec2 fan_vec2_hadamard(fan_vec2 v1, fan_vec2 v2);

FAN_API fan_vec2 fan_vec2_round(fan_vec2 v);
FAN_API fan_vec2 fan_vec2_rotate(fan_vec2 v, float32 angle);

FAN_API fan_vec2 fan_vec2_lerp(fan_vec2 v1, float32 t, fan_vec2 v2);

FAN_API int32 fan_rect_i32_isempty(fan_rect_i32 rect);
FAN_API int32 fan_rect_f32_isempty(fan_rect_f32 rect);

FAN_API fan_matrix fan_matrix_create_(ssize, ssize, int32 *);
#define fan_matrix_create(a, row, col) fan_matrix_create_(row, col, (a)->make((a)->ctx,sizeof(int32) * row * col))
#define fan_matrix_at(mat, i, j) ((mat).V[(i) * (mat).cols + (j)])

FAN_API void fan_matrix_fill(fan_matrix, int32);

// threading

#endif // FAN_PLATFORM_H
