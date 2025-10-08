add_executable(test_http
    test_http_parser_wrap.cpp
    test_uri_parser.cpp
)

target_link_libraries(test_http
    PRIVATE
    http
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_http)
