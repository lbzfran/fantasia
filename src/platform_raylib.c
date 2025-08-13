
#include "platform.h"

#define RAYLIB_IMPLEMENTATION
#include <raylib.h>
#include <math.h>

Color FanColorToRL(FanColor color) {
    Color rl_color = (Color){
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a
    };

    return rl_color;
}

Sound FanSoundToRL(FanSound sound) {
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

Music FanMusicToRL(FanMusic music) {
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

void FanWindowCreate(int width, int height, const char *title) {
    InitWindow(width, height, title);
}

void FanWindowClose(void) {
    CloseWindow();
}

int FanWindowShouldClose(void) {
    int result = WindowShouldClose();
    return result;
}

void FanSetLogLevel(int level) {
    SetTraceLogLevel(level);
}

int FanWindowWidth(void) {
    int result = GetScreenWidth();
    return result;
}
int FanWindowHeight(void) {
    int result = GetScreenHeight();
    return result;
}


void FanAudioDevCreate(void) {
    InitAudioDevice();
}

void FanAudioDevClose(void) {
    CloseAudioDevice();
}

FanSound FanSoundLoad(const char *filepath) {
    Sound rl_sound = LoadSound(filepath);

    FanSound sound = (FanSound) {
        .frame_count = rl_sound.frameCount,
        .stream = (FanAudioStream) {
            rl_sound.stream.buffer,
            rl_sound.stream.processor,
            rl_sound.stream.sampleRate,
            rl_sound.stream.sampleSize,
            rl_sound.stream.channels,
        },
    };

    return sound;
}

void FanSoundUnload(FanSound sound) {
    Sound rl_sound = FanSoundToRL(sound);

    UnloadSound(rl_sound);
}

void FanSoundPlay(FanSound sound) {
    Sound rl_sound = FanSoundToRL(sound);

    PlaySound(rl_sound);
}

void FanSoundStop(FanSound sound) {
    Sound rl_sound = FanSoundToRL(sound);

    StopSound(rl_sound);
}

void FanSoundPause(FanSound sound) {
    Sound rl_sound = FanSoundToRL(sound);

    PauseSound(rl_sound);
}

void FanSoundResume(FanSound sound) {
    Sound rl_sound = FanSoundToRL(sound);

    ResumeSound(rl_sound);
}

void FanSoundSetVolume(FanSound sound, float volume) {
    Sound rl_sound = FanSoundToRL(sound);

    SetSoundVolume(rl_sound, volume);
}

void FanSoundSetPitch(FanSound sound, float pitch) {
    Sound rl_sound = FanSoundToRL(sound);

    SetSoundPitch(rl_sound, pitch);
}

void FanSoundSetPan(FanSound sound, float pan) {
    Sound rl_sound = FanSoundToRL(sound);

    SetSoundPan(rl_sound, pan);
}

FanMusic FanMusicLoad(const char *filepath) {
    Music rl_music = LoadMusicStream(filepath);

    FanMusic music = (FanMusic) {
        .frame_count = rl_music.frameCount,
        .stream = (FanAudioStream) {
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

void FanMusicUnload(FanMusic music) {
    Music rl_music = FanMusicToRL(music);

    UnloadMusicStream(rl_music);
}

void FanMusicPlay(FanMusic music) {
    Music rl_music = FanMusicToRL(music);

    PlayMusicStream(rl_music);
}

void FanMusicStop(FanMusic music) {
    Music rl_music = FanMusicToRL(music);

    StopMusicStream(rl_music);
}

void FanMusicPause(FanMusic music) {
    Music rl_music = FanMusicToRL(music);

    PauseMusicStream(rl_music);
}

void FanMusicResume(FanMusic music) {
    Music rl_music = FanMusicToRL(music);

    ResumeMusicStream(rl_music);
}

void FanMusicSeek(FanMusic music, float pos) {
    Music rl_music = FanMusicToRL(music);

    SeekMusicStream(rl_music, pos);
}

void FanMusicSetVolume(FanMusic music, float volume) {
    Music rl_music = FanMusicToRL(music);

    SetMusicVolume(rl_music, volume);
}

void FanMusicSetPitch(FanMusic music, float pitch) {
    Music rl_music = FanMusicToRL(music);

    SetMusicPitch(rl_music, pitch);
}

void FanMusicSetPan(FanMusic music, float pan) {
    Music rl_music = FanMusicToRL(music);

    SetMusicPan(rl_music, pan);
}

float FanMusicTimePlayed(FanMusic music) {
    Music rl_music = FanMusicToRL(music);

    return GetMusicTimePlayed(rl_music);
}

float FanMusicTimeLength(FanMusic music) {
    Music rl_music = FanMusicToRL(music);

    return GetMusicTimeLength(rl_music);
}

float FanGetFrameTime(void) {
    float result = GetFrameTime();
    return result;
}

float FanGetTime(void) {
    float result = GetTime();
    return result;
}

int FanGetFPS(void) {
    float result = GetFPS();
    return result;
}

void FanRandomSeed(int seed) {
    SetRandomSeed(seed);
}

int FanRandomInt(int min, int max) {
    int result = GetRandomValue(min, max);
    return result;
}

int FanKeyPressed(FanKey key) {
    int result = IsKeyPressed(key);
    return result;
}

int FanKeyDown(FanKey key) {
    int result = IsKeyDown(key);
    return result;
}

FanTexture FanTextureLoad(const char *filepath) {
    Texture2D rl_texture = LoadTexture(filepath);
    FanTexture texture = (FanTexture){
        .id = rl_texture.id,
        .width = rl_texture.width,
        .height = rl_texture.height,
        .mipmaps = rl_texture.mipmaps,
        .format = rl_texture.format
    };
    return texture;
}
void FanTextureUnload(FanTexture texture) {
    Texture2D rl_texture = (Texture2D){
        .id = texture.id,
        .width = texture.width,
        .height = texture.height,
        .mipmaps = texture.mipmaps,
        .format = texture.format
    };
    UnloadTexture(rl_texture);
}

void FanDrawBegin(void) {
    BeginDrawing();
}

void FanDrawClear(FanColor color) {
    Color rl_color = FanColorToRL(color);
    ClearBackground(rl_color);
}

void FanDrawFPS(int x, int y) {
    DrawFPS(x, y);
}

void FanDrawEnd(void) {
    EndDrawing();
}

void FanDrawRect(int x, int y, int w, int h, FanColor color) {
    Color rl_color = FanColorToRL(color);

    DrawRectangle(x, y, w, h, rl_color);
}

void FanDrawRectV(FanVector2 pos, FanVector2 scale, FanColor color) {
    FanDrawRect(pos.x, pos.y, scale.x, scale.y, color);
}

void FanDrawRectR(FanRect rect, FanColor color) {
    FanDrawRect(rect.x, rect.y, rect.width, rect.height, color);
}

void FanDrawTexture(FanTexture texture, FanRect src, FanRect dst, FanVector2 origin, float angle, FanColor color) {
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
    Color rl_color = FanColorToRL(color);
    DrawTexturePro(rl_texture, rl_src, rl_dst, rl_origin, angle, rl_color);
}

void FanCameraBegin(FanCamera2D camera) {
    Camera2D rl_camera = (Camera2D){
        .target = (Vector2){ camera.target.x, camera.target.y },
        .offset = (Vector2){ camera.offset.x, camera.offset.y },
        .rotation = camera.rotation,
        .zoom = camera.zoom
    };

    BeginMode2D(rl_camera);
};

void FanCameraEnd(void) {
    EndMode2D();
}
