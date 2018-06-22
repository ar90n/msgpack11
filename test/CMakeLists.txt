FIND_PACKAGE (GTest REQUIRED)
FIND_PACKAGE (ZLIB REQUIRED)
FIND_PACKAGE (Threads REQUIRED)

INCLUDE_DIRECTORIES (
    ${GTEST_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
)

LIST (APPEND check_PROGRAMS
     array.cpp
     basic.cpp
     raw.cpp
     incomplete_data.cpp
     object.cpp
     multi.cpp
)

SET (MSGPACK_TEST_LIB msgpack11)

FOREACH (source_file ${check_PROGRAMS})
    GET_FILENAME_COMPONENT (source_file_we ${source_file} NAME_WE)
    ADD_EXECUTABLE (
        ${source_file_we}
        ${source_file}
    )
    TARGET_LINK_LIBRARIES (${source_file_we}
        ${MSGPACK_TEST_LIB}
        ${GTEST_BOTH_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
    )
    ADD_TEST (${source_file_we} ${source_file_we})
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        SET_PROPERTY (TARGET ${source_file_we} APPEND_STRING PROPERTY COMPILE_FLAGS "-Wall -Wextra -Werror -g -O3 ")
    ENDIF ()
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        SET_PROPERTY (TARGET ${source_file_we} APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-mismatched-tags")
    ENDIF ()

    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        IF (CMAKE_CXX_FLAGS MATCHES "/W[0-4] /WX")
            STRING(REGEX REPLACE "/W[0-4]" "/W3 /WX" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        ELSE ()
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
        ENDIF ()
    ENDIF ()
ENDFOREACH ()
