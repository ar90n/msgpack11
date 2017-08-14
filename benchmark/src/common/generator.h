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

#ifndef BENCHMARK_GENERATOR_H
#define BENCHMARK_GENERATOR_H 1

// This file implements a random structured data generator.
// It's used for generating random MessagePack or JSON.

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif


// A simple multiply-with-carry PRNG, implemented here for
// cross-platform consistency.
typedef struct random_t {
    uint32_t v;
    uint32_t c;
} random_t;

void random_seed(random_t* random, uint64_t val);
uint32_t random_next(random_t* random);

// generates a random number with (approximately) inverse distribution
// up to (approximately) the given max, so we get lots of small numbers
// and a few big ones
uint32_t random_inverse(random_t* random, uint32_t max);


typedef enum type_t {
    type_nil = 1,
    type_bool,
    type_double,
    type_int,
    type_uint,
    type_str,
    type_array,
    type_map
} type_t;

typedef struct object_t {
    type_t type;
    uint32_t l; // length of str, element count of array, key/value pair count of map
    union {
        bool b;
        double d;
        int64_t i;
        uint64_t u;
        struct object_t* children;
        char* str; // null-terminated, but l is also the non-terminated length
    };
} object_t;

// Generates a random object with the given arbitrary "size". This should
// somewhat represent "real-world" data.
//
// The resulting object is stored depth-first in one contiguous chunk of
// memory to minimize impact on benchmarks.
//
// (Storing the data contiguously has essentially no effect on fast modern
// computers, but on very low-end devices like a Raspberry Pi, it is
// necessary to properly subtract out the hashing time. Without it, the
// mpack-read test actually beats the hash-object test on RPi! Parsing and
// hashing contiguous encoded data is actually faster than hashing already
// decoded data scattered in memory. Just goes to show how slow memory
// access is on the RPi.)
object_t* object_create(uint64_t seed, int size);

// destroys the object
void object_destroy(object_t* object);


#ifdef __cplusplus
}
#endif

#endif
