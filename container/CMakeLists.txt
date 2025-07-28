add_library(container STATIC
    src/ring_buffer.cpp
)

target_include_directories(container PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
