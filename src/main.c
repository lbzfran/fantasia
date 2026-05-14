#include "runtime.c"

int TestMain(void) {
    fan_allocator heap_allocator = {
        .make   = fan_heap_make,
        .free   = fan_heap_free,
        .resize = fan_heap_resize,
        .ctx    = null
    };
    fan_ht ht = { 0 };

    ssize ht_capacity = 8;
    fan_ht_init(&ht, ht_capacity, &heap_allocator);
    fan_ht_setdefault(fan_str8_cstr("0"), &ht, &heap_allocator);

    fan_ht_put(fan_str8_cstr("key"),            fan_str8_cstr("val1"), &ht, &heap_allocator);
    fan_ht_put(fan_str8_cstr("wejwhdjas9i921"), fan_str8_cstr("val2"), &ht, &heap_allocator);
    fan_ht_put(fan_str8_cstr("wejas9i921"),     fan_str8_cstr("val3"), &ht, &heap_allocator);
    fan_ht_put(fan_str8_cstr("wejwhdas9i921"),  fan_str8_cstr("val4"), &ht, &heap_allocator);
    fan_ht_put(fan_str8_cstr("wejwhd9i921"),    fan_str8_cstr("val5"), &ht, &heap_allocator);
    fan_ht_put(fan_str8_cstr("wejwhdja9i1"),    fan_str8_cstr("val6"), &ht, &heap_allocator);
    fan_ht_put(fan_str8_cstr("wdja9i921"),      fan_str8_cstr("val7"), &ht, &heap_allocator);
    fan_ht_put(fan_str8_cstr("wejwhdjas9921"),  fan_str8_cstr("val8"), &ht, &heap_allocator);
    fan_ht_put(fan_str8_cstr("wjwdjas91"),      fan_str8_cstr("val9"), &ht, &heap_allocator);
    fan_str8 found     = fan_ht_get(fan_str8_cstr("key"), &ht);
    fan_str8 not_found = fan_ht_get(fan_str8_cstr("test"), &ht);

    assert(fan_str8_equals(found, fan_str8_cstr("val1")));
    assert(fan_str8_equals(not_found, fan_str8_cstr("0")));
    assert(ht.capacity == ht_capacity * 2);
    assert(false);

    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        fan_str8 arg = fan_str8_trimr(fan_str8_cstr(argv[1]));
        if (fan_str8_equals(arg, fan_str8_cstr("test"))) {
            return TestMain();
        }
        else {
            return GameMain();
        }
    }
    return GameMain();
}
