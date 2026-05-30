list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/catch")
find_package(Catch2 REQUIRED)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/glog")
find_package(glog REQUIRED)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/hash-map")
find_package(unordered_dense CONFIG REQUIRED)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/libs/re")
find_package(re2 REQUIRED)
