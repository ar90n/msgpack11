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

#ifndef BENCHMARK_HASH_H
#define BENCHMARK_HASH_H 1

/*
 * This is a trivial multiplicative hash. All output data of a benchmark test
 * are hashed to ensure it was serialized correctly, to simulate accessing the
 * data, and to ensure that no parts of it were optimized away.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define HASH_INITIAL_VALUE 15373

#if 0
// debugger to print the first few hash values. use it to
// debug why a new decoder's hash doesn't match
static void hash_p(const char* s, uint32_t hash, uint32_t val) {
    if (hash == 1) {
        printf("BREAK!\n");
    }
    static int i = 0;
    if (i < 20) {
        i++;
        printf("hashing into %08x (%s %u)\n", hash, s, val);
    }
}
#else
#define hash_p(...) /* nothing */
#endif

static inline uint32_t hash_u32(uint32_t hash, uint32_t val) {
    hash_p("u32", hash, val);
    return hash * 31 ^ val;
}

static inline uint32_t hash_u64(uint32_t hash, uint64_t val) {
    return hash_u32(hash_u32(hash, (uint32_t)(val >> 32)), (uint32_t)(val));
}

static inline uint32_t hash_u8(uint32_t hash, uint8_t val) {return hash_u32(hash, val);}
static inline uint32_t hash_u16(uint32_t hash, uint16_t val) {return hash_u32(hash, val);}

static inline uint32_t hash_i8 (uint32_t hash, int8_t  val) {return hash_u8 (hash, (uint8_t) val);}
static inline uint32_t hash_i16(uint32_t hash, int16_t val) {return hash_u16(hash, (uint16_t)val);}
static inline uint32_t hash_i32(uint32_t hash, int32_t val) {return hash_u32(hash, (uint32_t)val);}
static inline uint32_t hash_i64(uint32_t hash, int64_t val) {return hash_u64(hash, (uint64_t)val);}

static inline uint32_t hash_bool(uint32_t hash, bool val) {
    uint32_t i = val ? 1 : 0;
    return hash_u32(hash, i);
}

static inline uint32_t hash_double(uint32_t hash, double val) {
    // to avoid floating point differences between different parsers and
    // architectures, we skip over floats. there are very few floats in
    // the data anyway. instead we just mix in a prime.
    return hash_u32(hash, 43013);
}

static inline uint32_t hash_float(uint32_t hash, float val) {
    return hash_double(hash, val);
}

static inline uint32_t hash_str(uint32_t hash, const char* str, size_t len) {
    hash_p("str", hash, len);
    hash = hash_u32(hash, (uint32_t)len);

    // the string is hashed as a series of little-endian uint32, zero-padded.

    // hash every four bytes together
    const unsigned char* ustr = (const unsigned char*)str;
    for (; len >= 4; len -= 4) {
        uint32_t val = (((uint32_t)ustr[3]) << 24) | (((uint32_t)ustr[2]) << 16) |
            (((uint32_t)ustr[1]) << 8) | (((uint32_t)ustr[0]) << 0);
        hash = hash_u32(hash, val);
        ustr += 4;
    }

    // hash the remaining 0-3 bytes
    uint32_t val = 0;
    switch (len) {
        case 3:
            val |= ((uint32_t)ustr[2]) << 16;
            // fallthrough
        case 2:
            val |= ((uint32_t)ustr[1]) << 8;
            // fallthrough
        case 1:
            val |= ((uint32_t)ustr[0]);
            hash = hash_u32(hash, val);
            break;
        case 0:
            break;
        default:
            #if defined(__GNUC__) || defined(__clang__)
            __builtin_unreachable();
            #endif
            break;
    }

    hash_p("strdone", hash, 0);
    return hash;
}

static inline uint32_t hash_nil(uint32_t hash) {
    // don't use a simple number or string so that it can't
    // give the same hash as any other simple type
    return hash_str(hash_u32(hash, 0), "nil", 3);
}

#ifdef __cplusplus
}
#endif

#endif

