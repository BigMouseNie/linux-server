add_executable(test_socket_wrap
  test_socket_wrap.cpp
)

target_include_directories(test_socket_wrap
  PRIVATE
  ${CMAKE_SOURCE_DIR}/network/include
)

target_link_libraries(test_socket_wrap
  PRIVATE
  GTest::gtest
  GTest::gtest_main
  pthread
)

add_test(NAME test_socket_wrap COMMAND test_socket_wrap)
