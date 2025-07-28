add_library(network STATIC
    src/epoller.cpp
    src/socket_wrapper.cpp
    src/acceptor.cpp
)

target_include_directories(network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
