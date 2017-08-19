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
#include <string>
#include <stdexcept>

static object_t* root_object;

static msgpack11::MsgPack pack_object(object_t* object) {
    switch (object->type) {
        case type_bool:
            return msgpack11::MsgPack(object->b);
        case type_nil:
            return msgpack11::MsgPack();
        case type_int:
            return msgpack11::MsgPack(object->i);
        case type_uint:
            return msgpack11::MsgPack(object->u);
        case type_double:
            return msgpack11::MsgPack(object->d);
        case type_str:
            return msgpack11::MsgPack(object->str);
        case type_array: {
            msgpack11::MsgPack::array array_items(object->l);
            std::transform(object->children,
                           object->children + object->l,
                           array_items.begin(),
                           [](object_t& p){ return pack_object(&p); });
            return msgpack11::MsgPack( std::move( array_items ) );
        }
        case type_map: {
            msgpack11::MsgPack::object object_items;
            for (size_t i = 0; i < object->l; ++i) {
                object_t* key = object->children + i * 2;
                object_t* value = object->children + i * 2 + 1;
                assert(key->type == type_str);

                object_items[key->str] = pack_object( value );
            }
            return object_items;
        }
        default:
            break;
    }

    throw std::runtime_error("");
}

bool run_test(uint32_t* hash_out) {
    try {
        msgpack11::MsgPack pack = pack_object(root_object);
        std::string buffer = pack.dump();
        *hash_out = hash_str(*hash_out, buffer.c_str(), buffer.size());
    } catch (std::exception e) {
        return false;
    }
    return true;
}

bool setup_test(size_t object_size) {
    root_object = benchmark_object_create(object_size);
    return true;
}

void teardown_test(void) {
    object_destroy(root_object);
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
