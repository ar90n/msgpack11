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
#include "msgpack.hpp"

static char* file_data;
static size_t file_size;

static void hash_object(const msgpack::object& object, uint32_t* hash) {
    switch (object.type) {
        case msgpack::type::NIL:              *hash = hash_nil(*hash); return;
        case msgpack::type::BOOLEAN:          *hash = hash_bool(*hash, object.via.boolean); return;
        case msgpack::type::FLOAT:            *hash = hash_double(*hash, object.via.f64); return;
        case msgpack::type::NEGATIVE_INTEGER: *hash = hash_i64(*hash, object.via.i64); return;
        case msgpack::type::POSITIVE_INTEGER: *hash = hash_u64(*hash, object.via.u64); return;
        case msgpack::type::STR:              *hash = hash_str(*hash, object.via.str.ptr, object.via.str.size); return;

        case msgpack::type::ARRAY:
            for (size_t i = 0; i < object.via.array.size; ++i)
                hash_object(*(object.via.array.ptr + i), hash);
            *hash = hash_u32(*hash, object.via.array.size);
            return;

        case msgpack::type::MAP:
            for (size_t i = 0; i < object.via.map.size; ++i) {

                // we expect keys to be short strings
                const msgpack::object& key = object.via.map.ptr[i].key;
                assert(key.type == msgpack::type::STR);
                *hash = hash_str(*hash, key.via.str.ptr, key.via.str.size);

                hash_object(object.via.map.ptr[i].val, hash);
            }
            *hash = hash_u32(*hash, object.via.map.size);
            return;

        default:
            break;
    }

    throw msgpack::unpack_error("");
}

bool run_test(uint32_t* hash_out) {
    char* data = benchmark_in_situ_copy(file_data, file_size);
    if (!data)
        return false;

    try {
        msgpack::object_handle oh = msgpack::unpack(data, file_size);
        hash_object(oh.get(), hash_out);
    } catch (msgpack::unpack_error error) {
        benchmark_in_situ_free(data);
        return false;
    }

    benchmark_in_situ_free(data);
    return true;
}

bool setup_test(size_t object_size) {
    file_data = load_data_file(BENCHMARK_FORMAT_MESSAGEPACK, object_size, &file_size);
    if (!file_data)
        return false;
    return true;
}

void teardown_test(void) {
    free(file_data);
}

bool is_benchmark(void) {
    return true;
}

const char* test_version(void) {
    return MSGPACK_VERSION;
}

const char* test_language(void) {
    return BENCHMARK_LANGUAGE_CXX;
}

const char* test_format(void) {
    return "MessagePack";
}

const char* test_filename(void) {
    return __FILE__;
}
