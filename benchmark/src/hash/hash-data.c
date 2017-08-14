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

char* hash_data;
size_t hash_size;

bool run_test(uint32_t* hash_out) {
    *hash_out = hash_str(*hash_out, hash_data, hash_size);
    return true;
}

bool setup_test(size_t object_size) {

    // This is a very rough approximation of the size of encoded binary data
    // in any format. It doesn't actually matter if it's very close; the hash
    // time for flat data is nearly insignificant. we're just interested in
    // including the hash code (and object generation code and all other code in
    // benchmark.c) so its compiled size can be subtracted out of the results.
    hash_size = 100;
    for (size_t i = 0; i < object_size; ++i)
        hash_size <<= 3;

    srand(123);
    hash_data = (char*)malloc(hash_size);
    for (size_t i = 0; i < hash_size; ++i)
        hash_data[i] = (char)rand();

    return true;
}

void teardown_test(void) {
    free(hash_data);
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
    return "random data";
}

const char* test_filename(void) {
    return __FILE__;
}

