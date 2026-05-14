
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

        ssize path_len = fan_cstr_copy_str8(full_path, fan_str8_cstrv(path));
        full_path[path_len++] = '/';
        fan_cstr_copy_str8(full_path + path_len, file_name);
        full_path[path_len + file_name.length] = '\0';
        fan_texture tex = fan_texture_load(full_path);

        ssize key_len = base_name.length;
        char8 *key = mem->make(mem->ctx, key_len + 1);
        fan_cstr_copy_str8(key, base_name);
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

// djb2 hash by Dan Bernstein
usize fan_ht_hash_str8(fan_str8 buf) {
    usize result = 5381;
    int32 c;

    while (buf.length--) {
        c = *(buf.data++);
        result = ((result << 5) + result) + c;
    }

    return result;
}

void fan_ht_setdefault(fan_str8 buf, fan_ht *ht, fan_allocator *mem) {
    ht->default_entry = fan_str8_copy(buf, mem);
}

void fan_ht_init(fan_ht *ht, ssize capacity, fan_allocator *mem) {
    assume(ht->capacity == 0 && "initialize only zeroed ht.");

    ht->table         = mem->make(mem->ctx, sizeof(fan_ht_entry) * capacity);
    ht->default_entry = (fan_str8){ 0 };
    ht->size          = 0;
    ht->capacity      = capacity;

    for (ssize i = 0; i < ht->capacity; i++) {
        ht->table[i] = (fan_ht_entry){ 0 };
    }
}

void fan_ht_free(fan_ht *ht, fan_allocator *mem) {
    if (ht->default_entry.length > 0) {
        mem->free(mem->ctx, ht->default_entry.data, ht->default_entry.length);
    }

    for (ssize i = 0; i < ht->capacity; i++) {
        fan_ht_entry *entry = &ht->table[i];

        if (entry->key.length > 0) {
            mem->free(mem->ctx, entry->key.data, entry->key.length);
        }

        if (entry->value.length > 0) {
            mem->free(mem->ctx, entry->value.data, entry->value.length);
        }
    }

    mem->free(mem->ctx, ht->table, sizeof(fan_ht_entry) * ht->capacity);

    ht->table    = nullptr;
    ht->default_entry = (fan_str8){ 0 };
    ht->size     = 0;
    ht->capacity = 0;
}

static fan_str8 fan_ht_put_(fan_str8 key, fan_str8 value, fan_ht *ht, fan_allocator *mem) {
    usize hash = fan_ht_hash_str8(key);
    usize index = (ssize)(hash & (ssize)(ht->capacity - 1));

    // printf("index: %zu, hash: %zu\n", index, hash);

    fan_ht_entry *table = ht->table;

    while (table[index].key.length != 0) {
        if (fan_str8_equals(key, table[index].key)) {
            if (table[index].value.length > 0) {
                mem->free(
                    mem->ctx,
                    table[index].value.data,
                    table[index].value.length
                );
            }

            table[index].value = fan_str8_copy(value, mem);
            return table[index].key;
        }

        index++;
        if (index >= ht->capacity) {
            index = 0;
        }
    }

    table[index].key = fan_str8_copy(key, mem);
    table[index].value = fan_str8_copy(value, mem);

    // fan_str8_print(table[index].key);
    // printf("\n");
    // fan_str8_print(table[index].value);
    // printf("\n");

    ht->size++;

    return table[index].key;
}

bool32 fan_ht_resize(fan_ht *ht, fan_allocator *mem) {
    ssize old_capacity = ht->capacity;
    fan_ht_entry *old_table = ht->table;

    ssize new_capacity = ht->capacity * 2;
    if (new_capacity < ht->capacity) {
        return false;
    }

    printf("expanding ht: %zu -> %zu\n", old_capacity, new_capacity);

    fan_ht_entry *new_entries =
        mem->make(mem->ctx, sizeof(fan_ht_entry) * new_capacity);

    assert(new_entries != nullptr);

    // fan_memory_set((uint8 *)new_entries, 0, sizeof(fan_ht_entry) * new_capacity);
    for (ssize i = 0; i < new_capacity; i++) {
        new_entries[i] = (fan_ht_entry){ 0 };
    }

    ht->table = new_entries;
    ht->capacity = new_capacity;
    ht->size = 0;

    for (ssize i = 0; i < old_capacity; i++) {
        fan_ht_entry entry = old_table[i];

        if (entry.key.data != nullptr) {
            usize hash = fan_ht_hash_str8(entry.key);
            usize index =
                (usize)(hash & (usize)(ht->capacity - 1));

            while (ht->table[index].key.data != nullptr) {
                index++;
                if (index >= ht->capacity) {
                    index = 0;
                }
            }

            ht->table[index] = entry;
            ht->size++;
        }
    }

    mem->free(
        mem->ctx,
        old_table,
        sizeof(fan_ht_entry) * old_capacity
    );

    return true;
}

fan_str8 fan_ht_get(fan_str8 key, fan_ht *ht) {
    if (ht->size == 0) {
        return ht->default_entry;
    }

    usize hash = fan_ht_hash_str8(key);
    usize index = (usize)(hash & (usize)(ht->capacity - 1));

    // printf("index: %zu, hash: %zu\n", index, hash);

    fan_ht_entry *table = ht->table;

    while (table[index].key.length != 0) {
        if (fan_str8_equals(key, table[index].key)) {
            return table[index].value;
        }
        index++;
        if (index >= ht->capacity) {
            index = 0;
        }
    }
    return ht->default_entry;
}

void fan_ht_put(fan_str8 key, fan_str8 value, fan_ht *ht, fan_allocator *mem) {
    assert(value.data != nullptr);
    if (value.length <= 0) {
        return;
    }

    if ((ht->size + 1) >= ht->capacity) {
        bool32 ok = fan_ht_resize(ht, mem);
        assert(ok);
    }

    fan_ht_put_(key, value, ht, mem);
}

bool32 fan_ht_delete(fan_str8 key, fan_ht *ht, fan_allocator *mem) {
    usize hash = fan_ht_hash_str8(key);
    usize index = (ssize)(hash & (ssize)(ht->capacity - 1));

    fan_ht_entry *table = ht->table;

    while (table[index].key.data != nullptr) {
        if (fan_str8_equals(key, table[index].key)) {
            fan_ht_entry removed = table[index];

            // free removed entry strings
            mem->free(
                mem->ctx,
                removed.key.data,
                removed.key.length
            );

            mem->free(
                mem->ctx,
                removed.value.data,
                removed.value.length
            );

            // clear slot
            table[index].key.data = nullptr;
            table[index].key.length = 0;
            table[index].value.data = nullptr;
            table[index].value.length = 0;

            ht->size--;

            // reinsert following cluster
            usize next = index + 1;
            if (next >= ht->capacity) {
                next = 0;
            }

            while (table[next].key.data != nullptr) {
                fan_ht_entry entry = table[next];

                table[next].key.data = nullptr;
                table[next].key.length = 0;
                table[next].value.data = nullptr;
                table[next].value.length = 0;

                ht->size--;

                fan_ht_put_(entry.key, entry.value, ht, mem);

                next++;
                if (next >= ht->capacity) {
                    next = 0;
                }
            }

            return true;
        }

        index++;
        if (index >= ht->capacity) {
            index = 0;
        }
    }

    return false;
}
