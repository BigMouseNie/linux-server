add_library(pool STATIC
    src/thread_pool.cpp
)

target_include_directories(pool PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pool PRIVATE
    run container network
)
