include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(test_container)
# add_subdirectory(test_http)
add_subdirectory(test_network)
add_subdirectory(test_pool)
add_subdirectory(test_run)
