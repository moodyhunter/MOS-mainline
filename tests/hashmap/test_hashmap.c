// SPDX-License-Identifier: GPL-3.0-or-later

#include "lib/structures/hashmap.h"
#include "lib/structures/hashmap_common.h"
#include "mos/mos_global.h"
#include "test_engine.h"

#define HASHMAP_MAGIC MOS_FOURCC('H', 'M', 'a', 'p')

MOS_TEST_CASE(hashmap_init_simple_macro)
{
    hashmap_t map = { 0 };
    hashmap_common_type_init(&map, 64, string);
    MOS_TEST_CHECK(map.magic, HASHMAP_MAGIC);
    MOS_TEST_CHECK(map.capacity, 64);
    MOS_TEST_CHECK(map.size, 0);
    MOS_TEST_CHECK(map.hash_func, hashmap_hash_string);
    MOS_TEST_CHECK(map.key_compare_func, hashmap_compare_string);
    MOS_TEST_CHECK(map.entries != NULL, true);
    hashmap_deinit(&map);
}

MOS_TEST_CASE(hashmap_put_single)
{
    hashmap_t map = { 0 };
    hashmap_common_type_init(&map, 135, string);
    MOS_TEST_CHECK(map.magic, HASHMAP_MAGIC);

    MOS_TEST_CHECK(map.capacity, 135);
    MOS_TEST_CHECK(map.size, 0);
    void *old = hashmap_put(&map, "foo", "bar");
    MOS_TEST_CHECK(old, NULL);
    MOS_TEST_CHECK(map.capacity, 135);
    MOS_TEST_CHECK(map.size, 1);

    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "bar");
    hashmap_deinit(&map);
}

MOS_TEST_CASE(hashmap_get_function)
{
    hashmap_t map = { 0 };
    hashmap_common_type_init(&map, 1, string);
    MOS_TEST_CHECK(map.magic, HASHMAP_MAGIC);
    MOS_TEST_CHECK(map.capacity, 1);
    MOS_TEST_CHECK(map.size, 0);

    hashmap_put(&map, "foo", "foo1");
    MOS_TEST_CHECK(map.capacity, 1);
    MOS_TEST_CHECK(map.size, 1);
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "foo1");

    hashmap_put(&map, "bar", "bar1");
    MOS_TEST_CHECK(map.capacity, 1);
    MOS_TEST_CHECK(map.size, 2);
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "bar"), "bar1");

    hashmap_put(&map, "bar", "bar2");
    MOS_TEST_CHECK(map.capacity, 1);
    MOS_TEST_CHECK(map.size, 2);
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "bar"), "bar2");

    hashmap_deinit(&map);
}

MOS_TEST_CASE(hashmap_put_multiple)
{
    hashmap_t map = { 0 };
    void *old;
    hashmap_common_type_init(&map, 135, string);
    MOS_TEST_CHECK(map.magic, HASHMAP_MAGIC);
    MOS_TEST_CHECK(map.capacity, 135);
    MOS_TEST_CHECK(map.size, 0);

    old = hashmap_put(&map, "foo", "foo1");
    MOS_TEST_CHECK(old, NULL);
    MOS_TEST_CHECK(map.capacity, 135);
    MOS_TEST_CHECK(map.size, 1);
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "foo1");

    old = hashmap_put(&map, "foo", "foo2");
    MOS_TEST_CHECK(map.capacity, 135);
    MOS_TEST_CHECK(map.size, 1);
    MOS_TEST_CHECK_STRING(old, "foo1");
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "foo2");

    old = hashmap_put(&map, "bar", "bar1");
    MOS_TEST_CHECK(old, NULL);
    MOS_TEST_CHECK(map.capacity, 135);
    MOS_TEST_CHECK(map.size, 2);
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "bar"), "bar1");
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "foo2");

    old = hashmap_put(&map, "bar", "bar2");
    MOS_TEST_CHECK(map.capacity, 135);
    MOS_TEST_CHECK(map.size, 2);
    MOS_TEST_CHECK_STRING(old, "bar1");
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "bar"), "bar2");
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "foo2");
    hashmap_deinit(&map);
}

