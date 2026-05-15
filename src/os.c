
#include "os.h"
#if   defined(OS_WINDOWS)
# include "os_windows.c"
#elif defined(OS_LINUX)
# include "os_linux.c"
#endif
#include <string.h>

void *fan_heap_make(void *ctx, ssize size) {
    (void)ctx;
    void *result = malloc(size);
    assert(result && "ERROR: Reached Out-Of-Memory state.");

    return result;
}

void fan_heap_free(void *ctx, void *ptr, ssize size) {
    (void)ctx;
    (void)size;

    free(ptr);
    ptr = nullptr;
}

void *fan_heap_resize(void *ctx, void *ptr, ssize old, ssize new) {
    (void)ctx;
    void *result = fan_heap_make(ctx, new);

    if (ptr isnt null) {
        if (new > old) {
            memcpy(result, ptr, old);
        }
        else {
            memmove(result, ptr, old);
        }
        fan_heap_free(ctx, ptr, old);
    }

    return result;
}

void *fan_arena_make(void *ctx, ssize size) {
    fan_arena *a = (fan_arena *)ctx;

    uintptr base = (uintptr)(a->data + a->size);
    uintptr alignment = fan_align_forward(base, FAN_DEFAULT_ALIGNMENT);
    ssize offset = alignment - (uintptr)a->data;

    assert(size + offset <= a->capacity && "ERROR: Reached Out-Of-Memory state.");

    void *result = a->data + offset;
    a->size = offset + size;

    fan_memory_set(result, 0, size);

    return result;
}

void fan_arena_free(void *ctx, void *ptr, ssize size) {
    fan_arena *a = (fan_arena *)ctx;

    uintptr ptr_val = (uintptr)ptr;
    uintptr base_val = (uintptr)a->data;

    ssize offset = ptr_val - base_val;

    ssize expected_size = offset + size;
    if (a->size == expected_size) {
        a->size -= size;
    }
}

void fan_arena_clear(fan_arena *a) {
    a->size = 0;
}

fan_arena_temp fan_arena_temp_begin(fan_arena *a) {
    fan_arena_temp result;

    result.arena = a;
    result.start_size = a->size;

    return result;
}

void fan_arena_temp_end(fan_arena_temp temp) {
    temp.arena->size = temp.start_size;
}


void *fan_arena_resize(void *ctx, void *ptr, ssize old, ssize new) {
    fan_arena *a = (fan_arena *)ctx;

    if (new == old) {
        return ptr;
    }

    void *result = null;
    if (ptr is null) {
        result = fan_arena_make(ctx, new);
    }
    else {
        uintptr ptr_val  = (uintptr)ptr;
        uintptr base_val = (uintptr)a->data;

        ssize offset = ptr_val - base_val;

        if (new > old) {
            result = fan_arena_make(ctx, new);
            memcpy(result, ptr, old);
        }
        else if (a->size == offset + old) {
            a->size -= (offset + old - new);
            result = ptr;
        }
    }

    return result;
}

// NOTE(liam): freelist
void fan_freelist_clear(fan_freelist *fl) {
    fl->used = 0;
    fan_freelist_node *first_node = (fan_freelist_node *)fl->data;
    first_node->block_size = fl->size;
    first_node->next = NULL;
    fl->head = first_node;
}

void fan_freelist_init(fan_freelist *fl, void *data, ssize size) {
    fl->data = data;
    fl->size = size;
    fan_freelist_clear(fl);
}

ssize fan_calc_padding(uintptr ptr,
                   uintptr alignment,
                   ssize header_size) {
    assert(is_power_of_two(alignment));

    uintptr mask = alignment - 1;
    uintptr padding = (-ptr) & mask;

    if (padding < header_size) {
        padding += (header_size - padding + mask) & ~mask;
    }

    return (ssize)padding;
}

