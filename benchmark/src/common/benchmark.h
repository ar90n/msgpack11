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

// This is the benchmarking framework for C/C++ serialization libraries.

#ifndef BENCHMARK_H
#define BENCHMARK_H 1

#include "platform.h"
#include "generator.h"
#include "hash.h"

#define BENCHMARK_FORMAT_MESSAGEPACK "mp"

#define BENCHMARK_LANGUAGE_C "C"
#define BENCHMARK_LANGUAGE_CXX "C++"

#define BENCHMARK_NODE_MAX         (32*4096) /* upper bound */
#define BENCHMARK_VERSION 0.1

#define BENCHMARK_STRINGIFY2(x) #x
#define BENCHMARK_STRINGIFY(x) BENCHMARK_STRINGIFY2(x)
#define BENCHMARK_VERSION_STR BENCHMARK_STRINGIFY(BENCHMARK_VERSION)

// when this is enabled, all read/dom tests make a full copy of
// the data on each iteration whether or not they modify it. this
// allows correctly comparing against in-situ parsers.
// the actual in-situ parsing is enabled by BENCHMARK_IN_SITU in the Makefile.
#define BENCHMARK_MAKE_IN_SITU_COPIES 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * These functions should be implemented by the test.
 * run_test() is run repeatedly until a time limit has been reached.
 *
 * A hash of the resulting data must be output to ensure all data was
 * read correctly and to prevent the compiler from optimizing away
 * parts of the test.
 */
bool is_benchmark();
bool run_test(uint32_t* hash_out);
bool setup_test(size_t object_size);
void teardown_test(void);

const char* test_language(void);
const char* test_version(void);
const char* test_format(void);
const char* test_filename(void);

// Loads a data file. Should be freed with free().
char* load_data_file(const char* format, size_t object_size, size_t* size_out);

// Loads a special data file. Should be freed with free().
char* load_data_file_ex(const char* format, size_t object_size, size_t* size_out, const char* config);

// Generates a random object for benchmarking.
object_t* benchmark_object_create(size_t object_size);

// Generates the filename for a data file
void benchmark_filename(char* buf, size_t size, size_t object_size, const char* format, const char* config);

// Copies a data buffer if in-situ parsing is enabled. All
// parsing tests must call this on every iteration.
static inline char* benchmark_in_situ_copy(char* source, size_t size) {
    #if BENCHMARK_MAKE_IN_SITU_COPIES
    char* data = (char*)malloc(size + 1);
    if (!data)
        return NULL;
    memcpy(data, source, size);
    // some APIs (e.g. yajl, or RapidJSON with in-situ mode) require the
    // entire document to be null-terminated
    data[size] = '\0';
    return data;
    #else
    return source;
    #endif
}

// Frees a data buffer if in-situ parsing is enabled
static inline void benchmark_in_situ_free(char* data) {
    #if BENCHMARK_MAKE_IN_SITU_COPIES
    free(data);
    #endif
}

#ifdef __cplusplus
}
#endif

#endif
