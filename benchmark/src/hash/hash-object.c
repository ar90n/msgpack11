/*
 * Copyright (c) 2016 Nicholas Fraser
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "benchmark.h"

static object_t* root_object;
char* insitu_data;
size_t insitu_size;

static void hash_object(object_t* object, uint32_t* hash) {
    switch (object->type) {
        case type_nil:    *hash = hash_nil(*hash); return;
        case type_bool:   *hash = hash_bool(*hash, object->b); return;
        case type_double: *hash = hash_double(*hash, object->d); return;
        case type_int:    *hash = hash_i64(*hash, object->i); return;
        case type_uint:   *hash = hash_u64(*hash, object->u); return;

        case type_str:
            *hash = hash_str(*hash, object->str, object->l);
            return;

        // unused types in this benchmark
        #if 0
        case type_float:
            write_float(hash, node_float(node));
            return;
        case type_bin:
            *hash = hash_str(*hash, node_data(node), node_data_len(node));
            return;
        case type_ext:
            *hash = hash_u8(*hash, node_exttype(node));
            *hash = hash_str(*hash, node_data(node), node_data_len(node));
            return;
        #endif

        case type_array: {
            uint32_t count = object->l;
            for (uint32_t i = 0; i < count; ++i)
                hash_object(object->children + i, hash);
            *hash = hash_u32(*hash, count);
            return;
        }

        case type_map: {
            uint32_t count = object->l;
            for (uint32_t i = 0; i < count; ++i) {

                // we expect keys to be short strings
                object_t* key = object->children + (i * 2);
                *hash = hash_str(*hash, key->str, key->l);

                hash_object(object->children + (i * 2) + 1, hash);
            }
            *hash = hash_u32(*hash, count);
            return;
        }

        default:
            break;
    }

    abort();
}

bool run_test(uint32_t* hash_out) {
    char* data = benchmark_in_situ_copy(insitu_data, insitu_size);
    if (!data)
        return false;

    hash_object(root_object, hash_out);

    benchmark_in_situ_free(data);
    return true;
}

bool setup_test(size_t object_size) {
    root_object = benchmark_object_create(object_size);

    // As with hash-data, this is just a rough approximation of encoded
    // binary data. We need it here to create unused in-situ copies to
    // match all parsing tests.
    insitu_size = 100;
    for (size_t i = 0; i < object_size; ++i)
        insitu_size <<= 3;

    srand(123);
    insitu_data = (char*)malloc(insitu_size);
    for (size_t i = 0; i < insitu_size; ++i)
        insitu_data[i] = (char)rand();

    return true;
}

void teardown_test(void) {
    object_destroy(root_object);
}

bool is_benchmark(void) {
    return true;
}

const char* test_version(void) {
    return BENCHMARK_VERSION_STR;
}

const char* test_language(void) {
    return BENCHMARK_LANGUAGE_C;
}

const char* test_format(void) {
    return "C structs";
}

const char* test_filename(void) {
    return __FILE__;
}
