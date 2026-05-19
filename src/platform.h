#ifndef FAN_PLATFORM_H
#define FAN_PLATFORM_H

#include "os.h"
#include "core.h"

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

typedef struct {
    char8      *key;
    fan_texture value;
} fan_asset_sprite_entry;

typedef struct {
    fan_asset_sprite_entry *sprites;
    fan_texture             default_sprite;
} fan_asset;

FAN_API void fan_sprite_load(fan_asset *assets, fan_allocator *mem, char8 *const path);
FAN_API void fan_sprite_unload(fan_asset *assets, fan_allocator *mem);
FAN_API void fan_sprite_init(fan_asset *assets, fan_texture fallback);
FAN_API fan_texture fan_sprite_get(fan_asset *assets, char8 *const name);

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
FAN_API void fan_draw_rectv(fan_vec2 pos, fan_vec2 scale, fan_vec2 origin, float32 rotation, fan_color color);
FAN_API void fan_draw_rectr(fan_rect rect, fan_color color);
FAN_API void fan_draw_circle(int32 x, int32 y, float32 r, fan_color color);
FAN_API void fan_draw_circle_grad(int32 x, int32 y, float32 r, fan_color in, fan_color out);

FAN_API void fan_draw_texture(fan_texture texture, fan_rect src, fan_rect dst, fan_vec2 origin, float32 angle, fan_color);
FAN_API void fan_draw_text(const char8 *buf, fan_vec2 origin, fan_color text_color, fan_color background_color);

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
FAN_API float32 fan_f32_atan2(float32, float32);

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

FAN_API bool32 fan_rect_i32_valid(fan_rect_i32);
FAN_API fan_rect_i32 fan_rect_i32_intersection(fan_rect_i32, fan_rect_i32);
FAN_API fan_rect_i32 fan_rect_i32_bounding(fan_rect_i32, fan_rect_i32);
FAN_API bool32 fan_rect_i32_equals(fan_rect_i32, fan_rect_i32);
FAN_API bool32 fan_rect_i32_contains(fan_rect_i32, int32, int32);

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

FAN_API int32 fan_i32_clamp(int32 v, int32 min, int32 max);

FAN_API int32 fan_rect_i32_isempty(fan_rect_i32 rect);
FAN_API int32 fan_rect_f32_isempty(fan_rect_f32 rect);

FAN_API fan_matrix fan_matrix_create_(ssize, ssize, int32 *);
#define fan_matrix_create(a, row, col) fan_matrix_create_(row, col, (a)->make((a)->ctx,sizeof(int32) * row * col))
#define fan_matrix_at(mat, i, j) ((mat).V[(int32)((ssize)(i) * (mat).cols + (ssize)(j))])

FAN_API void fan_matrix_fill(fan_matrix, int32);
FAN_API void fan_matrix_randomize(fan_matrix, int32, int32);

// DSL parsing
typedef enum {
    FanToken_NULL = 1,
    FanToken_TYPE,
    FanToken_NAME,
    FanToken_LBRACKET,
    FanToken_RBRACKET,
    FanToken_FIELD,
    FanToken_VALUE_STRING,
    FanToken_VALUE_NUMBER,
} fan_dsl_token_type;

typedef struct {
    fan_dsl_token_type type;
    fan_str8 literal;
} fan_dsl_token;

typedef struct {
    fan_dsl_token *data;
    ssize size;
    ssize capacity;
} fan_dsl_token_array;

typedef struct {
    uint8 key[32];
    uint8 values[8][64];
    int32 value_count;
} fan_dsl_field;

typedef struct {
    void *default_value;
    ssize size;
    ssize capacity;
    ssize value_size;
    ssize key_offset;
} fan_ht_header;

#define fan_ht_define(NAME, TYPE)  \
                                   \
typedef struct {                   \
    fan_str8 key;                  \
    TYPE     value;                \
} fan_ht_entry_##NAME

fan_ht_define(str8, fan_str8);

typedef union {
    int32    i;
    float32  f;
    bool32   b;
    fan_str8 s;
} fan_cvar_value;

typedef enum : uint32 {
    FanCVar_NONE     = 0,
    FanCVar_ARCHIVE  = 1 << 0,
    FanCVar_ROM      = 1 << 1,
    FanCVar_CHEAT    = 1 << 2,
    FanCVar_MODIFIED = 1 << 3,

    FanCVar_INT      = 1 << 4,
    FanCVar_FLOAT    = 1 << 5,
    FanCVar_BOOL     = 1 << 6,
    FanCVar_STRING   = 1 << 7,

    FanCVar_SYSTEM   = 1 << 8,
    FanCVar_GAME     = 1 << 9,
    FanCVar_NONCHEAT = 1 << 10,
} fan_cvar_flags;

// typedef struct {
//     fan_str8          name;
//     fan_str8          description;
//     fan_cvar_type     type;
//     fan_cvar_value    value;
//     fan_str8          valueString;
//     fan_str8          resetString;
//     fan_cvar_flags    flags;
//     float32           min_value, max_value;
// } fan_cvar_internal_;

typedef struct {
    fan_str8 *data;
    ssize size;
    ssize capacity;
} fan_str8_array;

typedef struct fan_cvar fan_cvar;
struct fan_cvar {
    fan_str8           name;
    fan_str8           description;
    fan_cvar_value     value;
    fan_cvar_value     default_value;
    fan_cvar_flags     flags;

    float32            min_value;
    float32            max_value;
    fan_str8_array     string_values;

