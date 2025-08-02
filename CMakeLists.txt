cmake_minimum_required(VERSION 3.11)

project(playback-node LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(container)
add_subdirectory(run)
add_subdirectory(network)
add_subdirectory(logger)
add_subdirectory(pool)

add_executable(playback-node main.cpp)

target_link_libraries(playback-node PRIVATE
    container run network logger pool
)
