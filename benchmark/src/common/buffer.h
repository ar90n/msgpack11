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

#ifndef BENCHMARK_BUFFER_H
#define BENCHMARK_BUFFER_H 1

// some libraries don't have support for writing to a growable
// buffer (e.g. cmp or ubj) so we implement one here.

typedef struct buffer_t {
    char* data;
    size_t count;
    size_t capacity;
} buffer_t;

static void buffer_init(buffer_t* buffer) {
    buffer->count = 0;
    buffer->capacity = 4096;
    buffer->data = (char*)malloc(buffer->capacity);
}

static void buffer_destroy(buffer_t* buffer) {
    free(buffer->data);
}

static bool buffer_write(buffer_t* buffer, const char* data, size_t count) {
    if (buffer->count + count > buffer->capacity) {
        size_t new_capacity = buffer->capacity * 2;
        while (buffer->count + count > new_capacity)
            new_capacity *= 2;
        char* new_data = (char*)realloc(buffer->data, new_capacity);
        if (!new_data)
            return false;
        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }
    memcpy(buffer->data + buffer->count, data, count);
    buffer->count += count;
    return true;
}

#endif
