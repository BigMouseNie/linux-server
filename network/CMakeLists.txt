add_library(network STATIC
    src/socket_buffer.cpp
    src/socket_creator.cpp
    src/epoller.cpp
    src/socket_wrapper.cpp
    src/acceptor.cpp
    src/reactor_acc.cpp
)

target_include_directories(network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(network PUBLIC container)