fan_freelist_node *fan_freelist_findbest(fan_freelist *fl, ssize size, ssize alignment, ssize *padding_, fan_freelist_node **prev_node_) {
    ssize smallest_diff = ~(ssize)0;

    fan_freelist_node *node = fl->head;
    fan_freelist_node *prev_node = nullptr;
    fan_freelist_node *best_node = nullptr;

    ssize padding = 0;

    while (node != NULL) {
        padding = fan_calc_padding((uintptr)node, (uintptr)alignment, sizeof(fan_freelist_header));
        ssize required = size + padding;
        if (node->block_size >= required) {
            ssize diff = node->block_size - required;

            if (diff < smallest_diff) {
                smallest_diff = diff;
                best_node = node;

                if (padding_) *padding_ = padding;
                if (prev_node_) *prev_node_ = prev_node;
            }
        }
        prev_node = node;
        node = node->next;
    }
    return best_node;
}

fan_freelist_node *fan_freelist_findfirst(fan_freelist *fl, ssize size, ssize alignment, ssize *padding_, fan_freelist_node **prev_node_) {
    fan_freelist_node *node = fl->head;
    fan_freelist_node *prev_node = nullptr;

    ssize padding = 0;

    while (node != nullptr) {
        padding = fan_calc_padding(
            (uintptr)node,
            (uintptr)alignment,
            sizeof(fan_freelist_header)
        );
        ssize required = size + padding;

        if (node->block_size >= required) {
            break;
        }

        prev_node = node;
        node = node->next;
    }

    if (padding_)   *padding_   = padding;
    if (prev_node_) *prev_node_ = prev_node;
    return node;
}

void fan_freelist_node_insert(fan_freelist_node **phead, fan_freelist_node *prev_node, fan_freelist_node *new_node) {
    if (prev_node is nullptr) {
        new_node->next = *phead;
        *phead = new_node;
    }
    else {
        new_node->next = prev_node->next;
        prev_node->next = new_node;
    }
}

void fan_freelist_node_remove(fan_freelist_node **phead, fan_freelist_node *prev_node, fan_freelist_node *del_node) {
    if (prev_node is nullptr) {
        *phead = del_node->next;
    } else {
        prev_node->next = del_node->next;
    }
}

void *fan_freelist_make(void *ctx, ssize size) {
    fan_freelist *fl = (fan_freelist *)ctx;
    ssize padding = 0;
    fan_freelist_node *prev_node = nullptr;
    fan_freelist_node *node = nullptr;
    ssize alignment = FAN_DEFAULT_ALIGNMENT;
    assume(alignment >= 8 and is_power_of_two(alignment));
    ssize alignment_padding, required, remaining;
    fan_freelist_header *header_ptr;

    if (size < sizeof(fan_freelist_node)) {
        size = sizeof(fan_freelist_node);
    }

    if (fl->policy is FanFListPolicy_FindBest) {
        node = fan_freelist_findbest(fl, size, alignment, &padding, &prev_node);
    }
    else {
        node = fan_freelist_findfirst(fl, size, alignment, &padding, &prev_node);
    }
    fan_log(
            FanLog_DEBUG,
           "head=%p block=%zu size=%zu padding=%zu required=%zu\n",
           fl->head,
           (ssize)(node ? node->block_size : -1),
           (ssize)size,
           (ssize)padding,
           (ssize)(size + padding)
    );
    if (node is nullptr) {
        assert(false && "Freelist has run out of memory.");
        return nullptr;
    }

    alignment_padding = padding - sizeof(fan_freelist_header);
    required = size + padding;
    remaining = node->block_size - required;

    if (remaining >= sizeof(fan_freelist_node)) {
        fan_freelist_node *new_node = (fan_freelist_node *)((uint8 *)node + required);
        new_node->block_size = remaining;
        fan_freelist_node_insert(&fl->head, node, new_node);
    }

    fan_freelist_node_remove(&fl->head, prev_node, node);

    fl->used += required;

    header_ptr = (fan_freelist_header *)((uint8 *)node + alignment_padding);
    header_ptr->block_size = required;
    header_ptr->padding = alignment_padding;

    return (void *)((uint8 *)header_ptr + sizeof(fan_freelist_header));
}

