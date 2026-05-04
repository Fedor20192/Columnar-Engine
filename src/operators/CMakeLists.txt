project(columnar_operator)

set(SOURCES Scan.cpp Expression.cpp ExpressionCore.cpp Aggregation.cpp)
set(HEADERS Operator.h Scan.h Count.h Filter.h Expression.h Aggregation.h)

add_library(columnar_operator STATIC ${SOURCES} ${HEADERS})
target_include_directories(columnar_operator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(columnar_operator columnar_kernel glog::glog)