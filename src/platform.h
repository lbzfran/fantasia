#ifndef FAN_PLATFORM_H
#define FAN_PLATFORM_H

# ifndef FAN_API
#  define FAN_API extern
# endif

typedef struct FanVector2 {
    float x;
    float y;
} FanVector2;

typedef struct FanColor {
    int r;
    int g;
    int b;
    int a;
} FanColor;

typedef struct FanRect {
    int x;
    int y;
    int width;
    int height;
} FanRect;

typedef struct FanTexture {
    unsigned int id;
    int width;
    int height;
    int mipmaps;
    int format;
} FanTexture;

typedef struct FanCamera2D {
    FanVector2 target;
    FanVector2 offset;
    float rotation;
    float zoom;
} FanCamera2D;

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
} FanKey;

typedef enum {
    FanLog_ALL     = 0,
    FanLog_TRACE   = 1,
    FanLog_DEBUG   = 2,
    FanLog_INFO    = 3,
    FanLog_WARNING = 4,
    FanLog_ERROR   = 5,
    FanLog_FATAL   = 6,
    FanLog_NONE    = 7
} FanLogLevel;

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
} FanWindowFlag;

#define FanColor_WHITE   (FanColor){ 210, 210, 210, 255 }
#define FanColor_GRAY    (FanColor){  80,  80,  80, 255 }
#define FanColor_BLACK   (FanColor){   0,   0,   0, 255 }
#define FanColor_RED     (FanColor){ 255,   0,   0, 255 }
#define FanColor_ORANGE  (FanColor){ 255, 165,   0, 255 }
#define FanColor_YELLOW  (FanColor){ 255, 255,   0, 255 }
#define FanColor_GREEN   (FanColor){   0, 255,   0, 255 }
#define FanColor_CYAN    (FanColor){   0, 255, 255, 255 }
#define FanColor_BLUE    (FanColor){   0,   0, 255, 255 }
#define FanColor_MAGENTA (FanColor){ 255,   0, 255, 255 }

FAN_API void FanWindowCreate(int width, int height, const char *title);
FAN_API void FanWindowClose(void);
FAN_API int FanWindowShouldClose(void);

FAN_API void FanWindowConfig(int);
FAN_API void FanSetLogLevel(int);

FAN_API int FanWindowWidth(void);
FAN_API int FanWindowHeight(void);

FAN_API double FanGetFrameTime(void);
FAN_API double FanGetTime(void);

FAN_API void FanRandomSeed(int seed);
FAN_API int FanRandomInt(int min, int max);

FAN_API int FanKeyPressed(FanKey key);
FAN_API int FanKeyDown(FanKey key);

FAN_API FanTexture FanTextureLoad(const char *filepath);
FAN_API void FanTextureUnload(FanTexture texture);

FAN_API void FanDrawBegin(void);
FAN_API void FanDrawClear(FanColor color);
FAN_API void FanDrawFPS(int x, int y);
FAN_API void FanDrawEnd(void);

FAN_API void FanDrawRect(int x, int y, int w, int h, FanColor color);
FAN_API void FanDrawRectV(FanVector2 pos, FanVector2 scale, FanColor color);
FAN_API void FanDrawRectR(FanRect rect, FanColor color);
FAN_API void FanDrawTexture(FanTexture texture, FanRect src, FanRect dst, FanVector2 origin, float angle, FanColor);

FAN_API void FanCameraBegin(FanCamera2D);
FAN_API void FanCameraEnd(void);

// quick maths

FAN_API float FanClamp(float value, float min, float max);
FAN_API float FanLerp(float a, float x, float b);
FAN_API int   FanFloat32Equals(float x, float y);
FAN_API float FanFloat32Inf(void);
FAN_API float FanFloat32NegativeInf(void);

FAN_API FanVector2 FanVector2Zero(void);
FAN_API FanVector2 FanVector2One(void);

FAN_API FanVector2 FanVector2Add(FanVector2 v1, FanVector2 v2);
FAN_API FanVector2 FanVector2AddValue(FanVector2 v, float x);

FAN_API FanVector2 FanVector2Sub(FanVector2 v1, FanVector2 v2);
FAN_API FanVector2 FanVector2SubValue(FanVector2 v, float x);

FAN_API FanVector2 FanVector2Normalize(FanVector2 v);

FAN_API float FanVector2Length(FanVector2 v);
FAN_API float FanVector2LengthSqr(FanVector2 v);

FAN_API FanVector2 FanVector2Scale(FanVector2 v, float scale);
FAN_API FanVector2 FanVector2Negate(FanVector2 v);

FAN_API float FanVector2Dot(FanVector2 v1, FanVector2 v2);
FAN_API float FanVector2Cross(FanVector2 v1, FanVector2 v2);

// custom api

FAN_API int FanRectIsEmpty(FanRect rect);

#endif
