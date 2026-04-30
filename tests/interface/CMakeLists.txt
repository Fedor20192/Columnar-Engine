include(Catch)

add_executable(test_converter Converter.cpp)
target_link_libraries(test_converter Catch2::Catch2WithMain columnar_engine)

catch_discover_tests(test_converter)