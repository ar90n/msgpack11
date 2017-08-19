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
#include "msgpack11.hpp"

#include <algorithm>
#include <iostream>

static char* file_data;
static size_t file_size;

static uint32_t hash_object(const msgpack11::MsgPack& pack, uint32_t hash) {
    switch (pack.type()) {
        case msgpack11::MsgPack::NUL:
            return hash_nil(hash);
        case msgpack11::MsgPack::BOOL:
            return hash_bool(hash, pack.bool_value());
        case msgpack11::MsgPack::FLOAT32:
            return hash_double(hash, pack.float32_value());
        case msgpack11::MsgPack::FLOAT64:
            return hash_double(hash, pack.float64_value());
        case msgpack11::MsgPack::INT8:
            return hash_i64(hash, pack.int8_value());
        case msgpack11::MsgPack::INT16:
            return hash_i64(hash, pack.int16_value());
        case msgpack11::MsgPack::INT32:
            return hash_i64(hash, pack.int32_value());
        case msgpack11::MsgPack::INT64:
            return hash_i64(hash, pack.int64_value());
        case msgpack11::MsgPack::UINT8:
            return hash_u64(hash, pack.uint8_value());
        case msgpack11::MsgPack::UINT16:
            return hash_u64(hash, pack.uint16_value());
        case msgpack11::MsgPack::UINT32:
            return hash_u64(hash, pack.uint32_value());
        case msgpack11::MsgPack::UINT64:
            return hash_u64(hash, pack.uint64_value());
        case msgpack11::MsgPack::STRING: {
            std::string const& str = pack.string_value();
            return hash_str(hash, str.c_str(), str.size());
        }
        case msgpack11::MsgPack::ARRAY: {
            msgpack11::MsgPack::array const& items = pack.array_items();
            std::for_each(items.begin(), items.end(), [&hash](msgpack11::MsgPack const& item) {
                hash = hash_object( item, hash );
            });
            return hash_u32(hash, items.size());
        }
        case msgpack11::MsgPack::OBJECT: {
            msgpack11::MsgPack::object const& items = pack.object_items();
            std::for_each(items.begin(), items.end(), [&hash]( std::pair< msgpack11::MsgPack, msgpack11::MsgPack > const& item) {
                msgpack11::MsgPack const& key = item.first;
                msgpack11::MsgPack const& value = item.second;
                assert(key.type() == msgpack11::MsgPack::STRING);

                std::string const& key_str = key.string_value();
                hash = hash_str(hash, key_str.c_str(), key_str.size());
                hash = hash_object(value, hash);
            });
            return hash_u32(hash, items.size());
        }
        default:
            break;
    }

    throw std::runtime_error("");
}

bool run_test(uint32_t* hash_out) {
    char* data = benchmark_in_situ_copy(file_data, file_size);
    if (!data)
        return false;

    try {
        std::string err;
        msgpack11::MsgPack pack = msgpack11::MsgPack::parse(data, file_size, err);
        *hash_out = hash_object(pack, *hash_out);
    } catch (...) {
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
    return "0.0.9";
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
