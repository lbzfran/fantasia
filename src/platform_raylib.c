
#include "platform.h"

#define RAYLIB_IMPLEMENTATION
#include <raylib.h>
#include <math.h>

Color fan_color_rl(fan_color color) {
    Color rl_color = (Color){
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a
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

Music FanMusicToRL(fan_music music) {
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

Texture2D FanTextureToRL(fan_texture tx) {
    Texture2D rl_texture = (Texture2D){
        .id      = tx.id,
        .width   = tx.width,
        .height  = tx.height,
        .mipmaps = tx.mipmaps,
        .format  = tx.format
    };
    return rl_texture;
}

RenderTexture FanRTextureToRL(fan_rtexture rtx) {
    Texture2D rl_texture = FanTextureToRL(rtx.texture);
    Texture2D rl_depth = FanTextureToRL(rtx.depth);

    return (RenderTexture) {
        .texture = rl_texture,
        .depth   = rl_depth,
        .id      = rtx.id
    };
}

void fan_window_create(int width, int height, const char *title) {
    InitWindow(width, height, title);
}

void fan_window_close(void) {
    CloseWindow();
}

int fan_window_shouldclose(void) {
    int result = WindowShouldClose();
    return result;
}

void fan_log_set(int level) {
    SetTraceLogLevel(level);
}

int fan_window_width(void) {
    int result = GetScreenWidth();
    return result;
}
int fan_window_height(void) {
    int result = GetScreenHeight();
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

void fan_sound_volume_set(fan_sound sound, float volume) {
    Sound rl_sound = fan_sound_rl(sound);

    SetSoundVolume(rl_sound, volume);
}

void fan_sound_pitch_set(fan_sound sound, float pitch) {
    Sound rl_sound = fan_sound_rl(sound);

    SetSoundPitch(rl_sound, pitch);
}

void fan_sound_pan_set(fan_sound sound, float pan) {
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
    Music rl_music = FanMusicToRL(music);

    UnloadMusicStream(rl_music);
}

void fan_music_update(fan_music music) {
    Music rl_music = FanMusicToRL(music);

    UpdateMusicStream(rl_music);
}

void fan_music_play(fan_music music) {
    Music rl_music = FanMusicToRL(music);

    PlayMusicStream(rl_music);
}

void fan_music_stop(fan_music music) {
    Music rl_music = FanMusicToRL(music);

    StopMusicStream(rl_music);
}

void fan_music_pause(fan_music music) {
    Music rl_music = FanMusicToRL(music);

    PauseMusicStream(rl_music);
}

void fan_music_resume(fan_music music) {
    Music rl_music = FanMusicToRL(music);

    ResumeMusicStream(rl_music);
}

void fan_music_seek(fan_music music, float pos) {
    Music rl_music = FanMusicToRL(music);

    SeekMusicStream(rl_music, pos);
}

void fan_music_volume_set(fan_music music, float volume) {
    Music rl_music = FanMusicToRL(music);

    SetMusicVolume(rl_music, volume);
}

void fan_music_pitch_set(fan_music music, float pitch) {
    Music rl_music = FanMusicToRL(music);

    SetMusicPitch(rl_music, pitch);
}

void fan_music_pan_set(fan_music music, float pan) {
    Music rl_music = FanMusicToRL(music);

    SetMusicPan(rl_music, pan);
}

float fan_music_time_played(fan_music music) {
    Music rl_music = FanMusicToRL(music);

    return GetMusicTimePlayed(rl_music);
}

float fan_music_time_length(fan_music music) {
    Music rl_music = FanMusicToRL(music);

    return GetMusicTimeLength(rl_music);
}

float fan_frametime_get(void) {
    float result = GetFrameTime();
    return result;
}

float fan_time_get(void) {
    float result = GetTime();
    return result;
}

int fan_fps_get(void) {
    float result = GetFPS();
    return result;
}

void fan_random_seed(int seed) {
    SetRandomSeed(seed);
}

int fan_random_int(int min, int max) {
    int result = GetRandomValue(min, max);
    return result;
}

int fan_key_pressed(fan_key key) {
    int result = IsKeyPressed(key);
    return result;
}

int fan_key_down(fan_key key) {
    int result = IsKeyDown(key);
    return result;
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
    Texture2D rl_texture = FanTextureToRL(texture);
    UnloadTexture(rl_texture);
}

void fan_draw_begin(void) {
    BeginDrawing();
}

void fan_draw_clear(fan_color color) {
    Color rl_color = fan_color_rl(color);
    ClearBackground(rl_color);
}

void fan_draw_fps(int x, int y) {
    DrawFPS(x, y);
}

void fan_draw_end(void) {
    EndDrawing();
}

void fan_draw_rect(int x, int y, int w, int h, fan_color color) {
    Color rl_color = fan_color_rl(color);

    DrawRectangle(x, y, w, h, rl_color);
}

void fan_draw_rectv(fan_vec2 pos, fan_vec2 scale, fan_color color) {
    fan_draw_rect(pos.x, pos.y, scale.x, scale.y, color);
}

void fan_draw_rectr(fan_rect_i32 rect, fan_color color) {
    fan_draw_rect(rect.x, rect.y, rect.width, rect.height, color);
}

void fan_draw_line(int sx, int sy, int ex, int ey, fan_color color) {
    Color rl_color = fan_color_rl(color);

    DrawLine(sx, sy, ex, ey, rl_color);
}

void fan_draw_linev(fan_vec2 start, fan_vec2 end, fan_color color) {
    fan_draw_line(start.x, start.y, end.x, end.y, color);
}

void fan_draw_texture(fan_texture texture, fan_rect_i32 src, fan_rect_i32 dst, fan_vec2 origin, float angle, fan_color color) {
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
        .width  = src.width,
        .height = src.height
    };
    Rectangle rl_dst = (Rectangle){
        .x      = dst.x,
        .y      = dst.y,
        .width  = dst.width,
        .height = dst.height
    };
    Vector2 rl_origin = (Vector2){
        .x = origin.x,
        .y = origin.y
    };
    Color rl_color = fan_color_rl(color);
    DrawTexturePro(rl_texture, rl_src, rl_dst, rl_origin, angle, rl_color);
}

void fan_camera_begin(fan_camera2D camera) {
    Camera2D rl_camera = (Camera2D){
        .target = (Vector2){ camera.target.x, camera.target.y },
        .offset = (Vector2){ camera.offset.x, camera.offset.y },
        .rotation = camera.rotation,
        .zoom = camera.zoom
    };

    BeginMode2D(rl_camera);
};

void fan_camera_end(void) {
    EndMode2D();
}

fan_rtexture fan_rtexture_load(int width, int height) {
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
    RenderTexture rl_rtx = FanRTextureToRL(rtx);

    UnloadRenderTexture(rl_rtx);
}

void fan_mode_texture_begin(fan_rtexture rtx) {
    RenderTexture rl_rtx = FanRTextureToRL(rtx);

    BeginTextureMode(rl_rtx);
}
void fan_mode_texture_end(void) {
    EndTextureMode();
}

void fan_mode_blend_begin(int mode) {
    BeginBlendMode(mode);
}

void fan_mode_blend_end(void) {
    EndBlendMode();
}
void fan_draw_circle_grad(int x, int y, float r, fan_color in, fan_color out) {
    Color rl_in = fan_color_rl(in);
    Color rl_out = fan_color_rl(out);

    DrawCircleGradient(x, y, r, rl_in, rl_out);
}
