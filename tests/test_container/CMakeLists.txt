add_executable(test_container test_ring_buffer.cpp)

target_link_libraries(test_container
    PRIVATE
    container
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_container)