void fan_freelist_coalesce(fan_freelist *fl, fan_freelist_node *prev_node, fan_freelist_node *free_node);

void fan_freelist_free(void *ctx, void *ptr, ssize size) {
    (void)size;
    fan_freelist *fl = (fan_freelist *)ctx;
    fan_freelist_header *header;
    fan_freelist_node *free_node, *node;
    fan_freelist_node *prev_node = nullptr;

    if (ptr is null) {
        return;
    }

    header = (fan_freelist_header *)((uint8 *)ptr - sizeof(fan_freelist_header));
    free_node = (fan_freelist_node *)((uint8 *)header - header->padding);
    free_node->block_size = header->block_size;
    free_node->next = nullptr;

    node = fl->head;
    while (node isnt nullptr and (uintptr)free_node > (uintptr)node) {
        prev_node = node;
        node = node->next;
    }
    fan_freelist_node_insert(&fl->head, prev_node, free_node);
    fl->used -= free_node->block_size;
    fan_freelist_coalesce(fl, prev_node, free_node);
}

void fan_freelist_coalesce(fan_freelist *fl, fan_freelist_node *prev_node, fan_freelist_node *free_node) {
    if (free_node->next isnt nullptr and
        (void *)((uint8 *)free_node + free_node->block_size) is free_node->next) {
        free_node->block_size += free_node->next->block_size;

        fan_freelist_node *next = free_node->next;

        free_node->block_size += next->block_size;
        fan_freelist_node_remove(&fl->head, free_node, next);
    }

    if (prev_node isnt nullptr and prev_node->next isnt nullptr and
        (void *)((uint8 *)prev_node + prev_node->block_size) is free_node) {
        prev_node->block_size += free_node->block_size;
        fan_freelist_node_remove(&fl->head, prev_node, free_node);
    }
}


void fan_fbuf8_flush(fan_fbuf8 *b) {
    b->error |= b->pipe < 0;
    if (!b->error && b->length) {
        b->error |= !fan_os_write(b->pipe, b->buf, b->length);
        b->length = 0;
    }
}

void fan_fbuf8_append(fan_fbuf8 *b, uchar8 *src, ssize length) {
    uchar8 *end = src + length;
    while (!b->error && src<end) {
        ssize left = end - src;
        ssize available = b->capacity - b->length;
        ssize amount = available < left ? available : left;

        for (ssize i = 0; i < amount; i++) {
            b->buf[b->length+i] = src[i];
        }
        b->length += amount;
        src += amount;

        if (amount < left) {
            fan_fbuf8_flush(b);
        }
    }
}

void fan_fbuf8_append_char(fan_fbuf8 *b, uchar8 c) {
    fan_fbuf8_append(b, &c, 1);
}

void fan_fbuf8_append_str8(fan_fbuf8 *b, fan_str8 s) {
    fan_fbuf8_append(b, s.data, s.length);
}

void fan_fbuf8_append_cstr(fan_fbuf8 *b, const char8 *s) {
    fan_fbuf8_append(b, (uchar8 *)s, sizeof(s) - 1);
}

void fan_fbuf8_append_long(fan_fbuf8 *b, long x) {
    uchar8  tmp[64];
    uchar8 *end = tmp + sizeof(tmp);
    uchar8 *beg = end;
    long t = x > 0 ? -x : x;
    do {
        *--beg = (uchar8)('0' - t % 10);
    }
    while (t /= 10);

    if (x < 0) {
        *--beg = (uchar8)('-');
    }
    fan_fbuf8_append(b, beg, end - beg);
}

