list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/catch")
find_package(Catch2 REQUIRED)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/glog")
find_package(glog REQUIRED)