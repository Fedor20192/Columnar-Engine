cmake_minimum_required(VERSION 3.25)
project(ColumnarEngine CXX)

include(cmake/compile_options.cmake)
include(cmake/dependencies.cmake)

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)