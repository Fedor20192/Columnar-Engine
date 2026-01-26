project(ColumnExecutables)

add_executable(create_table CreateTable.cpp)
target_include_directories(create_table PUBLIC ${CMAKE_CURRENT_DIR})
target_link_libraries(create_table glog::glog columnar_engine)

add_executable(extract_table ExtractTable.cpp)
target_include_directories(extract_table PUBLIC ${CMAKE_CURRENT_DIR})
target_link_libraries(extract_table glog::glog columnar_engine)