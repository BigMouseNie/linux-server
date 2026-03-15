add_executable(test_network
  test_socket_wrap.cpp
  test_socket_buffer.cpp
  test_acceptor.cpp
  test_epoller.cpp
)

target_link_libraries(test_network
    PRIVATE
    network
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_network)
