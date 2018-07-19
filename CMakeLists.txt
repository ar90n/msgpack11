cmake_minimum_required(VERSION 3.2)
project(msgpack11 VERSION 0.0.9 LANGUAGES CXX C)

option(MSGPACK11_BUILD_TESTS "Build unit tests" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(msgpack11 msgpack11.cpp)
target_include_directories(msgpack11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(msgpack11 PRIVATE -fno-rtti)
if(NOT MSVC)
  target_compile_options(msgpack11 PRIVATE -Wall -Wextra -Werror)
endif()
configure_file("msgpack11.pc.in" "msgpack11.pc" @ONLY)

if (MSGPACK11_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

install(TARGETS msgpack11 DESTINATION lib)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/msgpack11.hpp" DESTINATION include)
install(FILES "${CMAKE_BINARY_DIR}/msgpack11.pc" DESTINATION lib/pkgconfig)
