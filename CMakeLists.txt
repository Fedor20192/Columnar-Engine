cmake_minimum_required(VERSION 3.25)
project(ColumnarEngine)

include(cmake/compile_options.cmake)
include(cmake/dependencies.cmake)

find_program(CLANG_TIDY_EXE clang-tidy)
set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE};)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)