void fan_fbuf8_append_double(fan_fbuf8 *b, double x) {
    long prec = 1000000;  // i.e. 6 decimals

    if (x < 0) {
        fan_fbuf8_append_char(b, '-');
        x = -x;
    }

    x += 0.5 / (double)prec;
    if (x >= (double)(-1UL>>1)) {
        fan_fbuf8_append_cstr(b, "inf");
    } else {
        long integral = (long)x;
        long fractional = (long)((long)(x - (double)integral) * prec);
        fan_fbuf8_append_long(b, integral);
        fan_fbuf8_append_char(b, '.');
        for (long i = prec/10; i > 1; i /= 10) {
            if (i > fractional) {
                fan_fbuf8_append_char(b, '0');
            }
        }
        fan_fbuf8_append_long(b, fractional);
    }
}

void fan_fbuf8_append_ptr(fan_fbuf8 *b, void *ptr) {
    fan_fbuf8_append_cstr(b, "0x");
    uintptr u = (uintptr)ptr;
    for (int i = 2*sizeof(u) - 1; i >= 0; i--) {
        fan_fbuf8_append_char(b, "0123456789abcdef"[(u>>(4 * i)) & 15]);
    }
}

// void fan_str8_print(fan_fbuf8 *b, fan_str8 s) {
//     fan_fbuf8_append_str8(b, s);
//     fan_fbuf8_flush(b);
// }
//
// void fan_str8_printn(fan_fbuf8 *b, fan_str8 s, uchar8 end) {
//     fan_fbuf8_append_str8(b, s);
//     fan_fbuf8_append_char(b, end);
//     fan_fbuf8_flush(b);
// }
//
// void fan_str8_println(fan_fbuf8 *b, fan_str8 s) {
//     fan_str8_printn(b, s, '\n');
// }

fan_str8 fan_str8_span(uchar8 *beg, uchar8 *end) {
    fan_str8 r = {0};
    r.data = beg;
    r.length  = beg ? end-beg : 0;
    return r;
}

fan_str8 fan_str8_cstrv(const char8 *s) {
    return (fan_str8){ (uchar8 *)s, (ssize)strlen(s) };
}

int fan_str8_equals(fan_str8 a, fan_str8 b) {
    return a.length==b.length && (!a.length || !memcmp(a.data, b.data, a.length));
}

fan_str8 fan_str8_triml(fan_str8 s) {
    for (; s.length && *s.data <= ' '; s.data++, s.length--) {}
    return s;
}

fan_str8 fan_str8_trimr(fan_str8 s) {
    for (; s.length && s.data[s.length-1] <= ' '; s.length--) {}
    return s;
}

fan_str8 fan_str8_substr(fan_str8 s, ssize i) {
    if (i) {
        s.data   += i;
        s.length -= i;
    }
    return s;
}

fan_cutstr8 fan_str8_cut(fan_str8 s, uchar8 c) {
    fan_cutstr8 r = { 0 };
    if (!s.length) return r;  // null pointer special case
    uchar8 *beg = s.data;
    uchar8 *end = s.data + s.length;
    uchar8 *cut = beg;
    for (; cut<end && *cut != c; cut++) {}
    r.ok   = cut < end;
    r.head = fan_str8_span(beg, cut);
    r.tail = fan_str8_span(cut+r.ok, end);
    return r;
}

ssize fan_cstr_length(char8 const *s) {
    return (ssize)strlen(s);
}

ssize fan_cstr_copy_str8(char8 *dst, fan_str8 src) {
    for (ssize i = 0; i < src.length; i++) {
        dst[i] = src.data[i];
    }
    return src.length;
}

fan_str8 fan_str8_copy(fan_str8 src, fan_allocator *mem) {
    fan_str8 result = {
        .data = mem->make(mem->ctx, src.length),
        .length = src.length
    };
    assert(result.data != nullptr);
    for (ssize i = 0; i < src.length; i++) {
        result.data[i] = src.data[i];
    }
    return result;
}

void fan_memory_set(uint8 *ptr, ssize value, ssize length) {
    memset(ptr, value, length);
}

void fan_str8_print(fan_str8 buf) {
    for (ssize i = 0; i < buf.length; i++) {
        fan_log(FanLog_INFO, "%c", buf.data[i]);
    }
}
