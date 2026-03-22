add_executable(test_logger
    test_logger.cpp
)

target_link_libraries(test_logger
    PRIVATE
    logger
    GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_logger)
