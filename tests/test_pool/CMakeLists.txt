add_executable(test_pool
  test_thread_pool.cpp
)

target_link_libraries(test_pool
    PRIVATE
    pool
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_pool)