    fan_cvar          *next;
};

fan_ht_define(cvar, fan_cvar);

// TODO(liam): figure out what to do with the hashtable.
// make str8-to-str8 and str8-to-ptr, or use a generic macro
// like the generic array?
typedef struct {
    fan_ht_entry_cvar  *table;
    fan_cvar           *head;

    fan_arena  arena;
} fan_cvar_system;

FAN_API usize fan_hash_str8(fan_str8);
FAN_API usize fan_hash_bytes(const void *ptr, usize len);

FAN_API void *fan_ht_create(ssize entry_size, ssize capacity, void *default_value, fan_allocator *mem);
FAN_API void fan_ht_free(void *table, fan_allocator *mem);
FAN_API ssize fan_ht_cap(void *table);
FAN_API ssize fan_ht_len(void *table);
FAN_API void *fan_ht_get(fan_str8 key, void *table);
FAN_API void *fan_ht_put(fan_str8 key, void *value, void *table, fan_allocator *mem);
FAN_API bool32 fan_ht_delete(fan_str8 key, void *table, fan_allocator *mem);

FAN_API fan_cvar *fan_cvar_register_(fan_cvar v, fan_cvar_system *sys);

static inline fan_cvar_value fan_cvar_value_i32(int32 v) {
    return (fan_cvar_value){ .i = v };
}

static inline fan_cvar_value fan_cvar_value_f32(float32 v) {
    return (fan_cvar_value){ .f = v };
}

static inline fan_cvar_value fan_cvar_value_b32(bool32 v) {
    return (fan_cvar_value){ .b = v };
}

#define FAN_CVAR_VALUE(v) \
    _Generic((v), \
             int32: fan_cvar_value_int \
             )(v)

#define fan_cvar_register(sys, name, value, ...) fan_cvar_register_((fan_cvar){ \
                                                                    .name = (name), \
                                                                    .default_value = FAN_CVAR_VALUE(value), \
                                                                    __VA_ARGS__}, (sys))

FAN_API void     fan_cvar_set(fan_str8, fan_str8);
FAN_API fan_str8 fan_cvar_get(fan_str8);

FAN_API int32   fan_cvar_get_int32(fan_str8);
FAN_API float32 fan_cvar_get_float32(fan_str8);
FAN_API bool32  fan_cvar_get_bool32(fan_str8);


FAN_API void fan_dsl_array_append(fan_allocator *mem, fan_dsl_token_array *arr, fan_dsl_token x);
FAN_API fan_dsl_token_array fan_dsl_tokenize(fan_allocator *mem, fan_str8 buf);

// Components
#define fan_component_declare(name, T)  \
    typedef struct name##Storage { \
         ssize *sparse;            \
         ssize *dense;             \
             T *data;              \
         ssize  size;              \
         ssize  capacity;          \
    } name##Storage

#define fan_component_has(storage, id) ((storage)->sparse[id] != -1)

#define fan_component_get_value_or_else(storage, id, default_value) \
    (fan_component_has(storage, id) ? (storage)->data[(storage)->sparse[(id)]] : (default_value))
#define fan_component_get_value(storage, id) fan_component_get_value_or_else(storage, id, 0)

#define fan_component_get_or_else(storage, id, default_value) \
    (fan_component_has(storage, id) ? &(storage)->data[(storage)->sparse[(id)]] : (default_value))
#define fan_component_get(storage, id) fan_component_get_or_else(storage, id, null)

// NOTE(liam): 'Fast' includes optimizations in 'release' build.
#define fan_component_get_value_fast(storage, id) \
    (assume(fan_component_has(storage, id)), (storage)->data[(storage)->sparse[(id)]])
#define fan_component_get_fast(storage, id) \
    (assume(fan_component_has(storage, id)), &(storage)->data[(storage)->sparse[(id)]])

#define fan_component_create(storage, mem, cap) do{                                                    \
    (storage)->sparse   = (mem)->make((mem)->ctx, sizeof(*(storage)->sparse) * (MAX_ENTITY_CAP)); \
    (storage)->dense    = (mem)->make((mem)->ctx, sizeof(*(storage)->dense)  * (cap));            \
    (storage)->data     = (mem)->make((mem)->ctx, sizeof(*(storage)->data)   * (cap));            \
    (storage)->capacity = (cap);                                                                  \
    (storage)->size = 0;                                                                          \
    memset((storage)->sparse, -1, sizeof(*(storage)->sparse) * (cap));                            \
}while(0);

#define fan_component_add(storage, id, ...) do{                                       \
    assert((storage)->size < (storage)->capacity);                                    \
    assert(!fan_component_has(storage,id));                                           \
    ssize i = (storage)->size++;                                                      \
    (storage)->dense[i] = (id);                                                       \
    (storage)->sparse[id] = i;                                                        \
    (storage)->data[(storage)->sparse[id]] = (typeof(*(storage)->data)){__VA_ARGS__}; \
}while(0);

#define fan_component_delete(storage, id) do{          \
    ssize i = (storage)->sparse[id];              \
    assert(i != -1);                              \
    ssize last_i = --(storage)->size;             \
    ssize last_entity = (storage)->dense[last_i]; \
    (storage)->dense[i] = last_entity;            \
    (storage)->sparse[last_entity] = i;           \
    (storage)->data[i] = (storage)->data[last_i]; \
    (storage)->sparse[id] = -1;                   \
}while(0);

#endif // FAN_PLATFORM_H
