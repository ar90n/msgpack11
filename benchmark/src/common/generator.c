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

#include "generator.h"

int cmp(const void *p, const void *q) {
     char const *pp = *(char const **)p;
     char const *qq = *(char const **)q;
     return strcmp(pp, qq);
}

uint32_t random_next(random_t* random) {
    static const uint64_t a = UINT64_C(3636507990);
    uint64_t r = (a * random->v) + random->c;
    random->v = (uint32_t)((0xffffffff - 1 - r) & 0xffffffff);
    random->c = (uint32_t)(r >> 32);
    return random->v;
}

void random_seed(random_t* random, uint64_t val) {
    // we force a few bits just to make sure these values are not 0 or UINT_MAX-1
    random->v = (val & 0xfffffffe) | 0x0000100;
    random->c = ((val >> 32) & 0xfffeffff) | 0x0100000;
}

// generates a random number with (approximately) inverse distribution
// up to (approximately) the given max, so we get lots of small numbers
// and a few big ones
uint32_t random_inverse(random_t* random, uint32_t max) {
    if (max == 0)
        return max;
    if (max == 1)
        return random_next(random) & 1;

    uint32_t bits = 1;
    uint32_t counter = max;
    while ((counter >>= 1))
        ++bits;

    uint32_t div = (random_next(random) % (1 << (bits * 2 / 3))) + 1;
    uint32_t ret = random_next(random) % (max / div);
    return ret + (random_next(random) % (1 << (bits / 3)));
}



// Some random object generation functions

// generates short lowercase ascii keys, realistic for real-world data
// (null-terminated for certain APIs that may require it)
static char* random_key(random_t* random) {
    uint32_t length = random_next(random) % 9 + 2;
    char* str = (char*)malloc(length + 1);
    for (int i = 0; i < length; ++i)
        str[i] = 'a' + (random_next(random) % ('z' - 'a' + 1));
    str[length] = '\0';
    return str;
}

// generates a random string of the given length (null-terminated for
// certain APIs that may require it)
static char* random_string(random_t* random, uint32_t length) {
    char* str = (char*)malloc(length + 1);

    // we'll assume most non-key strings don't have non-ascii characters
    // (all key strings are generated as lowercase ascii letters)
    bool ascii = random_next(random) % 4 != 0;

    // a string might have either lots of spaces (words) or
    // no spaces (miscellaneous small data, urls, etc.)
    bool spaces = length > 50 || (random_next(random) % 4) != 0;
    int next_space = (random_next(random) % 8) + 2;

    for (int i = 0; i < length; ++i) {

        // lots of spaces
        if (spaces && next_space-- == 0) {
            next_space = (random_next(random) % 8) + 2;
            str[i] = ' ';
            continue;
        }

        // rarely, generate a character that might need to be escaped
        if (random_next(random) % 128 == 0) {
            char specials[] = {'\n', '"', '\\'};
            str[i] = specials[random_next(random) % sizeof(specials)];
            continue;
        }

        // generate a utf-8 non-ascii character (for now a
        // character from Latin-1 supplement)
        if (!ascii && (length - i) >= 2 && random_next(random) % 4 == 0) {
            uint32_t codepoint = 0xA1 + random_next(random) % 0x5F;
            str[i++] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
            str[i]   = (char)(0x80 | (codepoint & 0x3F));
            continue;
        }

        // sometimes give us any ascii character (this will add a few
        // more quotes and backslashes)
        if (random_next(random) % 32 == 0) {
            str[i] = 33 + random_next(random) % 94;
            continue;
        }

        // sometimes give us a capital letter, but usually lowercase.
        str[i] = 'a' + (random_next(random) % ('z' - 'a' + 1));
        if (random_next(random) % 32 == 0)
            str[i] -= 'a' - 'A';
    }

    str[length] = '\0';
    return str;
}

static type_t random_type(random_t* random, int size, int depth, uint32_t* length) {
    *length = 0;

    // the odds of a map or array are proportional to the depth. at
    // the base depth it's always one or the other.
    int odds = 2;
    int d = depth;
    while (d-- > 0)
        odds <<= 1;
    if (depth < 31 && random_next(random) % odds <= 2) {
        type_t type = (random_next(random) & 1) ? type_map : type_array;

        // generate a random length close to the size
        int len = 3;
        while (size-- > depth)
            len *= 2;
        len += random_next(random) % len;
        if (type == type_map)
            len /= 2;
        *length = len;
        return type;
    }

    // reals are probably pretty rare
    if (random_next(random) % 64 == 0)
        return type_double;

    // the rest we distribute with a simple switch
    switch (random_next(random) % 8) {

        case 0:
            return type_nil;
        case 1:
            return type_bool;
        case 3:
            return type_uint;
        case 4:
        case 5:
        case 6:
            return type_int;
        case 7:
            // we get plenty of short strings as map keys. we'll
            // take 1/8 map/array values as potentially long strings
            break;

        default:
            break;
    }

    *length = random_inverse(random, 1000);
    return type_str;
}

// expand the given size to preserve object alignment
static size_t object_align(size_t size) {
    const size_t alignment = __alignof__(object_t);
    return (size + alignment - 1) & (~(alignment-1));
}

