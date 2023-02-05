cmake_minimum_required(VERSION 3.21)
project(huffman)

set(CMAKE_CXX_STANDARD 17)

find_package(GTest REQUIRED)
find_path(ARGAGG_INCLUDE_DIRS "argagg/argagg.hpp")

add_executable(tests unit-tests/tests.cpp)
add_library(huffman STATIC huffman-lib/huffman.cpp)
add_executable(huffman-tool tool.cpp)

if (NOT MSVC)
  target_compile_options(tests PRIVATE -Wall -Wno-sign-compare -pedantic)
endif()

option(USE_SANITIZERS "Enable to build with undefined,leak and address sanitizers" OFF)
if (USE_SANITIZERS)
  target_compile_options(huffman PUBLIC -fsanitize=address,undefined,leak -fno-sanitize-recover=all)
  target_link_options(huffman PUBLIC -fsanitize=address,undefined,leak)
  target_compile_options(tests PUBLIC -fsanitize=address,undefined,leak -fno-sanitize-recover=all)
  target_link_options(tests PUBLIC -fsanitize=address,undefined,leak)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(tests PUBLIC -stdlib=libc++)
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
  target_compile_options(tests PUBLIC -D_GLIBCXX_DEBUG)
endif()

target_link_libraries(tests GTest::gtest GTest::gtest_main huffman)
target_link_libraries(huffman-tool huffman)
target_include_directories(huffman-tool PRIVATE ${ARGAGG_INCLUDE_DIRS})
