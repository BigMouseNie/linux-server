cmake_minimum_required(VERSION 3.11)

project(playback-node LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
    base
    GIT_REPOSITORY https://github.com/BigMouseNie/base.git
    GIT_TAG        v1.0.0
)

FetchContent_MakeAvailable(base)

add_subdirectory(container)
add_subdirectory(run)
add_subdirectory(network)
add_subdirectory(logger)

add_executable(playback-node main.cpp)

target_link_libraries(playback-node PRIVATE
    base container run network logger
)