static void object_init(object_t* object, random_t* random, int size, int depth, size_t* total_size) {
    uint32_t length;
    object->type = random_type(random, size, depth, &length);

    switch (object->type) {
        case type_bool:
            object->b = random_next(random) & 1;
            break;

        case type_double: {
            // we should probably make it possible to generate nan and infinity here
            object->d = (double)((int)(random_next(random) % 2048) - 1024);
            // we add lots of mantissa to try to use the full range of doubles
            object->d += (double)(random_next(random) % 1024) / 1024.0;
            object->d += (double)(random_next(random) % 1024) / (1024.0 * 1024.0);
            object->d += (double)(random_next(random) % 1024) / (1024.0 * 1024.0 * 1024.0);
            object->d += (double)(random_next(random) % 1024) / (1024.0 * 1024.0 * 1024.0 * 1024.0);
            object->d += (double)(random_next(random) % 1024) / (1024.0 * 1024.0 * 1024.0 * 1024.0 * 1024.0);
            break;
        }

        // sometimes numbers are huge, and we want to test 64-bit. but
        // usually they're very small.
        case type_uint:
            if (random_inverse(random, 10000) > 5000) {
                // note: we don't allow numbers in the range [INT64_MAX, UINT64_MAX)
                object->u = ((uint64_t)(random_next(random) & ~(1<<31))) << 32;
                object->u |= (uint64_t)random_next(random);
            } else {
                object->u = random_inverse(random, 0xfffff);
            }
            break;
        case type_int:
            if (random_inverse(random, 10000) > 5000) {
                uint64_t u = ((uint64_t)random_next(random)) << 32;
                u |= (uint64_t)random_next(random);
                object->i = (int64_t)u;
            } else {
                object->i = random_inverse(random, 0xfffff);
                object->i *= (random_next(random) & 1) ? -1 : 1;
            }
            break;

        case type_str:
            *total_size += object_align(length + 1);
            object->l = length;
            object->str = random_string(random, length);
            break;

        case type_array:
            *total_size += object_align(length * sizeof(object_t));
            object->l = length;
            object->children = (object_t*)malloc(length * sizeof(object_t));
            for (int i = 0; i < length; ++i)
                object_init(object->children + i, random, size, depth + 1, total_size);
            break;

        case type_map:
            *total_size += object_align(2 * length * sizeof(object_t));
            object->l = length;
            object->children = (object_t*)malloc(2 * length * sizeof(object_t));

            char** keys = (char**)malloc(sizeof(char*) * length);
            for (int i = 0; i < length;) {
                bool unique = true;
                char* key = random_key(random);
                for (int j = 0; j < i; ++j) {
                    if (strcmp(key, keys[j]) == 0) {
                        unique = false;
                        break;
                    }
                }

                if(unique)
                {
                    keys[i] = key;
                    ++i;
                }
                else
                {
                    free(key);
                }
            }
            qsort( keys, length, sizeof(char*), cmp );

            for (int i = 0; i < length; ++i) {
                // maps must have string keys for json compatibility. they are
                // realistically always short lowercase ascii text.
                object->children[i * 2].type = type_str;
                object->children[i * 2].str = keys[i];
                object->children[i * 2].l = strlen(object->children[i * 2].str);
                *total_size += object_align(object->children[i * 2].l + 1);

                object_init(object->children + i * 2 + 1, random, size, depth + 1, total_size);
            }
            free(keys);
            break;

        default:
            break;
    }
}

static void object_teardown(object_t* object) {
    if (object->type == type_str) {
        free(object->str);
    } else if (object->type == type_map) {
        for (int i = 0; i < object->l * 2; ++i)
            object_teardown(object->children + i);
        free(object->children);
    } else if (object->type == type_array) {
        for (int i = 0; i < object->l; ++i)
            object_teardown(object->children + i);
        free(object->children);
    }
}

static char* object_copy(object_t* dest, object_t* src, char* pool) {
    *dest = *src;

    if (src->type == type_map) {
        dest->children = (object_t*)pool;
        pool += object_align(sizeof(object_t) * dest->l * 2);
        for (size_t i = 0; i < src->l * 2; ++i)
            pool = object_copy(dest->children + i, src->children + i, pool);

    } else if (dest->type == type_array) {
        dest->children = (object_t*)pool;
        pool += object_align(sizeof(object_t) * dest->l);
        for (size_t i = 0; i < src->l; ++i)
            pool = object_copy(dest->children + i, src->children + i, pool);

    } else if (src->type == type_str) {
        dest->str = pool;
        memcpy(dest->str, src->str, src->l + 1);
        pool += object_align(src->l + 1);
    }

    return pool;
}

object_t* object_create(uint64_t seed, int size) {
    random_t random;
    random_seed(&random, seed);

    // first we create an object, tracking its total size
    object_t* src = (object_t*)malloc(sizeof(object_t));
    size_t total_size = object_align(sizeof(object_t));
    object_init(src, &random, size, 0, &total_size);

    // next we allocate a contiguous chunk of memory and copy
    // the object into it
    char* pool = (char*)malloc(total_size);
    object_t* dest = (object_t*)pool;
    pool += object_align(sizeof(object_t));
    object_copy(dest, src, pool);

    object_teardown(src);
    free(src);
    return dest;
    /*
    (void)object_teardown;
    return src;
    */
}

void object_destroy(object_t* object) {
    // the external object is a flat array of data
    free(object);
}

#if 0
int main(void) {
    random_t random;
    random_seed(&random, 4);

#if 0
    for (int i = 0; i < 500; ++i) {
        printf("%4u ", random_inverse(&random, 1000));
        if (i % 10 == 9)
            printf("\n");
    }
    printf("\n");
#endif

#if 0
    for (int i = 0; i < 50; ++i) {
        //char* str = random_key(&random);
        char* str = random_string(&random, random_inverse(&random, 1000));
        printf("%s\n", str);
        free(str);
    }
#endif

#if 0
    for (int i = 0; i < 50; ++i) {
    }
#endif
}
#endif
