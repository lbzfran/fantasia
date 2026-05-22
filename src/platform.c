
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
        fan_dsl_token *new_data = fan_resize(mem, arr->data, arr->capacity, new_cap);
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
        assume(fan_str8_equal(cut.head, fan_str8_cstr("{")));
        fan_dsl_token lb = { FanToken_LBRACKET, cut.head };
        buf = fan_str8_triml(cut.tail);

        fan_dsl_array_append(mem, &result, lb);

        // TODO(liam): below cut fails to find end of file.
        cut = fan_str8_cut(buf, ' ');
        if (!cut.ok) break;
        assume(fan_str8_equal(cut.head, fan_str8_cstr("}")));
        fan_dsl_token rb = { FanToken_RBRACKET, cut.head };
        buf = fan_str8_triml(cut.tail);

        fan_dsl_array_append(mem, &result, rb);

        break;
    }

    for (ssize i = 0; i < result.size; i++) {
        fan_str8 dat = result.data[i].literal;
        fan_log_debug("LITERAL: %.*s\n", (int32)dat.length, dat.data);
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
        fan_free(mem, table[i].key, fan_cstr_length(table[i].key) + 1);
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
        char8 *key = fan_make(mem, key_len + 1);
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
// NOTE(liam): result must be unsigned.
usize fan_hash_str8(fan_str8 buf) {
    usize result = 5381;
    int32 c;

    while (buf.length--) {
        c = *(buf.data++);
        result = ((result << 5) + result) + c;
    }

    return result;
}

// NOTE(liam): fnv-a1 algorithm.
usize fan_hash_bytes(const void *ptr, usize len) {
    const uint8 *bytes = (const uint8 *)ptr;

    uint64_t hash = 14695981039346656037ULL;
    for (usize i = 0; i < len; i++) {
        hash ^= (usize)bytes[i];
        hash *= 1099511628211ULL;
    }

    return hash;
}

static inline ssize fan_ht_entry_size(fan_ht_header *h) {
    return sizeof(fan_str8) + h->value_size;
}

static inline fan_ht_header *fan_ht_header_get(void *table) {
    return (fan_ht_header *)((uint8 *)table - sizeof(fan_ht_header));
}

static inline fan_str8 *fan_ht_key(fan_ht_header *h, void *entry) {
    return (fan_str8 *)((uint8 *)entry + h->key_offset);
}

static inline void *fan_ht_value(fan_ht_header *h, void *entry) {
    return (void *)((uint8 *)entry + h->key_offset + sizeof(fan_str8));
}

static inline void *fan_ht_at(fan_ht_header *h, ssize index) {
    ssize entry_size = fan_ht_entry_size(h);
    return (void *)((uint8 *)h + sizeof(fan_ht_header) + index * entry_size);
}

void *fan_ht_create(ssize value_size, ssize capacity, void *default_value, fan_allocator *mem) {
    assert(is_power_of_two(capacity));
    ssize entry_size = sizeof(fan_str8) + value_size;
    ssize total = sizeof(fan_ht_header) + (capacity * entry_size);

    fan_ht_header *header = fan_make(mem, total);
    fan_log_debug("created header.\n");
    fan_memory_set((uint8 *)header, 0, total);
    if (header is nullptr) {
        return nullptr;
    }

    header->size = 0;
    header->capacity = capacity;
    header->key_offset = offsetof(fan_ht_entry_str8, key);
    header->value_size = value_size;
    header->default_value = default_value;
    // header->default_value = fan_memory_copy(header->default_value, default_value, value_size);

    void *table = (void *)((uint8 *)header + sizeof(fan_ht_header));

    return table;
}

void fan_ht_free(void *table, fan_allocator *mem) {
    assume(table isnt nullptr);
    fan_ht_header *h = fan_ht_header_get(table);
    ssize entry_size = fan_ht_entry_size(h);
    ssize total = sizeof(fan_ht_header) + (h->capacity * entry_size);

    for (ssize i = 0; i < h->capacity; i++) {
        void *entry = (uint8 *)table + (i * entry_size);

        fan_str8 *key = (fan_str8 *)((uint8 *)entry + h->key_offset);

        if (key->length == 0) continue;

        fan_free(mem, key->data, key->length);
        }

    fan_free(mem, h, total);
}

ssize fan_ht_cap(void *table) {
    assume(table isnt nullptr);
    fan_ht_header *h = fan_ht_header_get(table);
    return h->capacity;
}

ssize fan_ht_len(void *table) {
    assume(table isnt nullptr);
    fan_ht_header *h = fan_ht_header_get(table);
    return h->size;
}

void *fan_ht_get(fan_str8 key, void *table) {
    assume(table isnt nullptr);
    fan_ht_header *h = fan_ht_header_get(table);

    if (h->size == 0) {
        return h->default_value;
    }

    usize hash = fan_hash_str8(key);
    usize index = (usize)(hash & (usize)(h->capacity - 1));

    for (ssize i = 0; i < h->capacity; i++) {
        void *entry = fan_ht_at(h, index);

        fan_str8 *entry_key = fan_ht_key(h, entry);
        if (entry_key->length == 0) {
            break;
        }
        else if (fan_str8_equal(key, *entry_key)) {
            return fan_ht_value(h, entry);
        }
        index = (index + 1) & (h->capacity - 1);
    }
    return h->default_value;
}

static void *fan_ht_resize(fan_ht_header *h, fan_allocator *mem) {
    ssize old_capacity = h->capacity;
    ssize entry_size = fan_ht_entry_size(h);
    ssize old_total = sizeof(fan_ht_header) + (old_capacity * entry_size);
    void *old_table = (uint8 *)h + sizeof(fan_ht_header);

    ssize new_capacity = h->capacity * 2;
    if (new_capacity < h->capacity) {
        return nullptr;
    }
    assert(is_power_of_two(new_capacity));

    ssize new_total = sizeof(fan_ht_header) + (new_capacity * entry_size);
    if (new_total <= sizeof(fan_ht_header)) {
        return nullptr;
    }

    fan_ht_header *new_header = fan_make(mem, new_total);
    fan_log_debug("created upsized header.\n");
    if (new_header is nullptr) return nullptr;
    fan_memory_set((uint8 *)new_header, 0, new_total);

    new_header->capacity      = new_capacity;
    new_header->value_size    = h->value_size;
    new_header->key_offset    = h->key_offset;
    new_header->default_value = h->default_value;

    void *new_table = (uint8 *)new_header + sizeof(fan_ht_header);

    for (ssize i = 0; i < old_capacity; i++) {
        void *old_entry = (uint8 *)old_table + (i * entry_size);

        fan_str8 *old_key = (fan_str8 *)((uint8 *)old_entry + h->key_offset);

        if (old_key->length == 0) continue;

        void *old_value = (uint8 *)old_entry + h->key_offset + sizeof(fan_str8);

        usize hash = fan_hash_str8(*old_key);
        usize index = (usize)(hash & (usize)(new_capacity - 1));

        while (true) {
            void *new_entry = (uint8 *)new_table + index * entry_size;

            fan_str8 *new_key = (fan_str8 *)((uint8 *)new_entry + new_header->key_offset);
            if (new_key->length == 0) {
                *new_key = *old_key;
                void *new_value = (uint8 *)new_entry + new_header->key_offset + sizeof(fan_str8);
                fan_memory_copy(new_value, old_value, new_header->value_size);
                new_header->size++;
                break;
            }

            index = (index + 1) & (new_capacity - 1);
        }
    }

    fan_free(mem, h, old_total);

    return new_table;
}

static inline void fan_ht_put_(fan_str8 key, void *value, fan_ht_header *h, fan_allocator *mem) {
    assert(h != nullptr);
    assert(h->capacity > 0);
    fan_log_debug("ht capacity is: %zu\n", h->capacity);
    assert(is_power_of_two(h->capacity));
    ssize entry_size = fan_ht_entry_size(h);
    assert(entry_size > 0);

    usize hash = fan_hash_str8(key);
    usize index = (usize)(hash & (usize)(h->capacity - 1));
    assert(index < (usize)h->capacity);

    while (true) {
        void *entry = fan_ht_at(h, index);

        uint8 *table_start = (uint8 *)h + sizeof(fan_ht_header);
        uint8 *table_end   = table_start + h->capacity * entry_size;
        assert((uint8 *)entry >= table_start && (uint8 *)entry <table_end);

        fan_str8 *entry_key   = fan_ht_key(h, entry);
        void     *entry_value = fan_ht_value(h, entry);

        if (entry_key->length == 0) {
            *entry_key = fan_str8_copy(key, mem);
            fan_log_debug("copying string.\n");
            fan_memory_copy(entry_value, value, h->value_size);
            h->size++;
            return;
        }
        else if (fan_str8_equal(key, *entry_key)) {
            fan_memory_copy(entry_value, value, h->value_size);

            return;
        }

        index = (index + 1) & (h->capacity - 1);
    }
}

void *fan_ht_put(fan_str8 key, void *value, void *table, fan_allocator *mem) {
    assert(value != nullptr);

    fan_ht_header *h = fan_ht_header_get(table);

    if ((h->size + 1) * 100 >= h->capacity * 70) {
        fan_log_debug("resizing hashtable.\n");
        table = fan_ht_resize(h, mem);
        fan_log_debug("finished resizing hashtable.\n");

        assert(table isnt nullptr);

        h = fan_ht_header_get(table);
    }

    fan_ht_put_(key, value, h, mem);

    return table;
}

bool32 fan_ht_delete(fan_str8 key, void *table, fan_allocator *mem) {
    assume(table != nullptr);

    fan_ht_header *h = fan_ht_header_get(table);

    if (h->size == 0) {
        return true;
    }

    usize hash = fan_hash_str8(key);
    usize index = (usize)(hash & (usize)(h->capacity - 1));

    for (ssize i = 0; i < h->capacity; i++) {
        void *entry = fan_ht_at(h, index);

        fan_str8 *entry_key = fan_ht_key(h, entry);
        void *entry_value = fan_ht_value(h, entry);
        if (entry_key->length == 0) {
            break;
        }
        else if (fan_str8_equal(key, *entry_key)) {
            fan_free(mem, entry_key->data, entry_key->length);
            entry_key->data = nullptr;
            entry_key->length = 0;

            fan_memory_set(entry_value, 0, h->value_size);

            h->size--;

            return true;
        }
        index = (index + 1) & (h->capacity - 1);
    }

    return false;
}

#define fan_freelist_map(fl_ptr) &(fan_allocator){ \
    .make   = fan_freelist_make,                   \
    .free   = fan_freelist_free,                   \
    .resize = nullptr,                             \
    .ctx    = fl_ptr                               \
};


