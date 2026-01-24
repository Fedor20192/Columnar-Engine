project(columnar_kernel)

set(SOURCES BatchedCsvReader.cpp BatchedCsvWriter.cpp)
set(HEADERS BatchedCsvReader.h BatchedCsvWriter.h)

add_library(columnar_kernel STATIC ${SOURCES} ${HEADERS})
target_include_directories(columnar_kernel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(columnar_kernel glog::glog csv_handler)