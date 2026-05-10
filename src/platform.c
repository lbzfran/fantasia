
#include "os.h"
#include "platform.h"
#include <dirent.h>

#define FAN_PLATFORM_RAYLIB
#ifdef  FAN_PLATFORM_RAYLIB
 # include "platform_raylib.c"
#endif

#include "os.c"
#include "core.c"

// NOTE(liam): internal-only import of stbds.
#define FAN_PLATFORM_STBDS
#ifdef  FAN_PLATFORM_STBDS
# define STBDS_NO_SHORT_NAMES
# define STBDS_SIPHASH_2_4
# define STB_DS_IMPLEMENTATION
# include "stb_ds.h"
# define fan_ht_hmdefault stbds_hmdefault
# define fan_ht_shput stbds_shput
# define fan_ht_shget stbds_shget
# define fan_ht_shlen stbds_shlen
#endif

inline void fan_dsl_array_append(fan_allocator *mem, fan_dsl_token_array *arr, fan_dsl_token x) {
    assume(mem->resize != nullptr);
    if (arr->size >= arr->capacity) {
        ssize new_cap = max(arr->capacity * 2, FAN_ARRAY_INITIAL_CAPACITY) * sizeof(fan_dsl_token);
        fan_dsl_token *new_data = mem->resize(mem->ctx, arr->data, arr->capacity, new_cap);
        assume(new_data != nullptr);
        arr->data = new_data;
        arr->capacity = new_cap;
    }
    arr->data[arr->size++] = x;
}

fan_dsl_token_array fan_dsl_tokenize(fan_allocator *mem, fan_str8 buf) {
    fan_dsl_token_array result = {};
    while (true) {
        buf = fan_str8_triml(buf);
        fan_cutstr8 cut;

        cut = fan_str8_cut(buf, ' ');
        if (!cut.ok) break;
        fan_dsl_token type = { FanToken_TYPE, cut.head };
        buf = fan_str8_triml(cut.tail);

        fan_dsl_array_append(mem, &result, type);

        cut = fan_str8_cut(buf, ' ');
        if (!cut.ok) break;
        fan_dsl_token name = { FanToken_NAME, cut.head };
        buf = fan_str8_triml(cut.tail);

        fan_dsl_array_append(mem, &result, name);

        cut = fan_str8_cut(buf, ' ');
        if (!cut.ok) break;
        assume(fan_str8_equals(cut.head, fan_str8_cstr("{")));
        fan_dsl_token lb = { FanToken_LBRACKET, cut.head };
        buf = fan_str8_triml(cut.tail);

        fan_dsl_array_append(mem, &result, lb);

        // TODO(liam): below cut fails to find end of file.
        cut = fan_str8_cut(buf, ' ');
        if (!cut.ok) break;
        assume(fan_str8_equals(cut.head, fan_str8_cstr("}")));
        fan_dsl_token rb = { FanToken_RBRACKET, cut.head };
        buf = fan_str8_triml(cut.tail);

        fan_dsl_array_append(mem, &result, rb);

        break;
    }

    for (ssize i = 0; i < result.size; i++) {
        fan_str8 dat = result.data[i].literal;
        printf("LITERAL: %.*s\n", (int32)dat.length, dat.data);
    }

    return result;
}

// NOTE(liam): loads all assets within a directory AS sprites.
static fan_str8 fan_str8_strip_ext(fan_str8 s) {
    for (ssize i = s.length; i > 0; i--) {
        if (s.data[i - 1] == '.') {
            s.length = i - 1;
            break;
        }
    }
    return s;
}

static ssize fan_str8_copy(char8 *dst, fan_str8 src) {
    for (ssize i = 0; i < src.length; i++) {
        dst[i] = src.data[i];
    }
    return src.length;
}

void fan_sprite_init(fan_asset *assets, fan_texture fallback) {
    assets->sprites = nullptr;
    assets->default_sprite = fallback;
}

void fan_sprite_unload(fan_asset *assets, fan_allocator *mem) {
    fan_asset_sprite_entry *table = assets->sprites;
    for (ssize i = 0; i < fan_ht_shlen(table); i++) {
        mem->free(mem->ctx, table[i].key, fan_cstr_length(table[i].key) + 1);
        fan_texture_unload(table[i].value);
    }
}

void fan_sprite_load(fan_asset *assets, fan_allocator *mem, char8 *const path) {
    fan_asset_sprite_entry *table = assets->sprites;

    struct dirent *dp;
    DIR *dir = opendir(path);
    assert(dir);

    char8 full_path[262];

    while ((dp = readdir(dir))) {
        if (!dp->d_name[0] || dp->d_name[0] == '.') continue;

        fan_str8 file_name = fan_str8_cstrv(dp->d_name);
        fan_str8 base_name = fan_str8_strip_ext(file_name);

        ssize path_len = fan_str8_copy(full_path, fan_str8_cstrv(path));
        full_path[path_len++] = '/';
        fan_str8_copy(full_path + path_len, file_name);
        full_path[path_len + file_name.length] = '\0';
        fan_texture tex = fan_texture_load(full_path);

        ssize key_len = base_name.length;
        char8 *key = mem->make(mem->ctx, key_len + 1);
        fan_str8_copy(key, base_name);
        key[key_len] = '\0';

        fan_ht_shput(table, key, tex);
    }

    assets->sprites = table;
}

fan_texture fan_sprite_get(fan_asset *assets, char8 *const name) {
    fan_texture tex = fan_ht_shget(assets->sprites, name);
    if (tex.id == 0) {
        return assets->default_sprite;
    }
    return tex;
}
