project(CNGN_INTERFACE)
set(SOURCES CHSVConverter.cpp)
set(HEADERS CHSVConverter.h)
add_library(columnar_engine STATIC ${SOURCES} ${HEADERS})
target_include_directories(columnar_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(columnar_engine glog::glog columnar_kernel csv_handler)

add_executable(create_table CreateTable.cpp)
target_include_directories(create_table PUBLIC ${CMAKE_CURRENT_DIR})
target_link_libraries(create_table glog::glog columnar_engine)

add_executable(extract_table ExtractTable.cpp)
target_include_directories(extract_table PUBLIC ${CMAKE_CURRENT_DIR})
target_link_libraries(extract_table glog::glog columnar_engine)