try:
  include_defs('//BUCKAROO_DEPS')
except:
  BUCKAROO_DEPS = ['//gtest:googletest']

with allow_unsafe_import():
  from os import path
path_to_root = path.dirname(path.realpath(__file__))

cxx_library(
  name = 'gtest',
  header_namespace = '',
  srcs = [
    'googletest/src/gtest-all.cc',
    'googletest/src/gtest_main.cc',
  ],
  headers = subdir_glob([
    ('googletest', 'src/*.h'),
    ('googletest', 'src/*.cc'),
    ('googletest/include', 'internal/**/*.h'),
  ]),
  exported_headers = subdir_glob([
    ('googletest/include', '**/*.h'),
  ], excludes = [
    'googletest/include/internal/**/*.h',
  ]),
  preprocessor_flags = [
    '-U_STRICT_ANSI_',
  ],
  compiler_flags = [
    '-std=c++14',
  ],
  platform_linker_flags = [
    ('android', []),
    ('', ['-lpthread']),
  ],
  visibility = [
    'PUBLIC',
  ],
)

cxx_library(
  name = 'msgpack11',
  header_namespace = '',
  srcs = [
    './msgpack11.cpp',
  ],
  headers = [
    './msgpack11.hpp',
  ],
  exported_headers = [
    './msgpack11.hpp',
  ],
  compiler_flags = [
    '-std=c++11',
    '-fno-rtti',
    '-Wall',
    '-Wextra',
    '-Werror',
    '-O2',
  ],
  visibility = [
    'PUBLIC',
  ],
)

cxx_binary(
  name = 'msgpack11-example',
  srcs = [
    './example.cpp'
  ],
  compiler_flags = [
    '-std=c++11',
    '-O2'
  ],
  visibility = [
    'PUBLIC',
  ],
  deps = [
    ':msgpack11'
  ],
)

cxx_test(
  name = 'msgpack11-test',
  srcs = [
    'test/array.cpp',
    'test/basic.cpp',
    'test/multi.cpp',
    'test/object.cpp',
    'test/raw.cpp'
  ],
  compiler_flags = [
    '-std=c++11',
    '-O2'
  ],
  visibility = [
    'PUBLIC',
  ],
  deps = ([ ':msgpack11' ] + BUCKAROO_DEPS),
)

remote_file(
  name = 'msgpack-c-src',
  url = 'https://github.com/msgpack/msgpack-c/archive/cpp-2.1.4.zip',
  sha1 = '289ff076fb7f26ae30d5624223da8182469b3cc1',
  type = 'exploded_zip',
)

genrule(
  name = 'msgpack-c-build',
  cmd = 'install -d $(location :msgpack-c-src)/msgpack-c-cpp-2.1.4/build && \
         cd $(location :msgpack-c-src)/msgpack-c-cpp-2.1.4/build && \
         cmake -DCMAKE_INSTALL_PREFIX=$OUT .. && \
         make && \
         make install',
  out = '.',
)

prebuilt_cxx_library(
  name = 'msgpackc',
  visibility = [ 'PUBLIC', ],
  lib_dir = '$(location :msgpack-c-build)/lib',
  include_dirs = ['$(location :msgpack-c-build)/include'],
)

cxx_library(
  name = 'msgpack-c',
  visibility = [ 'PUBLIC', ],
  link_style = 'static',
  deps = [
    ':msgpackc',
  ]
)

cxx_library(
  name = 'benchmark-common',
  header_namespace = '',
  srcs = [
    './benchmark/src/common/benchmark.c',
    './benchmark/src/common/generator.c',
  ],
  compiler_flags = [
    '-DBENCHMARK_ROOT_PATH=' + path.join(path_to_root,'benchmark'),
    '-O2'
  ],
  exported_headers = subdir_glob([
    ('benchmark/src/common', '*.h')
  ]),
  visibility = [ 'PUBLIC', ],
)

cxx_binary(
  name = 'msgpack-c-unpack',
  srcs = [
    './benchmark/src/msgpack-c-unpack.cpp'
  ],
  compiler_flags = [
    '-std=c++11',
    '-O2'
  ],
  visibility = [ 'PUBLIC' ],
  link_style = 'static',
  deps = [
    ':msgpack-c',
    ':benchmark-common'
  ]
)

cxx_binary(
  name = 'msgpack-c-pack',
  srcs = [
    './benchmark/src/msgpack-c-pack.cpp'
  ],
  compiler_flags = [
    '-std=c++11',
    '-O2'
  ],
  visibility = [ 'PUBLIC' ],
  link_style = 'static',
  deps = [
    ':msgpack-c',
    ':benchmark-common'
  ]
)

cxx_binary(
  name = 'msgpack11-unpack',
  srcs = [
    './benchmark/src/msgpack11-unpack.cpp'
  ],
  compiler_flags = [
    '-std=c++11',
    '-O2',
  ],
  visibility = [ 'PUBLIC' ],
  link_style = 'static',
  deps = [
    ':msgpack11',
    ':benchmark-common'
  ]
)

cxx_binary(
  name = 'msgpack11-pack',
  srcs = [
    './benchmark/src/msgpack11-pack.cpp'
  ],
  compiler_flags = [
    '-std=c++11',
    '-O2'
  ],
  visibility = [ 'PUBLIC' ],
  link_style = 'static',
  deps = [
    ':msgpack11',
    ':benchmark-common'
  ]
)

cxx_binary(
  name = 'hash-data',
  srcs = [
    './benchmark/src/hash/hash-data.c'
  ],
  compiler_flags = [
    '-O2'
  ],
  visibility = [ 'PUBLIC' ],
  link_style = 'static',
  deps = [
    ':benchmark-common'
  ]
)

cxx_binary(
  name = 'hash-object',
  srcs = [
    './benchmark/src/hash/hash-object.c'
  ],
  compiler_flags = [
    '-O2'
  ],
  visibility = [ 'PUBLIC' ],
  link_style = 'static',
  deps = [
    ':benchmark-common'
  ]
)

genrule(
  name = 'benchmark',
  cmd = 'cd $OUT &&\
         $(exe :hash-data) 1 2 3 4 5 && \
         $(exe :hash-object) 1 2 3 4 5 &&\
         for i in 1 2 3 4 5; do $(exe :msgpack-c-unpack) 1 2 3 4 5 ; done &&\
         for i in 1 2 3 4 5; do $(exe :msgpack-c-pack) 1 2 3 4 5 ; done &&\
         for i in 1 2 3 4 5; do $(exe :msgpack11-unpack) 1 2 3 4 5 ; done &&\
         for i in 1 2 3 4 5; do $(exe :msgpack11-pack) 1 2 3 4 5 ; done &&\
         $SRCDIR/benchmark/tools/results.py > {output} &&\
         echo -n "Git revision : " >> {output} &&\
         git rev-parse HEAD >> {output}'.format(output=path.join(path_to_root, 'results.md')),
  srcs = [
    ':msgpack-c-unpack',
    ':msgpack-c-pack',
    ':msgpack11-unpack',
    ':msgpack11-pack',
    ':hash-data',
    ':hash-object',
    './benchmark/tools/results.py'
  ],
  out = '.',
)
