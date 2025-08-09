cmake_minimum_required(VERSION 3.28)

project(linux-server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(container)
add_subdirectory(run)
add_subdirectory(network)
add_subdirectory(logger)
add_subdirectory(pool)
add_subdirectory(business)
add_subdirectory(http)
add_subdirectory(database)
add_subdirectory(encryption)

add_executable(linux-server main.cpp)

target_link_libraries(linux-server PRIVATE
    container run network
    logger pool business
    nlohmann_json::nlohmann_json
    http database encryption
)
