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

#include "benchmark.h"

#include <sys/stat.h>

// with the below seed:
//   size 2:   2556 bytes MessagePack,   3349 bytes JSON
//   size 4: 187600 bytes MessagePack, 232342 bytes JSON
#define BENCHMARK_OBJECT_SEED 12345678

#define _STR(X) #X
#define STR(X) _STR(X)
#ifndef BENCHMARK_ROOT_PATH
  #define BENCHMARK_ROOT_PATH .
#endif
#define BENCHMARK_DATA_PATH_FORMAT "data/size%s-%i.%s"
#define BENCHMARK_DATA_PATH STR(BENCHMARK_ROOT_PATH)"/"BENCHMARK_DATA_PATH_FORMAT

#define FRAGMENT_MEMORY 1

#define WORK_TIME 10.0              // work for this many seconds
#define WARM_TIME (WORK_TIME / 4.0) // warm up for this many seconds

static double dtime(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / (1000.0 * 1000.0 * 1000.0);
}

// We wrap run function here to ensure it cannot be considered for inlining.
// This is to help prevent the compiler from optimizing away parts of the test.
__attribute__((noinline)) static bool run_wrapper(uint32_t* hash_out) {
    __asm__ (""); // recommended in gcc documentation of noinline
    return run_test(hash_out);
}

object_t* benchmark_object_create(size_t object_size) {
    return object_create(BENCHMARK_OBJECT_SEED, object_size);
}

void benchmark_filename(char* buf, size_t size, size_t object_size, const char* format, const char* config) {
    snprintf(buf, size, BENCHMARK_DATA_PATH, config ? config : "", (int)object_size, format);
}

#if FRAGMENT_MEMORY
#define MEMORY_COUNT 65536
static void* memory[MEMORY_COUNT];
#endif

static void fragment_memory(void) {
    #if FRAGMENT_MEMORY
    // Pre-fragment memory. We allocate a bunch of random-sized blobs,
    // shuffle them and free half of them. This creates a more realistic
    // memory layout, testing how well the library deals with real-world
    // memory usage rather than a nice flat empty malloc().
    // With this random seed, it peaks at about 22 megs before freeing half.
    random_t random;
    random_seed(&random, 34986);
    for (int i = 0; i < MEMORY_COUNT; ++i) {
        size_t bytes = (1 << (random_next(&random) % 12)) + random_next(&random) % 8;
        memory[i] = malloc(bytes);
    }
    for (int i = 0; i < MEMORY_COUNT; ++i) {
        int j = random_next(&random) % (MEMORY_COUNT - i) + i;
        void* l = memory[i];
        memory[i] = memory[j];
        memory[j] = l;
    }
    for (int i = 0; i < MEMORY_COUNT / 2; ++i)
        free(memory[i]);
    #endif
}

static void free_fragmented_memory(void) {
    #if FRAGMENT_MEMORY
    for (int i = MEMORY_COUNT / 2; i < MEMORY_COUNT; ++i)
        free(memory[i]);
    #endif
}

