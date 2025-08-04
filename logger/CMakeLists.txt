add_library(logger STATIC
    src/logger.cpp
)

target_include_directories(logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(logger PUBLIC network run)

target_link_libraries(logger PRIVATE nlohmann_json::nlohmann_json)
