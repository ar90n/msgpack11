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

static object_t* root_object;

static void pack_object(msgpack::packer<msgpack::sbuffer>& packer, object_t* object) {
    switch (object->type) {
        case type_bool:
            object->b ? packer.pack_true() : packer.pack_false();
            return;

        case type_nil:    packer.pack_nil();      return;
        case type_int:    packer.pack(object->i); return;
        case type_uint:   packer.pack(object->u); return;
        case type_double: packer.pack(object->d); return;

        case type_str:
            packer.pack_str(object->l);
            packer.pack_str_body(object->str, object->l);
            return;

        case type_array:
            packer.pack_array(object->l);
            for (size_t i = 0; i < object->l; ++i)
                pack_object(packer, object->children + i);
            return;

        case type_map:
            packer.pack_map(object->l);
            for (size_t i = 0; i < object->l; ++i) {

                // we expect keys to be short strings
                object_t* key = object->children + i * 2;
                assert(key->type == type_str);
                packer.pack_str(key->l);
                packer.pack_str_body(key->str, key->l);

                pack_object(packer, object->children + i * 2 + 1);
            }
            return;

        default:
            break;
    }

    throw msgpack::unpack_error("");
}

bool run_test(uint32_t* hash_out) {
    try {
        msgpack::sbuffer buffer;
        msgpack::packer<msgpack::sbuffer> packer(buffer);
        pack_object(packer, root_object);
        *hash_out = hash_str(*hash_out, buffer.data(), buffer.size());

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