fan_cvar *fan_cvar_register_(fan_cvar params, fan_cvar_system *sys) {
    assert(sys != nullptr);
    assert(params.name.length > 0);

    fan_allocator *mem = fan_freelist_map(&sys->freelist);
    fan_cvar *result   = fan_make(mem, sizeof(fan_cvar));
    fan_log_debug("created cvar.\n");

    result->name          = fan_str8_copy(params.name, mem);
    fan_log_debug("copying string.\n");
    result->description   = fan_str8_copy(params.description, mem);
    fan_log_debug("copying string.\n");
    result->default_value = params.default_value;
    result->value         = result->default_value;
    result->flags         = params.flags;
    if (params.min_value + params.max_value != 0.0f) {
        result->min_value = params.min_value;
        result->max_value = params.max_value;
    }
    else if (params.string_values.capacity > 0) {
        for (ssize i = 0; i < params.string_values.size; i++) {
            fan_str8 s = params.string_values.data[i];
            fan_array_append(mem, &result->string_values, fan_str8_copy(s, mem));
            fan_log_debug("copying string.\n");
        }
    }

    fan_ht_put(result->name, result, sys->table, mem);
    if (sys->head == nullptr) {
        sys->head = result;
    }

    return result;
}

fan_cvar_system fan_cvar_system_create(fan_freelist *fl) {
    fan_allocator *mem = fan_freelist_map(fl);

    fan_cvar_system sys = {
        .table    =  fan_ht_create(sizeof(fan_ht_entry_cvar), 8, nullptr, mem),
        .head     =  nullptr,
        .freelist = *fl
    };

    return sys;
}

void fan_cvar_system_free(fan_cvar_system *sys) {
    assert(sys != nullptr);

    fan_allocator *mem = fan_freelist_map(&sys->freelist);

    fan_cvar *current = sys->head;
    while (current != nullptr) {
        fan_cvar *next = current->next;

        fan_array_clear(mem, current->string_values);
        fan_free(mem, current, sizeof(fan_cvar));

        current = next;
    }
    sys->head = nullptr;
}

void fan_cvar_set(fan_str8 name, fan_str8 value) {

}

fan_cvar *fan_cvar_get(fan_str8 name, fan_cvar_system *sys) {
    return fan_ht_get(name, sys->table);
}
