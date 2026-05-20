
#include "platform.h"

#include <raylib.h>

Color fan_color_rl(fan_color color) {
    Color rl_color = (Color){
        .r = (unsigned char)color.r,
        .g = (unsigned char)color.g,
        .b = (unsigned char)color.b,
        .a = (unsigned char)color.a
    };

    return rl_color;
}

Sound fan_sound_rl(fan_sound sound) {
    Sound rl_sound = (Sound) {
        .frameCount = sound.frame_count,
        .stream = (AudioStream) {
            sound.stream.buffer,
            sound.stream.processor,
            sound.stream.sample_rate,
            sound.stream.sample_size,
            sound.stream.channels,
        },
    };
    return rl_sound;
}

Music fan_music_rl(fan_music music) {
    Music rl_music = (Music) {
        .frameCount = music.frame_count,
        .stream = (AudioStream) {
            music.stream.buffer,
            music.stream.processor,
            music.stream.sample_rate,
            music.stream.sample_size,
            music.stream.channels,
        },
        .ctxType = music.ctx_type,
        .ctxData = music.ctx_data,
    };
    return rl_music;
}

Texture2D fan_texture_rl(fan_texture tx) {
    Texture2D rl_texture = (Texture2D){
        .id      = tx.id,
        .width   = tx.width,
        .height  = tx.height,
        .mipmaps = tx.mipmaps,
        .format  = tx.format
    };
    return rl_texture;
}

RenderTexture fan_rtexture_rl(fan_rtexture rtx) {
    Texture2D rl_texture = fan_texture_rl(rtx.texture);
    Texture2D rl_depth = fan_texture_rl(rtx.depth);

    return (RenderTexture) {
        .texture = rl_texture,
        .depth   = rl_depth,
        .id      = rtx.id
    };
}

void fan_window_create(int32 width, int32 height, const char *title) {
    InitWindow(width, height, title);
}

void fan_window_close(void) {
    CloseWindow();
}

void fan_os_wait(uint32 ms) {
    WaitTime((float)ms / 1000.0f);
}

int32 fan_window_shouldclose(void) {
    int32 result = WindowShouldClose();
    return result;
}

void fan_log_set(int32 level) {
    SetTraceLogLevel(level);
}

int32 fan_window_width(void) {
    int32 result = GetScreenWidth();
    return result;
}
int32 fan_window_height(void) {
    int32 result = GetScreenHeight();
    return result;
}

void fan_window_config(int32 flags) {
    SetConfigFlags(flags);
}

bool32 fan_window_resized(void) {
    bool32 result = (bool32)IsWindowResized();
    return result;
}

void fan_dev_audio_create(void) {
    InitAudioDevice();
}

void fan_dev_audio_close(void) {
    CloseAudioDevice();
}

fan_sound fan_sound_load(const char *filepath) {
    Sound rl_sound = LoadSound(filepath);

    fan_sound sound = (fan_sound) {
        .frame_count = rl_sound.frameCount,
        .stream = (fan_audio_stream) {
            rl_sound.stream.buffer,
            rl_sound.stream.processor,
            rl_sound.stream.sampleRate,
            rl_sound.stream.sampleSize,
            rl_sound.stream.channels,
        },
    };

    return sound;
}

void fan_sound_unload(fan_sound sound) {
    Sound rl_sound = fan_sound_rl(sound);

    UnloadSound(rl_sound);
}

void fan_sound_play(fan_sound sound) {
    Sound rl_sound = fan_sound_rl(sound);

    PlaySound(rl_sound);
}

void fan_sound_stop(fan_sound sound) {
    Sound rl_sound = fan_sound_rl(sound);

    StopSound(rl_sound);
}

void fan_sound_pause(fan_sound sound) {
    Sound rl_sound = fan_sound_rl(sound);

    PauseSound(rl_sound);
}

void fan_sound_resume(fan_sound sound) {
    Sound rl_sound = fan_sound_rl(sound);

    ResumeSound(rl_sound);
}

void fan_sound_volume_set(fan_sound sound, float32 volume) {
    Sound rl_sound = fan_sound_rl(sound);

    SetSoundVolume(rl_sound, volume);
}

void fan_sound_pitch_set(fan_sound sound, float32 pitch) {
    Sound rl_sound = fan_sound_rl(sound);

    SetSoundPitch(rl_sound, pitch);
}

void fan_sound_pan_set(fan_sound sound, float32 pan) {
    Sound rl_sound = fan_sound_rl(sound);

    SetSoundPan(rl_sound, pan);
}

