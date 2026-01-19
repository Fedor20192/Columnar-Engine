cmake_minimum_required(VERSION 3.25)
project(ColumnarEngine)

include(cmake/compile_options.cmake)
include(cmake/dependencies.cmake)

# set(CMAKE_CXX_CLANG_TIDY .clang-tidy)

add_subdirectory(src)
add_subdirectory(tests)

# Пример, как подключить catch2 к цели
# add_executable(main main.cpp)
# target_link_libraries(main Catch2::Catch2WithMain)