static bool go(bool result_only, size_t object_size, size_t binary_size, const char* name) {

    // setup
    if (!result_only) {
        printf("%s: ================\n", name);
        printf("%s: setting up size %i\n", name, (int)object_size);
    }
    if (!setup_test(object_size)) {
        fprintf(stderr, "%s: failed to get setup result.\n", name);
        return false;
    }

    // if this isn't a benchmark (the file creators), nothing left to do
    if (!is_benchmark()) {
        teardown_test();
        if (!result_only)
            printf("%s: done\n", name);
        return true;
    }

    // figure out a reasonable number of iterations between checking the time
    int iterations;
    #ifdef __arm__
    iterations = 1;
    #else
    iterations = 32;
    #endif
    for (size_t i = 5; i > object_size; --i)
        iterations <<= 3;

    uint32_t hash_result;

    // warm up
    if (!result_only)
        printf("%s: warming for %.0f seconds \n", name, WARM_TIME);
    double start_time = dtime();
    while (true) {
        for (int i = 0; i < iterations; ++i) {
            hash_result = 0;
            if (!run_wrapper(&hash_result)) {
                fprintf(stderr, "%s: failed to get benchmark result.\n", name);
                return false;
            }
        }
        if (dtime() - start_time > WARM_TIME)
            break;
    }

    // run tests
    if (!result_only)
        printf("%s: running for %.0f seconds\n", name, WORK_TIME);
    int total_iterations = 0;
    start_time = dtime();
    double end_time;
    while (true) {
        for (int i = 0; i < iterations; ++i) {
            hash_result = HASH_INITIAL_VALUE;
            if (!run_wrapper(&hash_result)) {
                fprintf(stderr, "%s: failed to get benchmark result.\n", name);
                return false;
            }
            ++total_iterations;
        }
        end_time = dtime();
        if (end_time - start_time > WORK_TIME)
            break;
    }

    // print results
    double per_time = (end_time - start_time) / (double)total_iterations * (1000.0 * 1000.0);
    if (result_only) {
        printf("%f\n", per_time);
    } else {
        printf("%s: %i iterations took %f seconds\n", name, total_iterations, end_time - start_time);
        printf("%s: %f microseconds per iteration\n", name, per_time);
        printf("%s: hash result of last run: %08x\n", name, hash_result);
    }

    // write score
    if (!result_only) {
        FILE* file = fopen("results.csv", "a");
        fprintf(file, "\"%s\",\"%s\",%i,%f,%i,\"%08x\"\n",
                name, test_version(),
                (int)object_size, per_time, (int)binary_size, hash_result);
        fclose(file);
    }

    teardown_test();

    return true;
}

int main(int argc, char **argv) {
    const char* name = argv[0];
    ++argv;
    --argc;

    // argument "-r" will print only the per-iteration time result of the test
    bool result_only = false;
    if (argc >= 1 && strcmp(argv[0], "-r") == 0) {
        result_only = true;
        ++argv;
        --argc;
    }

    // need sizes
    if (argc == 0) {
        fprintf(stderr, "%s: object sizes in the range [1,5] must be "
                "provided as command-line arguments\n", name);
        return EXIT_FAILURE;
    }

    // generate a throwaway object. we do this so that the generator code
    // is included in hash readers (it is necessary for hash-object, so we
    // force all benchmarks to include it so we can subtract the
    // size correctly)
    object_destroy(benchmark_object_create(1));

    // get executable details
    struct stat st;
    stat(name, &st);
    size_t binary_size = st.st_size;
    static const char* build = "build/";
    if (strlen(name) > strlen(build) && memcmp(name, build, strlen(build)) == 0)
        name += strlen(build);
    if (!result_only)
        printf("%s: executable size: %i bytes\n",     name, (int)binary_size);

    // run different benchmark sizes
    fragment_memory();
    for (; argc > 0; --argc, ++argv) {

        size_t object_size = atoi(argv[0]);
        if (object_size < 1 || object_size > 5) {
            fprintf(stderr, "%s: object size must be in the range [1,5]\n", name);
            return EXIT_FAILURE;
        }

        if (!go(result_only, object_size, (int)st.st_size, name))
            return EXIT_FAILURE;
    }
    free_fragmented_memory();
    return EXIT_SUCCESS;
}

static char* load_file(const char* filename, size_t* size_out) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "missing file!\n");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* data = (char*)malloc(size);

    if (size != fread(data, 1, size, file)) {
        fprintf(stderr, "error reading file!\n");
        return NULL;
    }
    fclose(file);

    *size_out = size;
    return data;
}

char* load_data_file(const char* format, size_t object_size, size_t* size_out) {
    return load_data_file_ex(format, object_size, size_out, NULL);
}

char* load_data_file_ex(const char* format, size_t object_size, size_t* size_out, const char* config) {
    char filename[128];
    benchmark_filename(filename, sizeof(filename), object_size, format, config);
    printf("%s\n", filename);
    return load_file(filename, size_out);
}