fan_music fan_music_load(const char *filepath) {
    Music rl_music = LoadMusicStream(filepath);

    fan_music music = (fan_music) {
        .frame_count = rl_music.frameCount,
        .stream = (fan_audio_stream) {
            rl_music.stream.buffer,
            rl_music.stream.processor,
            rl_music.stream.sampleRate,
            rl_music.stream.sampleSize,
            rl_music.stream.channels,
        },
        .ctx_type = rl_music.ctxType,
        .ctx_data = rl_music.ctxData,
    };

    return music;
}

void fan_music_unload(fan_music music) {
    Music rl_music = fan_music_rl(music);

    UnloadMusicStream(rl_music);
}

void fan_music_update(fan_music music) {
    Music rl_music = fan_music_rl(music);

    UpdateMusicStream(rl_music);
}

void fan_music_play(fan_music music) {
    Music rl_music = fan_music_rl(music);

    PlayMusicStream(rl_music);
}

void fan_music_stop(fan_music music) {
    Music rl_music = fan_music_rl(music);

    StopMusicStream(rl_music);
}

void fan_music_pause(fan_music music) {
    Music rl_music = fan_music_rl(music);

    PauseMusicStream(rl_music);
}

void fan_music_resume(fan_music music) {
    Music rl_music = fan_music_rl(music);

    ResumeMusicStream(rl_music);
}

void fan_music_seek(fan_music music, float32 pos) {
    Music rl_music = fan_music_rl(music);

    SeekMusicStream(rl_music, pos);
}

void fan_music_volume_set(fan_music music, float32 volume) {
    Music rl_music = fan_music_rl(music);

    SetMusicVolume(rl_music, volume);
}

void fan_music_pitch_set(fan_music music, float32 pitch) {
    Music rl_music = fan_music_rl(music);

    SetMusicPitch(rl_music, pitch);
}

void fan_music_pan_set(fan_music music, float32 pan) {
    Music rl_music = fan_music_rl(music);

    SetMusicPan(rl_music, pan);
}

float32 fan_music_time_played(fan_music music) {
    Music rl_music = fan_music_rl(music);

    return GetMusicTimePlayed(rl_music);
}

float32 fan_music_time_length(fan_music music) {
    Music rl_music = fan_music_rl(music);

    return GetMusicTimeLength(rl_music);
}

float32 fan_frametime_get(void) {
    float32 result = GetFrameTime();
    return result;
}

float32 fan_time_get(void) {
    float32 result = (float32)GetTime();
    return result;
}

int32 fan_fps_get(void) {
    int32 result = GetFPS();
    return result;
}

void fan_random_seed(int32 seed) {
    SetRandomSeed(seed);
}

int32 fan_random_int(int32 min, int32 max) {
    int32 result = GetRandomValue(min, max);
    return result;
}

bool32 fan_key_pressed(fan_key key) {
    bool32 result = IsKeyPressed(key);
    return result;
}

bool32 fan_key_down(fan_key key) {
    bool32 result = IsKeyDown(key);
    return result;
}

fan_key fan_key_current_char(void) {
    return GetCharPressed();
}

fan_key fan_key_current(void) {
    return GetKeyPressed();
}

fan_texture fan_texture_load(const char *filepath) {
    Texture2D rl_texture = LoadTexture(filepath);
    fan_texture texture = (fan_texture){
        .id      = rl_texture.id,
        .width   = rl_texture.width,
        .height  = rl_texture.height,
        .mipmaps = rl_texture.mipmaps,
        .format  = rl_texture.format
    };
    return texture;
}
void fan_texture_unload(fan_texture texture) {
    Texture2D rl_texture = fan_texture_rl(texture);
    UnloadTexture(rl_texture);
}

void fan_draw_begin(void) {
    BeginDrawing();
}

void fan_draw_clear(fan_color color) {
    Color rl_color = fan_color_rl(color);
    ClearBackground(rl_color);
}

void fan_draw_fps(int32 x, int32 y) {
    DrawFPS(x, y);
}

void fan_draw_end(void) {
    EndDrawing();
}

void fan_draw_rect(int32 x, int32 y, int32 w, int32 h, fan_color color) {
    Color rl_color = fan_color_rl(color);

    DrawRectangle(x, y, w, h, rl_color);
}

void fan_draw_rectv(fan_vec2 pos, fan_vec2 scale, fan_vec2 origin, float32 rotation, fan_color color) {
    Vector2 rl_origin = {
        origin.x,
        origin.y
    };

    Rectangle rl_rect = {
        pos.x,
        pos.y,
        scale.x,
        scale.y
    };


    Color rl_color = fan_color_rl(color);

    DrawRectanglePro(rl_rect, rl_origin, rotation * RAD2DEG, rl_color);
}