MOS_TEST_CASE(hashmap_put_overflow)
{
    hashmap_t map = { 0 };
    void *old;
    hashmap_common_type_init(&map, 1, string);
    MOS_TEST_CHECK(map.magic, HASHMAP_MAGIC);
    MOS_TEST_CHECK(map.capacity, 1);
    MOS_TEST_CHECK(map.size, 0);

    old = hashmap_put(&map, "foo", "foo1");
    MOS_TEST_CHECK(old, NULL);
    MOS_TEST_CHECK(map.capacity, 1);
    MOS_TEST_CHECK(map.size, 1);
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "foo1");

    old = hashmap_put(&map, "bar", "bar1");
    MOS_TEST_CHECK(old, NULL);
    MOS_TEST_CHECK(map.capacity, 1);
    MOS_TEST_CHECK(map.size, 2);
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "bar"), "bar1");
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "foo1");

    old = hashmap_put(&map, "bar", "bar2");
    MOS_TEST_CHECK_STRING(old, "bar1");
    MOS_TEST_CHECK(map.capacity, 1);
    MOS_TEST_CHECK(map.size, 2);
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "bar"), "bar2");
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "foo1");

    hashmap_deinit(&map);
}

MOS_TEST_CASE(hashmap_remove_function)
{
    void *old;
    hashmap_t map = { 0 };
    hashmap_common_type_init(&map, 10, string);
    MOS_TEST_CHECK(map.magic, HASHMAP_MAGIC);
    MOS_TEST_CHECK(map.capacity, 10);
    MOS_TEST_CHECK(map.size, 0);

    old = hashmap_put(&map, "foo", "foo1");
    MOS_TEST_CHECK(map.capacity, 10);
    MOS_TEST_CHECK(map.size, 1);
    MOS_TEST_CHECK(old, NULL);
    MOS_TEST_CHECK_STRING(hashmap_get(&map, "foo"), "foo1");

    old = hashmap_remove(&map, "foo");
    MOS_TEST_CHECK(map.capacity, 10);
    MOS_TEST_CHECK(map.size, 0);
    MOS_TEST_CHECK_STRING(old, "foo1");
    const char *nothing = hashmap_get(&map, "foo");
    MOS_TEST_CHECK(nothing, NULL);

    old = hashmap_remove(&map, "foo");
    MOS_TEST_CHECK(old, NULL);
    MOS_TEST_CHECK(map.capacity, 10);
    MOS_TEST_CHECK(map.size, 0);

    MOS_TEST_CHECK(hashmap_get(&map, "foo"), NULL);
    hashmap_deinit(&map);
}

static size_t test_hashmap_foreach_count = 0;

bool test_foreach_function(const void *key, void *value)
{
    MOS_UNUSED(key);
    MOS_UNUSED(value);
    test_hashmap_foreach_count++;
    return true;
}

bool test_foreach_stop_at_quux(const void *key, void *value)
{
    MOS_UNUSED(value);
    test_hashmap_foreach_count++;
    if (strcmp(key, "quux") == 0)
        return false;
    return true;
}

MOS_TEST_CASE(hashmap_foreach_function)
{
    hashmap_t map = { 0 };
    hashmap_common_type_init(&map, 10, string);
    MOS_TEST_CHECK(map.magic, HASHMAP_MAGIC);
    MOS_TEST_CHECK(map.capacity, 10);
    MOS_TEST_CHECK(map.size, 0);
    hashmap_put(&map, "foo", "foo1");
    hashmap_put(&map, "bar", "bar1");
    hashmap_put(&map, "baz", "baz1");
    hashmap_put(&map, "qux", "qux1");
    hashmap_put(&map, "quux", "quux1");
    hashmap_put(&map, "corge", "corge1");
    hashmap_put(&map, "grault", "grault1");
    hashmap_put(&map, "garply", "garply1");
    hashmap_put(&map, "waldo", "waldo1");
    hashmap_put(&map, "fred", "fred1");
    hashmap_put(&map, "plugh", "plugh1");
    hashmap_put(&map, "xyzzy", "xyzzy1");

    test_hashmap_foreach_count = 0;
    hashmap_foreach(&map, test_foreach_function);
    MOS_TEST_CHECK(test_hashmap_foreach_count, map.size);

    test_hashmap_foreach_count = 0;
    hashmap_foreach(&map, test_foreach_stop_at_quux);
    MOS_TEST_CHECK(test_hashmap_foreach_count, 4);
    hashmap_deinit(&map);
}
