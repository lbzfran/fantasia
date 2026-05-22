#include "runtime.c"

int TestMain(void) {
    fan_freelist fl = { 0 };
    fan_freelist_init(&fl, fan_heap_make(nullptr, megabytes(1)), megabytes(1));
    fan_allocator *mem = &(fan_allocator){
        .make   = fan_freelist_make,
        .free   = fan_freelist_free,
        .resize = nullptr,
        .ctx    = (void *)&fl
    };
    fan_ht_entry_str8 *ht = fan_ht_create(sizeof(fan_ht_entry_str8), 8, &(fan_str8_cstr("0")), mem);

    ht = fan_ht_put(fan_str8_cstr("key"),            &fan_str8_cstr("value"), ht, mem);
    ht = fan_ht_put(fan_str8_cstr("wejwhdjas9i921"), &fan_str8_cstr("val2"),  ht, mem);
    ht = fan_ht_put(fan_str8_cstr("wejas9i921"),     &fan_str8_cstr("val3"),  ht, mem);
    ht = fan_ht_put(fan_str8_cstr("wejwhdas9i921"),  &fan_str8_cstr("val4"),  ht, mem);
    ht = fan_ht_put(fan_str8_cstr("wejwhd9i921"),    &fan_str8_cstr("val5"),  ht, mem);
    ht = fan_ht_put(fan_str8_cstr("wejwhdja9i1"),    &fan_str8_cstr("val6"),  ht, mem);
    ht = fan_ht_put(fan_str8_cstr("wdja9i921"),      &fan_str8_cstr("val7"),  ht, mem);
    ht = fan_ht_put(fan_str8_cstr("wejwhdjas9921"),  &fan_str8_cstr("val8"),  ht, mem);
    ht = fan_ht_put(fan_str8_cstr("wjwdjas91"),      &fan_str8_cstr("val9"),  ht, mem);

    fan_str8 found     = *((fan_str8 *)fan_ht_get(fan_str8_cstr("key"),  ht));
    fan_str8 not_found = *((fan_str8 *)fan_ht_get(fan_str8_cstr("keys"), ht));

    assert(fan_str8_equal(found, fan_str8_cstr("value")));
    assert(fan_str8_equal(not_found, fan_str8_cstr("0")));

    (void)fan_ht_delete(fan_str8_cstr("key"), ht, mem);
    found = *((fan_str8 *)fan_ht_get(fan_str8_cstr("key"),  ht));

    assert(fan_str8_equal(found, fan_str8_cstr("0")));

    fan_ht_free(ht, mem);
    fan_freelist_clear(&fl);
    fan_log_debug("cleared hashtable.\n");

    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        // fan_str8 arg = fan_str8_trimr(fan_str8_cstr(argv[1]));
        // if (fan_str8_equal(arg, fan_str8_cstr("test"))) {
            return TestMain();
        // }
        // else {
        //     return GameMain();
        // }
    }
    return GameMain();
}