void fan_draw_rectr(fan_rect rect, fan_color color) {
    fan_draw_rect((int32)rect.x, (int32)rect.y, (int32)rect.w, (int32)rect.h, color);
}

void fan_draw_line(int32 sx, int32 sy, int32 ex, int32 ey, fan_color color) {
    Color rl_color = fan_color_rl(color);

    DrawLine(sx, sy, ex, ey, rl_color);
}

void fan_draw_linev(fan_vec2 start, fan_vec2 end, fan_color color) {
    fan_draw_line((int32)start.x, (int32)start.y, (int32)end.x, (int32)end.y, color);
}

void fan_draw_texture(fan_texture texture, fan_rect src, fan_rect dst, fan_vec2 origin, float32 angle, fan_color color) {
    Texture2D rl_texture = (Texture2D){
        .id      = texture.id,
        .width   = texture.width,
        .height  = texture.height,
        .mipmaps = texture.mipmaps,
        .format  = texture.format
    };
    Rectangle rl_src = (Rectangle){
        .x      = src.x,
        .y      = src.y,
        .width  = src.w,
        .height = src.h
    };
    Rectangle rl_dst = (Rectangle){
        .x      = dst.x,
        .y      = dst.y,
        .width  = dst.w,
        .height = dst.h
    };
    Vector2 rl_origin = (Vector2){
        .x = origin.x,
        .y = origin.y
    };
    Color rl_color = fan_color_rl(color);
    DrawTexturePro(rl_texture, rl_src, rl_dst, rl_origin, angle, rl_color);
}

void fan_draw_text(char *buf, fan_vec2 origin, fan_color text_color, fan_color background_color) {
    // DrawTextPro();
    (void)background_color;
    Color rl_text_color = fan_color_rl(text_color);
    DrawText(buf, (int32)origin.x, (int32)origin.y, 14, rl_text_color);
}

void fan_camera_begin(fan_camera2D camera) {
    Camera2D rl_camera = (Camera2D){
        .target   = (Vector2){ camera.target.x, camera.target.y },
        .offset   = (Vector2){ camera.offset.x, camera.offset.y },
        .rotation = camera.rotation,
        .zoom     = camera.zoom
    };

    BeginMode2D(rl_camera);
};

void fan_camera_end(void) {
    EndMode2D();
}

fan_rtexture fan_rtexture_load(int32 width, int32 height) {
    RenderTexture rl_rtx = LoadRenderTexture(width, height);

    fan_texture texture = (fan_texture){
        .id      = rl_rtx.texture.id,
        .width   = rl_rtx.texture.width,
        .height  = rl_rtx.texture.height,
        .mipmaps = rl_rtx.texture.mipmaps,
        .format  = rl_rtx.texture.format
    };

    fan_texture depth = (fan_texture){
        .id      = rl_rtx.depth.id,
        .width   = rl_rtx.depth.width,
        .height  = rl_rtx.depth.height,
        .mipmaps = rl_rtx.depth.mipmaps,
        .format  = rl_rtx.depth.format
    };

    fan_rtexture rtx = (fan_rtexture) {
        .id      = rl_rtx.id,
        .texture = texture,
        .depth   = depth
    };

    return rtx;
}

void fan_rtexture_unload(fan_rtexture rtx) {
    RenderTexture rl_rtx = fan_rtexture_rl(rtx);

    UnloadRenderTexture(rl_rtx);
}

void fan_mode_texture_begin(fan_rtexture rtx) {
    RenderTexture rl_rtx = fan_rtexture_rl(rtx);

    BeginTextureMode(rl_rtx);
}
void fan_mode_texture_end(void) {
    EndTextureMode();
}

void fan_mode_blend_begin(int32 mode) {
    BeginBlendMode(mode);
}

void fan_mode_blend_end(void) {
    EndBlendMode();
}

void fan_draw_pixel(int32 x, int32 y, fan_color color) {
    Color rl_color = fan_color_rl(color);

    DrawPixel(x, y, rl_color);
}

void fan_draw_circle(int32 x, int32 y, float32 r, fan_color color) {
    Color rl_color = fan_color_rl(color);

    DrawCircle(x, y, r, rl_color);
}

void fan_draw_circle_grad(int32 x, int32 y, float32 r, fan_color in, fan_color out) {
    Color rl_in = fan_color_rl(in);
    Color rl_out = fan_color_rl(out);

    DrawCircleGradient((Vector2){ (float32)x, (float32)y }, r, rl_in, rl_out);
}

void fan_fps_target(int32 fps) {
    SetTargetFPS(fps);
}
