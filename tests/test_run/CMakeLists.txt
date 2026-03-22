add_executable(test_run
    test_process_wrap.cpp
)

target_link_libraries(test_run
    PRIVATE
    run
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_run)
