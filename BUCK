include_defs('//BUCKAROO_DEPS')

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
  ],
  visibility = [
    'PUBLIC',
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
  ],
  visibility = [
    'PUBLIC',
  ],
  deps = ([ ':msgpack11' ] + BUCKAROO_DEPS),
)
